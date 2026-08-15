/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FIRE_SENSOR_Pin GPIO_PIN_0
#define FIRE_SENSOR_GPIO_Port GPIOA
#define GAS_SENSOR_Pin GPIO_PIN_1
#define GAS_SENSOR_GPIO_Port GPIOA
#define DHT11_PIN_Pin GPIO_PIN_2
#define DHT11_PIN_GPIO_Port GPIOA
#define JOY_X_Pin GPIO_PIN_4
#define JOY_X_GPIO_Port GPIOA
#define JOY_Y_Pin GPIO_PIN_5
#define JOY_Y_GPIO_Port GPIOA
#define PUMP_RELAY_Pin GPIO_PIN_0
#define PUMP_RELAY_GPIO_Port GPIOB
#define SW_MODE_Pin GPIO_PIN_1
#define SW_MODE_GPIO_Port GPIOB
#define BTN_RESET_Pin GPIO_PIN_2
#define BTN_RESET_GPIO_Port GPIOB
#define LED_RED_Pin GPIO_PIN_12
#define LED_RED_GPIO_Port GPIOB
#define LED_GREEN_Pin GPIO_PIN_13
#define LED_GREEN_GPIO_Port GPIOB
#define LED_YELLOW_Pin GPIO_PIN_14
#define LED_YELLOW_GPIO_Port GPIOB
#define SERVO_PWM_Pin GPIO_PIN_6
#define SERVO_PWM_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
