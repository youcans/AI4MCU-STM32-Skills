# TIM_IT_500ms 编程说明（定时器周期中断实验）

> 面向嵌入式初学者：本文档配合 NUCLEO-C542RC 开发板，讲解如何使用定时器周期中断实现板载 LED 的周期性闪烁。

---

## 1. 项目概述

本项目使用 **STM32C542RCT6**（NUCLEO-C542RC 开发板）的 **TIM6 基本定时器** 产生**周期中断**，在中断回调中翻转板载 LED **LD1（PA5）**，实现 LED 每 **500 ms** 切换一次亮灭状态（即亮 500 ms、灭 500 ms，闪烁周期为 1 s）。

工程由 **STM32CubeMX2** 生成基础代码（时钟、GPIO、TIM6 初始化），我们只修改了入口文件 `main.c`：

- 在 `main()` 中启动 TIM6 周期中断；
- 实现 HAL 的回调函数 `HAL_TIM_UpdateCallback()` 翻转 LED。

不需要任何软件延时（如 `HAL_Delay`），CPU 可以在 `while(1)` 空循环中休息或做其他事情，定时精度完全由硬件保证。

---

## 2. 硬件说明

| 项目 | 说明 |
| ---- | ---- |
| 开发板 | NUCLEO-C542RC |
| MCU | STM32C542RCT6（ARM Cortex-M33 内核） |
| 板载 LED | **LD1**，连接在 **PA5** 引脚上，推挽输出，**高电平点亮** |
| 调试/烧录 | 板载 ST-LINK 调试器（USB 连接电脑） |

**引脚定义**（由 CubeMX 生成，见 `generated/hal/mx_gpio_default.h`）：

```c
#define LD1_PORT      HAL_GPIOA        /* 引脚所在端口 */
#define LD1_PIN       HAL_GPIO_PIN_5   /* 引脚编号 */
#define LD1_INIT_STATE   HAL_GPIO_PIN_RESET   /* 上电初始状态：LED 灭 */
```

---

## 3. 功能需求

1. 使用 **TIM6** 定时器产生周期中断，不需要软件延时；
2. 每次 TIM6 更新事件触发一次中断（每 **500 ms** 一次）；
3. 在中断回调中翻转 LD1（PA5），使 LED 每 500 ms 亮灭切换一次。

---

## 4. 实现思路

### 4.1 为什么用定时器中断？

- `HAL_Delay()` 会让 CPU 空转等待，浪费处理器时间，且延时期间无法响应其他事件；
- 定时器由**硬件独立计数**，溢出时通过 NVIC 触发中断，周期精确、CPU 不阻塞；
- 本实验是初学者理解"定时器 + 中断 + 回调"三要素组合的最佳入门例程。

### 4.2 系统的初始化流程

程序启动后调用 CubeMX 生成的 `mx_system_init()`，依次完成：

```
HAL_Init() ─→ 系统时钟(RCC 144 MHz) ─→ ICACHE ─→ GPIO(PA5 输出) ─→ TIM6 初始化
```

其中 `mx_tim6_init()`（见 `generated/hal/mx_tim6.c`）完成 TIM6 的时基配置，并使能了 TIM6 在 NVIC 中的中断。

### 4.3 TIM6 如何定时 500 ms？

TIM6 是基本定时器，时基由两个寄存器决定：

- **PSC（预分频器）**：对 TIM6 的输入时钟进行分频；
- **ARR（自动重载寄存器）**：计数器从 0 计数到 ARR 后溢出，产生更新事件。

本项目时钟配置为 SYSCLK = **144 MHz**，APB1 分频为 1，因此 TIM6 输入时钟 = **144 MHz**。

CubeMX 生成的参数（见 `mx_tim6_init()`）：

```c
config.prescaler   = 14399;   /* PSC = 14399 */
config.period      = 0x1387;  /* ARR = 4999 */
config.counter_mode = HAL_TIM_COUNTER_UP;   /* 向上计数 */
```

定时计算公式：

```
更新频率 = TIM6时钟 / ((PSC + 1) × (ARR + 1))
         = 144,000,000 / ((14399 + 1) × (4999 + 1))
         = 144,000,000 / (14400 × 5000)
         = 2 Hz
```

即每 **0.5 s（500 ms）** 产生一次更新事件，触发一次中断。

> **小提示**：想改变闪烁快慢，只需在 CubeMX 中修改 TIM6 的 Prescaler 和 Counter Period，再重新生成代码即可，`main.c` 不用改动。

### 4.4 中断的处理流程

TIM6 溢出后，硬件会自动完成以下链路：

```
TIM6 计数到 ARR 溢出
   → 置位更新标志 UIF
   → 触发 TIM6 全局中断（NVIC）
   → 进入 TIM6_IRQHandler()（CubeMX 已生成）
   → HAL_TIM_IRQHandler() 清除标志
   → 调用弱回调函数 HAL_TIM_UpdateCallback()
   → 我们实现的回调中翻转 LED
```

`TIM6_IRQHandler` 由 CubeMX 生成，我们**不需要也不能修改**（它把中断转交给 HAL）：

```c
void TIM6_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&hTIM6);
}
```

我们只需要做两件事：
1. 在 `main()` 中调用 `HAL_TIM_Start_IT()` 启动计数并使能更新中断；
2. 实现弱符号回调 `HAL_TIM_UpdateCallback()`（HAL 库中有个空的弱定义，我们提供一个强定义覆盖它）。

---

## 5. 关键代码

修改后的 `main.c`（工程根目录）核心内容如下。

### 5.1 包含头文件

```c
#include "main.h"
```

`main.h` 会间接包含所有需要的声明：`mx_system.h`（系统初始化）、`mx_tim6.h`（TIM6 句柄获取）、`mx_gpio_default.h`（LD1 引脚宏）以及 HAL 库头文件，所以我们不需要额外 `#include`。

### 5.2 主函数：启动 TIM6 周期中断

```c
int main(void)
{
  /* 系统初始化：时钟、GPIO、TIM6 配置（CubeMX 生成） */
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }
  else
  {
    /* 启动 TIM6 周期中断：
       使能更新中断（UIE）并启动计数器（CEN），
       之后每 500 ms 产生一次更新中断 */
    if (HAL_TIM_Start_IT(mx_tim6_gethandle()) != HAL_OK)
    {
      return (-1);
    }

    /* 主循环：定时与 LED 翻转全部在中断中完成，主循环无事可做 */
    while (1) {}
  }
}
```

说明：

- `mx_tim6_gethandle()` 返回 CubeMX 生成的 TIM6 句柄（`hal_tim_handle_t *`）；
- `HAL_TIM_Start_IT()` 是**中断模式启动**函数，它使能 TIM6 的更新中断并启动计数；
- 中断开启后，主循环可以处于空闲状态。

### 5.3 中断回调：翻转 LED

```c
void HAL_TIM_UpdateCallback(hal_tim_handle_t *htim)
{
  /* 每 500 ms 进入本函数一次，翻转板载 LED LD1（PA5） */
  HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);
}
```

说明：

- `HAL_TIM_UpdateCallback` 是 HAL 库中定义的**弱符号**（`__WEAK`）回调，HAL 的 `HAL_TIM_IRQHandler` 在处理更新事件时会调用它；
- 我们在用户代码中提供一个**强定义**，链接时强定义覆盖弱定义，中断就会调用我们的版本；
- 回调在**中断上下文**中执行，应保持简短——这里只有一条翻转语句，非常合适；
- 注意：当前 HAL 未开启 `USE_HAL_TIM_REGISTER_CALLBACKS`（回调注册模式），因此直接覆盖弱函数即可生效。

---

## 6. HAL 依赖前提

本实验能跑通，依赖于 CubeMX 生成代码提供的以下前提（**不需要手动修改**）：

| 依赖 | 生成位置 | 作用 |
| ---- | -------- | ---- |
| 系统时钟 144 MHz，APB1 分频 = 1 | `generated/hal/mx_rcc.c` | 提供 TIM6 的 144 MHz 时基时钟 |
| TIM6 时基配置（PSC/ARR/更新源） | `generated/hal/mx_tim6.c` | 决定 500 ms 周期 |
| TIM6 中断优先级设置与使能 | `generated/hal/mx_tim6.c` | 让 TIM6 中断能进入 `TIM6_IRQHandler` |
| `TIM6_IRQHandler` 转交 HAL | `generated/hal/mx_tim6.c` | 中断分发到回调 |
| PA5 输出模式配置 + `LD1_*` 宏 | `generated/hal/mx_gpio_default.c/.h` | 提供 LED 引脚操作 |
| HAL 库源码（`stm32c5xx_hal_tim.c` 等） | 构建时由 `cmake/components.cmake` 自动纳入 | 提供 `HAL_TIM_Start_IT`、`HAL_GPIO_TogglePin` 等函数 |

如果重新生成代码后发现 TIM6 参数不在本工程的配置里（比如未勾选 TIM6 的中断），可回到 CubeMX 检查：TIM6 时钟源为**内部时钟**，并确保在 **NVIC Settings** 中勾选了"TIM6 global interrupt"。

---

## 7. 编译烧录步骤

### 7.1 需要的工具

- **CMake**（≥ 3.30）与 **Ninja** 构建系统
- **ARM GCC 工具链**：本机使用 STMicroelectronics 提供的 GNU Tools for STM32（arm-none-eabi-gcc，14.3.1 版本）
- **烧录工具**：STM32CubeProgrammer（CLI：`STM32_Programmer_CLI`，或使用其图形界面）

### 7.2 编译

在工程根目录（含 `CMakeLists.txt` 的 `TIM_IT_500ms_cmake` 目录）执行：

```bash
# 1. 配置（生成构建文件，输出到 build/debug_GCC_STM32C542RCT6/）
cmake --preset debug_GCC_STM32C542RCT6

# 2. 构建（生成可执行文件 .elf）
cmake --build --preset debug_GCC_STM32C542RCT6
```

构建产物：

```
build/debug_GCC_STM32C542RCT6/TIM_IT_500ms.elf
build/debug_GCC_STM32C542RCT6/TIM_IT_500ms.map
```

### 7.3 烧录

用 USB 线连接开发板（板载 ST-LINK），然后用 STM32CubeProgrammer 命令行烧录：

```bash
STM32_Programmer_CLI -c port=SWD mode=HOTPLUG -w \
  build/debug_GCC_STM32C542RCT6/TIM_IT_500ms.elf -v -rst
```

参数说明：`-c port=SWD` 连接板载调试器；`-w` 写入固件；`-v` 校验；`-rst` 烧录完成后复位运行。

也可以打开 STM32CubeProgrammer 图形界面，选择 `.elf` 文件后点击"下载并运行"。

---

## 8. 测试结果

| 测试项 | 结果 |
| ------ | ---- |
| 编译 | ✅ 通过（GNU Tools for STM32 14.3.1，无错误无警告） |
| 烧录 | ✅ 成功（板载 ST-LINK，SWD） |
| 功能验证 | ✅ 上电后板载 LED **LD1** 每 500 ms 翻转一次：亮 500 ms → 灭 500 ms，循环闪烁 |

**验证方法**：烧录并复位后观察板上绿色 LED（LD1，PA5），闪烁间隔均匀、周期为 1 s（亮灭各 500 ms）；可用手机秒表计时核对。

---

## 附：工程结构速览

```
TIM_IT_500ms_cmake/
├── main.c                  ← 本次修改：启动 TIM6 中断 + 更新回调翻转 LED
├── main.h
├── CMakeLists.txt          ← 工程入口
├── CMakePresets.json       ← 构建预设（debug_GCC_STM32C542RCT6）
├── generated/hal/          ← CubeMX 生成：mx_system/mx_rcc/mx_tim6/mx_gpio_*...
├── stm32c5xx_drivers/      ← HAL/LL 驱动源码（read-only）
├── stm32c5xx_dfp/          ← 器件支持包（寄存器定义等）
├── arch/cmsis/             ← CMSIS-Core
└── build/                  ← 构建输出（.elf/.map 等）
```
