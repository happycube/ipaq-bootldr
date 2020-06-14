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
 * IDE drive support
 */

#include <errno.h>
#include "bootldr.h"
#include "commands.h"
#include "heap.h"
#include "fs.h"
#include "ide.h"
#include "params.h"
#include "disk-partition-table.h"

static int ide_debug = 0;
static int ide_verbose = 0;
static int ide_sector_buffer_stride = 2;

enum ide_registers {
   IDE_DATA_REG    = 0,
   IDE_WRITE_PRECOMPENSATION_REG = 0x1,
   IDE_ERROR_REG = 1,
   IDE_FEATURE_REG = 1,
   IDE_SECTOR_COUNT_REG = 2,
   IDE_SECTOR_NUMBER_REG = 3,
   IDE_CYLINDER_LOW_REG = 4,
   IDE_CYLINDER_HIGH_REG = 5,
   IDE_DRIVE_HEAD_REG = 6,
   IDE_COMMAND_REG = 7,
   IDE_STATUS_REG  = 7,
   // IDE_CONTROL_REG  = 8,
   IDE_SECTOR_BUF  = 0x400
};

/* reference: 
 * ANSI NCITS 317-1998
 * AT Attachment with Packet Interface Extension (ata/atapi-4)
 */
enum ide_command {
   IDE_COMMAND_READ_BUFFER  = 0xE4,
   IDE_COMMAND_READ_SECTORS = 0x20, /* page 139 */
   IDE_COMMAND_WRITE_SECTORS = 0x30, /* page 207 */
   IDE_COMMAND_SEEK = 0x70, /* page 157 */
   // IDE_COMMAND_ = 0x91;
   IDE_COMMAND_IDENTIFY = 0xEC,
   IDE_COMMAND_IDENTIFY_PACKET = 0xA1, /* page 93 */
};

/* set this bit if passing LBA instead of C/H/S */
#define DEVICE_HEAD_IS_LBA (1 << 6)

struct drive_identification {
  /* 0x00 */ unsigned short flags;
  /* 0x01 */ unsigned short n_cylinders;
  /* 0x02 */ unsigned short reserved02;
  /* 0x03 */ unsigned short n_heads;
  /* 0x04 */ unsigned short n_bytes_per_track;
  /* 0x05 */ unsigned short n_bytes_per_sector;
  /* 0x06 */ unsigned short n_sectors_per_track;
  /* 0x07 */ unsigned short inter_sector_gap;
  /* 0x08 */ unsigned short reserved08;
  /* 0x09 */ unsigned short n_vendor_status_words;
  /* 0x0a */ unsigned char serial_number[20];
  /* 0x14 */ unsigned short controller_type;
  /* 0x15 */ unsigned short n_buffer_512b;
  /* 0x16 */ unsigned short n_ecc_bytes;
  /* 0x17 */ unsigned char firmware_revision[8];
  /* 0x1b */ unsigned char model_number[40] ;
  /* 0x2f */ unsigned short read_write_multiples_implemented;
  /* 0x30 */ unsigned short double_word_io_supported;
  /* 0x31 */ unsigned short capabilities31;
  /* 0x32 */ unsigned short capabilities32;
  /* 0x33 */ unsigned short minimum_pio_mode_number;
  /* 0x34 */ unsigned short retired34;
  /* 0x35 */ unsigned short flags53;
  /* 0x36 */ unsigned short current_n_cylinders;
  /* 0x37 */ unsigned short current_n_heads;
  /* 0x38 */ unsigned short current_n_sectors_per_track;
  /* 0x39 */ unsigned long  current_n_sectors;
  /* 0x3a */
  /* 0x3b */ unsigned short reserved3b;
  /* 0x3c */ unsigned long n_sectors_user_addressable;
  /* 0x3d */ 
  /* 0x3e */ unsigned short retired3e;
  /* 0x3f */ unsigned short reserved3f;
  /* 0x40 - 0x4f */ unsigned short reserved40[16];
  /* 0x50 */ unsigned short major_version;
  /* 0x51 */ unsigned short minor_version;
  /* 0x52 */ unsigned short cmd_set[6];
};

struct ide_adapter {
   volatile char *ioport;
   char buf[512];
   int ptable_was_read;
   struct dos_ptable_entry ptable[16];
   struct drive_identification identification; 
};

static struct ide_adapter *ide_adapter = 0;

static __inline unsigned short short_byte_swap(unsigned short x)
{
  return ((x & 0xFF) << 8) | ((x & 0xFF00) >> 8);
}

/* Byte-swap (in 2-byte chunks) from src to dest and 
   len is in bytes and counts space for a trailing NUL
   to be added to the end of dest (and is thus odd)
   */
static void copy_ident_string(char *dest, const char *src, int len)
{
    int i;

    len--;
    
    for (i=0;i<len;i+=2) {
        dest[i] = src[i+1];
        dest[i+1] = src[i];	
    }

    dest[len] = 0;
}

#define IDE_STATUS_ERR  (1 << 0)
#define IDE_STATUS_DRQ  (1 << 3)
#define IDE_STATUS_DEVFLT (1 << 5)
#define IDE_STATUS_DRDY (1 << 6)
#define IDE_STATUS_BSY  (1 << 7)
 
INTERNAL_PARAM(ide_use_packet, PT_INT, PF_DECIMAL, 0, NULL);
INTERNAL_PARAM(ide_use_chs, PT_INT, PF_DECIMAL, 0, NULL);

int ide_identify(void)
{
    int i;
    if (ide_adapter) {
        char serial_number[21];
        char firmware_revision[9];
        char model_number[41];
        u8 identify_command = 0;
        if (param_ide_use_packet.value)
           identify_command = IDE_COMMAND_IDENTIFY_PACKET;
        else
           identify_command = IDE_COMMAND_IDENTIFY;
        ide_adapter->ioport[IDE_COMMAND_REG] = identify_command;
        if (ide_verbose)
           putLabeledWord(" identify command=", identify_command);
        while (ide_adapter->ioport[IDE_STATUS_REG] & IDE_STATUS_BSY) { 
            if (ide_debug) putLabeledWord(" ide status=", ide_adapter->ioport[IDE_STATUS_REG]);
        }
        /* copy in identification */
        for (i = 0; i < sizeof(struct drive_identification)/2; i++) { 
            unsigned short *buf = (unsigned short *)&ide_adapter->identification;
            unsigned short *sector_buffer = (unsigned short *)&ide_adapter->ioport[IDE_SECTOR_BUF];
            buf[i] = sector_buffer[i];
        }
        if (ide_verbose)
            hex_dump((unsigned char *)&ide_adapter->identification, sizeof(struct drive_identification));
        copy_ident_string(serial_number, ide_adapter->identification.serial_number, sizeof(serial_number));
        copy_ident_string(firmware_revision, ide_adapter->identification.firmware_revision, sizeof(firmware_revision));
        copy_ident_string(model_number, ide_adapter->identification.model_number, sizeof(model_number));
        putstr("  serial_number: "); putstr(serial_number); putstr("\r\n");
        putstr("  firmware_revision: "); putstr(firmware_revision); putstr("\r\n");
        putstr("  model_number: "); putstr(model_number); putstr("\r\n");
        putLabeledWord("  n_sectors_user_addressable=", ide_adapter->identification.n_sectors_user_addressable);
        putLabeledWord("  bytes_per_sector=", ide_adapter->identification.n_bytes_per_sector);
        putLabeledWord("  major_version=", ide_adapter->identification.major_version);
        putLabeledWord("  minor_version=", ide_adapter->identification.minor_version);
        putLabeledWord("  flags=", ide_adapter->identification.flags);
        if (ide_verbose) {
            putLabeledWord("  bytes_per_track=", ide_adapter->identification.n_bytes_per_track);
            putLabeledWord("  n_sectors_per_track=", ide_adapter->identification.n_sectors_per_track);
            putLabeledWord("  n_cylinders=", ide_adapter->identification.n_cylinders);
            putLabeledWord("  n_heads=", ide_adapter->identification.n_heads);
            putLabeledWord("  cmdset[0]=", ide_adapter->identification.cmd_set[0]);
            putLabeledWord("  cmdset[1]=", ide_adapter->identification.cmd_set[1]);
            putLabeledWord("  cmdset[2]=", ide_adapter->identification.cmd_set[2]);
            putLabeledWord("  cmdset[3]=", ide_adapter->identification.cmd_set[3]);
            putLabeledWord("  cmdset[4]=", ide_adapter->identification.cmd_set[4]);
            putLabeledWord("  cmdset[5]=", ide_adapter->identification.cmd_set[5]);
        }
        ide_adapter->identification.n_bytes_per_sector = 512;
        putLabeledWord("  ACTUAL bytes_per_sector=", ide_adapter->identification.n_bytes_per_sector);
        if (ide_debug) {
                putLabeledWord(__FUNCTION__": status reg=", ide_adapter->ioport[IDE_STATUS_REG]);
                putLabeledWord(__FUNCTION__": error reg=", ide_adapter->ioport[IDE_ERROR_REG]);
        }

        return 0;
    } else {
        return -EINVAL;
    }
}

int ide_attach(volatile char *ioport)
{
  if (!ide_adapter)
    ide_adapter = mmalloc(sizeof(struct ide_adapter));
  if (!ide_adapter)
    return -ENOMEM;
  memset(ide_adapter, 0, sizeof(struct ide_adapter));
  ide_adapter->ioport = ioport;
  ide_identify();
  return 0;
}

int ide_detach(volatile char *ioport)
{

  return 0;
}

int ide_read(char *buf, unsigned long offset, unsigned long len)
{
  if (!ide_adapter || !ide_adapter->identification.n_bytes_per_sector) {
    return -EINVAL;
  } else {
    struct drive_identification *id = &ide_adapter->identification;
    unsigned long sector_count = len / id->n_bytes_per_sector;
    unsigned long sector_number = offset / id->n_bytes_per_sector;
    unsigned long start_sector = sector_number & 0xff;
    unsigned long start_cylinder = (sector_number >> 8) &0xffff ;
    unsigned long start_head = (sector_number >> 24) & 0x7 ;
    u8 status;
    int i;
    unsigned long count;
    
    if (param_ide_use_chs.value) {
      u32 track = sector_number / id->n_sectors_per_track;
      u32 sect_offset  = sector_number % id->n_sectors_per_track + 1;
 
    }
    if (ide_debug) {
      putLabeledWord("  start_cylinder=", start_cylinder);
      putLabeledWord("  start_head=", start_head);
      putLabeledWord("  start_sector=", start_sector);
      putLabeledWord("  sector_count=", sector_count);
    }
    ide_adapter->ioport[IDE_SECTOR_NUMBER_REG] = start_sector;
    ide_adapter->ioport[IDE_CYLINDER_HIGH_REG] = (start_cylinder >> 8) & 0xff;
    ide_adapter->ioport[IDE_CYLINDER_LOW_REG] = start_cylinder & 0xff;
    ide_adapter->ioport[IDE_SECTOR_COUNT_REG] = sector_count & 0xff;
    ide_adapter->ioport[IDE_DRIVE_HEAD_REG] = DEVICE_HEAD_IS_LBA | (start_head & 0xf);
    if (0) putstr("issuing read command\r\n");
    ide_adapter->ioport[IDE_COMMAND_REG] = IDE_COMMAND_READ_SECTORS;
    count=0;    
    while (ide_adapter->ioport[IDE_STATUS_REG] & IDE_STATUS_BSY) { 
            count++;            
    }
    status = ide_adapter->ioport[IDE_STATUS_REG];
    if (ide_debug) {
      putLabeledWord("ide read: status reg=", status);
      putLabeledWord("ide read: error reg=", ide_adapter->ioport[IDE_ERROR_REG]);
      putLabeledWord("ide read: bsy wait for num times =", count);
    }
    if (status & (IDE_STATUS_ERR | IDE_STATUS_DEVFLT)) {
      putLabeledWord("ide command failed with status=", status);
      putLabeledWord("  error reg=", ide_adapter->ioport[IDE_ERROR_REG]);
      putLabeledWord("  cyl high reg=", ide_adapter->ioport[IDE_CYLINDER_HIGH_REG]);
      putLabeledWord("  cyl low reg=", ide_adapter->ioport[IDE_CYLINDER_LOW_REG]);
      return -EIO;
    }
    for (i = 0; i < 512; i += ide_sector_buffer_stride) { 
      if (ide_sector_buffer_stride == 1)
        buf[i] = ide_adapter->ioport[IDE_SECTOR_BUF + i];
      else
        *(short *)&buf[i] = *(short *)&ide_adapter->ioport[IDE_SECTOR_BUF + i];
    }
   
    return 0;
  }
}


int ide_read_ptable(struct dos_ptable_entry *ptable)
{
   unsigned char *buf = (unsigned char *)ide_adapter->buf;
   int result = 0;
   if (!ide_adapter)
      return -ENOMEM;
   result = ide_read(buf, 0, 0x200);
   if (result)
      return result;
   putLabeledWord("ptable signature=", (buf[511] << 8) | buf[510]);
   if (buf[510] != 0x55 || buf[511] != 0xaa) {
      putstr("did not find dos partition table signature\r\n");
      return -EINVAL;
   }
   memcpy(ide_adapter->ptable, &ide_adapter->buf[446], 4 * sizeof(struct dos_ptable_entry));
   ide_adapter->ptable_was_read = 1;
   if (ptable)
      memcpy(ptable, &ide_adapter->buf[446], 4 * sizeof(struct dos_ptable_entry));
   return 0;
}

/* returns start offset in bytes */
int ide_get_partition_start_byte(int partid)
{
   if (!ide_adapter)
      return -ENODEV;
   if (partid > 4)
      return -EINVAL;
   if (!ide_adapter->ptable_was_read)
      ide_read_ptable(0);
   return ide_adapter->ptable[partid].starting_sector_lba * ide_adapter->identification.n_bytes_per_sector;
}
int ide_get_partition_n_bytes(int partid)
{
   if (!ide_adapter)
      return -ENODEV;
   if (partid > 4)
      return -EINVAL;
   if (!ide_adapter->ptable_was_read)
      ide_read_ptable(0);
   return ide_adapter->ptable[partid].n_sectors * ide_adapter->identification.n_bytes_per_sector;
}


int ide_read_partition(char *buf, int partid, size_t *lenp)
{
    struct dos_ptable_entry *ptable = ide_adapter->ptable;
    unsigned long n_bytes_per_sector = 0;    if (!ide_adapter->ptable_was_read) 
        ide_read_ptable(0);
    n_bytes_per_sector = ide_adapter->identification.n_bytes_per_sector;
    if (ide_verbose) {
        putstr("reading partition\r\n");
        putLabeledWord("buf=", (u32)buf);
        putLabeledWord("partid=", partid);
        putLabeledWord("  start_sector=", ptable[partid].starting_sector_lba);
        putLabeledWord("  n_sectors   =", ptable[partid].n_sectors);
        putLabeledWord("  n_bytes     =", ptable[partid].n_sectors * n_bytes_per_sector);
    }
    if (buf) {
        unsigned long sector_size = n_bytes_per_sector;
        unsigned long sector;
        unsigned long start_sector = ptable[partid].starting_sector_lba;
        unsigned long n_sectors = ptable[partid].n_sectors;
        unsigned long end_sector = start_sector + n_sectors;
        unsigned long offset = 0;
        if (lenp) {
            if (*lenp && (*lenp < n_sectors * sector_size)) {
                n_sectors = *lenp / sector_size;
                end_sector = start_sector + n_sectors;
                putLabeledWord(" reading n_sectors ", n_sectors);
            }
            *lenp = n_sectors * sector_size;
        }
        for (sector = start_sector; sector < end_sector; sector++) {
            ide_read(buf+offset, sector*sector_size, sector_size);
#if 0 
            if ((offset & 0x3ffff) == 0) {
                putLabeledWord(" sector=", sector);
                putLabeledWord(" offset=", offset);
                putLabeledWord("  value=", *(long*)(buf+offset));
            }
#endif
            offset += sector_size;
        } 
    }      
    return 0;
}

int ide_write(char *buf, unsigned long offset, unsigned long len)
{
  if (!ide_adapter || !ide_adapter->identification.n_bytes_per_sector) {
    return -EINVAL;
  } else {
    unsigned long sector_count = len / ide_adapter->identification.n_bytes_per_sector;
    unsigned long sector_number = offset / ide_adapter->identification.n_bytes_per_sector;
    unsigned long start_sector = sector_number & 0xff;
    unsigned long start_cylinder = (sector_number >> 8) &0xffff ;
    unsigned long start_head = (sector_number >> 24) & 0x7 ;
    u8 status;
    int i;
    if (ide_verbose) {
      putLabeledWord("  start_cylinder=", start_cylinder);
      putLabeledWord("  start_head=", start_head);
      putLabeledWord("  start_sector=", start_sector);
      putLabeledWord("  sector_count=", sector_count);
    }
    ide_adapter->ioport[IDE_SECTOR_NUMBER_REG] = start_sector;
    ide_adapter->ioport[IDE_CYLINDER_HIGH_REG] = (start_cylinder >> 8) & 0xff;
    ide_adapter->ioport[IDE_CYLINDER_LOW_REG] = start_cylinder & 0xff;
    ide_adapter->ioport[IDE_SECTOR_COUNT_REG] = sector_count & 0xff;
    ide_adapter->ioport[IDE_DRIVE_HEAD_REG] = DEVICE_HEAD_IS_LBA | (start_head & 0xf);
    ide_adapter->ioport[IDE_COMMAND_REG] = IDE_COMMAND_WRITE_SECTORS;
    while (ide_adapter->ioport[IDE_STATUS_REG] & IDE_STATUS_BSY) { 
      /* wait for ready */
    }
    for (i = 0; i < 512; i += ide_sector_buffer_stride) { 
      while (!(ide_adapter->ioport[IDE_STATUS_REG] & IDE_STATUS_DRQ))
	/* wait for DRQ */;
      if (ide_sector_buffer_stride == 1)
        ide_adapter->ioport[IDE_SECTOR_BUF + i] = buf[i];
      else
        *(short*)&ide_adapter->ioport[IDE_SECTOR_BUF + i] = *(short*)&buf[i];
      while (ide_adapter->ioport[IDE_STATUS_REG] & IDE_STATUS_BSY) { 
	/* wait for ready */
      }
    }
    status = ide_adapter->ioport[IDE_STATUS_REG];
    if (ide_debug) {
        putLabeledWord("ide write: status reg=", status);
        putLabeledWord("ide write: error reg=", ide_adapter->ioport[IDE_ERROR_REG]);
    }
    if (status & (IDE_STATUS_ERR | IDE_STATUS_DEVFLT)) {
        putLabeledWord("ide command failed with status=", status);
        putLabeledWord("  error reg=", ide_adapter->ioport[IDE_ERROR_REG]);
        putLabeledWord("  cyl high reg=", ide_adapter->ioport[IDE_CYLINDER_HIGH_REG]);
        putLabeledWord("  cyl low reg=", ide_adapter->ioport[IDE_CYLINDER_LOW_REG]);
        return -EIO;
    }
    return 0;
  }
}

int ide_write_partition(char *buf, int partid, size_t *lenp)
{
    struct dos_ptable_entry *ptable = ide_adapter->ptable;
    unsigned long n_bytes_per_sector = 0;
    if (!ide_adapter->ptable_was_read) 
        ide_read_ptable(0);
    n_bytes_per_sector = ide_adapter->identification.n_bytes_per_sector;
    if (ide_verbose) {
        putstr("write partition\r\n");
        putLabeledWord("buf=", (u32)buf);
        putLabeledWord("partid=", partid);
        putLabeledWord("  start_sector=", ptable[partid].starting_sector_lba);
        putLabeledWord("  n_sectors   =", ptable[partid].n_sectors);
        putLabeledWord("  n_bytes     =", ptable[partid].n_sectors * n_bytes_per_sector);
    }
    if (buf) {
        unsigned long sector_size = n_bytes_per_sector;
        unsigned long sector;
        unsigned long start_sector = ptable[partid].starting_sector_lba;
        unsigned long n_sectors = ptable[partid].n_sectors;
        unsigned long end_sector = start_sector + n_sectors;
        unsigned long offset = 0;
        if (lenp) {
            if (*lenp && (*lenp < n_sectors * sector_size)) {
                n_sectors = *lenp / sector_size;
                end_sector = start_sector + n_sectors;
                putLabeledWord(" reading n_sectors ", n_sectors);
            }
            *lenp = n_sectors * sector_size;
        }
        for (sector = start_sector; sector < end_sector; sector++) {
            ide_write(buf+offset, sector*sector_size, sector_size);
#if 0
            if ((offset & 0x3ffff) == 0) {
                putLabeledWord(" sector=", sector);
                putLabeledWord(" offset=", offset);
                putLabeledWord("  value=", *(long*)(buf+offset));
            }
#endif
            offset += sector_size;
        } 
    }      
    return 0;
}

static int ide_iohandle_read(struct iohandle *ioh, char *buf, size_t offset, size_t len) 
{
   struct dos_ptable_entry *pentry = (struct dos_ptable_entry *)ioh->pdata;
   unsigned long sector_size = ide_adapter->identification.n_bytes_per_sector;
   unsigned long start_sector = pentry->starting_sector_lba;
   return ide_read(buf, offset + start_sector * sector_size, len);
}

static int ide_iohandle_write(struct iohandle *ioh, char *buf, size_t offset, size_t len)
{
   struct dos_ptable_entry *pentry = (struct dos_ptable_entry *)ioh->pdata;
   unsigned long sector_size = ide_adapter->identification.n_bytes_per_sector;
   unsigned long start_sector = pentry->starting_sector_lba;
   return ide_write(buf, offset + start_sector * sector_size, len);
}

static int ide_iohandle_close(struct iohandle *ioh)
{
   return 0;
}

static struct iohandle_ops ide_iohandle_ops = {
   read: ide_iohandle_read,
   write: ide_iohandle_write,
   close: ide_iohandle_close
};

struct iohandle *get_ide_partition_iohandle(u8 partid)
{
  struct iohandle *ioh = mmalloc(sizeof(struct iohandle));
   if (!ide_adapter->ptable_was_read)
      ide_read_ptable(0);
  ioh->ops = &ide_iohandle_ops;
  ioh->pdata = &ide_adapter->ptable[partid];
  ioh->sector_size = ide_adapter->identification.n_bytes_per_sector;
  return ioh;
} 

SUBCOMMAND(ide, identify,    command_ide_identify,
           "-- reads and prints disk identification/configuration information", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(ide, attach,      command_ide_attach,
           "<ioaddr> -- attaches ide drive at ioaddr -- not needed normally", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(ide, ptable,      command_ide_ptable,
           "-- reads and prints the dos partition table", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(ide, read,        command_ide_read,
           "<dstaddr> <startbyte> <nbytes> -- reads from disk to dram", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(ide, read_sector, command_ide_read_sector,
           "<dstaddr> <sectorno>    -- reads one sector to dram", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(ide, read_partition, command_ide_read_partition,
           "<dstaddr> <partno> [nbytes] -- reads from partition to dram.  partno starts at 0.", 
           BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(ide, write,        command_ide_write,
           "<srcaddr> <startbyte> <nbytes> -- writes to disk from dram", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(ide, write_sector, command_ide_write_sector,
           "<srcaddr> <sectorno>    -- writes one sector from dram", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(ide, write_partition, command_ide_write_partition,
           "<srcaddr> <partno> [nbytes] -- writes to partition from dram.  partno starts at 0.", 
           BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(ide, debug, command_ide_debug,
           "-- toggle ide debug (very verbose) mode",
           BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(ide, verbose, command_ide_verbose,
           "-- toggle ide verbose mode",
           BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(ide, 8bit, command_ide_8bit,
           "-- read/write sector buffer one byte at a time",
           BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(ide, 16bit, command_ide_16bit,
           "-- read/write sector buffer two bytes at a time",
           BB_RUN_FROM_RAM, 0); 

void command_ide_identify(int argc, const char* argv[])
{
     ide_identify();
}
void command_ide_attach(int argc, const char* argv[])
{
      volatile char *ioport = (volatile char *)strtoul(argv[2], 0, 0);
      putLabeledWord("attaching ide drive at ioport=", (u32)ioport);
      ide_attach(ioport);
}
void command_ide_read(int argc, const char* argv[])
{
      char *buf = (char *)strtoul(argv[2], 0, 0);
      unsigned long start_byte = strtoul(argv[3], 0, 0);
      unsigned long n_bytes = strtoul(argv[4], 0, 0);
      ide_read(buf, start_byte, n_bytes);
}
void command_ide_ptable(int argc, const char* argv[])
{
      struct dos_ptable_entry ptable[4]; 
      if (!ide_read_ptable(ptable)) {
         hex_dump((unsigned char *)ptable, sizeof(ptable));
      }
}
void command_ide_read_sector(int argc, const char* argv[])
{
      char *buf = (char *)strtoul(argv[2], 0, 0);
      unsigned long sector = strtoul(argv[3], 0, 0);
      ide_read(buf, sector * 0x200, 0x200);
}
void command_ide_read_partition(int argc, const char* argv[])
{
      char *buf = (char *)strtoul(argv[2], 0, 0);
      int partid = strtoul(argv[3], 0, 0);
      size_t len = 0;
      if (argv[4] != NULL) {
        len = strtoul(argv[4], 0, 0);
        putLabeledWord("  len=", len);
      }
      ide_read_partition(buf, partid, &len);
}

void command_ide_write(int argc, const char* argv[])
{
      char *buf = (char *)strtoul(argv[2], 0, 0);
      unsigned long start_byte = strtoul(argv[3], 0, 0);
      unsigned long n_bytes = strtoul(argv[4], 0, 0);
      ide_write(buf, start_byte, n_bytes);
}

void command_ide_write_sector(int argc, const char* argv[])
{
      char *buf = (char *)strtoul(argv[2], 0, 0);
      unsigned long sector = strtoul(argv[3], 0, 0);
      ide_write(buf, sector * 0x200, 0x200);
}
void command_ide_write_partition(int argc, const char* argv[])
{
      char *buf = (char *)strtoul(argv[2], 0, 0);
      int partid = strtoul(argv[3], 0, 0);
      size_t len = 0;
      if (argv[4] != NULL) {
        len = strtoul(argv[4], 0, 0);
        putLabeledWord("  len=", len);
      }
      ide_write_partition(buf, partid, &len);
}

void command_ide_debug(int argc, const char* argv[])
{
    ide_debug ^= 1;
    putLabeledWord("ide_debug = ", ide_debug);
}
void command_ide_verbose(int argc, const char* argv[])
{
    ide_verbose ^= 1;
    putLabeledWord("ide_verbose = ", ide_verbose);
}

void command_ide_8bit(int argc, const char* argv[])
{
    ide_sector_buffer_stride = 1;
    putLabeledWord("ide_sector_buffer_stride=", ide_sector_buffer_stride);
}
void command_ide_16bit(int argc, const char* argv[])
{
    ide_sector_buffer_stride = 2;
    putLabeledWord("ide_sector_buffer_stride=", ide_sector_buffer_stride);
}
