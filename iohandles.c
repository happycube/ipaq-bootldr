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
 * iohandles, copy command and friends
 */

#include "commands.h"
#include "bootldr.h"
#include "btflash.h"
#include "hal.h"
#include "h3600_sleeve.h"
#include "pcmcia.h"
#include "ide.h"
#include "fs.h"
#include "vfat.h"
#include "heap.h"
#include "params.h"
#include "zlib.h"
#include "zUtils.h"

#include <errno.h>
#include <string.h>

#define BUFSIZE 1024



struct flash_iohandle {
  u32 base;
  u32 size;
  u32 flags;
};



static int flash_iohandle_len(struct iohandle *ioh)
{
  if (ioh) {
    struct FlashRegion *partition = (struct FlashRegion *)ioh->pdata;
    return partition->size;
  } else {
    return 0;
  }
}

static int flash_iohandle_read(struct iohandle *ioh, char *buf, size_t offset, size_t len)
{
  if (ioh) {
    struct FlashRegion *partition = (struct FlashRegion *)ioh->pdata;
    if (!len || len > partition->size)
      len = partition->size;
    memcpy(buf, ((char *)flashword) + partition->base + offset, len);
    return len;
  } else {
    return -EINVAL;
  }
}

static int flash_iohandle_prewrite(struct iohandle *ioh, size_t offset, size_t len)
{
  if (ioh) {
    int err = 0;
    struct FlashRegion *partition = (struct FlashRegion *)ioh->pdata;
    int available_len = partition->size - offset;
    if (len > available_len)
      return -ENOSPC;

    set_vppen();
    if ((partition->flags & LFR_BOOTLDR) && (offset == 0)) {
      /* this does not return a useful error value, so we will not check for it */
      int rc = 0;
      putstr("flash_iohandle_prewrite: unprotecting bootldr\r\n");
      rc = protectFlashRange(partition->base, partition->size, 0);
      putLabeledWord("  rc=", rc);
    }

    err = eraseFlashRange(partition->base + offset, len);
    if (err)
	 putLabeledWord("eraseFlashRange returned rc=", err);

  clear_vppen:
    clr_vppen();
    if (err) 
      return err;
    else
      return len;
  } else {
    return -EINVAL;
  }  
}

static int flash_iohandle_write(struct iohandle *ioh, char *src, size_t offset, size_t len)
{
  if (ioh) {
    struct FlashRegion *partition = (struct FlashRegion *)ioh->pdata;
    unsigned long regionBase = partition->base + offset;
    // unsigned long regionSize = partition->size;
    // int flags = partition->flags;
    unsigned long i, remaining_bytes, bytes_programmed;
    int num_bytes;
    char *p;
    u32 q;
    if (!len)
      return -EINVAL;
    i = 0;
    remaining_bytes = len;
    set_vppen();
    while(remaining_bytes > 0) { 

      num_bytes = 64;
      p = src + i;
      q = regionBase + i;

      if (((q % 64) == 0) && (remaining_bytes > 64) && (num_bytes == 64)) {
        /* program a block */
        if (programFlashBlock(q, (unsigned long*)p, 64)) {
          putstr("error while copying to flash!\r\n");
          break;
        }
        bytes_programmed = 64;
      } else {
        if (programFlashWord(q, *(unsigned long *)p)) {
          putstr("error while copying to flash!\r\n");
          break;
        }
        bytes_programmed = 4;
      }
      i += bytes_programmed;
      remaining_bytes -= bytes_programmed;
      
    }
    clr_vppen();
    return i;
  } else {
    return -EINVAL;
  }
}

static int flash_iohandle_close(struct iohandle *ioh)
{
  if (ioh) {
    mfree(ioh);
    return 0;
  } else {
    return -EINVAL;
  }
}

struct iohandle_ops flash_iohandle_ops = {
  len:  flash_iohandle_len,
  read: flash_iohandle_read,
  prewrite: flash_iohandle_prewrite,
  write: flash_iohandle_write,
  close: flash_iohandle_close,
};



struct iohandle *flash_open_iohandle(const char *partname)
{
  struct FlashRegion *partition = btflash_get_partition(partname);
  char *storage = mmalloc(sizeof(struct iohandle));
  struct iohandle *ioh = (struct iohandle *)storage;
  if (!partition) {
     putstr(__FUNCTION__); putstr(": no partition named "); putstr(partname); putstr("\r\n");
     return (void*)-ENOENT;
  }
  if (!ioh) {
     putstr(__FUNCTION__); putstr(": out of memory\r\n");
     return (void*)-ENOMEM;
  }
  ioh->sector_size = 512;
  ioh->pdata = storage + sizeof(struct iohandle);
  ioh->ops = &flash_iohandle_ops;
  ioh->pdata = partition;
  return ioh;
} 


struct jffs2_iohandle {
  char *buf;
  u32 size;
};

static int jffs2_iohandle_len(struct iohandle *ioh)
{
  if (ioh) {
    struct jffs2_iohandle *jioh = (struct jffs2_iohandle *)ioh->pdata;
    return jioh->size;
  } else {
    return 0;
  }
}

static int jffs2_iohandle_read(struct iohandle *ioh, char *buf, size_t offset, size_t len)
{
  if (ioh) {
    struct FlashRegion *partition = (struct FlashRegion *)ioh->pdata;
    if (!len || len > partition->size)
      len = partition->size;
    memcpy(buf, ((char *)flashword) + partition->base + offset, len);
    return len;
  } else {
    return -EINVAL;
  }
}


static int jffs2_iohandle_close(struct iohandle *ioh)
{
  if (ioh) {
    struct jffs2_iohandle *jioh = (struct jffs2_iohandle *)ioh->pdata;
    if (jioh->buf)
      mfree(jioh->buf);
    mfree(jioh);
    mfree(ioh);
    return 0;
  } else {
    return -EINVAL;
  }
}

struct iohandle_ops jffs2_iohandle_ops = {
  len:  jffs2_iohandle_len,
  read: jffs2_iohandle_read,
  close: jffs2_iohandle_close,
};



struct iohandle *jffs2_open_iohandle(const char *partname, const char *filename)
{ 
  struct FlashRegion *partition = btflash_get_partition(partname);
  char *storage = mmalloc(sizeof(struct iohandle));
  struct iohandle *ioh = (struct iohandle *)storage;
  struct jffs2_iohandle *jioh = (struct jffs2_iohandle *)malloc(sizeof(struct jffs2_iohandle));
  char *buf = (char*)DRAM_BASE0;
  if (!partition) {
     putstr(__FUNCTION__); putstr(": no partition named "); putstr(partname); putstr("\r\n");
     return (void*)-ENOENT;
  }
  if (!ioh || !jioh) {
     putstr(__FUNCTION__); putstr(": out of memory\r\n");
     return (void*)-ENOMEM;
  }
  ioh->sector_size = 512;
  ioh->pdata = jioh;
  ioh->ops = &jffs2_iohandle_ops;
  jioh->buf = 0;

  jioh->size = body_p1_load_file(partname,filename,(void*)buf);
  if (jioh->size > 0) {
    jioh->buf = malloc(jioh->size);
    memcpy(jioh->buf, buf, jioh->size);
  }
  return ioh;
} 




struct iohandle *open_iohandle(const char *iohandle_spec)
{
  char *spec;
  struct FlashRegion *partition = NULL;
  char *colon_pos = NULL;
  char *filename = NULL;
  if (!iohandle_spec) {
    return NULL;
  }

  spec = malloc(strlen(iohandle_spec)+1);
  strcpy(spec, iohandle_spec);

  colon_pos = strchr(spec, ':');
  if (colon_pos) {
    filename = colon_pos+1;
    *colon_pos = 0;
  }
  if (strncmp(spec, "hda1:", 4) == 0) {
#if defined(CONFIG_VFAT)
    if (filename) {
      return vfat_file_open(filename);
    } else {
      return get_ide_partition_iohandle(0);
    }
#endif
  } else if ((partition = btflash_get_partition(spec))) {
    if (filename) {
      return jffs2_open_iohandle(spec, filename);
    } else {
      return flash_open_iohandle(spec);
    }
  } else {
    return NULL;
  }
  return NULL;
}



COMMAND(copy, command_copy, "src dst [len] -- copy from src to dst", BB_RUN_FROM_RAM);
void command_copy(int argc, const char **argv)
{
  const char *srcspec = argv[1];
  const char *dstspec = argv[2];
  const char *lenspec = argv[3];
  struct iohandle *srchandle;
  struct iohandle *dsthandle;
  char buf[BUFSIZE];
  u32 len = 0;
  u32 srclen = 0;
  u32 dstlen = 0;
  u32 offset;
  u32 bytes_read = BUFSIZE;
  if (!srcspec || !dstspec) {
    goto usage;
  }
  srchandle = open_iohandle(srcspec);
  if (!srchandle) {
    putstr("Could not open source "); putstr(srcspec); putstr("\r\n");
    return;
  }
  dsthandle = open_iohandle(dstspec);
  if (!dsthandle) {
    putstr("Could not open destination "); putstr(dstspec); putstr("\r\n");
    return;
  }
  if (!dsthandle->ops || !dsthandle->ops->write) {
    putstr("destination "); putstr(dstspec); putstr(" does not support writing.\r\n");
    return;
  }
  if (lenspec) {
    len = strtoul(lenspec, 0, 0);
  }
  if (srchandle->ops->len && srchandle->ops->len(srchandle))
    srclen = srchandle->ops->len(srchandle);
  if (!len || (srclen && srclen < len))
      len = srclen;
  if (dsthandle->ops->len && dsthandle->ops->len(dsthandle)) {
    dstlen = dsthandle->ops->len(dsthandle);

  }
  putLabeledWord("srclen=", srclen);
  putLabeledWord("dstlen=", dstlen);
  putLabeledWord("nbytes=", len);

  if (!len)
    return;

  /*  if (dstlen && dstlen < len) {
    putstr("nbytes is too large for destination size ", dstlen);
    return;
    } */
  /* now prepare the destination */
  if (dsthandle->ops->prewrite)
    if (dsthandle->ops->prewrite(dsthandle, 0, dstlen) < 0)
      return;
  /* now copy the data */
  for (offset = 0; offset < len; offset += bytes_read) {
    int rc;
    rc = srchandle->ops->read(srchandle, buf, offset, BUFSIZE);
    if (rc <= 0) {
      putstr(__FUNCTION__); putLabeledWord(": read failed err=", rc);
      break;
    }
    bytes_read = rc;
    if ((offset % SZ_64K) == 0) {
      putstr("\r\naddr: ");
      putHexInt32(offset);
      putstr(" data: ");
      putHexInt32(*(unsigned long *) buf);
    }
    rc = dsthandle->ops->write(dsthandle, buf, offset, bytes_read);
    if (rc < 0) {
      putstr(__FUNCTION__); putLabeledWord(": write failed err=", rc);
      break;
    }
  }
  dsthandle->ops->close(dsthandle);
  srchandle->ops->close(srchandle);

  if (strcmp(dstspec, "params") == 0) {
	params_eval(PARAM_PREFIX, 0);
  }
  return;
 usage:
  putstr("usage: copy <srcspec> <dstspec>\r\n"
	 "    srcspec = <srcaddr>\r\n"
	 "            | <flashpartname> \r\n"    
	 "            | hda1:<filename> [<len>]\r\n"    
	 );
}
