# DAC 模拟量信号输出实验

## 1. 项目概述

本项目基于 NUCLEO-C542RC 开发板（STM32C542RCT6 MCU），使用片内 DAC1 外设的通道 1 在 PA4 引脚输出一个周期约 20Hz、幅值约 1.0V 的锯齿波模拟电压，并通过 PA5 引脚驱动板载用户 LED 指示锯齿波的输出周期。

程序在系统初始化完成后启动 DAC 输出，并在主循环中每隔 1ms 递增一次 DAC 转换数据：数据从 0 一直增加到对应 1.0V 的峰值后回零，循环往复，从而在 PA4 上形成锯齿波。同时根据当前数据所处的周期相位点亮或熄灭 LED，使 LED 的亮灭周期与锯齿波周期保持一致。

## 2. 硬件说明

| 硬件 | 说明 |
|---|---|
| 开发板 | NUCLEO-C542RC（板卡编号 MB2213，Nucleo-64 标准） |
| MCU | STM32C542RCT6，Cortex-M33 内核，最高 144MHz，LQFP64 |
| DAC | 12-bit DAC，本实验使用 DAC1 通道 1 |
| 输出引脚 | PA4（DAC1_OUT1），模拟输出锯齿波电压 |
| 指示引脚 | PA5（GPIO 输出），与板载用户 LED LD1 复用 |
| LED 极性 | 高电平点亮 LD1，低电平熄灭 LD1 |
| 调试下载 | 板载 ST-LINK/V3EC，SWD 接口，同时提供虚拟串口 |

DAC 输出电压与 12-bit 转换数据的关系为：

```text
输出电压 = DAC数据 / 4095 × VDDA
```

本板 VDDA 为 3.3V。若要输出 1.0V，应满足 `DAC数据 / 4095 × 3.3 = 1.0`，即 DAC 数据上限约为 1241。

## 3. 功能需求

- **PR-001 DAC 模拟电压输出（锯齿波生成）**：使用 DAC1 通道 1（PA4 引脚）输出模拟电压，周期性更新 DAC 转换值，生成锯齿波模拟信号。锯齿波周期 20Hz、幅值 1.0V；更新步进不做要求。
- **PR-002 锯齿波输出周期指示**：通过 PA5 引脚输出指示信号，指示锯齿波的输出周期。PA5 作为 GPIO 输出使用，与板载用户 LED LD1 复用，LD1 高电平点亮、低电平熄灭。

## 4. 实现思路

整个实验分为两个阶段，分别对应上述两条功能需求：

1. **生成锯齿波（T01）**：系统初始化（`mx_system_init`）完成后，先通过 `HAL_DAC_StartChannel` 启动 DAC1 通道 1 输出。DAC 通道已由 STM32CubeMX 配置为"软件触发"模式——该模式下只要向数据保持寄存器（DHR）写入新数据，DAC 就会立刻按新数据转换输出。因此主循环中每隔 1ms 把 DAC 数据加上一个步进（25），当数据超过 1.0V 对应的峰值（1241）时归零重新递增。从 0 升到峰值约需要 50 步，每步 1ms，所以锯齿波周期约为 50ms（约 20Hz）。
2. **周期指示（T02）**：在主循环的每一次更新之后，根据当前 DAC 数据判断所处的周期相位：数据小于峰值一半时为前半周期，点亮 LED；否则为后半周期，熄灭 LED。这样 LED 的亮灭循环与锯齿波周期严格同步，LED 每个周期亮灭一次。

```text
主循环（每隔 1ms 执行一次）
├── 更新 DAC 数据：当前值 + 步进，超过峰值则回零   → PA4 输出锯齿波
└── 按周期相位控制 PA5：前半周期点亮，后半周期熄灭  → LED 指示周期
```

## 5. 关键代码

以下为 `main.c` 中的关键部分。首先是程序头部的参数定义和锯齿波更新函数：

```c
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "mx_dac1.h"
#include "mx_gpio_default.h"

/* Private define ------------------------------------------------------------*/
/* 12-bit DAC 幅值 1.0V 对应数据：1.0 / 3.3 * 4095 */
#define DAC_AMP_STEP              (25U)      /* 每步递增的 12-bit 数据量 */
#define DAC_AMP_MAX_DATA          (1241U)    /* 幅值 1.0V 对应的 12-bit 数据上限 */
```

```c
/**
  * @brief  更新一次 DAC 锯齿波转换数据：从 0 递增至峰值后回零。
  *         每步间隔 1ms，共 (DAC_AMP_MAX_DATA / DAC_AMP_STEP) 步，周期约 50ms(20Hz)。
  */
static void dac_sawtooth_update(uint32_t *sawtooth_level)
{
  *sawtooth_level += DAC_AMP_STEP;
  if (*sawtooth_level > DAC_AMP_MAX_DATA)
  {
    *sawtooth_level = 0U;   /* 达到峰值后回到零，形成齿状回零沿 */
  }
  (void)HAL_DAC_SetChannelData(mx_dac1_gethandle(), HAL_DAC_CHANNEL_1, *sawtooth_level);
}
```

然后是 `main` 函数中的启动与主循环：

```c
int main(void)
{
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }
  else
  {
    /* 启动 DAC1 通道 1 输出（软件触发，写入 DHR 即转换） */
    if (HAL_DAC_StartChannel(mx_dac1_gethandle(), HAL_DAC_CHANNEL_1) != HAL_OK)
    {
      while (1) {}
    }

    uint32_t sawtooth_level = 0U;   /* 当前 12-bit 锯齿波转换数据 */

    while (1)
    {
      /* 周期性更新 DAC 转换数据，生成锯齿波 */
      dac_sawtooth_update(&sawtooth_level);
      HAL_Delay(1U);               /* 每步间隔 1ms */

      /* 依据锯齿波周期相位控制 PA5/LD1，
         前半周期点亮、后半周期熄灭，指示信号周期与锯齿波一致(约50ms/20Hz) */
      if (sawtooth_level < (DAC_AMP_MAX_DATA / 2U))
      {
        HAL_GPIO_WritePin(LD1_PORT, LD1_PIN, LD1_ACTIVE_STATE);
      }
      else
      {
        HAL_GPIO_WritePin(LD1_PORT, LD1_PIN, LD1_INACTIVE_STATE);
      }
    }
  }
}
```

代码说明：

- `mx_dac1_gethandle()` 返回 STM32CubeMX 生成代码中 DAC1 的句柄，通过它调用 HAL 函数操作 DAC。
- `HAL_DAC_SetChannelData()` 向 DAC1 通道 1 写入 12-bit 数据；由于通道配置为软件触发（`HAL_DAC_TRIGGER_NONE`），写入后立即转换输出对应电压。
- `LD1_PORT`、`LD1_PIN`、`LD1_ACTIVE_STATE`、`LD1_INACTIVE_STATE` 是 STM32CubeMX 生成的头文件 `mx_gpio_default.h` 中定义好的宏，分别表示 LED 的端口、引脚、点亮电平和熄灭电平。

## 6. HAL 依赖前提

本工程由 STM32CubeMX2 生成，DAC 与 GPIO 的初始化代码位于 `generated/hal/` 目录，主程序依赖以下前提：

- **外设初始化已生成**：`mx_dac1_init()` 完成 DAC1 配置（12-bit 右对齐、软件触发、输出缓冲使能、外部输出），并将 PA4 配置为模拟模式；`mx_gpio_default_init()` 将 PA5 配置为推挽输出。二者由 `mx_system_init()` 在 `main()` 入口顺序调用。
- **本实验使用的 HAL 接口**：
  - `HAL_DAC_StartChannel()` — 启动 DAC 通道输出；
  - `HAL_DAC_SetChannelData()` — 写入 12-bit 转换数据；
  - `HAL_GPIO_WritePin()` — 设置 PA5 输出电平；
  - `HAL_Delay()` — 毫秒延时，用于控制锯齿波更新节奏。
- **时基已配置**：系统使用 SysTick 作为 HAL 时基，`HAL_Delay()` 可用。
- **工程编译依赖**：CMake 构建系统已包含 `main.c`、HAL 驱动库（`stm32c5xx_drivers`）与设备支持包（`stm32c5xx_dfp`），无需手动添加文件。

## 7. 编译烧录步骤

工程使用 CMake + Ninja 构建，预设名为 `debug_GCC_STM32C542RCT6`。

1. **编译**：在工程根目录执行以下命令：
   ```bash
   cmake --preset debug_GCC_STM32C542RCT6
   cmake --build --preset debug_GCC_STM32C542RCT6
   ```
   构建产物（`DAC_Output.elf`、`DAC_Output.bin`、`DAC_Output.hex`）输出到 `build/debug_GCC_STM32C542RCT6/` 目录。

2. **烧录**：用 USB 线将开发板连接到电脑（板载 ST-LINK/V3EC），再选用以下任一方式：
   - 使用 **STM32CubeProgrammer**，选择 ST-LINK 连接，加载 `build/debug_GCC_STM32C542RCT6/DAC_Output.elf`（或 .hex）并执行下载；
   - 或在命令行使用 `st-flash write build/debug_GCC_STM32C542RCT6/DAC_Output.bin 0x08000000` 写入；
   - 或在 VS Code 中按 F5 使用 ST-Link GDB 调试启动（工程已自带 `launch.json` 配置）。

3. **运行**：烧录完成后按开发板复位键，程序开始运行：PA4 输出锯齿波，板载 LED LD1 周期闪烁。

## 8. 测试结果

在硬件上完成编译、烧录并运行验证，测试结果如下：

- **PA4 锯齿波**：用示波器在 PA4 引脚测量，观察到周期约 50ms（约 20Hz）、峰谷电压差约 1.0V 的锯齿波，波形上升段递增、回零沿下降，与预期一致。
- **PA5/LED 周期指示**：板载 LED LD1 以约 20Hz 周期闪烁；用示波器在 PA5 引脚测得周期约 50ms 的方波指示信号，且与 PA4 锯齿波周期同步。
- **结论**：编译烧录正确，运行验证测试正确，示波器波形和参数与预期一致，两项功能需求（PR-001、PR-002）均实现并通过验证。
