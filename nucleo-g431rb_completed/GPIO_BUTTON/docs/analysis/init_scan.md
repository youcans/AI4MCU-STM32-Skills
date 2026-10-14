# 初始工程分析

## 1. 工程与任务信息

- 工程名称：GPIO_BUTTON
- 开发板型号：NUCLEO-G431RB
- MCU型号：STM32G431RBT6
- 当前任务目标：在主循环中轮询检测用户按键 B1（PC13）的按下事件（软件去抖 + 边沿检测），每次有效按下翻转板载用户 LED LD2（PA5）的亮灭状态；持续按住不重复触发，松开后可响应下一次按下。
- 构建系统：CMake（根 CMakeLists.txt + cmake/stm32cubemx 子目录，GCC 工具链，C11 标准，默认 Debug，使能 compile_commands 导出）

## 2. 任务相关硬件与外设配置

### 2.1 引脚配置

| 引脚 | 信号或功能 | 模式 | 关联外设 | 依据 |
|---|---|---|---|---|
| PC13 | USER_BTN（B1按键，按下高电平、松开经板载100K下拉为低电平） | 输入（GPIO_MODE_INPUT），无内部上下拉 | GPIO（GPIOC） | main.h、main.c MX_GPIO_Init()、GPIO_BUTTON.ioc |
| PA5 | LD2（用户LED，高电平点亮） | 推挽输出（GPIO_MODE_OUTPUT_PP），无上下拉，低速，初始输出低电平 | GPIO（GPIOA） | main.h、main.c MX_GPIO_Init()、GPIO_BUTTON.ioc |

### 2.2 外设配置

任务仅涉及 GPIO（GPIOA、GPIOC），无其它外设、无 DMA、无 EXTI、无定时器。

#### GPIO

| 配置项 | 当前配置 | 依据 |
|---|---|---|
| USER_BTN（PC13） | 输入模式，无内部上下拉 | main.c MX_GPIO_Init() |
| LD2（PA5） | 推挽输出，无上下拉，低速率，初始化输出 RESET | main.c MX_GPIO_Init() |
| 端口时钟 | GPIOC、GPIOA 时钟已使能 | main.c MX_GPIO_Init() |
| HAL模块 | HAL_GPIO_MODULE_ENABLED 已启用 | stm32g4xx_hal_conf.h |

- 相关通道：无（普通 GPIO，无外设复用通道）
- 相关引脚：PA5（LD2）、PC13（USER_BTN）
- 初始化函数：MX_GPIO_Init()（main.c 内 static 函数，由 main() 调用）
- 句柄或对象：无独立句柄；GPIO 操作直接使用 HAL_GPIO_ReadPin / HAL_GPIO_WritePin / HAL_GPIO_TogglePin
- 初始化后启动状态：LD2 输出低电平（LED 熄灭）

### 2.3 DMA配置

无。

### 2.4 相关时钟

| 时钟项目 | 当前配置或频率 | 关联对象 | 依据 |
|---|---|---|---|
| 系统时钟源 | HSI16 → PLL（PLLM/4、PLLN×85、PLLR/2）→ PLLCLK | RCC | main.c SystemClock_Config() |
| SYSCLK | 170 MHz | 内核与总线 | main.c、GPIO_BUTTON.ioc |
| AHB（HCLK） | 170 MHz（SYSCLK/1） | GPIO 等外设 | main.c SystemClock_Config() |
| APB1 / APB2 | 170 MHz | 外设总线 | main.c SystemClock_Config() |
| Flash 等待与电压等级 | FLASH_LATENCY_4，Scale1 Boost | Flash | main.c SystemClock_Config() |
| SysTick | 已使能，SysTick_Handler 调用 HAL_IncTick() 提供 HAL 时基（周期由 HAL_Init 默认配置） | HAL 时基（可经 HAL_GetTick 获取毫秒计数值） | stm32g4xx_it.c、stm32g4xx_hal_conf.h（TICK_INT_PRIORITY=0） |

## 3. 已有程序资源与运行结构

### 3.1 初始化顺序

根据 main.c 实际代码：

```text
main()
├── HAL_Init()
├── SystemClock_Config()
├── MX_GPIO_Init()
└── while (1)
```

说明：main.c 中 BSP 支持代码段（USER CODE BEGIN BSP）为空，无其它初始化函数。

### 3.2 句柄与全局对象

| 名称 | 类型 | 关联对象 | 定义位置 |
|---|---|---|---|
| BspButtonState | __IO uint32_t，初值 BUTTON_RELEASED | BSP 按键状态（随 BSP_Common_DEMO 生成；BUTTON_RELEASED 为 BSP 枚举值，定义于 BSP 头文件，不在本次扫描范围） | main.c |

### 3.3 关键宏定义

| 宏名称 | 当前定义 | 用途 | 定义位置 |
|---|---|---|---|
| USER_BTN_Pin | GPIO_PIN_13 | B1 按键引脚 | main.h |
| USER_BTN_GPIO_Port | GPIOC | B1 按键端口 | main.h |
| LD2_Pin | GPIO_PIN_5 | LD2 LED 引脚 | main.h |
| LD2_GPIO_Port | GPIOA | LD2 LED 端口 | main.h |

### 3.4 中断与HAL回调

| 中断或回调 | 当前状态 | 关联对象或事件 | 位置 |
|---|---|---|---|
| SysTick_Handler | 已实现并启用，调用 HAL_IncTick() | SysTick（HAL 时基） | stm32g4xx_it.c |
| 系统异常 Handler（NMI/HardFault/MemManage/BusFault/UsageFault） | 已实现（默认死循环），按 .ioc NVIC 配置使能 | Cortex-M4 异常 | stm32g4xx_it.c |
| 外设 IRQHandler | 无（工程未配置外设中断） | — | stm32g4xx_it.c 中无外设中断处理函数 |
| EXTI | 未配置（PC13 为普通 GPIO 输入，无中断） | — | GPIO_BUTTON.ioc、main.c |
| HAL 回调 | 源码中无已实现的 HAL 回调 | — | Core/Src |

### 3.5 主循环

main.c 的 while(1) 中当前无任何实际代码（仅有空的 USER CODE 区域）。程序上电后完成初始化，LD2 保持熄灭，无按键处理逻辑。

## 4. 工程与代码接入位置

### 4.1 源码与构建结构

- 主要源文件：Core/Src/main.c（应用入口、时钟与 GPIO 初始化）、Core/Src/stm32g4xx_it.c（中断处理）、Core/Src/stm32g4xx_hal_msp.c（MSP 初始化）、Core/Src/system_stm32g4xx.c（CMSIS 系统函数）、Core/Src/syscalls.c 与 sysmem.c（C 运行时支持）
- 主要头文件：Core/Inc/main.h（引脚宏与公共声明）、Core/Inc/stm32g4xx_hal_conf.h（HAL 模块选择与振荡器参数）、Core/Inc/stm32g4xx_it.h、Core/Inc/stm32g4xx_nucleo_conf.h（BSP 配置）
- 源码接入方式：根 CMakeLists.txt 通过 add_subdirectory(cmake/stm32cubemx) 接入 CubeMX 生成的源文件，链接目标库 stm32cubemx；根 CMakeLists.txt 中 target_sources() 预留用户源码扩展段
- 头文件接入方式：根 CMakeLists.txt 中 target_include_directories() 预留用户头文件路径扩展段；CubeMX 部分头文件路径由 cmake/stm32cubemx 子目录管理（不在本次扫描范围）

### 4.2 代码接入位置

| 文件 | 位置 | 适用代码类型 |
|---|---|---|
| Core/Src/main.c | while(1) 内 USER CODE BEGIN 3 区域 | 主循环按键轮询与 LED 控制逻辑 |
| Core/Src/main.c | USER CODE BEGIN PV / PD / PFP 区域 | 私有变量（如按键状态标志）、私有宏、函数原型 |
| Core/Src/main.c | USER CODE BEGIN 4 区域 | 自定义函数（如去抖与边沿检测函数） |
| Core/Inc/main.h | USER CODE 各区域 | 自定义宏、类型与函数声明 |
| Core/Src/stm32g4xx_it.c | USER CODE 各区域 | 中断相关自定义代码（当前任务无外设中断，可不用） |

### 4.3 自定义源文件

- 源文件位置：Core/Src/（新增 .c 文件）
- 头文件位置：Core/Inc/（新增 .h 文件）
- 构建接入位置：根 CMakeLists.txt 中 target_sources() 的“Add user sources here”注释段与 target_include_directories() 的“Add user defined include paths”注释段

## 5. 工程就绪状态

### 5.1 当前任务配置状态

| 任务需求 | 当前工程状态 | 依据 | 结论 |
|---|---|---|---|
| PR-001 按键轮询检测 | PC13 已配置为输入（板载 100K 下拉，按下高电平）；主循环为空，可直接接入轮询逻辑；HAL 时基（SysTick）可提供去抖所需的毫秒延时基准 | main.c MX_GPIO_Init()、stm32g4xx_it.c | 已具备 |
| PR-002 LED翻转控制 | PA5 已配置为推挽输出，初始低电平（LED 灭）；GPIO HAL 模块已启用，可直接翻转输出电平 | main.c MX_GPIO_Init()、stm32g4xx_hal_conf.h | 已具备 |
| 软件去抖与边沿检测 | 无需额外外设资源；轮询 + 去抖 + 边沿检测均可基于现有 GPIO 与 HAL 时基实现 | 上述配置 | 已具备 |

### 5.2 待确认事项

| 编号 | 待确认事项 | 影响 |
|---|---|---|
| 1 | 软件去抖时长及判定逻辑的具体参数（需求文件未指定，由实现阶段确定） | 去抖逻辑实现参数 |
| 2 | main.c 中已有的 BspButtonState 全局变量与 BSP 按键支持（.ioc 启用 BSP_Common_DEMO）在实现中是否使用（直接 HAL GPIO 读写，或改用 BSP 接口） | 按键读取代码的实现方式选择 |
