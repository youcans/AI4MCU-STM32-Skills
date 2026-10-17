# GPIO_IOToggle 实验

## 1. 项目概述

本项目在 **NUCLEO-C542RC** 开发板（STM32C542RCT6）上，将板载用户 LED（LD1，连接在 **PA5** 引脚）配置为 GPIO 推挽输出。系统运行后，每隔 500 ms 翻转一次 PA5 输出电平，驱动 LED 点亮与熄灭交替，实现 LED 闪烁（完整闪烁周期为 1 s，即亮 500 ms、灭 500 ms）。

这是一个最基础的 GPIO 输出实验，目的是熟悉 STM32CubeMX2 生成工程的结构，以及学会在 `main()` 主循环中调用 HAL 库接口控制 GPIO 输出。

## 2. 硬件说明

- 开发板：NUCLEO-C542RC（板卡编号 MB2213，STM32 Nucleo-64 板型）
- MCU：STM32C542RCT6，Cortex-M33 内核，最大主频 144 MHz
- 板载用户 LED：**LD1**，连接在 **PA5** 引脚
  - 推挽输出，**高电平点亮、低电平熄灭**，上电初始为低电平（灭）

在 CubeMX2 生成的工程中，PA5 的引脚宏统一命名为 `LD1`（`LD1_PORT` / `LD1_PIN`），与需求及硬件资料命名一致。

## 3. 功能需求

- 系统运行后，每隔 500 ms 翻转一次 PA5 输出电平，驱动板载用户 LED LD1 点亮与熄灭，实现闪烁。
- 翻转间隔为 500 ms（LED 每 500 ms 切换一次亮灭状态，完整闪烁周期为 1 s）。
- 不涉及按键输入、串口通信及其它外设。

## 4. 实现思路

PA5 的 GPIO 输出配置已由 **CubeMX2** 生成并在系统初始化中完成，编程阶段无需再配置引脚。任务只需在 `main()` 主循环中加入周期性翻转逻辑：

1. 系统调用 `mx_system_init()` 完成时钟、HAL、GPIO 等初始化，PA5 被配置为推挽输出且初值为低。
2. 进入 `while (1)` 主循环后，先用 `HAL_Delay(500)` 阻塞延时 500 ms（基于 SysTick 时基，无需新增外设）。
3. 再用 `HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN)` 翻转 PA5 输出电平。

如此反复执行，LED 即按 500 ms 间隔高/低交替，实现闪烁。整个过程无需新增中断或定时器外设。

## 5. 关键代码

修改文件为工程根目录的 `main.c`，在 `main()` 的 `else` 分支主循环中添加代码如下：

```c
    /*
      * You can start your application code here
      */
    while (1)
    {
      /* Toggle the onboard LED LD1 (PA5) every 500 ms */
      HAL_Delay(500);
      HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);
    }
```

- `HAL_Delay(500)`：阻塞延时 500 ms，基于系统 SysTick 时基。
- `HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN)`：翻转 PA5 引脚输出电平（高↔低）。`LD1_PORT`、`LD1_PIN` 宏由 `generated/hal/mx_gpio_default.h` 定义，经 `main.h` 间接包含，无需额外 `#include`。

## 6. HAL 依赖前提

- 系统时钟正常工作（HSIDIV3，SYSCLK/HCLK = 48 MHz）。
- SysTick 时基已启用，`HAL_Delay` 可用（已在 `mx_system.c` 的 `SysTick_Handler` 中调用了 `HAL_IncTick`）。
- PA5 已由 `mx_gpio_default_init()` 配置为 GPIO 推挽输出、初值低电平。
- 依赖 HAL GPIO 驱动（`stm32_hal.h`）与生成的引脚宏（`mx_gpio_default.h`）。

## 7. 编译烧录步骤

1. 用 STM32CubeMX2 打开工程根目录的 `GPIO_IOToggle.ioc2`，确认 **PA5 已配置为 GPIO 输出**（标签 LD1）。生成代码后，`generated/hal/mx_gpio_default.*` 会提供 `LD1_PORT` / `LD1_PIN` 宏。
2. 使用构建预设编译工程（工具链为 GCC 14.3.1）：
   ```sh
   cmake --preset debug_GCC_STM32C542RCT6
   ```
   然后执行构建，生成固件。
3. 通过板载 ST-LINK/V3EC 接口烧录固件到开发板（可使用 STM32CubeProgrammer 或将编译产物烧录）。
4. 复位运行，观察板载 LED LD1。

## 8. 测试结果

- 工程编译通过、烧录无误。
- 运行测试正确：板载 LED LD1 每隔 500 ms 翻转一次亮灭状态（亮 500 ms、灭 500 ms，完整闪烁周期 1 s），肉眼可见均匀闪烁。
- 项目功能目标全部达成。
