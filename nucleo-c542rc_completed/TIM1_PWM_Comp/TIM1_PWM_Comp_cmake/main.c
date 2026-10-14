/**
  ******************************************************************************
  * file           : main.c
  * brief          : Main program body
  *                  Calls target system initialization then loop in main.
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
#include "mx_tim1.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype -----------------------------------------------*/

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
    /* TIM1_CH1 (PA8) and TIM1_CH1N (PB13) complementary PWM, deadtime
     * already configured by mx_tim1_init() (500 ns). The timer is
     * initialized but the counter and the output channels are not yet started.
     */
    hal_tim_handle_t *htim1 = mx_tim1_gethandle();

    /* Start the output channels: enables CC1/CC1N and the main output MOE. */
    if (HAL_TIM_OC_StartChannel(htim1, HAL_TIM_CHANNEL_1) != HAL_OK)
    {
      return (-1);
    }
    if (HAL_TIM_OC_StartChannel(htim1, HAL_TIM_CHANNEL_1N) != HAL_OK)
    {
      return (-1);
    }

    /* Start the TIM1 counter to generate the PWM at 10 kHz/50% duty. */
    if (HAL_TIM_Start(htim1) != HAL_OK)
    {
      return (-1);
    }

    while (1) {}
  }
} /* end main */

