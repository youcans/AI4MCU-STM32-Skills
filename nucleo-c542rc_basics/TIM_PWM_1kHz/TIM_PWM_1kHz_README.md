# TIM_PWM_1kHz 定时器 PWM 输出实验(呼吸灯)

> 面向嵌入式初学者的编程说明。读者应具备 STM32 基础、C 语言基础,并能用 STM32CubeMX2 + STM32CubeIDE 生成与烧录工程。

## 1. 项目概述

本项目在 **NUCLEO-C542RC** 开发板(STM32C542RCT6)上,利用定时器 TIM6 输出频率约 **1 kHz** 的 PWM 信号,驱动板载 LED(LD1,引脚 **PA5**),并把 PWM 占空比在 **10% ~ 90%** 之间周期性往复变化,周期约 **2 s**,实现"呼吸灯"效果——LED 亮度由暗渐亮、再由亮渐暗,循环往复。

整个工程由 **STM32CubeMX2** 生成,**HAL 驱动库** 完成底层初始化,本次任务只修改了用户文件 `main.c`。

## 2. 硬件说明

| 硬件 | 说明 |
| --- | --- |
| 开发板 | NUCLEO-C542RC |
| 主控芯片 | STM32C542RCT6(Cortex-M33 内核) |
| 系统时钟 | 144 MHz(内部 PSIS 时钟) |
| 板载 LED | LD1,引脚 **PA5**,高电平点亮(active high) |
| 板载按键 | B1,引脚 PC13(与 LED 无关,本次未使用) |
| 定时器 | TIM6(**基本定时器**),内部时钟 144 MHz |

> 特别注意:**TIM6 是基本定时器(basic timer)**,它**没有硬件 PWM 输出通道**,不能像高级定时器 TIM1 那样把 PWM 直接接到引脚上。LED 引脚 PA5 在 CubeMX 中被配置为**普通 GPIO 推挽输出**。因此本实验的 PWM 是**软件 PWM**——由 TIM6 的更新中断周期性地翻转 PA5 电平,配合占空比控制来模拟 PWM 波形(详见第 5 节)。

## 3. 功能需求

1. TIM6 配置为定时**1 ms** 触发一次更新中断(即中断频率 1 kHz)。
2. 用软件在 PA5 上产生占空比可调的 PWM,载波周期约 **20 ms**(刷新率约 50 Hz,肉眼无闪烁)。
3. PWM 占空比在 **10% ~ 90%** 之间以三角波形式往复变化:
   - 前半周期约 1 s,占空比从 10% 逐渐升到 90%(LED 渐亮);
   - 后半周期约 1 s,占空比从 90% 逐渐降到 10%(LED 渐暗);
   - 完整呼吸周期约 **2 s**。
4. 示波器在 PA5 观测波形正常(脉冲宽度随时间呈三角波规律变化)。

## 4. 实现思路

### 4.1 为什么用软件 PWM

由于 TIM6 是基本定时器、没有硬件 PWM 通道,而 PA5 又是 GPIO 输出,所以采用**软件 PWM**:

- TIM6 每 1 ms 进入一次更新中断(`HAL_TIM_UpdateCallback`)。
- 我们把连续 **20 个 1 ms 中断**组成一帧 PWM 载波(20 ms),每帧内：
  - 前 `duty_ticks` 个中断让 PA5 = 高电平(LED 亮);
  - 剩余 `20 - duty_ticks` 个中断让 PA5 = 低电平(LED 灭)。
- `duty_ticks` 取值 $0..20$,对应占空比 $0\%..100\%$,分辨率 **5%**。本实验限制在 10%(`duty_ticks = 2`)~ 90%(`duty_ticks = 18`)。

### 4.2 呼吸调制(三角波)

用一个 `breath_ms` 计数器在 1 ms 中断里累加,周期 $T = 2000$ ms:

- 前半段($0 \le breath\_ms < 1000$):占空比线性上升(10% → 90%);
- 后半段($1000 \le breath\_ms < 2000$):占空比线性下降(90% → 10%)。

即把 `breath_ms` 映射为一条三角形函数,再换算成 `duty_ticks`,LED 亮度随之循环明暗。

### 4.3 关键结论

```text
TIM6 中断频率  = 144 MHz / (1439 + 1) / (99 + 1) = 144e6 / 1440 / 100 = 1000 Hz
软件 PWM 载波  = 1000 Hz / 20 ≈ 50 Hz(帧长 20 ms)
呼吸周期       = 2 s(占空比 10% ↔ 90% 三角波)
```

## 5. 关键代码

工程为 STM32CubeMX2 生成的新式工程结构,初始化由 `mx_system_init()` 统一完成,用户只需在 `main.c` 中启动定时器中断并实现回调。

### 5.1 用户自定义参数

```c
/* Software-PWM 载波:由 TIM6 的 1 kHz 更新中断分帧构建。
   一帧 SOFT_PWM_STEPS 个 1 ms 滴答 = 20 ms(刷新率约 50 Hz)。
   每帧前 <duty_ticks> 个滴答 LED 亮,其余灭,即 5% 步进的占空比。 */
#define SOFT_PWM_STEPS          20U   /* 20 x 1 ms = 20 ms 载波帧               */
#define BREATH_PERIOD_MS        2000U /* 完整呼吸周期(10%->90%->10%)            */
#define MIN_DUTY_TICKS          2U    /* 10% 占空比(20 格中的 2 格)             */
#define MAX_DUTY_TICKS          18U   /* 90% 占空比(20 格中的 18 格)            */
```

### 5.2 更新中断回调(软件 PWM + 三角波呼吸)

```c
/**
  * @brief TIM6 更新中断回调(覆写 HAL 的 __WEAK 弱函数)。
  *        每 1 ms 调用一次。在 LD1(PA5)上输出软件 PWM,并按三角波
  *        调制占空比(10% <-> 90%),周期 BREATH_PERIOD_MS,实现呼吸灯。
  */
void HAL_TIM_UpdateCallback(hal_tim_handle_t *htim)
{
  static uint16_t breath_ms   = 0U;   /* 呼吸周期内位置 0..BREATH_PERIOD_MS-1 */
  static uint8_t  duty_ticks  = MIN_DUTY_TICKS;
  static uint8_t  frame_tick  = 0U;   /* 一帧内 0..SOFT_PWM_STEPS-1            */

  /* 每 1 ms 前进一步,走完整个呼吸周期后回绕。 */
  breath_ms++;
  if (breath_ms >= BREATH_PERIOD_MS)
  {
    breath_ms = 0U;
  }

  /* 三角波:周期边缘回到 10%,半周期中点达到 90%。 */
  uint16_t half = BREATH_PERIOD_MS / 2U;
  uint16_t ph   = (breath_ms < half) ? breath_ms : (BREATH_PERIOD_MS - breath_ms);
  duty_ticks    = MIN_DUTY_TICKS +
                  ((uint32_t)(MAX_DUTY_TICKS - MIN_DUTY_TICKS) * ph) / half;

  /* 分帧伺服软件 PWM 载波。 */
  frame_tick++;
  if (frame_tick >= SOFT_PWM_STEPS)
  {
    frame_tick = 0U;
  }

  HAL_GPIO_WritePin(LD1_PORT, LD1_PIN,
                    (frame_tick < duty_ticks) ? HAL_GPIO_PIN_SET
                                              : HAL_GPIO_PIN_RESET);
}
```

### 5.3 主函数:启动 TIM6 更新中断

```c
int main(void)
{
  if (mx_system_init() != SYSTEM_OK)   /* CubeMX2 生成:时钟、GPIO、TIM6 初始化 */
  {
    return (-1);
  }
  else
  {
    /* 取 TIM6 句柄并启动更新中断。
       TIM6 以 144 MHz / 1440 / 100 = 1 kHz 触发,
       HAL_TIM_UpdateCallback() 在中断里驱动呼吸 PWM。 */
    hal_tim_handle_t *htim6 = mx_tim6_gethandle();
    if (HAL_TIM_Start_IT(htim6) != HAL_OK)
    {
      return (-1);
    }

    while (1) {}   /* 任务都在中断回调里完成,主循环保持空转 */
  }
}
```

## 6. HAL 依赖前提

本工程使用 STM32CubeMX2 新式 HAL 驱动(CubeMX2 2.1.0),与经典 CubeMX/HAL 有几处不同,初学者请留意:

1. **弱回调覆写方式**:TIM6 的更新中断由 `HAL_TIM_IRQHandler()` 分发给**弱回调** `HAL_TIM_UpdateCallback(hal_tim_handle_t *htim)`,我们在 `main.c` 中**直接定义同名函数**即可覆写,无需额外注册。
   - 工程配置中 `USE_HAL_TIM_REGISTER_CALLBACKS` 为 `0`,因此不使用 `HAL_TIM_RegisterUpdateCallback()`。
2. **句柄访问**:通过 `mx_tim6_gethandle()` 取得 `hal_tim_handle_t *`,再调用 `HAL_TIM_Start_IT()` 启动更新中断。
3. **GPIO 操作**:使用 `HAL_GPIO_WritePin(LD1_PORT, LD1_PIN, HAL_GPIO_PIN_SET/RESET)`。`LD1_PORT`/`LD1_PIN` 由 CubeMX2 在 `mx_gpio_default.h` 中自动定义为 `HAL_GPIOA` / `HAL_GPIO_PIN_5`。
4. **时钟前提**:TIM6 使用内部时钟 144 MHz(PSIS),CubeMX 已配置 Prescaler = 1439、Period = 99,得到 1 kHz 更新频率。`mx_system_init()` 已代为完成 RCC、GPIO、NVIC、TIM6 的全部初始化。
5. `main.h` 已通过 `mx_hal_def.h` 间接包含所用头文件,`main.c` 无需额外包含。

## 7. 编译烧录步骤

1. 用 STM32CubeMX2 打开工程根目录的 `TIM_PWM_1kHz.ioc2` 进行时钟/外设配置(必要时重新生成代码)。
2. 用 **STM32CubeIDE**(或 VS Code + Cube 工具链)打开工程目录打开并编译,生成 `TIM_PWM_1kHz.elf`。
3. 通过板载 ST-Link 的 SWD 接口将程序烧录到 MCU(STM32CubeProgrammer 或 IDE 内的烧录按钮均可)。
4. 复位运行:观察板载 LED(LD1)呈呼吸灯效果。

> 若使用 VS Code + Cube CMake 预设,构建命令等价于:
>
> ```bash
> cube-cmake -S . -B build/debug_GCC_STM32C542RCT6
> cube-cmake --build build/debug_GCC_STM32C542RCT6
> ```

## 8. 测试结果

已烧录到 NUCLEO-C542RC 运行验证:

- **LED 现象**:板载 LD1 呈**呼吸灯**效果,亮度由暗到亮、再逐渐变暗,循环往复,周期约 **2 s**。
- **波形观测**:用示波器测量 PA5,观察到 **约 1 kHz** 的 PWM 脉冲,脉宽随时间呈**三角波规律**变化:脉宽逐渐变大(占空比 10% → 90%),随后逐渐变小(90% → 10%),波形正常。
- **构建验证**:工程编译通过,烧录、运行正常。

## 附:常见问题排查

| 现象 | 可能原因 | 解决 |
| --- | --- | --- |
| LED 不亮 | TIM6 更新中断未启动 | 确认 `main()` 调用了 `HAL_TIM_Start_IT()` |
| 无 1 kHz 波形 | 时钟/分频配置被改动 | 核对 Prescaler=1439、Period=99、SYSCLK=144 MHz |
| LED 闪烁明显不柔和 | 载波帧过长 | 调小 `SOFT_PWM_STEPS`(如 10),提高刷新率 |
| 呼吸速度不对 | `BREATH_PERIOD_MS` 被改动 | 恢复为 2000 ms(三角波半周期 1000 ms) |
