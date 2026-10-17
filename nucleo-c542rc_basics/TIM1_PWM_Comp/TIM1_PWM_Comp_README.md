# TIM1_PWM_Comp实验

## 1. 项目概述

- 开发板：NUCLEO-C542RC
- MCU：STM32C542RCT6（Cortex-M33，最高 144 MHz）
- 功能：利用**高级定时器 TIM1** 的 **CH1 / CH1N 两个互补输出通道**，分别通过 **PA8** 与 **PB13** 输出：
  - 频率 **10 kHz**、占空比 **50%** 的两路 PWM 信号；
  - 两路信号呈**互补关系**（一路为高时另一路为低）；
  - 在切换边沿之间插入约 **500 ns** 的死区时间。

"互补输出 + 死区"是电机驱动、逆变器等电路的标准需求：互补的两路 PWM 分别驱动半桥的上下两个开关管，死区时间确保上下管**不会同时导通**，避免直通短路。

工程由 **STM32CubeMX2** 生成，HAL 驱动库完成底层初始化。TIM1 的配置已在 CubeMX 中完成，本次编程只修改了用户文件 `main.c`（添加启动 TIM1 互补输出通道与计数器的代码）。

## 2. 硬件说明

| 硬件 | 说明 |
| --- | --- |
| 开发板 | NUCLEO-C542RC（板卡编号 MB2213） |
| 主控芯片 | STM32C542RCT6（Cortex-M33 内核，LQFP64 封装） |
| 系统时钟 | 144 MHz（内部 PSIS 时钟，TIM1 内核时钟同为 144 MHz） |
| 定时器 | **TIM1（高级定时器）**，具有互补输出与死区时间发生器 |
| 输出通道 | **CH1 → PA8**、**CH1N（互补）→ PB13**，均配置为复用推挽输出 |
| 观测工具 | 示波器（两通道分别接 PA8、PB13） |

与普通定时器（如 TIM2、TIM6）不同，**TIM1 是"高级定时器"**，才具备：

- **互补输出通道**（CHxN，如 CH1N）；
- **死区时间发生器**（在上下管切换之间插入一段两路均为低电平的空白间隔）；
- **主输出使能 MOE**（配合"断路 Break"功能；本实验未使用断路输入）。

## 3. 功能需求

1. TIM1 配置为 **PWM 模式 1**，输出频率 **10 kHz**。
2. 使能 CH1（PA8）与其互补通道 CH1N（PB13），两路输出**互补 PWM**。
3. 占空比 **50%**。
4. 死区时间约 **500 ns**。
5. 主输出使能 MOE 需由**软件启动**（CubeMX 配置 `automatic_output=false`，不会自动使能输出）。
6. 启动顺序：先启动输出通道（MOE 就位），再启动计数器，避免启动瞬间产生毛刺脉冲。

## 4. 实现思路

### 4.1 互补 PWM 与死区是怎么产生的

TIM1 把一个通道的原始参考信号 OC1REF（由计数器与比较寄存器 CCR1 比较而成）分成两路：

- 一路经极性配置后作为 **CH1**（PA8）输出；
- 另一路**取反**后作为 **CH1N**（PB13）输出，形成互补关系。

在上下管切换的关口，**死区发生器**会在每个边沿插入一段延时（本实验 500 ns），使两路在这一小段时间内**都是低电平**（两管都不导通），从而避免直通。死区时间由 TIM1 的**死区寄存器 DTG** 配置，单位为"采样时钟 DTS 的倍数"。

### 4.2 频率与占空比的计算

TIM1 内核时钟 = **144 MHz**，配置如下（来自 CubeMX 生成的 `mx_tim1.c`）：

- 预分频 `PSC = 11` → 计数时钟 = 144 MHz / (11 + 1) = **12 MHz**；
- 自动重装 `ARR = 0x4AF = 1199` → 一个周期 = 1199 + 1 = **1200 个计数**；
- PWM 频率 = 12 MHz / 1200 = **10 kHz** ✓
- 比较值 `CCR1 = 0x258 = 600` → 占空比 = 600 / 1200 = **50%** ✓

### 4.3 死区时间的计算

死区采样时钟 DTS 配置为 **144 MHz、不分频**（DTS_DIV1）：

```text
死区时间 = 死区寄存器数值 / DTS = 72 / 144 MHz = 0.5 µs = 500 ns ✓
```

### 4.4 启动流程（本次编程的核心）

CubeMX 只负责**初始化**（配置寄存器、引脚），但 `automatic_output=false` 意味着**主输出 MOE 不会自动开启**，需要 `main.c` 手动启动通道与计数器：

1. 取 TIM1 句柄（`mx_tim1_gethandle()`）；
2. **启动 CH1 与 CH1N 输出通道**：使能 CC1/CC1N 输出并开启主输出 MOE；
3. **启动计数器**：计数开始运行，PWM 波形才真正产生。

顺序上先开通道、后开计数器；这样 MOE 先就位，计数器一开始走就输出稳定波形，避免启动瞬间多出一个毛刺脉冲。

## 5. 关键代码

工程为 STM32CubeMX2 生成的新式工程结构，初始化由 `mx_system_init()` 统一完成。用户只需在 `main.c` 中添加启动代码。

### 5.1 主函数：启动互补 PWM

```c
#include "main.h"
#include "mx_tim1.h"   /* 提供 mx_tim1_gethandle() 与通道宏 */

int main(void)
{
  if (mx_system_init() != SYSTEM_OK)   /* CubeMX2：时钟、GPIO、TIM1 等初始化 */
  {
    return (-1);
  }
  else
  {
    hal_tim_handle_t *htim1 = mx_tim1_gethandle();   /* 获取 TIM1 句柄 */

    /* 1) 启动输出通道：使能 CH1/CH1N 并开启主输出使能 MOE。
          automatic_output=false，MOE 必须由软件开启，否则引脚无输出。 */
    if (HAL_TIM_OC_StartChannel(htim1, TIM1_CH1_CHANNEL) != HAL_OK)
    {
      return (-1);
    }
    if (HAL_TIM_OC_StartChannel(htim1, TIM1_CH1N_CHANNEL) != HAL_OK)
    {
      return (-1);
    }

    /* 2) 主输出就绪后启动计数器，PWM 波形才真正产生（10 kHz / 50% / 500ns 死区）。 */
    if (HAL_TIM_Start(htim1) != HAL_OK)
    {
      return (-1);
    }

    while (1) {}   /* 输出由定时器硬件持续产生，主循环保持空转 */
  }
}
```

### 5.2 TIM1 初始化（由 CubeMX2 生成，无需修改）

频率 / 占空比 / 死区都在生成文件 `generated/hal/mx_tim1.c` 中设置，本工程**未改动**它。核心配置片段：

```c
/* 10 kHz：预分频 11，重装值 0x4AF（1199） */
config.prescaler              = 11;
config.period                 = 0x4AF;   /* → 144M/12/1200 = 10 kHz */

/* 死区采样时钟 144 MHz，不分频 */
HAL_TIM_SetDTSPrescaler(&hTIM1, HAL_TIM_DTS_DIV1);

/* 50% 占空比：比较值 0x258（600），PWM 模式 1 */
oc_compare_unit_config.mode  = HAL_TIM_OC_PWM1;
oc_compare_unit_config.pulse = 0x258;    /* → 600/1200 = 50% */

/* 死区：72 个 DTS 计数 = 72/144 MHz = 500 ns */
HAL_TIM_SetDeadtime(&hTIM1, 72, 72);
```

> 若在 CubeMX 中重新设置这些值，公式为：
> `频率 = TIM1时钟 / (PSC+1) / (ARR+1)`、`占空比 = CCR1 / (ARR+1)`、`死区 = 死区寄存器数值 / DTS`。

## 6. HAL 依赖前提

本工程使用 STM32CubeMX2 新式 HAL 驱动，与经典 CubeMX/HAL 有几处不同，初学者请留意：

1. **不再有 `HAL_TIM_PWM_Start()`**：CubeMX2 的 HAL 把"启动通道"与"启动计数器"拆成两个函数：
   - `HAL_TIM_OC_StartChannel(htim, channel)` —— 使能某个输出通道并开启主输出 MOE；
   - `HAL_TIM_Start(htim)` —— 启动定时器计数器（CEN）。
   - 两者都要调用，互补 PWM 才能输出。
2. **句柄访问**：用 `mx_tim1_gethandle()` 取得 `hal_tim_handle_t *`（此句柄在 `mx_system_init() → mx_tim1_init()` 中已初始化）。
3. **通道宏**：互补通道用 `HAL_TIM_CHANNEL_1N`（即 CH1N）。生成头文件 `mx_tim1.h` 还提供了别名 `TIM1_CH1_CHANNEL`、`TIM1_CH1N_CHANNEL`，也可以直接用它们。
4. **MOE 需软件开启**：CubeMX 中 `automatic_output=false`，必须显式调用 `HAL_TIM_OC_StartChannel()` 打开主输出，否则 PA8/PB13 一直无波形。
5. **时钟前提**：CPU 与 TIM1 均按 144 MHz（内部 PSIS 时钟）运行。请核对 `mx_rcc.c` 中 `SYSCLK=144 MHz`。死区精度完全取决于 DTS（144 MHz）。
6. `main.c` 需额外包含头文件 `mx_tim1.h`（不同于 `main.h` 自动涵盖，此处显式 include 更清晰）。

## 7. 编译烧录步骤

1. 用 STM32CubeMX2 打开工程根目录的 `TIM1_PWM_Comp.ioc2`，确认或修改 TIM1 配置（必要时重新生成代码；重新生成后请确认 `main.c` 中的启动代码仍保留，若被覆盖需重新添加上文 5.1 的代码）。
2. 编译：用 **STM32CubeIDE**（或 VS Code + Cube 工具链）按 CMake 预设 `debug_GCC_STM32C542RCT6` 编译，生成 `TIM1_PWM_Comp.elf`：

   ```bash
   # 若使用 VS Code + Cube CMake 预设
   cube-cmake -S . -B build/debug_GCC_STM32C542RCT6
   cube-cmake --build build/debug_GCC_STM32C542RCT6
   ```

3. 烧录：通过板载 **ST-Link** 的 SWD 接口将程序烧录到 MCU（STM32CubeProgrammer 或 IDE 内烧录按钮均可）。
4. 复位运行，用示波器测量 PA8 与 PB13。

## 8. 测试结果

已编译并烧录到 NUCLEO-C542RC 运行验证：

- **波形频率**：两路 PWM 信号频率均为 **10 kHz** ✓
- **互补关系**：CH1（PA8）与 CH1N（PB13）呈**互补输出**关系——一路为高时另一路为低 ✓
- **占空比**：两路均约 **50%** ✓
- **死区**：在两路之间的切换边沿可观察到明显的死区间隔，时间约 **500 ns**（上下管切换时两路均为低电平的空白段）✓
- **构建验证**：工程编译通过，烧录、运行正常。
