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
#ifndef SKIFF_H_INCLUDED
#define SKIFF_H_INCLUDED

#ifndef CONFIG_MACH_SKIFF
#error Only include this file on SKIFF
#endif //CONFIG_MACH_SKIFF

#define SKIFF_SOCKET_0_BASE (0x82100000)
#define SKIFF_SOCKET_1_BASE (0x82200000)

#define SKIFF_CARDBUS_STATE0 (0x08)
#define SKIFF_CARDBUS_STATE1 (0x09)
#define SKIFF_CARDBUS_STATE2 (0x0a)
#define SKIFF_CARDBUS_STATE3 (0x0b)

#define SKIFF_CARDBUS_CONTROL0 (0x10)
#define SKIFF_CARDBUS_CONTROL1 (0x11)
#define SKIFF_CARDBUS_CONTROL2 (0x12)
#define SKIFF_CARDBUS_CONTROL3 (0x13)



#define SKIFF_DEVICE_PME (0x802)
#define SKIFF_DEVICE_PME2 (0x803)
#define SKIFF_DEVICE_MAPPING (0x806)

#define SKIFF_DEVICE_MM0_HI (0x811)
#define SKIFF_DEVICE_MM0_END_LOW (0x813)
#define SKIFF_DEVICE_MM0_OFF_HI (0x815)

#define SKIFF_DEVICE_MM1_HI (0x819)
#define SKIFF_DEVICE_MM1_END_LOW (0x81B)
#define SKIFF_DEVICE_MM1_OFF_HI (0x81D)

#define SKIFF_DEVICE_TIMER0_SETUP (0x83A)
#define SKIFF_DEVICE_TIMER0_COMMAND (0x83B)
#define SKIFF_DEVICE_TIMER0_RECOVERY (0x83C)


#define SKIFF_VERSION (0x800)
#define SKIFF_IFACE (0x801)





#define SKIFF_SOCKET_0(x) (*((volatile unsigned char *)(SKIFF_SOCKET_0_BASE + x)))
#define SKIFF_SOCKET_1(x) (*((volatile unsigned char *)(SKIFF_SOCKET_1_BASE + x)))








// in cardbus_state0 reg
#define SKIFF_CARD_PRESENT 0x06
// in cardbus_state1 reg
#define SKIFF_CARD_3V 0x08
#define SKIFF_CARD_5V 0x04
// in cardbus_control0
#define SKIFF_CARD_3V_VPP (0x3 << 0)
#define SKIFF_CARD_3V_VCC (0x3 << 4)
#define SKIFF_CARD_5V_VPP (0x2 << 0)
#define SKIFF_CARD_5V_VCC (0x2 << 4)


// device pme
#define SKIFF_CARD_ENABLE (1<<7)
#define SKIFF_VCC_PWR_ENABLE (1<<4)
#define SKIFF_VPP_PWR_ENABLE (1<<0)
// device pme2
#define SKIFF_CARD_RESET_OFF (1<<6)


// in device_mapping
#define SKIFF_MM0_ENABLE (1<<0)
#define SKIFF_MM1_ENABLE (1<<1)
#define SKIFF_MM2_ENABLE (1<<2)
#define SKIFF_MM3_ENABLE (1<<3)
#define SKIFF_MM4_ENABLE (1<<4)
#define SKIFF_IOM0_ENABLE (1<<6)
#define SKIFF_IOM1_ENABLE (1<<7)

//device_mm0_hi
#define SKIFF_16BIT (1<<7)
//device_mm0_off_hi
#define SKIFF_REG_ACTIVE (1<<6)


// in the footbridge
#define SKIFF_RTC_BASE 0x42000300
#define	SKIFF_RTLOAD_OFFSET 0	/* RTC alarm reg */
#define	SKIFF_RTLOAD (SKIFF_RTC_BASE + SKIFF_RTLOAD_OFFSET)
#define	SKIFF_RCNR_OFFSET 4	/* RTC count reg */
#define	SKIFF_RCNR (SKIFF_RTC_BASE + SKIFF_RCNR_OFFSET)
#define	SKIFF_RTCR_OFFSET 8	/* RTC timer control reg */
#define	SKIFF_RTCR (SKIFF_RTC_BASE + SKIFF_RTCR_OFFSET)
#define	SKIFF_RTCLR_OFFSET 0x10	/* RTC clear reg */
#define	SKIFF_RTCLR (SKIFF_RTC_BASE + SKIFF_RTSR_OFFSET)


#define SKIFF_RTC_ENABLE 0x80
#define SKIFF_RTC_FCLK 0x0
#define SKIFF_RTC_FCLK_DIV16 0x4
#define SKIFF_RTC_FCLK_DIV256 0x8
#define SKIFF_COUNTS_PER_SEC    (187500)















#endif //SKIFF_H_INCLUDED
