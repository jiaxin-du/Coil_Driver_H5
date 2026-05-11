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
#include "stm32h5xx_hal.h"

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
#define OSC_IN_Pin GPIO_PIN_0
#define OSC_IN_GPIO_Port GPIOH
#define SYNC_FB_Pin GPIO_PIN_3
#define SYNC_FB_GPIO_Port GPIOA
#define AMP_VDROP_SENSE_Pin GPIO_PIN_4
#define AMP_VDROP_SENSE_GPIO_Port GPIOC
#define ZERO_CROSS_DETECT_Pin GPIO_PIN_2
#define ZERO_CROSS_DETECT_GPIO_Port GPIOB
#define ZERO_CROSS_DETECT_EXTI_IRQn EXTI2_IRQn
#define OT_LATCH_RSTN_Pin GPIO_PIN_7
#define OT_LATCH_RSTN_GPIO_Port GPIOE
#define OVERTEMP_SD_Pin GPIO_PIN_8
#define OVERTEMP_SD_GPIO_Port GPIOE
#define OVERTEMP_ANYN_Pin GPIO_PIN_9
#define OVERTEMP_ANYN_GPIO_Port GPIOE
#define OVERTEMP_ANYN_EXTI_IRQn EXTI9_IRQn
#define HV_EN_MCU_Pin GPIO_PIN_10
#define HV_EN_MCU_GPIO_Port GPIOE
#define DAC_LOADN_Pin GPIO_PIN_10
#define DAC_LOADN_GPIO_Port GPIOB
#define DAC_SPI_NSS_Pin GPIO_PIN_12
#define DAC_SPI_NSS_GPIO_Port GPIOB
#define DAC_SPI_SCK_Pin GPIO_PIN_13
#define DAC_SPI_SCK_GPIO_Port GPIOB
#define DAC_SPI_DOUT_Pin GPIO_PIN_14
#define DAC_SPI_DOUT_GPIO_Port GPIOB
#define DAC_SPI_DIN_Pin GPIO_PIN_15
#define DAC_SPI_DIN_GPIO_Port GPIOB
#define MCU_TEST_PIN_Pin GPIO_PIN_10
#define MCU_TEST_PIN_GPIO_Port GPIOD
#define TMP_I2C_SCL_Pin GPIO_PIN_12
#define TMP_I2C_SCL_GPIO_Port GPIOD
#define TMP_I2C_SDA_Pin GPIO_PIN_13
#define TMP_I2C_SDA_GPIO_Port GPIOD
#define USB_DETECT_Pin GPIO_PIN_10
#define USB_DETECT_GPIO_Port GPIOA
#define USB_DETECT_EXTI_IRQn EXTI10_IRQn
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define SYNC_IN_Pin GPIO_PIN_15
#define SYNC_IN_GPIO_Port GPIOA
#define SYNC_FBC10_Pin GPIO_PIN_10
#define SYNC_FBC10_GPIO_Port GPIOC
#define HV_ERRN_Pin GPIO_PIN_11
#define HV_ERRN_GPIO_Port GPIOC
#define HV_EN_CONSOLE_Pin GPIO_PIN_12
#define HV_EN_CONSOLE_GPIO_Port GPIOC
#define HV_EN_CONSOLE_EXTI_IRQn EXTI12_IRQn
#define RS485_UART_DE_Pin GPIO_PIN_4
#define RS485_UART_DE_GPIO_Port GPIOD
#define RS485_UART_TXD_Pin GPIO_PIN_5
#define RS485_UART_TXD_GPIO_Port GPIOD
#define RS485_UART_RXD_Pin GPIO_PIN_6
#define RS485_UART_RXD_GPIO_Port GPIOD
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define CAP_DISCH_Pin GPIO_PIN_7
#define CAP_DISCH_GPIO_Port GPIOB
#define CAP_SEL_Pin GPIO_PIN_8
#define CAP_SEL_GPIO_Port GPIOB
#define CAP_SW_Pin GPIO_PIN_9
#define CAP_SW_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
