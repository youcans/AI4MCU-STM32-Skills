/**
  ******************************************************************************
  * @file           : mx_tim1.h
  * @brief          : Header for mx_tim1.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_stm32c5xx_hal_drivers_license.md file
  * in the same directory as the generated code.
  * If no mx_stm32c5xx_hal_drivers_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MX_TIM1_H
#define MX_TIM1_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* Aliases of channels labels */
#define TIM1_CH1_CHANNEL  HAL_TIM_CHANNEL_1
#define TIM1_CH1N_CHANNEL  HAL_TIM_CHANNEL_1N

/** Primary aliases for TIM1_CH1 pin */
#define TIM1_CH1_PORT                         HAL_GPIOA
#define TIM1_CH1_PIN                          HAL_GPIO_PIN_8

/** Primary aliases for TIM1_CH1N pin */
#define TIM1_CH1N_PORT                        HAL_GPIOB
#define TIM1_CH1N_PIN                         HAL_GPIO_PIN_13

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for TIM in HAL layer */
/******************************************************************************/
/**
  * @brief  mx_tim1 init function.
  *         This function configures the hardware resources used in this example.
  * @retval Pointer to handle
  * @retval NULL in case of failure
  */
hal_tim_handle_t *mx_tim1_init(void);

/**
  * @brief  De-initialize mx_tim1 instance and return it.
  */
void mx_tim1_deinit(void);

/**
  * @brief  Get the mx_tim1 object.
  * @return Pointer on the mx_tim1 Handle
  */
hal_tim_handle_t *mx_tim1_gethandle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_TIM1_H */
