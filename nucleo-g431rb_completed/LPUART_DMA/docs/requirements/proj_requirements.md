# 项目任务需求文件

## 1. 项目信息

- 项目名称：LPUART_DMA
- 项目类型：基础实验（LPUART + DMA）
- 项目目标：使用 LPUART1 通过板载虚拟串口，以 DMA 方式连续发送 JustFloat 格式的锯齿波数据，并在 VOFA+ 中显示波形
- 开发板型号：NUCLEO-G431RB
- MCU型号：STM32G431RBT6
- 文件状态：已确认

## 2. 项目功能要求

### PR-001 LPUART1 DMA连续发送锯齿波数据

- 功能要求：
  系统上电后，通过 LPUART1 连接板载虚拟串口（ST-LINK Virtual COM Port），以 DMA 方式连续发送 JustFloat 格式的锯齿波数据。数据在 PC 端 VOFA+ 打开对应串口后实时显示锯齿波波形。

- 条件与边界：
  - 发送通道：LPUART1（PA2/PA3，默认连接 ST-LINK Virtual COM）；
  - 发送方式：DMA（无需 CPU 逐字节搬运，连续发送）；
  - 数据内容：周期性锯齿波采样数据；
  - 数据格式：JustFloat 格式，按项目根目录 `vofa_justfloat.md` 规定：
    - 单帧通道数 CH_COUNT=1，数据为 4 字节 float（小端字节序）；
    - 帧结构为 `float ch_data[1] + 固定帧尾 0x00 0x00 0x80 0x7F`，每帧共 8 字节；
    - 串口数据流中仅包含上述数据帧，不添加换行符；不实现普通文本打印和图片传输；
  - VOFA+ 侧：连接串口后数据引擎选择 `JustFloat`；
  - 系统上电即可持续自动发送，无需外部触发；
  - 波特率与 VOFA+ 串口设置保持一致，由 STM32CubeMX 配置确定。

## 3. 硬件定义

### 3.1 硬件平台

- 开发板版本：NUCLEO-G431RB（板卡编号 MB1367，STM32 Nucleo-64）
- MCU封装：LQFP64
- 其它说明：系统时钟160MHz（PLL，时钟源为板载24MHz HSE）；SWD调试接口（ST-LINK/V3E）；ST-LINK Virtual COM 默认通过 LPUART1 与目标 MCU 通信

### 3.2 MCU引脚定义

| MCU引脚 | 网络名 | 外设和通道 | 电气状态 | 用途与备注 |
|---|---|---|---|---|
| PA2 | LPUART1_TX | LPUART1_TX / DMA | 默认连接 ST-LINK VCP | 虚拟串口发送，默认连接 LPUART1 |
| PA3 | LPUART1_RX | LPUART1_RX / DMA | 默认连接 ST-LINK VCP | 虚拟串口接收，默认连接 LPUART1 |
| PA13 | T_SWDIO | SWDIO | Serial_Wire | 程序下载与调试 |
| PA14 | T_SWCLK | SWCLK | Serial_Wire | 程序下载与调试 |
| PF0 | RCC_OSC_IN | HSE | 外部晶振输入 | 24MHz HSE晶振输入 |
| PF1 | RCC_OSC_OUT | HSE | 外部晶振输出 | 24MHz HSE晶振输出 |

### 3.3 外围器件连接

| 类别 | 器件或接口 | 信号 | MCU连接 | 用途与备注 |
|---|---|---|---|---|
| 调试/虚拟串口 | ST-LINK/V3E Virtual COM | LPUART1 收发 | PA2（TX）/PA3（RX） | PC 与 MCU 通信，VOFA+ 数据接收通道 |
| 调试接口 | ST-LINK/V3E | SWD | PA13/PA14 | 程序下载与调试 |
| 板载晶振 | HSE晶振 | 24MHz时钟 | PF0/PF1 | 系统时钟源 |

## 4. 待确认事项

- 波特率取值（通常 115200 或 921600）由 STM32CubeMX 配置确定，请确认后填写。
- 锯齿波采样点数、单次发送帧数据量、发送周期等参数未在需求中明确，请开发者确认。
