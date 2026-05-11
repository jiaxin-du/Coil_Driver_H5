/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins
     PH0-OSC_IN(PH0)   ------> RCC_OSC_IN
     PA13(JTMS/SWDIO)   ------> DEBUG_JTMS-SWDIO
     PA14(JTCK/SWCLK)   ------> DEBUG_JTCK-SWCLK
     PB3(JTDO/TRACESWO)   ------> DEBUG_JTDO-SWO
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SYNC_FB_GPIO_Port, SYNC_FB_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, OT_LATCH_RSTN_Pin|HV_EN_MCU_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(MCU_TEST_PIN_GPIO_Port, MCU_TEST_PIN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SYNC_FBC10_GPIO_Port, SYNC_FBC10_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, CAP_DISCH_Pin|CAP_SEL_Pin|CAP_SW_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : SYNC_FB_Pin */
  GPIO_InitStruct.Pin = SYNC_FB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SYNC_FB_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ZERO_CROSS_DETECT_Pin */
  GPIO_InitStruct.Pin = ZERO_CROSS_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ZERO_CROSS_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : OT_LATCH_RSTN_Pin HV_EN_MCU_Pin */
  GPIO_InitStruct.Pin = OT_LATCH_RSTN_Pin|HV_EN_MCU_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : OVERTEMP_SD_Pin */
  GPIO_InitStruct.Pin = OVERTEMP_SD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OVERTEMP_SD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OVERTEMP_ANYN_Pin */
  GPIO_InitStruct.Pin = OVERTEMP_ANYN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OVERTEMP_ANYN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : MCU_TEST_PIN_Pin */
  GPIO_InitStruct.Pin = MCU_TEST_PIN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(MCU_TEST_PIN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_DETECT_Pin */
  GPIO_InitStruct.Pin = USB_DETECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_DETECT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SYNC_FBC10_Pin */
  GPIO_InitStruct.Pin = SYNC_FBC10_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SYNC_FBC10_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : HV_ERRN_Pin */
  GPIO_InitStruct.Pin = HV_ERRN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HV_ERRN_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : HV_EN_CONSOLE_Pin */
  GPIO_InitStruct.Pin = HV_EN_CONSOLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(HV_EN_CONSOLE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : CAP_DISCH_Pin CAP_SEL_Pin CAP_SW_Pin */
  GPIO_InitStruct.Pin = CAP_DISCH_Pin|CAP_SEL_Pin|CAP_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI9_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_IRQn);

  HAL_NVIC_SetPriority(EXTI10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI10_IRQn);

  HAL_NVIC_SetPriority(EXTI12_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI12_IRQn);

}

/* USER CODE BEGIN 2 */

/* USER CODE END 2 */
