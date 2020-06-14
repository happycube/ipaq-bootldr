/****************************************************************************/
/* Copyright 2000 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/

#include "bootldr.h"
#include "btflash.h"
#include "load_kernel/include/load_kernel.h"
#include "load_kernel/include/config.h"

#include <string.h>

extern struct kernel_loader cramfs_load;
extern struct kernel_loader zImage_load;
extern struct kernel_loader jffs2_load;

const struct kernel_loader *loader[] = {
	&zImage_load,
	&jffs2_load,
	&cramfs_load,
	NULL
};

inline void ldr_update_progress(void)
{
   putstr(".");
}

inline int ldr_strlen(const char *a)
{
   return strlen(a);
}

inline int ldr_strncmp(const char *a, const char *b, size_t n)
{
   return strncmp(a, b, n);
}

inline void *ldr_memcpy(void *dst, const void *src, size_t n)
{
   memcpy(dst, src, n);
   return dst;
}

inline void ldr_output_string(char *str)
{
   putstr(str);
}

inline void ldr_output_hex(u32 hex)
{
   putHexInt32(hex);
}

