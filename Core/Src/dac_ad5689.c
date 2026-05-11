/*
 * dac_ad5689.c
 *
 *  Created on: Oct 22, 2025
 *      Author: uqjdu2
 */
#include <stdlib.h>
#include "tim.h"
#include "spi.h"
#include "dac_ad5689.h"
#include "curr_ctrl.h"

#define DAC_CMD_WRITE               ((uint32_t)0x10U)  // Write to the input register
#define DAC_CMD_WRITE_CHNA          ((uint32_t)0x11U)  // Write to the input register of Channel A
#define DAC_CMD_WRITE_CHNB          ((uint32_t)0x18U)  // Write to the input register of Channel B
#define DAC_CMD_WRITE_ALL           ((uint32_t)0x19U)  // Write to the input register of Channel A and Channel B

#define DAC_CMD_WRITE_UPDATE        ((uint32_t)0x30U)  // Write to and update (power up) DAC
#define DAC_CMD_WRITE_UPDATE_CHNA   ((uint32_t)0x31U)  // Write to and update (power up) DAC channel A
#define DAC_CMD_WRITE_UPDATE_CHNB   ((uint32_t)0x38U)  // Write to and update (power up) DAC channel B
#define DAC_CMD_WRITE_UPDATE_ALL    ((uint32_t)0x39U)  // Write to and update all DAC channels

#define DAC_ADDR_CHNA               (0x01U)  // Channel A
#define DAC_ADDR_CHNB               (0x08U)  // Channel B
#define DAC_ADDR_ALL                (0x09U)  // All channels
#define DAC_ADDR_MASK               (0x09U)  // channel Mask

#define DAC_CMD_RESET               (0x60U)  // RESET command
#define DAC_WAVE_MAX_POINTS         (1000 ) //max points in waveform data array

uint32_t gDAC1_Wave[DAC_WAVE_MAX_POINTS] = { 0x8000 }; // waveform data for 4 channels, max 1024 points per channel
uint16_t gDAC1_NPnt = 0; //current pointer in waveform data array

/* defined in spi.c */
//extern SPI_HandleTypeDef hspi2;
SPI_HandleTypeDef *hpspi_dac = &hspi2;

/* defined in tim.c */
//extern TIM_HandleTypeDef htim2;
TIM_HandleTypeDef *hptim_ctrl = &htim2;

/* defined in curr_ctrl.h */
extern THV_State gHV_State;

// DAC coefficient
double gDAC_Coeff = 13107.2; // ==2^16/(2 * 2.5)

/********************************************************
 * V_out = V_ref x Gain * (DAC / 2^16)
 *
 * V_ref=2.5V, Gain=2(pin set)
 *
 * DAC = (2^16 x V_out)/(V_ref x Gain) = 13107 * V_out
 ********************************************************/

// Set a DAC channel to output a DC value
// @arg x_chn: DAC_CH_VOLT or DAC_CH_CURR
// @arg x_val: 0 to 5V (output voltage in Volts)
void Dac_Volt_Set(uint8_t x_chn, double x_volt)
{
   //Vout = V_ref * 2 * (D / 2^16) - V_ref
   //D = (Vout + V_ref) * 2^16/(2*V_ref)
   uint32_t tmp_dac_reg = ((uint32_t) (0.5 + gDAC_Coeff * x_volt) & 0x0000FFFF);

   tmp_dac_reg = ((uint32_t) (DAC_CMD_WRITE_UPDATE << 16) | tmp_dac_reg);

   if (x_chn == DAC_CH_VOLT) {
      tmp_dac_reg |= (DAC_ADDR_CHNA << 16);

   } else if (x_chn == DAC_CH_CURR) {
      tmp_dac_reg |= (DAC_ADDR_CHNB << 16);

   }

   __HAL_SPI_ENABLE(hpspi_dac);

   SET_BIT(hpspi_dac->Instance->CR1, SPI_CR1_CSTART);

   while (__HAL_SPI_GET_FLAG(hpspi_dac, SPI_FLAG_TXP) == 0);

   *((__IO uint32_t*) &(hpspi_dac->Instance->TXDR)) = tmp_dac_reg;

   while (__HAL_SPI_GET_FLAG(hpspi_dac, SPI_FLAG_TXC) == 0);

   __HAL_SPI_DISABLE(hpspi_dac);

#if DEBUG_LEVEL >= 2
      DPrint_CStr("DAC|SPI2 sent: ");
      DPrint_Hex(tmp_dac_reg);
      DPrint_CStr("\r\n");
#endif
}

// Set a DAC channel to output a DC value
// @arg x_chn: DAC_CH_VOLT or DAC_CH_CURR
// @arg x_val: 0 to 5V (output voltage in Volts)
void Dac_Volt_Preload(uint8_t x_chn, double x_volt)
{
   //Vout = V_ref * 2 * (D / 2^16) - V_ref
   //D = (Vout + V_ref) * 2^16/(2*V_ref)
   uint32_t tmp_dac_reg = ((uint32_t) (0.5 + gDAC_Coeff * x_volt) & 0x0000FFFF);

   tmp_dac_reg = ((uint32_t) (DAC_CMD_WRITE << 16) | tmp_dac_reg);

   if (x_chn == DAC_CH_VOLT) {
      tmp_dac_reg |= (DAC_ADDR_CHNA << 16);

   } else if (x_chn == DAC_CH_CURR) {
      tmp_dac_reg |= (DAC_ADDR_CHNB << 16);

   }

   __HAL_SPI_ENABLE(hpspi_dac);

   SET_BIT(hpspi_dac->Instance->CR1, SPI_CR1_CSTART);

   while (__HAL_SPI_GET_FLAG(hpspi_dac, SPI_FLAG_TXP) == 0);

   *((__IO uint32_t*) &(hpspi_dac->Instance->TXDR)) = tmp_dac_reg;

   while (__HAL_SPI_GET_FLAG(hpspi_dac, SPI_FLAG_TXC) == 0);

   __HAL_SPI_DISABLE(hpspi_dac);

#if DEBUG_LEVEL >= 2
      DPrint_CStr("DAC|SPI2 sent: ");
      DPrint_Hex(tmp_dac_reg);
      DPrint_CStr("\r\n");
#endif
}

/* @brief Set the expected voltage for coil power
 * @param x_volt Coil power voltage in volts, must be higher than 24 and lower than 120 volts
 * @return 0 if succeed, 1 otherwise
 */
TState Dac_Volt_Ctrl_Set(double x_volt)
{
   if (x_volt >= 24 && x_volt <= 120) {
      Dac_Volt_Set(DAC_CH_VOLT, (x_volt - 4.99) * 0.0332975) ; //coil power = 4.99 + 32 * V_ctrl
      gHV_State.volt_set = x_volt;
      return ST_OK;
   }
   else {
      return ERR_PARAM_INVALID;
   }
}

TState Dac_Volt_Ctrl_Preload(double x_volt)
{
   if (x_volt >= 24 && x_volt <= 120) {
      Dac_Volt_Preload(DAC_CH_VOLT, (x_volt - 4.99) * 0.0332975) ; //coil power = 4.99 + 32 * V_ctrl
      gHV_State.volt_set = x_volt;
      return ST_OK;
   }
   else {
      return ERR_PARAM_INVALID;
   }
}

/* @brief Set the expected current output to coil
 * @param x_amps Coil current in amps, must be lower than 20 Amps
 * @return 0 if succeed, 1 otherwise
 */

TState Dac_Curr_Ctrl_Set(double x_curr)
{
   if (x_curr >= 0. && x_curr <= 20.) {
      Dac_Volt_Set(DAC_CH_VOLT, 0.1 * x_curr); // current gain = 10A/V
      gHV_State.curr_set = x_curr;
      return ST_OK;
   }
   else {
      return ERR_PARAM_INVALID;
   }
}


TState Dac_Curr_Ctrl_Preload(double x_curr)
{
   if (x_curr >= 0. && x_curr <= 20.) {
      Dac_Volt_Preload(DAC_CH_VOLT, 0.1 * x_curr); // current gain = 10A/V
      gHV_State.curr_set = x_curr;
      return ST_OK;
   }
   else {
      return ERR_PARAM_INVALID;
   }
}
