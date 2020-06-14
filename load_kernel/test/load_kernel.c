/*-------------------------------------------------------------------------
 * Filename:      load_kernel.c
 * Version:       $Id: load_kernel.c,v 1.1 2001/08/14 21:17:40 jamey Exp $
 * Copyright:     Copyright (C) 2001, Russ Dill
 * Author:        Russ Dill <Russ.Dill@asu.edu>
 * Description:   interface between kernel loaders and boot loader.
 		  if everything was done right, this *should be the only
 		  file that needs changing for porting to different boot
 		  loaders.
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

#include <stdio.h>
#include <string.h>
typedef unsigned long	u32;
typedef unsigned short	u16;
typedef	unsigned char	u8;
#include "load_kernel.h"
#include "config.h"

extern struct kernel_loader cramfs_load;
extern struct kernel_loader zImage_load;
extern struct kernel_loader jffs2_load;


const struct kernel_loader *loader[] = {
	&cramfs_load,
	&zImage_load,
	&jffs2_load,
	NULL
};

void *fodder_ram_base;

/* function calls for the user space testing app */
inline void ldr_update_progress(void)
{
	putchar('.');
}

inline int ldr_strncmp(char *a, char *b, size_t n)
{
	return strncmp(a, b, n);
}

inline void *ldr_memcpy(void *dst, void *src, size_t n)
{
	return memcpy(dst, src, n);
}

inline void ldr_output_string(char *str)
{
	printf("%s", str);
	fflush(stdout);
}

inline void ldr_output_hex(u32 hex)
{
	printf("%lx", hex);
	fflush(stdout);
}

