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

#include "bootldr.h"
#include "btflash.h"
#include "bootconfig.h"
#include "jffs.h"

  /*
   * We just set up a linear virtual -> physical mapping
   *   After evacuating the bootldr from Flash, we will use
   *   the fourth to last 1MB of DRAM to hold a cache-enabled image of the first 1MB Flash
   */

unsigned long *mmu_table = (unsigned long *) MMU_TABLE_START;

#define EVACUATE_TO_DRAM

extern void *_start;

/*
 * a flag to let us know variables are in RAM
 */

//
// cdm - We don't even use this? 
//
// static unsigned long boot_flags_ram = 0;
//


void bootConfigureMMU(void)
{
  unsigned long pageoffset;
  unsigned long sectionNumber;

#ifdef EVACUATE_TO_DRAM
  putLabeledWord("FLASH_BASE=", (long)FLASH_BASE);
  if ((char*)&_start == (char*)FLASH_BASE) {
    /* copy ourselves out of flash into the 2nd-to-last megabyte of DRAM */
    putLabeledWord(" Evacuating 1MB of Flash to DRAM at: ", FLASH_IMG_START);
    memcpy((void *)FLASH_IMG_START, (void *)FLASH_BASE, SZ_1M);
    putstr("done\r\n");
  }
#endif

  for (sectionNumber = 0x000; sectionNumber <= 0xfff; sectionNumber++) {
    pageoffset = (sectionNumber << 20);
    mmu_table[pageoffset >> 20] = pageoffset | MMU_SECDESC;
  }

  /* create 1MB uncached flash mapping */
  mmu_table[UNCACHED_FLASH_BASE >> 20] = FLASH_BASE | MMU_SECDESC;
  /* at this point, we should swing the flashword pointer to
   * UNCACHED_FLASH_BASE, but the bootldr is still running from flash,
   * so writing to the variable has no effect.  We have to wait until
   * after the MMU is enabled. -Jamey 9/4/2000. 
   */

  /* make dram cacheable and bufferable */
  for (pageoffset = DRAM_BASE0; pageoffset < (DRAM_BASE0+DRAM_SIZE0); pageoffset += SZ_1M) {
    if (0) putLabeledWord("Make DRAM section cacheable: ", pageoffset);
    mmu_table[pageoffset >> 20] = pageoffset | MMU_SECDESC | 
      MMU_CACHEABLE | MMU_BUFFERABLE;
  }

  /* make cache-flush space cacheable and bufferable */
  for (pageoffset = CACHE_FLUSH_BASE;
       pageoffset < (CACHE_FLUSH_BASE + CACHE_FLUSH_SIZE);
       pageoffset += SZ_1M)
    mmu_table[pageoffset >> 20] = pageoffset | MMU_SECDESC | 
      MMU_CACHEABLE | MMU_BUFFERABLE;

#ifdef EVACUATE_TO_DRAM
  putLabeledWord("Map Flash virtual section to DRAM at: ", FLASH_IMG_START);
  /* point first 1MB of flash virtual addresses to its cacheable image in DRAM */
  mmu_table[FLASH_BASE >> 20] = FLASH_IMG_START | MMU_SECDESC | MMU_CACHEABLE;
#endif

  writeBackDcache(CACHE_FLUSH_BASE);
  flushTLB();
}

void flashConfigureMMU(unsigned long flash_size)
{
   unsigned long pageoffset;
   /* Make flash cacheable
    */
   for (pageoffset = 0; pageoffset < flash_size; pageoffset += SZ_1M) {
      unsigned long cached_flash_addr = FLASH_BASE + pageoffset;
      unsigned long uncached_flash_addr = UNCACHED_FLASH_BASE + pageoffset;
#ifdef EVACUATE_TO_DRAM
      if (cached_flash_addr != FLASH_BASE) {
#endif
         mmu_table[cached_flash_addr >> 20] = cached_flash_addr | MMU_SECDESC | MMU_CACHEABLE;
#ifdef EVACUATE_TO_DRAM
      }
#endif
      mmu_table[uncached_flash_addr >> 20] = cached_flash_addr | MMU_SECDESC;
   }

#ifdef EVACUATE_TO_DRAM
  /* point first 1MB of flash virtual addresses to its cacheable image in DRAM */
  mmu_table[FLASH_BASE >> 20] = FLASH_IMG_START | MMU_SECDESC | MMU_CACHEABLE;

#endif

  writeBackDcache(CACHE_FLUSH_BASE);
  flushTLB();

}

unsigned long
vaddr_to_paddr(
    unsigned long   vaddr)
{
    unsigned long   l1_entry = mmu_table[vaddr>>20];
    
    return (l1_entry & 0xfff00000) | (vaddr & ~0xfff00000);
}

    
