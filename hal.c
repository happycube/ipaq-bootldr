/****************************************************************************/
/* Copyright 2001 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * hardware abstraction layer (hal)
 *
 */

#include "bootldr.h"
#include "commands.h"
#ifdef CONFIG_MACH_IPAQ
#if defined(__linux__) || defined(__QNXNTO__)
#include <asm-arm/arch-sa1100/h3600.h>
#include <asm-arm/arch-sa1100/h3600_gpio.h>
#include <asm-arm/arch-sa1100/h3600_asic.h>	
#endif
#endif
#ifdef CONFIG_MACH_H3900
#include <asm-arm/arch-pxa/h3900-gpio.h>
#endif
#include "cpu.h"
#include "bsdsum.h"
#include "architecture.h"
#include "hal.h"

#ifdef CONFIG_PXA
#undef __REG
#define __REG(x) *((volatile unsigned long *) (x))
#endif

struct hal_ops *hal_ops;

#ifdef CONFIG_MACH_IPAQ

static int h3600_get_version( struct hal_version * );
static int h3600_eeprom_read( unsigned short address, unsigned char *data, unsigned short len );
static int h3600_eeprom_write( unsigned short address, unsigned char *data, unsigned short len );
static int h3600_get_thermal_sensor( unsigned short * );
static int h3600_set_notify_led( unsigned char mode, unsigned char duration, 
                                 unsigned char ontime, unsigned char offtime );
static int h3600_spi_read( unsigned short address, unsigned char *data, unsigned short len );
static int h3600_get_option_detect( int *result )
{
        int opt_ndet = (GPIO_GPLR_READ() & GPIO_H3600_OPT_DET);

	*result = opt_ndet ? 0 : 1;
	return 0;
}

static int h3800_get_option_detect( int *result )
{
        int opt_ndet = (GPIO_GPLR_READ() & GPIO_H3800_NOPT_IND);

	*result = opt_ndet ? 0 : 1;
	return 0;
}

static struct hal_ops h3600_hal_ops = {
  get_option_detect: h3600_get_option_detect,
};

static struct hal_ops h3100_hal_ops = {
  get_option_detect: h3600_get_option_detect,
};

static struct hal_ops h3800_hal_ops = {
  get_option_detect: h3800_get_option_detect,
};

#endif

#ifdef CONFIG_MACH_H3900

static int h3900_get_option_detect( int *result )
{
        int opt_ndet = (GPLR0 & GPIO_H3900_OPT_IND_N);

	*result = opt_ndet ? 0 : 1;
	return 0;
}

static struct hal_ops h3900_hal_ops = {
  get_option_detect: h3900_get_option_detect,
};
#endif

void hal_init(int mach_type) 
{
  hal_ops = 0;
  switch (mach_type) {
#ifdef CONFIG_MACH_IPAQ
  case MACH_TYPE_H3100:
    hal_ops = &h3100_hal_ops;
    break;
  case MACH_TYPE_H3600:
    hal_ops = &h3600_hal_ops;
    break;
  case MACH_TYPE_H3800:
    hal_ops = &h3800_hal_ops;
    break;
#endif
#ifdef CONFIG_MACH_H3900
  case MACH_TYPE_H3900:
    hal_ops = &h3900_hal_ops;
    break;
#endif
  }
}

void
command_hal(
    int		argc,
    const char*	argv[])
{
  if (strcmp(argv[1], "init") == 0) {
    int mach_type = strtoul(argv[2], 0, NULL);
    hal_init(mach_type);
    putLabeledWord("hal_ops=", hal_ops);
  } else if (strcmp(argv[1], "detect") == 0) {
    int detect = 0;
    hal_get_option_detect(&detect);
    putLabeledWord("opt detect=", detect);
  } else {
     putstr(argv[1]); putstr("\r\n");
     putstr("usage: hal init\r\n");
     putstr("       hal detect\r\n");
  }
}
SUBCOMMAND(hal, init, command_hal, "-- initialize hardware abstraction layer", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(hal, detect, command_hal, "-- detects hardware abstraction layer", BB_RUN_FROM_RAM, 0);
