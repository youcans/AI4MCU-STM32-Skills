/**
  ******************************************************************************
  * file           : main.c
  * brief          : Main program body
  *                  Calls target system initialization then loop in main.
  *
  * [实验] GPIO 外部中断 (GPIO_EXTI)
  * [目标] 按下用户按键 B1 (PC13) 时触发外部中断（上升沿触发），
  *        并切换板载 LED LD1 (PA5)。消抖时间 20 ms。
  ******************************************************************************
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "mx_gpio_default.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define B1_DEBOUNCE_MS         20U   /* 按键消抖时间 */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype -----------------------------------------------*/

/* EXTI 触发回调入口 */
static void ButtonExtiCallback(hal_exti_handle_t *hexti, hal_exti_trigger_t trigger);

/**
  * brief:  The application entry point.
  * retval: none but we specify int to comply with C99 standard
  */
int main(void)
{
  /** System Init: this code placed in targets folder initializes your system.
    * It calls the initialization (and sets the initial configuration) of the peripherals.
    * You can use STM32CubeMX to generate and call this code or not in this project.
    * It also contains the HAL initialization and the initial clock configuration.
    */
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }
  else
  {
    /*
      * You can start your application code here
      */
    hal_exti_handle_t *hexti;

    /* PC13 重配为上拉输入，保证空闲时电平稳定为高。
       按下拉低、释放回到高，从而在释放时产生上升沿(RISING)触发中断。 */
    hal_gpio_config_t gpio_config;
    gpio_config.mode = HAL_GPIO_MODE_INPUT;
    gpio_config.pull = HAL_GPIO_PULL_UP;
    HAL_GPIO_Init(B1_PORT, B1_PIN, &gpio_config);

    /* 注册 EXTI13 触发回调（沿用 CubeMX 默认的上升沿触发配置） */
    hexti = mx_gpio_default_exti13_gethandle();
    HAL_EXTI_RegisterTriggerCallback(hexti, ButtonExtiCallback);

    while (1)
    {
    }
  }
} /* end main */

/**
  * brief: EXTI 触发回调。在回调内以系统节拍做 20 ms 软件消抖，
  *        过滤按键抖动产生的连续触发，确认后切换 LED LD1。
  * note : 回调签名由 HAL2 EXTI 驱动定义：
  *        void (*cb)(hal_exti_handle_t *hexti, hal_exti_trigger_t trigger);
  */
static void ButtonExtiCallback(hal_exti_handle_t *hexti, hal_exti_trigger_t trigger)
{
  static uint32_t last_tick = 0U;
  uint32_t now;

  (void)hexti;
  (void)trigger;

  now = HAL_GetTick();

  /* 距上次有效响应不足 20 ms 的触发为抖动，忽略 */
  if ((now - last_tick) >= B1_DEBOUNCE_MS)
  {
    last_tick = now;
    HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);
  }
}
