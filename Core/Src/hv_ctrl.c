/*
 * curr_ctrl.h
 *
 *  Created on: 1 May 2026
 *      Author: Jiaxin Du
 */

#include <hv_ctrl.h>
#include "tim.h"
//#include "gpdma.h"
#include "defines.h"
#include "dbg_print.h"
#include "dac_ad5689.h"

#define CURR_PROF_SIZE_MAX   (128)
/*
const char HV_CMD_STR[ST_TOT_NUM][16] = {
   "OFF          ",
   "IDLE         ",
   "CURR_RAMPUP  ",
   "CURR_RAMPDOWN",
   "CURR_STEADY  ",
   "VOLT_CTRL    ",
   "VOLT_NO_CURR ",
   "FAULT        "
};
*/
THV_State gHV_State = {
      .state      = ST_OFF,
      .dir        = 0,
      .fault      = FLT_NONE,

      .bias_on    = 0,

      .curr_set   = 0.f,
      .volt_set   = 0.f,

      .volt_tgt   = 0.f,
      .curr_tgt   = 0.f,

      .curr_meas  = 0.f,
      .vcoil_meas = 0.f,
      .vgnd_meas  = 0.f,
      .vdrop_meas = 0.f
};

struct tCurrShape {
   uint32_t data[CURR_PROF_SIZE_MAX];
   uint16_t size;
   uint16_t pos;
} gCurr_Shape = { .size = 0, .pos = 0 };

TCtrl_Config gHV_Config = {
      .use_trig         = 0,

      .vhdrm_rampup     = 1.0f,
      .vhdrm_rampdown   = 0.f,
      .vhdrm_steady     = 0.5f,
      .vout_max         = 120.f,

      .max_curr_inc     = 0.1f,
      .max_curr_dec     = 0.1f,
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

//ADC is configured to 12-bit with 16x oversampling and no bit shift, per Table 265. in RM0481,
// the max raw output is 0xFFF0 = 65520
#define ADC_RAW_TOP        (65520.f)

#define CURR_SENSE_COEFF   (float)(25.f * 2.5f / ADC_RAW_TOP)
#define CURR_SENSE_OFFSET  (float)(25.f * 1.25f)

#define VDROP_COEFF        (float)(2.5f / ADC_RAW_TOP)
#define VDROP_OFFSET       (float)(1.25f)

#define VOLT_SENSE_COEFF   (float)((2.5f / ADC_RAW_TOP) * (10.f / (1000.f + 10.f)))

volatile uint8_t adc_updating = 0;

uint8_t No_Load_Fault_Cnt = 0;

void HV_State_Print(void)
{
   DPrint_CStr("ADC|Iout="); DPrint_Num(gHV_State.curr_meas);
   DPrint_CStr(" A, Vdrop="); DPrint_Num(gHV_State.vdrop_meas);
   DPrint_CStr(" V, VGND="); DPrint_Num(gHV_State.vgnd_meas);
   DPrint_CStr(" V, Coil_Pwr="); DPrint_Num(gHV_State.vcoil_meas);
   DPrint_CStr("V\r\n");

   DPrint_CStr("Mode: "); DPrint(HV_CMD_STR[gHV_State.state], HV_CMD_STR_LEN[gHV_State.state]);
   DPrint_CStr("\r\nTarget: Iout="); DPrint_Num(gHV_State.curr_tgt);
   DPrint_CStr("A, Vout="); DPrint_Num(gHV_State.volt_tgt);
   DPrint_CStr("V\r\n");
   if (gHV_State.state == ST_FAULT) {
      DPrint_CStr("Fault: ");
      if (gHV_State.fault & FLT_OVERTEMP) {
         DPrint_CStr("OVERTEMP ");
      }
      if (gHV_State.fault & FLT_EXT_DISABLE) {
         DPrint_CStr("EXT_DISABLE ");
      }
      if (gHV_State.fault & FLT_LOAD_OPEN) {
         DPrint_CStr("LOAD_OPEN");
      }
      DPrint_CStr("\r\n");
   }
}

void HV_Monitor_Start(void)
{
   HV_Monitor_Stop();

   HAL_ADCEx_Calibration_Start(hadc_curr, ADC_SINGLE_ENDED);
   HAL_ADCEx_Calibration_Start(hadc_volt, ADC_SINGLE_ENDED);

   //start ADC,
   // The ADc will sample the four signals per htim_adc cycle
   // and call HAL_ADC_ConvCpltCallback() afterwards
   HAL_ADC_Start_DMA(hadc_curr, (uint32_t*) adc_curr_buff, 2); //TODO: check ADC fired and two sample per trigger
   HAL_ADC_Start_DMA(hadc_volt, (uint32_t*) adc_volt_buff, 2); //

   //start timer
   HAL_TIM_Base_Start(htim_adc);
}

/** * @brief stop high-voltage bias monitor,
 * stop monitoring high-voltage bias and current output
 */
void HV_Monitor_Stop(void)
{
   HAL_TIM_Base_Stop(htim_adc);

   if ((HAL_ADC_GetState(hadc_curr) & HAL_ADC_STATE_READY) == 0) {
      HAL_ADC_Stop_DMA(hadc_curr);
   }
   if ((HAL_ADC_GetState(hadc_volt) & HAL_ADC_STATE_READY) == 0) {
      HAL_ADC_Stop_DMA(hadc_volt);
   }
}

void HV_init(void)
{
   //reset the latching circuit
   OT_Latching_Reset();

   //set volt and curr all to zero
   Dac_Volt_Ctrl_Set(COIL_VOLT_MIN);
   Dac_Curr_Ctrl_Set(0.);

   gHV_State.state = ST_OFF;
   gHV_State.dir = 0;
   gHV_State.fault = FLT_NONE;

   HAL_GPIO_WritePin(HV_EN_MCU_GPIO_Port, HV_EN_MCU_Pin, GPIO_PIN_RESET);
   gHV_State.bias_on = 0;

   if (HAL_GPIO_ReadPin(OVERTEMP_ANYN_GPIO_Port, OVERTEMP_ANYN_Pin) == GPIO_PIN_RESET) {
      gHV_State.state = ST_FAULT;
      gHV_State.fault |= FLT_OVERTEMP;
   }

   if (HAL_GPIO_ReadPin(HV_EN_CONSOLE_GPIO_Port, HV_EN_CONSOLE_Pin) == GPIO_PIN_RESET) {
      gHV_State.state = ST_FAULT;
      gHV_State.fault |= FLT_EXT_DISABLE;
   }

   gHV_State.curr_tgt = 0.;
   gHV_State.volt_tgt = COIL_VOLT_MIN;

   HV_Monitor_Start();
}

/* @brief HV monitoring ADC conversion complete callback function
 * This function will be called when ADC1 and ADC2 complete conversion on all channels, inluding the oversampling.
 * It will read the ADC values and update the gHV_State accordingly.
 * It also detects no-load condition, if the coil voltage is high but current is low,
 * it will trigger no-load fault after certain counts
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
   // read adc values
   HAL_GPIO_WritePin(CAP_DISCH_GPIO_Port, CAP_DISCH_Pin, GPIO_PIN_SET);

   if (hadc == hadc_curr) {
      adc_updating = 1;
      gHV_State.curr_meas = CURR_SENSE_COEFF * (adc_curr_buff[0]) - CURR_SENSE_OFFSET; //curr_sense
      gHV_State.vdrop_meas = VDROP_COEFF * (adc_curr_buff[1]) - VDROP_OFFSET;
      adc_updating = 0;
   }
   else if (hadc == hadc_volt) {
      adc_updating = 1;
      gHV_State.vgnd_meas = VOLT_SENSE_COEFF * adc_volt_buff[0];
      gHV_State.vcoil_meas = VOLT_SENSE_COEFF * adc_volt_buff[1];
      adc_updating = 0;
   }

   //detecting no load, voltage is enabled but no currents
   //TODO: check the conditions when external alternative power source is used.
   if (gHV_State.vcoil_meas > 24.0 && gHV_State.curr_set > 0.5
      && gHV_State.curr_meas> -0.1 && gHV_State.curr_meas < 0.1) {
      ++ No_Load_Fault_Cnt;
      if (No_Load_Fault_Cnt > 10) {
         gHV_State.state = ST_FAULT;
         gHV_State.fault |= FLT_LOAD_OPEN;

         Dac_Curr_Ctrl_Set(0.);
         Dac_Volt_Ctrl_Set(COIL_VOLT_MIN);

         No_Load_Fault_Cnt = 0;
      }
   }

   HAL_GPIO_WritePin(CAP_DISCH_GPIO_Port, CAP_DISCH_Pin, GPIO_PIN_RESET);
   //Curr_State_Print();
}

TState HV_Cmd(TCtrlCmd x_cmd, float x_curr_tgt, float x_volt_tgt)
{
   if (x_curr_tgt < 0. || x_curr_tgt > 20. || x_volt_tgt < 0. || x_volt_tgt > 150.) return ERR_PARAM_INVALID;

   if (HAL_GPIO_ReadPin(OVERTEMP_ANYN_GPIO_Port, OVERTEMP_ANYN_Pin) == GPIO_PIN_RESET
       || HAL_GPIO_ReadPin(OVERTEMP_SD_GPIO_Port, OVERTEMP_SD_Pin) == GPIO_PIN_SET) {
      gHV_State.state = ST_FAULT;
      gHV_State.fault |= FLT_OVERTEMP;
   }

   if (HAL_GPIO_ReadPin(HV_EN_CONSOLE_GPIO_Port, HV_EN_CONSOLE_Pin) == GPIO_PIN_RESET) {
      gHV_State.state = ST_FAULT;
      gHV_State.fault |= FLT_EXT_DISABLE;
   }

   if (gHV_State.state == ST_FAULT) { return ERR_HW_FAULT;}

   if (x_cmd == CMD_BIAS_ON) {
      HAL_GPIO_WritePin(HV_EN_MCU_GPIO_Port, HV_EN_MCU_Pin, GPIO_PIN_SET);
      gHV_State.bias_on = 1;

      return ST_OK;
   }

   if (x_cmd == CMD_BIAS_OFF) {
      HAL_GPIO_WritePin(HV_EN_MCU_GPIO_Port, HV_EN_MCU_Pin, GPIO_PIN_RESET);

      gHV_State.bias_on = 0;

      return ST_OK;
   }

   //turn current and voltage off, parameter x_curr_tgt and x_volt_tgt not in use
   if (x_cmd == CMD_CURR_OFF) {

      gHV_State.state = ST_OFF;

      gHV_State.volt_tgt = COIL_VOLT_MIN;
      gHV_State.curr_tgt = 0.f;

      Dac_Volt_Ctrl_Set(COIL_VOLT_MIN);
      Dac_Curr_Ctrl_Set(0.);

      return ST_OK;
   }

   // current control mode, x_volt_tgt not in use
   if (x_cmd == CMD_SET_CURR) {
      if (x_curr_tgt >= 0 && x_curr_tgt < 20.) {
         x_curr_tgt = 0.1; //minimum current to overcome the drop on the current pump
      }

      //rampup or rampdown ?
      if (x_curr_tgt < gHV_State.curr_meas) {
         gHV_State.state = ST_CURR_RAMPUP;
      }
      else if (x_curr_tgt > gHV_State.curr_meas) {
         gHV_State.state = ST_CURR_RAMPDOWN;
      }
      else {
         gHV_State.state = ST_CURR_STEADY;
      }

      gHV_State.volt_tgt = COIL_VOLT_MIN;

      gHV_State.curr_tgt = x_curr_tgt;

      if (gHV_State.bias_on == 0) {
         HAL_GPIO_WritePin(HV_EN_MCU_GPIO_Port, HV_EN_MCU_Pin, GPIO_PIN_SET);
         gHV_State.bias_on = 1;
      }

      Curr_Regulate_Start();

      return ST_OK;
   }

   // constant voltage control mode control mode, x_volt_tgt not in use
   if (x_cmd == CMD_SET_VOLT) {
      gHV_State.state = ST_VOLT_CTRL;

      gHV_State.volt_tgt = x_volt_tgt;

      gHV_State.curr_tgt = 0.;

      if (gHV_State.bias_on == 0) {
         HAL_GPIO_WritePin(HV_EN_MCU_GPIO_Port, HV_EN_MCU_Pin, GPIO_PIN_SET);
         gHV_State.bias_on = 1;
      }

      Curr_Regulate_Start();

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

void Curr_Regulate_Start(void)
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

   if (gHV_Config.use_trig) {
      __HAL_TIM_ENABLE_IT(htim_ctrl, TIM_IT_UPDATE | TIM_IT_TRIGGER);
   }
   else {
      __HAL_TIM_ENABLE_IT(htim_ctrl, TIM_IT_UPDATE);
   }

   HAL_TIM_PWM_Start(htim_ctrl, TIM_CHANNEL_3);
};

void Curr_Regulate_Stop()
{
   HAL_TIM_PWM_Stop(htim_ctrl, TIM_CHANNEL_3);
   htim_ctrl->Instance->CNT = 0;

   __HAL_TIM_DISABLE_IT(htim_ctrl, TIM_IT_UPDATE | TIM_IT_TRIGGER);
   __HAL_TIM_CLEAR_FLAG(htim_ctrl, TIM_IT_UPDATE | TIM_IT_TRIGGER);
};

//TODO: measure the time cost of the callback function, and make sure it can finish within 10 us
void Curr_Regulate_Callback()
{
   float tmp_volt, tmp_curr;

   if (__HAL_TIM_GET_FLAG(htim_ctrl, TIM_IT_TRIGGER | TIM_IT_UPDATE) != RESET) {
      __HAL_TIM_CLEAR_IT(htim_ctrl, TIM_IT_TRIGGER | TIM_IT_UPDATE);

      //no current or voltage
      if (gHV_State.state == ST_FAULT || gHV_State.state == ST_IDLE) {
         Curr_Regulate_Stop();
         return;
      }

      // current is turned off
      if (gHV_State.state == ST_OFF && gHV_State.curr_meas > -0.1 && gHV_State.curr_meas < 0.1) {
         gHV_State.state = ST_IDLE;

         return;
      }

      //ramp up currents and maintain voltage headrom
      if (gHV_State.state == ST_CURR_RAMPUP || gHV_State.state == ST_CURR_RAMPDOWN || gHV_State.state == ST_CURR_STEADY) {

         if (gHV_State.state == ST_CURR_RAMPUP) {
            tmp_volt = (gHV_Config.vhdrm_rampup - gHV_State.vdrop_meas); //difference
            tmp_curr = gHV_State.curr_set + gHV_Config.max_curr_inc;
            if (tmp_curr < gHV_State.curr_tgt) {
               tmp_curr = gHV_State.curr_tgt;
               gHV_State.state = ST_CURR_STEADY;
            }
         } else if (gHV_State.state == ST_CURR_RAMPDOWN) {
            tmp_volt = (gHV_Config.vhdrm_rampdown - gHV_State.vdrop_meas);
            tmp_curr = gHV_State.curr_set - gHV_Config.max_curr_dec;
            if (tmp_curr > -gHV_State.curr_tgt) {
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
   }
   //TODO: gHV_State.state == ST_VOLT_CTRL
}
;

/* @brief clear over-temperature state */
TState OT_Latching_Reset(void)
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
         gHV_State.state = ST_IDLE;
      }
      return ST_OK;
   }
};

void Overtemp_Callback(void)
{
   if (__HAL_GPIO_EXTI_GET_IT(OVERTEMP_ANYN_Pin)) {
      __HAL_GPIO_EXTI_CLEAR_IT(OVERTEMP_ANYN_Pin);

      if (HAL_GPIO_ReadPin(OVERTEMP_ANYN_GPIO_Port, OVERTEMP_ANYN_Pin) == GPIO_PIN_RESET) {
         //over-temperature triggerred
         gHV_State.state = ST_FAULT;
         gHV_State.fault |= FLT_OVERTEMP;

         gHV_State.volt_tgt = COIL_VOLT_MIN;
         gHV_State.curr_tgt = 0.;

         Dac_Volt_Ctrl_Set(COIL_VOLT_MIN);
         Dac_Curr_Ctrl_Set(0.);

         HAL_GPIO_WritePin(HV_EN_MCU_GPIO_Port, HV_EN_MCU_Pin, GPIO_PIN_RESET);
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

         gHV_State.volt_tgt = COIL_VOLT_MIN;
         gHV_State.curr_tgt = 0.;

         Dac_Volt_Ctrl_Set(COIL_VOLT_MIN);
         Dac_Curr_Ctrl_Set(0.);

         HAL_GPIO_WritePin(HV_EN_MCU_GPIO_Port, HV_EN_MCU_Pin, GPIO_PIN_RESET);
      }
   }
}

