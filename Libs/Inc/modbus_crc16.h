#ifndef MODBUS_CRC16_H_
#define MODBUS_CRC16_H_

#include <stdint.h>

uint16_t Modbus_CRC16(uint8_t x_buff[], uint16_t x_len);

// uint16_t Modbus_CRC16_v1(uint8_t x_buff[], uint16_t x_len);
//
// uint16_t Modbus_CRC16_v2(uint8_t x_buff[], uint16_t x_len);

#endif /* MODBUS_CRC16_H_ */
