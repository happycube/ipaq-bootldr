/*
 *  linux/include/linux/mtd/nand.h
 *
 *  Copyright (c) 2000 David Woodhouse <dwmw2@mvhi.com>
 *                     Steven J. Hill <sjhill@cotw.com>
 *		       Thomas Gleixner <tglx@linutronix.de>
 *
 * $Id: nand.h,v 1.1 2003/04/08 13:24:46 jamey Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Info:
 *   Contains standard defines and IDs for NAND flash devices
 *
 *  Changelog:
 *   01-31-2000 DMW     Created
 *   09-18-2000 SJH     Moved structure out of the Disk-On-Chip drivers
 *			so it can be used by other NAND flash device
 *			drivers. I also changed the copyright since none
 *			of the original contents of this file are specific
 *			to DoC devices. David can whack me with a baseball
 *			bat later if I did something naughty.
 *   10-11-2000 SJH     Added private NAND flash structure for driver
 *   10-24-2000 SJH     Added prototype for 'nand_scan' function
 *   10-29-2001 TG	changed nand_chip structure to support 
 *			hardwarespecific function for accessing control lines
 *   02-21-2002 TG	added support for different read/write adress and
 *			ready/busy line access function
 *   02-26-2002 TG	added chip_delay to nand_chip structure to optimize
 *			command delay times for different chips
 *   04-28-2002 TG	OOB config defines moved from nand.c to avoid duplicate
 *			defines in jffs2/wbuf.c
 *   08-07-2002 TG	forced bad block location to byte 5 of OOB, even if
 *			CONFIG_MTD_NAND_ECC_JFFS2 is not set
 *   08-10-2002 TG	extensions to nand_chip structure to support HW-ECC
 *
 *   08-29-2002 tglx 	nand_chip structure: data_poi for selecting 
 *			internal / fs-driver buffer
 *			support for 6byte/512byte hardware ECC
 *			read_ecc, write_ecc extended for different oob-layout
 *			oob layout selections: NAND_NONE_OOB, NAND_JFFS2_OOB,
 *			NAND_YAFFS_OOB
 */
#ifndef __LINUX_MTD_NAND_H
#define __LINUX_MTD_NAND_H


#if 0 // NCB 25/11/2002
#include <linux/config.h>
#include <linux/sched.h>
#else

#define CONFIG_MTD_NAND_ECC

//#include <sys/types.h>
//#ifndef _SYS_TYPES_H
typedef unsigned char u_char;
typedef unsigned long loff_t;
//#endif

#define MTD_ABSENT		0
#define MTD_RAM			1
#define MTD_ROM			2
#define MTD_NORFLASH		3
#define MTD_NANDFLASH		4
#define MTD_PEROM		5
#define MTD_OTHER		14
#define MTD_UNKNOWN		15



#define MTD_CLEAR_BITS		1       // Bits can be cleared (flash)
#define MTD_SET_BITS		2       // Bits can be set
#define MTD_ERASEABLE		4       // Has an erase function
#define MTD_WRITEB_WRITEABLE	8       // Direct IO is possible
#define MTD_VOLATILE		16      // Set for RAMs
#define MTD_XIP			32	// eXecute-In-Place possible
#define MTD_OOB			64	// Out-of-band data (NAND flash)
#define MTD_ECC			128	// Device capable of automatic ECC

// Some common devices / combinations of capabilities
#define MTD_CAP_ROM		0
#define MTD_CAP_RAM		(MTD_CLEAR_BITS|MTD_SET_BITS|MTD_WRITEB_WRITEABLE)
#define MTD_CAP_NORFLASH        (MTD_CLEAR_BITS|MTD_ERASEABLE)
#define MTD_CAP_NANDFLASH       (MTD_CLEAR_BITS|MTD_ERASEABLE|MTD_OOB)
#define MTD_WRITEABLE		(MTD_CLEAR_BITS|MTD_SET_BITS)


// Types of automatic ECC/Checksum available
#define MTD_ECC_NONE		0 	// No automatic ECC available
#define MTD_ECC_RS_DiskOnChip	1	// Automatic ECC on DiskOnChip
#define MTD_ECC_SW		2	// SW ECC for Toshiba & Samsung devices

struct mtd_info_user {
	u_char type;
	u_int32_t flags;
	u_int32_t size;	 // Total size of the MTD
	u_int32_t erasesize;
	u_int32_t oobblock;  // Size of OOB blocks (e.g. 512)
	u_int32_t oobsize;   // Amount of OOB data per block (e.g. 16)
	u_int32_t ecctype;
	u_int32_t eccsize;
};

#define MTD_ERASE_PENDING      	0x01
#define MTD_ERASING		0x02
#define MTD_ERASE_SUSPEND	0x04
#define MTD_ERASE_DONE          0x08
#define MTD_ERASE_FAILED        0x10


struct mtd_info {
	u_char type;
	u_int32_t flags;
	u_int32_t size;	 // Total size of the MTD

	/* "Major" erase size for the device. Naïve users may take this
	 * to be the only erase size available, or may use the more detailed
	 * information below if they desire
	 */
	u_int32_t erasesize;

	u_int32_t oobblock;  // Size of OOB blocks (e.g. 512)
	u_int32_t oobsize;   // Amount of OOB data per block (e.g. 16)
	u_int32_t ecctype;
	u_int32_t eccsize;

	// Kernel-only stuff starts here.
	char *name;
#if 0 // NCB 25/11/2002
	int index;

	/* Data for variable erase regions. If numeraseregions is zero,
	 * it means that the whole device has erasesize as given above. 
	 */
	int numeraseregions;
	struct mtd_erase_region_info *eraseregions; 

	/* This really shouldn't be here. It can go away in 2.5 */
	u_int32_t bank_size;

	struct module *module;
	int (*erase) (struct mtd_info *mtd, struct erase_info *instr);
#endif
	// new erase function NCB 25/11/2002
	int (*erase) (struct mtd_info *mtd, loff_t from, size_t len);

#if 0 // NCB 25/11/2002
	/* This stuff for eXecute-In-Place */
	int (*point) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char **mtdbuf);

	/* We probably shouldn't allow XIP if the unpoint isn't a NULL */
	void (*unpoint) (struct mtd_info *mtd, u_char * addr, loff_t from, size_t len);
#endif

	int (*read) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*write) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);

	int (*read_ecc) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf, u_char *eccbuf, int oobsel);
	int (*write_ecc) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf, u_char *eccbuf, int oobsel);

	int (*read_oob) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*write_oob) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);

#if 0 // NCB 25/11/2002
	/* 
	 * Methods to access the protection register area, present in some 
	 * flash devices. The user data is one time programmable but the
	 * factory data is read only. 
	 */
	int (*read_user_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);

	int (*read_fact_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);

	/* This function is not yet implemented */
	int (*write_user_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);

	/* iovec-based read/write methods. We need these especially for NAND flash,
	   with its limited number of write cycles per erase.
	   NB: The 'count' parameter is the number of _vectors_, each of 
	   which contains an (ofs, len) tuple.
	*/
	int (*readv) (struct mtd_info *mtd, struct iovec *vecs, unsigned long count, loff_t from, size_t *retlen);
	int (*readv_ecc) (struct mtd_info *mtd, struct iovec *vecs, unsigned long count, loff_t from, 
		size_t *retlen, u_char *eccbuf, int oobsel);
	int (*writev) (struct mtd_info *mtd, const struct iovec *vecs, unsigned long count, loff_t to, size_t *retlen);
	int (*writev_ecc) (struct mtd_info *mtd, const struct iovec *vecs, unsigned long count, loff_t to, 
		size_t *retlen, u_char *eccbuf, int oobsel);

	/* Sync */
	void (*sync) (struct mtd_info *mtd);

	/* Chip-supported device locking */
	int (*lock) (struct mtd_info *mtd, loff_t ofs, size_t len);
	int (*unlock) (struct mtd_info *mtd, loff_t ofs, size_t len);

	/* Power Management functions */
	int (*suspend) (struct mtd_info *mtd);
	void (*resume) (struct mtd_info *mtd);
#endif

	void *priv;
};

#endif


/*
 * Searches for a NAND device
 */
extern int nand_scan (struct mtd_info *mtd);

/*
 * Constants for hardware specific CLE/ALE/NCE function
*/
#define NAND_CTL_SETNCE 	1
#define NAND_CTL_CLRNCE		2
#define NAND_CTL_SETCLE		3
#define NAND_CTL_CLRCLE		4
#define NAND_CTL_SETALE		5
#define NAND_CTL_CLRALE		6

/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0		0
#define NAND_CMD_READ1		1
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READOOB	0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_READID		0x90
#define NAND_CMD_ERASE2		0xd0
#define NAND_CMD_RESET		0xff

/* 
 * Constants for ECC_MODES
 *
 * NONE:	No ECC
 * SOFT:	Software ECC 3 byte ECC per 256 Byte data
 * HW3_256:	Hardware ECC 3 byte ECC per 256 Byte data
 * HW3_512:	Hardware ECC 3 byte ECC per 512 Byte data
 *
 *
*/
#define NAND_ECC_NONE		0
#define NAND_ECC_SOFT		1
#define NAND_ECC_HW3_256	2
#define NAND_ECC_HW3_512	3
#define NAND_ECC_HW6_512	4
#define NAND_ECC_DISKONCHIP	5

/*
 * Constants for Hardware ECC
*/
#define NAND_ECC_READ		0
#define NAND_ECC_WRITE		1
	
/*
 * Enumeration for NAND flash chip state
 */
typedef enum {
	FL_READY,
	FL_READING,
	FL_WRITING,
	FL_ERASING,
	FL_SYNCING
} nand_state_t;


/*
 * NAND Private Flash Chip Data
 *
 * Structure overview:
 *
 *  IO_ADDR_R - address to read the 8 I/O lines of the flash device 
 *
 *  IO_ADDR_W - address to write the 8 I/O lines of the flash device 
 *
 *  hwcontrol - hardwarespecific function for accesing control-lines
 *
 *  dev_ready - hardwarespecific function for accesing device ready/busy line
 *
 *  waitfunc - hardwarespecific function for wait on ready
 *
 *  calculate_ecc - function for ecc calculation or readback from ecc hardware
 *
 *  correct_data - function for ecc correction, matching to ecc generator (sw/hw)
 *
 *  enable_hwecc - function to enable (reset) hardware ecc generator
 *
 *  eccmod - mode of ecc: see constants
 *
 *  eccsize - databytes used per ecc-calculation
 *
 *  chip_delay - chip dependent delay for transfering data from array to read regs (tR)
 *
 *  chip_lock - spinlock used to protect access to this structure
 *
 *  wq - wait queue to sleep on if a NAND operation is in progress
 *
 *  state - give the current state of the NAND device
 *
 *  page_shift - number of address bits in a page (column address bits)
 *
 *  data_buf - data buffer passed to/from MTD user modules
 *
 *  data_cache - data cache for redundant page access and shadow for
 *		 ECC failure
 *
 *  cache_page - number of last valid page in page_cache 
 */
struct nand_chip {
	unsigned long 	IO_ADDR_R;
	unsigned long 	IO_ADDR_W;
	void 		(*hwcontrol)(int cmd);
	int  		(*dev_ready)(void);
	void 		(*cmdfunc)(struct mtd_info *mtd, unsigned command, int column, int page_addr);
	int 		(*waitfunc)(struct mtd_info *mtd, struct nand_chip *this, int state);
	void		(*calculate_ecc)(const u_char *dat, u_char *ecc_code);
	int 		(*correct_data)(u_char *dat, u_char *read_ecc, u_char *calc_ecc);
	void		(*enable_hwecc)(int mode);
	int		eccmode;
	int		eccsize;
	int 		chip_delay;
#if 0 // NCB 25/11/2002
	spinlock_t 	chip_lock;
	wait_queue_head_t wq;
#endif
	nand_state_t 	state;
	int 		page_shift;
	u_char 		*data_buf;
	u_char		*data_poi;
	u_char 		*data_cache;
	int		cache_page;
};

/*
 * NAND Flash Manufacturer ID Codes
 */
#define NAND_MFR_TOSHIBA	0x98
#define NAND_MFR_SAMSUNG	0xec
#define NAND_MFR_FUJITSU	0x4	/* added NCB 11/4/02 */

/*
 * NAND Flash Device ID Structure
 *
 * Structure overview:
 *
 *  name - Complete name of device
 *
 *  manufacture_id - manufacturer ID code of device.
 *
 *  model_id - model ID code of device.
 *
 *  chipshift - total number of address bits for the device which
 *              is used to calculate address offsets and the total
 *              number of bytes the device is capable of.
 *
 *  page256 - denotes if flash device has 256 byte pages or not.
 *
 *  pageadrlen - number of bytes minus one needed to hold the
 *               complete address into the flash array. Keep in
 *               mind that when a read or write is done to a
 *               specific address, the address is input serially
 *               8 bits at a time. This structure member is used
 *               by the read/write routines as a loop index for
 *               shifting the address out 8 bits at a time.
 *
 *  erasesize - size of an erase block in the flash device.
 */
struct nand_flash_dev {
	char * name;
	int manufacture_id;
	int model_id;
	int chipshift;
	char page256;
	char pageadrlen;
	unsigned long erasesize;
};

/*
* Constants for oob configuration
*/
#define NAND_BADBLOCK_POS		5

#define NAND_NONE_OOB			0
#define NAND_JFFS2_OOB			1
#define NAND_YAFFS_OOB			2

#define NAND_NOOB_ECCPOS0		0
#define NAND_NOOB_ECCPOS1		1
#define NAND_NOOB_ECCPOS2		2
#define NAND_NOOB_ECCPOS3		3
#define NAND_NOOB_ECCPOS4		6
#define NAND_NOOB_ECCPOS5		7

#define NAND_JFFS2_OOB_ECCPOS0		0
#define NAND_JFFS2_OOB_ECCPOS1		1
#define NAND_JFFS2_OOB_ECCPOS2		2
#define NAND_JFFS2_OOB_ECCPOS3		3
#define NAND_JFFS2_OOB_ECCPOS4		6
#define NAND_JFFS2_OOB_ECCPOS5		7

#define NAND_YAFFS_OOB_ECCPOS0		8
#define NAND_YAFFS_OOB_ECCPOS1		9
#define NAND_YAFFS_OOB_ECCPOS2		10
#define NAND_YAFFS_OOB_ECCPOS3		13
#define NAND_YAFFS_OOB_ECCPOS4		14
#define NAND_YAFFS_OOB_ECCPOS5		15

#define NAND_JFFS2_OOB8_FSDAPOS		6
#define NAND_JFFS2_OOB16_FSDAPOS	8
#define NAND_JFFS2_OOB8_FSDALEN		2
#define NAND_JFFS2_OOB16_FSDALEN	8

#endif /* __LINUX_MTD_NAND_H */
