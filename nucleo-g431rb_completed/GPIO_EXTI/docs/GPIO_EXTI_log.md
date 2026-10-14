# STM32开发日志

项目名称：GPIO_EXTI

## 开发记录

### 2026-10-05 17:09 stm32-init

- 开发事项：项目文档初始化与项目任务需求准备
- 处理结果：创建 docs 文档目录结构与开发日志；确定开发板 NUCLEO-G431RB（STM32G431RBT6）；项目目标为按下用户按键 B1 触发外部中断并切换 LD2 状态；生成项目任务需求文件并经开发者确认，保存至 docs/requirements/proj_requirements.md
- 当前状态：项目文档初始化完成，任务需求已确认，等待使用 STM32CubeMX 完成硬件配置

### 2026-10-05 17:13 stm32-scan

- 开发事项：初始工程扫描分析
- 处理结果：分析 STM32CubeMX 生成的初始工程。PC13 已配置为 EXTI 上升沿中断输入（EXTI15_10_IRQn，优先级 5），PA5 已配置为推挽输出；SYSCLK 170 MHz（HSI→PLL）；IRQHandler 已生成但 HAL_GPIO_EXTI_Callback 未实现；发现 2 项待确认事项（触发边沿与按下电平方向一致性、PC13 无上下拉）。分析文件保存至 docs/analysis/init_scan.md
- 当前状态：初始工程分析完成，待确认事项影响后续编程任务规划

### 2026-10-05 17:15 stm32-plan

- 开发事项：编程任务规划
- 处理结果：拆分 2 项编程任务并经开发者确认细化：T01 按键中断与LD2切换实现（main.c USER CODE 4 实现 HAL_GPIO_EXTI_Callback）、T02 按键消抖处理（回调内时间间隔过滤）；补充说明提醒触发边沿与上拉配置事项。任务清单保存至 docs/tasks/task_list.md
- 当前状态：编程任务规划完成，等待按任务清单实施代码修改

### 2026-10-05 17:16 stm32-code T01

- 开发事项：按键中断与LD2切换实现（T01）
- 处理结果：在 main.c USER CODE 4 区域实现 HAL_GPIO_EXTI_Callback：识别 USER_BTN_Pin 触发源，调用 HAL_GPIO_TogglePin 翻转 LD2；未修改其它文件
- 当前状态：T01 代码修改完成，待开发者按任务清单验证要求验证；T02 待实施

### 2026-10-05 17:20 stm32-code T02

- 开发事项：按键消抖处理（T02）
- 处理结果：在 main.c USER CODE 4 的 HAL_GPIO_EXTI_Callback 内加入基于 HAL_GetTick 的时间间隔过滤：20 ms 消抖间隔，间隔内的中断视为抖动忽略，仅有效响应更新 last_tick；未修改其它文件
- 当前状态：T02 代码修改完成，待开发者按任务清单验证要求验证（T01 已验证通过）

### 2026-10-05 17:22 项目验证

- 开发事项：项目整体验证
- 处理结果：全部编程任务经开发者验证通过。编译烧录无误；按下/松开 B1 按键 LD2 亮灭改变；消抖后单次按压 LD2 仅翻转一次，无漏响应；项目功能要求全部满足
- 当前状态：项目完成
