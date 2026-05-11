/*
 * circ_fifo.c
 *
 *  Created on: Oct 2, 2025
 *      Author: Jiaxin Du
 */

#include <string.h>
#include "circ_fifo.h"

void fifo_init(TCircFifo* x_q, uint16_t x_cap)
{
   x_q->capacity = x_cap;
   fifo_clear(x_q);
};

void fifo_clear(TCircFifo* x_q)
{
   x_q->front = x_q->capacity - 1; // ensure the next push will add data to the beginning
   x_q->rear = 0;
   x_q->size = 0;
   x_q->overflow = 0;
};

// add data to the front
uint16_t fifo_push(TCircFifo* x_q)
{
  ++x_q->front;

  if (x_q->front == x_q->capacity) {
     x_q->front = 0;
  }

  //if the buffer is full
  //delete one data from the rear
  if (x_q->size == x_q->capacity) {
    ++x_q->rear;
    if (x_q->rear == x_q->capacity) {
      x_q->rear = 0;
    }
    x_q->overflow = 1;
  } else {
    ++x_q->size;
  }

  return x_q->front;
};

uint16_t fifo_push_cstr(TCircFifo* x_q, uint8_t* x_storage, uint8_t* x_str, uint16_t x_len)
{
   uint16_t end_idx;
   uint16_t tmp_cnt;

   //if more than the FIFO capacity, just push the last segment
   while ( x_len > x_q->capacity ) { //this should not happen
      x_len -= x_q->capacity;
      x_str += x_q->capacity;
      x_q->overflow = 1;
   };

   //if (x_len == 0) return 0; //rare case, will not cause any trouble below

   ++x_q->front; //next available position

   end_idx = x_q->front + x_len;

   if ( end_idx <= x_q->capacity ) {
      memcpy(x_storage + x_q->front, x_str, x_len);

      x_q->front = x_q->front + x_len - 1;
   }
   else {
      tmp_cnt = x_q->capacity - x_q->front;
      memcpy(x_storage + x_q->front, x_str, tmp_cnt);

      x_str += tmp_cnt;
      tmp_cnt = x_len - tmp_cnt;

      memcpy(x_storage, x_str, tmp_cnt);

      //reposition front
      x_q->front = (tmp_cnt - 1);
   }

   // deal with fLen
   x_q->size += x_len;
   if (x_q->size > x_q->capacity) { // FIFO overflow
      x_q->size = x_q->capacity;
      //reposition rear
      x_q->rear = x_q->front + 1;
      if (x_q->rear == x_q->capacity) {
         x_q->rear = 0;
      }
      x_q->overflow = 1;
   }

   return x_len;
};


// remove an element from the front
uint16_t fifo_revoke( TCircFifo* x_q )
{
   uint16_t tmp_idx = FIFO_NPOS;

   if( x_q->size > 0 ) {
      -- x_q->size;

      tmp_idx = x_q->front;

      if (x_q->front == 0) {
         x_q->front = x_q->capacity;
      }
      --x_q->front;
   }

   return tmp_idx;
};


// remove data to the rear
uint16_t fifo_pop(TCircFifo* x_q )
{
   uint16_t tmp_idx = FIFO_NPOS;

   if (x_q->size > 0) {
      -- x_q->size;

      tmp_idx = x_q->rear;

      ++ x_q->rear;
      if (x_q->rear == x_q->capacity) {
         x_q->rear = 0;
      }
   }

   return tmp_idx;
};

// return the data at position relative to the rear
uint16_t fifo_peek_rear(TCircFifo* x_q, uint16_t x_pos)
{
  uint16_t tmp_idx = FIFO_NPOS;

  if (x_pos < x_q->size) {
    tmp_idx = x_q->rear + x_pos;

    if (tmp_idx >= x_q->capacity) {
      tmp_idx -= x_q->capacity;
    }
  }

  return tmp_idx;
};

//fetch up to `x_sz` data, return how many data copied
uint16_t fifo_fetch_cstr(TCircFifo* x_q , uint8_t* x_storage, uint8_t* x_str, uint16_t x_sz)
{
   uint16_t end_idx, tmp_cnt;
   if (x_sz > x_q->size) {
      x_sz = x_q->size;
   }

   if (x_sz == 0) return 0; //nothing to do

   end_idx = x_q->rear + x_sz;
   if (end_idx <= x_q->capacity) {

      // |fpData------fpRear>>>>>>>>>>>>>>>fpFront-------fpEnd|
      memcpy(x_str, x_storage + x_q->rear, x_sz); // memcpy always count by bytes

      x_q->rear += x_sz;
      if (x_q->rear == x_q->capacity) {
         x_q->rear = 0;
      }
   }
   else {
      // |fpData>>>>>>>>>>fpFront-------fpRear>>>>>>>>>>>>>fpEnd|
      tmp_cnt = x_q->capacity - x_q->rear;
      memcpy (x_str, x_storage + x_q->rear, tmp_cnt);

      x_str += tmp_cnt;
      tmp_cnt = x_sz - tmp_cnt;

      memcpy(x_str, x_storage, tmp_cnt );

      x_q->rear = tmp_cnt;
   }

   x_q->size -= x_sz;

   return x_sz;
};


