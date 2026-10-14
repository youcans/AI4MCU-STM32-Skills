/**
  ******************************************************************************
  * file           : main.c
  * brief          : Main program body
  *                  GPIO button input experiment: polling the user button (B1/PC13)
  *                  in the main loop, toggling the onboard LED (LD1/PA5) on each
  *                  valid press. A button held down is not re-triggered; it only
  *                  responds again once released. Debounce time is 20ms.
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

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DEBOUNCE_TIME_MS    20u   /* 按键消抖时间 */
#define B1_PRESSED_STATE    HAL_GPIO_PIN_RESET   /* 按下时 B1(PC13) 为低电平 */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype -----------------------------------------------*/
static void Button_Scan(void);

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
    while (1)
    {
      Button_Scan();
    }
  }
} /* end main */

/**
  * brief:  Poll user button B1 (PC13) and toggle onboard LED LD1 (PA5) on each
  *         valid press. Debounces the press, and waits for the release so that
  *         holding the button down does not re-trigger.
  * retval: none
  */
static void Button_Scan(void)
{
  if (HAL_GPIO_ReadPin(B1_PORT, B1_PIN) == B1_PRESSED_STATE)
  {
    /* A press is detected, confirm it after the debounce time */
    HAL_Delay(DEBOUNCE_TIME_MS);
    if (HAL_GPIO_ReadPin(B1_PORT, B1_PIN) == B1_PRESSED_STATE)
    {
      /* Valid press: toggle the onboard LED */
      HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);

      /* Wait for the release before scanning again */
      while (HAL_GPIO_ReadPin(B1_PORT, B1_PIN) == B1_PRESSED_STATE)
      {
      }
    }
  }
} /* end Button_Scan */
