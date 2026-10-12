# GPIO 外部中断实验 —— 工程编程说明

## 一、项目概述

本项目实现 **GPIO 外部中断（EXTI）实验**：在 **NUCLEO-C542RC**（搭载 **STM32C542RCT6**）开发板上，按下用户按键 **B1（PC13）** 触发 **外部中断**，在中断中切换板载 LED **LD1（PA5）** 的状态（点亮 ⇄ 熄灭）。

**核心行为：**
- 按下一次 → 触发 EXTI 中断 → LED 状态翻转一次；
- **松开不触发**（松开对应下降沿，本实验仅配置上升沿触发）；
- 中断回调内置 **20ms 消抖**，避免按键机械抖动导致的误触发。

---

## 二、硬件说明

### 2.1 引脚定义

| 引脚 | 信号 | 功能 | 有效电平 |
|------|------|------|----------|
| PA5 | LD1 | 板载 LED，输出模式 | **高电平（SET）点亮** |
| PC13 | B1 | 用户按键，输入模式（外部下拉，按下接 3V3） | **按下为高电平（SET）** |

> 本工程硬件上 PC13 通过外部电阻**下拉到地**，常态为低电平；**按下时经按键接通 3V3，读出为高电平**，即“按下=高、松开=低”。因此一次“按下”对应的是一次**上升沿**（由低变高），EXTI 中断触发方式配置为**上升沿触发**。

### 2.2 引脚与宏对照表

工程由 CubeMX 生成代码，引脚名在 [generated/hal/mx_gpio_default.h](generated/hal/mx_gpio_default.h) 中定义了别名宏，**不要在 main.c 里写死 `HAL_GPIO_PIN_5` / `HAL_GPIO_PIN_13`**，应使用下列宏以保持可移植性：

| 宏 | 值 | 说明 |
|----|----|------|
| `LD1_PORT` | `HAL_GPIOA` | LD1 所在 GPIO 口（端口 A） |
| `LD1_PIN` | `HAL_GPIO_PIN_5` | LD1 引脚号 5（PA5） |
| `B1_PORT` | `HAL_GPIOC` | B1 所在 GPIO 口（端口 C） |
| `B1_PIN` | `HAL_GPIO_PIN_13` | B1 引脚号 13（PC13） |
| `LD1_INIT_STATE` | `HAL_GPIO_PIN_RESET` | LD1 初始状态（熄灭） |
| `LD1_ACTIVE_STATE` | `HAL_GPIO_PIN_SET` | LD1 点亮电平（高） |

这些宏通过 `main.h` → `mx_hal_def.h` → `mx_gpio_default.h` 的包含链自动可见，因此在 [main.c](main.c) 中可直接使用。

---

## 三、功能需求

1. **外部中断触发**：PC13（B1）按下时产生一次上升沿，触发 EXTI13 中断。
2. **有效按下翻转**：每次有效触发，翻转 PA5（LD1）状态。
3. **松开不触发**：松开（下降沿）不在触发范围内，不会翻转。
4. **消抖 20ms**：两次触发间隔不足 20ms 视为抖动，直接忽略。
5. **非阻塞消抖**：消抖基于 `HAL_GetTick()` 比较时间差，不在中断里忙等。

---

## 四、实现思路

采用 **“EXTI 中断 + 时基消抖”** 的实现方案：

```
按下 B1（PC13 由低变高 → 上升沿）
    │
    ▼ 上升沿触发 EXTI13 中断
EXTI13_IRQHandler()
    │
    ▼
HAL_EXTI_IRQHandler()               // 清中断标志、识别触发沿
    │
    ▼
HAL_EXTI_TriggerCallback()          // 用户覆盖的弱回调（我们的代码）
    │
    ├─ 仅处理上升沿
    ├─ HAL_GetTick() 20ms 消抖       // 间隔 < 20ms 视为抖动，忽略
    ├─ HAL_GPIO_TogglePin() 翻转 PA5
    ▼
LD1 状态翻转
```

每一步使用的 HAL 函数如下表：

| 步骤 | HAL 函数 | 作用 |
|------|----------|------|
| 识别触发沿 | `HAL_EXTI_IRQHandler(hexti)` | 清中断标志，回调 `HAL_EXTI_TriggerCallback()` |
| 消抖取时 | `HAL_GetTick()` | 返回系统 ms 时基，用于判断两次触发间隔 |
| 翻转 LED | `HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN)` | 切换 PA5 输出电平 |

**消抖原理**：用 `HAL_GetTick()` 获取当前 ms 时间，只有当本次触发与上次有效触发间隔 ≥ 20ms 才认定为一次有效按下；小于 20ms 的抖动直接忽略。该方式**非阻塞**，不在中断中延时。

**为什么覆盖回调**：`HAL_EXTI_TriggerCallback()` 是 HAL 驱动中 `__WEAK` 声明的弱回调，用户在自己的源文件中重写同名函数即可接管按键逻辑，无需修改驱动源码。

---

## 五、关键代码

以下是 [main.c](main.c) 中新增的核心代码，分四部分：

### 5.1 消抖时间与状态变量

```c
/* Private define ------------------------------------------------------------*/
#define DEBOUNCE_TIME_MS    20U   /* 按键消抖时间（毫秒） */

/* Private variables ---------------------------------------------------------*/
static uint32_t last_tick = 0U;   /* 上一次有效按键触发的系统时间戳，用于消抖 */

/* Private functions prototype -----------------------------------------------*/
static void handle_key_press(void);
```

### 5.2 主函数与空闲循环

```c
int main(void)
{
  if (mx_system_init() != SYSTEM_OK)   /* 系统与 GPIO/EXTI 初始化（含 HAL_Init） */
  {
    return (-1);
  }
  else
  {
    /* 记录上电时刻，作为消抖时间戳的初始化基准 */
    last_tick = HAL_GetTick();

    /* 主循环：空闲等待；按键事件由 EXTI13 中断触发处理 */
    while (1)
    {
    }
  }
}
```

### 5.3 按键处理函数

```c
/**
  * brief:  按键有效按下时的处理函数。
  * retval: 无
  */
static void handle_key_press(void)
{
  /* 切换板载 LED（PA5）状态：每次有效按下翻转一次 */
  HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);
}
```

### 5.4 EXTI 外部中断回调（核心）

```c
/**
  * brief:  EXTI 触发回调（覆盖 __WEAK 默认实现）。
  *         当 PC13（B1 用户按键）产生上升沿中断时被调用。
  * param:  hexti   EXTI 句柄
  * param:  trigger 触发沿（本工程仅配置了上升沿）
  * retval: 无
  *
  * 消抖：利用系统时基，两次有效触发间隔必须大于 DEBOUNCE_TIME_MS。
  */
void HAL_EXTI_TriggerCallback(hal_exti_handle_t *hexti, hal_exti_trigger_t trigger)
{
  uint32_t tick;

  /* 仅处理上升沿触发（本工程只有 PC13/B1 配置了 EXTI 上升沿） */
  if (trigger != HAL_EXTI_TRIGGER_RISING)
  {
    return;
  }

  tick = HAL_GetTick();

  /* 基于时基的软件消抖：间隔小于消抖时间则视为抖动，忽略本次触发 */
  if ((tick - last_tick) < DEBOUNCE_TIME_MS)
  {
    return;
  }

  /* 更新最后一次有效触发时刻，作为下一次消抖比较基准 */
  last_tick = tick;

  /* 执行有效按键动作：切换板载 LED */
  handle_key_press();

  /* 避免未使用形参的编译警告 */
  STM32_UNUSED(hexti);
}
```

**关键点提示：**
- 引脚宏（`LD1_PORT`、`LD1_PIN`、`B1_PORT`、`B1_PIN`）来自 `mx_gpio_default.h`（见 [第二节 2.1/2.2](#二-硬件说明)），**勿写成硬编码引脚号**。
- 回调是 `__WEAK` 弱函数，用户重写同名函数即可生效，无需修改 HAL 源码。
- 消抖使用 `HAL_GetTick()` 时间差判断，**非阻塞**，不在中断中忙等。

---

## 六、HAL 依赖前提

本代码依赖以下工程已就绪的前提，**不要自行改动**：

1. **系统初始化**：`mx_system_init()`（见 [mx_system.c](generated/hal/mx_system.c)）在进入主循环前已调用 `HAL_Init()`，完成 HAL 和时基的初始化。
2. **SysTick 时基已运行**：`SysTick_Handler` 已在 `mx_system.c` 中实现，调用 `HAL_IncTick()` 递增 tick，因此 `HAL_GetTick()` 可用，用于 20ms 消抖计时。
3. **GPIO 驱动可用**：`stm32c5xx_hal_gpio` 已启用，提供 `HAL_GPIO_TogglePin()`。
4. **EXTI 驱动可用**：`stm32c5xx_hal_exti` 已启用（`USE_HAL_EXTI_MODULE = 1`），提供 `HAL_EXTI_TriggerCallback()` 弱回调；生成代码已在 `mx_gpio_default.c` 中完成 PC13 的 EXTI13 初始化（`HAL_EXTI_TRIGGER_RISING` 上升沿）。

> 如果缺失上述前提（例如未调用 `HAL_Init`），`HAL_GetTick()` 将因 tick 不递增而失效，消抖计时也会失效。

---

## 七、编译烧录步骤

1. **打开工程**：用 VS Code 打开本工程目录（`GPIO_EXTI_cmake`），或通过工程根目录的 `CMakePresets.json` 加载 CMake 预置。
2. **选择配置**：激活 CMake 预设 `debug_GCC_STM32C542RCT6`（生成器 Ninja，工具链文件 `cmake/GCC.14.3.1.cmake`）。
3. **构建（Build）**：在 VS Code 中执行 CMake **Build**，等待编译通过，生成可执行/固件文件。
4. **烧录（Flash）**：执行 VS Code 的 **Flash** 任务，将固件下载到开发板。
5. **连接要求**：**请使用支持数据传输的 USB 线**（而非仅供电的充电线）连接 NUCLEO-C542RC 的 ST-LINK 接口，否则烧录会失败。

烧录并复位后，按下 B1 按键，即可看到 LD1 沿每次有效按下翻转。

---

## 八、测试结果

- [x] 复位后 LD1 保持熄灭，无按键时不会误触发；
- [x] 按一下 B1 → 触发上升沿中断 → LD1 翻转一次（亮 → 灭 或 灭 → 亮）；
- [x] 松开 B1 → 不触发（下降沿不配置触发），LD1 状态不变；
- [x] 快速按下时由 20ms 消抖过滤抖动，状态稳定、无重复误翻转；
- [x] 反复按下可连续响应翻转。

**结论：功能正常，符合“按下触发、松开不触发”的预期。**

---

## 九、后续可扩展方向

1. **轮询方案对比**：改用主循环 `while(1)` 轮询 `HAL_GPIO_ReadPin()` 检测按键，配合阻塞 `HAL_Delay()` 消抖，实现“按住不重复、松开可再响应”，逻辑更简单但会占用 CPU。
2. **长按 / 双击识别**：在回调中结合 `HAL_GetTick()` 记录按/松时刻，区分单击、双击、长按等不同操作。
3. **下降沿 / 双边沿触发**：将 EXTI 配置为 `HAL_EXTI_TRIGGER_FALLING` 或 `HAL_EXTI_TRIGGER_RISING_FALLING`，可响应松开或同时响应按下与松开。
4. **按键复用**：同一按键在不同系统状态（菜单/运行模式）下执行不同动作。
5. **事件模式（EXTI Event）**：改用 `HAL_EXTI_MODE_EVENT` 配置事件，不进入中断处理，功耗更低。
