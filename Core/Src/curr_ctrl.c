/*
 * curr_ctrl.h
 *
 *  Created on: 1 May 2026
 *      Author: Jiaxin Du
 */

#include "tim.h"
//#include "gpdma.h"
#include "defines.h"
#include "dbg_print.h"
#include "dac_ad5689.h"
#include "curr_ctrl.h"

#define CURR_PROF_SIZE_MAX   (128)

THV_State gHV_State = {
      .state      = ST_OFF,
      .dir        = 0,
      .fault      = FLT_NONE,

      .curr_set   = 0.,
      .volt_set   = 0.,

      .volt_tgt   = 0.,
      .curr_tgt   = 0.,

      .curr_meas  = 0.,
      .vcoil_meas = 0.,
      .vgnd_meas  = 0.,
      .vdrop_meas = 0.
};

struct tCurrShape {
   uint32_t data[CURR_PROF_SIZE_MAX];
   uint16_t size;
   uint16_t pos;
} gCurr_Shape = { .size = 0, .pos = 0 };

TCtrl_Config gHV_Config = {
      .use_trig         = 0,

      .vhdrm_rampup     = 1,
      .vhdrm_rampdown   = 0,
      .vhdrm_steady     = 0.5,
      .vout_max         = 120,

      .max_curr_inc     = 0.1,
      .max_curr_dec     = 0.1,
};

// handles
extern TIM_HandleTypeDef   htim2;
TIM_HandleTypeDef         *htim_ctrl = &htim2;

extern TIM_HandleTypeDef   htim15;
TIM_HandleTypeDef         *htim_adc = &htim15;

extern ADC_HandleTypeDef   hadc1;
ADC_HandleTypeDef         *hadc_curr = &hadc1; //for sensing Curr_Pump_Vdrop and Curr_Sense

extern ADC_HandleTypeDef   hadc2;
ADC_HandleTypeDef         *hadc_volt = &hadc2; //for sensing V_GND and Coil Power Voltage

volatile uint16_t adc_volt_buff[8]; //[0]-> V-GND, [2]->Coil power

volatile uint16_t adc_curr_buff[8]; //[0]-> output current, [1]-> Vdrop on current pump

/** @brief initialise current control state
 * We use TIM2 (and potentially TIM3) to timing voltage regulation and current control operations
 *
 * TIM2_ETR: this is the input trigger pin, in RS485-control mode, this pin will be used to start the TIM2
 * TIM2_CH0: initiate the SPI transfer to DAC to set voltage and current for the next step.
 * TIM2_CH1: Not used now
 * TIM2_CH2: configured to output compare mode. This signal is used to trigger ADC conversion
 *           for Curr_PUMP_VDROP (voltage drop on power mosfets), CURR_SENSE_BUFF(output current sensing)
 * TIM2_CH3: configure to output compare mode and output on pin PB10, which is connected to !DAC_LOAD! signal on
 *           DAC chip AD5689.
 */

#define CURR_SENSE_COEFF   (25.0 * 2.5 / 32760.)
#define CURR_SENSE_OFFSET  (16380)

#define VDROP_COEFF        (2.5 / 32760.0)
#define VDROP_OFFSET       (16380)

#define VOLT_SENSE_COEFF   ((2.5 / 32760.0) * (10. / (1000. + 10.)))

uint8_t adc_updated = 0;

uint8_t No_Load_Fault_Cnt = 0;

void Curr_State_Print(void)
{
   DPrint_CStr("ADC|Iout="); DPrint_Num(gHV_State.curr_meas);
   DPrint_CStr(" A, Vdrop="); DPrint_Num(gHV_State.vdrop_meas);
   DPrint_CStr(" V, VGND="); DPrint_Num(gHV_State.vgnd_meas);
   DPrint_CStr(" V, Coil_Pwr="); DPrint_Num(gHV_State.vcoil_meas);
   DPrint_CStr("V\r\n");
}

/* @brief Initial current and voltage control
 * 1. TIM2: 10 us every cycle,
 *    CC1:
 *    CC2:  1/100 -> Start ADC1 for sensing curr output and amplifier volt drop,
 *                   and ADC2 for sensing V-GND and coil power
 *    CC3: 99/100 -> LDAC signal
 *    CC4:
 * 2. ADC1 and ADC2: auto-calibration first and
 *
 * 3.
 */

void Curr_Ctrl_Reset(void)
{
   if ((HAL_ADC_GetState(hadc_curr) & HAL_ADC_STATE_REG_BUSY) != 0UL) {
      HAL_ADC_Stop_DMA(hadc_curr);
   }
   if ((HAL_ADC_GetState(hadc_volt) & HAL_ADC_STATE_REG_BUSY) != 0UL) {
      HAL_ADC_Stop_DMA(hadc_volt);
   }
   //start ADC,
   // The ADc will sample the four signals per htim_adc cycle
   // and call HAL_ADC_ConvCpltCallback() afterwards
   HAL_ADC_Start_DMA(hadc_curr, (uint32_t*) adc_curr_buff, 2); //TODO: check ADC fired and two sample per trigger
   HAL_ADC_Start_DMA(hadc_volt, (uint32_t*) adc_volt_buff, 2); //

   //start timer
   HAL_TIM_Base_Start(htim_adc);
   //HAL_TIM_PWM_Start(htim_adc, TIM_CHANNEL_1);

   //reset the latching circuit
   OT_Latching_Reset();

   //set volt and curr all to zero
   Dac_Volt_Ctrl_Set(0.);
   Dac_Curr_Ctrl_Set(0.);

   gHV_State.state = ST_OFF;
   gHV_State.dir = 0;
   gHV_State.fault = FLT_NONE;

   if (HAL_GPIO_ReadPin(OVERTEMP_ANYN_GPIO_Port, OVERTEMP_ANYN_Pin) == GPIO_PIN_RESET) {
      gHV_State.fault |= FLT_OVERTEMP;
   }

   if (HAL_GPIO_ReadPin(HV_EN_CONSOLE_GPIO_Port, HV_EN_CONSOLE_Pin) == GPIO_PIN_RESET) {
      gHV_State.fault |= FLT_EXT_DISABLE;
   }

   gHV_State.curr_tgt = 0.;
   gHV_State.volt_tgt = 0.;
}


// Callback function
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
   // read adc values
   HAL_GPIO_WritePin(CAP_DISCH_GPIO_Port, CAP_DISCH_Pin, GPIO_PIN_SET);

   if (hadc == hadc_curr) {
      gHV_State.curr_meas = CURR_SENSE_COEFF * (adc_curr_buff[0] - CURR_SENSE_OFFSET); //curr_sense
      gHV_State.vdrop_meas = VDROP_COEFF * (adc_curr_buff[1] - VDROP_OFFSET);
   }
   else if (hadc == hadc_volt) {
      gHV_State.vgnd_meas = VOLT_SENSE_COEFF * adc_volt_buff[0];
      gHV_State.vcoil_meas = VOLT_SENSE_COEFF * adc_volt_buff[1];
   }

   //detecting no load, voltage is enabled but no currents
   //TODO: check the conditions when external alternative power source is used.
   if (gHV_State.vcoil_meas > 24.0 && gHV_State.curr_set > 0.1
      && gHV_State.curr_meas> -0.1 && gHV_State.curr_meas < 0.1) {
      ++ No_Load_Fault_Cnt;
      if (No_Load_Fault_Cnt > 10) {
         gHV_State.state = ST_FAULT;
         gHV_State.fault |= FLT_LOAD_OPEN;

         Dac_Curr_Ctrl_Set(0.);
         Dac_Volt_Ctrl_Set(0.);

         No_Load_Fault_Cnt = 0;
      }
   }

   HAL_GPIO_WritePin(CAP_DISCH_GPIO_Port, CAP_DISCH_Pin, GPIO_PIN_RESET);
   //Curr_State_Print();
   adc_updated = 1;
}

TState Curr_Ctrl_Set(TCtrlCmd x_cmd, double x_curr_tgt, double x_volt_tgt)
{
   if (x_curr_tgt < 0. || x_curr_tgt > 20. || x_volt_tgt < 0. || x_volt_tgt > 150.) return ERR_PARAM_INVALID;

   if (gHV_State.state == ST_FAULT) { return ERR_HW_FAULT;}

   //turn current and voltage off, parameter x_curr_tgt and x_volt_tgt not in use
   if (x_cmd == CMD_OFF) {
      Dac_Volt_Ctrl_Set(0.);
      Dac_Curr_Ctrl_Set(0.);

      gHV_State.state = ST_OFF;

      gHV_State.volt_tgt = 0.;
      gHV_State.curr_tgt = 0.;

      return ST_OK;
   }

   // current control mode, x_volt_tgt not in use
   if (x_cmd == CMD_SET_CURR) {
      //rampup or rampdown ?
      if (x_curr_tgt < gHV_State.curr_meas) {
         gHV_State.state = ST_CURR_RAMPDOWN;
      }
      else if (x_curr_tgt > gHV_State.curr_meas) {
         gHV_State.state = ST_CURR_RAMPUP;
      }
      else {
         gHV_State.state = ST_CURR_STEADY;
      }

      gHV_State.volt_tgt = 0.; //voltage automatically adjusted

      gHV_State.curr_tgt = x_curr_tgt;

      Curr_Monitor_Start();

      return ST_OK;
   }

   // constant voltage control mode control mode, x_volt_tgt not in use
   if (x_cmd == CMD_SET_VOLT) {
      gHV_State.state = ST_VOLT_CTRL;

      gHV_State.volt_tgt = x_volt_tgt;

      gHV_State.curr_tgt = 0.;

      Curr_Monitor_Start();

      return ST_OK;
    }

    //constant voltage but no current, x_curr_tgt not in use
    if (x_cmd == CMD_SET_VOLT_NO_CURR) {
       gHV_State.state = ST_VOLT_NO_CURR;

       gHV_State.volt_tgt = x_volt_tgt;
       gHV_State.curr_tgt = 0.;

       Dac_Curr_Ctrl_Set(0.);
       Dac_Volt_Ctrl_Set(x_volt_tgt);

       return ST_OK;
    }

    return ERR_CMD_INVALID;
}

void Curr_Monitor_Start()
{
   TIM_SlaveConfigTypeDef sSlaveConfig = {0};

   //TODO: check triggering
   if (gHV_Config.use_trig) {
      sSlaveConfig.SlaveMode = TIM_SLAVEMODE_TRIGGER;
      sSlaveConfig.InputTrigger = TIM_TS_ETRF;
      sSlaveConfig.TriggerPolarity = TIM_TRIGGERPOLARITY_NONINVERTED;
      sSlaveConfig.TriggerPrescaler = TIM_TRIGGERPRESCALER_DIV1;
      sSlaveConfig.TriggerFilter = 0;
   }
   else {
      sSlaveConfig.SlaveMode = TIM_SLAVEMODE_DISABLE;
   }

   HAL_TIM_SlaveConfigSynchro(htim_ctrl, &sSlaveConfig);

   htim_ctrl->Instance->CNT = 0;

   HAL_TIM_PWM_Start(htim_ctrl, TIM_CHANNEL_3);
   __HAL_TIM_ENABLE_IT(htim_ctrl, TIM_IT_UPDATE);
};

void Curr_Monitor_Stop()
{
   HAL_TIM_PWM_Stop(htim_ctrl, TIM_CHANNEL_3);
   htim_ctrl->Instance->CNT = 0;

   __HAL_TIM_DISABLE_IT(htim_ctrl, TIM_IT_UPDATE);
   __HAL_TIM_CLEAR_FLAG(htim_ctrl, TIM_IT_UPDATE);

};

void Curr_Monitor_Callback()
{
   double tmp_volt, tmp_curr;
   __HAL_TIM_CLEAR_IT(htim_ctrl, TIM_IT_UPDATE);

   //no current or voltage
   if (gHV_State.state == ST_FAULT || gHV_State.state == ST_OFF || gHV_State.state == ST_IDLE) {
      Curr_Monitor_Stop(); return;
   }

   //ramp up currents and maintain voltage headrom
   if (gHV_State.state == ST_CURR_RAMPUP
         || gHV_State.state == ST_CURR_RAMPDOWN
         || gHV_State.state == ST_CURR_STEADY) {

      if (gHV_State.state == ST_CURR_RAMPUP) {
         tmp_volt = (gHV_Config.vhdrm_rampup - gHV_State.vdrop_meas); //difference
         tmp_curr = gHV_State.curr_set + gHV_Config.max_curr_inc;
         if (tmp_curr < gHV_State.curr_tgt) {
            tmp_curr = gHV_State.curr_tgt;
            gHV_State.state = ST_CURR_STEADY;
         }
      } else if (gHV_State.state == ST_CURR_RAMPDOWN){
         tmp_volt = (gHV_Config.vhdrm_rampdown - gHV_State.vdrop_meas);
         tmp_curr = gHV_State.curr_set - gHV_Config.max_curr_dec;
         if (tmp_curr >- gHV_State.curr_tgt) {
            tmp_curr = gHV_State.curr_tgt;
            gHV_State.state = ST_CURR_STEADY;
         }
      } else {
         tmp_volt = (gHV_Config.vhdrm_steady - gHV_State.vdrop_meas);
      }

      //to avoid frequent voltage change, we only update when the difference is significant
      if (tmp_volt < -0.1 || tmp_volt > 0.1) {
         tmp_volt += gHV_State.volt_set;
         if (tmp_volt > gHV_Config.vout_max) {
            tmp_volt = gHV_Config.vout_max;
         }
         Dac_Volt_Ctrl_Preload(tmp_volt);
      }

      if (gHV_State.state != ST_CURR_STEADY) {
         Dac_Curr_Ctrl_Preload(tmp_curr);
      }
   }

   //TODO: gHV_State.state == ST_VOLT_CTRL
};

/* @brief clear over-temperature state
   Clear over-temperature latching.
   1. if one or more of the over-temperature signals is low, bias voltage will be turned off.
   2. if the no overtemperature signal is low, it will reset all flip-flop, OT_ANYN

   @return ST_OK if one of over-temperature is still triggered
           true over-temperature lock is released
*/
uint8_t OT_Latching_Reset(void)
{
   // check state
   if (HAL_GPIO_ReadPin(OVERTEMP_ANYN_GPIO_Port, OVERTEMP_ANYN_Pin) == GPIO_PIN_RESET) {
      //HAL_GPIO_WritePin(HV_EN_MCU_GPIO_Port, HV_EN_MCU_Pin, GPIO_PIN_RESET);
      gHV_State.state = ST_FAULT;
      gHV_State.fault |= FLT_OVERTEMP;
      return FLT_OVERTEMP;
   } else {
      //Reset flip flops
      HAL_GPIO_WritePin(OT_LATCH_RSTN_GPIO_Port, OT_LATCH_RSTN_Pin, GPIO_PIN_RESET);
      __asm__ __volatile__("nop");   __asm__ __volatile__("nop");   __asm__ __volatile__("nop");
      HAL_GPIO_WritePin(OT_LATCH_RSTN_GPIO_Port, OT_LATCH_RSTN_Pin, GPIO_PIN_SET);

      //existing from over-temperature
      gHV_State.fault &= (~FLT_OVERTEMP);
      if (gHV_State.fault == FLT_NONE) {
         gHV_State.state = ST_OFF;
      }
      return ST_OK;
   }
};

void Overtemp_Callback(void)
{
   if (__HAL_GPIO_EXTI_GET_IT(OVERTEMP_ANYN_Pin) ) {
      __HAL_GPIO_EXTI_CLEAR_IT(OVERTEMP_ANYN_Pin);

      if (HAL_GPIO_ReadPin(OVERTEMP_ANYN_GPIO_Port, OVERTEMP_ANYN_Pin) == GPIO_PIN_RESET) {
         //over-temperature triggerred
         gHV_State.state = ST_FAULT;
         gHV_State.fault |= FLT_OVERTEMP;

         gHV_State.volt_tgt = 0.;
         gHV_State.curr_tgt = 0.;

         Dac_Volt_Ctrl_Set(0.);
         Dac_Curr_Ctrl_Set(0.);
      }
   }
}

void HV_EN_Console_Callback(void)
{
   if (__HAL_GPIO_EXTI_GET_IT(HV_EN_CONSOLE_Pin) ) {
      __HAL_GPIO_EXTI_CLEAR_IT(HV_EN_CONSOLE_Pin);

      if (HAL_GPIO_ReadPin(HV_EN_CONSOLE_GPIO_Port, HV_EN_CONSOLE_Pin) == GPIO_PIN_SET) {
         gHV_State.fault &= (~FLT_EXT_DISABLE);
         if (gHV_State.fault == FLT_NONE) {
             gHV_State.state = ST_OFF;
         }

      } else {
         gHV_State.state = ST_FAULT;
         gHV_State.fault |= FLT_EXT_DISABLE;

         gHV_State.volt_tgt = 0.;
         gHV_State.curr_tgt = 0.;

         Dac_Volt_Ctrl_Set(0.);
         Dac_Curr_Ctrl_Set(0.);

         gHV_State.volt_set = 0.;
         gHV_State.curr_set = 0.;
      }
   }
}

