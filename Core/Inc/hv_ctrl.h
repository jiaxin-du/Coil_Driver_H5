/*
 * curr_ctrl.h
 *
 *  Created on: 1 May 2026
 *      Author: Jiaxin Du
 */

#ifndef INC_HV_CTRL_H_
#define INC_HV_CTRL_H_

#include "defines.h"

#define COIL_VOLT_MIN      (float)(4.99f)
#define COIL_CURR_MAX      (float)(20.f)

typedef struct {
   uint8_t  use_trig;

   float   vhdrm_rampup; // voltage headroom for ramp up
   float   vhdrm_rampdown; // voltage headroom for ramp down
   float   vhdrm_steady; // voltage headroom for steady curr
   float   vout_max;

   float   max_curr_inc;
   float   max_curr_dec;
} TCtrl_Config;

/** @brief State control
 *  We use two state variable for controls
 *  bias : the status of the 120V bias voltage
 *  dir:   H-bridge control status
 *  fault: over-current and over-temperature fault
 *  curr: ramp up, ramp down, or steady
 */
typedef struct  {
   uint8_t  state;  // current state, one of @tHVState
   uint8_t  dir;    // not in use right now
   uint8_t  fault;  // != 0 indicates something wrong, bit-wise error defined in @TFault

   uint8_t  bias_on; // whether bias voltage is on, this is updated in Bias_Volt_Disable and HV_Cmd

   float   curr_set;  // current control setting, updated in Dac_Curr_Ctrl_Set or Dac_Curr_Ctrl_Preload
   float   volt_set;  // voltage control setting, updated in Dac_Volt_Ctrl_Set or Dac_Volt_Ctrl_Preload

   float   volt_tgt;  // target voltage
   float   curr_tgt;  // target current

   float   curr_meas;  // measured current output, update every 20 us in HAL_ADC_ConvCpltCallback
   float   vcoil_meas; // measured coil power voltage, updated every 20 us in HAL_ADC_ConvCpltCallback
   float   vgnd_meas;  // measured v-gnd voltage, updated every 20 us in HAL_ADC_ConvCpltCallback
   float   vdrop_meas; // measured coil power voltage, updated every 20 us in HAL_ADC_ConvCpltCallback
} THV_State;

enum tHVState {
   ST_OFF,           // current is turing off, coil power off, but there might be some current output due to discharging of the coil
   ST_IDLE,          // coil power off and no current output
   ST_CURR_RAMPUP,   // current ramp up
   ST_CURR_RAMPDOWN, // current ramp down
   ST_CURR_STEADY,   // current steady
   ST_VOLT_CTRL,     // constant voltage mode
   ST_VOLT_NO_CURR,  // voltage to the set value but no current
   ST_FAULT,
   ST_TOT_NUM
};

static const char HV_CMD_STR[ST_TOT_NUM][16] = {
   "OFF          ",
   "IDLE         ",
   "CURR_RAMPUP  ",
   "CURR_RAMPDOWN",
   "CURR_STEADY  ",
   "VOLT_CTRL    ",
   "VOLT_NO_CURR ",
   "FAULT        "
};

static const uint8_t HV_CMD_STR_LEN[] = {
      3, 4, 10, 12, 11, 9, 12, 5
};

//faulty state, each occupy one bit
enum TFault {
   FLT_NONE        = 0x0,
   FLT_OVERTEMP    = 0x1,
   FLT_EXT_DISABLE = 0x2,
   FLT_LOAD_OPEN   = 0x4
};

typedef enum {
   CMD_BIAS_ON,
   CMD_BIAS_OFF,
   CMD_CURR_OFF,
   CMD_SET_CURR,
   CMD_SET_VOLT,
   CMD_SET_VOLT_NO_CURR,
   CMD_SET_CURR_DIR,
   CMD_SET_CAP_BANK,
} TCtrlCmd;

/** @addtogroup HV_Monitor
 *  @brief functions for monitoring high-voltage bias and current output
 *  @{
 */
/** @brief print current high-voltage power rail state,
 * including target current and voltage, measured current and voltage
 */
void HV_State_Print(void);

/** * @brief start high-voltage bias monitor,
 * this will start TIM15, ADC1 and ADC2 in DMA mode to continously monitor high-voltage bias and current output
 */
void HV_Monitor_Start(void);

/** * @brief stop high-voltage bias monitor,
 * stop monitoring high-voltage bias and current output
 */
void HV_Monitor_Stop(void);

/**
 * @}
 */

/* @brief initialize high-voltage bias control,
 * 1. reset the latching circuit by calling OT_Latching_Reset()
 * 2. Initialise gHV_State, set state to ST_OFF, dir to 0, target current and voltage to 0
 * 3. check over-temperature and external disable signal, update fault status accordingly
 * 4. set current and voltage to zero
 * 5. start HV monitoring by calling HV_Monitor_Start()
 */
void HV_init(void);

/* @brief Main HV control entry point*
 * It updates the target current and voltage, and the control mode (current control,
 * voltage control, or voltage no current control).
 *
 * Note that, most of the control logic is implemented in Curr_Monitor_Callback,
 * which will be called in TIM2 interrupt handler every 10 us, and adjust voltage and current output accordingly.
 *
 *  @param x_cmd: command for current control, see TCtrlCmd
 *  @param x_curr_tgt: target current, only used when x_cmd is CMD_SET_CURR
 *  @param x_volt_tgt: target voltage, only used when x_cmd is CMD_SET_VOLT or CMD_SET_VOLT_NO_CURR
 *
 *  @retval ST_OK if the command is executed successfully, otherwise return error code
 */
TState HV_Cmd(TCtrlCmd x_cmd, float x_curr_tgt, float x_volt_tgt);

/* @brief start current monitor, this will start TIM2 to continously monitor current control loop */
void Curr_Regulate_Start(void);

/* @brief stop current monitor, this will stop TIM2 */
void Curr_Regulate_Stop(void);

/* @brief callback function for current regulation,
 * this will be called in TIM2 interrupt handler,
 * it will update current control state and adjust voltage
 * and current output accordingly */
void Curr_Regulate_Callback(void);

/* @brief clear over-temperature state
   Clear over-temperature latching.
   1. if one or more of the over-temperature signals is low, bias voltage will be turned off.
   2. if the no overtemperature signal is low, it will reset all flip-flop, OT_ANYN

   @retval ST_OK over-temperature lock is released
           ERR_OVERTEMP if one of the overtemp alert is still asserted */
TState OT_Latching_Reset(void);

/* @brief callback function for over-temperature signal, this will be called in EXTI interrupt handler, it will disable bias voltage when over-temperature is triggered */
void Overtemp_Callback(void);

/* @brief callback function for external disable signal, this will be called in EXTI interrupt handler, it will disable bias voltage when external disable signal is triggered */
void HV_EN_Console_Callback(void);

#endif /* INC_HV_CTRL_H_ */
