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
/*
 * heap.h
 *
 */

int mmalloc_init(unsigned char *base, dword size);
void *mmalloc(dword size);
void mfree(void *block);

// for use by the zlib functions
void * zalloc(void* opaque,unsigned int items,unsigned int size);
void zfree(void* opaque,void *address);


#ifndef USER_MODE_TEST
#define malloc(x)   mmalloc(x)
#define free(x)	    mfree(x)
#endif
