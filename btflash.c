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
 * btflash.c - Flash erase and program routines for Compaq Personal Server Monitor
 *
 */

#if !(defined(CONFIG_PXA) || defined(CONFIG_PXA1))
#include "sa1100.h"
#endif
#include "architecture.h"
#include "btflash.h"
#include "bootldr.h"
#include "params.h"
#include "serial.h"
#include "heap.h"
#include <errno.h>
#include <string.h>

#if defined(__linux__) || defined(__QNXNTO__)
#include <mtd-bootldr.h>
#ifdef CONFIG_MACH_IPAQ
#include <asm-arm/arch-sa1100/h3600_gpio.h>
#include <asm-arm/arch-sa1100/h3600_asic.h>	
#endif /* IPAQ */
#endif

#define FLASH_TIMEOUT 20000000

int btflash_verbose = 0;

#define bothbanks(w_) ((((w_)&0xFFFF) << 16)|((w_)&0xFFFF))

#ifdef CONFIG_AMD_FLASH
static unsigned long flashSectors_Am29LV160BB[] = {
  0x00000000, /* first sector of bootldr */
  0x00008000, /* last sector of bootldr */
  0x0000c000, /* param sector */
  0x00010000, /* kernel starts here */
  0x00020000,
  0x00040000,
  0x00060000,
  0x00080000,
  0x000A0000,
  0x000C0000,
  0x000E0000,
  0x00100000,
  0x00120000,
  0x00140000,
  0x00160000,
  0x00180000,
  0x001A0000,
  0x001C0000,
  0x001E0000,
  0x00200000,
  0x00220000,
  0x00240000,
  0x00260000,
  0x00280000,
  0x002A0000,
  0x002C0000,
  0x002E0000,
  0x00300000, /* user code starts here */
  0x00320000,
  0x00340000,
  0x00360000,
  0x00380000,
  0x003A0000,
  0x003C0000,
  0x003E0000,
  0x00400000 /* guard sector -- not in flash */
};


static int amdFlashReset(void);
static int amdFlashProgramWord(unsigned long flashAddress, unsigned long value);
static int amdFlashProgramBlock(unsigned long flashAddress, unsigned long *values, int nbytes);
static int amdFlashEraseChip(void);
static int amdFlashEraseSector(unsigned long sectorAddress);
static int amdFlashEraseRange(unsigned long start, unsigned long len);
static int amdFlashProtectRange(unsigned long start, unsigned long len, int protect);

static FlashAlgorithm amdFlashAlgorithm = {
   amdFlashReset,
   amdFlashProgramWord,
   amdFlashProgramBlock,
   amdFlashEraseChip,
   amdFlashEraseSector,
   amdFlashEraseRange,
   amdFlashProtectRange
};

static struct FlashDescriptor flashDescriptor_Am29LV160BB = {
   "Am29LV160BB",
   bothbanks(1), bothbanks(0x2249), 
   &amdFlashAlgorithm,
   sizeof(flashSectors_Am29LV160BB)/sizeof(dword) - 1, flashSectors_Am29LV160BB,
   { "bootldr",   0x00000000, 0x0000c000, LFR_BOOTLDR },
   //{ "params",    0x0000c000, 0x00004000, 0 },
   //{ "kernel",    0x00010000, 0x00200000, 0 },
   { "root",    0x00010000, 0x0, LFR_EXPAND|LFR_JFFS2 },
};
#endif /* CONFIG_AMD_FLASH */

#ifdef CONFIG_AMD_FLASH
static unsigned long flashSectors_Am29DL323CT[] = {
    0x00000000,
    0x00020000,
    0x00040000,
    0x00060000,
    0x00080000,
    0x000a0000,
    0x000c0000,
    0x000e0000,
    0x00100000,
    0x00120000,
    0x00140000,
    0x00160000,
    0x00180000,
    0x001a0000,
    0x001c0000,
    0x001e0000,
    0x00200000,
    0x00220000,
    0x00240000,
    0x00260000,
    0x00280000,
    0x002a0000,
    0x002c0000,
    0x002e0000,
    0x00300000,
    0x00320000,
    0x00340000,
    0x00360000,
    0x00380000,
    0x003a0000,
    0x003c0000,
    0x003e0000,
    0x00400000,
    0x00420000,
    0x00440000,
    0x00460000,
    0x00480000,
    0x004a0000,
    0x004c0000,
    0x004e0000,
    0x00500000,
    0x00520000,
    0x00540000,
    0x00560000,
    0x00580000,
    0x005a0000,
    0x005c0000,
    0x005e0000,
    0x00600000,
    0x00620000,
    0x00640000,
    0x00660000,
    0x00680000,
    0x006a0000,
    0x006c0000,
    0x006e0000,
    0x00700000,
    0x00720000,
    0x00740000,
    0x00760000,
    0x00780000,
    0x007a0000,
    0x007c0000,
    0x007e0000,
    0x007e4000,
    0x007e8000,
    0x007ec000,
    0x007f0000,
    0x007f4000,
    0x007f8000,
    0x007fc000,
    0x00800000 /* guard entry */
};

static struct FlashDescriptor flashDescriptor_Am29DL323CT =  {
   "Am29DL323CT", 
   bothbanks(1), bothbanks(0x2250), 
   &amdFlashAlgorithm,
   sizeof(flashSectors_Am29DL323CT)/sizeof(dword) - 1, flashSectors_Am29DL323CT,
   { "bootldr",   0x00000000, 0x00040000, LFR_BOOTLDR },
   //{ "params",    0x007f4000, 0x00004000, 0 },
   //{ "kernel",    0x00020000, 0x000e0000, 0 },
   //{ "root",    0x00040000, 0x0, LFR_EXPAND|LFR_JFFS2 }, nope, linux jffs2 needs them to all be the same size
   { "root",    0x00040000, 0x7A0000, LFR_JFFS2 }, 
};

static unsigned long flashSectors_Am29DL323CB[] = {
    0x00000000,
    0x00004000,
    0x00008000,
    0x0000c000,
    0x00010000,
    0x00014000,
    0x00018000,
    0x0001c000,
    0x00020000,
    0x00040000,
    0x00060000,
    0x00080000,
    0x000a0000,
    0x000c0000,
    0x000e0000,
    0x00100000,
    0x00120000,
    0x00140000,
    0x00160000,
    0x00180000,
    0x001a0000,
    0x001c0000,
    0x001e0000,
    0x00200000,
    0x00220000,
    0x00240000,
    0x00260000,
    0x00280000,
    0x002a0000,
    0x002c0000,
    0x002e0000,
    0x00300000,
    0x00320000,
    0x00340000,
    0x00360000,
    0x00380000,
    0x003a0000,
    0x003c0000,
    0x003e0000,
    0x00400000,
    0x00420000,
    0x00440000,
    0x00460000,
    0x00480000,
    0x004a0000,
    0x004c0000,
    0x004e0000,
    0x00500000,
    0x00520000,
    0x00540000,
    0x00560000,
    0x00580000,
    0x005a0000,
    0x005c0000,
    0x005e0000,
    0x00600000,
    0x00620000,
    0x00640000,
    0x00660000,
    0x00680000,
    0x006a0000,
    0x006c0000,
    0x006e0000,
    0x00700000,
    0x00720000,
    0x00740000,
    0x00760000,
    0x00780000,
    0x007a0000,
    0x007c0000,
    0x007e0000,
    0x00800000 /* guard entry */
};

static struct FlashDescriptor flashDescriptor_Am29DL323CB=  {
   "Am29DL323CB", 
   bothbanks(1), bothbanks(0x2253), 
   &amdFlashAlgorithm,
   sizeof(flashSectors_Am29DL323CB)/sizeof(dword) - 1, flashSectors_Am29DL323CB,
   { "bootldr",   0x00000000, 0x00040000, LFR_BOOTLDR },
   //{ "params",    0x007e0000, 0x00004000, 0 },
   //{ "kernel",    0x00040000, 0x000c0000, 0 },
   { "root",    0x00040000, 0x0, LFR_EXPAND|LFR_JFFS2 },
};
#endif /* CONFIG_AMD_FLASH */


#ifdef CONFIG_INTEL_FLASH

static int intelFlashReset(void);
static int intelFlashProgramBlock(unsigned long flashAddress, unsigned long *values, int nbytes);
static int intelFlashProgramBlock_1x16(unsigned long flashAddress, unsigned long *values, int nbytes);
static int intelFlashProgramWord(unsigned long flashAddress, unsigned long value);
static int intelFlashProgramWord_1x16(unsigned long flashAddress, unsigned long value);
static int intelFlashEraseChip(void);
static int intelFlashEraseSector(unsigned long sectorAddress);
static int intelFlashEraseRange(unsigned long start, unsigned long len);
static int intelFlashProtectRange(unsigned long start, unsigned long len, int protect);
static int intelFlashProtectRange_1x16(unsigned long start, unsigned long len, int protect);

static unsigned long flashSectors_28F128J3A_1x16[] = {
  0x00000000, 
  0x00040000/2, 
  0x00080000/2, 
  0x000c0000/2, 
  0x00100000/2, 
  0x00140000/2, 
  0x00180000/2, 
  0x001c0000/2, 
  0x00200000/2, 
  0x00240000/2, 
  0x00280000/2, 
  0x002c0000/2, 
  0x00300000/2, 
  0x00340000/2, 
  0x00380000/2, 
  0x003c0000/2, 
  0x00400000/2, 
  0x00440000/2, 
  0x00480000/2, 
  0x004c0000/2, 
  0x00500000/2, 
  0x00540000/2, 
  0x00580000/2, 
  0x005c0000/2, 
  0x00600000/2, 
  0x00640000/2, 
  0x00680000/2, 
  0x006c0000/2, 
  0x00700000/2, 
  0x00740000/2, 
  0x00780000/2, 
  0x007c0000/2, 
  0x00800000/2, 
  0x00840000/2, 
  0x00880000/2, 
  0x008c0000/2, 
  0x00900000/2, 
  0x00940000/2, 
  0x00980000/2, 
  0x009c0000/2, 
  0x00a00000/2, 
  0x00a40000/2, 
  0x00a80000/2, 
  0x00ac0000/2, 
  0x00b00000/2, 
  0x00b40000/2, 
  0x00b80000/2, 
  0x00bc0000/2, 
  0x00c00000/2, 
  0x00c40000/2, 
  0x00c80000/2, 
  0x00cc0000/2, 
  0x00d00000/2, 
  0x00d40000/2, 
  0x00d80000/2, 
  0x00dc0000/2, 
  0x00e00000/2, 
  0x00e40000/2, 
  0x00e80000/2, 
  0x00ec0000/2, 
  0x00f00000/2, 
  0x00f40000/2, 
  0x00f80000/2, 
  0x00fc0000/2, 
  0x01000000/2, 
  0x01040000/2, 
  0x01080000/2, 
  0x010c0000/2, 
  0x01100000/2, 
  0x01140000/2, 
  0x01180000/2, 
  0x011c0000/2, 
  0x01200000/2, 
  0x01240000/2, 
  0x01280000/2, 
  0x012c0000/2, 
  0x01300000/2, 
  0x01340000/2, 
  0x01380000/2, 
  0x013c0000/2, 
  0x01400000/2, 
  0x01440000/2, 
  0x01480000/2, 
  0x014c0000/2, 
  0x01500000/2, 
  0x01540000/2, 
  0x01580000/2, 
  0x015c0000/2, 
  0x01600000/2, 
  0x01640000/2, 
  0x01680000/2, 
  0x016c0000/2, 
  0x01700000/2, 
  0x01740000/2, 
  0x01780000/2, 
  0x017c0000/2, 
  0x01800000/2, 
  0x01840000/2, 
  0x01880000/2, 
  0x018c0000/2, 
  0x01900000/2, 
  0x01940000/2, 
  0x01980000/2, 
  0x019c0000/2, 
  0x01a00000/2, 
  0x01a40000/2, 
  0x01a80000/2, 
  0x01ac0000/2, 
  0x01b00000/2, 
  0x01b40000/2, 
  0x01b80000/2, 
  0x01bc0000/2, 
  0x01c00000/2, 
  0x01c40000/2, 
  0x01c80000/2, 
  0x01cc0000/2, 
  0x01d00000/2, 
  0x01d40000/2, 
  0x01d80000/2, 
  0x01dc0000/2, 
  0x01e00000/2, 
  0x01e40000/2, 
  0x01e80000/2, 
  0x01ec0000/2, 
  0x01f00000/2, 
  0x01f40000/2, 
  0x01f80000/2, 
  0x01fc0000/2, 
  0x02000000/2 /* guard sector */
#ifdef CONFIG_IPCOM_32M
	  ,
  0x02040000/2, 
  0x02080000/2, 
  0x020c0000/2, 
  0x02100000/2, 
  0x02140000/2, 
  0x02180000/2, 
  0x021c0000/2, 
  0x02200000/2, 
  0x02240000/2, 
  0x02280000/2, 
  0x022c0000/2, 
  0x02300000/2, 
  0x02340000/2, 
  0x02380000/2, 
  0x023c0000/2, 
  0x02400000/2, 
  0x02440000/2, 
  0x02480000/2, 
  0x024c0000/2, 
  0x02500000/2, 
  0x02540000/2, 
  0x02580000/2, 
  0x025c0000/2, 
  0x02600000/2, 
  0x02640000/2, 
  0x02680000/2, 
  0x026c0000/2, 
  0x02700000/2, 
  0x02740000/2, 
  0x02780000/2, 
  0x027c0000/2, 
  0x02800000/2, 
  0x02840000/2, 
  0x02880000/2, 
  0x028c0000/2, 
  0x02900000/2, 
  0x02940000/2, 
  0x02980000/2, 
  0x029c0000/2, 
  0x02a00000/2, 
  0x02a40000/2, 
  0x02a80000/2, 
  0x02ac0000/2, 
  0x02b00000/2, 
  0x02b40000/2, 
  0x02b80000/2, 
  0x02bc0000/2, 
  0x02c00000/2, 
  0x02c40000/2, 
  0x02c80000/2, 
  0x02cc0000/2, 
  0x02d00000/2, 
  0x02d40000/2, 
  0x02d80000/2, 
  0x02dc0000/2, 
  0x02e00000/2, 
  0x02e40000/2, 
  0x02e80000/2, 
  0x02ec0000/2, 
  0x02f00000/2, 
  0x02f40000/2, 
  0x02f80000/2, 
  0x02fc0000/2, 
  0x03000000/2, 
  0x03040000/2, 
  0x03080000/2, 
  0x030c0000/2, 
  0x03100000/2, 
  0x03140000/2, 
  0x03180000/2, 
  0x031c0000/2, 
  0x03200000/2, 
  0x03240000/2, 
  0x03280000/2, 
  0x032c0000/2, 
  0x03300000/2, 
  0x03340000/2, 
  0x03380000/2, 
  0x033c0000/2, 
  0x03400000/2, 
  0x03440000/2, 
  0x03480000/2, 
  0x034c0000/2, 
  0x03500000/2, 
  0x03540000/2, 
  0x03580000/2, 
  0x035c0000/2, 
  0x03600000/2, 
  0x03640000/2, 
  0x03680000/2, 
  0x036c0000/2, 
  0x03700000/2, 
  0x03740000/2, 
  0x03780000/2, 
  0x037c0000/2, 
  0x03800000/2, 
  0x03840000/2, 
  0x03880000/2, 
  0x038c0000/2, 
  0x03900000/2, 
  0x03940000/2, 
  0x03980000/2, 
  0x039c0000/2, 
  0x03a00000/2, 
  0x03a40000/2, 
  0x03a80000/2, 
  0x03ac0000/2, 
  0x03b00000/2, 
  0x03b40000/2, 
  0x03b80000/2, 
  0x03bc0000/2, 
  0x03c00000/2, 
  0x03c40000/2, 
  0x03c80000/2, 
  0x03cc0000/2, 
  0x03d00000/2, 
  0x03d40000/2, 
  0x03d80000/2, 
  0x03dc0000/2, 
  0x03e00000/2, 
  0x03e40000/2, 
  0x03e80000/2, 
  0x03ec0000/2, 
  0x03f00000/2, 
  0x03f40000/2, 
  0x03f80000/2, 
  0x03fc0000/2,
  0x04000000/2
#endif
};
static FlashAlgorithm intelFlashAlgorithm_1x16 = {
   intelFlashReset,
   intelFlashProgramWord_1x16,
   intelFlashProgramBlock_1x16,
   intelFlashEraseChip,		/* not implemented anyway */
   intelFlashEraseSector, /*  intelFlashEraseSector_1x16, */
   intelFlashEraseRange, /*  intelFlashEraseRange_1x16,	*/ /* will be ok if we fix erase sector */
   intelFlashProtectRange_1x16 
};

static FlashDescriptor flashDescriptor_28F128J3A_1x16 = {
   "28F128J3A_1x16", 
   0x89, 0x18, 
   &intelFlashAlgorithm_1x16,
   sizeof(flashSectors_28F128J3A_1x16)/sizeof(dword) - 1,
   flashSectors_28F128J3A_1x16,
   { "bootldr",   0x00000000, 0x00040000, LFR_BOOTLDR },
   //{ "params",    0x00020000, 0x00020000, 0 },
   //{ "kernel",    0x00040000, 0x000c0000, 0 }
   //IPCOM ADDITION
   { "root",    0x00040000, 0x0, LFR_EXPAND|LFR_JFFS2 }
};

static unsigned long flashSectors_28F128J3A[] = {
  0x00000000, 
  0x00040000, 
  0x00080000, 
  0x000c0000, 
  0x00100000, 
  0x00140000, 
  0x00180000, 
  0x001c0000, 
  0x00200000, 
  0x00240000, 
  0x00280000, 
  0x002c0000, 
  0x00300000, 
  0x00340000, 
  0x00380000, 
  0x003c0000, 
  0x00400000, 
  0x00440000, 
  0x00480000, 
  0x004c0000, 
  0x00500000, 
  0x00540000, 
  0x00580000, 
  0x005c0000, 
  0x00600000, 
  0x00640000, 
  0x00680000, 
  0x006c0000, 
  0x00700000, 
  0x00740000, 
  0x00780000, 
  0x007c0000, 
  0x00800000, 
  0x00840000, 
  0x00880000, 
  0x008c0000, 
  0x00900000, 
  0x00940000, 
  0x00980000, 
  0x009c0000, 
  0x00a00000, 
  0x00a40000, 
  0x00a80000, 
  0x00ac0000, 
  0x00b00000, 
  0x00b40000, 
  0x00b80000, 
  0x00bc0000, 
  0x00c00000, 
  0x00c40000, 
  0x00c80000, 
  0x00cc0000, 
  0x00d00000, 
  0x00d40000, 
  0x00d80000, 
  0x00dc0000, 
  0x00e00000, 
  0x00e40000, 
  0x00e80000, 
  0x00ec0000, 
  0x00f00000, 
  0x00f40000, 
  0x00f80000, 
  0x00fc0000, 
  0x01000000, 
  0x01040000, 
  0x01080000, 
  0x010c0000, 
  0x01100000, 
  0x01140000, 
  0x01180000, 
  0x011c0000, 
  0x01200000, 
  0x01240000, 
  0x01280000, 
  0x012c0000, 
  0x01300000, 
  0x01340000, 
  0x01380000, 
  0x013c0000, 
  0x01400000, 
  0x01440000, 
  0x01480000, 
  0x014c0000, 
  0x01500000, 
  0x01540000, 
  0x01580000, 
  0x015c0000, 
  0x01600000, 
  0x01640000, 
  0x01680000, 
  0x016c0000, 
  0x01700000, 
  0x01740000, 
  0x01780000, 
  0x017c0000, 
  0x01800000, 
  0x01840000, 
  0x01880000, 
  0x018c0000, 
  0x01900000, 
  0x01940000, 
  0x01980000, 
  0x019c0000, 
  0x01a00000, 
  0x01a40000, 
  0x01a80000, 
  0x01ac0000, 
  0x01b00000, 
  0x01b40000, 
  0x01b80000, 
  0x01bc0000, 
  0x01c00000, 
  0x01c40000, 
  0x01c80000, 
  0x01cc0000, 
  0x01d00000, 
  0x01d40000, 
  0x01d80000, 
  0x01dc0000, 
  0x01e00000, 
  0x01e40000, 
  0x01e80000, 
  0x01ec0000, 
  0x01f00000, 
  0x01f40000, 
  0x01f80000, 
  0x01fc0000, 
  0x02000000 /* guard sector */
};

static FlashAlgorithm intelFlashAlgorithm = {
   intelFlashReset,
   intelFlashProgramWord,
   intelFlashProgramBlock,
   intelFlashEraseChip,
   intelFlashEraseSector,
   intelFlashEraseRange,
   intelFlashProtectRange
};

static FlashDescriptor flashDescriptor_28F128J3A = {
   "28F128J3A", 
   bothbanks(0x89), bothbanks(0x18), 
   &intelFlashAlgorithm,
   sizeof(flashSectors_28F128J3A)/sizeof(dword) - 1, flashSectors_28F128J3A,
   { "bootldr",   0x00000000, 0x00040000, LFR_BOOTLDR },
#if defined(CONFIG_BIG_KERNEL)
   //{ "params",    0x00040000, 0x00040000, 0 },
   //{ "kernel",    0x00080000, 0x000c0000, 0 },
   { "root",    0x00040000, 0x0, LFR_EXPAND|LFR_JFFS2 },
#else
   { "params",    0x00040000, 0x00040000, 0 },
   { "kernel",    0x00080000, 0x00080000, 0 },
#endif
};

static unsigned long flashSectors_28F640J3A[] = {
  0x00000000, 
  0x00040000, 
  0x00080000, 
  0x000c0000, 
  0x00100000, 
  0x00140000, 
  0x00180000, 
  0x001c0000, 
  0x00200000, 
  0x00240000, 
  0x00280000, 
  0x002c0000, 
  0x00300000, 
  0x00340000, 
  0x00380000, 
  0x003c0000, 
  0x00400000, 
  0x00440000, 
  0x00480000, 
  0x004c0000, 
  0x00500000, 
  0x00540000, 
  0x00580000, 
  0x005c0000, 
  0x00600000, 
  0x00640000, 
  0x00680000, 
  0x006c0000, 
  0x00700000, 
  0x00740000, 
  0x00780000, 
  0x007c0000, 
  0x00800000, 
  0x00840000, 
  0x00880000, 
  0x008c0000, 
  0x00900000, 
  0x00940000, 
  0x00980000, 
  0x009c0000, 
  0x00a00000, 
  0x00a40000, 
  0x00a80000, 
  0x00ac0000, 
  0x00b00000, 
  0x00b40000, 
  0x00b80000, 
  0x00bc0000, 
  0x00c00000, 
  0x00c40000, 
  0x00c80000, 
  0x00cc0000, 
  0x00d00000, 
  0x00d40000, 
  0x00d80000, 
  0x00dc0000, 
  0x00e00000, 
  0x00e40000, 
  0x00e80000, 
  0x00ec0000, 
  0x00f00000, 
  0x00f40000, 
  0x00f80000, 
  0x00fc0000, 
  0x01000000   /* guard sector */
};

static FlashDescriptor flashDescriptor_28F640J3A = {
   "28F640J3A", 
   bothbanks(0x89), bothbanks(0x17), 
   &intelFlashAlgorithm,
   sizeof(flashSectors_28F640J3A)/sizeof(dword) - 1, flashSectors_28F640J3A,
   { "bootldr",   0x00000000, 0x00040000, LFR_BOOTLDR },
#if defined(CONFIG_BIG_KERNEL)
   //{ "params",    0x00040000, 0x00040000, 0 },
   //{ "kernel",    0x00080000, 0x000c0000, 0 },
   { "root",    0x00040000, 0x0, LFR_EXPAND|LFR_JFFS2 },
#else
   { "params",    0x000c0000, 0x00040000, 0 },
   { "kernel",    0x00040000, 0x00080000, 0 },
#endif
};

static unsigned long flashSectors_28F160B3[] = {
  0x00004000, 
  0x00008000,
  0x0000c000,
  0x00010000,
  0x00014000,
  0x00018000,
  0x0001c000,
  0x00020000,
  0x00040000, 
  0x00060000, 
  0x00080000, 
  0x000a0000, 
  0x000c0000, 
  0x000e0000, 
  0x00100000, 
  0x00120000, 
  0x00140000, 
  0x00160000, 
  0x00180000, 
  0x001a0000, 
  0x001c0000,
  0x001e0000, 
  0x00200000, 
  0x00220000, 
  0x00240000, 
  0x00280000, 
  0x002a0000, 
  0x002c0000, 
  0x002e0000, 
  0x00300000, 
  0x00320000, 
  0x00340000, 
  0x00360000, 
  0x00380000, 
  0x003a0000, 
  0x003c0000, 
  0x003e0000,
  0x00400000   /* guard sector */
};

static FlashDescriptor flashDescriptor_28F160B3 = {
   "28F160B3", 
   bothbanks(0x89), bothbanks(0xd0), 
   &intelFlashAlgorithm,
   sizeof(flashSectors_28F160B3)/sizeof(dword) - 1, flashSectors_28F160B3,
   { "bootldr",   0x00000000, 0x0001c000, LFR_BOOTLDR },
   //{ "params",    0x0001c000, 0x00004000, 0 },
   //{ "kernel",    0x00020000, 0x00100000, 0 },
   { "root",    0x00020000, 0x0, LFR_EXPAND|LFR_JFFS2 },
};

static unsigned long flashSectors_28F160S3[] = {
  0x00000000,  /* 16-Mbit flash has 31 "main" (32-Kword) blocks */
  0x00020000,
  0x00040000,
  0x00060000,
  0x00080000,
  0x000a0000,
  0x000c0000,
  0x000e0000,
  0x00100000,
  0x00120000,
  0x00140000,
  0x00160000,
  0x00180000,
  0x001a0000,
  0x001c0000,
  0x001e0000,
  0x00200000,
  0x00220000,
  0x00240000,
  0x00260000,
  0x00280000,
  0x002a0000,
  0x002c0000,
  0x002e0000,
  0x00300000,
  0x00320000,
  0x00340000,
  0x00360000,
  0x00380000,
  0x003a0000,
  0x003c0000,

  0x003e0000,  /* 16-Mbit flash has 8 "parameter" (4-Kword) blocks */
  0x003e4000,
  0x003e8000,
  0x003ec000,
  0x003f0000,
  0x003f4000,
  0x003f8000,
  0x003fc000,

  0x00400000   /* guard sector */ 
};

static FlashDescriptor flashDescriptor_28F160S3 = {
   "28F160S3", 
   bothbanks(0xb0), bothbanks(0xd0), 
   &intelFlashAlgorithm,
   sizeof(flashSectors_28F160S3)/sizeof(dword) - 1, flashSectors_28F160S3,
   { "bootldr",   0x00000000, 0x00020000, LFR_BOOTLDR },
   { "params",    0x00020000, 0x00020000, 0 },
};

#endif /* CONFIG_INTEL_FLASH */


static FlashDescriptor *flashDescriptors[] = {
#ifdef CONFIG_AMD_FLASH
   &flashDescriptor_Am29LV160BB,
   &flashDescriptor_Am29DL323CT,
   &flashDescriptor_Am29DL323CB,
#endif /* CONFIG_AMD_FLASH */
#ifdef CONFIG_INTEL_FLASH 
   &flashDescriptor_28F128J3A,
   &flashDescriptor_28F640J3A,
   &flashDescriptor_28F160B3,
   &flashDescriptor_28F160S3,
#endif /* CONFIG_INTEL_FLASH */
   NULL
};

static FlashDescriptor *flashDescriptors_1x16[] = {
#if defined(CONFIG_MACH_IPAQ)
   &flashDescriptor_28F128J3A_1x16,
#endif   
   NULL
};




unsigned long flash_size = 0;
unsigned long flash_address_mask = -1;
int nsectors = 0;
unsigned long *flashSectors = NULL;
FlashDescriptor *flashDescriptor = NULL;


int verifiedFlashRange( unsigned long addr, unsigned long len ) {

  if ( addr + len > flash_size )
    return 0;
  else
    return 1;
}

static int checkFlashDescriptor(const char *where)
{
   if (flashDescriptor == NULL) {
      putstr("No flash descriptor: "); putstr(where); putstr("\r\n");
      return -1;
   } else {
      return 0;
   }
}

int resetFlash ()
{
   if (!checkFlashDescriptor("reset flash")) {
      return flashDescriptor->algorithm->reset();
   } else {
      return -EINVAL;
   }
}

/*
 * Programs value at flashAddress
 * Sectors must be erased before they can be programmed.
 */
int programFlashWord(unsigned long flashAddress, unsigned long value)
{
   if (!checkFlashDescriptor("program flash word")) {
      return flashDescriptor->algorithm->programWord(flashAddress, value);
   } else {
      return -EINVAL;
   }
}

/*
 * Programs block of values starting at flashAddress
 * Sectors must be erased before they can be programmed.
 */
int programFlashBlock(unsigned long flashAddress, unsigned long *values, int nbytes)
{
   if (!checkFlashDescriptor("program flash word")) {
      return flashDescriptor->algorithm->programBlock(flashAddress, values, nbytes);
   } else {
      return -EINVAL;
   }
}

int eraseFlashChip ()
{
   if (!checkFlashDescriptor("erase flash chip")) {
      return flashDescriptor->algorithm->eraseChip();
   } else {
      return -EINVAL;
   }
}

/* sectorAddress must be a valid start of sector address. sectors
   must be erased before they can be programmed! */
int eraseFlashSector (unsigned long sectorAddress)
{
   if (!checkFlashDescriptor("erase flash sector")) {
      return flashDescriptor->algorithm->eraseSector(sectorAddress);
   } else {
      return -EINVAL;
   }
}

int eraseFlashRange(unsigned long startAddress, unsigned long len)
{
   if (!checkFlashDescriptor("erase flash range")) {
      return flashDescriptor->algorithm->eraseRange(startAddress, len);
   } else {
      return -EINVAL;
   }
}

int protectFlashRange(unsigned long startAddress, unsigned long len, int protect)
{
   if (!checkFlashDescriptor("erase flash range")) {
      return flashDescriptor->algorithm->protectRange(startAddress, len, protect);
   } else {
      return -EINVAL;
   }
}

unsigned long queryFlash(unsigned long flashWordAddress)
{
  /* there's a reason why we don't shift flashWordOffset by 2
   * here... we want the values we pass to queryFlash match the values on pages 15-17 of the spec.
   */
   unsigned long result;
   unsigned long flashWordOffset = flashWordAddress&flash_address_mask;
   unsigned long offset = 0; /* IPCOM */
#ifdef CONFIG_IPCOM_32M
   /* IPCOM : here we are geting a half-word address */
   offset = flashWordAddress & IPCOM_WRITE_OFFSET;
#endif

   /* put flash in query mode */
   /* davep flashword[0x55] = bothbanks(0x98); */
   flash_write_cmd(offset + 0x55, 0x98); /* IPCOM */
   
   /*  davep result = flashword[flashWordOffset]; */
   result = flash_read_val(flashWordOffset);
   
   /* reset flash */
   flash_write_cmd(offset + 0x55, 0xff); /* flashword[0x55] = bothbanks(0xFF);*/ /* IPCOM */
   return result;
}

static FlashAlgorithm *flashAlgorithm = NULL;
int updateFlashAlgorithm()
{
  int algorithm = queryFlash(0x13) & 0xFF;
  switch (algorithm) {
#ifdef CONFIG_INTEL_FLASH
  case 1:
    flashAlgorithm = &intelFlashAlgorithm;
    break;
#endif
#ifdef CONFIG_AMD_FLASH
  case 2:
    flashAlgorithm = &amdFlashAlgorithm;
    break;
#endif
  default:
    putLabeledWord("No flash algorithm known for CFI vendorID=", algorithm);
    algorithm = -1;
  }
  return algorithm;
}


unsigned long queryFlashID(unsigned long flashWordAddress)
{
   unsigned long result;
   unsigned long flashWordOffset = (flashWordAddress&flash_address_mask);
   int algorithm = queryFlash(0x13) & 0xFF;

   switch (algorithm) {
   case 1:
     /* reset flash -- Intel */
     flash_write_cmd(0x55, 0xff); /* flashword[0x55] = bothbanks(0xFF); */
     /* put flash in read identifier codes mode */
     flash_write_cmd(0x555, 0x90); /*  flashword[0x555] = bothbanks(0x90); */
     break;

   case 2:
     /* reset flash -- AMD */
     flash_write_cmd(0x55, 0xf0); /*  flashword[0x55] = bothbanks(0xF0); */
     /* put flash in query mode */
     flash_write_cmd(0x555, 0xaa); /*  flashword[0x555] = bothbanks(0xAA); */
     flash_write_cmd(0x2aa, 0x55); /*  flashword[0x2AA] = bothbanks(0x55); */
     flash_write_cmd(0x555, 0x90); /*  flashword[0x555] = bothbanks(0x90); */
     break;
   }

   /* read autoselect word */
   result = flash_read_val(flashWordOffset); /* flashword[flashWordOffset]; */

   switch (algorithm) {
   case 1:
     /* reset flash -- Intel */
     flash_write_cmd(0x55, 0xff); /*  flashword[0x55] = bothbanks(0xFF); */
     break;
   case 2:
     /* reset flash -- AMD */
     flash_write_cmd(0x55, 0xf0); /*  flashword[0x55] = bothbanks(0xF0); */
     break;
   }
   return result;
}

unsigned long queryFlashSecurity(unsigned long flashWordAddress)
{
  /* there's a reason why we don't shift flashWordOffset by 2
   * here... we want the values we pass to queryFlash match the values on pages 15-17 of the spec.
   */
   unsigned long result;
   unsigned long flashWordOffset = (flashWordAddress&flash_address_mask);

   /* reset flash */
   flash_write_cmd(0x555, 0xf0); /*  flashword[0x555] = bothbanks(0xF0); */
   flash_write_cmd(0x555, 0xf0); /*  flashword[0x555] = bothbanks(0xF0); */
   /* enter SecSi Sector Region */
   flash_write_cmd(0x555, 0xaa); /*  flashword[0x555] = bothbanks(0xAA); */
   flash_write_cmd(0x2aa, 0x55); /*  flashword[0x2AA] = bothbanks(0x55); */
   flash_write_cmd(0x555, 0x88); /*  flashword[0x555] = bothbanks(0x88); */

   /* read word from SecSi region */
   result = flash_read_val(flashWordOffset); /*  flashword[flashWordOffset]; */

   /* exit region */
   nflash_write_cmd(0x555, 0xAA); /*  flashword[0x555] = bothbanks(0xAA); */
   nflash_write_cmd(0x2AA, 0x55); /*  flashword[0x2AA] = bothbanks(0x55); */
   nflash_write_cmd(0x555, 0x90); /*  flashword[0x555] = bothbanks(0x90); */
   nflash_write_cmd(0x000, 0x00); /*  flashword[0x000] = bothbanks(0x00); */

   return result;
}


#ifdef CONFIG_AMD_FLASH
int amdFlashReset ()
{
   /* send flash the reset command */
   nflash_write_cmd(0x55, 0xF0); /*  flashword[0x55] = bothbanks(0xF0); */
   return 0;
}

/*
 * Programs value at flashAddress
 * Sectors must be erased before they can be programmed.
 */
static int amdFlashProgramWord(unsigned long flashAddress, unsigned long value)
{
   unsigned long flashWordOffset = (flashAddress&flash_address_mask) >> 2;
   long timeout = FLASH_TIMEOUT;
   unsigned long flashContents = nflash_read_val(flashWordOffset);
   unsigned long oldFlashContents;
   unsigned long hivalue = (value >> 16) & 0xFFFFl;
   unsigned long lovalue = (value >> 0) & 0xFFFFl;

   /* see if we can program the value without erasing */
   if ((flashContents & value) != value) {
      putstr("the flash sector needs to be erased first!\r\n");
      putLabeledWord("  flashAddress=", flashAddress);
      putLabeledWord("  flashWordOffset=", flashWordOffset);
#if 0 /* jca */
      putLabeledWord("  &flashword[flashWordOffset]=", 
		     (dword)&flashword[flashWordOffset]);
#endif
      putLabeledWord("  flashContents=", flashContents);
      putLabeledWord("          value=", value);
   }
   
   /* send flash the program word command */
   nflash_write_cmd(0x555, 0xAA); /*  flashword[0x555] = bothbanks(0xAA); */
   nflash_write_cmd(0x2AA, 0x55); /*  flashword[0x2AA] = bothbanks(0x55); */
   nflash_write_cmd(0x555, 0xA0); /*  flashword[0x555] = bothbanks(0xA0); */
   nflash_write_val(flashWordOffset, value);
   /* now wait for it to be programmed */
   while (((flashContents = nflash_read_val(flashWordOffset)) != value) &&
	  (timeout > 0)) {
      unsigned long hiword = (flashContents >> 16) & 0xFFFFl;
      unsigned long loword = (flashContents >> 0) & 0xFFFFl;
      if (0 && (hiword != hivalue) && (hiword & (1 << 5))) {
         /* programming upper bank of flash timed out */
         putstr("Upper bank of flash timed out!!");
	 timeout = 0;
         break;
      }
      if (0 && (loword != lovalue) && (loword & (1 << 5))) {
         /* programming upper bank of flash timed out */
         putstr("Lower bank of flash timed out!!");
	 timeout = 0;
         break;
      }
      oldFlashContents = flashContents;
      timeout--;
   }
   if (timeout <= 0) {
      putstr("programFlashWord timeout\r\n");
      putLabeledWord("  flashAddress=", flashAddress);
      putLabeledWord("  value=", value);
      putLabeledWord("  flashContents=", flashContents);
      putLabeledWord("  oldFlashContents=", oldFlashContents);
      return(-1);
   }
   return 0;
}

static int amdFlashProgramBlock(unsigned long flashAddress, unsigned long *values, int nbytes)
{
   int nwords = nbytes >> 2;
   int result = 0;
   int i;
   for (i = 0; i < nwords; i++) {
      result |= amdFlashProgramWord(flashAddress + (i*4), values[i]);
      if (result)
         break;
   }
   return result;
}

int amdFlashEraseChip ()
{
   int i;
   long timeout = FLASH_TIMEOUT;
   unsigned long flashWordOffset = 0;
   unsigned long flashContents;
   unsigned long oldFlashContents;
   const unsigned long hivalue = 0xFFFFl; /* when chip is erased, this is what we should read */
   const unsigned long lovalue = 0xFFFFl; /* when chip is erased, this is what we should read */
   
   nflash_write_cmd(0x555, 0xAA); /*  flashword[0x555] = bothbanks(0xAA); */
   nflash_write_cmd(0x2AA, 0x55); /*  flashword[0x2AA] = bothbanks(0x55); */
   nflash_write_cmd(0x555, 0x80); /*  flashword[0x555] = bothbanks(0x80); */
   nflash_write_cmd(0x555, 0xAA); /*  flashword[0x555] = bothbanks(0xAA); */
   nflash_write_cmd(0x2AA, 0x55); /*  flashword[0x2AA] = bothbanks(0x55); */
   nflash_write_cmd(0x555, 0x10); /*  flashword[0x555] = bothbanks(0x10); */
   while (((flashContents = nflash_read_val(flashWordOffset)) != 0xFFFFFFFFL) &&
          (timeout > 0)) {
      unsigned long hiword = (flashContents >> 16) & 0xFFFFl;
      unsigned long loword = (flashContents >> 0) & 0xFFFFl;
      if (0 && (hiword != hivalue) && (hiword & (1 << 5))) {
         /* programming upper bank of flash timed out */
         putstr("Upper bank of flash timed out!!");
         break;
      }
      if (0 && (loword != lovalue) && (loword & (1 << 5))) {
         /* programming upper bank of flash timed out */
         putstr("Lower bank of flash timed out!!");
         break;
      }
      oldFlashContents = flashContents; /* to check for toggle bits */
      timeout--;
   }
   if (timeout <= 0) {
      putstr("eraseFlashChip timeout\r\n");
      putLabeledWord("  flashp=", (dword)(&flashword[flashWordOffset]));
      putLabeledWord("  flashContents=", flashContents);
      putLabeledWord("  oldFlashContents=", oldFlashContents);
      return(-1);
   }
   return 0;
}

/* sectorAddress must be a valid start of sector address. sectors
   must be erased before they can be programmed! */
static int amdFlashEraseSector (unsigned long sectorAddress)
{
   int i;
   long timeout = FLASH_TIMEOUT;
   unsigned long flashWordOffset = (sectorAddress&flash_address_mask) >> 2;
   unsigned long flashContents;
   unsigned long oldFlashContents;
   const unsigned long hivalue = 0xFFFFl; /* when sector is erased, this is what we should read */
   const unsigned long lovalue = 0xFFFFl; /* when sector is erased, this is what we should read */
   
   for (i = 0; i < nsectors; i++) {
      if (flashSectors[i] == sectorAddress)
         break;
   }
   if (i >= nsectors) {
      putLabeledWord("eraseFlashSector: sectorAddress must be start of a sector! address=", sectorAddress);
      putLabeledWord("nsectors=", nsectors);
      return -EINVAL;
   }
   
   nflash_write_cmd(0x555, 0xAA); /*  flashword[0x555] = bothbanks(0xAA); */
   nflash_write_cmd(0x2AA, 0x55); /*  flashword[0x2AA] = bothbanks(0x55); */
   nflash_write_cmd(0x555, 0x80); /*  flashword[0x555] = bothbanks(0x80); */
   nflash_write_cmd(0x555, 0xAA); /*  flashword[0x555] = bothbanks(0xAA); */
   nflash_write_cmd(0x2AA, 0x55); /*  flashword[0x2AA] = bothbanks(0x55); */
   nflash_write_cmd(flashWordOffset, 0x30); /*  flashword[flashWordOffset] = bothbanks(0x30); */
   while (((flashContents = nflash_read_val(flashWordOffset)) != 0xFFFFFFFFL) &&
          (timeout > 0)) {
      unsigned long hiword = (flashContents >> 16) & 0xFFFFl;
      unsigned long loword = (flashContents >> 0) & 0xFFFFl;
      if (0 && (hiword != hivalue) && (hiword & (1 << 5))) {
         /* programming upper bank of flash timed out */
         putstr("Upper bank of flash timed out!!");
         break;
      }
      if (0 && (loword != lovalue) && (loword & (1 << 5))) {
         /* programming upper bank of flash timed out */
         putstr("Lower bank of flash timed out!!");
         break;
      }
      oldFlashContents = flashContents; /* to check for toggle bits */
      timeout--;
   }
   if (timeout <= 0) {
      putstr("eraseFlashSector timeout\r\n");
      putLabeledWord("  flashp=", (dword)(&flashword[flashWordOffset]));
      putLabeledWord("  flashAddress=", sectorAddress);
      putLabeledWord("  flashContents=", flashContents);
      putLabeledWord("  oldFlashContents=", oldFlashContents);
      putLabeledWord("  timeout=", timeout);
      return(-EIO);
   }
   return 0;
}

static int amdFlashEraseRange(unsigned long startAddress, unsigned long len)
{
  unsigned long lastSectorAddress = flashSectors[0];
  unsigned long limitAddress = startAddress + len;
  unsigned long sectorAddress, i;

  startAddress &= flash_address_mask;
  if (limitAddress >= flashSectors[nsectors])
    limitAddress = flashSectors[nsectors];

  for (i=0;i<nsectors;i++) {
     sectorAddress = flashSectors[i+1]; /* actually nsectors entries in this array -- last is fictitious guard sector */
    if ((lastSectorAddress <= startAddress) && (sectorAddress > startAddress)) {
      putLabeledWord("Erasing sector ",lastSectorAddress);
      if (eraseFlashSector(lastSectorAddress))
	return -EIO;
      len -= (sectorAddress - startAddress);
      startAddress = sectorAddress;
      if (startAddress >= limitAddress)
	break;
    }
    lastSectorAddress = sectorAddress;
  }
  return 0;
}

static int amdFlashProtectRange(unsigned long startAddress, unsigned long len, int protect)
{
   putstr("amdFlashProtectRange: unimplemented!!\r\n");
   return 0;
}
#endif /* CONFIG_AMD_FLASH */

#ifdef CONFIG_INTEL_FLASH

int intelFlashReset ()
{
   /* send flash the reset command */
   /* address does not matter -- only the data */
   flash_write_cmd(0x55, 0xff); /*  flashword[0x55] = bothbanks(0xFF); */
#ifdef CONFIG_IPCOM_32M
   flash_write_cmd(IPCOM_WRITE_OFFSET + 0x55, 0xff);  /* reset the second chip */
#endif
   return 0;
}

/* IPCOM : Added a parameter */
static int intelFlashReadStatus(unsigned long addr)
{
   /* address does not matter */
   unsigned long offset = 0;
#ifdef CONFIG_IPCOM_32M
   offset = (IPCOM_CHIP2_OFFSET & addr) >> 1;
#endif
   flash_write_cmd(0x55 + offset, 0x70); /*  flashword[0x55] = bothbanks(0x70); */
   return flash_read_val(0x55 + offset);    /* flashword[0x55]; */
}

static int intelFlashClearStatus()
{
   /* address does not matter */
   flash_write_cmd(0x55, 0x50); /*  flashword[0x55] = bothbanks(0x50); */
#ifdef CONFIG_IPCOM_32M
   flash_write_cmd(IPCOM_WRITE_OFFSET + 0x55, 0x50);
#endif
   return 0;
}

   /* now wait for it to be programmed */
static int intelFlashWaitforStatus(
    unsigned long addr,
    long*	  timeoutp)
{
   long	timeout = *timeoutp;
   unsigned long    done_status;
   unsigned long    status = 0;

   done_status = flash_make_val(0x80);
   while (timeout > 0) {
      status = flash_read_val(addr); /* XXX ??? */
      if ((status & done_status) == done_status)
         break;
      timeout--;
   }
   *timeoutp = timeout;
   return (status);
}

/*
 * Programs value at flashAddress
 * Sectors must be erased before they can be programmed.
 */
static int intelFlashProgramWord(unsigned long flashAddress, unsigned long value)
{
   unsigned long flashWordOffset = (flashAddress&flash_address_mask) >> 2;
   long timeout = FLASH_TIMEOUT;
   unsigned long flashContents = nflash_read_val(flashWordOffset);
   unsigned long status = 0;

   /* see if we can program the value without erasing */
   if ((flashContents & value) != value) {
      putstr("the flash sector needs to be erased first!\r\n");
      putLabeledWord("  flashAddress=", flashAddress);
      putLabeledWord("  flashWordOffset=", flashWordOffset);
#if 0 /* jca */
      putLabeledWord("  &flashword[flashWordOffset]=", 
		     (dword)&flashword[flashWordOffset]);
#endif
      putLabeledWord("  flashContents=", flashContents);
      return -1;
   }
   
   /* send flash the program word command */
   nflash_write_cmd(0x55, 0x40); /*  flashword[0x55] = bothbanks(0x40); */
   nflash_write_val(flashWordOffset, value);
   /* now wait for it to be programmed */
   while (timeout > 0) {
      status = nflash_read_val(flashWordOffset);
      if ((status & bothbanks(0x80)) == bothbanks(0x80))
         break;
      timeout--;
   }
   intelFlashClearStatus();
   
   if ((timeout <= 0) || (status & bothbanks(0x7f))) {
      putstr("programFlashWord error\r\n");
      putLabeledWord("  flashAddress=", flashAddress);
      putLabeledWord("  value=", value);
      putLabeledWord("  flashContents=", flashContents);
      putLabeledWord("  status=", status);
      return(-1);
   }
   return 0;
}

static int intelFlashProgramShort(unsigned long flashAddress, unsigned short value)
{
   long timeout = FLASH_TIMEOUT;
   unsigned short flashContents = flash_read_array(flashAddress);
   unsigned long status = 0;
   unsigned long offset = 0;  /* IPCOM Addition */

   /* see if we can program the value without erasing */
   if ((flashContents & value) != value) {
      putstr("the flash sector needs to be erased first!\r\n");
      putLabeledWord("  flashAddress=", flashAddress);
      putLabeledWord("  flashContents=", flashContents);
      return -1;
   }
   
#ifdef CONFIG_IPCOM_32M
   offset = (flashAddress & IPCOM_CHIP2_OFFSET) >> 1;
#endif
   /* send flash the program word command */
   flash_write_cmd(offset+0x55, 0x40); /*  flashword[0x55] = bothbanks(0x40); */ /* IPCOM */
   flash_write_val(flashAddress, value); /*  flashword[flashWordOffset] = value; */
   /* now wait for it to be programmed */
   status = intelFlashWaitforStatus(offset + 0, &timeout);  /* IPCOM */
   intelFlashClearStatus();
   
   if ((timeout <= 0) || (status & 0x7f)) {
      putstr("programFlashWord error\r\n");
      putLabeledWord("  flashAddress=", flashAddress);
      putLabeledWord("  value=", value);
      putLabeledWord("  flashContents=", flashContents);
      putLabeledWord("  status=", status);
      return(-1);
   }
   return 0;
}

static int intelFlashProgramWord_1x16(unsigned long flashAddress, unsigned long value)
{
    int	status;

    status = intelFlashProgramShort(flashAddress, value & 0xffff);
    if (status == 0)
	status = intelFlashProgramShort(flashAddress+2, (value>>16)&0xffff);

    return (status);
}
	
/* each flash chip has 32B block, 64B block for chip array */
static int intelFlashProgramBlock(unsigned long flashAddress, unsigned long *values, int nbytes)
{
   unsigned long flashWordOffset = (flashAddress&flash_address_mask) >> 2;
   unsigned long blockOffset = (flashWordOffset & 0xFFFFFFC0);
   int nwords = nbytes >> 2;
   long timeout = FLASH_TIMEOUT;
   unsigned long status = 0;
   int i;

   if (0) putLabeledWord("intelFlashProgramFlashBlock\r\n", flashAddress);

   /* send the "write to buffer" command */
   nflash_write_cmd(flashWordOffset, 0xE8); /*  flashword[flashWordOffset] = bothbanks(0xE8); */
   /* read the extended status register */
   do {
      status = nflash_read_val(flashWordOffset);
      if (0) putLabeledWord("  XSR=", status);
   } while ((status & 0x00800080) != 0x00800080);

   /* write word count at block start address */
   /* wordcount = length minus one */
   nflash_write_cmd(blockOffset, nwords-1); /*  flashword[blockOffset] = bothbanks(nwords-1); */

   /* send the data */
   for (i = 0; i < nwords; i++) {
      nflash_write_val(flashWordOffset+i, values[i]);
      if (0) putLabeledWord(" sr=", nflash_read_val(blockOffset));
   }

   /* send the confirmation to program */
   nflash_write_cmd(blockOffset, 0xD0); /*  flashword[blockOffset] = bothbanks(0xD0); */

   /* now wait for it to be programmed */
   timeout = FLASH_TIMEOUT;
   while (timeout > 0) {
      status = nflash_read_val(blockOffset);
      if (0) putLabeledWord("  status=", status);
      if ((status & bothbanks(0x80)) == bothbanks(0x80))
         break;
      timeout--;
   }
   status = intelFlashReadStatus(blockOffset);
   if (0) putLabeledWord("final status=", status);
   intelFlashClearStatus();
   
   if ((timeout <= 0) || (status & bothbanks(0x7f))) {
      putstr("programFlashBlock error\r\n");
      putLabeledWord("  flashAddress=", flashAddress);
      putLabeledWord("  status=", status);
      return(-1);
   }
   return 0;
}

#define	FLASH_BUFFER_SIZE  (32)

static int intelFlashProgramBlock_1x16(unsigned long flashAddress, unsigned long *long_values, int resid)
{
   unsigned long flashWordOffset = (flashAddress&flash_address_mask) >> 1;
   unsigned long blockOffset = (flashWordOffset & 0xFFFFFFF0);
   int nwords;
   long timeout = FLASH_TIMEOUT;
   unsigned long status = 0;
   int i;
   unsigned short* values = (unsigned short*)long_values;
   int	word_size_shift = flash_addr_shift();
   int	max_words;

#if 0
   putLabeledWord("intelFlashProgramFlashBlock(1x16)\r\n", flashAddress);
#endif   

   nwords = resid >> word_size_shift;
   max_words = FLASH_BUFFER_SIZE >> word_size_shift;
   
   while (nwords) {

       int  num_to_program;

       if (nwords < max_words)
	   num_to_program = nwords;
       else
	   num_to_program = max_words;

       /* send the "write to buffer" command */
       flash_write_cmd(flashWordOffset, 0xe8);
       /* read the extended status register */
       do {
	   status = flash_read_val(flashWordOffset);
#if 0
	   putLabeledWord("  XSR=", status);
#endif
       } while ((status & 0x0080) != 0x0080);
#if 0
       putLabeledWord("final XSR=", status);
#endif

       /* write word count at block start address */
       flash_write_cmd(blockOffset, num_to_program-1);

       /* send the data */
       for (i = 0; i < num_to_program; i++) {
	   flash_write_val_short(flashWordOffset+i, values[i]);
	   if (0) putLabeledWord(" sr=", flash_read_val_short(blockOffset));
       }

       /* send the confirmation to program */
       flash_write_cmd(blockOffset, 0xD0);/*flashshort[blockOffset] = 0xD0; */

       /* now wait for it to be programmed */
       timeout = FLASH_TIMEOUT;
       while (timeout > 0) {
	   status = flash_read_val_short(blockOffset);
	   if (0) putLabeledWord("  status=", status);
	   if ((status & 0x80) == 0x80)
	       break;
	   timeout--;
       }
       status = intelFlashReadStatus(blockOffset);
#if 0
       putLabeledWord("final status=", status);
#endif       
       intelFlashClearStatus();
   
       if ((timeout <= 0) || (status & 0x7f)) {
	   putstr("programFlashBlock error\r\n");
	   putLabeledWord("  flashAddress=", flashAddress);
	   putLabeledWord("  status=", status);
	   return(-1);
       }

       /*
	* prepare for next iteration.
	*/
       nwords -= num_to_program;
       values += num_to_program;
       flashWordOffset += num_to_program;
   }
   
   return 0;
}

int intelFlashEraseChip ()
{
   
   putstr("intelFlashEraseChip unimplemented\r\n");

   return 0;
}

/* sectorAddress must be a valid start of sector address. sectors
   must be erased before they can be programmed! */
static int intelFlashEraseSector (unsigned long sectorAddress)
{
   int i;
   long timeout = FLASH_TIMEOUT;
   unsigned long flashWordOffset = (sectorAddress&flash_address_mask);
   unsigned long flashContents;
   unsigned long status;
   unsigned long done_status;
   unsigned long offset = 0;
#ifdef CONFIG_IPCOM_32M
   offset = (sectorAddress & IPCOM_CHIP2_OFFSET) >> flash_addr_shift();
#endif

   flashWordOffset >>= flash_addr_shift();
   
   for (i = 0; i < nsectors; i++) {
      if (flashSectors[i] == sectorAddress)
         break;
   }
   if (i >= nsectors) {
      putLabeledWord("eraseFlashSector: sectorAddress must be start of a sector! address=", sectorAddress);
      putLabeledWord("nsectors=", nsectors);
      return -EINVAL;
   }

   /* send flash the erase sector command */
   flash_write_cmd(offset+0x55, 0x20); /*  flashword[0x55] = bothbanks(0x20); *//* IPCOM */
   flash_write_cmd(flashWordOffset, 0xd0); /*  flashword[flashWordOffset] = bothbanks(0xD0); */

   /* address doesn't matter */ /* IPCOM : Address does matter */
   status = intelFlashWaitforStatus(0+offset, &timeout);
   
   intelFlashClearStatus();

   /* need to read from divided address (>>2 or >>1) */
   done_status = flash_make_val(0x7f);
#if 0
   putLabeledWord("done_status=0x", done_status);
#endif

   flashContents = flash_read_array(sectorAddress);
   if ((timeout <= 0) || (status & done_status)) {
      putstr("eraseSector error\r\n");
      putLabeledWord("  sectorAddress=", sectorAddress);
      putLabeledWord("  flashContents=", flashContents);
      putLabeledWord("  status=", status);
      putLabeledWord("  timeout=", timeout);
      return(-EIO);
   }   

   return 0;
}

static int intelFlashEraseRange(unsigned long startAddress, unsigned long len)
{
  unsigned long lastSectorAddress = flashSectors[0];
  unsigned long limitAddress = startAddress + len;
  unsigned long sectorAddress, i;

  startAddress &= flash_address_mask;
  if (limitAddress >= flashSectors[nsectors])
    limitAddress = flashSectors[nsectors];

  for (i=0;i<nsectors;i++) {
     sectorAddress = flashSectors[i+1]; /* actually nsectors entries in this array -- last is fictitious guard sector */
    if ((lastSectorAddress <= startAddress) && (sectorAddress > startAddress)) {
      putLabeledWord("Erasing sector ",lastSectorAddress);
      if (eraseFlashSector(lastSectorAddress))
	return -1;
      len -= (sectorAddress - startAddress);
      startAddress = sectorAddress;
      if (startAddress >= limitAddress)
	break;
    }
    lastSectorAddress = sectorAddress;
  }
  return 0;
}

static int intelFlashProtectRange(unsigned long startAddress, unsigned long len, int protect)
{

  /*
    startAddress - the start of the address range to protect
    len          - the len in bytes to protect
    protect      - 1 protect blocks within the address range startAddress + len
    protect      - 0 unprotect all blocks
  */

  unsigned long currentSectorAddress;
  unsigned long limitAddress = startAddress + len;
  unsigned long nextSectorAddress, i;
  int result = 0;
  int status = 0;

  if (protect == 1 ) {
    putLabeledWord("startAddress :", startAddress );
    putLabeledWord("limitAddress :", limitAddress );
    for (i=0;i<nsectors;i++) {
      unsigned long timeout = FLASH_TIMEOUT;
      currentSectorAddress = flashSectors[i];
      nextSectorAddress = flashSectors[i+1]; /* actually nsectors entries in this array -- last is fictitious guard sector */
      if (((currentSectorAddress >= startAddress)
	   || (nextSectorAddress > startAddress))
	  && (currentSectorAddress < limitAddress)) {
	putLabeledWord("Protecting sector ",currentSectorAddress);
	nflash_write_cmd(currentSectorAddress >> 2, 0x60); /*  flashword[currentSectorAddress >> 2] = bothbanks(0x60); */
	nflash_write_cmd(currentSectorAddress >> 2, 0x01); /*  flashword[currentSectorAddress >> 2] = bothbanks(0x01); */
	while (timeout > 0) {
	  status = nflash_read_val(currentSectorAddress >> 2);
	  if ((status & bothbanks(0x80)) == bothbanks(0x80))
	    break;
	  timeout--;
	}
	status = intelFlashReadStatus(currentSectorAddress);
	intelFlashClearStatus();
	/* put back in read mode */
	nflash_write_cmd(currentSectorAddress >> 2, 0xff); /*  flashword[currentSectorAddress >> 2] = bothbanks(0xff); */
 	if (status != bothbanks(0x80)) {
	  putLabeledWord("ERROR - status=", status );         
	  return -EIO;
	}
	putLabeledWord("status=", status); 
	putLabeledWord("Protect=", queryFlash((currentSectorAddress>>2)+2)); 
      }
    }
    result = 0;
  }
  if ( protect == 0) {
    unsigned long timeout = FLASH_TIMEOUT;
    /* unprotects whole chip */

    /*
     * any address will do, but since we also use this to get
     * status after the operation, a BA is required.
     * 0 is easy to type
     */
    currentSectorAddress = 0;
    
	nflash_write_cmd(currentSectorAddress >> 2, 0x60); /*  flashword[currentSectorAddress >> 2] = bothbanks(0x60); */
	nflash_write_cmd(currentSectorAddress >> 2, 0xd0); /*  flashword[currentSectorAddress >> 2] = bothbanks(0xd0); */
   /* now wait for it to be complete */
    while (timeout > 0) {
      status = nflash_read_val(currentSectorAddress >> 2);
      if ((status & bothbanks(0x80)) == bothbanks(0x80))
         break;
      timeout--;
    }
    status = intelFlashReadStatus(currentSectorAddress);
    if (status != bothbanks(0x80))
      putLabeledWord("status=", status );         
    result = -EIO;
    intelFlashClearStatus();
    /* put back in read mode */
	nflash_write_cmd(currentSectorAddress >> 2, 0xff); /*  flashword[currentSectorAddress >> 2] = bothbanks(0xff); */
    putLabeledWord("Protect=", queryFlash((currentSectorAddress>>2)+2)); 
  }
  return result;
}

/*
 * XXX this'll probably work for 2x16, too.
 * or should be able to do so easily
 */
static int intelFlashProtectRange_1x16(unsigned long startAddress, unsigned long len, int protect)
{

  /*
    startAddress - the start of the address range to protect
    len          - the len in bytes to protect
    protect      - 1 protect blocks within the address range startAddress + len
    protect      - 0 unprotect all blocks
  */

  unsigned long currentSectorAddress;
  unsigned long limitAddress = startAddress + len;
  unsigned long nextSectorAddress, i;
  int result = 0;
  int status = 0;
  int done_status;
#if 0 /* jca */
  unsigned short* flashshort = (unsigned short*)flashword;
#endif

  if (protect == 1 ) {
    putLabeledWord("(1x16)startAddress :", startAddress );
    putLabeledWord("(1x16)limitAddress :", limitAddress );
    for (i=0;i<nsectors;i++) {
      unsigned long timeout = FLASH_TIMEOUT;
      currentSectorAddress = flashSectors[i];
      nextSectorAddress = flashSectors[i+1]; /* actually nsectors entries in this array -- last is fictitious guard sector */
      if (((currentSectorAddress >= startAddress)
	   || (nextSectorAddress > startAddress))
	  && (currentSectorAddress < limitAddress)) {
	putLabeledWord("Protecting sector ",currentSectorAddress);
#if 0
	putLabeledWord("fp(1x16), Addr=0x",
		       (unsigned long)&flashshort[currentSectorAddress >> 1]);
#endif
	flash_write_cmd(currentSectorAddress >> 1, 0x60);	
	flash_write_cmd(currentSectorAddress >> 1, 0x01);
	while (timeout > 0) {
	  status = flash_read_val_short(currentSectorAddress >> 1);
	  if ((status & 0x80) == 0x80)
	    break;
	  timeout--;
	}
#if 0
	putLabeledWord("status=", status);
	putLabeledWord("timeout=", timeout);
#endif
	status = intelFlashReadStatus(currentSectorAddress);
	intelFlashClearStatus();
#if 0
	putLabeledWord("status=", status);
#endif
	/* put back in read mode */
	flash_write_val_short(currentSectorAddress >> 1, 0xff);
 	if (status != 0x80) {
	  putLabeledWord("ERROR - status :", status );         
	  return status;
	}
	putLabeledWord("Protect=", queryFlash((currentSectorAddress>>1)+2)); 
      }
    }
    result = 0;
  }
  if ( protect == 0) {
    unsigned long timeout = FLASH_TIMEOUT;
    /* unprotects whole chip */

    /*
     * any address will do, but since we also use this to get
     * status after the operation, a BA is required.
     * 0 is easy to type
     */
    /* IPCOM Any address won't do !!!! */
    currentSectorAddress = startAddress;
    
    flash_write_cmd(currentSectorAddress >> 1, 0x60);	
    flash_write_cmd(currentSectorAddress >> 1, 0xd0);
   /* now wait for it to be complete */
    while (timeout > 0) {
      status = flash_read_val_short(currentSectorAddress >> 1);
      if ((status & 0x80) == 0x80)
         break;
      timeout--;
    }
    
    done_status = flash_make_val(0x80);
    status = intelFlashReadStatus(currentSectorAddress);
    if (status != done_status)
      putLabeledWord("status :", status );         
    result = status;
    intelFlashClearStatus();
    /* put back in read mode */
    flash_write_val_short(currentSectorAddress >> 1, 0xff);

    putLabeledWord("Protect=", queryFlash((currentSectorAddress>>1)+2)); 
  }
  return result;
}

#endif /* CONFIG_INTEL_FLASH */


void btflash_print_types()
{
  int i;
  putstr("Flash types supported (2x16):\r\n");
  for (i = 0; flashDescriptors[i] != NULL; i++) {
    FlashDescriptor *fd = flashDescriptors[i];
    putstr("  "); putstr(fd->deviceName); putstr("\r\n");
  }
  putstr("Flash types supported (1x16):\r\n");
  for (i = 0; flashDescriptors_1x16[i] != NULL; i++) {
    FlashDescriptor *fd = flashDescriptors_1x16[i];
    putstr("  "); putstr(fd->deviceName); putstr("\r\n");
  }
  putstr("Current flash type is "); 
  if ((flashDescriptor != NULL) &&
      (flashDescriptor->deviceName != NULL)) {
    putstr(flashDescriptor->deviceName);
    print_mem_size("\n\rFlash size: ", flash_size);
  }
  else
    putstr("UNKNOWN!\r\n");
}

void btflash_set_type(const char *devicename)
{
  int i;
  if (bt_flash_organization == BT_FLASH_ORGANIZATION_2x16) {
    for (i = 0; flashDescriptors[i] != NULL; i++) {
      FlashDescriptor *fd = flashDescriptors[i];
      if (strcmp(fd->deviceName, devicename) == 0) {
	putstr("setting flash type="); putstr(flashDescriptor->deviceName); putstr("\r\n");
	flashDescriptor = fd;

	nsectors = flashDescriptor->nsectors;
	flashSectors = flashDescriptor->sectors;
	flash_size = flashSectors[nsectors];
	flash_address_mask = flash_size-1;

	return;
      }
    }
  } else {
    for (i = 0; flashDescriptors_1x16[i] != NULL; i++) {
      FlashDescriptor *fd = flashDescriptors_1x16[i];
      if (strcmp(fd->deviceName, devicename) == 0) {
	putstr("setting flash type="); putstr(flashDescriptor->deviceName); putstr("\r\n");
	flashDescriptor = fd;

	nsectors = flashDescriptor->nsectors;
	flashSectors = flashDescriptor->sectors;
	flash_size = flashSectors[nsectors];
	flash_address_mask = flash_size-1;

	return;
      }
    }
  }
  putstr("  Unknown flash device type: "); putstr(devicename); 
  putstr(" for flash organization: "); putstr(bt_flash_organization == BT_FLASH_ORGANIZATION_2x16 ? "2x16" : "1x16");
  putstr("\r\n");
}



struct BootldrFlashPartitionTable *partition_table = NULL;

void btflash_reset_partitions(void)
{
    unsigned long mach_type = MACH_TYPE;
    struct FlashRegion *rootP;
    
    putLabeledWord("reset_partitions: partition_table = 0x",partition_table);
    
    if (partition_table != NULL) {
	partition_table->npartitions = 0;
    }
    putLabeledWord("reset_partitions: flashDescriptor = 0x",flashDescriptor);
    if (flashDescriptor != NULL) {
	btflash_define_partition(flashDescriptor->bootldr.name, 
				 flashDescriptor->bootldr.base,
				 flashDescriptor->bootldr.size,
				 flashDescriptor->bootldr.flags);   
	// we are dropping the params sector
#if 0
	btflash_define_partition(flashDescriptor->params.name, 
				 flashDescriptor->params.base,
				 flashDescriptor->params.size,
				 flashDescriptor->params.flags);   
#endif


	btflash_define_partition(flashDescriptor->part3.name, 
				 flashDescriptor->part3.base,
				 flashDescriptor->part3.size,
				 flashDescriptor->part3.flags);


        mach_type = param_mach_type.value;
	if ((mach_type == MACH_TYPE_H3800) || (mach_type == MACH_TYPE_H3900)){
	    rootP = btflash_get_partition(flashDescriptor->part3.name);
	    if (!rootP){
		putstr(" get root partition failed\r\n");
		return;
	    }
	    // make a 3800 specific asset partition
	    btflash_define_partition("asset", 
				 rootP->base + rootP->size,
				 256*1024,
				 0x0);
	    
	}
    }
}

struct FlashRegion *btflash_get_partition(const char *region_name)
{
  if (partition_table != NULL) {
    int i;
    for (i = 0; i < partition_table->npartitions; i++) {
      if (strcmp(region_name, partition_table->partition[i].name) == 0) {
        return &partition_table->partition[i];
      }
    }
    return NULL;
  } else {
    return NULL;
  }
}

unsigned long
partition_next_base(
    void)
{
    unsigned long   tnext = 0;
    unsigned long   next = 0;
    
    if (partition_table != NULL) {
	int i;
	for (i = 0; i < partition_table->npartitions; i++) {
	    tnext = partition_table->partition[i].base +
		partition_table->partition[i].size;
	    if (tnext > next) {
		next = tnext;
		/*  putLabeledWord("next: 0x", next); */
	    }
	}
    }
    
    return (next);
}
void btflash_define_partition(const char *partition_name, 
                              unsigned long base, unsigned long size,
			      enum LFR_FLAGS flags)
{
  struct FlashRegion *partition;
  int i;
  
  putLabeledWord("define_partition: base 0x", base);
  putLabeledWord("define_partition: partition_table 0x", partition_table); 
  if (partition_table == NULL) {
    partition_table = (BootldrFlashPartitionTable *)mmalloc(sizeof(BootldrFlashPartitionTable) + 10 * sizeof(FlashRegion));
    if (partition_table == NULL) {
      putstr("btflash_add_partition: could not allocate memory for partition table\r\n");
      return;
    }
    putstr("allocated partition_table\r\n");
    partition_table->magic = BOOTLDR_PARTITION_MAGIC;
    partition_table->npartitions = 0;
  }
  if (base == 0xffffffff)
      base = partition_next_base();
  
  partition = btflash_get_partition(partition_name);
  //
  for (i=partition_table->npartitions-1; i >0 ; i--){
      if (base <  partition_table->partition[i].base){
	  // move it up
	  partition_table->partition[i+1] = partition_table->partition[i];
	  partition = &partition_table->partition[i];
      }
  }
  if (partition == NULL) {
    partition = &partition_table->partition[partition_table->npartitions];
  }
  partition_table->npartitions++;
  
  putstr("defining partition: "); putstr(partition_name); putstr("\r\n");
  memset(partition->name, 0, FLASH_PARTITION_NAMELEN);
  if (strlen(partition_name) < FLASH_PARTITION_NAMELEN) {
    strcpy(&partition->name[0], partition_name);
  } else {
    strncpy(&partition->name[0], partition_name, FLASH_PARTITION_NAMELEN);
    partition->name[FLASH_PARTITION_NAMELEN-1] = 0;
  }
  partition->base = base;
  partition->size = size;
  partition->flags = flags;
  if (partition->flags & LFR_EXPAND) {
      unsigned long mach_type = param_mach_type.value;
      //3800 is different, we wanna keep the asset information intact for now
      if ((mach_type == MACH_TYPE_H3800) || (mach_type == MACH_TYPE_H3900))
	  partition->size = flash_size - base - 256 * 1024;	  
      else
	  partition->size = flash_size - base;
  }
}

void btflash_delete_partition(const char *partition_name)                               
{
  struct FlashRegion *partition;
  int i;
  int jPart = -1;
  
  if (partition_table == NULL) {
      putstr("no partition table!\r\n");
      return;
  }
  
  partition = btflash_get_partition(partition_name);
  for (i=0; i < partition_table->npartitions; i++){
      FlashRegion *partition = &partition_table->partition[i];
      if (!strcmp(partition_name,&partition->name[0]))
	  jPart = i;
  }
  
  
  if (jPart < 0) {
      putstr("no partition <");
      putstr(partition_name);
      putstr("> found\r\n");
      return;
  }

  for (i=jPart+1;i < partition_table->npartitions;i++){
      memcpy(&partition_table->partition[i-1],&partition_table->partition[i],sizeof(FlashRegion));
  }

  putstr("partition <");
  putstr(partition_name);
  putstr("> deleted\r\n");
  partition_table->npartitions--;
  
  
}


/*
 * read the CFI string and ensure that it looks like it should
 * given our flash size deduction.
 */
int
verify_flash_size(
    void)
{
    unsigned long   query_datum;
    char*	    query_string = "QRY";
    int		    i;
    int		    query_base = 0x10;
    int		    stat;

    for (i = 0; i < 3; i++) {
	stat = flash_make_val(query_string[i]);
	query_datum = queryFlash(query_base + i);
	if (stat != query_datum)
	    return (0);
    }
    return (1);
}


void btflash_init()
{
   int algorithm;
   int mfrid;
   int devid;
   int protected;
   FlashDescriptor **fd;
   char* desc_type = "NONE";

   /*
    * for now, we assume flash being 16 bit wide --> monochrome iPAQ
    */
   if (is_flash_16bit()) {
       //jca moved to is_flash_16bit SET_BTF_1x16();
       fd = &flashDescriptors_1x16[0];
       desc_type = "1x16";
   }
   else {
      fd = &flashDescriptors[0];
      desc_type = "2x16";
   }

   putstr(" Flash style = ");
   putstr(desc_type);
   putstr("\r\n");

   algorithm = updateFlashAlgorithm();

   putstr("verify flash size...\r\n");
   if (!verify_flash_size()) {
       putstr("verify flash size failed.\r\n");
       return;
   }
   
   mfrid = queryFlashID(0x00);
   devid = queryFlashID(0x01);
   protected = queryFlashID(0x02);
   
   putLabeledWord("btflash_init: mfrid=", mfrid);
   putLabeledWord("  devid=", devid);
   if ((devid&0xFFFF) != ((devid>>16)&0xFFFF)) {
#ifndef linux
      struct bootblk_param *noeraseParam = (void *)param_noerase.value;
      putLabeledWord("\r\n\r\n@@@@@@@@ mismatched flash parts @@@@@@@@\r\n\r\n", devid);
      if (noeraseParam != NULL)
         noeraseParam->value = 1;
#endif
      flashDescriptor = fd[0];
   }
   putLabeledWord("mfrid=", mfrid);
   putLabeledWord("deviceid=", devid);

   putstr("walking flash descriptors (");
   putstr(desc_type);
   putstr(")\r\n");
   for (flashDescriptor = *fd; 
        (flashDescriptor != NULL); 
        fd++, flashDescriptor = *fd) {
#if 0
	putstr("btflash_init: trying flash_type "); putstr(flashDescriptor->deviceName); putstr("\r\n");
	putLabeledWord("        mfrid=", flashDescriptor->manufacturerID);
	putLabeledWord("     deviceid=", flashDescriptor->deviceID);
#endif
      if (flashDescriptor->manufacturerID == mfrid
          && flashDescriptor->deviceID == devid) {
         putstr("btflash_init: found flash "); putstr(flashDescriptor->deviceName); putstr("\r\n");
         nsectors = flashDescriptor->nsectors;
         flashSectors = flashDescriptor->sectors;
         flash_size = flashSectors[nsectors];
         flash_address_mask = flash_size-1;

         putLabeledWord("  flashDescriptor=", (long)flashDescriptor);
         putLabeledWord("  flashSectors=", (long)flashSectors);
         putLabeledWord("  nsectors=", nsectors);
         putLabeledWord("  flash_size=", flash_size);
         putLabeledWord("  flash_address_mask=", flash_address_mask);
         break;
      }
   }
   if (flashDescriptor == NULL) {
     putstr("btflash_init: FAILED! Unknown flash type!\r\n");
     return;
   }
#ifdef CONFIG_PROTECT_BOOTLDR  /* not needed on assabet or platforms with JTAG */
   if (!protected) {
     putstr("protecting the bootldr\r\n");
     set_vppen();
     protectFlashRange(flashDescriptor->bootldr.base, flashDescriptor->bootldr.size, 1);
     clr_vppen();
   }
#else
   putstr("not protecting the bootldr\r\n");
#endif
}



void btflash_jffs2_format_region(unsigned long base, unsigned long size, 
                                 unsigned long marker0, unsigned long marker1, unsigned long marker2)
{
  int i;
  unsigned long limit = base + size - 12; /* leave room for 12 byte mark */
  for (i = 0; i < nsectors; i++) {
    unsigned long sectorAddress = flashSectors[i];
    if (sectorAddress >= base && sectorAddress < limit) {
      /* mark this sector for JFFS2 */
      programFlashWord(sectorAddress + 0, marker0);
      programFlashWord(sectorAddress + 4, marker1);
      programFlashWord(sectorAddress + 8, marker2);
    }
  }

}
