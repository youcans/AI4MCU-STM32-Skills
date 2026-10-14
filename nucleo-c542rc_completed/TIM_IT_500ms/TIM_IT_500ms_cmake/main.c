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
      * Start the periodic interrupt generation of TIM6.
      * TIM6 is configured to generate an update event every 500 ms,
      * the LD1 (PA5) LED is toggled in the HAL_TIM_UpdateCallback().
      */
    if (HAL_TIM_Start_IT(mx_tim6_gethandle()) != HAL_OK)
    {
      return (-1);
    }

    while (1) {}
  }
} /* end main */

/**
  * brief:  TIM6 update interrupt callback (period elapsed).
  *         Called by the HAL in IRQ context each time TIM6 reaches its period
  *         (every 500 ms). Toggle the on-board LED LD1 on PA5.
  * param:  htim  Pointer to the TIM handle.
  */
void HAL_TIM_UpdateCallback(hal_tim_handle_t *htim)
{
  HAL_GPIO_TogglePin(LD1_PORT, LD1_PIN);
}

