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
 * btflash.h - Flash erase and program routines for Compaq Personal Server Monitor
 *
 */

#include "bootconfig.h"
#ifndef BOOTLDR
#error
#include <stdio.h>
typedef unsigned long dword;
#define putstr(x) fprintf(stderr, (x))
#define putLabeledWord(str, v) fprintf(stderr, "%s%08x\n", (str), (v))
#define get_param(name) NULL
#else
/* bootldrd specific includes and defines here */
#include "bootldr.h"
#endif

/* two 16bit parts in parallel (original iPAQ) */
#define	BT_FLASH_ORGANIZATION_2x16	(1)

/* one single 16bit part (B&W iPAQ) */
#define	BT_FLASH_ORGANIZATION_1x16	(2)

#define	BTF_2x16()  (bt_flash_organization == BT_FLASH_ORGANIZATION_2x16)
#define	BTF_1x16()  (bt_flash_organization == BT_FLASH_ORGANIZATION_1x16)

#define	SET_BTF_2x16()  (bt_flash_organization = BT_FLASH_ORGANIZATION_2x16)
#define	SET_BTF_1x16()  (bt_flash_organization = BT_FLASH_ORGANIZATION_1x16)

extern int  bt_flash_organization;

extern unsigned long flash_size;
extern unsigned long flash_address_mask;
extern volatile unsigned long *flashword; 

#include "partition.h"

typedef struct {
   int (*reset)(void);
   int (*programWord)(unsigned long flashAddress, unsigned long value);
   int (*programBlock)(unsigned long flashAddress, unsigned long *values, int nbytes);
   int (*eraseChip)(void);
   int (*eraseSector)(unsigned long sectorAddress);
   int (*eraseRange)(unsigned long start, unsigned long len);
   int (*protectRange)(unsigned long start, unsigned long len, int protect);
} FlashAlgorithm;

typedef struct FlashDescriptor {
   char *deviceName;
   int manufacturerID;
   int deviceID;
   FlashAlgorithm *algorithm;
   int nsectors;
   unsigned long *sectors;
   struct FlashRegion bootldr;
    //struct FlashRegion params;
    struct FlashRegion part3;// sometimes kernel, sometimes root depending
} FlashDescriptor;

extern int nsectors;
extern FlashDescriptor *flashDescriptor;
extern unsigned long *flashSectors;


/*
 * btflash_init()
 *
 * Query flash to determine type and then initialize sector and region data structures appropriately.
 */
void btflash_init();

/**
 * reset the flash chip state machine
 */
int resetFlash(void);

enum FlashType {
   FT_Am29LV160BB,
   FT_Am29DL323CT
};
void setFlashType(enum FlashType);

/* returns 0 on success */
int programFlashWord(unsigned long flashAddress, unsigned long value);

/*
 * Programs block of values starting at flashAddress
 * Sectors must be erased before they can be programmed.
 */
int programFlashBlock(unsigned long flashAddress, unsigned long *values, int nbytes);

/*
 * returns 0 on success
*/
int eraseFlashChip (void);

/*
 * assumes sectorAddress is a valid start of sector address 
 * returns 0 on success
*/
int eraseFlashSector (unsigned long sectorAddress);

/*
 * erases a range of flash
 * returns 0 on success
 */
int eraseFlashRange(unsigned long start, unsigned long len);

/*
 * Puts flash in query mode, reads flashWordAddress, resets flash, and returns result.
 */
unsigned long queryFlash(unsigned long flashWordAddress);

/*
 * Puts flash in CFI ID mode, reads flashWordAddress, resets flash, and returns result.
 */
unsigned long queryFlashID(unsigned long flashWordAddress);

/*
 * Puts flash in security sector mode, reads flashWordAddress, resets flash, and returns result.
 */
unsigned long queryFlashSecurity(unsigned long flashWordAddress);

/*
 * Protects or unprotects flash range and returns non-zero on error
 */
int protectFlashRange(unsigned long flashWordAddress, unsigned long len, int protect);

/*
 * verified that the memory range is inside of flash 
 */
int verifiedFlashRange( unsigned long addr, unsigned long len );

void set_vppen(void);
void clr_vppen(void);

void btflash_print_types(void);
void btflash_set_type(const char *devicename);

int is_flash_16bit(void);

extern BootldrFlashPartitionTable *partition_table;
void btflash_reset_partitions(void);
struct FlashRegion *btflash_get_partition(const char *region_partition_name); 
void btflash_define_partition(const char *partition_name, 
                              unsigned long base, unsigned long size, enum LFR_FLAGS flags);
void btflash_delete_partition(const char *partition_name); 

void btflash_jffs2_format_region(unsigned long base, unsigned long size, 
                                 unsigned long marker0, unsigned long marker1, unsigned long marker2);

extern int btflash_verbose;

/* existing functions for flash primitives */
void set_vppen();
void clr_vppen();
int flash_addr_shift( void);
unsigned long flash_make_val( unsigned long inval);
void flash_write_cmd( unsigned cmd_addr, int cmd);
unsigned long flash_read_val( unsigned addr);
unsigned long flash_read_array( unsigned addr);
void flash_write_val( unsigned addr, int val);
/* new functions for flash primitives */
void nflash_write_cmd(unsigned long addr, unsigned long val);
void nflash_write_val( unsigned addr, int val);
unsigned long nflash_read_val( unsigned addr);
void flash_write_val_short( unsigned addr, int val);
unsigned long flash_read_val_short( unsigned addr);
