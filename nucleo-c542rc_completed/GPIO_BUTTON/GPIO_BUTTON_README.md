# 用户按键轮询实验 —— 工程编程说明

## 一、项目概述

本项目实现 **GPIO 按键输入实验**：在 **NUCLEO-C542RC**（搭载 **STM32C542RCT6**）开发板上，通过主循环**轮询**用户按键 **B1（PC13）**，每次**有效按下**时切换板载 LED **LD1（PA5）** 的状态（点亮 ⇄ 熄灭）。

**核心行为：**
- 按下一次 → LED 状态翻转一次；
- **持续按住不会重复触发**，松开后才可响应下一次按下；
- 按下信号经过 **20ms 消抖**，避免机械抖动导致的误触发。

---

## 二、硬件说明

### 2.1 引脚定义

| 引脚 | 信号 | 功能 | 有效电平 |
|------|------|------|----------|
| PA5 | LD1 | 板载 LED，输出模式 | **高电平（SET）点亮** |
| PC13 | B1 | 用户按键，输入模式 | **按下为低电平（RESET）** |

> NUCLEO-C542RC 的 B1 按键按下时与 GND 短接，读出为低电平；松开后恢复高电平。LED 采用低侧驱动，PA5 输出高电平时 LED 点亮。

### 2.2 引脚与宏对照表

工程由 CubeMX2 生成代码，引脚名在 [generated/hal/mx_gpio_default.h](generated/hal/mx_gpio_default.h) 中定义了别名宏，**不要在 main.c 里写死 `GPIO_PIN_5` / `GPIO_PIN_13`**，应使用下列宏以保持可移植性：

| 宏 | 值 | 说明 |
|----|----|------|
| `LD1_PORT` | `HAL_GPIOA` | LD1 所在 GPIO 口（端口 A） |
| `LD1_PIN` | `HAL_GPIO_PIN_5` | LD1 引脚号 5（PA5） |
| `B1_PORT` | `HAL_GPIOC` | B1 所在 GPIO 口（端口 C） |
| `B1_PIN` | `HAL_GPIO_PIN_13` | B1 引脚号 13（PC13） |
| `LD1_INIT_STATE` | `HAL_GPIO_PIN_RESET` | LD1 初始状态（熄灭） |
| `LD1_ACTIVE_STATE` | `HAL_GPIO_PIN_SET` | LD1 点亮电平（高） |

这些宏通过 `main.h` → `mx_hal_def.h` → `mx_gpio_default.h` 的包含链自动可见，因此在 [main.c](main.c) 中可直接使用。

---

## 三、功能需求

1. **轮询检测**：在主循环 `while(1)` 中持续读取 PC13 引脚电平。
2. **有效按下翻转**：每次检测到一次有效按下，翻转 PA5（LD1）状态。
3. **按住不重复**：按键持续按住期间不重复触发翻转。
4. **松开可再响应**：松开按键后，程序才恢复轮询，可响应下一次按下。
5. **消抖 20ms**：按下判定前延时 20ms 并二次确认，过滤机械抖动。

---

## 四、实现思路

采用 **“消抖 + 只响应一次 + 等待松开”** 的经典轮询方案：

```
检测 PC13 电平
    │
    ├─ 非按下 → 继续轮询
    │
    └─ 按下（低电平）
          ├─ HAL_Delay(20ms) 消抖
          ├─ 二次确认仍为按下？
          │      └─ 否 → 视为抖动，忽略
          ├─ 是 → 有效按下 → HAL_GPIO_TogglePin 翻转 LED
          └─ 阻塞等待松开（读到高电平）后才回到轮询
```

每一步使用的 HAL 函数如下表：

| 步骤 | HAL 函数 | 作用 |
|------|----------|------|
| 读取按键 | `HAL_GPIO_ReadPin(B1_PORT, B1_PIN)` | 读 PC13 电平，返回 `hal_gpio_pin_state_t` |
| 消抖延时 | `HAL_Delay(DEBOUNCE_TIME_MS)` | 延时 20ms |
| 翻转 LED | `HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN)` | 切换 PA5 输出电平 |

---

## 五、关键代码

以下是 [main.c](main.c) 中完整的 `while(1)` 主程序与按键处理函数：

```c
/* 本文件自定义的宏 */
#define DEBOUNCE_TIME_MS    20u                /* 按键消抖时间 */
#define B1_PRESSED_STATE    HAL_GPIO_PIN_RESET /* 按键按下时 B1(PC13) 为低电平 */

int main(void)
{
  if (mx_system_init() != SYSTEM_OK)   /* 系统与 GPIO 初始化（含 HAL_Init） */
  {
    return (-1);
  }
  else
  {
    while (1)                          /* 主循环：轮询按键 */
    {
      Button_Scan();                   /* 扫描按键并处理 LED */
    }
  }
}

/**
  * brief:  轮询用户按键 B1(PC13)，每次有效按下翻转板载 LED LD1(PA5)。
  *         按住不重复触发，松开后才响应下一次；消抖时间 20ms。
  */
static void Button_Scan(void)
{
  /* 1. 检测按键是否被按下（PC13 读到低电平 = 按下） */
  if (HAL_GPIO_ReadPin(B1_PORT, B1_PIN) == B1_PRESSED_STATE)
  {
    /* 2. 消抖：延时 20ms */
    HAL_Delay(DEBOUNCE_TIME_MS);       /* DEBOUNCE_TIME_MS = 20u */

    /* 3. 二次确认：延时后仍为按下，才认定是"有效按下"（否则为抖动，忽略） */
    if (HAL_GPIO_ReadPin(B1_PORT, B1_PIN) == B1_PRESSED_STATE)
    {
      /* 4. 有效按下：翻转 LD1（点亮 ⇄ 熄灭） */
      HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);

      /* 5. 阻塞等待松开：按住期间不断循环，保证"按住不重复触发" */
      while (HAL_GPIO_ReadPin(B1_PORT, B1_PIN) == B1_PRESSED_STATE)
      {
      }
    }
  }
}
```

**关键点提示：**
- 引脚宏（`B1_PORT`、`B1_PIN`、`LD1_PORT`、`LD1_PIN`）来自 `mx_gpio_default.h`（见 [第二节对照表](#22-引脚与宏对照表)），**勿写成硬编码引脚号**。
- 第 5 步的“等待松开”是整个方案“按住不重复、松开可再响应”的关键，不可缺少。
- 引脚状态用 `HAL_GPIO_PIN_SET` / `HAL_GPIO_PIN_RESET` 表达高低电平。

---

## 六、HAL 依赖前提

本代码依赖以下工程已就绪的前提，**不要自行改动**：

1. **系统初始化**：`mx_system_init()`（见 [mx_system.c](generated/hal/mx_system.c)）在进入主循环前已调用 `HAL_Init()`，完成 HAL 和时基的初始化。
2. **SysTick 时基已运行**：`SysTick_Handler` 已在 `mx_system.c` 中实现，调用 `HAL_IncTick()` 递增 tick，`mx_gpio_default_init()` 也已在系统初始化中完成 GPIO 配置。
3. **因此 `HAL_Delay()` 可用**：正是因为 SysTick 中断在跑，`HAL_Delay()` 才能提供基于 tick 的毫秒级延时，用于 20ms 消抖。

> 如果缺失上述前提（例如未调用 `HAL_Init`），`HAL_Delay()` 将因 tick 不递增而卡死或失效——这也是轮询式延时实现要特别注意的地方。

---

## 七、编译烧录步骤

1. **打开工程**：用 VS Code 打开本工程目录（`GPIO_Button_cmake`），或通过工程根目录的 `CMakePresets.json` 加载 CMake 预置。
2. **构建（Build）**：在 VS Code 中执行 CMake **Build**（对应调试配置 `debug_GCC_STM32C542RCT6`），等待编译通过，生成可执行/固件文件。
3. **烧录（Flash）**：执行 VS Code 的 **Flash** 任务，将固件下载到开发板。
4. **连接要求**：**请使用支持数据传输的 USB 线**（而非仅供电的充电线）连接 NUCLEO-C542RC 的 ST-LINK 接口，否则烧录会失败。

烧录并复位后，按下 B1 按键，即可看到 LD1 沿每次有效按下翻转。

---

## 八、测试结果

- [x] 按一下 B1 → LD1 翻转一次（亮 → 灭 或 灭 → 亮）；
- [x] 持续按住 B1 → LD1 只翻转一次，不重复触发；
- [x] 松开后再次按下 → 再次翻转，可连续响应；
- [x] 按下时无明显因抖动导致的误触发（20ms 消抖生效）。

**结论：功能正常。**

---

## 九、后续可扩展方向

1. **外部中断（EXTI）**：将 PC13 配置为 EXTI 上升/下降沿中断，在中断回调中翻转 LED，替代轮询，CPU 占用更低。
2. **长按 / 双击识别**：结合 `HAL_GetTick()` 记录按下/松开时刻，区分单击、双击、长按等不同操作。
3. **非阻塞消抖**：用状态机 + `HAL_GetTick()` 计时替代 `HAL_Delay()` 阻塞延时，避免消抖期间阻塞主循环其他任务。
4. **按键复用**：同一按键在不同状态（系统菜单/运行模式）下执行不同动作。
