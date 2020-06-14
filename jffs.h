/* JFFS support for bootldr
 * ^^^^^^^^^^^^^^^^^^^^^^^^
 * Copyright (C) 2000, 2001  John G Dorsey
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The author may be contacted via electronic mail at <john+@cs.cmu.edu>,
 * or at the following address:
 *
 *   John Dorsey
 *   Carnegie Mellon University
 *   HbH2201 - ICES
 *   5000 Forbes Avenue
 *   Pittsburgh, PA  15213
 */

#ifndef CONFIG_ACCEPT_GPL
#error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#if !defined(_JFFS_H)
#define _JFFS_H

#include "bootconfig.h"
#include "load_kernel/include/load_kernel.h"

#define JFFS_MAGIC  (0x34383931)
#define JFFS2_MAGIC (0x1985)      /* new-generation (header CRC) JFFS2 */

#define JFFS_ROOT_INODE      (1)  /* only root has this inumber */
#define JFFS_INVALID_INODE   (0)  /* nobody has this inumber */

#define JFFS_INVALID_VERSION (-1) /* nobody has this version */

/* Flags for the JFFS2 nodetype field: */
#define JFFS2_NODE_ACCURATE	(0x2000)

#define JFFS2_FEATURE_INCOMPAT	(0xc000 | JFFS2_NODE_ACCURATE)

#define JFFS2_NODETYPE_DIRENT	(JFFS2_FEATURE_INCOMPAT | 1)
#define JFFS2_NODETYPE_INODE	(JFFS2_FEATURE_INCOMPAT | 2)


/* Values for the JFFS2 compr field: */
#define JFFS2_COMPR_NONE      (0x0)
#define JFFS2_COMPR_ZERO      (0x1)
#define JFFS2_COMPR_RTIME     (0x2)
#define JFFS2_COMPR_RUBINMIPS (0x3)
#define JFFS2_COMPR_COPY      (0x4)
#define JFFS2_COMPR_DYNRUBIN  (0x5)
#define JFFS2_COMPR_ZLIB      (0x6)


/* Constants and definitions for JFFS2 Rubin decoder: */

#define RUBIN_REG_SIZE	      (16)
#define UPPER_BIT_RUBIN       (((long) 1) << (RUBIN_REG_SIZE - 1))
#define LOWER_BITS_RUBIN      ((((long) 1) << (RUBIN_REG_SIZE - 1)) - 1)

struct pushpull {
  unsigned char *buf;
  unsigned int   buflen;
  unsigned int   ofs;
  unsigned int   reserve;
};

struct rubin_state {
  unsigned long   p;		
  unsigned long   q;	
  unsigned long   rec_q;
  long            bit_number;
  struct pushpull pp;
  int             bit_divider;
  int             bits[8];
};


/* Values for the JFFS2 dirent type field: */
#define JFFS2_TYPE_UNKNOWN (0x0)
#define JFFS2_TYPE_FIFO    (0x1)
#define JFFS2_TYPE_CHR     (0x2)
#define JFFS2_TYPE_DIR     (0x4)
#define JFFS2_TYPE_BLK     (0x6)
#define JFFS2_TYPE_REG     (0x8)
#define JFFS2_TYPE_LNK     (0xa)
#define JFFS2_TYPE_SOCK    (0xc)
#define JFFS2_TYPE_WHT     (0xe)


/* Add this value to dirent versions in order to push them to the end of
 * the replay log lists:
 */
#define JFFS2_DIRENT_VERSION (0x40000000)


#define JFFS_PATH_SEPARATOR '/'
#define JFFS_PARENT_DIR     ".."
#define JFFS_MAXPATHLEN     (255)

#define JFFS_LOOP_CHECK_LENGTH (16)

#define JFFS_LIST_HEADER \
"   mode       links   ino     mtime        size        name\r\n" \
"-----------------------------------------------------------------------\r\n"
#define JFFS_LIST_LINKS_WIDTH   (4)
#define JFFS_LIST_INUMBER_WIDTH (5)
#define JFFS_LIST_SIZE_WIDTH    (8)

#define JFFS_MODE_FMT   (0170000)
#define JFFS_MODE_FSOCK (0140000)
#define JFFS_MODE_FLNK  (0120000)
#define JFFS_MODE_FREG  (0100000)
#define JFFS_MODE_FBLK  (0060000)
#define JFFS_MODE_FDIR  (0040000)
#define JFFS_MODE_FCHR  (0020000)
#define JFFS_MODE_FIFO  (0010000)
#define JFFS_MODE_SUID  (0004000)
#define JFFS_MODE_SGID  (0002000)
#define JFFS_MODE_SVTX  (0001000)
#define JFFS_MODE_RWXU  (0000700)
#define JFFS_MODE_RUSR  (0000400)
#define JFFS_MODE_WUSR  (0000200)
#define JFFS_MODE_XUSR  (0000100)
#define JFFS_MODE_RWXG  (0000070)
#define JFFS_MODE_RGRP  (0000040)
#define JFFS_MODE_WGRP  (0000020)
#define JFFS_MODE_XGRP  (0000010)
#define JFFS_MODE_RWXO  (0000007)
#define JFFS_MODE_ROTH  (0000004)
#define JFFS_MODE_WOTH  (0000002)
#define JFFS_MODE_XOTH  (0000001)

#define JFFS_DUMP_LINEWIDTH (16)


/* Parameters for displaying the inode histogram: */
#define JFFS_HISTOGRAM_WIDTH  (64)
#define JFFS_HISTOGRAM_HEIGHT (10)


/* From linux/include/linux/jffs.h, by Finn Hakansson: */
struct jffs_inode {
  unsigned int   magic;		/* A constant magic number.  */
  unsigned int   ino;		/* Inode number.  */
  unsigned int   pino;		/* Parent's inode number.  */
  unsigned int   version;	/* Version number.  */
  unsigned int   mode;		/* The file's type or mode.  */
  unsigned short uid;		/* The file's owner.  */
  unsigned short gid;		/* The file's group.  */
  unsigned int   atime;		/* Last access time.  */
  unsigned int   mtime;		/* Last modification time.  */
  unsigned int   ctime;		/* Creation time.  */
  unsigned int   offset;	/* Where to begin to write.  */
  unsigned int   dsize;		/* Size of the node's data.  */
  unsigned int   rsize;		/* How much are going to be replaced?  */
  unsigned char  nsize;		/* Name length.  */
  unsigned char  nlink;		/* Number of links.  */
  unsigned char  spare:   6;	/* For future use.  */
  unsigned char  rename:  1;	/* Rename to a name of an existing file?  */
  unsigned char  deleted: 1;	/* Has this file been deleted?  */
  unsigned char  accurate;	/* The inode is obsolete if accurate == 0.  */
  unsigned int   dchksum;	/* Checksum for the data.  */
  unsigned short nchksum;	/* Checksum for the name.  */
  unsigned short chksum;	/* Checksum for the raw inode.  */
};

/* From linux/include/linux/jffs2.h, by David Woodhouse: */
struct jffs2_unknown_node {
  unsigned short magic;
  unsigned short nodetype;
  unsigned int   totlen;
  unsigned int   hdr_crc;    /* Unfortunate overhead. */
};

struct jffs2_raw_dirent {
  unsigned short magic;
  unsigned short nodetype;
  unsigned int   totlen;
  unsigned int   hdr_crc;
  unsigned int   pino;
  unsigned int   version;
  unsigned int   ino;
  unsigned int   mctime;
  unsigned char  nsize;
  unsigned char  type;
  unsigned char  unused[2];
  unsigned int   node_crc;
  unsigned int   name_crc;
};

struct jffs2_raw_inode {
  unsigned short magic;      /* A constant magic number.  */
  unsigned short nodetype;   /* == JFFS2_NODETYPE_INODE */
  unsigned int   totlen;     /* Total length of this node (inc data, etc.) */
  unsigned int   hdr_crc;
  unsigned int   ino;        /* Inode number.  */
  unsigned int   version;    /* Version number.  */
  unsigned int   mode;       /* The file's type or mode.  */
  unsigned short uid;        /* The file's owner.  */
  unsigned short gid;        /* The file's group.  */
  unsigned int   isize;      /* Total resultant size of this inode (used for truncations)  */
  unsigned int   atime;      /* Last access time.  */
  unsigned int   mtime;      /* Last modification time.  */
  unsigned int   ctime;      /* Change time.  */
  unsigned int   offset;     /* Where to begin to write.  */
  unsigned int   csize;      /* (Compressed) data size */
  unsigned int   dsize;	     /* Size of the node's data. (after decompression) */
  unsigned char  compr;      /* Compression algorithm used */
  unsigned char  unused[3];
  unsigned int   data_crc;   /* CRC for the (compressed) data.  */
  unsigned int   node_crc;   /* CRC for the raw inode (excluding data)  */
};

union jffs2_inode {
  struct jffs2_unknown_node u;
  struct jffs2_raw_dirent   d;
  struct jffs2_raw_inode    i;
};


union jffs_generic_inode {
  struct jffs_inode  v1;
  union  jffs2_inode v2;
};


struct jffs_log {
  struct jffs_log           *next;  /* 0-term'ed ordered list (by version) */
  union  jffs_generic_inode *inode; /* inode in flash */
};


#define JFFS_CONF_ACCESS_RO (0x01)
#define JFFS_CONF_ACCESS_RW (0x02)

/* Strings in struct jffs_conf and struct jffs_conf_image are NULL-terminated,
 * or are NULL pointers if not present.
 */

struct jffs_conf_image {
  char         *image;		/* "image" keyword (begins image block) */
  char         *label;		/* "label" keyword */
  char         *alias;		/* "alias" keyword */
  unsigned int  access;		/* mask for "read-only" and "read-write" */
  unsigned int  ramdisk;	/* "ramdisk" keyword (size in KB) */
  char         *root;		/* "root" keyword */
  char         *append;		/* "append" keyword */
  char         *literal;	/* "literal" keyword */
};


struct jffs_conf {
  char                    *deflt;	/* "default" keyword */
  unsigned int             access;	/* "read-only" and "read-write" */
  unsigned int             ramdisk;	/* "ramdisk" keyword (size in KB) */
  char                    *root;	/* "root" keyword */
  char                    *append;	/* "append" keyword */

  unsigned int             nimages;	/* # of image blocks in `images' */
  struct jffs_conf_image **images;	/* array of image blocks */
};


struct jffs_conf_parse {
  char         *keyword;		/* start-of-line keyword */
  enum { param_string, param_int, param_flag } param;
  unsigned int  field;			/* offset in conf structure */
  unsigned int  flag;			/* for param_flag keywords */
};


#define JFFS_CONF_PARSE_GLOBAL(keyword, param, field, flag) \
{ (keyword), (param), (unsigned int)&(((struct jffs_conf *)0)->field), (flag) }

#define JFFS_CONF_PARSE_IMAGE(keyword, param, field, flag) \
{ (keyword), (param), (unsigned int)&(((struct jffs_conf_image *)0)->field), \
  (flag) }

#define JFFS_CONF_KEYWORD_IMAGE "image"

#define JFFS_CONF_TOKEN_WHITESPACE " \t\n"
#define JFFS_CONF_TOKEN_EQUALS     "="
#define JFFS_CONF_TOKEN_DELIM JFFS_CONF_TOKEN_WHITESPACE JFFS_CONF_TOKEN_EQUALS


struct jffs_name {
  unsigned int      parent;    /* JFFS parent inode number */
  unsigned char     nsize;     /* name length */
  char             *name;      /* pointer to final file name in flash */
  struct jffs_name *next;      /* NULL-terminated unordered list */
};

struct jffs_child {
  struct jffs_hash *child;     /* the child inode */
  struct jffs_name *name;      /* specifically, which link instance we want */
};

#define JFFS_HASH_MODULUS (32) /* number of hash buckets */

struct jffs_hash {
  unsigned int      inumber; /* JFFS inode number */
  struct jffs_hash *next;    /* NULL-terminated unordered list */
  struct jffs_log  *log;     /* NULL-terminated _ordered_ list (by inumber) */

  /* Replay information: */

  unsigned int        nlink;     /* number of hard links to this file */
  struct jffs_name   *names;     /* each hard link can have its own name */
  unsigned int        mtime;     /* modification time */
  unsigned int        mode;      /* file type/mode */
  unsigned int        dsize;     /* final data size of file (-1: deleted) */
  unsigned char      *data;      /* pointer to assembled file data in RAM */
  unsigned int        nchildren; /* number of children of this node */
  struct jffs_child  *children;  /* array of pointers to child inodes */

  /* Miscellaneous runtime information: */

  union {

    struct jffs_conf *conf;	/* bootldr configuration data */

  } misc;

}; 


/* Memory map
 * ^^^^^^^^^^
 * We have to ensure that we don't overrun the relocated bootloader.
 * In addition, we should leave space aside in low memory for the
 * zImage which we'll copy before booting.
 *
 * Note that all of this assumes that we have relatively more DRAM
 * than flash (or at least, more DRAM than JFFS data). For example,
 * the H3650 has 32MB SDRAM and 16MB flash. The largest JFFS log 
 * which could be replayed on this machine is approximately 25.4MB.
 * (This could be an issue on Assabet, which can accept 32MB of
 * StrataFlash, but SDRAM bank 0 is only 32MB.) Practically speaking,
 * however, since we only replay file data on demand, we would have
 * to peek/md5sum/boot large files (or many files) in order to run
 * up against this limitation.
 *
 *  ________________________
 * |                        | DRAM_BASE0 + DRAM_SIZE0  (top of bank 0)
 * |   relocated bootldr    |
 * |________________________| DRAM_BASE0 + DRAM_SIZE0 - SZ_2M (FLASH_IMG_START)
 * |                        |
 * |      bootldr heap      |
 * |________________________| FLASH_IMG_START - SZ_4M (HEAP_START)
 * |                        |
 * |    bootldr r/w data    |
 * |________________________| HEAP_START - SZ_32K (BOOT_DATA_START)
 * |                        |
 * |     MMU page table     |
 * |________________________| BOOT_DATA_START - SZ_16K (MMU_TABLE_START)
 * |                        |
 * |      bootldr stack     |
 * |________________________| MMU_TABLE_START - SZ_16K (STACK_BASE)
 * |                        |
 * |                        |
 * |                        |
 * |    JFFS replay space   |
 * |                        |
 * |                        |
 * |                        |
 * |________________________| DRAM_BASE0 + JFFS_KERNEL_LENGTH (top of kernel)
 * |                        |
 * |      kernel zImage     |
 * |________________________| DRAM_BASE0 (base of bank 0)
 */

#define JFFS_KERNEL_OFFSET (0x8000)
#define JFFS_KERNEL_BASE   (VKERNEL_BASE + JFFS_KERNEL_OFFSET)
#define JFFS_KERNEL_LENGTH (0xc0000)  /* big kernels are popular these days */

#define JFFS_REPLAY_BASE   (DRAM_BASE0 + JFFS_KERNEL_LENGTH)
#define JFFS_REPLAY_LIMIT  (STACK_BASE)

extern int   jffs_init(int force);
extern void  jffs_statistics(void);
extern void  jffs_dump_inode(unsigned int inumber);
extern void  jffs_dump_file(char *path, unsigned int size);
extern void  jffs_list(char *path);
extern void *jffs_file(char *path, unsigned int *length);
extern int   jffs_conf(char *path, struct jffs_conf **conf);
extern void  jffs_conf_list(struct jffs_conf *conf);
extern int   jffs_conf_image(struct jffs_conf *conf, const char *boot_file,
			     char *imagename, char *bootargs);

extern u32 jffs_init_1pass_list(void);
u32 jffs2_1pass_load(char *dest, struct part_info *part,const char *fname);

u32 jffs2_info(struct part_info *part);
u32 jffs2_scan_test( struct part_info *part);
u32 jffs2_1pass_ls(struct part_info *part,const char *fname);

#ifdef CONFIG_MD5
extern void  jffs_md5sum(char *path);
#endif

#endif  /* !defined(_JFFS_H) */
