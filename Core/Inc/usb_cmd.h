/*
 * usb_cmd.h
 *
 *  Created on: Oct 14, 2025
 *      Author: jiaxindu
 */

#ifndef INC_USB_CMD_H_
#define INC_USB_CMD_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

//this function is called inside the CDC receiving callback function
void USB_Cmd_Proc(uint8_t*, uint32_t); // CDC receiving handler

#ifdef __cplusplus
}
#endif


#endif /* INC_USB_CMD_H_ */

