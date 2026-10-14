# 项目任务需求文件

## 1. 项目信息

- 项目名称：GPIO_IOToggle
- 项目类型：基础实验（GPIO输出）
- 项目目标：板载LD2 LED闪烁，每隔500ms切换一次亮灭状态
- 开发板型号：NUCLEO-G431RB
- MCU型号：STM32G431RBT6
- 文件状态：已确认

## 2. 项目功能要求

### PR-001 LD2 LED周期闪烁

- 功能要求：
  系统上电后，驱动板载用户LED LD2（PA5）输出，每隔500ms切换一次亮灭状态（500ms点亮、500ms熄灭交替），循环执行。

- 条件与边界：
  - 翻转周期：500ms（每500ms切换一次状态）；
  - 仅使用GPIO输出功能，无需按键、串口等其它外设参与；
  - 延时方式不限（软件延时、SysTick、定时器等均可）。

## 3. 硬件定义

### 3.1 硬件平台

- 开发板版本：NUCLEO-G431RB（板卡编号MB1367，STM32 Nucleo-64）
- MCU封装：LQFP64
- 其它说明：系统时钟160MHz（PLL，时钟源为板载24MHz HSE）；SWD调试接口（ST-LINK/V3E）

### 3.2 MCU引脚定义

| MCU引脚 | 网络名 | 外设和通道 | 电气状态 | 用途与备注 |
|---|---|---|---|---|
| PA5 | LD2 | GPIO_Output | 推挽输出，高电平点亮 | 板载用户LED，经焊桥SB6连接（默认ON） |
| PA13 | T_SWDIO | SWDIO | Serial_Wire | 程序下载与调试 |
| PA14 | T_SWCLK | SWCLK | Serial_Wire | 程序下载与调试 |
| PF0 | RCC_OSC_IN | HSE | 外部晶振输入 | 24MHz HSE晶振输入 |
| PF1 | RCC_OSC_OUT | HSE | 外部晶振输出 | 24MHz HSE晶振输出 |

### 3.3 外围器件连接

| 类别 | 器件或接口 | 信号 | MCU连接 | 用途与备注 |
|---|---|---|---|---|
| 板载LED | LD2 | GPIO输出 | PA5 | 用户LED，500ms周期切换亮灭 |
| 板载晶振 | HSE晶振 | 24MHz时钟 | PF0/PF1 | 系统时钟源 |
| 调试接口 | ST-LINK/V3E | SWD | PA13/PA14 | 程序下载与调试 |

## 4. 待确认事项

无
