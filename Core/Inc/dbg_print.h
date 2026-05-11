/*
 * dbg_print.h
 *
 *  Created on: Oct 7, 2025
 *      Author: uqjdu2
 */

#ifndef INC_DBG_PRINT_H_
#define INC_DBG_PRINT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_cdc_if.h"

extern uint8_t gUSB_Connected;

uint8_t Hexify(uint8_t x_val);

// print out the hexified text of the data via CDC interface.
// x_len is the length of x_data
void DPrint_Hexify(uint8_t *x_data, uint16_t x_len);

// print out a message via CDC interface
#define DPrint(x_str, x_len)   do { if (gUSB_Connected != 0) { CDC_Send((uint8_t*)x_str, x_len); } } while(0)

#define DPrint_Putc(x_ch)      do { if (gUSB_Connected != 0) { CDC_Putc(x_ch); } } while(0)

// the constant string version. The x_str must be defined like this
// uint8_t tmp_str[] = "this is a test"; // can be printed out using DPrint_CStr(tmp_str); sizeof(tmp_ptr)-1) return length of the char string
// uint8_t *tmp_ptr = tmp_str + 2; // should NOT be printed out using DPrint_CStr(tmp_ptr); because sizeof(tmp_ptr)-1) always return 3
#define DPrint_CStr(x_str)    do { if (gUSB_Connected != 0) { CDC_Send((uint8_t*)x_str, (uint16_t)(sizeof(x_str)-1));} } while(0)

//print out a byte in hex, such as "0x02"
void DPrint_Hex_UInt8(uint8_t);

//print out unsigned char as a number, such as "12"
void DPrint_UInt8(uint8_t);

//print out unsigned char as a number, such as "12"
void DPrint_Int8(int8_t);

//print out unsigned short int in hex, such as "0x1234"
void DPrint_Hex_UInt16(uint16_t);

//print out unsigned short int as a number, such as "1234"
void DPrint_UInt16 (uint16_t x_val);

//print out signed short int as a number, such as "-1234"
void DPrint_Int16 (int16_t x_val);

//print out unsigned int in hex, such as "0x12345678"
void DPrint_Hex_UInt32(uint32_t);

//print out unsigned short int as a number, such as "1234"
void DPrint_UInt32 (uint32_t x_val);

//print out signed short int as a number, such as "-1234"
void DPrint_Int32 (int32_t x_val);

//print out floating point number with specified number of decimals
void DPrint_Float(double x_val, uint8_t x_decimals);

#define DPrint_Num(x_var)   _Generic((x_var),                         \
                               uint8_t:    DPrint_UInt8(x_var),       \
                               int8_t:     DPrint_Int8(x_var),        \
                               uint16_t:   DPrint_UInt16(x_var),      \
                               int16_t:    DPrint_Int16(x_var),       \
                               uint32_t:   DPrint_UInt32(x_var),      \
                               int32_t:    DPrint_Int32(x_var),       \
                               float:      DPrint_Float(x_var, 2),    \
                               double:     DPrint_Float(x_var, 2))

#define DPrint_Hex(x_var)   _Generic((x_var),                         \
                               uint8_t:    DPrint_Hex_UInt8(x_var),   \
                               uint16_t:   DPrint_UInt16(x_var),      \
                               uint32_t:   DPrint_UInt32(x_var),      \
                               default:    DPrint_UInt32(x_var))

#ifdef __cplusplus
}
#endif

#endif /* INC_DBG_PRINT_H_ */
