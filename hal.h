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

#ifndef _HAL_H
#define _HAL_H

#ifndef _ERRNO_H
#include <errno.h>
#endif

#ifdef CONFIG_HAL

struct hal_version {
	unsigned char host_version[8];	/* ascii "x.yy" */
	unsigned char pack_version[8];	/* ascii "x.yy" */
	unsigned char boot_type;		/* TODO ?? */
};

/* Interface to the hardware-type specific functions */
struct hal_ops {
	/* Functions provided by the underlying hardware */
	int (*get_version)( struct hal_version * );
	int (*eeprom_read)( unsigned short address, unsigned char *data, unsigned short len );
	int (*eeprom_write)( unsigned short address, unsigned char *data, unsigned short len );
	int (*get_thermal_sensor)( unsigned short * );
	int (*set_notify_led)( unsigned char mode, unsigned char duration, 
			       unsigned char ontime, unsigned char offtime );
	int (*spi_read)( unsigned short address, unsigned char *data, unsigned short len );
	int (*get_option_detect)( int *result );
};

extern struct hal_ops *hal_ops;

void hal_init(int mach_type);
/* Do not use these macros in driver files - instead, use the inline functions defined below */
#define CALL_HAL_INTERFACE( f, args... ) \
        ( hal_ops && hal_ops->f ? hal_ops->f(args) : -EIO )

#define CALL_HAL(f, args...) \
        { return ( hal_ops && hal_ops->f ? hal_ops->f(args) : -EIO ); }

#define HFUNC  static __inline__ int

/* The eeprom_read/write address + len has a maximum value of 512.  Both must be even numbers */
HFUNC hal_eeprom_read( u16 addr, u8 *data, u16 len )  CALL_HAL(eeprom_read,addr,data,len)
HFUNC hal_spi_read( u8 addr, u8 *data, u16 len)       CALL_HAL(spi_read,addr,data,len)
HFUNC hal_get_version( struct hal_version *v )        CALL_HAL(get_version,v)
HFUNC hal_set_led( u8 mode, u8 dur, u8 ont, u8 offt ) CALL_HAL(set_notify_led, mode, dur, ont, offt)
HFUNC hal_get_option_detect( int *result)             CALL_HAL(get_option_detect,result)


#else /* not CONFIG_HAL */

#define hal_eeprom_read(args...) (-EIO)
#define hal_spi_read(args...) (-EIO)
#define hal_get_version(args...) (-EIO)
#define hal_set_led(args...) (-EIO)
#define hal_get_option_detect(args...) (-EIO)

#endif /* not CONFIG_HAL */

#endif /* _HAL_H_ */
