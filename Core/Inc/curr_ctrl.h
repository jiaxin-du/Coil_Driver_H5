/*
 * curr_ctrl.h
 *
 *  Created on: 1 May 2026
 *      Author: Jiaxin Du
 */

#ifndef INC_CURR_CTRL_H_
#define INC_CURR_CTRL_H_

#include "defines.h"

typedef struct {
   uint8_t  use_trig;

   double   vhdrm_rampup; // voltage headroom for ramp up
   double   vhdrm_rampdown; // voltage headroom for ramp down
   double   vhdrm_steady; // voltage headroom for steady curr
   double   vout_max;

   double   max_curr_inc;
   double   max_curr_dec;
} TCtrl_Config;

/** @brief State control
 *  We use two state variable for controls
 *  bias : the status of the 120V bias voltage
 *  dir:   H-bridge control status
 *  fault: over-current and over-temperature fault
 *  curr: ramp up, ramp down, or steady
 */
typedef struct  {
   uint8_t  state;  // == 0 current control, or == 1 voltage control
   uint8_t  dir;    // not in use right now
   uint8_t  fault;  // != 0 indicates something wrong, error code to be defined

   double   curr_set;  // current control setting, updated in Dac_Volt_Set
   double   volt_set;  // voltage control setting, updated in Dac_Volt_Set

   double   volt_tgt;  // target voltage
   double   curr_tgt;  // target current

   double   curr_meas;  // measured current output
   double   vcoil_meas;  // measured coil power voltage
   double   vgnd_meas;  // measured v-gnd voltage
   double   vdrop_meas; // measured coil power voltage
} THV_State;

enum tHVState {
   ST_OFF,    // HV bias off
   ST_IDLE,   // HV bias on, but coil power off and no current output
   ST_CURR_RAMPUP, // current ramp up
   ST_CURR_RAMPDOWN, // current ramp down
   ST_CURR_STEADY, // current steady
   ST_VOLT_CTRL,   // constant voltage mode
   ST_VOLT_NO_CURR, // voltage to the set value but no current
   ST_FAULT
};

//faulty state, each occupy one bit
enum tFault {
   FLT_NONE        = 0x0,
   FLT_OVERTEMP    = 0x1,
   FLT_EXT_DISABLE = 0x2,
   FLT_LOAD_OPEN   = 0x4
};

typedef enum {
   CMD_OFF,
   CMD_SET_CURR,
   CMD_SET_VOLT,
   CMD_SET_VOLT_NO_CURR,
   CMD_SET_CURR_DIR,
   CMD_SET_CAP_BANK,
} TCtrlCmd;

/* @brief print out current state */
void Curr_State_Print(void);

/* @brief reset current control state */
void Curr_Ctrl_Reset(void);

/* @brief set current control state
 *  @param x_cmd: command for current control, see TCtrlCmd
 *  @param x_curr_tgt: target current, only used when x_cmd is CMD_SET_CURR
 *  @param x_volt_tgt: target voltage, only used when x_cmd is CMD_SET_VOLT or CMD_SET_VOLT_NO_CURR
 *
 *  @return ST_OK if the command is executed successfully, otherwise return error code
 */
TState Curr_Ctrl_Set(TCtrlCmd x_cmd, double x_curr_tgt, double x_volt_tgt);

/* @brief start current monitor, this will start TIM2 to continously monitor current control loop */
void Curr_Monitor_Start();

/* @brief stop current monitor, this will stop TIM2 */
void Curr_Monitor_Stop();

/* @brief callback function for current monitor,
 * this will be called in TIM2 interrupt handler,
 * it will update current control state and adjust voltage
 * and current output accordingly */
void Curr_Monitor_Callback(void);


uint8_t Bias_Volt_State(void);

uint8_t Bias_Volt_Enable(void);

/* @brief clear over-temperature state
   Clear over-temperature latching.
   1. if one or more of the over-temperature signals is low, bias voltage will be turned off.
   2. if the no overtemperature signal is low, it will reset all flip-flop, OT_ANYN

   @return ST_OK if one of over-temperature is still triggered
           true over-temperature lock is released
 */
uint8_t OT_Latching_Reset(void);

/* @brief disable bias voltage, this will be called when over-temperature is triggered or external disable signal is triggered */
void Bias_Volt_Disable(void);

/* @brief callback function for over-temperature signal, this will be called in EXTI interrupt handler, it will disable bias voltage when over-temperature is triggered */
void Overtemp_Callback(void);

/* @brief callback function for external disable signal, this will be called in EXTI interrupt handler, it will disable bias voltage when external disable signal is triggered */
void HV_EN_Console_Callback(void);

#endif /* INC_CURR_CTRL_H_ */
