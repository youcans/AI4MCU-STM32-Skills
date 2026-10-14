# AI4MCU-STM32-Skills

[English](README.md) | 简体中文

**基于Claude Code的STM32基础实验AI协同开发Skill**

- 项目名称：AI4STM32 Skills
- GitHub仓库：`AI4MCU-STM32-Skills`
- 作者：Huang Xiaofeng & Huang Shan
- 邮箱：ai4mcu@qq.com
- GitHub：[@youcans](https://github.com/youcans)

---

## 项目简介

AI4STM32 Skills是一套面向STM32基础实验开发的Claude Code用户级Skill体系，用于将项目任务需求整理、初始工程分析、编程任务规划、代码实现和开发记录组织为一套简化、可实际运行的AI协同STM32开发流程。

项目主要面向GPIO、定时器、PWM、ADC、DAC、UART、I2C、SPI、中断、DMA、RTC、看门狗、Flash等STM32基础实验，通过Claude Code与STM32CubeMX、VS Code及实际开发板配合完成开发。

AI4STM32 Skills当前由`stm32-init`、`stm32-scan`、`stm32-plan`、`stm32-code`和`stm32-log`5个Skill组成。

项目遵循：

> **准确、清晰、简洁。**

> **最小实现，先跑通；实践验证，再迭代。**

---

## 项目组成

AI4STM32 Skills主要由Claude Code Skill、硬件资料、基础实验起始项目和完整参考项目组成：

```text
AI4STM32 Skills
│
├── claude-user-skills/         # 提供AI协同开发方法
├── hardware/                   # 提供开发所需硬件资料
├── nucleo-g431rb_basics/        # 提供NUCLEO-G431RB的15个基础实验起始项目
├── nucleo-c542rc_basics/        # 当前提供NUCLEO-C542RC的6个基础实验起始项目
└── nucleo-g431rb_completed/    # 提供NUCLEO-G431RB已完成Skill开发流程的完整参考项目
```

仓库中的`docs/`目录用于保存AI4STM32 Skills公开项目文档。

主要目录结构如下：

```text
AI4MCU-STM32-Skills/
├── claude-user-skills/
├── nucleo-g431rb_basics/
├── nucleo-c542rc_basics/
├── nucleo-g431rb_completed/
├── docs/
├── hardware/
├── .gitignore
├── LICENSE
├── README.md
├── README_zh-CN.md
└── THIRD_PARTY_NOTICES.md
```

其中：

- `claude-user-skills/`保存5个Claude Code用户级Skill及相关模板文件；
- `hardware/`保存开发板硬件概述、数据手册、原理图、用户手册和BOM等资料；
- `nucleo-g431rb_basics/`提供NUCLEO-G431RB的15个基础实验的`.ioc`文件和`requirements.txt`，用于用户自行运行AI4STM32 Skills开发流程；
- `nucleo-c542rc_basics/`当前提供NUCLEO-C542RC的6个基础实验起始项目，后续继续增加实验；
- `nucleo-g431rb_completed/`提供NUCLEO-G431RB已经完成完整Skill开发流程的参考项目，目前包含7个完整项目；
- `docs/`保存AI4STM32 Skills安装、使用和Skill规范等公开项目文档。

---

## Skill体系与开发流程

### Skill一览

| Skill | 主要功能 | 调用方式 |
|---|---|---|
| `stm32-init` | 获取项目目标、功能要求和硬件信息，生成项目任务需求文件 | `/stm32-init [<User_Req>]` |
| `stm32-scan` | 结合项目任务需求分析STM32CubeMX生成的初始工程 | `/stm32-scan` |
| `stm32-plan` | 拆分并细化编程任务，生成编程任务清单 | `/stm32-plan` |
| `stm32-code` | 按照指定编程任务实施代码修改 | `/stm32-code <task-id>` |
| `stm32-log` | 追加项目开发日志 | 由其它Skill自动调用 |

`stm32-init`中的`<User_Req>`为可选的用户需求文件路径和名称。调用时提供该参数，则读取指定的需求文件；未提供参数时，默认读取当前项目根目录下的`requirements.txt`。如果未找到需求文件、文件读取失败或需求内容不充分，则通过对话交互获取和完善项目需求。

### 典型开发流程

```text
开发者创建STM32项目目录
        ↓
进入项目目录并启动Claude Code
        ↓
stm32-init
        │
        └── 生成项目任务需求文件
        ↓
开发者使用STM32CubeMX完成配置
并生成.ioc文件和初始工程
        ↓
开发者完成初始Build
        ↓
stm32-scan
        │
        └── 生成初始工程分析文件
        ↓
stm32-plan
        │
        ├── 第一阶段：生成编程任务列表
        │        ↓
        │     开发者确认
        │
        └── 第二阶段：逐项细化编程任务
        ↓
stm32-code T01
        │
        └── 实施当前编程任务代码修改
        ↓
开发者Build、烧录、调试和硬件运行验证
        ↓
stm32-code T02
        ↓
继续实施后续编程任务
```

`stm32-log`不作为独立开发阶段使用，而是在其它业务Skill结束前自动调用，用于追加开发记录。

---

## 快速开始

### 1. 获取项目

AI4STM32 Skills当前使用固定的硬件资料路径：

```text
C:\AI4MCU-STM32-Skills\hardware\
```

因此，应将仓库克隆到：

```text
C:\AI4MCU-STM32-Skills
```

在Windows PowerShell中执行：

```powershell
git clone https://github.com/youcans/AI4MCU-STM32-Skills.git C:\AI4MCU-STM32-Skills
```

后续更新项目：

```powershell
cd C:\AI4MCU-STM32-Skills
git pull
```

### 2. 安装Skill

创建Claude Code用户级Skill目录：

```powershell
New-Item -ItemType Directory -Path "$HOME\.claude\skills" -Force | Out-Null
```

将AI4STM32 Skills复制到Claude Code用户级Skill目录：

```powershell
Copy-Item -Path "C:\AI4MCU-STM32-Skills\claude-user-skills\*" -Destination "$HOME\.claude\skills\" -Recurse -Force
```

### 3. 选择基础实验

根据所用开发板，选择`nucleo-g431rb_basics/`或`nucleo-c542rc_basics/`中的基础实验。NUCLEO-G431RB的每个实验目录提供两个起始文件：

```text
<project-name>.ioc
requirements.txt
```

以下以NUCLEO-G431RB的`GPIO_EXTI`实验为例：

```text
nucleo-g431rb_basics/
└── GPIO_EXTI/
    ├── GPIO_EXTI.ioc
    └── requirements.txt
```

进入实验目录并启动Claude Code：

```powershell
cd C:\AI4MCU-STM32-Skills\nucleo-g431rb_basics\GPIO_EXTI
claude
```

Claude Code启动时所在目录即作为STM32项目根目录，当前项目名称为`GPIO_EXTI`。

### 4. 初始化项目

直接执行：

```text
/stm32-init
```

由于调用时未提供`<User_Req>`，`stm32-init`会自动读取当前目录中的：

```text
requirements.txt
```

也可以显式指定其它需求文件：

```text
/stm32-init <User_Req>
```

例如：

```text
/stm32-init D:\requirements\gpio_exti.txt
```

如果需求文件不存在、读取失败或内容不充分，`stm32-init`会通过对话交互获取和完善项目需求。

完成项目任务需求整理后，根据提示继续进行STM32CubeMX工程生成。

### 5. 生成初始工程

使用STM32CubeMX打开当前实验已经提供的`.ioc`文件，例如：

```text
GPIO_EXTI.ioc
```

检查配置后生成初始工程，并使用VS Code完成初始Build，确认初始工程能够正常构建。

### 6. 继续开发

分析初始工程：

```text
/stm32-scan
```

规划编程任务：

```text
/stm32-plan
```

实施第一个编程任务：

```text
/stm32-code T01
```

代码修改完成后，由开发者按照编程任务清单中的验证要求完成Build、烧录、调试和实际硬件运行验证。

继续实施后续任务：

```text
/stm32-code T02
```

详细安装、更新、环境检查和完整使用方法参见：

[安装与使用指南](docs/Installation_Guide_zh-CN.md)

---

## 基础实验

### `nucleo-g431rb_basics/`

`nucleo-g431rb_basics/`提供面向NUCLEO-G431RB的15个STM32基础实验起始项目，用于用户自行运行和验证AI4STM32 Skills开发流程。

每个实验目录只保留：

- 已经完成基础配置的STM32CubeMX`.ioc`文件；
- 描述实验目标和功能要求的`requirements.txt`文件。

当前15个基础实验按照实验实施顺序排列如下：

```text
nucleo-g431rb_basics/
├── GPIO_IOToggle/      # GPIO输出翻转闪灯实验
├── GPIO_BUTTON/        # GPIO按键输入实验
├── GPIO_EXTI/          # GPIO外部中断实验
├── TIM_IT_500ms/       # 定时器500ms周期中断实验
├── TIM_PWM_1kHz/       # 定时器1kHz PWM输出实验
├── TIM1_PWM_Comp/      # TIM1互补PWM输出实验
├── ADC_IT/             # ADC模拟量数据采集实验
├── DAC_Output/         # DAC模拟量信号输出实验
├── USART_Echo/         # USART数据收发回显实验
├── LPUART_DMA/         # LPUART DMA通信实验
├── I2C_Master_Slave/   # I2C主从通信实验
├── SPI_Master_Slave/   # SPI主从通信实验
├── IWDG_Reset/         # 独立看门狗复位实验
├── RTC_Alarm/          # RTC日历与闹钟实验
└── Flash_Parameter/    # Flash参数存储实验
```

这些实验覆盖GPIO、定时器、PWM、ADC/DAC、USART/LPUART、I2C、SPI、中断、DMA、RTC、独立看门狗和Flash等典型STM32开发内容。

用户可以从`.ioc`和`requirements.txt`开始，依次运行`stm32-init`、`stm32-scan`、`stm32-plan`和`stm32-code`，完成整个AI协同开发过程。

### `nucleo-c542rc_basics/`

`nucleo-c542rc_basics/`提供面向NUCLEO-C542RC的基础实验起始项目，当前包含6个实验，后续继续增加。

当前实验按照实验实施顺序排列如下：

```text
nucleo-c542rc_basics/
├── GPIO_IOToggle/      # GPIO输出翻转闪灯实验
├── GPIO_BUTTON/        # GPIO按键输入实验
├── GPIO_EXTI/          # GPIO外部中断实验
├── TIM_IT_500ms/       # 定时器500ms周期中断实验
├── TIM_PWM_1kHz/       # 定时器1kHz PWM输出实验
└── TIM1_PWM_Comp/      # TIM1互补PWM输出实验
```

### `nucleo-g431rb_completed/`

`nucleo-g431rb_completed/`保存面向NUCLEO-G431RB、已经按照AI4STM32 Skills项目的Skill体系完成开发和验证的完整参考项目。

当前提供7个完整参考项目，用于展示从用户需求、STM32CubeMX初始工程、工程分析、编程任务规划到代码实现和实际硬件验证后的完整结果。

参考项目通常包括：

```text
<project-name>/
├── cmake/
├── Core/
├── docs/
│   ├── tmp/
│   ├── requirements/
│   ├── analysis/
│   ├── tasks/
│   └── <project-name>_log.md
├── Drivers/
├── <project-name>.ioc
├── requirements.txt
├── CMakeLists.txt
├── startup_*.s
└── *.ld
```

具体文件和目录根据实验工程实际内容有所不同。

`nucleo-g431rb_basics/`和`nucleo-c542rc_basics/`分别提供两种开发板的“实验起点”，用于用户自行运行AI4STM32 Skills开发流程；`nucleo-g431rb_completed/`提供NUCLEO-G431RB的“完整参考”，用于查看已经完成开发后的参考结果。

---

## 支持的硬件

当前硬件资料库位于：

```text
C:\AI4MCU-STM32-Skills\hardware\
```

目前包含：

```text
hardware/
├── nucleo-g431rb/
└── nucleo-c542rc/
```

当前支持的开发板：

- NUCLEO-G431RB
- NUCLEO-C542RC

每个开发板目录包含AI4STM32 Skills项目整理的硬件概述文件，以及数据手册、开发板原理图、用户手册、BOM等相关硬件资料。

`stm32-init`根据开发板型号读取对应硬件资料，为项目任务需求整理提供硬件平台、MCU引脚定义和外围器件连接信息。

---

## 项目文档

AI4STM32 Skills公开项目文档保存在`docs/`目录。

主要文档包括：

- [安装与使用指南](docs/Installation_Guide_zh-CN.md)
- [AI4MCU Skill编写规范](docs/AI4MCU_Skill编写规范.md)

具体STM32项目在运行AI4STM32 Skills后，会在各自项目根目录中生成独立的`docs/`开发文档目录，用于保存项目任务需求、初始工程分析、编程任务清单和开发日志。

---

## 许可证

AI4STM32 Skills项目自行编写的Skill、代码和文档采用MIT License，具体许可条款见：

[LICENSE](LICENSE)

项目中引用或收录的第三方资料和代码仍遵循其原有版权和许可条件，相关说明见：

[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

---

## 联系方式

- 项目名称：AI4STM32 Skills
- GitHub仓库：`AI4MCU-STM32-Skills`
- 作者：Huang Xiaofeng & Huang Shan
- 邮箱：ai4mcu@qq.com
- GitHub：[@youcans](https://github.com/youcans)
