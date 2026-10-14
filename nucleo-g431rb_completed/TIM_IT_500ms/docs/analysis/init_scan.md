# 初始工程分析

## 1. 工程与任务信息

- 工程名称：TIM_IT_500ms
- 开发板型号：nucleo-g431rb（NUCLEO-G431RB）
- MCU型号：STM32G431RBT6（LQFP64）
- 当前任务目标：使用基础定时器 TIM6 每 500ms 产生一次更新事件，在更新中断服务程序中切换板载 LD2(PA5) 指示灯的开关状态，实现 LD2 每 500ms 周期闪烁。
- 构建系统：CMake（CubeMX CMake 工程，`cwd/CMakeLists.txt` + `cmake/stm32cubemx/CMakeLists.txt`）

## 2. 任务相关硬件与外设配置

### 2.1 引脚配置

| 引脚 | 信号或功能 | 模式 | 关联外设 | 依据 |
|---|---|---|---|---|
| PA5 | LD2_Pin / LD2_GPIO_Port(GPIOA) | GPIO_MODE_OUTPUT_PP，输出低电平 RESET，速度 LOW | GPIO（TIM6中断中切换） | main.h:64-65；main.c MX_GPIO_Init |
| PC13 | USER_BTN_Pin / GPIOC | GPIO_MODE_INPUT，GPIO_NOPULL | GPIO | main.h:62-63；main.c MX_GPIO_Init |
| PA13 | T_SWDIO_Pin / GPIOA | SWDIO（Serial_Wire） | SYS 调试 | main.h:66-67；.ioc PA13 |
| PA14 | T_SWCLK_Pin / GPIOA | SWCLK（Serial_Wire） | SYS 调试 | main.h:68-69；.ioc PA14 |

依据：本实验核心为 PA5(LD2) 输出驱动板载 LED，PC13 与 SWD 引脚为工程固定配置。

### 2.2 外设配置

#### TIM6（基础定时器）

| 配置项 | 当前配置 | 依据 |
|---|---|---|
| 计数器模式 | TIM_COUNTERMODE_UP（向上计数） | main.c:179 |
| 预分频值 Prescaler | 1599 | main.c:178 |
| 自动重载值 Period | 49999 | main.c:180 |
| 自动重载预装载 | TIM_AUTORELOAD_PRELOAD_DISABLE | main.c:181 |
| 主从同步 | 主输出触发 TIM_TRGO_RESET，主从模式禁用 | main.c:186-187 |
| 时钟源 | 内部时钟（TIM6 基本定时器，无外部引脚） | .ioc VP_TIM6_VS_ClockSourceINT |

- 相关通道：无（基本定时器，无通道/引脚）
- 相关引脚：无
- 初始化函数：`MX_TIM6_Init()`（main.c:165）
- 句柄或对象：`TIM_HandleTypeDef htim6`（main.c:45）
- 初始化后启动状态：**未启动**。`MX_TIM6_Init()` 仅完成 `HAL_TIM_Base_Init()` 与主从配置，未调用 `HAL_TIM_Base_Start_IT()`，也未使能更新中断 `__HAL_TIM_ENABLE_IT(htim6, TIM_IT_UPDATE)`。定时启动与更新中断使能需在 USER CODE 区域实现（属编程任务）。

### 2.3 DMA配置

| 用途 | DMA资源 | 方向 | 模式 | 关联外设 | 依据 |
|---|---|---|---|---|---|
| 无 | - | - | - | - | 工程未配置 DMA（.ioc 无 DMA 项；CMake 仅链接 hal_dma 驱动） |

### 2.4 相关时钟

| 时钟项目 | 当前配置或频率 | 关联对象 | 依据 |
|---|---|---|---|
| SYSCLK | 160 MHz（PLL，HSI×20/1，PLLR÷2） | CPU | main.c:130-157；.ioc RCC |
| HCLK | 160 MHz（AHB ÷1） | AHB | main.c:150；.ioc RCC |
| APB1 Timer Clock | 160 MHz（PCLK1 ÷1） | TIM6 | main.c:151；.ioc APB1TimFreq_Value |
| APB2 Timer Clock | 160 MHz（PCLK2 ÷1） | APB2 定时器 | .ioc APB2TimFreq_Value |

- TIM6 时钟频率：160 MHz。
- 500ms 定时验证：计数时钟 160MHz，分频 (1599+1)×(49999+1)=1600×50000，更新周期=160MHz/(1600×50000)=2Hz=500ms。与需求条件一致。

## 3. 已有程序资源与运行结构

### 3.1 初始化顺序

根据 main.c 实际代码：

```text
main()
├── HAL_Init()
├── SystemClock_Config()
├── MX_GPIO_Init()
├── MX_TIM6_Init()
└── while (1)
```

- `HAL_TIM_Base_MspInit()`（hal_msp.c:90）在 `MX_TIM6_Init()`→`HAL_TIM_Base_Init()` 时被调用，完成 TIM6 时钟使能、`HAL_NVIC_SetPriority(TIM6_DAC_IRQn,5,0)`、`HAL_NVIC_EnableIRQ()`。

### 3.2 句柄与全局对象

| 名称 | 类型 | 关联对象 | 定义位置 |
|---|---|---|---|
| htim6 | TIM_HandleTypeDef | TIM6 | main.c:45（stm32g4xx_it.c:58 以 extern 引用） |
| BspButtonState | __IO uint32_t | 板载按键状态 | main.c:44（初始化为 BUTTON_RELEASED） |

### 3.3 关键宏定义

| 宏名称 | 当前定义 | 用途 | 定义位置 |
|---|---|---|---|
| LD2_Pin | GPIO_PIN_5 | LD2 引脚 | main.h:64 |
| LD2_GPIO_Port | GPIOA | LD2 端口 | main.h:65 |
| USER_BTN_Pin | GPIO_PIN_13 | 用户按键引脚 | main.h:62 |
| USER_BTN_GPIO_Port | GPIOC | 用户按键端口 | main.h:63 |
| BUTTON_RELEASED | (BSP 定义) | 按键释放状态 | stm32g4xx_nucleo.h |

### 3.4 中断与HAL回调

| 中断或回调 | 当前状态 | 关联对象或事件 | 位置 |
|---|---|---|---|
| TIM6_DAC_IRQHandler | 已实现，调用 `HAL_TIM_IRQHandler(&htim6)` | TIM6 全局中断 | stm32g4xx_it.c:204-213 |
| HAL_TIM_PeriodElapsedCallback | **未实现**（弱定义，无用户重载） | TIM6 更新事件 | 需在 USER CODE 区域添加 |
| TIM6_DAC_IRQn | NVIC 已使能，优先级 (5,0) | TIM6 中断 | hal_msp.c:100-101 |

说明：MSP 已完成中断向量使能；更新中断使能（`__HAL_TIM_ENABLE_IT`）与定时器启动（`HAL_TIM_Base_Start_IT`）以及周期到期回调 `HAL_TIM_PeriodElapsedCallback` 的实现在当前工程中尚缺失，属编程任务范畴。

### 3.5 主循环

`while(1)` 循环体为空，无阻塞轮询逻辑。当前程序上电后仅完成外设初始化，未启动定时器，LD2 保持初始熄灭状态。主循环预留 USER CODE WHILE / USER CODE 3 区域，可安全接入代码或保持空循环。

## 4. 工程与代码接入位置

### 4.1 源码与构建结构

- 主要源文件：
  - `Core/Src/main.c`（主程序、时钟与外设初始化、错误处理）
  - `Core/Src/stm32g4xx_it.c`（中断服务程序）
  - `Core/Src/stm32g4xx_hal_msp.c`（外设 MSP 初始化）
  - `Core/Src/system_stm32g4xx.c`（系统启动时钟）
  - `Core/Src/sysmem.c`、`syscalls.c`（运行时支持）
- 主要头文件：
  - `Core/Inc/main.h`（应用宏定义：引脚宏）
  - `Core/Inc/stm32g4xx_it.h`（中断服务原型）
  - `Core/Inc/stm32g4xx_hal_conf.h`（HAL 开关配置）
  - `Core/Inc/stm32g4xx_nucleo_conf.h`（BSP 配置）
- 源码接入方式：CubeMX 生成的 `Core/Src/*.c` 及启动文件由 `cmake/stm32cubemx/CMakeLists.txt` 中 `MX_Application_Src`/`STM32_Drivers_Src` 列表加入编译；顶层 `CMakeLists.txt` 通过 `add_subdirectory(cmake/stm32cubemx)` 引入，并为可执行文件 `TIM_IT_500ms` 提供 `target_sources`/`target_include_directories` 用户接入点。
- 头文件接入方式：`mx_Include_Dirs`（cmake/stm32cubemx/CMakeLists.txt:13-20）将 `Core/Inc`、HAL、BSP、CMSIS 目录加入全局包含路径；用户新增头文件可置于现有 include 目录或经顶层 CMakeLists 追加 include 路径。

### 4.2 代码接入位置

| 文件 | 位置 | 适用代码类型 |
|---|---|---|
| Core/Src/main.c | USER CODE BEGIN 2 / END 2（MX_TIM6_Init 之后） | 启动定时器、使能更新中断 |
| Core/Src/main.c | USER CODE BEGIN WHILE / 3 | 主循环程序逻辑 |
| Core/Src/stm32g4xx_it.c | USER CODE BEGIN Includes / PV / 0 | 中断相关自定义代码 |
| Core/Src/stm32g4xx_hal_msp.c | USER CODE BEGIN 区 | 外设初始化的 MSP 补充配置 |
| Core/Src/stm32g4xx_it.c | USER CODE BEGIN 1 / 中断回调区域 | 周期到期回调实现 |

说明：CubeMX 生成代码的 USER CODE 区域为安全接入点，重新生成工程时不会被覆盖。中断回调（如 `HAL_TIM_PeriodElapsedCallback`）通常在 USER CODE 区域实现。

### 4.3 自定义源文件

- 源文件位置：`Core/Src/`（按 CubeMX 工程结构）
- 头文件位置：`Core/Inc/`（或顶层 CMakeLists 追加 include 目录）
- 构建接入位置：顶层 `CMakeLists.txt` 的 `target_sources(... PRIVATE ...)`（CMakeLists.txt:46-48）添加自定义 `.c`，`target_include_directories(...)`（:51-53）添加自定义头文件目录；或将新文件加入 `cmake/stm32cubemx/CMakeLists.txt` 的 `MX_Application_Src` 列表。

## 5. 工程就绪状态

### 5.1 当前任务配置状态

| 任务需求 | 当前工程状态 | 依据 | 结论 |
|---|---|---|---|
| 系统时钟 HSI+PLL→SYSCLK 160MHz，APB1 定时器时钟 160MHz | 已配置 160MHz | main.c SystemClock_Config；.ioc RCC | 已具备 |
| TIM6 内部时钟源，Prescaler/Period 使更新周期=500ms | Prescaler 1599、Period 49999，周期 500ms | main.c:178-180 | 已具备 |
| TIM6 每 500ms 产生更新事件 | 硬件配置满足，但定时器未启动、更新中断未使能 | main.c MX_TIM6_Init 后未启动 | 已具备（初始化配置就绪，启动逻辑待实现） |
| 更新中断服务程序中切换 LD2(PA5) | 引脚输出已配置；IRQHandler 已存在；中断回调与 LED 切换逻辑未实现 | main.c、stm32g4xx_it.c | 待确认（引脚与 IRQ 已具备，回调实现待编程） |
| LD2(PA5) 高电平点亮，周期切换 | PA5 配置为输出、初始 RESET | main.c MX_GPIO_Init | 已具备 |

### 5.2 待确认事项

| 编号 | 待确认事项 | 影响 |
|---|---|---|
| 1 | TIM6 更新中断使能与定时器启动、`HAL_TIM_PeriodElapsedCallback` 中的 LED 切换逻辑尚未实现 | 属编程任务范畴，需 stm32-plan 规划代码实现；不影响本初始工程配置评估 |

结论：当前初始工程已具备实现项目目标所需的全部硬件与外设初始化配置条件（时钟、TIM6 定时参数、GPIO、TIM6 中断向量），实现 500ms 周期闪烁所需的功能逻辑（启动定时器、使能更新中断、周期回调切换 LED）需在后续编程任务中编写。
