/*-------------------------------------------------------------------------
 * Filename:      load_kernel.c
 * Version:       $Id: load_kernel.c,v 1.1 2001/08/14 21:17:39 jamey Exp $
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

#include "types.h"
#include "load_kernel.h"
#include "config.h"
#include "serial.h"
#include "util.h"
#include "main.h"

#ifdef CRAMFS_LOAD
extern struct kernel_loader cramfs_load;
#endif
#ifdef ZIMAGE_LOAD
extern struct kernel_loader zImage_load;
#endif
#ifdef JFFS2_LOAD
extern struct kernel_loader jffs2_load;
#endif


const struct kernel_loader *loader[] = {
#ifdef CRAMFS_LOAD
	&cramfs_load,
#endif
#ifdef ZIMAGE_LOAD
	&zImage_load,
#endif
#ifdef JFFS2_LOAD
	&jffs2_load,
#endif
	NULL
};

const void *fodder_ram_base = (void *) FODDER_RAM_BASE;

/* function calls for blob */
inline void ldr_update_progress(void)
{
	SerialOutputByte('.');
}

inline int ldr_strncmp(char *a, char *b, size_t n)
{
	return MyStrNCmp(a, b, n);
}

inline void *ldr_memcpy(void *dst, void *src, size_t n)
{
	MyMemCpyChar(dst, src, n);
	return dst;
}

inline void ldr_output_string(char *str)
{
	SerialOutputString(str);
}

inline void ldr_output_hex(u32 hex)
{
	SerialOutputHex(hex);
}

