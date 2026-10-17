# NUCLEO-C542RC 硬件概述

## 1. 硬件信息

### 1.1 开发板信息

- 开发板型号：NUCLEO-C542RC
- 板卡编号：MB2213
- 板卡类型：STM32 Nucleo-64
- MCU型号：STM32C542RCT6
- MCU封装：LQFP64
- 扩展接口：
  - Arduino Uno V3 接口
  - ST Morpho 扩展接口
- 调试接口：
  - ST-LINK/V3EC
  - SWD 调试接口
  - Virtual COM Port

主要板载资源：

- 用户LED
- 用户按键
- 复位按键
- BOOT按键
- USB Type-C 接口
- FDCAN接口
- ST-LINK/V3EC 调试和虚拟串口接口
- 外部扩展接口


### 1.2 MCU信息

- MCU型号：STM32C542RCT6
- MCU系列：STM32C5
- CPU核心：Arm Cortex-M33
- 最大主频：144 MHz
- Flash容量：256 KB
- SRAM容量：64 KB
- 工作电压：2.7 V ~ 3.6 V

主要内部资源：

- CPU资源：
  - Cortex-M33 内核
  - FPU
  - DSP 指令支持
  - MPU

- GPIO：
  - 多功能复用GPIO
  - 外部中断支持

- ADC/DAC：
  - 12-bit ADC
  - DAC

- 模拟资源：
  - 比较器 COMP
  - 运算放大器 OPAMP

- 定时器：
  - 高级控制定时器
  - 通用定时器
  - 基本定时器
  - 低功耗定时器

- 通信接口：
  - USART
  - UART
  - LPUART
  - SPI
  - I2C
  - I3C
  - FDCAN
  - USB FS

- DMA：
  - DMA控制器

- 数学与安全资源：
  - CORDIC 数学加速器
  - 安全启动和安全功能支持


### 1.3 板载资源

| 资源                   | MCU引脚           | STM32功能   | 用途           |
| ---------------------- | ----------------- | ----------- | -------------- |
| 用户LED                | 待确认            | GPIO        | LED输出        |
| 用户按键               | 待确认            | GPIO/EXTI   | 用户输入       |
| Reset按键              | NRST              | Reset       | MCU复位        |
| BOOT按键               | BOOT0             | Boot配置    | 启动模式选择   |
| ST-LINK Virtual COM TX | 待确认            | UART/LPUART | 调试串口发送   |
| ST-LINK Virtual COM RX | 待确认            | UART/LPUART | 调试串口接收   |
| SWDIO                  | SWDIO             | SWD调试     | 程序下载与调试 |
| SWCLK                  | SWCLK             | SWD调试     | 程序下载与调试 |
| USB Type-C             | USB FS            | USB通信     | USB实验        |
| FDCAN接口              | FDCAN_TX/FDCAN_RX | FDCAN       | CAN通信实验    |


说明：

- 板载资源具体GPIO连接关系根据 NUCLEO-C542RC 用户手册、原理图和 STM32C542RCT6 数据手册确定。
- ST-LINK/V3EC 提供程序下载、调试和 Virtual COM Port 功能。
- MCU引脚与板载资源可能存在复用关系，实际使用时需要结合 STM32CubeMX 配置确认。

## 2. MCU引脚与接口分配

## 2. MCU引脚与接口分配

说明：

- 本表用于 AI4MCU 硬件资料库，描述 STM32C542RCT6 MCU 引脚功能以及 NUCLEO-C542RC 开发板默认连接关系。
- MCU主要复用功能来自 STM32C542RCT6 数据手册。
- 开发板默认连接关系来自 NUCLEO-C542RC 用户手册和原理图。
- Arduino接口和ST Morpho接口映射尚未完成全部核对，未确认内容标记为“待确认”。
- 未根据其它Nucleo开发板进行推测。

| MCU引脚 | MCU主要复用功能 | Arduino接口 | ST Morpho接口 | Nucleo默认连接 | 备注 |
|---|---|---|---|---|---|
| PA0 | GPIO / ADC1_IN0 / TIM2_CH1 | 待确认 | 待确认 | 待确认 | |
| PA1 | GPIO / ADC1_IN1 / TIM2_CH2 | 待确认 | 待确认 | 待确认 | |
| PA2 | GPIO / ADC1_IN2 / LPUART1_RX / USART2_TX | 待确认 | 待确认 | ST-LINK Virtual COM TX | USART2用于Virtual COM |
| PA3 | GPIO / ADC1_IN3 / LPUART1_TX / USART2_RX | 待确认 | 待确认 | ST-LINK Virtual COM RX | USART2用于Virtual COM |
| PA4 | GPIO / ADC1_IN4 / DAC1_OUT1 / SPI1_NSS | 待确认 | 待确认 | 待确认 | |
| PA5 | GPIO / ADC1_IN5 / DAC2_OUT1 / SPI1_SCK / TIM2_CH1 | 待确认 | 待确认 | LD1用户LED | 使用SPI1时需注意LED复用影响 |
| PA6 | GPIO / ADC1_IN6 / SPI1_MISO / USART1_TX | 待确认 | 待确认 | 待确认 | |
| PA7 | GPIO / ADC1_IN7 / SPI1_MOSI / USART1_RX | 待确认 | 待确认 | 待确认 | |
| PA8 | GPIO / TIM1_CH1 | 待确认 | 待确认 | 待确认 | |
| PA9 | GPIO / USART1_TX / TIM1_CH2 | 待确认 | 待确认 | 待确认 | |
| PA10 | GPIO / USART1_RX / TIM1_CH3 | 待确认 | 待确认 | 待确认 | |
| PA11 | GPIO / USB_DM | 待确认 | 待确认 | USB FS DM | USB数据线 |
| PA12 | GPIO / USB_DP | 待确认 | 待确认 | USB FS DP | USB数据线 |
| PA13 | GPIO / SWDIO | - | 待确认 | ST-LINK SWDIO | 调试接口 |
| PA14 | GPIO / SWCLK | - | 待确认 | ST-LINK SWCLK | 调试接口 |
| PA15 | GPIO / SPI1_NSS / TIM2_CH1 | 待确认 | 待确认 | 待确认 | JTAG复用 |

| MCU引脚 | MCU主要复用功能 | Arduino接口 | ST Morpho接口 | Nucleo默认连接 | 备注 |
|---|---|---|---|---|---|
| PB0 | GPIO / ADC1_IN8 / TIM3_CH3 | 待确认 | 待确认 | 待确认 | |
| PB1 | GPIO / ADC1_IN9 / TIM3_CH4 | 待确认 | 待确认 | 待确认 | |
| PB2 | GPIO / ADC1_IN10 | 待确认 | 待确认 | 待确认 | |
| PB3 | GPIO / SPI1_SCK / JTDO | 待确认 | 待确认 | 待确认 | JTAG复用 |
| PB4 | GPIO / SPI1_MISO / NJTRST | 待确认 | 待确认 | 待确认 | JTAG复用 |
| PB5 | GPIO / SPI1_MOSI | 待确认 | 待确认 | 待确认 | |
| PB6 | GPIO / I2C1_SCL / USART1_TX | 待确认 | 待确认 | 待确认 | |
| PB7 | GPIO / I2C1_SDA / USART1_RX | 待确认 | 待确认 | 待确认 | |
| PB8 | GPIO / I2C1_SCL / FDCAN_RX | 待确认 | 待确认 | CAN收发器 RX | FDCAN接收 |
| PB9 | GPIO / I2C1_SDA / FDCAN_TX | 待确认 | 待确认 | CAN收发器 TX | FDCAN发送 |
| PB10 | GPIO / USART3_TX | 待确认 | 待确认 | 待确认 | |
| PB11 | GPIO / USART3_RX | 待确认 | 待确认 | 待确认 | |
| PB12 | GPIO / SPI2_NSS | 待确认 | 待确认 | 待确认 | |
| PB13 | GPIO / SPI2_SCK | 待确认 | 待确认 | 待确认 | |
| PB14 | GPIO / SPI2_MISO | 待确认 | 待确认 | 待确认 | |
| PB15 | GPIO / SPI2_MOSI | 待确认 | 待确认 | 待确认 | |

| MCU引脚 | MCU主要复用功能 | Arduino接口 | ST Morpho接口 | Nucleo默认连接 | 备注 |
|---|---|---|---|---|---|
| PC0 | GPIO / ADC1_IN11 | 待确认 | 待确认 | 待确认 | |
| PC1 | GPIO / ADC1_IN12 | 待确认 | 待确认 | 待确认 | |
| PC2 | GPIO / ADC1_IN13 | 待确认 | 待确认 | 待确认 | |
| PC3 | GPIO / ADC1_IN14 | 待确认 | 待确认 | 待确认 | |
| PC4 | GPIO / USART1_TX | 待确认 | 待确认 | 待确认 | |
| PC5 | GPIO / USART1_RX | 待确认 | 待确认 | 待确认 | |
| PC6 | GPIO / USART6_TX | 待确认 | 待确认 | 待确认 | |
| PC7 | GPIO / USART6_RX | 待确认 | 待确认 | 待确认 | |
| PC8 | GPIO / TIM8_CH3 | 待确认 | 待确认 | 待确认 | |
| PC9 | GPIO / TIM8_CH4 | 待确认 | 待确认 | 待确认 | |
| PC10 | GPIO / USART3_TX | 待确认 | 待确认 | 待确认 | |
| PC11 | GPIO / USART3_RX | 待确认 | 待确认 | 待确认 | |
| PC12 | GPIO / USART3_TX | 待确认 | 待确认 | 待确认 | |
| PC13 | GPIO / RTC / EXTI | 待确认 | 待确认 | USER Button B1 | 用户按键输入 |
| PC14 | GPIO / LSE_IN | - | 待确认 | LSE晶振 | |
| PC15 | GPIO / LSE_OUT | - | 待确认 | LSE晶振 | |

| MCU引脚 | MCU主要复用功能 | Arduino接口 | ST Morpho接口 | Nucleo默认连接 | 备注 |
|---|---|---|---|---|---|
| PD2 | GPIO / USART3_RX | 待确认 | 待确认 | 待确认 | |
| PE2 | GPIO | 待确认 | 待确认 | CAN_STBY | CAN收发器待机控制 |
| PH0 | OSC_IN | - | 待确认 | HSE晶振 | |
| PH1 | OSC_OUT | - | 待确认 | HSE晶振 | |
| PH2 | BOOT0 | - | 待确认 | BOOT Button B3 | 启动模式选择 |

### 引脚使用注意事项

- PA13 和 PA14 默认用于 SWD 调试接口，不建议基础实验中作为普通GPIO使用。
- PA11 和 PA12 用于 USB Full-Speed 数据通信，使用 USB 功能时不能同时作为普通GPIO使用。
- PA5 与板载用户LED LD1复用，使用 SPI1_SCK 或定时器相关功能时需注意板载LED影响。
- PB8/PB9同时支持 I2C1 和 FDCAN 功能，实际工程中根据项目需求选择对应复用功能。
- PA2/PA3连接 ST-LINK Virtual COM 时，应配置 USART2。
- SPI、USART、I2C/I3C 等外设存在多组复用引脚，实际工程配置由 STM32CubeMX 确定。
- Arduino接口和 ST Morpho 接口映射中标记为“待确认”的内容，需要根据对应硬件资料进一步补充。

## 3. MCU外设资源

### 3.1 GPIO资源

STM32C542RCT6提供多功能GPIO资源，支持：

- 输入模式；
- 输出模式；
- 外部中断模式；
- 外设复用模式。

GPIO可以通过STM32CubeMX配置为不同外设功能，包括：

- ADC模拟输入；
- 定时器通道；
- SPI通信接口；
- I2C/I3C通信接口；
- USART/UART/LPUART通信接口；
- FDCAN通信接口；
- USB通信接口。

GPIO具体引脚分配和复用关系见第2章“MCU引脚与接口分配”。


### 3.2 ADC/DAC/模拟资源

STM32C542RCT6包含模拟外设资源，用于模拟信号采集、处理和输出。

| 外设 | 资源 | 用途 |
|---|---|---|
| ADC | 12-bit ADC | 模拟信号采集 |
| DAC | DAC输出 | 模拟信号输出 |
| COMP | 模拟比较器 | 电压比较和阈值检测 |
| OPAMP | 运算放大器 | 模拟信号处理 |

模拟外设的具体引脚映射和通道配置由STM32CubeMX根据项目需求进行配置。


### 3.3 定时器资源

STM32C542RCT6包含多种定时器资源：

| 定时器类型 | 主要用途 |
|---|---|
| 高级控制定时器 | PWM输出、电机控制等高级控制应用 |
| 通用定时器 | 定时、输入捕获、输出比较、PWM生成 |
| 基本定时器 | 基础时间管理 |
| 低功耗定时器 | 低功耗计时和周期唤醒 |

定时器实例、通道分配和GPIO复用关系由STM32CubeMX配置确定。


### 3.4 通信资源

#### USART/UART/LPUART

STM32C542RCT6支持多种串行通信接口：

- USART；
- UART；
- LPUART。

主要用于：

- 调试信息输出；
- 外部设备通信；
- 传感器数据交互。

具体串口实例、GPIO复用和DMA配置由STM32CubeMX根据项目需求确定。


#### SPI

STM32C542RCT6支持SPI通信接口。

主要用于：

- 外部传感器通信；
- 存储器通信；
- 扩展模块通信。

SPI实例、通信模式和GPIO复用关系由STM32CubeMX配置确定。


#### I2C/I3C

STM32C542RCT6支持：

- I2C通信接口；
- I3C通信接口。

主要用于：

- 传感器通信；
- 外部低速设备扩展。

具体接口实例和引脚配置由STM32CubeMX确定。


#### FDCAN

STM32C542RCT6支持FDCAN通信接口。

主要用于：

- CAN FD通信实验；
- 工业通信扩展。

FDCAN实例、GPIO复用和通信参数由STM32CubeMX配置确定。


#### USB

STM32C542RCT6支持USB Full-Speed通信。

主要用于：

- USB设备通信；
- USB扩展应用。

USB引脚配置和功能模式由STM32CubeMX配置确定。


### 3.5 系统资源

#### 时钟资源

STM32C542RCT6支持多种时钟源：

- 外部高速时钟HSE；
- 外部低速时钟LSE；
- 内部时钟源。

NUCLEO-C542RC板载晶振信息见硬件平台说明。


#### DMA资源

STM32C542RCT6包含DMA控制器，用于外设数据传输。

典型应用包括：

- ADC数据传输；
- UART/USART数据收发；
- SPI数据传输；
- 定时器事件处理。

DMA通道和请求映射由STM32CubeMX配置确定。


#### 调试资源

STM32C542RCT6支持SWD调试接口。

NUCLEO-C542RC通过板载ST-LINK/V3EC提供：

- 程序下载；
- 在线调试；
- 调试通信功能。


#### 安全资源

STM32C542RCT6基于Arm Cortex-M33内核，支持相关安全功能。

安全功能配置根据项目需求和STM32CubeMX配置确定。


## 4. 硬件资源文件

| 文件 | 用途 |
|---|---|
| NUCLEO-C542RC_User_Manual_UM3615.pdf | 开发板用户手册，用于确认开发板结构、板载资源、接口定义和I/O分配 |
| NUCLEO-C542RC_Schematic_b02.pdf | 开发板原理图，用于确认MCU引脚连接、板载外设连接关系和硬件网络定义 |
| MB2213-C542RC-B02_BOM.xlsx | 开发板元器件清单，用于确认板载器件型号和硬件组成 |

后续可根据项目需求增加其它硬件资源文件。