/*
 * dbg_print.c
 *
 *  Created on: Oct 7, 2025
 *      Author: uqjdu2
 */

#include <ctype.h>
#include "main.h"
#include "usbd_cdc_if.h"
#include "dbg_print.h"
#include "circ_fifo.h"

uint8_t Hexify(uint8_t x_val)
{
   return (((x_val) < 10) ? ((uint8_t)('0' + (x_val))) : \
         ((uint8_t)('7' + (x_val)))); // '7' + 10 == 'A' (65)
};

void DPrint_Hexify(uint8_t *x_str, uint16_t x_len)
{
   uint8_t tmp_buff[32];
   uint16_t pos = 0;
   uint16_t idx = 0;

   if (gUSB_Connected == 0) return;

   while (idx < x_len) {
      if (idx > 0) {
         tmp_buff[pos++] = ' ';
      }

      tmp_buff[pos++] = Hexify(x_str[idx] >> 4);
      tmp_buff[pos++] = Hexify(x_str[idx] & 0x0F);
      ++idx;

      if (pos >= 30 || idx == x_len) {
         DPrint(tmp_buff, pos);
         pos = 0;
      }
   }
};

void DPrint_Hex_UInt8(uint8_t x_val)
{
   if (gUSB_Connected == 0) return;

   CDC_Send((uint8_t*)"0x", 2);
   CDC_Putc(Hexify(x_val >> 4));
   CDC_Putc(Hexify(x_val & 0x0F));
};

void DPrint_UInt8(uint8_t x_val)
{
   uint8_t tmp_base = 100;
   uint8_t leading_zero = 1;
   uint8_t tmp_ch;

   if (gUSB_Connected == 0) return;

   do {
      tmp_ch = x_val / tmp_base;
      x_val = (x_val % tmp_base);

      if (leading_zero == 0 || tmp_ch != 0 || tmp_base == 1) {
         CDC_Putc('0' + tmp_ch);
         leading_zero = 0;
      }

      tmp_base /= 10;

   } while (tmp_base != 0);
};

void DPrint_Int8 (int8_t x_val)
{
   if (gUSB_Connected == 0) {
      return;
   }
   if (x_val < 0) {
      DPrint_Putc('-');
      x_val = -x_val;
   }
   DPrint_UInt8(x_val);
};

void DPrint_Hex_UInt16(uint16_t x_val)
{
   if (gUSB_Connected == 0) {
      return;
   }

   CDC_Send((uint8_t *)"0x", 2);
   CDC_Putc(Hexify((x_val >> 12) & 0x0F));
   CDC_Putc(Hexify((x_val >> 8) & 0x0F));
   CDC_Putc(Hexify((x_val >> 4) & 0x0F));
   CDC_Putc(Hexify(x_val & 0x0F));
};

void DPrint_UInt16 (uint16_t x_val)
{
   uint16_t tmp_base = 10000;
   uint8_t leading_zero = 1;
   uint8_t tmp_ch;

   if (gUSB_Connected == 0) return;

   do {
      tmp_ch = x_val / tmp_base;
      x_val = (x_val % tmp_base);

      if (leading_zero == 0 || tmp_ch != 0 || tmp_base == 1) {
         CDC_Putc('0' + tmp_ch);
         leading_zero = 0;
      }

      tmp_base /= 10;

   } while (tmp_base != 0);
};

void DPrint_Int16 (int16_t x_val)
{
   if (gUSB_Connected == 0) return;

   if (x_val < 0) {
      CDC_Putc('-');
      x_val = -x_val;
   }
   DPrint_UInt16(x_val);
};

void DPrint_Hex_UInt32(uint32_t x_val)
{
   if (gUSB_Connected == 0) return;

   CDC_Send((uint8_t*)"0x", 2);
   CDC_Putc(Hexify((x_val >> 28) & 0x0F));
   CDC_Putc(Hexify((x_val >> 24) & 0x0F));
   CDC_Putc(Hexify((x_val >> 20) & 0x0F));
   CDC_Putc(Hexify((x_val >> 16) & 0x0F));
   CDC_Putc(Hexify((x_val >> 12) & 0x0F));
   CDC_Putc(Hexify((x_val >> 8) & 0x0F));
   CDC_Putc(Hexify((x_val >> 4) & 0x0F));
   CDC_Putc(Hexify(x_val & 0x0F));
};

void DPrint_UInt32(uint32_t x_val)
{
   uint32_t tmp_base = 1000000000;
   uint8_t leading_zero = 1;
   uint8_t tmp_ch;

   if (gUSB_Connected == 0) return;

   do {
      tmp_ch = x_val / tmp_base;
      x_val = (x_val % tmp_base);

      if (leading_zero == 0 || tmp_ch != 0 || tmp_base == 1) {
         CDC_Putc('0' + tmp_ch);
         leading_zero = 0;
      }

      tmp_base /= 10;

   } while (tmp_base != 0);
};

void DPrint_Int32 (int32_t x_val)
{
   if (gUSB_Connected == 0) return;

   if (x_val < 0) {
      CDC_Putc('-');
      x_val = -x_val;
   }
   DPrint_UInt32(x_val);
};

void DPrint_Float(double x_val, uint8_t x_decimals)
{
   uint32_t int_part;
   double  frac_part;

   uint8_t idx;

   if (gUSB_Connected == 0) return;

   if (x_val < 0.0) {
      CDC_Putc('-');
      x_val = -x_val;
   }

   int_part = (uint32_t)x_val;
   DPrint_UInt32(int_part);

   if (x_decimals > 0) {
      if (x_decimals > 10) {x_decimals = 5; } //limit to max 10 decimals
      CDC_Putc('.');

      frac_part = x_val - (double)int_part;

      for (idx = 0; idx < x_decimals; ++idx) {
         frac_part *= 10.0;
         CDC_Putc('0' + (uint8_t)frac_part);
         frac_part -= (uint32_t)(frac_part);
      }
   }
};
