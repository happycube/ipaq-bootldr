#if defined(CONFIG_INFRARED)
#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900) || defined(CONFIG_MACH_H5400)

#include "commands.h"
#include "serial.h"
#include "cpu.h"
#include "lcd.h"

/*
 * See p300- of the SA-1100 Developer's Manual, developer.intel.com/design/strong/manuals/278240.htm
 * IrLite specifications
 * http://www.hw-server.com/docs/irda/irda.html
 * TODO:
 * - fill out the code
 * - change serial.c and serial.h to have a cur_serial variable or something like that
 */

struct stats {
  int rx_errors;
  int rx_frame_errors;
  int rx_fifo_errors;
};

static int sa1100_irda_set_speed(struct serial_device *device, int speed) {
  extern int sa1100_set_baudrate(device, speed);
  int new_baudrate = sa1100_set_baudrate(device, speed);
  clr_h3600_egpio(IPAQ_EGPIO_IR_FSEL);
  return new_baudrate;
}

/*
 * This turns the IRDA power on or off on the Compaq H3600
 */
static inline int sa1100_irda_set_power_h3600(unsigned int state)
{
  assign_h3600_egpio(IPAQ_EGPIO_IR_ON, state);
  return 0;
}

static int sa1100_irda_startup(struct serial_device *device)
{
  int ret;
  
	/*
	 * Configure PPC for IRDA - we want to drive TXD2 low.
	 * We also want to drive this pin low during sleep.
	 */
	*(volatile byte *)(PPSR_REG) &= ~PPC_TXD2;
	//	*(volatile word *)(PSDR) = ~PPC_TXD2;
	// *(volatile word *)(PPDR) |= PPC_TXD2;

	/*
	 * Enable HP-SIR modulation, and ensure that the port is disabled.
	 */
	*(volatile byte *)(UART2_UTCR3) = 0;
	*(volatile byte *)(UART2_HSCR0) = HSCR0_UART;
	*(volatile byte *)(UART2_UTCR4) = UTCR4_HPSIR;
	*(volatile byte *)(UART2_UTCR0) = UTCR0_8BIT;
	// *(volatile byte *)(UART2_HSCR2) = HSCR2_TrDataH | HSCR2_RcDataL;

	/*
	 * Clear status register
	 */
	*(volatile byte *)(UART2_UTSR0) = UTSR0_REB | UTSR0_RBB | UTSR0_RID;

	ret = sa1100_irda_set_speed(device, device->speed = 9600);
	return ret;
}

static void sa1100_irda_shutdown(struct serial_device *si)
{
	/*
	 * Stop all DMA activity.
	 */
	//sa1100_dma_stop(si->rxdma);
	//sa1100_dma_stop(si->txdma);

	/* Disable the port. */
	*(volatile byte *)(UART2_UTCR3) = 0;
	*(volatile byte *)(UART2_HSCR0) = 0;
}

struct serial_device dev_ir;

static void sa1100_ir_init() {
  sa1100_irda_startup(&dev_ir);

  /*
   * Initially enable HP-SIR modulation, and ensure that the port
   * is disabled.
   */
  *((volatile byte *) UART2_UTCR3) = 0;
  *((volatile byte *) UART2_UTCR4) = UTCR4_HPSIR;
  *((volatile byte *) UART2_HSCR0) = HSCR0_UART;
}

//extern void *Ser2Base;
/*
 * Attempt to send something
 */
extern void *Ser2Base;
static void sa1100_irda_try(struct serial_device *dev)
{
  char *s = "Hello world!";
  int i;
  for (i = 0; i < strlen(s); i++)
	 PrintChar(s[i], Ser2Base);
}

static void sa1100_ir_uninit(void) 
{
   /* turn off the IR transceiver */
   clr_h3600_egpio(IPAQ_EGPIO_IR_ON); 
}


COMMAND(ir_con, command_ir_con, "-- starts an infrared terminal (secondary)", BB_RUN_FROM_RAM);
void command_ir_con(int argc, const char **argv) {
  byte b;

  putstr("This doesn't really do anything yet, except turn on IR (hopefully!). Setting up IR...\r\n");
  sa1100_ir_init();
  sa1100_irda_try(&dev_ir);
}

#endif
#endif
