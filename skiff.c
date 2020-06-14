/****************************************************************************/
/* Copyright 2002 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * bootldr file for Compaq Personal Server Bootloader
 *
 */

#include "bootldr.h"
#include "serial.h"
#include "cpu.h"

void *skiff_serbase(void) {   /* Shouldn't use this */
  	return 0;
}

int skiff_char_ready() {
  return (!(CSR_READ_BYTE(UARTFLG_REG) & UART_RX_FIFO_EMPTY));
}

unsigned char skiff_read_char() {
  return CSR_READ_BYTE(UARTDR_REG);
}

unsigned int skiff_read_status() {
  return CSR_READ_BYTE(RXSTAT_REG);
}

void skiff_write_char(struct serial_device *device, unsigned char c) {
  if (!device->enabled) return;
  while (CSR_READ_BYTE(UARTFLG_REG) & UART_TX_FIFO_BUSY)
	; /* do nothing */
  CSR_WRITE_BYTE(UARTDR_REG,c);
}

struct serial_device skiff_serial = {
   skiff_char_ready, skiff_read_char, skiff_read_status, skiff_write_char, 0, 0, skiff_serbase, 0  
};
