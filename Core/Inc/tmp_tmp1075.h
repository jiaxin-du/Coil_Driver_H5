/*
 * tmp_tmp1075.h
 *
 *  Created on: 1 May 2026
 *      Author: uqjdu2
 */

#ifndef INC_TMP_TMP1075_H_
#define INC_TMP_TMP1075_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define TEMP_ALERT_THRESHOLD     (80.0f)    // in degree celsius
#define TEMP_LSB_PER_DEG         (16.0f)    // temperature
#define TEMP_DEG_PER_LSB         (0.0625f)  // temperature
#define TEMP_INVALID             (-273.15f)

enum tTmp_Sensor_Id {
   TEMP_SENSOR_1   = (0), //U15, left side of transformer
   TEMP_SENSOR_2   = (1), //U16, right side of transformer
   TEMP_SENSOR_3   = (2), //U51, close to power amp mosfet
   TEMP_SENSOR_NUM = (3)
};

//initialisation
void Temp_Init(void);

//read temperature from sensor idx
float Temp_Read(uint16_t idx);

//Start a temperature reading in interrupt mode
HAL_StatusTypeDef Temp_Read_Init(void);

//set the temperature limit for temperature monitoring
void Temp_Limit_Set(float x_low, float x_high);

//return the lower threshold for temperature monitoring
float Temp_Limit_Low(void);

//return the upper threshold for temperature monitoring
float Temp_Limit_High(void);

#ifdef __cplusplus
}
#endif
#endif /* INC_TMP_TMP1075_H_ */
