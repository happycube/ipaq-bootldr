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


#include <stdlib.h>
#include <stdio.h>
#ifdef __linux__
#include <error.h>
#endif

unsigned long checksum = 0;     /* The BSD checksum mod 2^16 */
unsigned long totalbytes = 0;
extern   int  errno;

int bsd_checksum_bootldr()
{
  FILE *fp;
  unsigned char ch;		/* Each character read. */

  fp = fopen ("bootldr", "rb");
  if (fp == NULL)
     {
#ifdef __linux__
         error (0, errno, "%s", "bootldr file");
#else
	 perror("bootldr file");
#endif
         exit( EXIT_FAILURE);
     }
  ch = getc(fp);
  while ( !feof(fp))
    {
      totalbytes++;
      /* Do a right rotate */
      if (checksum & 01)
         checksum = (checksum >> 1) + 0x8000;
      else
         checksum >>= 1;
      checksum += ch;      /* add the value to the checksum */
      checksum &= 0xffff;  /* Keep it within bounds. */
      ch = getc(fp);
    }
  if (ferror (fp))
    {
#ifdef __linux__
      error (0, errno, "%s", "bootldr file");
#else
      perror("bootldr file");
#endif
      fclose (fp);
      exit( EXIT_FAILURE );
    }
  printf("total bytes >%d\n", totalbytes);
  return(checksum & 0xffff);
}

/* addes values to the end of the bootldr file to make the checksum zero */

int zero_bsd_checksum_bootldr() {

  FILE *fp;
  unsigned char ch;
  int firstbit;     /*  keeps the value of the first bit */
  int i;		

  fp = fopen ("bootldr", "ab");
  if (fp == NULL)
     {
#ifdef __linux__
         error (0, errno, "%s", "bootldr file");
#else
	 perror("bootldr file");
#endif
         exit( EXIT_FAILURE);
     }
  firstbit = checksum & 01;
  for (i=0; i < 15; i++) {
    checksum >>= 1;
    if (checksum & 01)
       putc( 0, fp );
    else
       putc( 1, fp );
  }
  if (firstbit & 01) 
     putc( 1, fp );
  else
     putc( 2, fp );
  totalbytes = totalbytes + 16;
  /* pad with zeros, because the xmodem in the bootloader only gets the
     size correct to the nearest 128 bytes */
  for (i=1; i <= (128 - totalbytes % 128); i++ )
    putc( 0, fp );
  if (ferror (fp))
    {
#ifdef __linux__
      error (0, errno, "%s", "bootldr file");
#else
      perror("bootldr file");
#endif
      fclose (fp);
      exit( EXIT_FAILURE );
    }
  fclose(fp);

}

int main (int argc, char **argv)
{
  /* This is a simple program, it:

     Computes the BSD checksum
     Computes the values to be added to the end of the 'bootldr' file,
              which gives a new checksum a value of zero. 
     Adds the Computed values to the end of the bootldr file.
  */
     bsd_checksum_bootldr();
     if (checksum != 0) {
        zero_bsd_checksum_bootldr();
        printf("Bootldr BSD sum zeroed\n");
     }
     exit( EXIT_SUCCESS );
}




