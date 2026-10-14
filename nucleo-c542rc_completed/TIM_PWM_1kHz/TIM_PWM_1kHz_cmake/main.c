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
/* Software-PWM carrier: the carrier is built by software from TIM6's 1 kHz
   update interrupt, because TIM6 is a basic timer (no hardware PWM channel).
   A "frame" is SOFT_PWM_STEPS consecutive 1 ms ticks (20 ms -> 50 Hz refresh).
   LED is ON during the first <duty_ticks> ticks of each frame, OFF for the rest,
   so duty_ticks encodes the LED brightness in 5 % steps.                          */
#define SOFT_PWM_STEPS          20U   /* 20 x 1 ms = 20 ms carrier frame         */
#define BREATH_PERIOD_MS        2000U /* full breathing cycle (10%->90%->10%)    */
#define MIN_DUTY_TICKS          2U    /* 10 % duty  (10 % of 20 ticks)           */
#define MAX_DUTY_TICKS          18U   /* 90 % duty  (90 % of 20 ticks)           */

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype -----------------------------------------------*/

/**
  * @brief TIM6 update interrupt callback (override of the __WEAK HAL routine).
  *        Called at 1 kHz (every 1 ms). Drives a software PWM on LD1 (PA5) and
  *        modulates its duty cycle with a triangle wave (10 % <-> 90 %) over
  *        BREATH_PERIOD_MS ms, producing the "breathing" LED effect.
  */
void HAL_TIM_UpdateCallback(hal_tim_handle_t *htim)
{
  static uint16_t breath_ms   = 0U;   /* 0..BREATH_PERIOD_MS-1 position in cycle */
  static uint8_t  duty_ticks  = MIN_DUTY_TICKS;
  static uint8_t  frame_tick  = 0U;   /* 0..SOFT_PWM_STEPS-1 within a frame     */

  /* Advance along the breathing cycle (1 ms per call). */
  breath_ms++;
  if (breath_ms >= BREATH_PERIOD_MS)
  {
    breath_ms = 0U;
  }

  /* Triangle: 10 % at the cycle edges, 90 % at the half-period middle point. */
  uint16_t half = BREATH_PERIOD_MS / 2U;
  uint16_t ph   = (breath_ms < half) ? breath_ms : (BREATH_PERIOD_MS - breath_ms);
  duty_ticks    = MIN_DUTY_TICKS +
                  ((uint32_t)(MAX_DUTY_TICKS - MIN_DUTY_TICKS) * ph) / half;

  /* Serve one tick of the software-PWM carrier frame. */
  frame_tick++;
  if (frame_tick >= SOFT_PWM_STEPS)
  {
    frame_tick = 0U;
  }

  HAL_GPIO_WritePin(LD1_PORT, LD1_PIN,
                    (frame_tick < duty_ticks) ? HAL_GPIO_PIN_SET
                                              : HAL_GPIO_PIN_RESET);
}

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

    /* Grab the TIM6 HAL handle and start the update interrupt.
       TIM6 fires at 1 kHz (prescaler 1439, period 99 on a 144 MHz clock) and the
       HAL_TIM_UpdateCallback() above runs the breathing PWM on LD1 (PA5). */
    hal_tim_handle_t *htim6 = mx_tim6_gethandle();
    if (HAL_TIM_Start_IT(htim6) != HAL_OK)
    {
      return (-1);
    }

    while (1) {}
  }
} /* end main */

