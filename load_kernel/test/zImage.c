/*-------------------------------------------------------------------------
 * Filename:      zimage.c
 * Version:       $Id: zImage.c,v 1.1 2001/08/14 21:17:40 jamey Exp $
 * Copyright:     Copyright (C) 2001, Russ Dill
 * Author:        Russ Dill <Russ.Dill@asu.edu>
 * Description:   Module to load kernel straight from flash
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

#include "types.h"
#include "load_kernel.h"

#define ZIMAGE_MAGIC 0x016f2818

static int zImage_check_magic(struct part_info *part)
{		
	return ((u32 *) part->offset)[9] == ZIMAGE_MAGIC;
}

static u32 zImage_load_kernel(u32 *dest, struct part_info *part, const char *kernel_filename)
{
	unsigned long size;
	int i;
	size = ((u32 *) part->offset)[11] - ((u32 *) part->offset)[10];

	/* yes, it could be copied all at once, but I wanted to call 
	 * ldr_update_progress */
	for (i = 0; i <= size - 0x40000; i += 0x40000) {
		ldr_update_progress();
		ldr_memcpy((char *) dest + i, part->offset + i, 0x40000);
	}
	ldr_memcpy((char *) dest + i, part->offset + i, size - i);
	
	return size;
}

struct kernel_loader zImage_load = {
	check_magic: zImage_check_magic,
	load_kernel: zImage_load_kernel,
	name:	     "zImage"
};
	
