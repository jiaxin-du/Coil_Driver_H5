/*
 * dac_ad5689.h
 *
 *  Created on: Oct 22, 2025
 *      Author: uqjdu2
 */

#ifndef INC_DAC_AD5689_H_
#define INC_DAC_AD5689_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dbg_print.h"
#include "defines.h"

#define DAC_CH_A                    (0x00)
#define DAC_CH_B                    (0x01)
#define DAC_CH_VOLT                 DAC_CH_A  // DAC channel A
#define DAC_CH_CURR                 DAC_CH_B  // DAC channel B

// set all DAC channels to output 0V
void Dac_Init(void);

/* Set expected coil power voltage
 NB, call this function directly may mess up the states of the internal control.
 Users should use function provided in curr_ctrl to set voltage and current
*/
TState Dac_Volt_Ctrl_Set(double x_volt);

/* Preload expected coil power voltage
 * The voltage will not load until !LOAD! is pulled low by PWM on CH3 TIM2
 */
TState Dac_Volt_Ctrl_Preload(double x_volt);

/* Set expected output current amplitude
 NB, call this function directly may mess up the states of the internal control.
 Users should use function provided in curr_ctrl to set voltage and current
*/
TState Dac_Curr_Ctrl_Set(double x_curr);

/* Preload expected output current amplitude
 * The voltage will not load until !LOAD! is pulled low by PWM on CH3 TIM2
 */
TState Dac_Curr_Ctrl_Preload(double x_curr);

#ifdef __cplusplus
}
#endif

#endif /* INC_DAC_AD5689_H_ */
