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
/****************************************************************************/
/* Quickly written by George France france@crl.dec.com                      */
/****************************************************************************/

#include "bootldr.h" 
#include "bsdsum.h"


unsigned int bsd_sum_memory(unsigned long img_src, size_t img_size) 
{
  unsigned long checksum = 0;   /* The checksum mod 2^16. */
  unsigned char *pch;		/* Each character read. */
  size_t i;

  pch = (unsigned char *)img_src;
  for (i=1; i <= img_size; i++)
   {
      /* Do a right rotate */
      if (checksum & 01)
         checksum = (checksum >>1) + 0x8000;
      else
         checksum >>= 1;
      checksum += *pch;      /* add the value to the checksum */
      checksum &= 0xffff;  /* Keep it within bounds. */
      pch++;
    }
  return(checksum & 0xffff);
}

