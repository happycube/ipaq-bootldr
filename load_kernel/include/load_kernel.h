/*-------------------------------------------------------------------------
 * Filename:      load_kernel.h
 * Version:       $Id: load_kernel.h,v 1.2 2002/10/30 15:48:44 jamey Exp $
 * Copyright:     Copyright (C) 2001, Russ Dill
 * Author:        Russ Dill <Russ.Dill@asu.edu>
 * Description:   header for load kernel modules
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

#ifndef _LOAD_KERNEL_H_
#define _LOAD_KERNEL_H_

/* this struct is very similar to mtd_info */
struct part_info {
	u32 size;	 // Total size of the Partition

	/* "Major" erase size for the device. Naïve users may take this
	 * to be the only erase size available, or may use the more detailed
	 * information below if they desire
	 */
	u32 erasesize;
	
	/* Where in memory does this partition start? */
	char *offset;
};

struct kernel_loader {
	
	/* Return true if there is a kernel contained at src */
	int (* check_magic)(struct part_info *part);
	
	/* load the kernel from the partition part to dst, return the number
	 * of bytes copied if successful, zero if not */
	u32 (* load_kernel)(u32 *dst, struct part_info *part, const char *kernel_filename);

	/* A brief description of the module (ie, "cramfs") */
	char *name;
};

extern const struct kernel_loader *loader[];

#ifndef USER_SPACE_TEST
/* this is a large area of ram for the loaders to use as a scratchpad */
extern void *fodder_ram_base;
#define UDEBUG(str, args...) 
#else
extern void *fodder_ram_base;
extern int printf(const char *, ...);
#define UDEBUG(str, args...) printf(str, ## args)
#endif

/* the modules call this every 0x40000 bytes to update a progress bar */
inline void ldr_update_progress(void);

/* self explanitory */
inline int ldr_strlen(const char *a);
inline int ldr_strncmp(const char *a, const char *b, size_t n);
inline void *ldr_memcpy(void *dst, const void *src, size_t n);

/* the first one outputs a string to the serial port, the second a u32 hex # */
inline void ldr_output_string(char *str);
inline void ldr_output_hex(u32 hex);


#endif /* _LOAD_KERNEL_H_ */
