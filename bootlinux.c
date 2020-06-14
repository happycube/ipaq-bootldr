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
 * support for booting linux
 *
 */

/*
 * Maintainer: Jamey Hicks (jamey@crl.dec.com)
 */

#include "commands.h"
#include "bootldr.h"
#include "btflash.h"
#include "bootlinux.h"
#include "heap.h"
#include "params.h"
#include "cpu.h"
#include "util.h"

#include <string.h>
#ifdef __linux__
#include <mtd-bootldr.h>
#include <asm-arm/setup.h>
#endif

#ifdef __linux__
void setup_ramdisk_from_flash(void)
{
  const char *initrd_partition_name = (const char *)param_initrd_partition.value;
  struct FlashRegion *initrd_partition = btflash_get_partition(initrd_partition_name);

  if (initrd_partition) {
    param_use_initrd.value = 1;
    param_copy_initrd.value = 1;
    param_initrd_start.value = initrd_partition->base + FLASH_BASE;
    param_initrd_size.value = initrd_partition->size;
  }
}

INTERNAL_PARAM(use_ptable, PT_INT, PF_DECIMAL, 1, NULL);
INTERNAL_PARAM(use_mtd_cmdline, PT_INT, PF_DECIMAL, 1, NULL);
INTERNAL_PARAM(rootfstype, PT_STRING, PF_STRING, (long)"jffs2", NULL);

static void fixup_dram_bank1(void)
{
#if !defined(CONFIG_PXA)
  if (param_dram_n_banks.value < 2) {
    unsigned long mdcnfg = ABS_MDCNFG;
    putLabeledWord("mdcnfg=", mdcnfg);
    /* turn off bank 1 if it is enabled but there is no dram there */
    if (mdcnfg & MDCNFG_BANK1_ENABLE) {
      mdcnfg &= ~MDCNFG_BANK1_ENABLE;
      ABS_MDCNFG = mdcnfg;
    }
  }
#endif
}

void setup_linux_params(long bootimg_dest, long memc_ctrl_reg, const char *cmdline)
{
  int rootdev = 0x00ff;
  struct tag *tag;
  int i;
  int npartitions = partition_table->npartitions;
  int newcmdlinelen = 0;
  char *newcmdline = NULL;

  fixup_dram_bank1();

  // add the partition table to the commandline
  newcmdlinelen = strlen(cmdline) + 128 + 64*npartitions;
  newcmdline = mmalloc(newcmdlinelen);
  memset(newcmdline, 0, newcmdlinelen);
#if defined(CONFIG_MACH_SKIFF)
  strcpy(newcmdline, "mtdparts=dc21285:");
#else
  strcpy(newcmdline, "mtdparts=ipaq:");
#endif // defined(CONFIG_MACH_SKIFF)
  for (i = 0; i < npartitions; i++) {
    struct FlashRegion *partition = &partition_table->partition[i];
    if (i != 0)
      strcat(newcmdline, ",");
    strcat(newcmdline, "0x");
    binarytohex(newcmdline+strlen(newcmdline), partition->size, 4);
    strcat(newcmdline, "@0x");
    binarytohex(newcmdline+strlen(newcmdline), partition->base, 4);
    strcat(newcmdline, "(");
    strcat(newcmdline, partition->name);
    strcat(newcmdline, ")");
    if (partition->flags & LFR_BOOTLDR) {
      strcat(newcmdline, "ro");
    }
  }
  strcat(newcmdline, " ");
  strcat(newcmdline, cmdline);

  strcat(newcmdline, " ");
  strcat(newcmdline, " rootfstype=");
  strcat(newcmdline, (char *)param_rootfstype.value);
  strcat(newcmdline, " ");

  cmdline = newcmdline;

  // start with the core tag       
  tag = (struct tag *)(bootimg_dest + 0x100);
   
  putLabeledWord("Making core tag at ",(unsigned long) tag);
   
  tag->hdr.tag = ATAG_CORE;
  tag->hdr.size = tag_size(tag_core);
  tag->u.core.flags =0;
  tag->u.core.pagesize = LINUX_PAGE_SIZE;
  tag->u.core.rootdev = rootdev;
  tag = tag_next(tag);

  // now the cmdline tag
  putLabeledWord("Making cmdline tag at ",(unsigned long) tag);
  tag->hdr.tag = ATAG_CMDLINE;
  // must be at least +3!! 1 for the null and 2 for the ???
  tag->hdr.size = (strlen(cmdline) + 3 + sizeof(struct tag_header)) >> 2;
  //tag->hdr.size = (strlen(cmdline) + 10 + sizeof(struct tag_header)) >> 2;
  strcpy(tag->u.cmdline.cmdline,cmdline);
  tag = tag_next(tag);

  // now the mem32 tag
  putLabeledWord("Making mem32 tag at ",(unsigned long) tag);
  tag->hdr.tag = ATAG_MEM;
  tag->hdr.size = tag_size(tag_mem32);
  tag->u.mem.size = param_dram0_size.value;
  tag->u.mem.start = DRAM_BASE0;
  tag = tag_next(tag);

#if !defined(CONFIG_MACH_SKIFF)
  if (param_dram_n_banks.value > 1) {
    // now the mem32 tag
    putLabeledWord("Making mem32 tag at ",(unsigned long) tag);
    tag->hdr.tag = ATAG_MEM;
    tag->hdr.size = tag_size(tag_mem32);
    tag->u.mem.size = param_dram1_size.value;
    tag->u.mem.start = DRAM_BASE1;
    tag = tag_next(tag);
  }
#endif // CONFIG_MACH_SKIFF
  
  /* and now the initrd tag */
  if (param_use_initrd.value) {
    putLabeledWord("Making initrd tag at ",(unsigned long) tag);
    putLabeledWord("  initrd.start=", param_initrd_start.value);
    putLabeledWord("  initrd.size=", param_initrd_size.value);
    tag->hdr.tag = ATAG_INITRD2;
    tag->hdr.size = tag_size(tag_initrd);
    tag->u.initrd.start = param_initrd_start.value;
    tag->u.initrd.size = param_initrd_size.value;
    tag = tag_next(tag);
  }

  // now the NULL tag
  tag->hdr.tag = ATAG_NONE;
  tag->hdr.size = 0;
       
  putstr("command line is: ");
  putstr(cmdline);
  putstr("\r\n");
}
#endif
