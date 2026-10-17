# GPIO外部中断实验

## 1. 项目概述

本项目在 **NUCLEO-C542RC** 开发板（搭载 **STM32C542RCT6** MCU）上实现 **GPIO 外部中断（EXTI）实验**：用户按键 **B1（PC13）** 的电平变化触发 **EXTI13 外部中断**，在**中断回调**中切换板载 LED **LD1（PA5）** 的状态（点亮 ⇄ 熄灭）。

**核心行为：**
- 一次有效触发 → 进入 EXTI 中断 → LED 状态翻转一次；
- 仅配置**上升沿触发**，非上升沿不处理；
- 中断回调内置 **20ms 时基消抖**，避免按键机械抖动导致的误触发；
- 消抖基于 `HAL_GetTick()` 时间差判断，**非阻塞**，不在中断里忙等。

## 2. 硬件说明

### 2.1 引脚定义

| 引脚 | 信号 | 功能 | 有效电平 |
|------|------|------|----------|
| PA5 | LD1 | 板载 LED，GPIO 输出，推挽无上下拉 | **高电平（SET）点亮** |
| PC13 | B1 | 用户按键，GPIO 输入（无上下拉），EXTI 上升沿触发 | 按下接 GND，为低电平 |

> NUCLEO-C542RC 的 B1 按键按下时与 GND 短接，读出为低电平（`HAL_GPIO_PIN_RESET`）；松开后恢复高电平（`HAL_GPIO_PIN_SET`）。因此按下对应**下降沿**、松开对应**上升沿**。本工程将 EXTI13 配置为**上升沿触发**，所以实际是在**松开按键**的那一刻翻转 LED。LD1 采用低侧驱动，PA5 输出高电平时点亮。

### 2.2 引脚与宏对照表

工程由 CubeMX2 生成代码，引脚名在 `generated/hal/mx_gpio_default.h` 中定义了别名宏，**不要在 main.c 里写死引脚号**，应使用下列宏以保持可移植性：

| 宏 | 值 | 说明 |
|----|----|------|
| `LD1_PORT` | `HAL_GPIOA` | LD1 所在 GPIO 口（端口 A） |
| `LD1_PIN` | `HAL_GPIO_PIN_5` | LD1 引脚号 5（PA5） |
| `B1_PORT` | `HAL_GPIOC` | B1 所在 GPIO 口（端口 C） |
| `B1_PIN` | `HAL_GPIO_PIN_13` | B1 引脚号 13（PC13） |
| `LD1_INIT_STATE` | `HAL_GPIO_PIN_RESET` | LD1 初始状态（熄灭） |
| `LD1_ACTIVE_STATE` | `HAL_GPIO_PIN_SET` | LD1 点亮电平（高） |

这些宏通过 `main.h` → `mx_hal_def.h` → `mx_gpio_default.h` 的包含链自动可见，因此在 `main.c` 中可直接使用。

## 3. 功能需求

1. **PR-001 EXTI 中断触发**：B1（PC13）电平变化产生上升沿时，触发 EXTI13 中断。
2. **PR-002 有效触发翻转 LED**：每次**有效触发**，翻转 LD1（PA5）的输出状态（点亮 ⇄ 熄灭）。
3. **PR-003 非上升沿不处理**：EXTI13 配置为 `HAL_EXTI_TRIGGER_RISING`（见 `mx_gpio_default.c`），非上升沿根本不会产生中断、不会进入回调，保证动作只在上升沿那一刻执行。
4. **PR-004 时基消抖 20ms**：两次触发间隔不足 20ms 视为抖动，直接忽略，且消抖过程**非阻塞**（不占用中断长时间延时）。

## 4. 实现思路

采用 **"EXTI 中断 + HAL 时基消抖"** 的实现方案，主循环空闲运行，所有逻辑都放在中断回调中：

```text
B1（PC13）电平产生上升沿
    │
    ▼ 硬件触发 EXTI13 中断
EXTI13_IRQHandler()                    // 见 mx_gpio_default.c
    │
    ▼
HAL_EXTI_IRQHandler(&hEXTI13)          // 判定触发沿、清中断标志
    │
    ▼
HAL_EXTI_TriggerCallback()             // 被覆盖的弱回调（我们的代码）
    │
    ├─ 上升沿触发（初始化配置保证，回调不判断 trigger）
    ├─ HAL_GetTick() 20ms 时基消抖      // 间隔 < 20ms 视为抖动，忽略
    ├─ 更新 last_button_tick
    ├─ HAL_GPIO_TogglePin() 翻转 PA5
    ▼
LD1 状态翻转
```

每一步使用的 HAL 函数如下表：

| 步骤 | HAL 函数 | 作用 |
|------|----------|------|
| 中断入口 | `EXTI13_IRQHandler()` | EXTI13 中断服务函数（工程已生成） |
| 分发处理 | `HAL_EXTI_IRQHandler(&hEXTI13)` | 识别触发沿、清除中断标志、回调 TriggerCallback |
| 消抖取时 | `HAL_GetTick()` | 返回系统 ms 时基，判断两次触发的时间间隔 |
| 翻转 LED | `HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN)` | 切换 PA5 输出电平（点亮 ⇄ 熄灭） |

**消抖原理**：用 `HAL_GetTick()` 获取当前 ms 时间，只有当本次触发与上次有效触发间隔 ≥ 20ms 才认定为一次有效按下；小于 20ms 的抖动直接忽略。因为只是取时间戳做比较，**不做延时等待**，所以消抖过程非阻塞，不会长时间占用中断。

**为什么覆盖回调**：`HAL_EXTI_TriggerCallback()` 是 HAL 驱动中 `__WEAK` 声明的弱回调，用户在自己的源文件中重写同名函数即可接管按键逻辑，无需修改驱动源码。

## 5. 关键代码

以下是 [main.c](main.c) 中的完整实现，分三部分：

### 5.1 消抖时间与状态变量

```c
/* Private define ------------------------------------------------------------*/
/* Debounce time in ms: triggers closer than this are treated as button bounce. */
#define BUTTON_DEBOUNCE_TIME_MS 20U

/* Private variables ---------------------------------------------------------*/
/* Timestamp (ms) of the last valid button trigger, used for 20ms debounce. */
static uint32_t last_button_tick = 0U;
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
    /* 主循环：空闲等待；按键事件由 EXTI13 中断触发处理 */
    while (1) {}
  }
}
```

### 5.3 EXTI 外部中断回调（核心）

```c
/**
  * brief:  EXTI line trigger default callback (user override).
  *         Called by HAL_EXTI_IRQHandler when an interrupt fires on an
  *         enabled EXTI line. Here EXTI13 maps to the user button B1 (PC13);
  *         a trigger toggles the onboard LED LD1 (PA5).
  * param:  hexti   Pointer to EXTI handle.
  * param:  trigger Trigger value (not used here).
  */
void HAL_EXTI_TriggerCallback(hal_exti_handle_t *hexti, hal_exti_trigger_t trigger)
{
  /* Prevent unused argument(s) compilation warning */
  STM32_UNUSED(hexti);
  STM32_UNUSED(trigger);

  uint32_t tick = HAL_GetTick();

  /* Debounce: ignore triggers closer than 20ms to the last valid one */
  if ((tick - last_button_tick) < BUTTON_DEBOUNCE_TIME_MS)
  {
    return;
  }
  last_button_tick = tick;

  /* Toggle the onboard LED LD1 (PA5) on a valid EXTI13 (B1) trigger */
  HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);
}
```

**关键点提示：**
- 引脚宏（`LD1_PORT`、`LD1_PIN`、`B1_PORT`、`B1_PIN`）来自 `mx_gpio_default.h`（见 [2.2 引脚与宏对照表](#22-引脚与宏对照表)），勿写成硬编码引脚号。
- 回调是 `__WEAK` 弱函数，用户在 main.c 重写同名函数即生效，无需修改 HAL 源码。
- `tick - last_button_tick` 采用无符号减法，即使 tick 回绕（SysTick 溢出）也能正确比较，避免 `tick < last_tick` 时出错——这是时基消抖的常用写法。
- 消抖使用 `HAL_GetTick()` 时间差判断，**非阻塞**，不在中断中忙等。

## 6. HAL 依赖前提

本代码依赖以下工程已就绪的前提，**不要自行改动**：

1. **系统初始化**：`mx_system_init()`（见 `generated/hal/mx_system.c`）在进入主循环前已调用 `HAL_Init()`，完成 HAL 和时基的初始化。
2. **SysTick 时基已运行**：`SysTick_Handler` 已在 `mx_system.c` 中实现，调用 `HAL_IncTick()` 递增 tick，因此 `HAL_GetTick()` 可用，用于 20ms 消抖计时。
3. **GPIO 驱动可用**：`stm32c5xx_hal_gpio` 已启用，提供 `HAL_GPIO_TogglePin()`。
4. **EXTI 驱动可用**：`stm32c5xx_hal_exti` 已启用（`USE_HAL_EXTI_MODULE = 1`），且 `mx_gpio_default.c` 已对 PC13 完成 EXTI13 初始化：触发方式 `HAL_EXTI_TRIGGER_RISING`（上升沿）、关联 `HAL_EXTI_GPIOC`、`HAL_EXTI_MODE_INTERRUPT`，并已使能 NVIC（`EXTI13_IRQn`），同时在 `EXTI13_IRQHandler()` 中调用 `HAL_EXTI_IRQHandler(&hEXTI13)`。

> 如果缺失上述前提（例如未调用 `HAL_Init`），`HAL_GetTick()` 将因 tick 不递增而失效，消抖计时也会失效，甚至回调不会触发。

## 7. 编译烧录步骤

1. **打开工程**：用 VS Code 打开本工程目录，通过工程根目录的 `CMakePresets.json` 加载 CMake 预置。
2. **选择配置**：激活 CMake 预设 `debug_GCC_STM32C542RCT6`（生成器 Ninja，工具链文件 `cmake/GCC.14.3.1.cmake`）。
3. **构建（Build）**：在 VS Code 中执行 CMake **Build**，等待编译通过，生成可执行/固件文件。
4. **烧录（Flash）**：执行 VS Code 的 **Flash** 任务，将固件下载到开发板。
5. **连接要求**：**请使用支持数据传输的 USB 线**（而非仅供电的充电线）连接 NUCLEO-C542RC 的 ST-LINK 接口，否则烧录会失败。

烧录并复位后，操作 B1 按键，即可看到 LD1 沿每次有效触发翻转。

## 8. 测试结果

- [x] **编译**：`debug_GCC_STM32C542RCT6` 编译通过；
- [x] **烧录**：烧录成功，程序正常启动运行；
- [x] 复位后 LD1 保持熄灭，无按键时不会误触发；
- [x] 触发一次上升沿中断（对应 B1）→ LD1 翻转一次（亮 ⇄ 灭）；
- [x] 上升沿以外的触发被忽略，LD1 状态不变；
- [x] 快速触发时由 20ms 时基消抖过滤抖动，状态稳定、无重复误翻转；
- [x] 反复触发可连续响应翻转。

**结论：功能正常，PR-001、PR-002、PR-003、PR-004 全部验证通过。**
