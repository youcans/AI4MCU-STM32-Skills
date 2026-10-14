# 用户按键外部中断实验 —— 工程编程说明

## 一、项目概述

本项目实现 **GPIO 外部中断（EXTI）实验**：在 **NUCLEO-C542RC**（搭载 **STM32C542RCT6**）开发板上，将用户按键 **B1（PC13）** 配置为**外部中断**输入，每当产生中断（上升沿触发）时，在**中断回调**中切换板载 LED **LD1（PA5）** 的状态（点亮 ⇄ 熄灭）。

**核心行为：**
- 按下并松开 B1 一次 → LED 状态翻转一次；
- 使用 **外部中断机制**，CPU 在空闲时无需轮询按键，按键触发时才进入中断服务；
- 触发信号经过 **20ms 软件消抖**，避免机械抖动导致的误触发。

> 与“用户按键轮询实验”的区别在于：轮询版在主循环 `while(1)` 中不停地读引脚判断是否按下；本实验用硬件 **EXTI** 边沿产生中断，由中断回调响应，主循环保持空闲。

---

## 二、硬件说明

### 2.1 引脚定义

| 引脚 | 信号 | 功能 | 有效电平 |
|------|------|------|----------|
| PA5 | LD1 | 板载 LED，输出模式 | **高电平（SET）点亮** |
| PC13 | B1 | 用户按键，触发 EXTI13 | **按下为低电平（RESET）** |

> NUCLEO-C542RC 的 B1 按键按下时与 GND 短接，读出为低电平；松开后恢复高电平。因此按下瞬间产生**下降沿**、松开瞬间产生**上升沿**。本工程使用 **上升沿触发**，即中断发生在按键**松开**的瞬间。

### 2.2 引脚与宏对照表

工程由 CubeMX2 生成代码，引脚名在 [generated/hal/mx_gpio_default.h](generated/hal/mx_gpio_default.h) 中定义了别名宏，**不要在 main.c 里写死 `HAL_GPIO_PIN_5` / `HAL_GPIO_PIN_13`**，应使用下列宏以保持可移植性：

| 宏 | 值 | 说明 |
|----|----|------|
| `LD1_PORT` | `HAL_GPIOA` | LD1 所在 GPIO 口（端口 A） |
| `LD1_PIN` | `HAL_GPIO_PIN_5` | LD1 引脚号 5（PA5） |
| `B1_PORT` | `HAL_GPIOC` | B1 所在 GPIO 口（端口 C） |
| `B1_PIN` | `HAL_GPIO_PIN_13` | B1 引脚号 13（PC13） |

这些宏通过 `main.h` → `mx_hal_def.h` → `mx_gpio_default.h` 的包含链自动可见，因此在 [main.c](main.c) 中可直接使用。

---

## 三、功能需求

1. **外部中断触发**：将 PC13 配置为 EXTI13，产生中断后进入中断服务，无需主循环轮询。
2. **中断回调翻转 LED**：在 EXTI 触发回调中翻转 PA5（LD1）状态。
3. **消抖 20ms**：以 `HAL_GetTick()` 记录时间，两次有效响应间隔小于 20ms 的中断视为抖动并忽略。

---

## 四、实现思路

采用 **“EXTI 边沿中断 + 时间戳消抖 + 回调中翻转 LED”** 的方案：

```
用户操作 B1（PC13）
    │
    ├─ 按下 → 引脚拉低（下降沿）
    ╰─ 松开 → 引脚恢复高（上升沿）──┐
                                  │ 硬件 EXTI13 检测到上升沿
                                  ▼
                     EXTI13_IRQHandler → HAL_EXTI_IRQHandler
                                  │
                                  ▼
                    HAL 调用注册的触发回调 ButtonExtiCallback
                                  │
                          ┌───────┴────────┐
                          │ 距上次响应 < 20ms?     │
                          └───┬───────────┬───────┘
                              │ 是         │ 否 = 有效触发
                              ▼            ▼
                          视为抖动        更新 last_tick，翻转 LED
                          忽略            HAL_GPIO_TogglePin
```

每一步使用的 HAL 函数如下表：

| 步骤 | HAL 函数 | 作用 |
|------|----------|------|
| 获取 EXTI13 句柄 | `mx_gpio_default_exti13_gethandle()` | 返回 CubeMX 生成的 `hal_exti_handle_t *` |
| 注册触发回调 | `HAL_EXTI_RegisterTriggerCallback(hexti, cb)` | 绑定 EXTI 触发回调函数 |
| 读当前时间 | `HAL_GetTick()` | 获取系统毫秒节拍，用于消抖计时 |
| 翻转 LED | `HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN)` | 切换 PA5 输出电平 |

---

## 五、关键代码

以下是 [main.c](main.c) 中的核心代码。它由两部分组成：**应用初始化段**（重新配置 PC13 上拉并注册回调）和**中断回调函数**。

### 5.1 应用初始化段

```c
#include "main.h"
#include "mx_gpio_default.h"   /* 提供 LD1_PORT / B1_PORT 等引脚宏及 EXTI 相关 API */

#define B1_DEBOUNCE_MS   20U    /* 按键消抖时间 */

int main(void)
{
  if (mx_system_init() != SYSTEM_OK)   /* 系统初始化：HAL_Init + 时钟 + GPIO + EXTI 配置 */
  {
    return (-1);
  }
  else
  {
    hal_exti_handle_t *hexti;

    /* PC13 重配为上拉输入，保证空闲时电平稳定为高。
       按下拉低、释放回到高，从而在释放时产生上升沿(RISING)触发中断。 */
    hal_gpio_config_t gpio_config;
    gpio_config.mode = HAL_GPIO_MODE_INPUT;
    gpio_config.pull = HAL_GPIO_PULL_UP;
    HAL_GPIO_Init(B1_PORT, B1_PIN, &gpio_config);

    /* 注册 EXTI13 触发回调（沿用 CubeMX 默认的上升沿触发配置） */
    hexti = mx_gpio_default_exti13_gethandle();
    HAL_EXTI_RegisterTriggerCallback(hexti, ButtonExtiCallback);

    while (1)   /* 主循环保持空闲，等待中断 */
    {
    }
  }
}
```

### 5.2 外部中断触发回调

```c
/**
  * brief: EXTI 触发回调。以系统节拍做 20ms 软件消抖，
  *        过滤按键抖动产生的连续触发，确认后切换 LED LD1。
  * note : 回调签名由 HAL2 EXTI 驱动定义：
  *        void (*cb)(hal_exti_handle_t *hexti, hal_exti_trigger_t trigger);
  */
static void ButtonExtiCallback(hal_exti_handle_t *hexti, hal_exti_trigger_t trigger)
{
  static uint32_t last_tick = 0U;
  uint32_t now;

  (void)hexti;     /* 本实验中不使用句柄参数，置空避免告警 */
  (void)trigger;   /* 本实验中不使用触发边沿参数，置空避免告警 */

  now = HAL_GetTick();

  /* 距上次有效响应不足 20ms 的触发为抖动，忽略 */
  if ((now - last_tick) >= B1_DEBOUNCE_MS)
  {
    last_tick = now;
    HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);   /* 翻转 LED：点亮 ⇄ 熄灭 */
  }
}
```

**关键点提示：**
- 回调的签名 `void (*)(hal_exti_handle_t *, hal_exti_trigger_t)` 是 HAL2 EXTI 驱动固定要求的，参数 `hexti`（句柄）与 `trigger`（触发边沿）本实验中不需要，用 `(void)` 置空避免编译器告警（工程开启了 `-Wall -Werror`）。
- `mx_gpio_default_exti13_gethandle()` 返回 CubeMX 生成代码持有的 EXTI13 句柄 `hEXTI13`（见 [mx_gpio_default.c](generated/hal/mx_gpio_default.c)），**不要自己另建 EXTI 句柄**，以免与生成代码配置冲突。
- 消抖用“时间差 `>= 20ms`”而非“`==`”，避免节拍计数器回绕及单点比较带来的边界问题。

---

## 六、HAL 依赖前提

本代码依赖以下工程已就绪的前提，**不要自行改动**：

1. **系统初始化**：`mx_system_init()`（见 [mx_system.c](generated/hal/mx_system.c)）在进入主循环前已调用 `HAL_Init()`、初始化时钟、ICACHE、MPU 及各外设。
2. **GPIO 与 EXTI 已配置**：`mx_gpio_default_init()` 已把 PA5 配为输出、PC13 配为输入，并把 EXTI13 配置为**上升沿（RISING）触发 + 中断模式**，使能了 `EXTI13_IRQn` 中断；`EXTI13_IRQHandler` 已在 [mx_gpio_default.c](generated/hal/mx_gpio_default.c) 中实现并调用 `HAL_EXTI_IRQHandler`。
3. **SysTick 时基已运行**：`SysTick_Handler` 已在 `mx_system.c` 中实现，调用 `HAL_IncTick()` 递增 tick，因此 `HAL_GetTick()` 可以返回毫秒时刻供消抖使用。
4. **回调用宏已开启**：`generated/hal/stm32c5xx_hal_conf.h` 中 `USE_HAL_EXTI_REGISTER_CALLBACKS=1` 与 `USE_HAL_EXTI_USER_DATA=1`，故 `HAL_EXTI_RegisterTriggerCallback()` 可用。

> 如果缺失上述前提（例如未调用 `HAL_Init`），`HAL_GetTick()` 不会递增，消抖将失效。

---

## 七、编译烧录步骤

1. **打开工程**：用 VS Code 打开本工程目录（`GPIO_Button_cmake`），或通过工程根目录的 `CMakePresets.json` 加载 CMake 预置。
2. **构建（Build）**：在 VS Code 中执行 CMake **Build**（对应调试配置 `debug_GCC_STM32C542RCT6`），等待编译通过，生成固件文件。
3. **烧录（Flash）**：执行 VS Code 的 **Flash** 任务，将固件下载到开发板。
4. **连接要求**：**请使用支持数据传输的 USB 线**（而非仅供电的充电线）连接 NUCLEO-C542RC 的 ST-LINK 接口，否则烧录会失败。

烧录并复位后，按下并松开 B1 按键，即可看到 LD1 沿每次触发翻转。

---

## 八、测试结果

- [x] 按一下 B1 → LD1 翻转一次（亮 → 灭 或 灭 → 亮）；
- [x] 触发使用 EXTI13 外部中断，主循环空闲，无需轮询；
- [x] 连续快速触发时，抖动被 20ms 消抖过滤，LD1 不会误闪烁；
- [x] 编译、烧录、运行均正常。

**结论：功能正常。**
