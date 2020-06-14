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

/*  ********************************************************************
 *  Bitfield defines for some of the pxa250 registers
 *  
 *       
 *  Copyright 1999 Compaq Computer Corporation
 *  All Rights Reserved
 * 
 *  ********************************************************************
 */
#ifndef PXA_H_INCLUDED
#define PXA_H_INCLUDED

#ifndef CONFIG_PXA
#error Only include this file on PXA
#endif

#include "asm-arm/arch-pxa/pxa-regs.h"

// bit definitions for the FFUART
#define FFUART_ERROR_MASK 0x0e


#define ABS_MDCNFG (*(volatile long *)0x48000000)

#define MDCNFG_BANK0_ENABLE (1 << MDCNFG_DE0_BITPOS)
#define MDCNFG_BANK1_ENABLE (1 << MDCNFG_DE1_BITPOS)

// bit definitions for the MDCNFG Register
#define MDCNFG_DE0_BITPOS	0
#define MDCNFG_DE1_BITPOS	1
#define MDCNFG_DWID0_BITPOS	2
#define MDCNFG_DCAC0_BITPOS	3
#define MDCNFG_DRAC0_BITPOS	5
#define MDCNFG_DNB0_BITPOS	7
#define MDCNFG_DTC0_BITPOS	8
#define MDCNFG_DADDR0_BITPOS	10
#define MDCNFG_DLATCH0_BITPOS	11
#define MDCNFG_DSA1111_0_BITPOS	12
#define MDCNFG_DE2_BITPOS	16
#define MDCNFG_DE3_BITPOS	17
#define MDCNFG_DWID2_BITPOS	18
#define MDCNFG_DCAC2_BITPOS	19
#define MDCNFG_DRAC2_BITPOS	21
#define MDCNFG_DNB2_BITPOS	23
#define MDCNFG_DTC2_BITPOS	24
#define MDCNFG_DADDR2_BITPOS	26
#define MDCNFG_DLATCH2_BITPOS	27
#define MDCNFG_DSA1111_2_BITPOS	28

#define MDCNFG_DE0	(1 << 0)
#define MDCNFG_DE1	(1 << 1)
#define MDCNFG_DWID0	(1 << 2)
#define MDCNFG_DCAC0	(1 << 3)
#define MDCNFG_DRAC0	(1 << 5)
#define MDCNFG_DNB0	(1 << 7)
#define MDCNFG_DTC0	(1 << 8)
#define MDCNFG_DADDR0	(1 << 10)
#define MDCNFG_DLATCH0	(1 << 11)
#define MDCNFG_DSA1111_0	(1 << 12)
#define MDCNFG_DE2	(1 << 16)
#define MDCNFG_DE3	(1 << 17)
#define MDCNFG_DWID2	(1 << 18)
#define MDCNFG_DCAC2	(1 << 19)
#define MDCNFG_DRAC2	(1 << 21)
#define MDCNFG_DNB2	(1 << 23)
#define MDCNFG_DTC2	(1 << 24)
#define MDCNFG_DADDR2	(1 << 26)
#define MDCNFG_DLATCH2	(1 << 27)
#define MDCNFG_DSA1111_2	(1 << 28)

// bit definitions for the MDREFR Register
#define MDREFR_DRI_MASK      0xfff
#define MDREFR_E0PIN_BITPOS     12
#define MDREFR_K0RUN_BITPOS     13
#define MDREFR_K0DB2_BITPOS     14
#define MDREFR_E1PIN_BITPOS     15
#define MDREFR_K1RUN_BITPOS     16
#define MDREFR_K1DB2_BITPOS     17
#define MDREFR_K2RUN_BITPOS     18
#define MDREFR_K2DB2_BITPOS     19
#define MDREFR_APD_BITPOS    	20
#define MDREFR_SLFRSH_BITPOS    22
#define MDREFR_K0FREE_BITPOS    23
#define MDREFR_K1FREE_BITPOS    24
#define MDREFR_K2FREE_BITPOS    25

#if defined(CONFIG_MACH_H3900)
/* The EGPIO is a write only control register at physical address 0x49000000
 * See the hardware spec for more details.
 */
#define H3900_ASIC3_VIRT 0x14800000
#define H3800_ASIC_BASE  0x15000000
#define H3600_EGPIO_VIRT 0x15000000

//3800 stuff
#define H3800_FLASH_VPP_ADDR (H3800_ASIC_BASE + _H3800_ASIC2_FlashWP_Base)
#define H3800_FLASH_VPP_ON 0xf1e1;
#define H3800_FLASH_VPP_OFF 0xf1e0;

// defines for our access to the 3800 asics.  requires h3600_asic.h andh3600_gpio.h from linux.
#define H3800_ASIC1_GPIO_MASK_ADDR (H3800_ASIC_BASE  + _H3800_ASIC1_GPIO_Base + _H3800_ASIC1_GPIO_Mask)
#define H3800_ASIC1_GPIO_DIR_ADDR (H3800_ASIC_BASE  + _H3800_ASIC1_GPIO_Base + _H3800_ASIC1_GPIO_Direction)
#define H3800_ASIC1_GPIO_OUT_ADDR (H3800_ASIC_BASE  + _H3800_ASIC1_GPIO_Base + _H3800_ASIC1_GPIO_Out)
#define _H3800_ASIC2_KPIO_Base                 0x0200
#define _H3800_ASIC2_KPIO_Data                 0x0014    /* R/W, 16 bits */
#define _H3800_ASIC2_KPIO_Alternate            0x003c    /* R/W, 6 bits */
#define H3800_ASIC2_KPIO_ADDR	(H3800_ASIC_BASE  + _H3800_ASIC2_KPIO_Base + _H3800_ASIC2_KPIO_Data)
#define H3800_ASIC1_GPIO_MASK_INIT 0x7fff
#define H3800_ASIC1_GPIO_DIR_INIT  0x7fff
#define H3800_ASIC1_GPIO_OUT_INIT  0x2405

#include "ipaq-egpio.h"

#endif

#endif //PXA_H_INCLUDED
