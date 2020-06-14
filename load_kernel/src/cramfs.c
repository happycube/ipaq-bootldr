/*-------------------------------------------------------------------------
 * Filename:      cramfs.c
 * Version:       $Id: cramfs.c,v 1.1 2001/08/14 21:17:39 jamey Exp $
 * Copyright:     Copyright (C) 2001, Russ Dill
 * Author:        Russ Dill <Russ.Dill@asu.edu>
 * Description:   Module to load kernel from a cramfs
 *-----------------------------------------------------------------------*/
/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
/* note: this module doesn't currently support symlinks, but because of the
 * nature of cramfs, this shouldn't be a big deal */

#include "types.h"
#include "load_kernel.h"
#include "mini_inflate.h"

#define CRAMFS_MAGIC		0x28cd3d45	/* some random number */

struct cramfs_inode { /* sizeof == 12*/
	u32 mode:16, uid:16;
	/* SIZE for device files is i_rdev */
	u32 size:24, gid:8;
	/* NAMELEN is the length of the file name, divided by 4 and
           rounded up.  (cramfs doesn't support hard links.) */
	/* OFFSET: For symlinks and non-empty regular files, this
	   contains the offset (divided by 4) of the file data in
	   compressed form (starting with an array of block pointers;
	   see README).  For non-empty directories it is the offset
	   (divided by 4) of the inode of the first file in that
	   directory.  For anything else, offset is zero. */
	u32 namelen:6, offset:26;
};

/*
 * Superblock information at the beginning of the FS.
 */
struct cramfs_super {		/* sizeof == 62 == 0x40 + 12 */
	u32 magic;		/* 0x28cd3d45 - random number */
	u32 size;		/* Not used.  mkcramfs currently
                                   writes a constant 1<<16 here. */
	u32 flags;		/* 0 */
	u32 future;		/* 0 */
	u8 signature[16];	/* "Compressed ROMFS" */
	u8 fsid[16];		/* random number */
	u8 name[16];		/* user-defined name */
	struct cramfs_inode root;	/* Root inode data */
};

static int cramfs_check_magic(struct part_info *part)
{		
	return *((u32 *) part->offset) == CRAMFS_MAGIC;
}

static long cramfs_uncompress_page(char *dest, char *src, u32 srclen)
{
	long size;
	src++; /* *src == 0x78 method */
	src++; /* *src == 0x9C flags (!PRESET_DICT) */

	size = decompress_block(dest, src, ldr_memcpy);	
	return size;
}


static long cramfs_load_file(u32 *dest, struct part_info *part, 
			     struct cramfs_inode *inode)
{

	long int curr_block;
	unsigned long *block_ptrs;
	long size, total_size = 0;
	int i;

	ldr_update_progress();
	block_ptrs = ((u32 *) part->offset) + inode->offset;
	curr_block = (inode->offset + ((inode->size + 4095) >> 12)) << 2;
	for (i = 0; i < ((inode->size + 4095) >> 12); i++) {
		size = cramfs_uncompress_page((char *) dest, 
					      curr_block + part->offset, 
					      block_ptrs[i] - curr_block);
		if (size < 0) return size;
		((char *) dest) += size;
		/* Print a '.' every 0x40000 bytes */
		if (((total_size + size) & ~0x3ffff) - (total_size & ~0x3ffff))
			ldr_update_progress();
		total_size += size;
		curr_block = block_ptrs[i];
	}
	return total_size;
}

static u32 cramfs_load_kernel(u32 *dest, struct part_info *part, const char *kernel_filename)
{
	struct cramfs_super *super;
	struct cramfs_inode *inode;
	unsigned long next_inode;
	char *name;
	long size;
	
	size = -1;
	super = (struct cramfs_super *) part->offset;
	next_inode = super->root.offset << 2;
	
	while (next_inode < (super->root.offset << 2) + super->root.size) {
		inode = (struct cramfs_inode *) (next_inode + part->offset);
		next_inode += (inode->namelen << 2) + 
			       sizeof(struct cramfs_inode);
		
		if (!(inode->mode & 0x8000)) continue; /* Is it a file? */

		/* does it have 5 to 8 characters? */
		if (!inode->namelen == 2) continue;
		name = (char *) (inode + 1);
		if (!ldr_strncmp(kernel_filename, name, ldr_strlen(kernel_filename)+1)) {
			size = cramfs_load_file(dest, part, inode);
			break;
		}
	}
	return size <= 0 ? 0 : size;
}

struct kernel_loader cramfs_load = {
	check_magic: cramfs_check_magic,
	load_kernel: cramfs_load_kernel,
	name:	     "cramfs"
};
	
	
