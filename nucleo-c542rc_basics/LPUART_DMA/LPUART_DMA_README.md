# LPUART_DMA 实验

## 1. 项目概述

本项目是 NUCLEO-C542RC 开发板（STM32C542RCT6 MCU）上的**串口 DMA 通信基础实验**。实验通过 LPUART1 外设经板载 ST-LINK 虚拟串口，以 **DMA 方式连续发送 JustFloat 格式**的数据帧，上位机使用 VOFA+（数据引擎选择 JustFloat）即可实时显示锯齿波波形。

实验内容分两个阶段实现：
- **T01**：建立 LPUART1 DMA 单帧发送能力，验证 JustFloat 数据帧格式正确。
- **T02**：生成锯齿波数据（幅度 0→100、步长 0.1、到顶复位），并实现 1ms 周期节拍连续发送，在 VOFA+ 中显示锯齿波形。

本项目只做"发送"，不接收、不做图片传输、不做文本打印，串口数据流中不添加换行符。

## 2. 硬件说明

### 2.1 开发板与 MCU

| 项 | 说明 |
|---|---|
| 开发板 | NUCLEO-C542RC（板卡型号 MB2213，Nucleo-64） |
| MCU | STM32C542RCT6（Cortex-M33，144 MHz，LQFP64） |
| 调试与串口 | 板载 ST-LINK/V3EC，提供程序下载、调试与虚拟串口（Virtual COM Port） |
| 供电 | USB Type-C 接口 |

### 2.2 与实验相关的引脚

| MCU 引脚 | 信号 | 功能 | 说明 |
|---|---|---|---|
| PA3 | LPUART1_TX | 发送 | 连接至 ST-LINK 虚拟串口 TX，MCU→PC 方向 |
| PA2 | LPUART1_RX | 接收 | 连接至 ST-LINK 虚拟串口 RX，本实验不使用 |
| PA13/PA14 | SWDIO/SWCLK | 调试 | ST-LINK 调试接口，程序下载与调试 |

> 说明：硬件概述资料中"PA2/PA3 接虚拟串口时应配置 USART2"为通用建议，本实验按需求将 PA2/PA3 复用为 **LPUART1**（以工程配置为准）。

### 2.3 通信与时钟

- LPUART1：波特率 **115200**，8 数据位，1 停止位，无校验，无硬件流控。
- 时钟：SYSCLK / HCLK / PCLK3 = 144 MHz，LPUART1 内核时钟源取 PCLK3。
- 时基：SYSTICK（HAL 1ms tick），用作 1ms 帧节拍来源。

## 3. 功能需求

### 3.1 功能要求（PR-001）

通过 LPUART1（板载 ST-LINK 虚拟串口）以 DMA 方式连续发送 JustFloat 格式的锯齿波数据，数据帧格式以项目目录下 `vofa_justfloat.md` 为准。锯齿波幅度从 0 递增至 100 后复位，重新从 0 开始，每帧递增步长 0.1，每 1ms 提交一帧。

### 3.2 条件与边界

- LPUART1 通信参数：波特率 115200，8 数据位，1 停止位，无校验，无硬件流控。
- 仅发送 JustFloat 采样数据帧；不实现图片传输；不实现普通文本打印；串口数据流中不添加换行符。

### 3.3 JustFloat 数据帧格式

JustFloat 采用小端浮点数组字节流协议，每帧由通道数据和 4 字节固定帧尾组成：

| 字节位置 | 内容 | 说明 |
|---|---|---|
| 0~3 | float 通道数据 | 4 字节小端字节序 |
| 4~7 | 帧尾 `0x00 0x00 0x80 0x7F` | 固定帧尾 |

本实验为单通道（CH_COUNT=1），每帧固定 8 字节。上位机 VOFA+ 数据引擎选择 JustFloat。

## 4. 实现思路

### 4.1 工程结构

本工程使用 STM32CubeMX2 生成（CMake 构建系统），系统初始化入口为 `mx_system_init()`（位于 `generated/hal/mx_system.c`），它依次完成 MPU、HAL、NVIC、ICACHE、时钟 RCC、GPIO 以及 LPUART1（含 DMA 联接）的初始化。用户应用代码从 `main()` 主循环接入。

### 4.2 阶段分解

- **T01**：新增 `app_justfloat.h/.c`，提供 JustFloat 帧构建与 DMA 发送接口；`main()` 中以约 100ms 间隔发送固定递增测试帧，验证单帧格式。
- **T02**：在 `app_justfloat.c` 中增加锯齿波状态生成、1ms 节拍判定与连续发送衔接；`main()` 主循环调用 `APP_JustFloat_UpdateTick()`。

### 4.3 关键机制

1. **数据帧缓冲**：使用模块级静态帧缓冲，DMA 异步读取期间缓存生命周期稳定，避免被覆盖。
2. **锯齿波生成**：用整数帧索引 0..1000 精确对应幅度 0.0..100.0，规避浮点累加误差；到上限后索引复位为 0。
3. **1ms 节拍**：基于 SYSTICK/HAL 时基（`HAL_GetTick()`），两次调用时间差 ≥1ms 才提交一帧，不新增硬件定时器。
4. **连续发送**：设置 DMA 发送忙标志；覆盖 `__weak HAL_UART_TxCpltCallback()`，一帧发送完成时清除标志，使下一节拍能启动下一帧，形成无间断循环；忙时跳过本帧，防止缓冲覆盖。

## 5. 关键代码

### 5.1 帧结构与接口定义（app_justfloat.h）

```c
/* 固定帧尾：0x00 0x00 0x80 0x7F */
#define APP_JUSTFLOAT_TAIL_0   0x00u
#define APP_JUSTFLOAT_TAIL_1   0x00u
#define APP_JUSTFLOAT_TAIL_2   0x80u
#define APP_JUSTFLOAT_TAIL_3   0x7Fu

/* 锯齿波生成参数 */
#define APP_JUSTFLOAT_STEP          0.1f    /* 每帧递增步长 */
#define APP_JUSTFLOAT_MAX           100.0f  /* 幅度上限，达到后复位 */
#define APP_JUSTFLOAT_PERIOD_FRAMES 1001u   /* 周期帧数：帧序号 0..1000 对应 0.0..100.0 */

/* 单通道 JustFloat 数据帧：前 4 字节小端 float 数据，后 4 字节固定帧尾 */
typedef struct
{
  float   ch_data;
  uint8_t frame_tail[4];
} app_justfloat_frame_t;

hal_status_t APP_JustFloat_SendFrame(hal_uart_handle_t *huart, float value);
void APP_JustFloat_Init(void);
void APP_JustFloat_UpdateTick(hal_uart_handle_t *huart);
```

### 5.2 帧构建与 DMA 发送（app_justfloat.c）

```c
/* 模块级发送缓冲：DMA 异步读取期间缓冲生命周期稳定 */
static app_justfloat_frame_t s_tx_frame;

hal_status_t APP_JustFloat_SendFrame(hal_uart_handle_t *huart, float value)
{
  s_tx_frame.ch_data       = value;
  s_tx_frame.frame_tail[0] = APP_JUSTFLOAT_TAIL_0;
  s_tx_frame.frame_tail[1] = APP_JUSTFLOAT_TAIL_1;
  s_tx_frame.frame_tail[2] = APP_JUSTFLOAT_TAIL_2;
  s_tx_frame.frame_tail[3] = APP_JUSTFLOAT_TAIL_3;

  /* 通过 LPUART1 已联接的 LPDMA1_CH0 启动 DMA 发送（无换行，纯字节流） */
  return HAL_UART_Transmit_DMA(huart, (const void *)&s_tx_frame, sizeof(s_tx_frame));
}
```

### 5.3 锯齿波生成与 1ms 周期节拍（app_justfloat.c）

```c
static uint32_t s_frame_index;      /* 帧序号 0..1000，对应幅度 0.0..100.0 */
static uint32_t s_send_in_progress; /* DMA 发送进行中标志 */

void APP_JustFloat_UpdateTick(hal_uart_handle_t *huart)
{
  /* 1ms 周期节拍：基于 SYSTICK/HAL 时基（HAL_GetTick） */
  static uint32_t s_last_tick_ms;
  if ((HAL_GetTick() - s_last_tick_ms) < 1u)
  {
    return;
  }
  s_last_tick_ms = HAL_GetTick();

  /* 等待上一帧 DMA 发送完成后再发送下一帧，避免发送缓冲被覆盖 */
  if (s_send_in_progress != 0u)
  {
    return;
  }

  /* 周期性复位：超过上限帧号（索引 1000）后复位重新从 0 开始 */
  if (s_frame_index > (APP_JUSTFLOAT_PERIOD_FRAMES - 1u))
  {
    s_frame_index = 0u;
  }

  /* 当前帧幅度：索引 0..1000 精确对应 0.0..100.0（步长 0.1，无浮点累积误差） */
  float wave_value = APP_JUSTFLOAT_STEP * (float)s_frame_index;

  /* 启动 DMA 发送并标志进行中（发送完成回调中清除） */
  (void)APP_JustFloat_SendFrame(huart, wave_value);
  s_send_in_progress = 1u;

  /* 下一帧索引递增 */
  s_frame_index++;
}
```

### 5.4 DMA 发送完成回调（app_justfloat.c）

```c
/* 覆盖 HAL 的 __weak 默认实现：一帧 DMA 发送完成，允许下一帧启动 */
void HAL_UART_TxCpltCallback(hal_uart_handle_t *huart)
{
  s_send_in_progress = 0u;
}
```

### 5.5 主程序接入（main.c）

```c
int main(void)
{
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }
  else
  {
    /* 获取 LPUART1 句柄（内含已联接的 LPDMA1_CH0 TX 通道） */
    hal_uart_handle_t *huart1 = mx_lpuart1_uart_gethandle();

    /* T02：锯齿波生成与 1ms 周期连续发送 */
    APP_JustFloat_Init();

    while (1)
    {
      /* 完成 1ms 节拍判定、锯齿波更新与 DMA 帧发送 */
      APP_JustFloat_UpdateTick(huart1);
    }
  }
}
```

## 6. HAL 依赖前提

代码依赖 STM32CubeMX2 生成的 HAL（含 DMA 外设驱动，见 `stm32c5xx_drivers` 与 `generated/hal`），关键前提如下。本实验已具备，无需额外配置：

| 依赖 | 现状 | 说明 |
|---|---|---|
| LPUART1 初始化 | 已具备 | `mx_lpuart1_uart_init()`：115200、8N1、无校验、无流控 |
| TX DMAD（LPDMA1_CH0） | 已具备 | 已联接到 LPUART1（`HAL_UART_SetTxDMA`），NVIC 已使能 |
| 时钟 | 已具备 | SYSCLK/HCLK/PCLK3 = 144 MHz，LPUART1 时钟源取 PCLK3 |
| SYSTICK 时基 | 已具备 | 1ms tick，`HAL_GetTick()` 可用 |
| TX 完成回调 | 已具备 | `HAL_UART_TxCpltCallback()`（`__weak`，可覆盖） |
| LPUART1 句柄 | 已具备 | 经 `mx_lpuart1_uart_gethandle()` 获取 |

> 前提要点：LPUART1 的 DMA TX 通道必须在 CubeMX 中已配置并联接到外设，NVIC 中断已使能（否则 DMA 发送完成回调不会触发，连续发送会停顿）。本工程初始配置已包含这些，无须改动硬件配置即可运行。

## 7. 编译烧录步骤

本工程为 CMake 构建（STM32CubeMX2 生成，GCC 工具链）。以 VS Code 环境为例：

1. **安装依赖**：确保已安装 STM32CubeMX2、Arm GCC 工具链、CMake；VS Code 插件（C/C++、CMake）可辅助。
2. **打开工程**：在 VS Code 中打开本项目根目录（含 `CMakePresets.json`、`CMakeLists.txt`）。
3. **配置构建**：执行 CMake 配置（选择预设 `debug_GCC_STM32C542RCT6`），生成构建目录。
4. **编译**：执行构建（Build），生成镜像文件（如 `.elf`/`.hex`）。
5. **下载**：使用 STM32CubeProgrammer 或 VS Code 下载工具，通过板载 ST-LINK（SWD）将镜像烧录到 NUCLEO-C542RC。程序下载后自动运行，从板载 USB 虚拟串口发送数据。
6. **连接串口**（上位机）：打开 VOFA+，协议选择 Serial，端口选板卡枚举出的 ST-LINK Virtual COM Port（如 COMx），波特率 115200（8N1），数据引擎选择 **JustFloat**，点击连接。

> 说明：本说明不提供逐条终端命令；具体构建/download 命令依你的 CMake 预设与工具链而定。若尚未编译烧录，直接按上述流程即可。

## 8. 测试结果

两个阶段均已编译烧录并通过运行验证：

| 阶段 | 验证项 | 结果 |
|---|---|---|
| T01 | 单帧 JustFloat 格式 | 通过：VOFA+ 中数值按 10→20→…→100 循环递增，帧格式可被正确解析，帧尾 `0x00 0x00 0x80 0x7F` 正确 |
| T02 | 锯齿波连续发送 | 通过：VOFA+ 显示锯齿波形连续上升至 100 后复位再循环，帧间隔约 1ms、一周期约 1000 帧（约 1s），与需求一致 |

最终运行效果：连接 VOFA+（JustFloat 引擎）后，可看到周期约 1 秒、幅度 0→100 循环的锯齿波波形。
