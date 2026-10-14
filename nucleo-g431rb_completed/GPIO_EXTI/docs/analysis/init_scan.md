# 初始工程分析

## 1. 工程与任务信息

- 工程名称：GPIO_EXTI
- 开发板型号：NUCLEO-G431RB
- MCU型号：STM32G431RBT6
- 当前任务目标：按下用户按键 B1 触发外部中断，切换板载 LD2 亮灭状态（PR-001、PR-002）
- 构建系统：CMake（STM32CubeMX 生成，HAL 及 BSP 以静态库 `stm32cubemx` 接入）

## 2. 任务相关硬件与外设配置

### 2.1 引脚配置

| 引脚 | 信号或功能 | 模式 | 关联外设 | 依据 |
|---|---|---|---|---|
| PC13 | USER_BTN（B1 用户按键） | GPIO 外部中断输入，上升沿触发，无上下拉 | EXTI13（EXTI15_10_IRQn） | main.c MX_GPIO_Init；main.h |
| PA5 | LD2（用户LED） | GPIO 推挽输出，低速，无上下拉 | GPIOA | main.c MX_GPIO_Init；main.h |

### 2.2 外设配置

#### GPIO / EXTI

| 配置项 | 当前配置 | 依据 |
|---|---|---|
| 按键引脚 | PC13（GPIO_PIN_13 / GPIOC） | main.h |
| 按键中断模式 | GPIO_MODE_IT_RISING（上升沿触发） | main.c |
| 按键上下拉 | GPIO_NOPULL | main.c |
| LED 引脚 | PA5（GPIO_PIN_5 / GPIOA） | main.h |
| LED 输出模式 | GPIO_MODE_OUTPUT_PP，GPIO_SPEED_FREQ_LOW | main.c |
| LED 初始输出 | GPIO_PIN_RESET（低电平） | main.c |
| 按键 EXTI 中断 | EXTI15_10_IRQn，抢占优先级 5，已使能 | main.c；GPIO_EXTI.ioc |

- 相关通道：EXTI13（按键中断线）
- 相关引脚：PC13（输入）、PA5（输出）
- 初始化函数：MX_GPIO_Init()（GPIO 配置与 EXTI 的 NVIC 设置均在其中完成）
- 句柄或对象：无独立句柄，引脚通过 main.h 中的宏直接操作
- 初始化后启动状态：EXTI15_10 中断已使能；LD2 输出低电平

### 2.3 DMA配置

无（当前任务未使用 DMA）。

### 2.4 相关时钟

| 时钟项目 | 当前配置或频率 | 关联对象 | 依据 |
|---|---|---|---|
| SYSCLK | 170 MHz，PLL 时钟源为 HSI（16 MHz ÷4 ×85 ÷2）；HSE 未启用 | CPU 内核 | main.c SystemClock_Config |
| HCLK | 170 MHz（AHB 不分频） | AHB 总线、GPIO（AHB2，GPIOA/GPIOC 时钟使能） | main.c |
| PCLK1/PCLK2 | 170 MHz（APB 不分频） | APB 外设 | main.c |
| 电源与 Flash | 电压调节 SCALE1_BOOST，Flash 延迟 4 个等待周期 | 电源、Flash | main.c SystemClock_Config |

## 3. 已有程序资源与运行结构

### 3.1 初始化顺序

```text
main()
├── HAL_Init()
├── SystemClock_Config()
├── MX_GPIO_Init()          // GPIO 引脚配置 + EXTI15_10 NVIC 优先级与使能
└── while (1)               // 空循环
```

### 3.2 句柄与全局对象

| 名称 | 类型 | 关联对象 | 定义位置 |
|---|---|---|---|
| BspButtonState | __IO uint32_t，初值 BUTTON_RELEASED（BSP 演示脚手架变量） | B1 按键状态 | main.c |

### 3.3 关键宏定义

| 宏名称 | 当前定义 | 用途 | 定义位置 |
|---|---|---|---|
| USER_BTN_Pin | GPIO_PIN_13 | B1 按键引脚 | main.h |
| USER_BTN_GPIO_Port | GPIOC | B1 按键端口 | main.h |
| USER_BTN_EXTI_IRQn | EXTI15_10_IRQn | B1 按键对应中断号 | main.h |
| LD2_Pin | GPIO_PIN_5 | LD2 引脚 | main.h |
| LD2_GPIO_Port | GPIOA | LD2 端口 | main.h |

### 3.4 中断与HAL回调

| 中断或回调 | 当前状态 | 关联对象或事件 | 位置 |
|---|---|---|---|
| EXTI15_10_IRQHandler | 已定义，调用 HAL_GPIO_EXTI_IRQHandler(USER_BTN_Pin) | PC13 外部中断（EXTI15~10 共享） | stm32g4xx_it.c |
| HAL_GPIO_EXTI_Callback | Core 源码中未定义，使用 HAL 弱默认空实现 | EXTI 中断后回调 | 无（需用户实现） |

### 3.5 主循环

while(1) 为空循环，无任何业务代码。按键响应需全部在中断处理链路（HAL_GPIO_EXTI_Callback）中完成。

## 4. 工程与代码接入位置

### 4.1 源码与构建结构

- 主要源文件：Core/Src/main.c（入口与初始化）、stm32g4xx_it.c（中断服务）、stm32g4xx_hal_msp.c（MSP）、system_stm32g4xx.c、syscalls.c、sysmem.c
- 主要头文件：Core/Inc/main.h、stm32g4xx_it.h、stm32g4xx_hal_conf.h、stm32g4xx_nucleo_conf.h
- 源码接入方式：HAL 与 NUCLEO BSP 由 cmake/stm32cubemx 子目录构建为静态库 `stm32cubemx` 链接至可执行文件；用户源码通过根 CMakeLists.txt 的 target_sources 接入
- 头文件接入方式：用户头文件目录通过根 CMakeLists.txt 的 target_include_directories 接入

### 4.2 代码接入位置

| 文件 | 位置 | 适用代码类型 |
|---|---|---|
| Core/Src/main.c | USER CODE BEGIN 4（文件末尾函数区） | HAL_GPIO_EXTI_Callback 回调实现、自定义函数 |
| Core/Src/main.c | USER CODE BEGIN 2（外设初始化之后） | 用户初始化代码（如需） |
| Core/Src/stm32g4xx_it.c | USER CODE BEGIN 1（文件末尾） | 中断相关的自定义辅助函数 |
| Core/Inc/main.h | USER CODE 区 | 自定义宏、全局声明 |

### 4.3 自定义源文件

- 源文件位置：项目根目录或 Core/Src 下新建（本任务逻辑简单，可直接在 main.c 的 USER CODE 区实现，无需新增源文件）
- 头文件位置：Core/Inc 下新建
- 构建接入位置：根 CMakeLists.txt 中 target_sources / target_include_directories 的用户注释区

## 5. 工程就绪状态

### 5.1 当前任务配置状态

| 任务需求 | 当前工程状态 | 依据 | 结论 |
|---|---|---|---|
| PR-001 按键外部中断检测 | PC13 已配置为 GPIO 外部中断输入（上升沿触发）；EXTI15_10_IRQn 已使能（优先级 5）；IRQHandler 已生成 | main.c；stm32g4xx_it.c | 已具备 |
| PR-002 LD2 状态切换 | PA5 已配置为推挽输出，初始输出低电平 | main.c | 已具备 |
| 中断触发边沿与"按下触发"的一致性 | 当前配置为上升沿触发；按键按下时电平变化方向无法从输入文件确认 | main.c | 待确认 |

### 5.2 待确认事项

| 编号 | 待确认事项 | 影响 |
|---|---|---|
| 1 | 按键按下/释放对应 PC13 电平变化的方向（当前配置为上升沿触发 IT_RISING，若按下对应下降沿，则中断将在释放时触发，需在 CubeMX 中改为下降沿或双边沿） | 决定按键中断是否在"按下"时触发，直接影响 PR-001 的验收 |
| 2 | PC13 配置为 GPIO_NOPULL，按键电路是否自带外部上拉未确认 | 若按键仅接地且无外部上拉，未按下时输入悬空，可能产生误触发 |
