# 初始工程分析

## 1. 工程与任务信息

- 工程名称：TIM1_PWM_Comp
- 开发板型号：NUCLEO-G431RB（nucleo-g431rb）
- MCU型号：STM32G431RBT6
- 当前任务目标：使用高级控制定时器 TIM1 的 CH1/CH1N，通过 PA8 和 PB13 输出频率为 10 kHz、占空比为 0.5、死区时间约 500 ns 的互补 PWM 信号。
- 构建系统：CMake（STM32CubeMX生成，C11 + GCC）

## 2. 任务相关硬件与外设配置

### 2.1 引脚配置

| 引脚 | 信号或功能 | 模式 | 关联外设 | 依据 |
|---|---|---|---|---|
| PA8 | TIM1_CH1 | 复用推挽输出（GPIO_MODE_AF_PP，AF6） | TIM1 | stm32g4xx_hal_msp.c HAL_TIM_MspPostInit |
| PB13 | TIM1_CH1N | 复用推挽输出（GPIO_MODE_AF_PP，AF6） | TIM1 | stm32g4xx_hal_msp.c HAL_TIM_MspPostInit |

### 2.2 外设配置

#### TIM1（高级控制定时器，PWM Generation1 CH1 CH1N）

| 配置项 | 当前配置 | 依据 |
|---|---|---|
| 计数时钟源 | 内部时钟（TIM_CLOCKSOURCE_INTERNAL） | main.c MX_TIM1_Init |
| 预分频器 Prescaler | 159 | main.c MX_TIM1_Init |
| 计数模式 | 向上计数（UP） | main.c MX_TIM1_Init |
| 周期 Period（ARR） | 99 | main.c MX_TIM1_Init |
| 时钟分频 ClockDivision | DIV1 | main.c MX_TIM1_Init |
| 重复计数 RepetitionCounter | 0 | main.c MX_TIM1_Init |
| 自动重载预装载 | 禁用（DISABLE） | main.c MX_TIM1_Init |
| PWM 模式 | PWM1（OCMode = TIM_OCMODE_PWM1） | main.c MX_TIM1_Init |
| 脉宽 Pulse（CCR1） | 50 | main.c MX_TIM1_Init |
| OC 极性 / OCN 极性 | 高有效 HIGH（两路均） | main.c MX_TIM1_Init |
| 空闲状态 OC / OCN | RESET（两路均） | main.c MX_TIM1_Init |
| 死区时间 DeadTime | 80（DTG 计数值） | main.c MX_TIM1_Init HAL_TIMEx_ConfigBreakDeadTime |
| 刹车 Break1/Break2 | 均禁用，自动输出 AUTO 禁用（AutomaticOutput=DISABLE） | main.c MX_TIM1_Init |
| MSP 引脚映射 | PA8=CH1、PB13=CH1N，AF6 | stm32g4xx_hal_msp.c HAL_TIM_MspPostInit |

- 相关通道：TIM_CHANNEL_1（CH1 与互补 CH1N 同通道配置）
- 相关引脚：PA8（CH1 输出）、PB13（CH1N 互补输出）
- 初始化函数：MX_TIM1_Init()（static，main.c 内），内部依次调用 HAL_TIM_Base_Init、HAL_TIM_ConfigClockSource、HAL_TIM_PWM_Init、HAL_TIMEx_MasterConfigSynchronization、HAL_TIM_PWM_ConfigChannel、HAL_TIMEx_ConfigBreakDeadTime、HAL_TIM_MspPostInit
- 句柄或对象：htim1（TIM_HandleTypeDef，全局，main.c）
- 初始化后启动状态：未启动——main.c 中无 HAL_TIM_PWM_Start / HAL_TIMEx_PWMN_Start 等启动调用，PA8/PB13 当前无输出

参数核算（160 MHz 定时器时钟）：

- PWM 频率 = 160 MHz / (159+1) / (99+1) = 10 kHz，与需求一致
- 占空比 = 50 / 100 = 0.5，与需求一致
- 死区 = DTG(80) × tDTS(1/160 MHz=6.25 ns) = 500 ns，与需求"约 500 ns"一致

### 2.3 DMA配置

| 用途 | DMA资源 | 方向 | 模式 | 关联外设 | 依据 |
|---|---|---|---|---|---|
| 无 | — | — | — | — | .ioc 中 TIM1 未配置 DMA |

### 2.4 相关时钟

| 时钟项目 | 当前配置或频率 | 关联对象 | 依据 |
|---|---|---|---|
| 系统时钟 SYSCLK | 160 MHz（PLL：HSI 16 MHz，M=1、N=20、P=2） | 全系统 | main.c SystemClock_Config |
| AHB / HCLK | 160 MHz（DIV1） | CPU/总线 | main.c SystemClock_Config |
| APB1 / APB2 | 160 MHz（DIV1） | 外设总线 | main.c SystemClock_Config |
| TIM1 定时器时钟 | 160 MHz（APB2 分频为 1） | TIM1 | .ioc APB2TimFreq_Value / main.c |

注：硬件概述资料记录板载 HSE 24 MHz 晶振，但当前工程实际以 HSI 16 MHz 作为 PLL 时钟源；两种方式下系统均运行于 160 MHz，TIM1 定时器时钟不变。

## 3. 已有程序资源与运行结构

### 3.1 初始化顺序

```text
main()
├── HAL_Init()
├── SystemClock_Config()
├── MX_GPIO_Init()
├── MX_TIM1_Init()
└── while (1)
```

（依据 main.c；MX_TIM1_Init 执行完毕后主循环为空转）

### 3.2 句柄与全局对象

| 名称 | 类型 | 关联对象 | 定义位置 |
|---|---|---|---|
| htim1 | TIM_HandleTypeDef | TIM1 | main.c 全局变量区 |

### 3.3 关键宏定义

| 宏名称 | 当前定义 | 用途 | 定义位置 |
|---|---|---|---|
| 无 | — | 任务相关引脚以 GPIO_PIN_8/GPIO_PIN_13 + GPIO_AF6_TIM1 直接配置，无任务相关工程宏 | — |

### 3.4 中断与HAL回调

| 中断或回调 | 当前状态 | 关联对象或事件 | 位置 |
|---|---|---|---|
| TIM1 中断（TIM1_UP、TIM1_CC 等） | 未使能 | — | .ioc NVIC 未配置；stm32g4xx_it.c 无 TIM1 处理函数 |
| SysTick_Handler | 已实现（HAL_IncTick） | 系统时基 | stm32g4xx_it.c |
| TIM HAL 回调 | 无 | — | 源码中未定义 |

### 3.5 主循环

main.c 中 while(1) 内仅存在 USER CODE 注释区域，无实际可执行代码。当前程序运行方式：完成初始化和外设配置后主循环空转。

## 4. 工程与代码接入位置

### 4.1 源码与构建结构

- 主要源文件：Core/Src/main.c（应用入口与外设初始化）、stm32g4xx_hal_msp.c（外设 MSP：时钟使能与 GPIO 复用配置）、stm32g4xx_it.c（中断服务程序）、system_stm32g4xx.c、sysmem.c、syscalls.c
- 主要头文件：Core/Inc/main.h（公共头，内含句柄与引脚宏）、stm32g4xx_it.h、stm32g4xx_hal_conf.h、stm32g4xx_nucleo_conf.h
- 源码接入方式：CMakeLists.txt 通过 add_subdirectory(cmake/stm32cubemx) 接入 CubeMX 生成的全部源文件；顶层 CMakeLists.txt 的 target_sources 提供"Add user sources here"用户扩展区域
- 头文件接入方式：CubeMX 生成源经 cmake 子目录片段提供包含路径；用户扩展包含路径在顶层 target_include_directories 的"Add user defined include paths"区域

### 4.2 代码接入位置

| 文件 | 位置 | 适用代码类型 |
|---|---|---|
| main.c | USER CODE BEGIN 2 / END 2 | 外设初始化完成后、进入主循环前的启动逻辑（如启动 PWM 输出） |
| main.c | USER CODE BEGIN WHILE / BEGIN 3 | 主循环内周期执行逻辑 |
| main.c | USER CODE BEGIN PV / PD / PFP / 4 | 全局变量、宏定义、函数原型与用户函数实现 |
| stm32g4xx_hal_msp.c | USER CODE 各区域（MspInit/PostInit 等） | 引脚时钟、复用配置的扩展修改 |
| stm32g4xx_it.c | USER CODE BEGIN 1 | 新增外设中断处理 |
| CMakeLists.txt | target_sources / target_include_directories 用户注释区 | 新增源文件与包含路径的构建接入 |

### 4.3 自定义源文件

- 源文件位置：建议放置于 Core/Src/（与 CubeMX 生成源同级）
- 头文件位置：建议放置于 Core/Inc/
- 构建接入位置：顶层 CMakeLists.txt 的 target_sources PRIVATE 用户区域添加 .c 文件；包含路径在 target_include_directories 用户区域添加。cmake/stm32cubemx 子目录为 CubeMX 生成内容，其内部文件清单本次未扫描，新增文件是否需同步登记待确认

## 5. 工程就绪状态

### 5.1 当前任务配置状态

| 任务需求 | 当前工程状态 | 依据 | 结论 |
|---|---|---|---|
| TIM1 CH1/CH1N 互补 PWM（PA8/PB13） | PA8、PB13 已配置为 AF6 复用推挽输出 | stm32g4xx_hal_msp.c | 已具备 |
| 频率 10 kHz | Prescaler=159、ARR=99，核算为 10 kHz | main.c | 已具备 |
| 占空比 0.5 | Pulse=50、ARR=99，核算为 50% | main.c | 已具备 |
| 死区约 500 ns | DeadTime=80，核算为 500 ns | main.c | 已具备 |
| 输出互补 PWM 信号 | 无 HAL_TIM_PWM_Start / HAL_TIMEx_PWMN_Start 调用，输出未启动 | main.c | 未具备（需编程实现启动） |

### 5.2 待确认事项

| 编号 | 待确认事项 | 影响 |
|---|---|---|
| 1 | 输出启动方式与时机：初始化后直接持续输出，或由按键/外部信号触发（需求文件待确认项） | 决定 USER CODE BEGIN 2 区域接入的启动逻辑 |
| 2 | 时钟源：工程实际使用 HSI 16 MHz 作 PLL 源（非硬件概述记录的 HSE 24 MHz），系统时钟均 160 MHz | 当前不影响任务；如需切换 HSE 需修改 SystemClock_Config |
| 3 | 新增自定义源文件的构建登记方式（cmake/stm32cubemx 生成段未在扫描范围） | 决定自定义源码的工程接入方式 |
