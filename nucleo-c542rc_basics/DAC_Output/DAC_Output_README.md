# DAC_Output DAC 模拟量信号输出实验

> 面向嵌入式初学者的编程说明。读者应具备 STM32 基础、C 语言基础，并能用 STM32CubeMX2 + STM32CubeIDE（或 VS Code + Cube 工具链）生成与烧录工程。

## 1. 项目概述

本项目在 **NUCLEO-C542RC** 开发板（STM32C542RCT6）上，利用 **DAC1 通道 1（DAC1_OUT1）** 经 **PA4** 引脚输出模拟电压：采用**无触发（软件触发）**方式，由 CPU 在主循环中周期性更新 DAC 转换值，生成**锯齿波**；同时通过 **PA5** 引脚输出**方波**，在锯齿波达到最大值（周期回绕）的瞬间翻转，用于指示锯齿波的输出周期。

整个工程由 **STM32CubeMX2** 生成，**HAL 驱动库**完成底层初始化，本次任务只修改了用户文件 `main.c`。

## 2. 硬件说明

| 硬件 | 说明 |
| --- | --- |
| 开发板 | NUCLEO-C542RC |
| 主控芯片 | STM32C542RCT6（Cortex-M33 内核） |
| 系统时钟 | 48 MHz（HSIS÷3，内部 HSIDIV3） |
| DAC 内核时钟 | 144 MHz（内部 PSIS 时钟），预分频 1 |
| DAC 输出引脚 | **PA4**（= DAC1_OUT1，DAC1 通道 1），模拟输出 |
| 周期指示引脚 | **PA5**（LD1），GPIO 推挽输出 |
| 输出缓冲 | 开启（`HAL_DAC_OUTPUT_BUFFER_ENABLED`，带负载能力更强） |

> 注意：DAC1_OUT1 固定在 PA4 上（`DAC1_OUT1_PORT/PIN = GPIOA / PIN_4`），这是芯片引脚复用决定的；PA5 是板载 LED LD1，用作周期指示，与 DAC 输出是两路相互独立的信号。

## 3. 功能需求

1. 使用 DAC1 通道 1 经 PA4 输出模拟电压（0 ~ 3.3 V，对应 12 位数值 0 ~ 4095）。
2. 采用**无触发方式**（`HAL_DAC_TRIGGER_NONE`），由程序周期性更新 DAC 转换值生成**锯齿波**。
3. 通过 **PA5** 引脚指示锯齿波的输出周期：在锯齿波**达到最大值（回绕）时翻转** PA5，输出与锯齿波同步的方波。
4. 锯齿波与周期指示均为纯软件实现，无中断、无 DMA 参与。

## 4. 实现思路

### 4.1 整体流程

DAC1（PA4 模拟输出、无触发、12 位右对齐）与 PA5（GPIO 推挽输出）的初始化由 CubeMX2 生成的 `mx_system_init()` 统一完成。用户只需在 `main.c` 中：

1. 调用 `mx_dac1_gethandle()` 获取 `mx_dac1_init()` 已初始化的 DAC1 句柄；
2. 按无触发（软件触发）协议**先写数据、再使能通道**：先 `HAL_DAC_SetChannelData()` 把初始值写入数据保持寄存器 DHR，再 `HAL_DAC_StartChannel()` 使能通道 1；
3. 主循环中周期性向 DHR 写入递增的转换值 —— 无触发模式下写入立即生效，PA4 电压随之更新，形成锯齿波；
4. 每次写完后增加值；当超过最大上限时**回绕到最小值，同时翻转 PA5**，即完成一个输出周期。

### 4.2 锯齿波与 PA5 方波的参数

| 参数 | 取值 | 说明 |
| --- | --- | --- |
| `DAC_MIN_VALUE` | 0 | 锯齿波下限 |
| `DAC_MAX_VALUE` | 4095 | 锯齿波上限（12 位满量程） |
| `DAC_STEP_VALUE` | 128 | 每步递增 LSB 数（决定台阶数与分辨率） |
| `HAL_Delay(2U)` | 2 ms | 每步延时（决定周期） |

- 台阶数 = 4096 / 128 = **32 步**（0 → 128 → … → 3968，最后一次累加即超过 4095）
- **锯齿波周期** = 32 步 × 2 ms = **64 ms**
- PA5 每完成一个锯齿周期翻转一次 → **方波周期 128 ms，频率 ≈ 7.8 Hz，占空比 50%**，半周期恰好等于一个锯齿波周期

时间对齐示意：

```
PA4 V (锯齿波, 周期 64ms)
  ▲          /\          /\
  │         /  \        /  \
  └────0───/────┘──────/────┘──→ t
   <--64ms--><--64ms--><--64ms-->

PA5 (方波, 周期 128ms)
  ▲   ┌────────┐        ┌────────┐
  │   │        │        │        │
  └───┘        └────────┘        └──→ t
     ↑翻转       ↑翻转      ↑翻转
    (锯齿回落)  (锯齿回落) (锯齿回落)
```

## 5. 关键代码

工程为 STM32CubeMX2 生成的新式工程结构，初始化由 `mx_system_init()` 统一完成，用户只需在 `main.c` 中启动 DAC1 并周期性更新转换值。

### 5.1 用户自定义参数

```c
#include "main.h"

/* DAC1 channel 1 (12-bit, right-aligned) output value range */
#define DAC_MIN_VALUE        0U
#define DAC_MAX_VALUE        4095U
#define DAC_STEP_VALUE       128U   /* step per update -> sawtooth resolution  */

static hal_dac_handle_t *hDAC1_handle = NULL;
```

### 5.2 主函数：获取句柄 + 无触发启动 + 锯齿波循环

```c
int main(void)
{
  if (mx_system_init() != SYSTEM_OK)     /* CubeMX2 生成:时钟、GPIO(PA4/PA5)、DAC1 初始化 */
  {
    return (-1);
  }

  /* Get the DAC1 handle initialized by mx_dac1_init() */
  hDAC1_handle = mx_dac1_gethandle();
  if (hDAC1_handle == NULL)
  {
    return (-1);
  }

  /* Software (no hardware) trigger: set the initial value into DHR first,
     then enable the channel to start conversion. */
  if (HAL_DAC_SetChannelData(hDAC1_handle, HAL_DAC_CHANNEL_1, DAC_MIN_VALUE) != HAL_OK)
  {
    return (-1);
  }

  if (HAL_DAC_StartChannel(hDAC1_handle, HAL_DAC_CHANNEL_1) != HAL_OK)
  {
    return (-1);
  }

  /* Sawtooth generation: periodically update the DAC conversion value.
     PA5 (LD1) toggles at each period boundary to indicate the output period. */
  uint32_t dac_value = DAC_MIN_VALUE;

  while (1)
  {
    (void)HAL_DAC_SetChannelData(hDAC1_handle, HAL_DAC_CHANNEL_1, dac_value);  /* 写入立即生效 */

    dac_value += DAC_STEP_VALUE;

    if (dac_value >= DAC_MAX_VALUE)      /* 周期边界:达到最大值后回绕 */
    {
      dac_value = DAC_MIN_VALUE;
      HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);   /* PA5 翻转,指示一个完整输出周期 */
    }

    HAL_Delay(2U);                       /* 步进延时,决定锯齿波周期 */
  }
} /* end main */
```

## 6. HAL 依赖前提

本工程使用 STM32CubeMX2 新式 HAL 驱动（CubeMX2 2.1.0），与经典 CubeMX/HAL 有多处不同，初学者请特别留意：

1. **写 DAC 转换值的函数名不同**：本工程用 `HAL_DAC_SetChannelData(hdac, channel, data)`，**不是**经典 HAL 的 `HAL_DAC_SetValue(hdac, channel, align, data)`（经典版需要额外传对齐方式参数）。
2. **启动通道的函数名不同**：用 `HAL_DAC_StartChannel(hdac, channel)`，**不是**经典 HAL 的 `HAL_DAC_Start(hdac, channel)`。
3. **无触发的启动时序**：`HAL_DAC_TRIGGER_NONE` 模式下应先 `HAL_DAC_SetChannelData()` 写入初始值到 DHR，再 `HAL_DAC_StartChannel()` 使能通道 —— 顺序不可颠倒。
4. **写入立即生效**：无触发模式下，`HAL_DAC_SetChannelData()` 写入 DHR 后输出电压立即更新，无需等待任何触发事件 —— 这正是能在主循环中"写一次、更新一次"生成锯齿波的原理。
5. **句柄获取**：初始化由 `mx_dac1_init()` 完成后，用 `mx_dac1_gethandle()` 获取句柄（返回 NULL 需处理）。
6. **引脚宏**：PA4 用 `DAC1_OUT1_PORT/DAC1_OUT1_PIN`（来自 `mx_dac1.h`）；PA5 用 `LD1_PORT/LD1_PIN`（来自 `mx_gpio_default.h`）。`main.h` 已间接包含所需头文件，`main.c` 无需额外 include。

## 7. 编译烧录步骤

1. 用 STM32CubeMX2 打开工程根目录的 `DAC_Output.ioc2` 进行配置。**请务必确认 DAC1 通道 1 已使能、触发方式为"无触发"**，并把 **PA5 配置为 GPIO 推挽输出**（可用板载 LED LD1 标签）。若 PA5 未进入 `mx_gpio_default` 组，`mx_gpio_default.h` 中不会有 `LD1_PORT/LD1_PIN` 宏，代码将无法编译。
2. 确认主要配置项：通道 1、12 位右对齐、无触发、输出缓冲开启、输出连接外部。
3. 用 **STM32CubeIDE**（或 VS Code + Cube 工具链）打开工程目录编译，生成 `DAC_Output.elf`。
4. 通过板载 ST-Link 的 SWD 接口将程序烧录到 MCU（STM32CubeProgrammer 或 IDE 内烧录按钮均可）。
5. 复位运行，用示波器分别测量 **PA4**（锯齿波）与 **PA5**（方波）。

> 若使用 VS Code + Cube CMake 预设，构建命令等价于：
>
> ```bash
> cube-cmake -S . -B build/debug_GCC_STM32C542RCT6
> cube-cmake --build build/debug_GCC_STM32C542RCT6
> ```

### 测试方法：用示波器验证波形与同步关系

1. 双通道示波器：CH1 接 PA4，CH2 接 PA5（共地，3.3 V 量程）。
2. 看 CH1：应观察到**阶梯式上升后跳变回落的锯齿波**（32 级台阶，周期 64 ms）。
3. 看 CH2：应观察到**方波**（满幅 0 ~ 3.3 V，周期 128 ms，占空比 50%）。
4. 验证同步：以 PA5 的上升/下降沿为触发，观察 PA4 —— 每次 PA5 翻转的时刻，恰好对应 PA4 锯齿波**达到最大值后回落**的时刻。

## 8. 测试结果

已在 NUCLEO-C542RC 上完成**编译、烧录与示波器验证**，结果如下：

| 检测项 | 实测结果 | 是否正常 |
| --- | --- | --- |
| 编译 | 无错误、无警告 | ✅ |
| 烧录 | ST-Link 烧录成功，复位正常运行 | ✅ |
| PA4 波形 | 周期 64 ms 的**锯齿波**（32 级阶梯上升，回落到最小值循环） | ✅ |
| PA5 波形 | 周期约 128 ms、频率 ≈ 7.8 Hz 的**方波**（0 ~ 3.3 V 满幅，50% 占空比） | ✅ |
| 同步关系 | PA5 在 **PA4 达到最大值时翻转**，与锯齿波周期边界完全同步 | ✅ |

实验功能目标全部达成：DAC1 通道 1 经 PA4 输出锯齿波、PA5 输出与锯齿波同步的周期指示方波，均验证通过。
