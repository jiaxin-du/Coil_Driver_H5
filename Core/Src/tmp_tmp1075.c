/*
 * tmp_tmp1075.c
 *
 *  Created on: 1 May 2026
 *      Author: uqjdu2
 */

#include "i2c.h"
#include "tmp_tmp1075.h"
#include "dbg_print.h"

#define TMP1075_REG_RESULT  (0x00)
#define TMP1075_REG_CFG     (0x01)
#define TMP1075_REG_LOW     (0x02)
#define TMP1075_REG_HIGH    (0x03)
#define TMP1075_REG_ID      (0x40)

//I2C instance
extern I2C_HandleTypeDef hi2c4;
I2C_HandleTypeDef *Temp_I2C = &hi2c4;

uint8_t TempSensorAddr[TEMP_SENSOR_NUM] = { (0x48 << 1), (0x49 << 1), (0x4A << 1) };
uint8_t TempMeasRaw[2] = { 0x0, 0x0 }; //used to hold raw data from sensors

uint8_t CurrSensorId = 0; // sensor whose temperature is currently being read
uint8_t TempAlert = 0;

uint32_t PrevPrintTime = 0;
float TempMeasPrev[TEMP_SENSOR_NUM] = { TEMP_INVALID, TEMP_INVALID, TEMP_INVALID };
float TempMeasCurr[TEMP_SENSOR_NUM] = { 0.f, 0.f, 0.f };

float TempLimitHigh = TEMP_ALERT_THRESHOLD;
float TempLimitLow = TEMP_ALERT_THRESHOLD - 10;

static HAL_StatusTypeDef Tmp1075_Reg_Write(uint8_t, uint8_t, uint16_t);

static HAL_StatusTypeDef Tmp1075_Reg_Read(uint8_t, uint8_t, uint16_t*);

/**
 * @brief Write a 16-bit value to a TMP1075 register via I2C
 *
 * Uses STM32 HAL I2C Mem_Write to transfer a 16-bit register value to the
 * TMP1075 device. The 16-bit value is split into two bytes and transmitted
 * in big-endian order (MSB first) as required by the TMP1075 register format.
 *
 * @param[in] i2c_addr I2C device address (7-bit, already shifted)
 * @param[in] mem_addr Register address on TMP1075
 * @param[in] value 16-bit value to write
 *
 * @return HAL_OK if success
 */
HAL_StatusTypeDef Tmp1075_Reg_Write(uint8_t x_i2c_addr, uint8_t x_mem_addr, uint16_t x_val)
{
   uint8_t tmp_dat[2] = { (uint8_t) ((x_val >> 8) & 0xFF00), (uint8_t) (x_val & 0x00FF) };
   return HAL_I2C_Mem_Write(Temp_I2C, x_i2c_addr, x_mem_addr, I2C_MEMADD_SIZE_8BIT, tmp_dat, 2, 1000);
}

/**
 * @brief Read a 16-bit value from a TMP1075 register via I2C
 *
 * Uses STM32 HAL I2C Mem_Read to retrieve a 16-bit register value from the
 * TMP1075 device. Reads two bytes and reassembles them into a 16-bit value
 * in big-endian order (MSB first) as per the TMP1075 register format.
 *
 * @param[in] context Pointer to STM32 I2C_HandleTypeDef
 * @param[in] i2c_addr I2C device address (7-bit, already shifted)
 * @param[in] mem_addr Register address on TMP1075
 * @param[out] value Pointer to 16-bit variable to store the read value
 *
 * @return HAL_OK if success
 *
 * @note The value is only valid when HAL_OK is returned
 */
HAL_StatusTypeDef Tmp1075_Reg_Read(uint8_t x_i2c_addr, uint8_t x_mem_addr, uint16_t *x_val_ptr)
{
   uint8_t tmp_dat[2] = { 0, 0 };
   HAL_StatusTypeDef status = HAL_I2C_Mem_Read(Temp_I2C, x_i2c_addr, x_mem_addr, I2C_MEMADD_SIZE_8BIT, tmp_dat, 2, 1000);

   if (status == HAL_OK) {
      *x_val_ptr = ((uint16_t) tmp_dat[0] << 8 | (uint16_t) tmp_dat[1]);
   }

   return status;
}

// Initialise the sensors
void Temp_Init(void)
{
   uint16_t tmp_cfg = ((0x0U << 15) // 0->continous mode, 1->one shot
                     | (0x1U << 13) // bit[14:13] 0->27.5ms/sample, 1->55, 2->110, 3->220
                     | (0x0U << 11)  // bit[12:11] 0->one fault trigger alert, 1->two, 2->three, 3->four
                     | (0x0U << 10)  // bit[10] 0->active low alert, 1->high
                     | (0x0  << 9)    // bit[9] 0->alert in comparator mode, 1->interrupt mode
                     | (0x0  << 8));  // bit[8] 0->normal, 1->shutdown

   uint16_t idx = 0;
   for (idx = 0; idx < TEMP_SENSOR_NUM; ++idx) {
      Tmp1075_Reg_Write(TempSensorAddr[idx], TMP1075_REG_CFG, tmp_cfg);
      Tmp1075_Reg_Write(TempSensorAddr[idx], TMP1075_REG_HIGH, ((int16_t) (TempLimitHigh * TEMP_LSB_PER_DEG + 0.5)) << 4);
      Tmp1075_Reg_Write(TempSensorAddr[idx], TMP1075_REG_LOW, ((int16_t) (TempLimitLow * TEMP_LSB_PER_DEG + 0.5)) << 4);
   }
}

//return the limit of sensor alert
float Temp_Limit_Low(void)
{
   return TempLimitLow;
};

//return the limit of sensor alert
float Temp_Limit_High(void)
{
   return TempLimitHigh;
};

//set the temperature limit for temperature monitoring
void Temp_Limit_Set(float x_low, float x_high)
{
   uint16_t idx = 0;
   if (x_high > x_low) {
      TempLimitLow = x_low;
      TempLimitHigh = x_high;
   } else {
      TempLimitHigh = x_low;
      TempLimitLow = x_high;
   }

   for (idx = 0; idx < TEMP_SENSOR_NUM; ++idx) {
      Tmp1075_Reg_Write(TempSensorAddr[idx], TMP1075_REG_HIGH, ((int16_t) (TempLimitHigh * TEMP_LSB_PER_DEG + 0.5)) << 4);
      Tmp1075_Reg_Write(TempSensorAddr[idx], TMP1075_REG_LOW, ((int16_t) (TempLimitLow * TEMP_LSB_PER_DEG + 0.5)) << 4);
   }
}
//read the temperature from sensor idx
float Temp_Read(uint16_t idx)
{
   int16_t raw_temp = 0;

   if (Tmp1075_Reg_Read(TempSensorAddr[idx], TMP1075_REG_RESULT, (uint16_t*) &raw_temp) != HAL_OK) {
      return TEMP_INVALID;
   }
   return ((raw_temp >> 4) * TEMP_DEG_PER_LSB);
}

// this function issue a read command to the first sensors,
// the result will be processed in `HAL_I2C_MemRxCpltCallback`,
// and the reading command for next sensor will be also issued in that function
// execution time ~ 400 CPU cycles
HAL_StatusTypeDef Temp_Read_Init()
{
   CurrSensorId = TEMP_SENSOR_1;

   HAL_StatusTypeDef status = HAL_I2C_Mem_Read_IT(Temp_I2C, TempSensorAddr[CurrSensorId], TMP1075_REG_RESULT, \
         I2C_MEMADD_SIZE_8BIT, TempMeasRaw, 2);

   if (HAL_OK != status) {
      DPrint_CStr("initiate temperature reading failed, err="); DPrint_Num(status); DPrint_CStr("\r\n");
   }

   return status;
}

//Callback function when temperature reading finish
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
   HAL_StatusTypeDef status;
   uint8_t idx;
   float tmp_diff;

   if (hi2c->Instance != Temp_I2C->Instance || CurrSensorId >= TEMP_SENSOR_NUM) {
      return;
   }

   TempMeasPrev[CurrSensorId] = TempMeasCurr[CurrSensorId];
   TempMeasCurr[CurrSensorId] = (float) ((uint16_t) TempMeasRaw[0] << 4 | (uint16_t) TempMeasRaw[1] >> 4) * TEMP_DEG_PER_LSB;
   tmp_diff = TempMeasCurr[CurrSensorId] - TempMeasPrev[CurrSensorId];

   if (TempMeasCurr[CurrSensorId] > TempLimitHigh || tmp_diff < -10 || tmp_diff > 10) {
      TempAlert = 1;
   }

   ++CurrSensorId; // read the next one
   if (CurrSensorId < TEMP_SENSOR_NUM) {
      status = HAL_I2C_Mem_Read_IT(Temp_I2C, TempSensorAddr[CurrSensorId], TMP1075_REG_RESULT, I2C_MEMADD_SIZE_8BIT, TempMeasRaw, 2);
      if (HAL_OK != status) {
         DPrint_CStr("initiate temperature reading failed\r\n");
      }
   } else {
      if (TempAlert || (HAL_GetTick() - PrevPrintTime) > 10000) {
         PrevPrintTime = HAL_GetTick();
         DPrint_CStr("Temp|T=");
         for (idx = 0; idx < TEMP_SENSOR_NUM; ++idx) {
            DPrint_Num(TempMeasCurr[idx]); DPrint_Putc(' ');
         }
         DPrint_CStr("\r\n");
      }
      TempAlert = 0;
   }
};

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
   if (hi2c->Instance != Temp_I2C->Instance || CurrSensorId >= TEMP_SENSOR_NUM) {
      return;
   }
   DPrint_CStr("TMP|Read temperature error!\r\n");
}
