# 定时器 PWM 输出实验

## 1. 项目概述

本实验基于 NUCLEO-C542RC 开发板（MCU 为 STM32C542RCT6），使用定时器 **TIM2 通道1** 输出 PWM 信号来驱动板载用户 LED（PA5/LD1）。PWM 频率约为 1 kHz，占空比在 10%~90% 之间周期性往复变化（一个完整往复周期约为 2 s），从而让 LED 实现"呼吸灯"效果：由暗渐亮、再由亮渐暗，循环往复。

项目围绕两层功能增量展开：
- **T01 启动 PWM 输出**：先让 LED 以固定占空比（约 50%）稳定点亮，验证 TIM2_CH1 的 PWM 输出链路正常。
- **T02 呼吸灯效果**：在 PWM 正常输出的基础上，让占空比在 10%~90% 间平滑往复变化，实现呼吸灯。

## 2. 硬件说明

| 硬件项 | 说明 |
|---|---|
| 开发板 | NUCLEO-C542RC（Nucleo-64，板卡编号 MB2213） |
| MCU | STM32C542RCT6（Cortex-M33，最大主频 144 MHz，LQFP64） |
| 板载 LED | LD1，连接到 PA5 |
| 输出通道 | TIM2 通道1（TIM2_CH1），复用映射到 PA5 |

关键连接关系：

- **PA5（LD1）**：在 STM32CubeMX2 中配置为 TIM2_CH1 的复用功能（复用号 AF1），代替普通 GPIO 输出。
- **TIM2**：作为 PWM 输出定时器，内部时钟驱动，产生约 1 kHz 的 PWM 基频。

> 提示：LED 亮度由占空比决定。占空比越高，单位时间内 LED 点亮的时间比例越大，人眼看起来越亮。

## 3. 功能需求

| 编号 | 需求 | 说明 |
|---|---|---|
| PR-001 | PWM 输出 | 使用 TIM2_CH1 输出 PWM，驱动 PA5 板载 LED，PWM 频率约为 1 kHz |
| PR-002 | 呼吸灯效果 | 占空比在 10%~90% 之间周期性往复变化，变化周期约为 2 s |

## 4. 实现思路

整个工程由 STM32CubeMX2 生成初始框架（时钟、GPIO、TIM2 等初始化代码），用户只需在 `main.c` 的 `main()` 函数中补充应用逻辑。

**PWM 输出的关键步骤**（T01）：
1. 通过 `mx_tim2_gethandle()` 获取 TIM2 的句柄。
2. 设置 CH1 比较单元的占空比脉冲值（如 50%）。
3. 启动 TIM2 计数器（使 PWM 时基运行）。
4. 启动 TIM2_CH1 输出通道（把 PWM 信号真正送到 PA5）。

**呼吸灯的实现思路**（T02）：
以 SysTick 产生的 HAL 毫秒时基（`HAL_GetTick()`）作为时间基准，在 `while(1)` 主循环中**非阻塞**地周期性更新占空比。占空比在 10%~90% 之间线性递增，到达 90% 后切换方向递减回 10%，如此往复；一个完整的"由暗到亮再到暗"周期约为 2 s（上半段约 1 s，下半段约 1 s）。

## 5. 关键代码

以下代码位于 `main.c` 的 `main()` 函数中。系统初始化成功后，先启动 PWM，再进入呼吸灯循环。

**（1）启动 TIM2_CH1 PWM 输出**

```c
/* T01: 启动 TIM2_CH1 PWM 输出，固定占空比约 50% */
hal_tim_handle_t *htim2 = mx_tim2_gethandle();

/* 设置 CH1 占空比脉冲（初始值 50%，匹配 ARR=99） */
(void)HAL_TIM_OC_SetCompareUnitPulse(htim2, HAL_TIM_OC_COMPARE_UNIT_1, TIM2_CH1_DUTY_PULSE);

/* 启动 TIM2 计数器（HAL_TIM_OC_StartChannel 仅使能输出通道，不启动计数器） */
(void)HAL_TIM_Start(htim2);

/* 启动 TIM2_CH1 PWM 输出 */
(void)HAL_TIM_OC_StartChannel(htim2, HAL_TIM_CHANNEL_1);
```

**（2）呼吸灯效果（占空比周期性往复）**

```c
/* T02: 呼吸灯状态初始化 */
uint32_t breath_cycle_start = HAL_GetTick();   /* 当前呼吸半周期起点时刻 */
uint32_t breath_rising      = 1U;              /* 1=占空比递增，0=递减 */

while (1)
{
  /* T02: 基于 SysTick 时基更新呼吸灯占空比（非阻塞，约2s往复） */
  uint32_t now = HAL_GetTick();
  uint32_t elapsed = now - breath_cycle_start;

  if (elapsed >= TIM2_BREATH_HALF_MS)
  {
    /* 越过半周期，切换方向并重置半周期起点 */
    breath_rising = !breath_rising;
    breath_cycle_start = now;
    elapsed = 0U;
  }

  /* 在当前半周期内线性计算占空比脉冲值（10%~90%） */
  uint32_t range = TIM2_DUTY_PULSE_MAX - TIM2_DUTY_PULSE_MIN;
  uint32_t offset = (range * elapsed) / TIM2_BREATH_HALF_MS;
  uint32_t pulse = breath_rising
                   ? (TIM2_DUTY_PULSE_MIN + offset)   /* 递增：由暗渐亮 */
                   : (TIM2_DUTY_PULSE_MAX - offset);  /* 递减：由亮渐暗 */

  (void)HAL_TIM_OC_SetCompareUnitPulse(htim2, HAL_TIM_OC_COMPARE_UNIT_1, pulse);
}
```

**（3）占空比相关参数宏定义（文件顶部）**

```c
/* T01: 启动 TIM2_CH1 PWM 输出，固定占空比约 50% */
#define TIM2_CH1_DUTY_PULSE  50U   /* 占空比脉冲值（ARR=99，50/100 ≈ 50%） */

/* T02: 呼吸灯占空比变化参数（ARR=99，脉冲值即占空比百分比） */
#define TIM2_DUTY_PULSE_MIN   10U   /* 最小占空比脉冲值（10%） */
#define TIM2_DUTY_PULSE_MAX   90U   /* 最大占空比脉冲值（90%） */
#define TIM2_BREATH_HALF_MS   1000U /* 半周期时长（由暗到亮或由亮到暗，1s） */
#define TIM2_BREATH_CYCLE_MS  (2U * TIM2_BREATH_HALF_MS) /* 完整呼吸周期（约2s） */
```

> **说明**：ARR 自动重载值为 99，占空比脉冲值即 `CCR` 比较值，`占空比 = CCR / (ARR+1)`。因此脉冲值 50 对应约 50% 占空比，脉冲值范围 10~90 对应 10%~90% 占空比。

## 6. HAL 依赖前提

本项目基于 STM32CubeMX2 生成的 HAL 驱动，在 `generated/hal/stm32c5xx_hal_conf.h` 中已使能对应模块：

| 依赖 | 说明 |
|---|---|
| HAL TIM 模块 | `USE_HAL_TIM_MODULE`，提供 TIM 初始化、PWM 输出、比较单元脉冲设置等接口 |
| SysTick 时基 | 系统时基由 SysTick 驱动，`SysTick_Handler` 调用 `HAL_IncTick()`，供 `HAL_GetTick()` 提供毫秒计数，用于呼吸灯时间调度 |

**初始化前提**：`main()` 中必须先调用 `mx_system_init()`（内部完成 RCC 时钟、GPIO、TIM2 等初始化）并确认返回 `SYSTEM_OK`，之后才能使用 TIM2 句柄和 PWM 相关接口。

**TIM2 配置前提**（由 STM32CubeMX2 生成在 `generated/hal/mx_tim2.c`）：
- 预分频 PSC = 1439，自动重载 ARR = 99，向上计数 → PWM 频率约 1 kHz（144 MHz /（1440 × 100））。
- CH1 为 PWM 模式 1、高电平有效，PA5 配置为 TIM2_CH1 复用功能（AF1）。

## 7. 编译烧录步骤

1. **打开工程**：使用 STM32CubeMX2 打开 `TIM_PWM_1kHz.ioc2`，生成并初始工程（如已配置可跳过）。
2. **配置构建**：工程采用 CMake + Ninja 构建系统，预设为 `debug_GCC_STM32C542RCT6`（GCC 工具链）。
3. **编译**：
   - 在工程目录执行 CMake 配置：`cmake --preset debug_GCC_STM32C542RCT6`
   - 构建：`cmake --build build/debug_GCC_STM32C542RCT6`
   - 或者使用 IDE（VS Code）的 CMake 工具进行一键构建。
4. **烧录**：将编译生成的 `.elf`/`.bin`/`.hex` 文件通过板载 ST-LINK 烧录到开发板（可用 IDE 的烧录功能或 ST-LINK 命令行工具）。
5. **复位运行**：烧录完成后复位开发板，观察 LED 呼吸灯效果。

> 编译、烧录与调试属于验证手段，建议按第 8 节逐项核对测试结果。

## 8. 测试结果

| 阶段 | 测试项 | 结果 |
|---|---|---|
| T01 | 编译烧录 | 通过，无错误 |
| T01 | LED 状态 | LD1 半亮度稳定点亮 |
| T01 | PA5 波形（示波器） | 约 1 kHz、约 50% 占空比的 PWM 波形 |
| T02 | 编译烧录 | 通过，无错误 |
| T02 | 呼吸灯效果 | LED 亮度在约 2 s 周期内由暗渐亮、由亮渐暗，往复循环，变化平滑无跳变 |

实验全部功能目标（PR-001、PR-002）均已实现并经硬件运行验证通过。
