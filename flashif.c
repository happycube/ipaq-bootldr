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
 * flashif.c - Low level flash interface routines
 *
 */

#include "btflash.h"
#if !(defined(CONFIG_PXA) || defined(CONFIG_PXA1))
#include "sa1100.h"
#elif (defined(CONFIG_PXA) || defined(CONFIG_PXA1))
#include "asm-arm/arch-pxa/pxa-regs.h"
#define __REG(x) ((unsigned long *) (x))
#endif

#include "params.h"

#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900) || defined(CONFIG_MACH_H5400)
#include <asm-arm/arch-sa1100/h3600_gpio.h>
#include <asm-arm/arch-sa1100/h3600_asic.h>	
#endif

#define FLASH_TIMEOUT 20000000

/* default to physical address, update this variable when MMU gets changed */
volatile unsigned long *flashword = (volatile unsigned long*)FLASH_BASE;
#define bothbanks(w_) ((((w_)&0xFFFF) << 16)|((w_)&0xFFFF))


#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_JORNADA56X)
void set_vppen() {
    if (machine_is_jornada56x()) {
        GPIO_GPDR_WRITE(0x0f5243fc);
        GPIO_GPSR_WRITE(0x00100000);
        GPIO_GPCR_WRITE(0x0f424000);
        GPIO_GPSR_WRITE((1<<26));
    }
#if defined(CONFIG_MACH_IPAQ)
    else {
        set_h3600_egpio(IPAQ_EGPIO_VPP_ON);
    }
#endif
}

void clr_vppen() {
    if (machine_is_jornada56x())
        GPIO_GPCR_WRITE((1<<26));
#if defined(CONFIG_MACH_IPAQ)
    else
        clr_h3600_egpio(IPAQ_EGPIO_VPP_ON);
#endif
}

#elif defined(CONFIG_MACH_JORNADA720)
void set_vppen()
{
    /* PPAR 0x00000000 PPDR 0x000049ff PPSR 0x00000870 */
    (*((volatile int *)PPSR_REG)) |= 0x80;
    (*((volatile int *)PPDR_REG)) |= 0x80;
}

void clr_vppen()
{
    (*((volatile int *)PPSR_REG)) &= ~0x80;
}

#elif defined(CONFIG_MACH_JORNADA56X)
void set_vppen()
{
    GPIO_GPDR_WRITE(0x0f5243fc);
    GPIO_GPSR_WRITE(0x00100000);
    GPIO_GPCR_WRITE(0x0f424000);
    GPIO_GPSR_WRITE((1<<26));
}

void clr_vppen()
{
    GPIO_GPCR_WRITE((1<<26));
}

#elif defined(CONFIG_MACH_H3900)
//vpp for the 3900 is tied to gpio 16
void set_vppen()
{
    unsigned long reg;
    // set the alternate function to straight gpio
    reg = *GAFR0_U;    
    reg &= 0xfffffffc;
    *GAFR0_U = reg;

    // set the data direction register to out
    reg = *GPDR0;    
    reg |= (1<<16);
    *GPDR0 = reg;
    
    // set the level to 1
    *GPSR0 = (1<<16);
    
}

void clr_vppen()
{
    
    // cleart the level 
    *GPCR0 = (1<<16);

}


#else
/* nothing to do here on Skiff */
void set_vppen() {
}
void clr_vppen() {
}
#endif



/*
 * XXX ugly hack.  we'll work this out to be more elegan later...
 */

int  bt_flash_organization = BT_FLASH_ORGANIZATION_2x16;

int
flash_addr_shift(
    void)
{
    switch (bt_flash_organization) {
    case BT_FLASH_ORGANIZATION_2x16:
	return (2);
	break;
	    
    case BT_FLASH_ORGANIZATION_1x16:
	return (1);
	break;
	    
    default:
	putLabeledWord("flash_addr_shift(): "
		       "bad bt_flash_organization=",
		       bt_flash_organization);
	return (0);
	break;
    }

    return (-1);
}
    
unsigned long
flash_make_val(
    unsigned long   inval)
{
    switch (bt_flash_organization) {
    case BT_FLASH_ORGANIZATION_2x16:
	return (bothbanks(inval));
	break;
	    
    case BT_FLASH_ORGANIZATION_1x16:
	return (inval);
	break;
	    
    default:
	putLabeledWord("flash_make_val(): "
		       "bad bt_flash_organization=",
		       bt_flash_organization);
	return (0);
	break;
    }

    return (-1);
}

#ifdef __ARM_ARCH_3__
#error 16 bit flash interface does not work with armv3 instructions
#endif

void
flash_write_cmd(
    unsigned	cmd_addr,
    int		cmd)
{
#if 0
  putLabeledWord("flash_write_cmd: cmd_addr=", cmd_addr);
  putLabeledWord("        cmd=", cmd);
  putLabeledWord("        org=", bt_flash_organization);
  putLabeledWord("        flashword=", flashword);
#endif
    switch (bt_flash_organization) {
    case BT_FLASH_ORGANIZATION_2x16:
	flashword[cmd_addr] = bothbanks(cmd);
	break;

    case BT_FLASH_ORGANIZATION_1x16:
	((short*)flashword)[cmd_addr] = cmd;
	break;

    default:
	putLabeledWord("flash_write_cmd(): bad bt_flash_organization=",
		       bt_flash_organization);
	break;
    }
}

unsigned long
flash_read_val(
    unsigned addr)
{
    unsigned long retval = 0;
    
    switch (bt_flash_organization) {
    case BT_FLASH_ORGANIZATION_2x16:
	retval = flashword[addr];
	break;

    case BT_FLASH_ORGANIZATION_1x16:
	retval = ((unsigned short*)flashword)[addr];
	break;

    default:
	putLabeledWord("flash_read_val(): bad bt_flash_organization=",
		       bt_flash_organization);
	break;
    }

    return (retval);
}

unsigned long
flash_read_array(
    unsigned	addr)
{
    char*   dst;

    /* force read array mode */
    flash_write_cmd(0x55, 0xff);
    
    switch (bt_flash_organization) {
    case BT_FLASH_ORGANIZATION_2x16:
	dst = (char*)flashword + (addr & ~0x3);
	return (*((unsigned long*)dst));

    case BT_FLASH_ORGANIZATION_1x16:
	dst = (char*)flashword + (addr & ~0x1);
#if 0
	putLabeledWord("fra(1x16), addr=0x", (unsigned long)dst);
#endif
	return (*((unsigned short*)dst));
	break;

    default:
	putLabeledWord("flash_read_array(): bad bt_flash_organization=",
		       bt_flash_organization);
	break;
    }
    return 0;
}

void
flash_write_val(
    unsigned	addr,
    int		val)
{
    char*   dst;
    
    switch (bt_flash_organization) {
    case BT_FLASH_ORGANIZATION_2x16:
	dst = (char*)flashword + (addr & ~0x3);
	*((unsigned long*)dst) = bothbanks(val);
	break;

    case BT_FLASH_ORGANIZATION_1x16:
	dst = (char*)flashword + (addr & ~0x1);
	*((unsigned short*)dst) = val;
	break;

    default:
	putLabeledWord("flash_write_val(): bad bt_flash_organization=",
		       bt_flash_organization);
	break;
    }
}

int
is_flash_16bit(void)
{
#if (defined(CONFIG_PXA)||defined(CONFIG_MACH_SKIFF))

    return 0;
#else
    int ret;
    /*
     * look at the memory control register for
     * the boot memory and see how wide it is.
     */
    ret = (CTL_REG_READ(DRAM_CONFIGURATION_BASE+MSC0) &
	   MSC_RBW16);
    if (ret)
	SET_BTF_1x16();
    return ret;
#endif
}

/* remove remaining direct accesses to flashword */
void nflash_write_cmd(unsigned long addr, unsigned long val)
{
    flashword[addr] = bothbanks(val);
}
void nflash_write_val( unsigned addr, int val)
{
    flashword[addr] = val;
}
unsigned long nflash_read_val( unsigned addr)
{
    return flashword[addr];
}
/* short read/write */
void flash_write_val_short( unsigned addr, int val)
{
    unsigned short* flashshort = (unsigned short*)flashword;
    flashshort[addr] = val;
}
unsigned long flash_read_val_short( unsigned addr)
{
    unsigned short* flashshort = (unsigned short*)flashword;
    return flashshort[addr];
}
