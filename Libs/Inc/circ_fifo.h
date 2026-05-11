/* -------------------------------------------------
 FIFO structure using circular storage
 The function only maintains the index, the user are responsible for managing the storage
 Example,

 char tx_buff[128];
 TCircFifo tx_fifo;

 fifo_init(&tx_fifo, 128); //the capacity need to match the size of the storage

 //push data
 tx_buff[fifo_push(&tx_fifo)] = a_src;

 //fetch data
 a_dest = tx_buff[fifo_pop(&tx_fifo)];

 Two special functions named `fifo_push_cstr` and `fifo_fetch_cstr` are provided for byte-typed fifos
 to push and pull bulk data into the FIFO, usage

 fifo_push_cstr(&tx_fifo, tx_buff, str_src, str_src_len);// str_src is the data to be pushed, and str_src_len is its length

 fifo_fecth_cstr(&tx_fifo, tx_buff, str_dest, str_dest_len);// str_dest is the data will be stored and str_dest_len is max number of data to be fetched.
 -------------------------------------------------
 */
#ifndef CIRCULAR_FIFO_H_
#define CIRCULAR_FIFO_H_

#include <stdint.h>

#define FIFO_NPOS  ((uint16_t)0xFFFF)

typedef struct circ_fifo{
   uint16_t   front;
   uint16_t   rear;
   uint16_t   size;
   uint16_t   capacity;
   unsigned char  overflow;
} TCircFifo;

// fifo_init: initialise FIFO
// @param x_cap: the capacity of the FIFO storage
void fifo_init(TCircFifo* x_q, uint16_t x_cap);

// fifo_clear: clear FIFO
void fifo_clear(TCircFifo*);

// add data to the front
// This function always return a valid position,
// a data will be pop-ed from the rear, if the fifo is already full
uint16_t fifo_push(TCircFifo*);

// add char string to FIFO
// @param x_storage: storage of FIFO
// @param x_str: char string to be pushed
// @param x_len: length of the char string
uint16_t fifo_push_cstr(TCircFifo* x_q, uint8_t* x_storage, uint8_t* x_str, uint16_t x_len);

// remove an element from the front
uint16_t fifo_revoke(TCircFifo*);

// remove data to the rear
uint16_t fifo_pop(TCircFifo*);

// return the data at position relative to the rear
uint16_t fifo_peek_rear(TCircFifo* x_q, uint16_t x_pos);

//fetch up to `x_sz` data, return how many data copied
// @param x_storage: storage of FIFO
// @param x_str: memory where the fetched data to be stored
// @param x_sz: maximum data to be fetched
uint16_t fifo_fetch_cstr(TCircFifo* x_q , uint8_t* x_storage, uint8_t* x_str, uint16_t x_sz);

#endif
