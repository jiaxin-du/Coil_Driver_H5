/*
 * usb_cmd.c
 *
 *  Created on: Oct 14, 2025
 *      Author: jiaxindu
 */

#include <string.h>
#include "usb_cmd.h"
#include "dbg_print.h"
#include "curr_ctrl.h"

extern uint32_t gUID[3]; //UID is 96 bits == 12 bytes, defined in main.c
uint8_t tmp_str[128];

// read a integer from string
static uint16_t read_uint(uint32_t *x_val, uint8_t *x_str, uint16_t x_len)
{
   uint32_t tmp_num = 0;

   uint8_t *tmp_ptr = x_str;
   uint8_t *tmp_ptr_end = tmp_ptr + x_len;

   if (tmp_ptr == tmp_ptr_end) { return 0; }

   if (tmp_ptr_end - tmp_ptr > 10) {
      tmp_ptr_end = tmp_ptr + 10;
   }

   while (tmp_ptr < tmp_ptr_end) {
      if (*tmp_ptr >= '0' && *tmp_ptr <= '9') {
         if (tmp_num != 0) { tmp_num *= 10; }

         tmp_num += (uint8_t) (*tmp_ptr - '0');
      } else {
         break;
      }
      ++tmp_ptr;
   }

   *x_val = tmp_num;

   return (uint16_t) (tmp_ptr - x_str);
};

static uint16_t read_float(double *x_val, uint8_t *x_str, uint16_t x_len)
{
   int8_t tmp_sign = 1;
   uint16_t tmp_len, idx;
   uint32_t int_part, frac_part;

   uint8_t *tmp_ptr = x_str;
   uint8_t *tmp_ptr_end = tmp_ptr + x_len;

   if (tmp_ptr == tmp_ptr_end) {
      return 0;
   }

   /* ----- read sign ----- */
   if (*tmp_ptr == '+') {
      ++tmp_ptr;
   } else if (*tmp_ptr == '-') {
      tmp_sign = -1;
      ++tmp_ptr;
   }

   /* ----- read integer part ----- */
   tmp_len = read_uint(&int_part, tmp_ptr, (uint16_t) (tmp_ptr_end - tmp_ptr));
   tmp_ptr += tmp_len;

   /* ----- read fractional part ----- */
   if (tmp_ptr < tmp_ptr_end && *tmp_ptr == '.') {
      ++tmp_ptr;
      tmp_len = read_uint(&frac_part, tmp_ptr, (uint16_t) (tmp_ptr_end - tmp_ptr));
   } else {
      tmp_len = 0;
   }

   /* ----- calculate the value ----- */
   if (tmp_len == 0 || frac_part == 0) {
      *x_val = (double) int_part * (double) tmp_sign;
      return (uint16_t) (tmp_ptr - x_str);
   }

   double frac_base = 0.1;
   for (idx = 1; idx < tmp_len; ++idx) {
      frac_base *= 0.1;
   }

   *x_val = (double) tmp_sign * ((double) int_part + (double) frac_part * frac_base);

   return (uint16_t) (tmp_ptr - x_str);
}
;

void USB_Cmd_Proc(uint8_t *x_data, uint32_t x_len)
{
   uint8_t *tmp_ptr = x_data;
   uint8_t *tmp_ptr_end = x_data + x_len;

   uint8_t cmd_char = 0;
   uint8_t is_val_changed = 0;

   uint8_t idx;
   double tmp_val;

   if (tmp_ptr == NULL)
      return;

   DPrint_CStr("> ");
   DPrint(x_data, x_len);

   /* skip leading and trailing space */
   while ((*tmp_ptr == ' ' || *tmp_ptr == '\t' || *tmp_ptr == '\r' || *tmp_ptr == '\n') && tmp_ptr < tmp_ptr_end) {
      ++tmp_ptr;
   }

   if (tmp_ptr == tmp_ptr_end)
      return;

   while (*(tmp_ptr_end - 1) == ' ' || *(tmp_ptr_end - 1) == '\t' || *(tmp_ptr_end - 1) == '\r' || *(tmp_ptr_end - 1) == '\n') {
      --tmp_ptr_end;
   }

   if (tmp_ptr == tmp_ptr_end)
      return;

   *tmp_ptr_end = 0; // need to make sure this is not out of range

   /* ----- parse command ----- */
   //set voltage
   if ((*tmp_ptr == 'v' || *tmp_ptr == 'V')) {
      cmd_char = 'v';
      tmp_ptr += 1;
   }
   //set currents
   else if ((*tmp_ptr == 'i' || *tmp_ptr == 'I')) {
      cmd_char = 'i';
      tmp_ptr += 1;
   }
   //stop bias voltage
   else if ((*tmp_ptr == 'x' || *tmp_ptr == 'X')) {
      cmd_char = 'x';
      tmp_ptr += 1;
   }
   //enable bias voltage
   else if ((*tmp_ptr == 'o' || *tmp_ptr == 'O')) {
      cmd_char = 'o';
      tmp_ptr += 1;
   } else if (*tmp_ptr == 's' || *tmp_ptr == 'S') {
      cmd_char = 's';
      tmp_ptr += 1;
   } else {
      DPrint_CStr("Unknown action: '");
      DPrint_Hexify(tmp_ptr, tmp_ptr_end - tmp_ptr);
      DPrint_CStr("'.\r\n");
      return;
   }

   //set the value
   if (tmp_ptr < tmp_ptr_end && *tmp_ptr == '=') {
      ++tmp_ptr;
      if (read_float(&tmp_val, tmp_ptr, tmp_ptr_end - tmp_ptr) == 0) {
         DPrint_CStr("Unknown value: '");
         DPrint_Hexify(tmp_ptr, tmp_ptr_end - tmp_ptr);
         DPrint_CStr("'.\r\n");
         return;
      } else { // value is valid
         if (cmd_char == 'v') {  // frequency
            DPrint_CStr("PWR|Setting voltage to ");
            DPrint_Float(tmp_val, 3);
            DPrint_CStr(" V.\r\n");
            Curr_Ctrl_Set(CMD_SET_VOLT_NO_CURR, 0., tmp_val);
            is_val_changed = 1;
         } else if (cmd_char == 'i') { // bandwidth
            DPrint_CStr("PWR|Setting current output to ");
            DPrint_Float(tmp_val, 3);
            DPrint_CStr(" A.\r\n");
            Curr_Ctrl_Set(CMD_SET_CURR, tmp_val, 0.);
            is_val_changed = 1;
         }
      }
   }

   DPrint_CStr("-----\r\n");
}



