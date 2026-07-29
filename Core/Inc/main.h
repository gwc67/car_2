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
#include "stdbool.h"
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
#define LED_PID_Pin GPIO_PIN_13
#define LED_PID_GPIO_Port GPIOC
#define LEFT_PWM_Pin GPIO_PIN_0
#define LEFT_PWM_GPIO_Port GPIOA
#define RIGHT_PWM_Pin GPIO_PIN_1
#define RIGHT_PWM_GPIO_Port GPIOA
#define Key4_Pin GPIO_PIN_4
#define Key4_GPIO_Port GPIOA
#define Key3_Pin GPIO_PIN_5
#define Key3_GPIO_Port GPIOA
#define Key2_Pin GPIO_PIN_0
#define Key2_GPIO_Port GPIOB
#define Key1_Pin GPIO_PIN_1
#define Key1_GPIO_Port GPIOB
#define S4_Pin GPIO_PIN_10
#define S4_GPIO_Port GPIOB
#define S3_Pin GPIO_PIN_11
#define S3_GPIO_Port GPIOB
#define S2_Pin GPIO_PIN_12
#define S2_GPIO_Port GPIOB
#define S1_Pin GPIO_PIN_13
#define S1_GPIO_Port GPIOB
#define RIGHT_DIR_Pin GPIO_PIN_14
#define RIGHT_DIR_GPIO_Port GPIOB
#define LEFT_DIR_Pin GPIO_PIN_15
#define LEFT_DIR_GPIO_Port GPIOB
#define CE_Pin GPIO_PIN_8
#define CE_GPIO_Port GPIOA
#define CSN_Pin GPIO_PIN_15
#define CSN_GPIO_Port GPIOA
#define OLED_SCK_Pin GPIO_PIN_8
#define OLED_SCK_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_9
#define OLED_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
