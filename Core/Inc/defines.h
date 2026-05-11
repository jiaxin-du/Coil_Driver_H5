/*
 * defines.h
 *
 *  Created on: 7 May 2026
 *      Author: uqjdu2
 */

#ifndef INC_DEFINES_H_
#define INC_DEFINES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
   ST_OK = 0,
   ERR_OVERTEMP,
   ERR_PARAM_INVALID,
   ERR_HW_FAULT,
   ERR_CMD_INVALID,
   ERR_UNKNOW
} TState;

#ifdef __cplusplus
}
#endif

#endif /* INC_DEFINES_H_ */
