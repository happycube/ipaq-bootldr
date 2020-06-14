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
 * serial support file for Compaq Bootloadr
 *
 */

#include "bootldr.h"
#include "serial.h"
#include "buttons.h"
#include "cpu.h"
#include <string.h>
#include <errno.h>

#if defined(CONFIG_MACH_SKIFF)
#include "skiff.h"
#endif //defined(CONFIG_MACH_SKIFF)

#define __REG(x) ((unsigned long *) (x))

int packetize_output = 0;
int getc_verbose_errors = 1;
int getc_errno = 0;

extern int get_pushed_char(void);
void serial_putnstr(struct serial_device *device,
                    const char	*str,
                    size_t	n);


#ifdef CONFIG_SA1100
int sa1100_char_ready(struct serial_device *sd) {
  return ((*(volatile long *)(sd->_serbase +  UTSR1)) &  UTSR1_RNE);
}

unsigned char sa1100_read_char(struct serial_device *sd) {
  return *(volatile unsigned char *)(sd->_serbase +  UTDR);
}

unsigned int sa1100_read_status(struct serial_device *sd) {
  return ((*(volatile byte *)(sd->_serbase +  UTSR1)) &  UTSR1_ERROR_MASK);
}

int sa1100_set_baudrate(struct serial_device *sd, int speed) {
  int brd;
  if (speed <= 230400) {
	brd = 23;
	while (speed > 9600) {
	  speed >>= 1;
	  brd++;
	}
	
	*(volatile byte *)(sd->_serbase +  UTCR3) = 0;
	*(volatile byte *)(sd->_serbase +  UTCR1) = brd >> 8;
	*(volatile byte *)(sd->_serbase +  UTCR2) = brd;
	
	/*
	 * Clear status register
	 */
	*(volatile byte *)(sd->_serbase +  UTSR0) =  UTSR0_REB |  UTSR0_RBB |  UTSR0_RID;
	*(volatile byte *)(sd->_serbase +  UTCR3) =  UTCR3_RIE |  UTCR3_RXE |  UTCR3_TXE;
	
   sd->_speed = speed;
  }
  return sd->_speed;
}

void *sa1100_serbase(struct serial_device *sd) {
  	return sd->_serbase;
}

void sa1100_write_char(struct serial_device *sd, unsigned char c) {
  if (!sd->enabled) return;
  while (!((*(volatile long *)(sd->_serbase +  UTSR1)) &  UTSR1_TNF)); /* wait until TX FIFO not full */
  *(byte *) (sd->_serbase +  UTDR) = c;
}

void
sa1100_drain_uart(struct serial_device *device)
{
    while (CTL_REG_READ(device->_serbase + UTSR1) & UTSR1_TBY)
	;
}

#endif // CONFIG_SA1100


#ifdef CONFIG_MACH_SKIFF
int skiff_char_ready(struct serial_device *sd) {
        return (! (CSR_READ_BYTE(UARTFLG_REG) & UART_RX_FIFO_EMPTY));

}

unsigned char skiff_read_char(struct serial_device *sd) {
        return (CSR_READ_BYTE(UARTDR_REG));

}

unsigned int skiff_read_status(struct serial_device *sd) {
        return (CSR_READ_BYTE(RXSTAT_REG));
}

int skiff_set_baudrate(struct serial_device *sd, int speed) {
        volatile unsigned long *csr = (volatile unsigned long *)DC21285_ARMCSR_BASE;
        unsigned long fclk_hz = 48000000;
        unsigned long ubrlcr;
        unsigned long l_ubrlcr = 0;
        unsigned long m_ubrlcr = 0;
        unsigned long h_ubrlcr = 0;

  if (speed <= 57600) {

          ubrlcr = (fclk_hz / 64 / speed)-1;
          h_ubrlcr = csr[H_UBRLCR_REG];

          l_ubrlcr = ubrlcr & 0xFF;
          m_ubrlcr = (ubrlcr >> 8) & 0xFF;
          /* wait for the TX FIFO to empty */
          while ((csr[UARTFLG_REG]&UART_TX_BUSY) != 0);
          /* disable the uart */
          csr[UARTCON_REG] = 0;

          /* set the new values, in the right order */
          csr[L_UBRLCR_REG] = l_ubrlcr;
          csr[M_UBRLCR_REG] = m_ubrlcr;
          csr[H_UBRLCR_REG] = h_ubrlcr;

          /* reenable the uart */
          csr[UARTCON_REG] = 1;
          
          sd->_speed = speed;
  }
  return sd->_speed;
}

void *skiff_serbase(struct serial_device *sd) {
  	return sd->_serbase;
}

void skiff_write_char(struct serial_device *sd, unsigned char c) {
        //PrintHexWord2(0x0030beef);
        if (!sd->enabled) return;
        //PrintHexWord2(0x0031beef);
        while (CSR_READ_BYTE(UARTFLG_REG) & UART_TX_FIFO_BUSY);
        //PrintHexWord2(0x0032beef);
        CSR_WRITE_BYTE(UARTDR_REG,c);
        //PrintHexWord2(0x0033beef);
}

void
skiff_drain_uart(struct serial_device *device)
{
    while (CTL_REG_READ(device->_serbase + UTSR1) & UTSR1_TBY)
	;
}

#endif // CONFIG_MACH_SKIFF



#if defined(CONFIG_MACH_BALLOON) || defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_JORNADA720) || defined(CONFIG_MACH_JORNADA56X) 


/* ipaq serial is disabled by default, enabled if squelch serial is off */
static struct serial_device ipaq_serial = { 
  char_ready: sa1100_char_ready, 
  read_char: sa1100_read_char, 
  read_status: sa1100_read_status,
  write_char: sa1100_write_char, 
  set_baudrate: sa1100_set_baudrate, 
  serbase: sa1100_serbase,
  drain_uart: sa1100_drain_uart,
  enabled: 0,
  _serbase:  UART3BASE
};

struct serial_device *serial = &ipaq_serial;


#elif defined(CONFIG_MACH_JORNADA720) || defined(CONFIG_MACH_JORNADA56X) 
static struct serial_device jornada_serial = { 
  char_ready: sa1100_char_ready, 
  read_char: sa1100_read_char, 
  read_status: sa1100_read_status,
  write_char: sa1100_write_char, 
  set_baudrate: sa1100_set_baudrate, 
  serbase: sa1100_serbase,
  drain_uart: sa1100_drain_uart,
  enabled: 0,
  _serbase:  UART3BASE
};

struct serial_device *serial = &jornada_serial;


#elif defined(CONFIG_MACH_ASSABET)

int assabet_char_ready() {
  return ((machine_has_neponset()  
		   ? (*(volatile long *) UART3_UTSR1) 
		   : (*(volatile long *) UART1_UTSR1))
		  &  UTSR1_RNE);
}

unsigned char assabet_read_char() {
  return (machine_has_neponset()
		  ? (*(volatile byte *) UART3_UTDR) 
		  : (*(volatile byte *) UART1_UTDR));
}

unsigned int assabet_read_status() {
  return ((machine_has_neponset()
		   ? (*(volatile long *) UART3_UTSR1)
		   : (*(volatile long *) UART1_UTSR1))
		  &  UTSR1_ERROR_MASK);
}

void assabet_write_char(struct serial_device *device, unsigned char c) {
  if (!device->enabled) return;
  while (!((machine_has_neponset() ? (*(volatile long *) UART3_UTSR1) :
			(*(volatile long *) UART1_UTSR1)) &  UTSR1_TNF));
  (machine_has_neponset() ? (*(byte *) UART3_UTDR) :
   (*(byte *) UART1_UTDR)) = c;
}

void *assabet_serbase() {
  return machine_has_neponset() ? Ser3Base : Ser1Base;
}
static struct serial_device assabet_serial = {
 assabet_char_ready, assabet_read_char, assabet_read_status, assabet_write_char, 0, 0, sa1100_set_baudrate, assabet_serbase, 1 
};

struct serial_device *serial = &assabet_serial;
#elif defined(CONFIG_PXA)

int pxa_char_ready(struct serial_device *sd) {
  return ((*(volatile long *)(FFLSR)) &  LSR_DR);
}

unsigned char pxa_read_char(struct serial_device *sd) {
  return *(volatile unsigned char *)(FFRBR);
}

unsigned int pxa_read_status(struct serial_device *sd) {
  return ((*(volatile byte *)(FFLSR)) &  FFUART_ERROR_MASK);
}

int pxa_set_baudrate(struct serial_device *sd, int speed) 
{
  putstr("pxa_set_baudrate unimplemented\r\n");
  return sd->_speed;
}

void *pxa_serbase(struct serial_device *sd) {
  	return sd->_serbase;
}

void pxa_write_char(struct serial_device *sd, unsigned char c) {
    if (!sd->enabled) return;
    while (!((*(volatile long *)(FFLSR)) &  LSR_TEMT)); /* wait until TX FIFO not full */
    *(byte *) (FFTHR) = c;
}

void
pxa_drain_uart(struct serial_device *device)
{
    // nothing needed as teh xmit fifo isnt used. only the receive fifo
}

/*  serial is disabled by default, enabled if squelch serial is off */
static struct serial_device pxa_serial = { 
  char_ready: pxa_char_ready, 
  read_char: pxa_read_char, 
  read_status: pxa_read_status,
  write_char: pxa_write_char, 
  set_baudrate: pxa_set_baudrate, 
  serbase: pxa_serbase,
  drain_uart: pxa_drain_uart,
  enabled: 1,
  _serbase:  FFUART
};

struct serial_device *serial = &pxa_serial;



#elif defined(CONFIG_MACH_SKIFF) 
static struct serial_device skiff_serial = { 
  char_ready: skiff_char_ready, 
  read_char: skiff_read_char, 
  read_status: skiff_read_status,
  write_char: skiff_write_char, 
  set_baudrate: skiff_set_baudrate, 
  serbase: skiff_serbase,
  drain_uart: skiff_drain_uart,
  enabled: 1,
  _serbase:  0
};

struct serial_device *serial = &skiff_serial;


#else
#error no architecture defined for SERIAL_CHAR_READY(), et al
#endif




void serial_putc(struct serial_device *device, char c) 
{
        if (!device->enabled) return;
        device->write_char(device, c);
}

byte serial_do_getc(struct serial_device *device,
    vfuncp	    idler,
    unsigned long   timeout,
    int*	    statp)
{
   byte c, rxstat;
   int	do_timeout = timeout != 0;
   int	ch;

   getc_errno = 0; /* reset errno */

   while (!device->char_ready(device)) {
       if ((ch = get_pushed_char()) != -1)
	   return (ch);
   
       if (do_timeout) {
	   if (!timeout)
	       break;
	   timeout--;
       }
       
       if (idler)
	   idler();
   }

   /* give priority to pushed chars */
   if ((ch = get_pushed_char()) != -1)
       return (ch);
   
   if (do_timeout && timeout == 0) {
       c = 0;
       rxstat = -1;
   }
   else {
       c = device->read_char(device);
       rxstat = device->read_status(device);
   }
   
   if (rxstat) {
      getc_errno = rxstat;
#ifdef CONFIG_CONFUSE_USERS
      if (getc_verbose_errors && !statp) {
         putLabeledWord("RXSTAT error: ", rxstat);
      }
#endif
      if (statp)
	  *statp = rxstat;
   }
   return(c);
}

byte
serial_getc(struct serial_device *device)
{
#ifdef CONFIG_MACH_SPOT
    return (serial_do_getc(device, spot_idle, 0, NULL));
#else
    return (serial_do_getc(device, button_check, 0, NULL));
#endif
}

/*
 * Reads and returns a character from the serial port
 *  - Times out after delay iterations checking for presence of character
 *  - Sets *error_p to UART error bits or -1 on timeout
 *  - On timeout, sets *error_p to -1 and returns 0
 */
byte
serial_awaitkey(struct serial_device *device, 
    unsigned long   delay,
    int*	    error_p)
{
    return (serial_do_getc(device, button_check, delay, error_p));
}

byte serial_do_getc_seconds(struct serial_device *device,
                     vfuncp	    idler,
                     unsigned long   timeout_seconds,
                     int*	    statp)
{
  byte c, rxstat;
  int	do_timeout = timeout_seconds != 0;
  int  timed_out = 0;
  unsigned long start_time = CTL_REG_READ(RCNR);
  int	ch;

  getc_errno = 0; /* reset errno */
//XXX the time stuff needs better abstraction
#if defined (CONFIG_MACH_SKIFF)
  // first we need to load it
  CTL_REG_WRITE(SKIFF_RTLOAD, 0xFFFFFF);
  // then we need to enable it
  CTL_REG_WRITE(SKIFF_RTCR, SKIFF_RTC_ENABLE|SKIFF_RTC_FCLK_DIV256);
  start_time = CTL_REG_READ(SKIFF_RCNR);
  
  while (!device->char_ready(device)) {
    if ((ch = get_pushed_char()) != -1)
      return (ch);
   
    if (do_timeout) {
      unsigned long time = CTL_REG_READ(SKIFF_RCNR);
      if ((start_time - time) > (timeout_seconds*SKIFF_COUNTS_PER_SEC)) {
        timed_out = 1;
        break;
      }
    }
    if (idler)
      idler();
  }
#else
  while (!device->char_ready(device)) {
    if ((ch = get_pushed_char()) != -1)
      return (ch);
   
    if (do_timeout) {
      unsigned long time = CTL_REG_READ(RCNR);
      if ((time - start_time) > timeout_seconds) {
        timed_out = 1;
        break;
      }
    }
    if (idler)
      idler();
  }
#endif // defiend(CONFIG_MACH_SKIFF)
  /* give priority to pushed chars */
  if ((ch = get_pushed_char()) != -1)
    return (ch);
   
  if (do_timeout && timed_out) {
    c = 0;
    rxstat = -1;
  }
  else {
    c = device->read_char(device);
    rxstat = device->read_status(device);
  }
   
  if (rxstat) {
    getc_errno = rxstat;
    if (getc_verbose_errors && !statp) {
      putLabeledWord("RXSTAT error: ", rxstat);
    }
    if (statp)
      *statp = rxstat;
  }
  return(c);
}

/*
 * Reads and returns a character from the serial port
 *  - Times out after delay seconds checking for presence of character
 *  - Sets *error_p to UART error bits or -1 on timeout
 *  - On timeout, sets *error_p to -1 and returns 0
 */
byte
serial_awaitkey_seconds(struct serial_device *device,
    unsigned long   delay_seconds,
    int*	    error_p)
{
    return (serial_do_getc_seconds(device, button_check, delay_seconds, error_p));
}

void serial_do_putnstr(struct serial_device *device,
    const char* str,
    size_t	n)
{
   while (n && *str != '\0') {
      serial_putc(device, *str) ;
      str++ ;
      n--;
   }
}

void
serial_putnstr(struct serial_device *device,
    const char	*str,
    size_t	n)
{

        if (str == NULL)
                return;

#if defined(CONFIG_PACKETIZE)
   if (packetize_output) {
       int  len;
       char len_str[16];
       len = strlen(str);
       if (n < len)
	   len = n;

       /* add the msg header */
       dwordtodecimal(len_str, len);
       serial_do_putnstr(device, "MSG/", 4);
       len = strlen(len_str);
       len_str[len] = '/';
       ++len;
       len_str[len] = '\0';
       serial_do_putnstr(device, len_str, len);
   }
#endif   

   serial_do_putnstr(device, str, n);
}

void serial_putstr(struct serial_device *device, const char *str)
{
    serial_putnstr(device, str, strlen(str));
}

void
serial_putstr_sync(struct serial_device *device, 
    const char*   s)
{
    serial_putstr(device, s);
    serial_drain_uart(device);
}

void serial_drain_uart(struct serial_device *device)
{
  if (device->drain_uart != NULL)
    device->drain_uart(device); 
}

int serial_set_baudrate(struct serial_device *device, int speed)
{
  if (device->set_baudrate != NULL)
    return device->set_baudrate(device, speed); 
  return -ENODEV;
}

void drain_uart(void)
{
  serial_drain_uart(serial); 
}

unsigned char getc() { return serial_getc(serial); }
//void putc(char c) { serial_putc(serial, c); }
void putc(char c) {
    if (packetize_output) {
	char	buf[2];
	buf[0] = c;
	buf[1] = '\0';
	serial_putnstr(serial, buf, 1);
    }
    else
	serial_putc(serial, c);
}
void putstr(const char *s) { serial_putstr(serial, s); }
void putnstr(const char *s, int n) { serial_putnstr(serial, s, n); }
byte awaitkey_seconds(unsigned long delay_seconds, int* error_p) {
  return serial_awaitkey_seconds(serial, delay_seconds, error_p);
}
void putstr_sync(const char* s) { serial_putstr_sync(serial, s); }
void do_putnstr(const char* s, int n) { serial_do_putnstr(serial, s, n); }
void putLabeledWord(const char *msg, unsigned long value)
{
   char buf[9];
   binarytohex(buf,value,4);
   putstr(msg);
   putstr(buf);
   putstr("\r\n");
}

void putLabeledAddrWord(const char *msg, unsigned long *value)
{
   char buf[9];

   putstr(msg);
   binarytohex(buf, (unsigned long) value, 4);
   putstr(" *0x" );
   putstr(buf);
   putstr( " == " );
   binarytohex(buf,*value,4);
   putstr(buf);
   putstr("\r\n");
}


void putHexInt32(unsigned long value)
{
  char buf[9];
  binarytohex(buf,value,4);
  putstr(buf);
}

void putHexInt16(word value)
{
  char buf[9];
  binarytohex(buf,value,2);
  putstr(buf);
}

void putHexInt8(byte value)
{
  char buf[3];
  binarytohex(buf,value,1);
  putstr(buf);
}

void putDecInt(unsigned long value)
{
  char buf[16];
  dwordtodecimal(buf, value);
  putstr(buf);
}

void putDecIntWidth(unsigned long value, unsigned int width)
{
  char buf[16];
  unsigned int length;

  dwordtodecimal(buf, value);

  for(length = strlen(buf); length < width; ++length)
    putc(' ');

  putstr(buf);
}

