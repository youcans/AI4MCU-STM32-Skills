# TIM_IT_500ms实验

## 1. 项目概述

本实验在 NUCLEO-C542RC 开发板上，使用 STM32C542RCT6 芯片的 **TIM6 基本定时器**产生周期中断，驱动板载用户 LED（PA5）周期性闪烁。

每次 TIM6 周期事件到来（周期 500ms），就在中断回调中翻转一次 LED 输出状态，因此 LED 会以「亮 500ms → 灭 500ms」的节奏循环闪烁（一个完整的亮灭周期为 1s）。

本项目面向嵌入式初学者，目标是理解定时器周期中断在 STM32 中的基本用法。

## 2. 硬件说明

- 开发板：NUCLEO-C542RC（STM32 Nucleo-64 板型，板卡编号 MB2213）
- MCU：STM32C542RCT6
  - 内核：Arm Cortex-M33，最大主频 144 MHz
  - Flash：256 KB，SRAM：64 KB
- 板载用户 LED：LD1，连接在 MCU 的 PA5 引脚（GPIO 推挽输出）
- 上电初态：PA5 输出低电平（定义在 `LD1_INACTIVE_STATE`）

工程中与实验相关的引脚定义如下（定义见 `generated/hal/mx_gpio_default.h`）：

| 宏名 | 值 | 含义 |
|---|---|---|
| `LD1_PORT` | `HAL_GPIOA` | 端口 A |
| `LD1_PIN` | `HAL_GPIO_PIN_5` | 引脚 5（PA5） |
| `LD1_INIT_STATE` | `HAL_GPIO_PIN_RESET` | 初始输出低电平 |
| `LD1_ACTIVE_STATE` | `HAL_GPIO_PIN_SET` | LED 激活电平（高电平点亮） |
| `LD1_INACTIVE_STATE` | `HAL_GPIO_PIN_RESET` | 非激活电平 |

## 3. 功能需求

- 使用 TIM6 定时器产生周期中断，中断周期为 500ms。
- 每次进入定时器周期中断时，翻转板载用户 LED（PA5）的输出状态。
- LED 状态切换仅由 TIM6 周期中断驱动，无其它外部触发来源。
- 周期 500ms 允许存在定时器时钟配置带来的正常偏差。

## 4. 实现思路

### 4.1 TIM6 的时基如何算出 500ms

TIM6 的输入时钟来自 APB1（PCLK1）。工程将系统时钟 SYSCLK 配置为 144 MHz，APB1 分频为 1，因此 TIM6 输入时钟为 144 MHz。

在 `generated/hal/mx_tim6.c` 中已由 STM32CubeMX2 配置好分频：

- 预分频寄存器（PSC）= `14399`，即把 144 MHz 分为 144 MHz / (14399 + 1) = **144 MHz / 14400 = 10 kHz**
- 自动重装载寄存器（ARR）= `0x1387` = 4999，即计数到 4999 + 1 = 5000 个计数后产生更新事件

所以更新事件（即周期中断）频率为：

```
10 kHz / 5000 = 2 Hz  ⟹  周期 = 1 / 2 Hz = 500 ms
```

### 4.2 程序流程

`main()` 先调用 `mx_system_init()` 完成系统初始化（时钟、GPIO、TIM6 配置、中断使能），随后我们在其中启动 TIM6 计数：

```text
main()
├── mx_system_init()          // 系统/时钟/GPIO/TIM6 配置
│   └── mx_tim6_init()        // 配置 TIM6 时基并使能 TIM6 中断
├── HAL_TIM_Start_IT()        // 启动 TIM6 计数并开启更新中断
└── while(1)                  // 主循环为空，任务都在中断里完成
```

TIM6 每满 500ms 产生一次更新事件，中断链路为：

```text
TIM6_IRQHandler
→ HAL_TIM_IRQHandler(&hTIM6)
→ HAL_TIM_UPD_IRQHandler
→ HAL_TIM_UpdateCallback(htim)     // 用户覆盖的弱函数
→ HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN)   // 翻转 LED
```

## 5. 关键代码

### 5.1 定时器时基配置（工程生成，摘自 `generated/hal/mx_tim6.c`）

```c
hal_tim_handle_t *mx_tim6_init(void)
{
  if (HAL_TIM_Init(&hTIM6, HAL_TIM6) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_TIM6_EnableClock();

  /* Timer configuration to reach the output frequency at 2 Hz */
  hal_tim_config_t config;
  config.prescaler              = 14399;              // 144MHz / 14400 = 10 kHz
  config.counter_mode           = HAL_TIM_COUNTER_UP;
  config.period                 = 0x1387;             // 4999 → 计数5000 → 2 Hz
  config.repetition_counter     = 0;
  config.clock_sel.clock_source = HAL_TIM_CLK_INTERNAL;
  if (HAL_TIM_SetConfig(&hTIM6, &config) != HAL_OK)
  {
    return NULL;
  }

  /* Enable the Timer global interrupt */
  HAL_CORTEX_NVIC_SetPriority(TIM6_IRQn, HAL_CORTEX_NVIC_PREEMP_PRIORITY_0, HAL_CORTEX_NVIC_SUB_PRIORITY_0);
  HAL_CORTEX_NVIC_EnableIRQ(TIM6_IRQn);

  return &hTIM6;
}
```

### 5.2 GPIO 配置（工程生成，摘自 `generated/hal/mx_gpio_default.c`）

```c
system_status_t mx_gpio_default_init(void)
{
  hal_gpio_config_t  gpio_config;

  HAL_RCC_GPIOA_EnableClock();

  /* Configure PA5 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = LD1_INIT_STATE;
  if (HAL_GPIO_Init(LD1_PORT, LD1_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  return SYSTEM_OK;
}
```

### 5.3 用户代码（本项目新增，位于 `main.c`）

启动 TIM6 周期计数：

```c
int main(void)
{
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }
  else
  {
    /* T01: Start TIM6 periodic counting with update interrupt
       (update period: 500 ms, see generated TIM6 configuration) */
    if (HAL_TIM_Start_IT(mx_tim6_gethandle()) != HAL_OK)
    {
      return (-2);
    }

    while (1) {}
  }
}
```

覆盖 HAL 弱回调，在每次周期中断中翻转 LED：

```c
void HAL_TIM_UpdateCallback(hal_tim_handle_t *htim)
{
  (void)htim;
  HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);
}
```

说明：本实验使用本工程的 HAL 接口。周期事件处理通过覆盖**弱函数** `HAL_TIM_UpdateCallback` 实现（而非传统 HAL1 的 `HAL_TIM_PeriodElapsedCallback`）。

## 6. HAL 依赖前提

- 本工程由 STM32CubeMX2 生成，使用对应版本的新 HAL（头文件含 `hal_tim_handle_t`、`hal_gpio_config_t` 等类型）。
- TIM6 时基与中断已在生成代码 `mx_tim6_init()` 中配置，PA5 已在 `mx_gpio_default_init()` 中配置为推挽输出，无需修改生成文件。
- 使用 `HAL_TIM_Start_IT()` 启动定时器并开启更新中断。
- 周期事件回调是 `__WEAK` 修饰的 `HAL_TIM_UpdateCallback()`，用户可在 `main.c` 中直接覆盖实现，无需注册接口（工程中 `USE_HAL_TIM_REGISTER_CALLBACKS` 为 0）。
- LED 引脚宏 `LD1_PORT` / `LD1_PIN` 来自 `mx_gpio_default.h`，翻转使用 `HAL_GPIO_TogglePin()`。

## 7. 编译烧录步骤

工程使用 CMake + Ninja 构建，工具链为 GCC。构建类型预设名为 `debug_GCC_STM32C542RCT6`。

1. 配置工程（生成编译系统）：

   ```bash
   cmake --preset debug_GCC_STM32C542RCT6
   ```

2. 编译工程：

   ```bash
   cmake --build --preset debug_GCC_STM32C542RCT6
   ```

   编译产物 ELF 文件位于 `build/debug_GCC_STM32C542RCT6/TIM_IT_500ms.elf`。

3. 烧录（通过板载 ST-LINK/V3EC，不同工具任选其一）：

   使用 STM32CubeProgrammer：

   ```bash
   STM32_Programmer_CLI -c port=SWD -w build/debug_GCC_STM32C542RCT6/TIM_IT_500ms.elf -v -rst
   ```

   或使用 st-flash：

   ```bash
   st-flash write build/debug_GCC_STM32C542RCT6/TIM_IT_500ms.bin 0x08000000
   ```

4. 复位或重新上电，观察板载 LED。

## 8. 测试结果

- 编译：工程编译无错误。
- 烧录：下载到开发板正常无误。
- 运行测试：板载 LED 每 500ms 切换一次状态（亮 500ms、灭 500ms，亮灭周期 1s），闪烁稳定，无其它外部输入驱动。
