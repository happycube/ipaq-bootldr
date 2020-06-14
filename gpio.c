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
 * gpio.c - General Purpose IO Interface routines
 */

#include "bootldr.h"
#include "params.h"
#include "cpu.h"

#include "asm-arm/arch-sa1100/h3600.h"
#include "asm-arm/arch-sa1100/h3600_gpio.h"
#include "asm-arm/arch-sa1100/h3600_asic.h"

#if defined(CONFIG_MACH_IPAQ)
/* The EGPIO is a write only control register at physical address 0x49000000
 * See the hardware spec for more details.
 */

/************************* H3100 *************************/

#define H3100_EGPIO	*(volatile int *)IPAQ_EGPIO
unsigned int h3100_egpio = EGPIO_H3600_RS232_ON;

void h3100_control_egpio( enum ipaq_egpio_type x, int setp )
{
	unsigned int egpio = 0;
	long         gpio = 0;
	// unsigned long flags;

	switch (x) {
	case IPAQ_EGPIO_LCD_ON:
		egpio |= EGPIO_H3600_LCD_ON;
		gpio  |= GPIO_H3100_LCD_3V_ON;
		// do_blank(setp);
		break;
	case IPAQ_EGPIO_CODEC_NRESET:
		egpio |= EGPIO_H3600_CODEC_NRESET;
		break;
	case IPAQ_EGPIO_AUDIO_ON:
		gpio |= GPIO_H3100_AUD_PWR_ON
			| GPIO_H3100_AUD_ON;
		break;
	case IPAQ_EGPIO_QMUTE:
		gpio |= GPIO_H3100_QMUTE;
		break;
	case IPAQ_EGPIO_OPT_NVRAM_ON:
		egpio |= EGPIO_H3600_OPT_NVRAM_ON;
		break;
	case IPAQ_EGPIO_OPT_ON:
		egpio |= EGPIO_H3600_OPT_ON;
		break;
	case IPAQ_EGPIO_CARD_RESET:
		egpio |= EGPIO_H3600_CARD_RESET;
		break;
	case IPAQ_EGPIO_OPT_RESET:
		egpio |= EGPIO_H3600_OPT_RESET;
		break;
	case IPAQ_EGPIO_IR_ON:
		gpio |= GPIO_H3100_IR_ON;
		break;
	case IPAQ_EGPIO_IR_FSEL:
		gpio |= GPIO_H3100_IR_FSEL;
		break;
	case IPAQ_EGPIO_RS232_ON:
		egpio |= EGPIO_H3600_RS232_ON;
		break;
	case IPAQ_EGPIO_VPP_ON:
		egpio |= EGPIO_H3600_VPP_ON;
		break;
	}
	
	if ( egpio || gpio ) {
	  /*local_irq_save(flags);*/
	  if ( setp ) {
	    h3100_egpio |= egpio;
	    GPSR = gpio;
	  } else {
	    h3100_egpio &= ~egpio;
	    GPCR = gpio;
	  }
	  if (0) {
	    putLabeledWord("setp=", setp);
	    putLabeledWord("gpio=", gpio);
	    putLabeledWord("egpio=", h3100_egpio);
	  }
	  H3100_EGPIO = h3100_egpio;
	  /*local_irq_restore(flags);*/
	}
}

static unsigned long h3100_read_egpio( void )
{
	return h3100_egpio;
}

struct ipaq_model_ops h3100_model_ops = {
	generic_name : "3100",
	control      : h3100_control_egpio,
	read         : h3100_read_egpio,
};

#define H3100_DIRECT_EGPIO (GPIO_H3100_BT_ON      \
                          | GPIO_H3100_GPIO3      \
                          | GPIO_H3100_QMUTE      \
                          | GPIO_H3100_LCD_3V_ON  \
	                  | GPIO_H3100_AUD_ON     \
		          | GPIO_H3100_AUD_PWR_ON \
			  | GPIO_H3100_IR_ON      \
			  | GPIO_H3100_IR_FSEL)

/************************* H3600 *************************/

#define H3600_EGPIO	*(volatile int *)IPAQ_EGPIO
unsigned int h3600_egpio = EGPIO_H3600_RS232_ON;

static void h3600_control_egpio( enum ipaq_egpio_type x, int setp )
{
	unsigned int egpio = 0;
	// unsigned long flags;

	if (machine_is_jornada56x()) return;
	switch (x) {
	case IPAQ_EGPIO_LCD_ON:
		egpio |= EGPIO_H3600_LCD_ON |
			 EGPIO_H3600_LCD_PCI |
			 EGPIO_H3600_LCD_5V_ON |
			 EGPIO_H3600_LVDD_ON;
		// do_blank(setp);
		break;
	case IPAQ_EGPIO_CODEC_NRESET:
		egpio |= EGPIO_H3600_CODEC_NRESET;
		break;
	case IPAQ_EGPIO_AUDIO_ON:
		egpio |= EGPIO_H3600_AUD_AMP_ON |
			EGPIO_H3600_AUD_PWR_ON;
		break;
	case IPAQ_EGPIO_QMUTE:
		egpio |= EGPIO_H3600_QMUTE;
		break;
	case IPAQ_EGPIO_OPT_NVRAM_ON:
		egpio |= EGPIO_H3600_OPT_NVRAM_ON;
		break;
	case IPAQ_EGPIO_OPT_ON:
		egpio |= EGPIO_H3600_OPT_ON;
		break;
	case IPAQ_EGPIO_CARD_RESET:
		egpio |= EGPIO_H3600_CARD_RESET;
		break;
	case IPAQ_EGPIO_OPT_RESET:
		egpio |= EGPIO_H3600_OPT_RESET;
		break;
	case IPAQ_EGPIO_IR_ON:
		egpio |= EGPIO_H3600_IR_ON;
		break;
	case IPAQ_EGPIO_IR_FSEL:
		egpio |= EGPIO_H3600_IR_FSEL;
		break;
	case IPAQ_EGPIO_RS232_ON:
		egpio |= EGPIO_H3600_RS232_ON;
		break;
	case IPAQ_EGPIO_VPP_ON:
		egpio |= EGPIO_H3600_VPP_ON;
		break;
	}

	if ( egpio ) {
	  // local_irq_save(flags);
		if ( setp )
			h3600_egpio |= egpio;
		else
			h3600_egpio &= ~egpio;
		H3600_EGPIO = h3600_egpio;
		// local_irq_restore(flags);
	}
}

static unsigned long h3600_read_egpio( void )
{
	return h3600_egpio;
}

struct ipaq_model_ops h3600_model_ops = {
	generic_name : "3600",
	control      : h3600_control_egpio,
	read         : h3600_read_egpio,
};

/************************* H3800 *************************/


#define SET_ASIC1(x) \
   do {if ( setp ) { H3800_ASIC1_GPIO_OUT |= (x); } else { H3800_ASIC1_GPIO_OUT &= ~(x); }} while(0)

#define SET_ASIC2(x) \
   do {if ( setp ) { H3800_ASIC2_GPIOPIOD |= (x); } else { H3800_ASIC2_GPIOPIOD &= ~(x); }} while(0)

#define CLEAR_ASIC1(x) \
   do {if ( setp ) { H3800_ASIC1_GPIO_OUT &= ~(x); } else { H3800_ASIC1_GPIO_OUT |= (x); }} while(0)

#define CLEAR_ASIC2(x) \
   do {if ( setp ) { H3800_ASIC2_GPIOPIOD &= ~(x); } else { H3800_ASIC2_GPIOPIOD |= (x); }} while(0)


static void h3800_control_egpio( enum ipaq_egpio_type x, int setp )
{
	switch (x) {
	case IPAQ_EGPIO_LCD_ON:
		SET_ASIC1( GPIO1_LCD_5V_ON 
			   | GPIO1_LCD_ON 
			   | GPIO1_LCD_PCI
			   | GPIO1_VGH_ON 
			   | GPIO1_VGL_ON );
		// do_blank(setp);
		break;
	case IPAQ_EGPIO_CODEC_NRESET:
	case IPAQ_EGPIO_AUDIO_ON:
	case IPAQ_EGPIO_QMUTE:
                putstr(__FUNCTION__ ": error - should not be called\n");
		break;
	case IPAQ_EGPIO_OPT_NVRAM_ON:
		SET_ASIC2( GPIO2_OPT_ON_NVRAM );
		break;
	case IPAQ_EGPIO_OPT_ON:
		SET_ASIC2( GPIO2_OPT_ON );
		break;
	case IPAQ_EGPIO_CARD_RESET:
		SET_ASIC2( GPIO2_OPT_PCM_RESET );
		break;
	case IPAQ_EGPIO_OPT_RESET:
		SET_ASIC2( GPIO2_OPT_RESET );
		break;
	case IPAQ_EGPIO_IR_ON:
		CLEAR_ASIC1( GPIO1_IR_ON_N );
		break;
	case IPAQ_EGPIO_IR_FSEL:
		break;
	case IPAQ_EGPIO_RS232_ON:
		SET_ASIC1( GPIO1_RS232_ON );
		break;
	case IPAQ_EGPIO_VPP_ON:
		H3800_ASIC2_FlashWP_VPP_ON = setp;
		break;
	}
}

static unsigned long h3800_read_egpio( void )
{
	return H3800_ASIC1_GPIO_OUT | (H3800_ASIC2_GPIOPIOD << 16);
}

static struct ipaq_model_ops h3800_model_ops = {
	generic_name : "3800",
	control      : h3800_control_egpio,
	read         : h3800_read_egpio,
};
#endif

#if defined(CONFIG_MACH_H3900)

/************************* H3900 *************************/


#define SET_ASIC3(x) \
   do {if ( setp ) { H3900_ASIC3_GPIO_B_OUT |= (x); } else { H3900_ASIC3_GPIO_B_OUT &= ~(x); }} while(0)

#define SET_ASIC2(x) \
   do {if ( setp ) { H3800_ASIC2_GPIOPIOD |= (x); } else { H3800_ASIC2_GPIOPIOD &= ~(x); }} while(0)

#define CLEAR_ASIC3(x) \
   do {if ( setp ) { H3900_ASIC3_GPIO_B_OUT &= ~(x); } else { H3900_ASIC3_GPIO_B_OUT |= (x); }} while(0)

#define CLEAR_ASIC2(x) \
   do {if ( setp ) { H3800_ASIC2_GPIOPIOD &= ~(x); } else { H3800_ASIC2_GPIOPIOD |= (x); }} while(0)


static void h3900_control_egpio( enum ipaq_egpio_type x, int setp )
{
	switch (x) {
	case IPAQ_EGPIO_LCD_ON:
		SET_ASIC3( GPIO3_LCD_5V_ON 
			   | GPIO3_LCD_ON 
			   | GPIO3_LCD_PCI
			   | GPIO3_LCD_9V_ON 
			   | GPIO3_LCD_NV_ON );
		// do_blank(setp);
		break;
	case IPAQ_EGPIO_CODEC_NRESET:
	case IPAQ_EGPIO_AUDIO_ON:
	case IPAQ_EGPIO_QMUTE:
                putstr(__FUNCTION__ ": error - should not be called\n");
		break;
	case IPAQ_EGPIO_OPT_NVRAM_ON:
		SET_ASIC2( GPIO2_OPT_ON_NVRAM );
		break;
	case IPAQ_EGPIO_OPT_ON:
		SET_ASIC2( GPIO2_OPT_ON );
		break;
	case IPAQ_EGPIO_CARD_RESET:
		SET_ASIC2( GPIO2_OPT_PCM_RESET );
		break;
	case IPAQ_EGPIO_OPT_RESET:
		SET_ASIC2( GPIO2_OPT_RESET );
		break;
	case IPAQ_EGPIO_IR_ON:
		CLEAR_ASIC3( GPIO1_IR_ON_N );
		break;
	case IPAQ_EGPIO_IR_FSEL:
		break;
	case IPAQ_EGPIO_RS232_ON:
		SET_ASIC3( GPIO1_RS232_ON );
		break;
	case IPAQ_EGPIO_VPP_ON:
		H3800_ASIC2_FlashWP_VPP_ON = setp;
		break;
	}
}

static unsigned long h3900_read_egpio( void )
{
	return H3900_ASIC3_GPIO_B_OUT | (H3800_ASIC2_GPIOPIOD << 16);
}

static struct ipaq_model_ops h3900_model_ops = {
	generic_name : "3900",
	control      : h3900_control_egpio,
	read         : h3900_read_egpio,
};

#endif /* CONFIG_MACH_IPAQ || CONFIG_MACH_H3900 */

#if defined(CONFIG_MACH_H5400)
static void h5400_control_egpio( enum ipaq_egpio_type x, int setp )
{
	switch (x) {
	case IPAQ_EGPIO_LCD_ON:

		break;
	case IPAQ_EGPIO_CODEC_NRESET:
	case IPAQ_EGPIO_AUDIO_ON:
	case IPAQ_EGPIO_QMUTE:
                putstr(__FUNCTION__ ": error - should not be called\n");
		break;
	case IPAQ_EGPIO_OPT_NVRAM_ON:
	case IPAQ_EGPIO_OPT_ON:
	case IPAQ_EGPIO_CARD_RESET:
	case IPAQ_EGPIO_OPT_RESET:
	case IPAQ_EGPIO_IR_ON:
	case IPAQ_EGPIO_IR_FSEL:
		putstr(__FUNCTION__); putstr(": not implemented yet for this egpio\r\n");
		break;
	case IPAQ_EGPIO_RS232_ON:
		/* nothing to do here */
		break;
	case IPAQ_EGPIO_VPP_ON:
		/* nothing to do here */
		break;
	}
}

static unsigned long h5400_read_egpio( void )
{
	return 0;
}

static struct ipaq_model_ops h5400_model_ops = {
	generic_name : "h5400",
	control      : h5400_control_egpio,
	read         : h5400_read_egpio,
};
#endif /* CONFIG_MACH_H5400 */

#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900)

static void setup_asic2 (void)
{
  // initialise pin direction register
  H3800_ASIC2_GPIODIR = GPIO2_PEN_IRQ | GPIO2_SD_DETECT | GPIO2_EAR_IN_N | GPIO2_USB_DETECT_N | GPIO2_SD_CON_SLT;
}

#endif

struct ipaq_model_ops ipaq_model_ops = {
  generic_name: "h3xxx generic",
};

void gpio_init(int mach_type)
{
  switch (mach_type) {
#ifdef CONFIG_MACH_IPAQ
  case MACH_TYPE_H3100:
    ipaq_model_ops = h3100_model_ops;
    break;
  case MACH_TYPE_H3600:
    ipaq_model_ops = h3600_model_ops;
    break;
  case MACH_TYPE_H3800:
    setup_asic2 ();
    ipaq_model_ops = h3800_model_ops;
    break;
#endif
#ifdef CONFIG_MACH_H3900
  case MACH_TYPE_H3900:
    setup_asic2 ();
    ipaq_model_ops = h3900_model_ops;
    break;
#endif
#ifdef CONFIG_MACH_H5400
  case MACH_TYPE_H5400:
    ipaq_model_ops = h5400_model_ops;
    break;
#endif
  default: 
    putLabeledWord("gpio_init: unhandled mach_type=", mach_type);
  }
  set_h3600_egpio(IPAQ_EGPIO_RS232_ON);
}

