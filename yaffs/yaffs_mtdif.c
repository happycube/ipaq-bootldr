/*
 * YAFFS: Yet another FFS. A NAND-flash specific file system. 
 * yaffs_mtdif.c  NAND mtd wrapper functions.
 *
 * Copyright (C) 2002 Aleph One Ltd.
 *   for Toby Churchill Ltd and Brightstar Engineering
 *
 * Created by Charles Manning <charles@aleph1.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

const char *yaffs_mtdif_c_version = "$Id: yaffs_mtdif.c,v 1.1 2003/04/08 13:24:16 jamey Exp $";

#ifdef CONFIG_YAFFS_MTD_ENABLED
 
#include "yportenv.h"

#include "yaffs_mtdif.h"

#include "yaffs_mtdif.h"

// NCB change for bootldr inclusion
#if defined(CONFIG_YAFFS_BOOTLDR)
#include "bootldr.h"
#include "nand/nand.h"

unsigned yaffs_traceMask = YAFFS_TRACE_ALWAYS | YAFFS_TRACE_BAD_BLOCKS;

#else

#include "linux/mtd/mtd.h"
#include "linux/types.h"
#include "linux/time.h"

#ifndef	CONFIG_YAFFS_USE_OLD_MTD
#include "linux/mtd/nand.h"
#endif
#endif

int nandmtd_WriteChunkToNAND(yaffs_Device *dev,int chunkInNAND,const __u8 *data, yaffs_Spare *spare)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	size_t dummy;
    int retval = 0;
	
	loff_t addr = ((loff_t)chunkInNAND) * dev->nBytesPerChunk;
	
	__u8 *spareAsBytes = (__u8 *)spare;

#if 1
	if (spareAsBytes) {
	    if (spareAsBytes[5]!=0xff)
		putLabeledWord("nandmtd_WriteChunkToNAND: at block mark, writing 0x\n",spareAsBytes[5]);
	}
#endif

#ifndef	CONFIG_YAFFS_USE_OLD_MTD
	if(data && spare)
	{
		if(dev->useNANDECC)
			mtd->write_ecc(mtd,addr,dev->nBytesPerChunk,&dummy,data,spareAsBytes,NAND_YAFFS_OOB);
		else
			mtd->write_ecc(mtd,addr,dev->nBytesPerChunk,&dummy,data,spareAsBytes,NAND_NONE_OOB);
	}
	else
	{
#endif
	if(data)
		retval = mtd->write(mtd,addr,dev->nBytesPerChunk,&dummy,data);
	if(spare)
		retval = mtd->write_oob(mtd,addr,YAFFS_BYTES_PER_SPARE,&dummy,spareAsBytes);
#ifndef	CONFIG_YAFFS_USE_OLD_MTD
	}
#endif

    if (retval == 0)
    	return YAFFS_OK;
    else
        return YAFFS_FAIL;
}

int nandmtd_ReadChunkFromNAND(yaffs_Device *dev,int chunkInNAND, __u8 *data, yaffs_Spare *spare)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	size_t dummy;
    int retval = 0;
	
	loff_t addr = ((loff_t)chunkInNAND) * dev->nBytesPerChunk;
	
	__u8 *spareAsBytes = (__u8 *)spare;
	
#ifndef	CONFIG_YAFFS_USE_OLD_MTD
	if(data && spare)
	{
		if(dev->useNANDECC)
		{
		        u8 tmpSpare[ YAFFS_BYTES_PER_SPARE + (2*sizeof(int)) ];
			retval = mtd->read_ecc(mtd,addr,dev->nBytesPerChunk,&dummy,data,tmpSpare,NAND_YAFFS_OOB);
		        memcpy(spareAsBytes, tmpSpare, YAFFS_BYTES_PER_SPARE);
		}
		else
		{
			retval = mtd->read_ecc(mtd,addr,dev->nBytesPerChunk,&dummy,data,spareAsBytes,NAND_NONE_OOB);
		}
	}
	else
	{
#endif
	if(data)
		retval = mtd->read(mtd,addr,dev->nBytesPerChunk,&dummy,data);
	if(spare)
		retval = mtd->read_oob(mtd,addr,YAFFS_BYTES_PER_SPARE,&dummy,spareAsBytes);
#ifndef	CONFIG_YAFFS_USE_OLD_MTD
	}
#endif

    if (retval == 0)
    	return YAFFS_OK;
    else
        return YAFFS_FAIL;
}


#ifndef CONFIG_YAFFS_MTD_ENABLED
static void nandmtd_EraseCallback(struct erase_info *ei)
{
	yaffs_Device *dev = (yaffs_Device *)ei->priv;	
	up(&dev->sem);
}
#endif

int nandmtd_EraseBlockInNAND(yaffs_Device *dev, int blockNumber)
{
	struct mtd_info *mtd = (struct mtd_info *)(dev->genericDevice);
	__u32 addr = ((loff_t) blockNumber) * dev->nBytesPerChunk * dev->nChunksPerBlock;
#ifdef CONFIG_YAFFS_MTD_ENABLED
    return mtd->erase(mtd,addr,YAFFS_BYTES_PER_BLOCK) ? YAFFS_FAIL:YAFFS_OK;
#else
	struct erase_info ei;
    int retval = 0;
	
	ei.mtd = mtd;
	ei.addr = addr;
	ei.len = dev->nBytesPerChunk * dev->nChunksPerBlock;
	ei.time = 1000;
	ei.retries = 2;
	ei.callback = nandmtd_EraseCallback;
	ei.priv = (u_long)dev;
	
	// Todo finish off the ei if required
	
	sema_init(&dev->sem,0);

	retval = mtd->erase(mtd,&ei);	
	
	down(&dev->sem); // Wait for the erasure to complete

    if (retval == 0)	
    	return YAFFS_OK;
    else
        return YAFFS_FAIL;
#endif
}

int nandmtd_InitialiseNAND(yaffs_Device *dev)
{
	return YAFFS_OK;
}

#endif // CONFIG_YAFFS_MTD_ENABLED

