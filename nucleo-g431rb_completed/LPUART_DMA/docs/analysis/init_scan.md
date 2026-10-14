# 初始工程分析

## 1. 工程与任务信息

- 工程名称：LPUART_DMA
- 开发板型号：NUCLEO-G431RB
- MCU型号：STM32G431RBT6
- 当前任务目标：使用LPUART1通过板载虚拟串口（ST-LINK Virtual COM），以DMA方式连续发送JustFloat格式锯齿波数据，并在PC端VOFA+（数据引擎JustFloat）中显示波形（PR-001）。
- 构建系统：CMake（GCC，arm-none-eabi；工程根目录含CMakePresets.json与.vscode构建配置）

## 2. 任务相关硬件与外设配置

### 2.1 引脚配置

| 引脚 | 信号或功能 | 模式 | 关联外设 | 依据 |
|---|---|---|---|---|
| PA2 | LPUART1_TX | 复用推挽（GPIO_MODE_AF_PP，GPIO_AF12_LPUART1，无上下拉，低速），默认连接ST-LINK VCP | LPUART1 | stm32g4xx_hal_msp.c:117-125；LPUART_DMA.ioc：PA2.Signal=LPUART1_TX；proj_requirements.md |
| PA3 | LPUART1_RX | 复用推挽（GPIO_MODE_AF_PP，GPIO_AF12_LPUART1，无上下拉，低速），默认连接ST-LINK VCP | LPUART1 | stm32g4xx_hal_msp.c:117-125；LPUART_DMA.ioc：PA3.Signal=LPUART1_RX；proj_requirements.md |

说明：PA2/PA3 经 ST-LINK/V3E 虚拟串口与 PC 通信，为 VOFA+ 数据接收通道（proj_requirements.md 3.3）。

### 2.2 外设配置

#### LPUART1

| 配置项 | 当前配置 | 依据 |
|---|---|---|
| 波特率 | 115200 | main.c:180；LPUART_DMA.ioc：LPUART1.BaudRate=115200 |
| 数据位 | 8位（UART_WORDLENGTH_8B） | main.c:181 |
| 停止位 | 1（UART_STOPBITS_1） | main.c:182 |
| 校验 | 无（UART_PARITY_NONE） | main.c:183 |
| 模式 | 收发（UART_MODE_TX_RX） | main.c:184 |
| 硬件流控 | 无（UART_HWCONTROL_NONE） | main.c:185 |
| 时钟预分频 | UART_PRESCALER_DIV1 | main.c:187 |
| FIFO | 阈值1/8设置后禁用FIFO模式 | main.c:193-204 |
| 外设时钟源 | PCLK1（RCC_LPUART1CLKSOURCE_PCLK1） | stm32g4xx_hal_msp.c:105-106 |

- 相关通道：无（LPUART1为单实例）
- 相关引脚：PA2（TX）、PA3（RX）
- 初始化函数：MX_LPUART1_UART_Init()（main.c:169）；MSP初始化 HAL_UART_MspInit()（stm32g4xx_hal_msp.c:93，配置外设时钟、GPIO、DMA链接与NVIC）
- 句柄或对象：hlpuart1（UART_HandleTypeDef）
- 初始化后启动状态：仅完成配置，未启动数据收发

### 2.3 DMA配置

| 用途 | DMA资源 | 方向 | 模式 | 关联外设 | 依据 |
|---|---|---|---|---|---|
| LPUART1发送 | DMA1_Channel1 | 存储器→外设（DMA_MEMORY_TO_PERIPH） | NORMAL | LPUART1 | main.c:46；stm32g4xx_hal_msp.c:129-143；LPUART_DMA.ioc：Dma.LPUART1_TX.0.* |
| LPUART1接收 | DMA1_Channel2 | 外设→存储器（DMA_PERIPH_TO_MEMORY） | NORMAL | LPUART1 | main.c:47；stm32g4xx_hal_msp.c:146-160；LPUART_DMA.ioc：Dma.LPUART1_RX.1.* |

说明：

- DMA请求映射（DMAMUX）：LPUART1_TX / LPUART1_RX（stm32g4xx_hal_msp.c:130、147）；
- 均为字节对齐（外设/存储器数据对齐BYTE）、存储器地址递增使能、优先级LOW；
- 已通过 __HAL_LINKDMA 链接至 hlpuart1 的 hdmatx/hdmarx（stm32g4xx_hal_msp.c:143、160）；
- 当前状态：DMA通道已配置、中断已使能，但未启动（主循环无业务代码，无传输进行）。

### 2.4 相关时钟

| 时钟项目 | 当前配置或频率 | 关联对象 | 依据 |
|---|---|---|---|
| SYSCLK | 160MHz（HSI16×PLLN20，PLLP=DIV2） | 系统 | main.c:134-156 |
| HCLK（AHB） | 160MHz（DIV1） | 系统总线 | main.c:154 |
| PCLK1（APB1） | 160MHz（DIV1） | LPUART1等 | main.c:155 |
| PCLK2（APB2） | 160MHz（DIV1） | 外设总线 | main.c:156 |
| LPUART1时钟 | 160MHz（PCLK1） | hlpuart1 | stm32g4xx_hal_msp.c:105-106；LPUART_DMA.ioc：LPUART1Freq_Value |
| HSI | 16MHz（PLL输入源） | SystemClock_Config | main.c:134-139 |

说明：需求文档（proj_requirements.md 3.1）注明系统时钟源为板载24MHz HSE；当前工程 SystemClock_Config 使用 HSI 作为PLL输入且未使能HSE（PF0/PF1未配置）。差异详见 5.2 待确认事项 C-01。

## 3. 已有程序资源与运行结构

### 3.1 初始化顺序

```text
main()
├── HAL_Init()
├── SystemClock_Config()
├── MX_GPIO_Init()
├── MX_DMA_Init()
├── MX_LPUART1_UART_Init()   （HAL_UART_Init → HAL_UART_MspInit 配置GPIO/DMA/NVIC）
└── while (1)
```

### 3.2 句柄与全局对象

| 名称 | 类型 | 关联对象 | 定义位置 |
|---|---|---|---|
| hlpuart1 | UART_HandleTypeDef | LPUART1（hdmatx/hdmarx已链接） | main.c:45；extern：stm32g4xx_it.c:60 |
| hdma_lpuart1_tx | DMA_HandleTypeDef | DMA1_Channel1，LPUART1_TX | main.c:46；extern：stm32g4xx_hal_msp.c:26、stm32g4xx_it.c:58 |
| hdma_lpuart1_rx | DMA_HandleTypeDef | DMA1_Channel2，LPUART1_RX | main.c:47；extern：stm32g4xx_hal_msp.c:28、stm32g4xx_it.c:59 |

### 3.3 关键宏定义

| 宏名称 | 当前定义 | 用途 | 定义位置 |
|---|---|---|---|
| USE_HAL_UART_REGISTER_CALLBACKS | 0U | 关闭HAL UART注册回调机制，回调采用默认弱函数方式 | Core/Inc/stm32g4xx_hal_conf.h:107 |

说明：main.h 中 USER_BTN/LD2/T_SWDIO 等引脚宏与当前任务无关；构建符号（USE_HAL_DRIVER、STM32G431xx、USE_NUCLEO_64）见 4.1。

### 3.4 中断与HAL回调

| 中断或回调 | 当前状态 | 关联对象或事件 | 位置 |
|---|---|---|---|
| DMA1_Channel1_IRQn | 已使能（抢占优先级0/子优先级0） | hdma_lpuart1_tx | main.c:223-224；stm32g4xx_it.c:206-215（HAL_DMA_IRQHandler） |
| DMA1_Channel2_IRQn | 已使能（抢占优先级0/子优先级0） | hdma_lpuart1_rx | main.c:226-227；stm32g4xx_it.c:220-229（HAL_DMA_IRQHandler） |
| LPUART1_IRQn | 已使能（抢占优先级0/子优先级0） | hlpuart1 | stm32g4xx_hal_msp.c:163-164；stm32g4xx_it.c:234-243（HAL_UART_IRQHandler） |
| SysTick_IRQn | 已使能 | HAL时基（HAL_IncTick） | stm32g4xx_it.c:185-194 |
| HAL UART/DMA 用户回调 | 未实现任何用户回调（TxCplt/Error等均未定义） | 传输完成/错误事件 | 用户代码为空；USE_HAL_UART_REGISTER_CALLBACKS=0（默认弱函数回调方式） |

### 3.5 主循环

当前 main() 中 while(1) 为空循环：工程仅完成系统与外设初始化后空转，无任何业务逻辑；循环体内保留 USER CODE BEGIN WHILE / BEGIN 3 用户代码区域供接入（main.c:107-115）。

## 4. 工程与代码接入位置

### 4.1 源码与构建结构

- 主要源文件：Core/Src/main.c、stm32g4xx_it.c、stm32g4xx_hal_msp.c、sysmem.c、syscalls.c、system_stm32g4xx.c；startup_stm32g431xx.s；Drivers（HAL/LL与BSP驱动，列表见cmake/stm32cubemx/CMakeLists.txt）
- 主要头文件：Core/Inc/main.h、stm32g4xx_it.h、stm32g4xx_hal_conf.h、stm32g4xx_nucleo_conf.h；Drivers（HAL、CMSIS、BSP头文件目录）
- 源码接入方式：根CMakeLists.txt通过 add_subdirectory(cmake/stm32cubemx) 接入；cmake/stm32cubemx/CMakeLists.txt 的 MX_Application_Src 列出应用源文件，STM32_Drivers（OBJECT库）列出HAL/BSP驱动；用户源码在根CMakeLists.txt target_sources(${CMAKE_PROJECT_NAME} PRIVATE ...) 处添加
- 头文件接入方式：MX_Include_Dirs 提供 Core/Inc、HAL、CMSIS、BSP 头文件路径；编译符号 USE_NUCLEO_64、USE_HAL_DRIVER、STM32G431xx（Debug下含DEBUG）由 MX_Defines_Syms 传入；main.h 包含 stm32g4xx_hal.h 与 stm32g4xx_nucleo.h

### 4.2 代码接入位置

| 文件 | 位置 | 适用代码类型 |
|---|---|---|
| Core/Src/main.c | USER CODE BEGIN/END Includes、PV、PFP、0 | 头文件包含、私有变量、函数原型、辅助函数 |
| Core/Src/main.c | USER CODE BEGIN/END 1、2 | 初始化准备代码（外设初始化前后） |
| Core/Src/main.c | USER CODE BEGIN WHILE、3 | 主循环业务逻辑（周期发送等） |
| Core/Src/main.c | USER CODE BEGIN/END 4 | HAL回调与辅助函数（直接写于main.c时） |
| Core/Src/stm32g4xx_it.c | USER CODE区域（各Handler及文件末尾） | 中断处理辅助代码（一般不改动Handler本体） |
| Core/Src/stm32g4xx_hal_msp.c | USER CODE区域 | MSP补充配置（一般不改动） |

### 4.3 自定义源文件

- 源文件位置：Core/Src/（或工程内新建目录）
- 头文件位置：Core/Inc/
- 构建接入位置：根 CMakeLists.txt 的 target_sources(${CMAKE_PROJECT_NAME} PRIVATE ...) 添加源文件、target_include_directories(...) 添加头文件路径；也可在 cmake/stm32cubemx/CMakeLists.txt 的 MX_Application_Src / MX_Include_Dirs 追加

## 5. 工程就绪状态

### 5.1 当前任务配置状态

| 任务需求 | 当前工程状态 | 依据 | 结论 |
|---|---|---|---|
| PR-001：LPUART1通过DMA连续发送JustFloat锯齿波数据至VOFA+ | LPUART1（115200、8N1、TX/RX）、PA2/PA3复用AF12、TX DMA通道（DMA1_Channel1）已配置并链接、NVIC中断已使能；DMA未启动、无数据帧生成与发送代码 | main.c、stm32g4xx_hal_msp.c、LPUART_DMA.ioc、proj_requirements.md、vofa_justfloat.md | 已具备（外设/引脚/DMA通道/中断配置齐全；发送运行参数与数据参数见5.2） |
| 系统时钟与调试基础 | SYSCLK 160MHz（PLL），SWD调试使能（PA13/PA14） | SystemClock_Config（main.c:122-162）、LPUART_DMA.ioc | 已具备（时钟源与需求描述存在差异，见5.2 C-01） |

### 5.2 待确认事项

| 编号 | 待确认事项 | 影响 |
|---|---|---|
| C-01 | 需求文档注明系统时钟源为板载24MHz HSE（PF0/PF1）；当前工程SystemClock_Config使用HSI作为PLL输入，HSE未使能、PF0/PF1未配置 | 系统时钟源与需求描述不一致；功能不受影响，需确认是否按需求启用HSE |
| C-02 | 波特率已由工程配置确定为115200；需确认与VOFA+串口设置一致 | 影响VOFA+能否正确接收解析数据 |
| C-03 | 锯齿波采样点数、单帧数据量（缓冲大小）、发送周期未在需求与工程中明确 | 影响发送缓冲设计与波形显示效果 |
| C-04 | DMA当前为NORMAL模式且未启动；连续发送所需的工作模式或触发方式未确定 | 影响连续发送运行方式（在配置或实现阶段确定） |
