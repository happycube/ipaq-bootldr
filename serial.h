#ifndef __SERIAL_H__
#define __SERIAL_H__

/*
 * Serial device support
 */

struct serial_device {
   /* 
    * returns true if there is a character in the input fifo
    */
   int (*char_ready)(struct serial_device *);
   /* 
    * Reads a character from the input fifo
    */
   unsigned char (*read_char)(struct serial_device *);

   /*
    * Reads the UART status
    */
   unsigned int (*read_status)(struct serial_device *);

   /*
    * Writes a character to the output FIFO
    */
   void (*write_char)(struct serial_device *, unsigned char);

   /*
    * Sets the DCD, etc. modem control lines
    */
   void (*set_modem_control)(struct serial_device *, unsigned int mctrl);
   /*
    * Reads the status of the DCD, etc. modem control lines
    */
   int (*get_modem_control)(struct serial_device *);

   /*
    * Returns the serial base
	*/
   void * (*serbase)(struct serial_device *);
   unsigned short enabled;    /* opposite of squelched */

  /*
   * Sets and returns the uart baudrate
   */
  int (*set_baudrate)(struct serial_device *, int baudrate);

   /*
    * Lets the transmit fifo drain
    */
   void (*drain_uart)(struct serial_device *); 

  char *_serbase;
  int _speed;
  int _stats;
};

extern void serial_putc(struct serial_device *device, char c);
extern byte serial_do_getc(struct serial_device *device, vfuncp idler, unsigned long timeout, int* statp);
extern byte serial_do_getc_seconds(struct serial_device *device, vfuncp	    idler,
							unsigned long   timeout_seconds,
							int*	    statp);
extern void putdebugstr(const char *str);

extern byte serial_getc(struct serial_device *device);
extern byte serial_awaitkey(struct serial_device *device, unsigned long delay, int* error_p);
extern byte serial_awaitkey_seconds(struct serial_device *device, unsigned long delay_seconds, int* error_p);
extern void serial_drain_uart(struct serial_device *device);
int serial_set_baudrate(struct serial_device *device, int speed);

/* Uses the default serial device just so that we don't have to rewrite all of the rest of the code */
extern void putc(char c);
extern byte awaitkey_seconds(unsigned long delay_seconds, int* error_p);
extern void do_putnstr(const char *s, int n);
extern void putnstr(const char *s, int n);
extern void putstr(const char *s);
extern void putstr_sync(const char* s);
extern void do_putnstr(const char* s, int n);

extern unsigned char getc(void);
extern void drain_uart(void);

extern int getc_verbose_errors;
extern int getc_errno;
extern int packetize_output;

extern struct serial_device *serial;
extern void putLabeledWord(const char *msg, unsigned long value);
extern void putLabeledAddrWord(const char *msg, unsigned long *value);
extern void putHexInt32(unsigned long value);
extern void putHexInt16(word value);
extern void putHexInt8(byte value);
extern void putDecInt(unsigned long value);
extern void putDecIntWidth(unsigned long value, unsigned int width);
#define       PLW(x)  putLabeledWord(#x"=0x", (unsigned long)(x))

#endif
