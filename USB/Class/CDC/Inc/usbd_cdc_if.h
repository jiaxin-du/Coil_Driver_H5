/**
  ******************************************************************************
  * @file    usbd_cdc_if_template.h
  * @author  MCD Application Team
  * @brief   Header for usbd_cdc_if_template.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBD_CDC_IF_TEMPLATE_H
#define __USBD_CDC_IF_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc.h"
#include "circ_fifo.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define APP_RX_DATA_SIZE  1024
#define APP_TX_DATA_SIZE  1024

#define USB_CMD_SIZE_MAX  64

extern USBD_CDC_ItfTypeDef  USBD_CDC_fops;

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

//send a char via CDC
void CDC_Putc(uint8_t);

//send data
//@param x_data data to be sent
//@param x_len size of the data in x_data
void CDC_Send(uint8_t* x_data, uint16_t x_len);

//flush buffer out
void CDC_Flush(void);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_IF_TEMPLATE_H */

