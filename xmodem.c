/****************************************************************************/
/* Copyright 2000 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * xmodem.c - an implementation of the xmodem protocol from the spec.
 *
 * Edwin Foo <efoo@crl.dec.com> January 1999
 */

#include "bootconfig.h"
#include "bootldr.h"
#include "params.h"
#include "serial.h"
#include <string.h>

/*
 * xmodem bootldr parameters
 */
INTERNAL_PARAM( xmodem_initial_timeout, PT_INT, PF_DECIMAL, 15, NULL ); /* seconds */
INTERNAL_PARAM( xmodem_timeout, PT_INT, PF_DECIMAL, 10, NULL ); /* seconds */
    /* send one or repeated NAKs when starting xmodem xfer */
INTERNAL_PARAM( xmodem_one_nak, PT_INT, PF_HEX, 0, NULL );


/* XMODEM parameters */
#define BLOCK_SIZE	128	/* size of transmit blocks */
#define RETRIES		20	/* maximum number of RETRIES */

/* Line control codes */
#define SOH			0x01	/* start of header */
#define ACK			0x06	/* Acknowledge */
#define NAK			0x15	/* Negative acknowledge */
#define CAN			0x18	/* Cancel */
#define EOT			0x04	/* end of text */

#define GET_BYTE_TIMEOUT 10000000

/* global error variable */
char *xmodem_errtxt = NULL;
int get_byte_err = 0;
byte volatile rbuffer[BLOCK_SIZE];

/* prototypes of helper functions */
static int get_record(void);
static byte get_byte(void);

enum
{
    SAC_SEND_NAK = 0,
    SAC_SENT_NAK = 1,
    SAC_PAST_START_NAK = 2
};

static volatile int  seen_a_char = SAC_SEND_NAK;

static int		one_nak = 0;
static unsigned long	xmodem_timeout = GET_BYTE_TIMEOUT;

char	debugbuf[4096];
int	db_idx = 0;

void
static bufputs(
    char*   s)
{
    size_t len = strlen(s) + 1;

    if (len + db_idx > sizeof(debugbuf))
	len = sizeof(debugbuf) - db_idx;

    if (len) {
	memcpy(&debugbuf[db_idx], s, len);
	db_idx += len;
    }
}

void
static reset_debugbuf(
    void)
{
    memset(debugbuf, 0x2a, sizeof(debugbuf));
    db_idx = 0;
}

    
#if defined(CONFIG_PACKETIZE)
extern int packetize_output;
#endif

dword modem_receive(char *dldaddr, size_t len)
{
  char ochr;
  int r = 0, rx_block_num = 0, error_count = 0;
  dword foffset = 0;
  int i;
  int	old_packetize_output;
  

  putstr("ready for xmodem download..\r\n");
  xmodem_errtxt = NULL;
  seen_a_char = 0;
#if defined(CONFIG_PACKETIZE)
  reset_debugbuf();
  bufputs("starting");
  old_packetize_output = packetize_output;
  packetize_output = 0;
#endif
  
  one_nak = param_xmodem_one_nak.value;

  xmodem_timeout = param_xmodem_initial_timeout.value;
  
  rx_block_num = 1;
  error_count = RETRIES;

  do {
    if((r = get_record()) == (rx_block_num & 255)) {
      error_count = RETRIES;
      for (i=0;i<BLOCK_SIZE;i++)
	*(byte *)(dldaddr+foffset+i) = rbuffer[i];
      xmodem_errtxt = "RX PACKET";
      rx_block_num++;
      ochr = ACK;
      foffset += BLOCK_SIZE;
    } else {
      switch(r) {
      case -1: /*TIMEOUT */
	xmodem_errtxt = "TIMEOUT";
	ochr = NAK;
	break;
      case -2 :		/* Bad block */
	xmodem_errtxt = "BAD BLOCK#";
        /* eat the rest of the block */
        get_byte_err = 0;
        while (get_byte_err != -1) get_byte();
	ochr = NAK;
	break;
      case -3 :		/* Bad checksum */
	xmodem_errtxt = "BAD CHKSUM";
	ochr = NAK;
	break;
      case -4 :		/* End of file */
	xmodem_errtxt = "DONE";
	ochr = ACK;
	break;
      case -5 :		/* Cancel */
	xmodem_errtxt = "ABORTED";
	ochr = ACK;
	break;
      default:		/* Block out of sequence */
	xmodem_errtxt = "WRONG BLK";
	ochr = NAK;
      }
#ifdef XMODEM_DEBUG
      putstr(xmodem_errtxt);
      putstr("\r\n");
#endif
      error_count--;
    }
    putc(ochr);
  } while((r > -3) && error_count);

  if ((!error_count) || (r != -4)) {
    putstr(xmodem_errtxt);
    putstr("\r\n");
    putstr("Download Failed!\r\n");
    foffset = 0;		/* indicate failure to caller */
  } else {
    putstr("Download Successful\r\n");
  }

#if defined(CONFIG_PACKETIZE)
  packetize_output = old_packetize_output;
#endif  
  
  return foffset;
}

/*
 * Read a record in the XMODEM protocol, return the block number
 * (0-255) if successful, or one of the following return codes:
 *	-1 = Bad byte
 *	-2 = Bad block number
 *	-3 = Bad block checksum
 *	-4 = End of file
 *	-5 = Canceled by remote
 */
static int get_record(void)
{
  int c, block_num;
  int i;
  dword check_sum;

  /* clear the buffer */
  for (i=0;i<BLOCK_SIZE;i++)
    rbuffer[i] = 0x00;

  check_sum = 0;
  i = -2;
  c = get_byte();
  if (get_byte_err)
    return -1;
  switch(c) {
  case SOH :		/* Receive packet */
    for(;;) {
      c = get_byte();
      if (get_byte_err)
	return -1;

      switch (i) {
      case -2: block_num = c; break;
      case -1: 
#ifdef CHECK_NEGATED_SECTNUM
         if (c != (-block_num - 1)) return -2; 
#endif
         break;
      case BLOCK_SIZE:
	if ((check_sum & 0xff) != c)
	  return -3;
	else
	  return block_num;
	break;
      default:
	rbuffer[i] = c;
	check_sum += c;
      }
      i++;
    }
  case EOT :		/* end of file encountered */
    return -4;
  case CAN :		/* cancel protocol */
    return -5;
  default :
    return -5;
  }
}


/* get_byte should use one of the timer's for a CPU clock independent timeout */
static byte get_byte()
{
   int c;

  again:
   c = 0;
   get_byte_err = 0; /* reset errno */
   c = awaitkey_seconds(xmodem_timeout, &get_byte_err);
   
   if (get_byte_err) {
       if (seen_a_char == SAC_SEND_NAK || !one_nak) {
	   bufputs("timeout nak");
	   putc(NAK);		/* make the sender go */
       }

       if (seen_a_char < SAC_PAST_START_NAK) {
	   bufputs("goto again");
	   seen_a_char = SAC_SENT_NAK;

           xmodem_timeout = param_xmodem_timeout.value;
	   goto again;
       }
   }
   
   if (get_byte_err == 0)
       seen_a_char = SAC_PAST_START_NAK;
   
   return(c);
}
