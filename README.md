# AI4MCU-STM32-Skills

English | [简体中文](README_zh-CN.md)

**Claude Code Skills for AI-Collaborative STM32 Basic Experiment Development**

- Project Name: AI4STM32 Skills
- GitHub Repository: `AI4MCU-STM32-Skills`
- Authors: Huang Xiaofeng & Huang Shan
- Email: ai4mcu@qq.com
- GitHub: [@youcans](https://github.com/youcans)

---

## Project Overview

AI4STM32 Skills is a set of Claude Code user-level Skills designed for STM32 basic experiment development. It organizes project requirement preparation, initial project analysis, programming task planning, code implementation, and development logging into a simplified and practical AI-collaborative STM32 development workflow.

The project mainly targets STM32 basic experiments involving GPIO, timers, PWM, ADC, DAC, UART, I2C, SPI, interrupts, DMA, RTC, watchdogs, Flash, and other common MCU functions. Claude Code works together with STM32CubeMX, VS Code, and real development boards throughout the development process.

AI4STM32 Skills currently consists of five Skills: `stm32-init`, `stm32-scan`, `stm32-plan`, `stm32-code`, and `stm32-log`.

The project follows two basic principles:

> **Accurate, clear, and simple.**

> **Start with the minimum implementation, make it work first, validate in practice, and iterate.**

---

## Project Components

AI4STM32 Skills mainly consists of Claude Code Skills, hardware documentation, basic experiment starter projects, and complete reference projects:

```text
AI4STM32 Skills
│
├── claude-user-skills/         # Provides the AI-collaborative development method
├── hardware/                   # Provides hardware documentation required for development
├── nucleo-g431rb_basics/        # Provides 15 basic experiment starter projects for NUCLEO-G431RB
├── nucleo-c542rc_basics/        # Currently provides 6 basic experiment starter projects for NUCLEO-C542RC
└── nucleo-g431rb_completed/    # Provides complete NUCLEO-G431RB reference projects developed with the Skill workflow
```

The `docs/` directory contains public project documentation for AI4STM32 Skills.

The main repository structure is:

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

The main directories are used as follows:

- `claude-user-skills/` contains the five Claude Code user-level Skills and their related template files;
- `hardware/` contains board hardware overviews, datasheets, schematics, user manuals, BOM files, and other hardware documentation;
- `nucleo-g431rb_basics/` provides `.ioc` files and `requirements.txt` files for 15 NUCLEO-G431RB basic experiments, allowing users to run the AI4STM32 Skills workflow themselves;
- `nucleo-c542rc_basics/` currently provides six basic experiment starter projects for NUCLEO-C542RC, with more experiments to be added;
- `nucleo-g431rb_completed/` provides complete NUCLEO-G431RB reference projects that have already gone through the full Skill development workflow. It currently contains seven complete projects;
- `docs/` contains public documentation for installation, usage, and Skill specifications.

---

## Skill System and Development Workflow

### Skill Overview

| Skill | Main Function | Invocation |
|---|---|---|
| `stm32-init` | Collects project goals, functional requirements, and hardware information, and generates the project requirements document | `/stm32-init [<User_Req>]` |
| `stm32-scan` | Analyzes the STM32CubeMX-generated initial project together with the project requirements | `/stm32-scan` |
| `stm32-plan` | Splits and refines programming tasks and generates the programming task list | `/stm32-plan` |
| `stm32-code` | Implements code changes for a specified programming task | `/stm32-code <task-id>` |
| `stm32-log` | Appends project development logs | Automatically called by other Skills |

In `stm32-init`, `<User_Req>` is an optional path and filename of a user requirement file. If the parameter is provided, the specified requirement file is read. If no parameter is provided, `requirements.txt` in the current project root directory is read by default. If the requirement file cannot be found, cannot be read, or does not provide sufficient information, the Skill obtains and refines the project requirements through dialogue with the developer.

### Typical Development Workflow

```text
Developer creates an STM32 project directory
        ↓
Enter the project directory and start Claude Code
        ↓
stm32-init
        │
        └── Generate the project requirements document
        ↓
Developer configures the project with STM32CubeMX
and generates the .ioc file and initial project
        ↓
Developer performs the initial Build
        ↓
stm32-scan
        │
        └── Generate the initial project analysis document
        ↓
stm32-plan
        │
        ├── Stage 1: Generate the programming task list
        │        ↓
        │     Developer confirmation
        │
        └── Stage 2: Refine each programming task
        ↓
stm32-code T01
        │
        └── Implement code changes for the current programming task
        ↓
Developer performs Build, flashing, debugging,
and hardware runtime validation
        ↓
stm32-code T02
        ↓
Continue with subsequent programming tasks
```

`stm32-log` is not used as an independent development stage. It is automatically called before the other business Skills exit and appends a development record.

---

## Quick Start

### 1. Get the Project

AI4STM32 Skills currently uses the following fixed hardware documentation path:

```text
C:\AI4MCU-STM32-Skills\hardware\
```

Therefore, the repository should be cloned to:

```text
C:\AI4MCU-STM32-Skills
```

Run the following command in Windows PowerShell:

```powershell
git clone https://github.com/youcans/AI4MCU-STM32-Skills.git C:\AI4MCU-STM32-Skills
```

To update the repository later:

```powershell
cd C:\AI4MCU-STM32-Skills
git pull
```

### 2. Install the Skills

Create the Claude Code user-level Skill directory:

```powershell
New-Item -ItemType Directory -Path "$HOME\.claude\skills" -Force | Out-Null
```

Copy the AI4STM32 Skills into the Claude Code user-level Skill directory:

```powershell
Copy-Item -Path "C:\AI4MCU-STM32-Skills\claude-user-skills\*" -Destination "$HOME\.claude\skills\" -Recurse -Force
```

### 3. Select a Basic Experiment

Select a basic experiment from `nucleo-g431rb_basics/` or `nucleo-c542rc_basics/` according to your development board. Each NUCLEO-G431RB experiment directory provides two starter files:

```text
<project-name>.ioc
requirements.txt
```

The following example uses the NUCLEO-G431RB `GPIO_EXTI` experiment:

```text
nucleo-g431rb_basics/
└── GPIO_EXTI/
    ├── GPIO_EXTI.ioc
    └── requirements.txt
```

Enter the experiment directory and start Claude Code:

```powershell
cd C:\AI4MCU-STM32-Skills\nucleo-g431rb_basics\GPIO_EXTI
claude
```

The directory in which Claude Code is started is treated as the STM32 project root directory. In this example, the project name is `GPIO_EXTI`.

### 4. Initialize the Project

Run:

```text
/stm32-init
```

Because `<User_Req>` is not provided, `stm32-init` automatically reads:

```text
requirements.txt
```

from the current project directory.

You can also explicitly specify another requirement file:

```text
/stm32-init <User_Req>
```

For example:

```text
/stm32-init D:\requirements\gpio_exti.txt
```

If the requirement file does not exist, cannot be read, or does not contain sufficient information, `stm32-init` obtains and refines the project requirements through dialogue.

After the project requirements have been prepared and confirmed, continue with STM32CubeMX project generation.

### 5. Generate the Initial Project

Open the provided `.ioc` file in STM32CubeMX, for example:

```text
GPIO_EXTI.ioc
```

Review the existing configuration as needed, generate the initial project, and perform the initial Build in VS Code to confirm that the generated project builds successfully.

### 6. Continue Development

Analyze the initial project:

```text
/stm32-scan
```

Plan the programming tasks:

```text
/stm32-plan
```

Implement the first programming task:

```text
/stm32-code T01
```

After the code modification is completed, the developer performs Build, flashing, debugging, and hardware runtime validation according to the validation requirements in the programming task list.

Continue with the next task:

```text
/stm32-code T02
```

For detailed installation, update, environment checking, and complete usage instructions, see:

[Installation and Usage Guide](docs/Installation_Guide.md)

---

## Basic Experiments

### `nucleo-g431rb_basics/`

The `nucleo-g431rb_basics/` directory provides 15 STM32 basic experiment starter projects for NUCLEO-G431RB, allowing users to run and validate the AI4STM32 Skills workflow themselves.

Each experiment directory contains only:

- a preconfigured STM32CubeMX `.ioc` file;
- a `requirements.txt` file describing the experiment goals and functional requirements.

The 15 experiments are listed below in the intended experiment sequence rather than alphabetical order:

```text
nucleo-g431rb_basics/
├── GPIO_IOToggle/      # GPIO output toggle and LED blinking experiment
├── GPIO_BUTTON/        # GPIO button input experiment
├── GPIO_EXTI/          # GPIO external interrupt experiment
├── TIM_IT_500ms/       # Timer 500 ms periodic interrupt experiment
├── TIM_PWM_1kHz/       # Timer 1 kHz PWM output experiment
├── TIM1_PWM_Comp/      # TIM1 complementary PWM output experiment
├── ADC_IT/             # ADC analog data acquisition experiment
├── DAC_Output/         # DAC analog signal output experiment
├── USART_Echo/         # USART data receive/transmit echo experiment
├── LPUART_DMA/         # LPUART DMA communication experiment
├── I2C_Master_Slave/   # I2C master-slave communication experiment
├── SPI_Master_Slave/   # SPI master-slave communication experiment
├── IWDG_Reset/         # Independent watchdog reset experiment
├── RTC_Alarm/          # RTC calendar and alarm experiment
└── Flash_Parameter/    # Flash parameter storage experiment
```

These experiments cover typical STM32 development topics including GPIO, timers, PWM, ADC/DAC, USART/LPUART, I2C, SPI, interrupts, DMA, RTC, independent watchdogs, and Flash.

Users can start from the provided `.ioc` and `requirements.txt` files and then run `stm32-init`, `stm32-scan`, `stm32-plan`, and `stm32-code` in sequence to complete the full AI-collaborative development workflow.

### `nucleo-c542rc_basics/`

The `nucleo-c542rc_basics/` directory provides basic experiment starter projects for NUCLEO-C542RC. It currently contains six experiments, with more to be added.

The current experiments are listed below in the intended experiment sequence:

```text
nucleo-c542rc_basics/
├── GPIO_IOToggle/      # GPIO output toggle and LED blinking experiment
├── GPIO_BUTTON/        # GPIO button input experiment
├── GPIO_EXTI/          # GPIO external interrupt experiment
├── TIM_IT_500ms/       # Timer 500 ms periodic interrupt experiment
├── TIM_PWM_1kHz/       # Timer 1 kHz PWM output experiment
└── TIM1_PWM_Comp/      # TIM1 complementary PWM output experiment
```

### `nucleo-g431rb_completed/`

The `nucleo-g431rb_completed/` directory contains complete NUCLEO-G431RB reference projects that have already been developed and validated using the AI4STM32 Skills workflow.

It currently provides seven complete reference projects. These projects demonstrate the complete results from user requirements and the STM32CubeMX initial project through project analysis, programming task planning, code implementation, and hardware validation.

A reference project may contain:

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

The actual files and directories may vary depending on the experiment.

`nucleo-g431rb_basics/` and `nucleo-c542rc_basics/` provide the “experiment starting point” for their respective development boards, allowing users to run the AI4STM32 Skills workflow themselves. `nucleo-g431rb_completed/` provides the “complete reference result” for NUCLEO-G431RB, allowing users to review completed projects.

---

## Supported Hardware

The hardware documentation is located at:

```text
C:\AI4MCU-STM32-Skills\hardware\
```

The repository currently contains:

```text
hardware/
├── nucleo-g431rb/
└── nucleo-c542rc/
```

Currently supported development boards:

- NUCLEO-G431RB
- NUCLEO-C542RC

Each board directory contains a hardware overview prepared for the AI4STM32 Skills project, together with related datasheets, board schematics, user manuals, BOM files, and other hardware references.

`stm32-init` reads the corresponding hardware documentation according to the selected development board and uses it to provide hardware platform information, MCU pin definitions, and peripheral connection information for the project requirements.

---

## Documentation

Public AI4STM32 Skills documentation is stored in the `docs/` directory.

Main documents include:

- [Installation and Usage Guide](docs/Installation_Guide.md)
- [AI4MCU Skill Writing Specification](docs/AI4MCU_Skill编写规范.md)

When AI4STM32 Skills are used in a specific STM32 project, an independent `docs/` directory is generated in that project root directory to store the project requirements, initial project analysis, programming task list, and development log.

---

## License

Skills, code, and documentation originally developed for the AI4STM32 Skills project are released under the MIT License.

See:

[LICENSE](LICENSE)

Third-party materials and code included or referenced in this repository remain subject to their original copyright and license terms.

See:

[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

---

## Contact

- Project Name: AI4STM32 Skills
- GitHub Repository: `AI4MCU-STM32-Skills`
- Authors: Huang Xiaofeng & Huang Shan
- Email: ai4mcu@qq.com
- GitHub: [@youcans](https://github.com/youcans)
