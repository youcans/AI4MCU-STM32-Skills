# 初始工程分析

## 1. 工程与任务信息

- 工程名称：TIM_PWM_1kHz
- 开发板型号：nucleo-g431rb（NUCLEO-G431RB，MB1367）
- MCU型号：STM32G431RBT6（LQFP64）
- 当前任务目标：使用 TIM2 通道 1 输出约 1 kHz PWM 信号驱动板载 LD2 LED，占空比在 10%~90% 间周期性往复变化，实现呼吸灯效果（单周期约 2 秒）。
- 构建系统：CMake（STM32CubeMX 生成，GCC 工具链）

## 2. 任务相关硬件与外设配置

### 2.1 引脚配置

| 引脚 | 信号或功能 | 模式 | 关联外设 | 依据 |
|---|---|---|---|---|
| PA5 | TIM2_CH1（PWM输出，连接板载LD2） | 复用推挽 GPIO_MODE_AF_PP，GPIO_AF1_TIM2 | TIM2 | stm32g4xx_hal_msp.c HAL_TIM_MspPostInit、.ioc |
| PA13 | SYS_JTMS-SWDIO | Serial Wire（调试） | SYS | .ioc、main.h |
| PA14 | SYS_JTCK-SWCLK | Serial Wire（调试） | SYS | .ioc、main.h |

### 2.2 外设配置

#### TIM2

| 配置项 | 当前配置 | 依据 |
|---|---|---|
| Prescaler | 159 | main.c MX_TIM2_Init |
| Period | 999 | main.c MX_TIM2_Init |
| CounterMode | TIM_COUNTERMODE_UP | main.c MX_TIM2_Init |
| AutoReloadPreload | TIM_AUTORELOAD_PRELOAD_DISABLE | main.c MX_TIM2_Init |
| 时钟源 | TIM_CLOCKSOURCE_INTERNAL | main.c MX_TIM2_Init |
| PWM模式 | TIM_OCMODE_PWM1 | main.c MX_TIM2_Init |
| Pulse（比较值CCR） | 100 | main.c MX_TIM2_Init |
| 输出极性 | TIM_OCPOLARITY_HIGH | main.c MX_TIM2_Init |
| 主输出触发 | TIM_TRGO_RESET | main.c MX_TIM2_Init |

- 相关通道：TIM2_CH1
- 相关引脚：PA5
- 初始化函数：MX_TIM2_Init()（static，在 main() 中调用），GPIO 初始化在 HAL_TIM_MspPostInit 内完成
- 句柄或对象：htim2（TIM_HandleTypeDef）
- 初始化后启动状态：未启动（main() 中未调用 HAL_TIM_PWM_Start）

### 2.3 DMA配置

无

### 2.4 相关时钟

| 时钟项目 | 当前配置或频率 | 关联对象 | 依据 |
|---|---|---|---|
| SYSCLK | 160 MHz（HSI + PLL，PLLN=20, PLLR=DIV2） | CPU | main.c SystemClock_Config |
| HCLK | 160 MHz（/1） | Cortex 总线 | main.c SystemClock_Config |
| APB1 | 160 MHz（HCLK/1） | TIM2等 | main.c SystemClock_Config、.ioc |
| 定时器时钟 | 160 MHz（APB1不分频） | TIM2 | .ioc（APB1TimFreq_Value=160000000） |
| HSI | 16 MHz 内部高速时钟 | PLL源 | main.c SystemClock_Config |
| HSE | 24 MHz 板载晶振（未启用，当前用HSI） | - | .ioc（HSE_VALUE，未配置） |

注：TIM2 PWM 频率 = 定时器时钟/(PSC+1)/(ARR+1) = 160 MHz/160/1000 = 1 kHz。

## 3. 已有程序资源与运行结构

### 3.1 初始化顺序

```text
main()
├── HAL_Init()
├── SystemClock_Config()
├── MX_GPIO_Init()
├── MX_TIM2_Init()
└── while (1)
```

### 3.2 句柄与全局对象

| 名称 | 类型 | 关联对象 | 定义位置 |
|---|---|---|---|
| htim2 | TIM_HandleTypeDef | TIM2 | main.c（全局） |
| BspButtonState | __IO uint32_t | BSP按键状态 | main.c（全局，BSP公共模块生成） |

### 3.3 关键宏定义

| 宏名称 | 当前定义 | 用途 | 定义位置 |
|---|---|---|---|
| T_SWDIO_Pin / T_SWDIO_GPIO_Port | GPIO_PIN_13 / GPIOA | SWD调试引脚定义 | main.h |
| T_SWCLK_Pin / T_SWCLK_GPIO_Port | GPIO_PIN_14 / GPIOA | SWD调试引脚定义 | main.h |

（PA5、PC13 无对应宏定义，TIM2 相关配置以句柄 htim2 方式使用）

### 3.4 中断与HAL回调

| 中断或回调 | 当前状态 | 关联对象或事件 | 位置 |
|---|---|---|---|
| SysTick_Handler | 已实现（调用 HAL_IncTick） | SysTick 时基 | stm32g4xx_it.c |
| TIM2 中断 | 未使能，无 IRQHandler | - | 无 |

当前 task 未配置任何 TIM 相关中断或 HAL 回调（如 HAL_TIM_PeriodElapsedCallback、HAL_TIM_PWM_PulseFinishedCallback 均无）。

### 3.5 主循环

while(1) 循环体为空（USER CODE BEGIN WHILE / 3 之间无代码），当前程序初始化完成后无实际操作。

## 4. 工程与代码接入位置

### 4.1 源码与构建结构

- 主要源文件：Core/Src/main.c、Core/Src/system_stm32g4xx.c、Core/Src/stm32g4xx_it.c、Core/Src/stm32g4xx_hal_msp.c、Core/Src/sysmem.c、Core/Src/syscalls.c
- 主要头文件：Core/Inc/main.h、Core/Inc/stm32g4xx_it.h、Core/Inc/stm32g4xx_hal_conf.h、Core/Inc/stm32g4xx_nucleo_conf.h
- 源码接入方式：由 STM32CubeMX 生成的 cmake/stm32cubemx 子目录接入（target_sources 由 CubeMX 管理），核心源文件位于 Core/Src
- 头文件接入方式：由 cmake/stm32cubemx 子目录的 target_include_directories 统一管理 Core/Inc

（CMakeLists.txt 根文件中 target_sources、target_include_directories 预留了"Add user sources here"自定义接入区域）

### 4.2 代码接入位置

| 文件 | 位置 | 适用代码类型 |
|---|---|---|
| main.c | USER CODE BEGIN 2 / END 2 | 外设启动调用（如 PWM 启动）、初始化后配置 |
| main.c | USER CODE BEGIN 3 / END 3（while(1) 内） | 主循环主逻辑，如呼吸灯占空比周期性变化调度 |
| main.c | USER CODE BEGIN 4 / END 4 | 自定义函数实现（如占空比更新辅助函数） |
| main.c | USER CODE BEGIN PV / END PV | 自定义私有变量定义 |
| main.c | USER CODE BEGIN Includes | 自定义头文件包含 |
| main.c | USER CODE BEGIN 0 / END 0 | 私有用户代码/函数 |
| stm32g4xx_it.c | USER CODE BEGIN 1 | 新增中断处理代码（本任务未用） |

### 4.3 自定义源文件

- 源文件位置：Core/Src/（可新增，如呼吸灯控制模块 .c 文件）
- 头文件位置：Core/Inc/（可新增 .h 文件）
- 构建接入位置：若新增 .c 文件，需在 cmake/stm32cubemx 的 STM32CubeMX 源列表或 CMakeLists.txt 根文件 target_sources 的"Add user sources here"处将其接入构建；头文件路径已由 Core/Inc 自动包含

## 5. 工程就绪状态

### 5.1 当前任务配置状态

| 任务需求 | 当前工程状态 | 依据 | 结论 |
|---|---|---|---|
| TIM2_CH1 输出约 1 kHz PWM 至 PA5（LD2） | TIM2 已配置 PWM Generation CH1，Prescaler=159，Period=999，PA5 已配置为 TIM2_CH1 复用 | main.c、stm32g4xx_hal_msp.c、.ioc | 已具备 |
| PWM 频率约 1 kHz | 频率 = 160 MHz/160/1000 = 1 kHz | .ioc 时钟、main.c 分频配置 | 已具备 |
| PWM 占空比 10%~90% 周期性往复变化（呼吸灯） | 当前 Pulse=100 固定，PWM 未启动（未调用 HAL_TIM_PWM_Start），主循环为空，无常量/逻辑实现占空比动态调节 | main.c | 未具备 |
| 呼吸周期约 2 秒 | 无相关定时/延时逻辑 | main.c | 未具备 |

### 5.2 待确认事项

已确认（2026-10-05）：

1. 占空比更新定时方式采用 **SysTick 软件调度**（HAL_GetTick 在主循环中判断）；当前工程 SysTick 时基已启用（HAL_IncTick），已具备条件。
2. 呼吸灯占空比步进与延时由编程任务规划（stm32-plan）阶段自行设计平滑步进，满足约 2 秒/周期。
3. 不需要配置额外引脚观测 PWM 波形，仅依赖 PA5/LD2 输出。

无待确认事项。
