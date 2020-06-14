#ifndef PARTITION_H_INCLUDED
#define PARTITION_H_INCLUDED

#define BOOTLDR_PARTITION_MAGIC  0x646c7470  /* btlp: marks a valid bootldr partition table in params sector */

enum LFR_FLAGS {
   LFR_SIZE_PREFIX = 1,		/* prefix data with 4-byte size */
   LFR_BOOTLDR = 2,	        /* patch bootloader's 0th instruction */
   LFR_KERNEL = 4,		/* add BOOTIMG_MAGIC, imgsize and VKERNEL_BASE to head of programmed region (see bootldr.c) */
   LFR_EXPAND = 8,              /* expand partition size to fit rest of flash */
   LFR_JFFS2 = 16               /* erase whole partition, format unwritten sectors so JFFS2 won't re-erase and format */
#if defined(CONFIG_NAND)
   ,LFR_NAND = 32               /* found in a NAND chip not bootable NOR */
#if defined(CONFIG_YAFFS)
   ,LFR_NAND_YAFFS = 64         /* write yaffs images */
#endif
#endif
};

#define FLASH_PARTITION_NAMELEN 32
typedef struct FlashRegion {
   char name[FLASH_PARTITION_NAMELEN];
   unsigned long base;
   unsigned long size;
   unsigned long  flags;
} FlashRegion;

typedef struct BootldrFlashPartitionTable {
  int magic; /* should be filled with 0x646c7470 (btlp) BOOTLDR_PARTITION_MAGIC */
  int npartitions;
  struct FlashRegion partition[0];
} BootldrFlashPartitionTable;

#endif
