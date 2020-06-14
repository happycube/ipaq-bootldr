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
/* This is a simple program, it:

     Computes the BSD checksum
     Computes the values to be added to the end of the 'bootldr' file,
              which gives a new checksum a value of zero. 
     Adds the Computed values to the end of the bootldr file.
*/
/* Rewritten 17-Dec-2000 Dirk van Hennekeler                                */
/*                       <dirk.vanhennekeler@compaq.com>                    */
/* Description:                                                             */
/*              Calcualates a BSD checksum and modifies the file (if        */
/*              necessary) so that if the checksum is again calculated it   */
/*              will be zero.                                               */
/****************************************************************************/


#include <stdlib.h>
#include <stdio.h>
#if defined(__linux__) || defined(__FreeBSD__) || defined(__QNXNTO__)
#include <string.h>
#endif
#if defined(__linux__)
#include <error.h>
#include <errno.h>
#endif
#include <sys/stat.h>

#define ZEROSUM_VERSION_MAJOR 0
#define ZEROSUM_VERSION_MINOR 2
#define ZEROSUM_VERSION_MICRO 0
char DEFAULT_TARGETFILE[]="bootldr";

#define ERROR_ZEROSUM_NO_ERROR        0
#define ERROR_ZEROSUM_BAD_ARGS        2
#define ERROR_ZEROSUM_BAD_VERSION     3
#define ERROR_ZEROSUM_BAD_FILE        4

extern   int  errno;

#ifndef __linux__
char buf[256];
int error(FILE *fp, int errno, char *fmt, char *str)
{
    char buf[256];
    sprintf(buf, fmt, str);
    printf(buf);
    return 0;
}
#endif



struct user_args {
    char           *program_name;
    char           *target_filename;
    int             verbose;
    int             checksum_only;
    int             show_version;
    int             quiet;
};

static void show_usage(char * prog_name)
{
    printf("Modify a file to generate a zero BSD checksum v%d.%d.%d\r\n", 
        ZEROSUM_VERSION_MAJOR, 
	ZEROSUM_VERSION_MINOR, 
	ZEROSUM_VERSION_MICRO) ;

    printf("\nusage: %s [-cv] [filename]\n",prog_name);
    printf("   filename        the name of the file to modify. \r\n");
    printf("                   If no filename is entered the file 'bootldr' will be\r\n");
    printf("                   used. This is to ensure backward compatibility.\r\n");
    printf("   -c              calculate a BSD checksum and display it, but do not\r\n");
    printf("                   modify the file.\r\n");
    printf("   -v              show version information about %s\r\n", prog_name);
}


static void show_version(char * prog_name)
{
    printf("%s v%d.%d.%d\r\n", prog_name,
        ZEROSUM_VERSION_MAJOR, ZEROSUM_VERSION_MINOR, ZEROSUM_VERSION_MICRO) ;
}


static int process_args(int argc, char *argv[], struct user_args *settings)
{
    int             mandatory_args    = 1;
    int             current_arg       = 1;
    unsigned int    current_switch    = 0;
    char          * name_start;


    settings->program_name = argv[0];

    if ( (name_start = strrchr(argv[0], '/'))  != 0 ||
         (name_start = strrchr(argv[0], '\\')) != 0 ) {
        settings->program_name = ++name_start;
    }

    if (argc > 3 )    {
        show_usage( settings->program_name );
        return -ERROR_ZEROSUM_BAD_ARGS;
    }
    else if (argc == 1) {
        settings->target_filename = DEFAULT_TARGETFILE;
        return ERROR_ZEROSUM_NO_ERROR;
    }


    if ( argv[current_arg][current_switch] == '-' ) {
        for( 	current_switch++; 
		current_switch < strlen(argv[current_arg]); 
		current_switch++) {
            switch (argv[current_arg][current_switch]) {
                case 'v':
                    show_version(settings->program_name);
                    settings->show_version = 1;
                    if ((argc == 2) && (current_switch+1 >= strlen(argv[current_arg]))) {
                        return ERROR_ZEROSUM_NO_ERROR; /* we are done */
                    }
                    break;

                case 'c':
                    settings->checksum_only = 1;
                    break;

                default:
                    show_usage( settings->program_name );
                    return -ERROR_ZEROSUM_BAD_ARGS;
            }
        }

        current_arg++;
    }


    /* now we know we can fill in the structure, so let's do it */
    if (argc - current_arg >= mandatory_args ) {
        settings->target_filename = argv[current_arg++];
    }
    else
    {
        settings->target_filename = DEFAULT_TARGETFILE;
    }



    return ERROR_ZEROSUM_NO_ERROR;
}


static int calculate_bsd_checksum(char *targetfile, unsigned int *checksum)
{
  FILE          *fp;
  unsigned char  ch;		/* Each character read. */
  int            rc = -1;   /* return code */

  if ( (fp = fopen (targetfile, "rb")) != NULL) 
  {
      *checksum = 0;

      ch = getc(fp);
      while ( !feof(fp) ) 
      {
          /* Do a right rotate */
          if (*checksum & 01)
              *checksum = (*checksum >> 1) + 0x8000;
          else
              *checksum >>= 1;
          
          *checksum += ch;      /* add the value to the checksum */
          *checksum &= 0xffff;  /* Keep it within bounds. */
          
          ch = getc(fp);
      }
      
      if (!ferror (fp))
      {
          printf("BSD checksum=0x%04x for file '%s'\n", *checksum, targetfile);
          rc = 0;
      }

      fclose(fp);
  }
  
  if (rc)
  {
      error (0, errno, "'%s'", targetfile);
  } 

  return rc;
}


/* addes values to the end of the bootldr file to make the checksum zero */
static int add_zero_bsd_checksum(char *targetfile)
{
  FILE         *fp;
  int           firstbit;     /*  keeps the value of the first bit */
  int           i;		
  unsigned int  checksum;
  int           rc;
  struct stat   targetfile_stat;
  unsigned long totalbytes;
  char  *       mods = "was already";
#define XMODEM_PACKET_SIZE 128

  if ((rc = calculate_bsd_checksum(targetfile, &checksum)) == 0) 
  {
      if ( (fp = fopen (targetfile, "ab")) != NULL) 
      {
          if (!stat(targetfile, &targetfile_stat))
          {
              /* if the checksum is already zero and the size is a multiple
                 of 128, don't do anything. */
              if ((targetfile_stat.st_size % XMODEM_PACKET_SIZE) || checksum)
              {
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
              
                  totalbytes = targetfile_stat.st_size + 16;
              
                  /* pad with zeros, because the xmodem in the bootloader only gets the
                  size correct to the nearest 128 bytes */
                  for (i=1; i <= (int)(XMODEM_PACKET_SIZE - totalbytes % XMODEM_PACKET_SIZE); i++ )
                  {
                      putc( 0, fp );
                  }

                  if (!ferror (fp))
                  {
                      printf("Modified '%s'. ", targetfile);
                      mods = "is now";
                      rc = 0;    
                  }
              }
          }

          fclose(fp);
      }
      if (rc)
      {
          error (0, errno, "'%s'", targetfile);
      }
      else
      {
          printf( "The BSD checksum for this file %s zero.\n", mods);
      }
  }
 
  return rc;
}


int zerosum(const struct user_args *settings)
{
    unsigned int checksum;

    if (settings->checksum_only)
    {
        return calculate_bsd_checksum(settings->target_filename, &checksum);
    }

    return add_zero_bsd_checksum(settings->target_filename);
}


int main (int argc, char **argv)
{
    int                 rc = EXIT_FAILURE;
    struct user_args    settings;

    memset(&settings, 0, sizeof(settings));

    if (!process_args(argc, argv, &settings)) {
        rc = zerosum(&settings);
    }

    return rc;
}




