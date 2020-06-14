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
 * bootldr file for Compaq Personal Server Bootloader
 *
 */


#if 0
#if defined(CONFIG_MACH_SKIFF)
     int tmp_mach_skiff = 0x0;     
     unsigned int *boot_flags_ptr=&tmp_mach_skiff;
#endif //defined(CONFIG_MACH_SKIFF)
#endif


#if defined(CONFIG_MACH_IPAQ) 

#if 0
#error "
*************
We are in the process of adding support for the iPAQ 3900.
We suggest that you not use a bootloader built from src for
iPAQ 31xx,36xx,37xx, or 38xx until the changes stabilize and
we have a chance to test the SA-1100 architectures.
*************
"
#endif


#endif


/*
 * Maintainer: Jamey Hicks (jamey@crl.dec.com)
 * Original Authors: Edwin Foo, Jamey Hicks, Dave Panariti, Mike Schexnaydre, Chris Joerg
 * June 29, 2000 - save commands and xmodem send added.
 *                             - George France <france@crl.dec.com>
 * July  3, 2000 - add commands to allow the baudrate to be changed.
 *                             - George France <france@crl.dec.com>
 * July  5, 2000 - extended the range of baudrate change.
 *                             - George France <france@crl.dec.com>
 * July  5, 2000 - added PF_LOGICAL.
 *                             - George France <france@crl.dec.com>
 * July  6, 2000 - added display command.
 *                             - George France <france@crl.dec.com>
 * July 31, 2000 - add Architecture field to boot loader.
 *                             - George France <france@crl.dec.com>
 * Aug  04, 2000 - made the cached and uncached flash size the 
 *                 same as the actual flash size.
 *                             - George France <france@crl.dec.com>
 * Aug  04, 2000 - Auto protected the boot sector after the boot
 *                 loader is loaded.
 *                             - George France <france@crl.dec.com> 
 * Nov  22, 2000 - JFFS commands and params added.
 *                             - John G Dorsey <john+@cs.cmu.edu>
 * Nov  24, 2000 - bootldr.conf support added.
 *                             - John G Dorsey <john+@cs.cmu.edu>
 * Mar  14, 2001 - YMODEM (with MD5) support added.
 *                             - John G Dorsey <john+@cs.cmu.edu>
 * Oct  31, 2001 - Support for PocketPCSE, Alan Smith
 *                 Same code base is use to build two bootldrs. One allows
 *                 full command line functionality, but erases the memory
 *                 before allowing the user control. The other only allows 
 *                 entry into wince and disables the command line interface. 
 *                 This is all controlled via CONFIG_SECURE definable. 
 *                 Makefile will build both files automatically.
 * 
 *                 bootldr-0000-<version>-disabled
 *                 bootldr-0000-<version>-erase
 *
 * Feb	13, 2002 - Updated memory erase algorithm - Alan Smith
 *                 PPC/4/1 & GEN-REV-0233
 *				
 *		   We now write six times to the same memory location
 *                 in one pass. All caching is disabled. This brings
 *                 the bootloader into line with the main PocketPC(SE)
 *                 algorithm.
 *
 *                 Code base now using CRLs 2-18-01 bootldr which supports 
 *                 iPAQ 3800 models.
 *
 *		   Image names are now:
 *                     bootldr-<version>-disabled
 *                     bootldr-<version>-erase
 */

static char bootprog_name[] = "Compaq OHH BootLoader";
static char bootprog_date[] = DATE;

static char cvsversion[] = "$Id: bootldr.c,v 1.207 2003/09/26 21:42:04 bavery Exp $";

#define USE_XMODEM


#include "commands.h"
#include "serial.h"
#include "bootldr.h"
#include "bootlinux.h"
#include "btpci.h"
#include "btflash.h"
#include "btusb.h"
#include "buttons.h"
#include "hal.h"
#include "h3600_sleeve.h"
#include "pcmcia.h"
#include "ide.h"
#include "fs.h"
#include "vfat.h"
#include "heap.h"
#include "params.h"
#include "partition.h"
#include "modem.h"
#include "xmodem.h"
#include "util.h"
#ifdef CONFIG_YMODEM
#include "ymodem.h"
#endif
#include "lcd.h"
#include "pbm.h"
#include "aux_micro.h"
#include "zlib.h"
#include "zUtils.h"
#include "jffs.h"
#include "fakelibc.h"
#if defined(CONFIG_MACH_SKIFF)
#include "skiff.h"
#endif //defined(CONFIG_MACH_SKIFF)
#if !defined(CONFIG_PXA)
#include "sa1100.h"
#else
#include "asm-arm/arch-pxa/h3900.h"
#endif
#include <string.h>
#include <ctype.h>

#if defined(CONFIG_MACH_IPAQ) 
#include <asm-arm/arch-sa1100/h3600_gpio.h>
#include <asm-arm/arch-sa1100/h3600_asic.h>	
void gpio_init( int mach_type );
#endif

#include "cpu.h"
#include "bsdsum.h"
#include "architecture.h"
#ifdef CONFIG_USE_DATE_CODE
extern char _bootldr_date_code[];
#endif
#ifdef CONFIG_LOAD_KERNEL
#include "lkernel.h"
#endif

#ifdef CONFIG_MD5
#include "md5.h"
#endif


#ifdef BZIPPED_KERNELS
#define BZ_NO_STDIO
#include "bzip/bzlib.h"
#endif

#define	DEB(s)	putstr(s)

#include "cyclone_boot.h"
		
#define ROUNDUP(v,szpwr2) (((v)+((szpwr2)-1))&~(szpwr2-1))

int do_splash = DO_SPLASH;
int ack_commands = 0;
int no_cmd_echo = 0;
int use_extended_getcmd = 1;
int check_for_func_buttons = CHECK_FOR_FUNC_BUTTONS;
long reboot_button_enabled = 0;

unsigned long last_ram_load_address = 0;
unsigned long last_ram_load_size = 0;

extern void *SerBase, *Ser1Base, *Ser3Base;

#ifdef CONFIG_REFLASH
extern int reflash_enabled;
#endif

#if defined(CONFIG_MACH_ASSABET)
unsigned int *assabet_bcr = (unsigned int *)ASSABET_BCR;
unsigned int *assabet_scr = (unsigned int *)(HEAP_START - sizeof(int));
#endif

extern void getcmd_ex(char*, size_t);

/* helper function */
static void command_load_flash_region(const char *regionName, unsigned long regionBase, unsigned long regionSize, int flags);

void discover_machine_type(long boot_flags);

void parseargs(char *argstr, int *argc_p, char **argv, char** resid);
void unparseargs(char *argstr, int argc, const char **argv);
void splash_file(const char * fileName, const char *partName);
void splash();

//
// PocketPC(SE)
//
#define ERASE_PATTERN1 0x55555555
#define ERASE_PATTERN2 0xAAAAAAAA

void eraseRAM( unsigned long pattern1, unsigned long pattern2 );

/**********************************************************
 * zlib tests
 * *******************************************************/

void command_tdz(int argc, const char* argv[]);
void body_tdz(unsigned long address,unsigned long size,unsigned long dAddress);

unsigned long autoboot_timeout = TIMEOUT;

void bootmenu(void);

void monitorConfigureMMU(void);

#if defined(CONFIG_GZIP) || defined(CONFIG_GZIP_REGION)
int gunzip_region(char*   src,    char*   dst,
		  long len, const char *name);
#endif

enum bootldr_image_status { 
	BOOTLDR_IMAGE_OK = 0, 
	BOOTLDR_NOT_OHHIMAGE,
	BOOTLDR_WRONG_ADDR,
	BOOTLDR_WRONG_ARCH,
	BOOTLDR_BAD_CHECKSUM,
	BOOTLDR_BAD_SIZE,
	BOOTLDR_BAD_MAGIC,
	BOOTLDR_NOT_LOADABLE,
};
       
BOOL isValidBootloader(unsigned long p,unsigned long size);
static enum bootldr_image_status isValidOHHImage(unsigned long p,unsigned long size);
static enum bootldr_image_status isValidParrotImage(unsigned long p,unsigned long size);

/*
 * this variable MUST be initialized explicitly to zero to prevent its
 * placement in the BSS.
 */
enum {
    mmue_enabled = 99,
    mmue_notenabled = 101
};

byte mmuEnabled = mmue_notenabled; /* we set this after we call enableMMU() */
#define isMmuEnabled()	(mmuEnabled == mmue_enabled)
#define setMmuEnabled()	(mmuEnabled =  mmue_enabled)

/******************* PARAMS ********************************/
/******************************** END PARAMS **************************************/


#ifdef ZBSS
/*
 * zero bss (using no bss vars!)
 */
void
zbss(void)
{
    char*	    start;
    unsigned	    len;
    aout_header*    hdrp;
    
    hdrp = (aout_header*)FLASH_BASE;
    start = (char*)hdrp + hdrp->a_text + hdrp->a_data;
    
    len = hdrp->a_bss;
    if (len) {
	memset(start, 0x00, len);
    }
    
}
#endif /* ZBSS */


static struct ebsaboot bootinfo = {
   BT_MAGIC_NUMBER,	/* boot info magic number */
   0x20000000,		/* virtual addr of arg page */
   0x20000000,		/* physical addr of arg page */
   NULL,		/* kernel args string pointer */
   (pd_entry_t *)MMU_TABLE_START,	/* active L1 page table */
   0,			/* start of physical memory */
   DRAM_SIZE,		/* end of physical memory */
   SZ_2M+SZ_1M,         /* start of avail phys memory */
   48000000,		/* fclk frequency */
   UART_BAUD_RATE,    	/* baudrate of serial console */
   1 			/* n stop bits */
};

#define	VER_SEP	    "-"

void print_bootldr_version2(
    char*   prefix)
{
  char vbuf[32];

  if (prefix)
      putstr(prefix);
  putstr(bootprog_name);
  putstr(", Rev ");
  dwordtodecimal(vbuf, VERSION_MAJOR); putstr(vbuf); putstr(VER_SEP);
  dwordtodecimal(vbuf, VERSION_MINOR); putstr(vbuf); putstr(VER_SEP);
  dwordtodecimal(vbuf, VERSION_MICRO); putstr(vbuf);
  if(strlen(VERSION_SPECIAL) > 0){
    putstr("-");
    putstr(VERSION_SPECIAL);
  }
#ifdef CONFIG_BIG_KERNEL
  putstr(" [BIG_KERNEL]");
#endif
#ifdef CONFIG_JFFS
  putstr(" [JFFS]");
#endif
#ifdef CONFIG_MD5
  putstr(" [MD5]");
#endif
  putstr(" [MONO]");
  putstr("\r\n");
}

void print_bootldr_version(void)
{
    print_bootldr_version2(">> ");
}

void
print_version_verbose(
    char*   prefix)
{
    print_bootldr_version2(prefix);

#ifdef CONFIG_USE_DATE_CODE
    if (prefix)
	putstr(prefix);
    putstr("Last link date: ");
    putstr(_bootldr_date_code);
    putstr("\n\r");
#endif
    if (prefix)
	putstr(prefix);

    putstr("Contact: bootldr@handhelds.org\r\n");
}

void print_banner(void)
{
  long armrev = 0;
  long armcomp;
  long armarch;
  long armpart;
  long cpsr = 0;
  __asm__("mrc p15, 0, %0, c0, c0, 0" : "=r" (armrev));
  __asm__("mrs %0, cpsr" : "=r" (cpsr));

  armcomp = (armrev & ARM_COMP_MASK) >> ARM_COMP_SHIFT;
  armarch = (armrev & ARM_ARCH_MASK) >> ARM_ARCH_SHIFT;
  armpart = (armrev & ARM_PART_MASK) >> ARM_PART_SHIFT;

  print_bootldr_version();

  putstr(">> ");
  putstr(bootprog_date);
#ifdef CONFIG_USE_DATE_CODE
  putstr("\n\r>> Last link date: ");
  putstr(_bootldr_date_code);
#endif
  putstr("\r\n>> Contact: bootldr@handhelds.org\r\n");
  
  putstr("\r\n");

  switch(armcomp){
      case ARM_ID_COMP_DEC:
	  putstr("Cpu company: DEC\r\n");	  
	  switch (armarch){
	      case ARM_ID_ARCH_SA:
		  putstr("Cpu Architecture: StrongArm\r\n");
		  switch (armpart){
		      case ARM_ID_PART_SA110:
			  putstr("Cpu Part: SA110\r\n");
			  break;
		      case ARM_ID_PART_SA1100:
			  putstr("Cpu Part: SA1100\r\n");
			  break;
		      default:
			  putLabeledWord("Unknown Part: ", armpart);
		  }		  
		  break;
	      default:
		  putLabeledWord("Unknown Architecture: ", armarch);
	  }	  
	  break;	  
      case ARM_ID_COMP_INTEL:
	  putstr("Cpu company: INTEL\r\n");
	  switch (armarch){
	      case ARM_ID_ARCH_SA:
		  putstr("Cpu Architecture: StrongArm\r\n");
		  switch (armpart){
		      case ARM_ID_PART_SA1110:
			  putstr("Cpu Part: SA1110\r\n");
			  break;
		      case ARM_ID_PART_SA1100:
			  putstr("Cpu Part: SA1100\r\n");
			  break;
		      default:
			  putLabeledWord("Unknown Part: ", armpart);
		  }
		  break;
	      case ARM_ID_ARCH_XSCALE:
		  putstr("Cpu Architecture: XScale\r\n");
		  switch (armpart){
		      case ARM_ID_PART_PXA250:
			  putstr("Cpu Part: PXA250 (Cotulla)\r\n");
			  break;
		      default:
			  putLabeledWord("Unknown Part: ", armpart);
		  }

		  break;
	      default:
		  putLabeledWord("Unknown Architecture: ", armarch);
	  }	  
	  break;
      default:
	  putLabeledWord("CPU made by unknown Company: ", armcomp);
	  
  }
  
  
  switch(armpart){
      case ARM_ID_PART_SA1110:
	  switch(armrev & ARM_REVISION_MASK){
	      case  0: putstr("revision A0\r\n"); break;
	      case  4: putstr("revision B0\r\n"); break;
	      case  5: putstr("revision B1\r\n"); break;
	      case  6: putstr("revision B2\r\n"); break;
	      case  8: putstr("revision B4\r\n"); break;
	      default: putLabeledWord("processor ID: ", armrev);
	  }
	  break;

      default:
	  putLabeledWord(">> ARM Processor Rev=", armrev & ARM_REVISION_MASK);

  }
  


#if CONFIG_MACH_SKIFF
  putLabeledWord(">>  Corelogic Rev=", *(volatile unsigned long *)(DC21285_ARMCSR_BASE + PCI_VENDOR_ID));
  putLabeledWord(">>  Corelogic fclk=", param_mem_fclk_21285.value);
#endif

#if defined(CONFIG_LOAD_KERNEL) || defined(CONFIG_ACCEPT_GPL)
  putstr(">> (c) 2000-2001 Compaq Computer Corporation, provided with NO WARRANTY under the terms of the GNU General Public License.\r\n");
#else
  putstr(">> (c) 2000-2001 Compaq Computer Corporation, provided with NO WARRANTY.\r\n");
#endif
  putstr(">> See http://www.handhelds.org/bootldr/ for full license and sources");
  putstr("Press Return to start the OS now, any other key for monitor menu\r\n");
}

void
bootldr_reset(
    void)
{
    clr_sleep_reset();
    putstr_sync("Rebooting...");
#if !defined(CONFIG_PXA)
    CTL_REG_WRITE(RSRR, RSRR_SWR);
#else
    CTL_REG_WRITE(OWER, 1);
    CTL_REG_WRITE(OSCR, 0xffffff00);
#endif    
}

int	pushed_chars_len = 0;
int	pushed_chars_idx = 0;
char	pushed_chars[128];


int
push_cmd_chars(
    char*   chars,
    int	    len)
{
    if (pushed_chars_len + len > sizeof(pushed_chars))
	return (-1);

    memcpy(&pushed_chars[pushed_chars_len], chars, len);
    pushed_chars_len += len;
    return 0;
}

int
get_pushed_char(
    void)
{
    int	    ret;

    if (pushed_chars_idx >= pushed_chars_len) {
	pushed_chars_idx = 0;
	pushed_chars_len = 0;
	ret = -1;
    }
    else
	ret = pushed_chars[pushed_chars_idx++] & 0xff;

    /*  putLabeledWord("gpc, ret: 0x", ret); */
    
    return(ret);
}

#ifdef CONFIG_MACH_SPOT
void spot_idle(void)
{
#define N_LIMIT 25

  unsigned long *olr = (unsigned long *)SPOT_OLR, olr_val;
  static int i = 0, n = 1, t = 0, greenish = 1, increasing = 1;
  static int times[] = {
    6000, 7243, 8409, 9423, 10222, 10755, 10990, 10911, 10524, 9853, 
    8939, 7841, 6627, 5373, 4159, 3061, 2147, 1476, 1089, 1010, 1245, 
    1778, 2577, 3591, 4757
  };

  if(greenish){

    if(i == n){
      i = 0;

      if(t < times[n - 1])
	++t;
      else {
	t = 0;
	n = increasing ? (n + 1) : (n - 1);
      }

      olr_val = 0xff00;
    } else {
      ++i;
      olr_val = 0x00ff;
    }

  } else {  /* reddish */

    if(i == n){
      i = 0;

      if(t < times[n - 1])
	++t;
      else {
	t = 0;
	n = increasing ? (n + 1) : (n - 1);
      }

      olr_val = 0x00ff;
    } else {
      ++i;
      olr_val = 0xff00;
    }

  }

  if(n == N_LIMIT)
    increasing = 0;
  else if(n == 0){
    n = 1;
    increasing = 1;
    greenish ^= 1;
  }

  *olr = olr_val;

}
#endif


#if defined(CONFIG_MACH_SKIFF)
/* Iterations for delay loop, assumes caches on: */


void delay_seconds(unsigned long units)
{
  volatile int i;
  unsigned long start_time;
  volatile unsigned long time;
  
  // first we need to load it
  CTL_REG_WRITE(SKIFF_RTLOAD, 0xFFFFFF);
  // then we need to enable it
  CTL_REG_WRITE(SKIFF_RTCR, SKIFF_RTC_ENABLE|SKIFF_RTC_FCLK_DIV256);
  start_time = CTL_REG_READ(SKIFF_RCNR);
  //putLabeledWord(__FUNCTION__ ": start_time = 0x",start_time);
  
  while (1){
          time = CTL_REG_READ(SKIFF_RCNR);
          //putLabeledWord(__FUNCTION__ ": time = 0x",time);
          if ((start_time - time) > (units*SKIFF_COUNTS_PER_SEC))
                  break;
  }
}
#else
/* Iterations for delay loop, assumes caches on: */
#define DELAY_UNIT    (15000000)
#define AWAITKEY_UNIT (1000000)

void delay_seconds(unsigned long units)
{
  volatile int i;
  unsigned long start_time = CTL_REG_READ(RCNR);
  for(i = 0; i < units * DELAY_UNIT; ++i) {
    unsigned long time = CTL_REG_READ(RCNR);
    if ((time - start_time) > units) {
      break;
    }
  } 
}

#endif

#define CMDBUFSIZE 256
char cmdBuffer[CMDBUFSIZE];
void getcmd(void) {
   byte curpos = 0; /* curpos position - index into cmdBuffer */
   byte c;
   byte noreturn = 1;

   /* first clear out the buffer.. */
   memset(cmdBuffer, 0, CMDBUFSIZE);

   /* here we go..*/

   while (noreturn) {
      c = getc();
      switch (c)
         {
	 case 0x08:
         case 0x06:
         case 0x07:
         case 0x7E:
         case 0x7F: /* backspace / delete */
            if (curpos) { /* we're not at the beginning of the line */
               curpos--;
               putc(0x08); /* go backwards.. */
               putc(' ');  /* overwrite the char */
               putc(0x08); /* go back again */
            }
            cmdBuffer[curpos] = '\0';
            break;
         case '\r':
         case '\n':
         case '\0':
            noreturn = 0;
            putc('\r');
            putc('\n');
            break;

         case CTL_CH('x'):
	    curpos = 0;
	    break;
	     
         default:
            if (curpos < CMDBUFSIZE) {
               cmdBuffer[curpos] = c;
               /* echo it back out to the screen */
	       if (!no_cmd_echo)
		   putc(c);
               curpos++;
            }
            break;
         }
   }
   /*
     putstr("COMMAND: ");
     putstr(cmdBuffer);
     for (c=0;c<CMDBUFSIZE;c++) {
       putHexInt8(cmdBuffer[c]);
       putc(' ');
     }
     putstr("\r\n");
   */

}

int argc;

enum ParseState {
   PS_WHITESPACE,
   PS_TOKEN,
   PS_STRING,
   PS_ESCAPE
};

enum ParseState stackedState;

void parseargs(char *argstr, int *argc_p, char **argv, char** resid)
{
  // const char *whitespace = " \t";
  int argc = 0;
  char c;
  enum ParseState lastState = PS_WHITESPACE;
   
  /* tokenize the argstr */
  while ((c = *argstr) != 0) {
    enum ParseState newState;

    if (c == ';' && lastState != PS_STRING && lastState != PS_ESCAPE)
	break;
    
    if (lastState == PS_ESCAPE) {
      newState = stackedState;
    } else if (lastState == PS_STRING) {
      if (c == '"') {
        newState = PS_WHITESPACE;
        *argstr = 0;
      } else {
        newState = PS_STRING;
      }
    } else if ((c == ' ') || (c == '\t')) {
      /* whitespace character */
      *argstr = 0;
      newState = PS_WHITESPACE;
    } else if (c == '"') {
      newState = PS_STRING;
      *argstr = 0;
      argv[argc++] = argstr + 1;
    } else if (c == '\\') {
      stackedState = lastState;
      newState = PS_ESCAPE;
    } else {
      /* token */
      if (lastState == PS_WHITESPACE) {
        argv[argc++] = argstr;
      }      
      newState = PS_TOKEN;
    }

    lastState = newState;
    argstr++;
  }

  if (0) { 
    int i;
    putLabeledWord("parseargs: argc=", argc);
    for (i = 0; i < argc; i++) {
      putstr("   ");
      putstr(argv[i]);
      putstr("\r\n");
    }
  }

  argv[argc] = NULL;
  if (argc_p != NULL)
    *argc_p = argc;

  if (*argstr == ';') {
      *argstr++ = '\0';
  }
  *resid = argstr;
}



// this is getting more compliacated,  this function will averride any of the
// args in argstr with tthe args from argv.  this will allow you to override the
// param linuxargs from the commandline. e.g. init=/myRC will override
// init=linuxRC from the params.
void unparseargs(char *argstr, int argc, const char **argv)
{
   int i;
   char *cutStart;
   char *cutEnd;
   char *paramEnd;
   int delta;   
   int j;
   
   
   for (i = 0; i < argc; i++) {
      if (argv[i] != NULL) {
         if ((paramEnd = strchr(argv[i],'=')))// we have an a=b arg
            {
	      int argstrlen = strlen(argstr);
	      int needlelen = 0;
	      char needle[256];
	       paramEnd++;
	       putstr("haystack = <");
	       putstr(argstr);
	       putstr(">\r\n");
   
	       needlelen = (int)(paramEnd - argv[i]);
	       if (needlelen >= sizeof(needle))
		 needlelen = sizeof(needle) - 1;

	       strncpy(needle, argv[i], needlelen);
	       needle[needlelen] = 0;
	       putstr("needle = <");
	       putnstr(needle,needlelen);
	       putstr(">\r\n");

	       if ((cutStart = (char *)strstr(argstr, needle)) != NULL){
                  // found a match
                  if (!(cutEnd = strchr(cutStart,' '))) {
                     cutEnd = argstr + argstrlen;
		  } else {
                     cutEnd++; // skip the space
                  }
                  delta = (int)(cutEnd - cutStart);		   
                  for (j=(int) (cutEnd - argstr); j < argstrlen; j++) 
                     argstr[j-delta] = argstr[j];
                  // new end of argstr
                  argstr[argstrlen - delta] = '\0';
		   
	       }
            }	   
         strcat(argstr, " ");
         strcat(argstr, argv[i]);
      }
   }
}

int parsecmd(struct bootblk_command *cmdlist, int argc, const char **argv)
{
   /* find the command name */
   const char *cmdname = argv[0];
   int cmdnum = 0;

#if 0
   putLabeledWord("parsecmd: argc=", argc);   
   putstr("parsecmd: cmdname=>"); putstr(cmdname); putstr("<\r\n");
#endif
   if (argc < 1)
      return -1;
   /* do a string compare for the first offset characters of cmdstr
      against each member of the cmdlist */
   while (cmdlist[cmdnum].cmdstr != NULL) {
      if (strcmp(cmdlist[cmdnum].cmdstr, cmdname) == 0)
         return(cmdnum);
     cmdnum++;
   }
   return(-1);
}

void
exec_string(
    char*   buf)
{
  int argc;
  char *argv[128];
  char*	resid;

  while (*buf) {
      memset(argv, 0, sizeof(argv));
      parseargs(buf, &argc, argv, &resid);
      if (argc > 0) {
		do_command(argc, (const char **)argv);
      } else {
		exec_string("help");
      }
      buf = resid;
  }
}

void bootmenu(void)
{
  clr_squelch_serial();
  serial->enabled = 1;

  use_extended_getcmd = param_cmdex.value;

  while (1) {
    if (!packetize_output)
	putstr("boot> ");
    if (use_extended_getcmd)
	getcmd_ex(cmdBuffer, CMDBUFSIZE);
    else
	getcmd();
    
    if (cmdBuffer[0]) {
        /*
	 * useful if calling a modified bootldr's bootmenu
	 * function from another place.
	 */
	if (strcmp("quit", cmdBuffer) == 0)
	    return;
	
	exec_string(cmdBuffer);
    }
    
    if (ack_commands)
	do_putnstr("ACK0\n", 5);
  }
}

COMMAND(partition, command_partition_show, "-- displays the partitions", BB_RUN_FROM_RAM);
SUBCOMMAND(partition, show, command_partition_show, "-- displays the partitions", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(partition, save, command_params_save, "-- same as params save", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(partition, delete, command_partition_delete, "<partition_name> -- deletes a partition", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(partition, reset, command_partition_reset, "-- resets all partitions to the default", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(partition, define, command_partition_define, "<name> <base> <size> <flags> \r\n-- flags: 16 for JFFS2, 2 for bootldr, 8 for rest-of-flash", BB_RUN_FROM_RAM, 0);

void command_partition_show(int argc, const char **argv)
{
  int i;
  if (partition_table != NULL) {
    putLabeledWord("npartitions=", partition_table->npartitions);
    for (i = 0; i < partition_table->npartitions; i++) {
      FlashRegion *partition = &partition_table->partition[i];
      putstr(&partition->name[0]); putstr("\r\n");
      putLabeledWord("  base: ", partition->base);
      putLabeledWord("  size: ", partition->size);
      putLabeledWord("   end: ", partition->size + partition->base);
      putLabeledWord(" flags: ", partition->flags); 
    }
  }
}

void command_partition_delete(int argc, const char **argv)
{
  if (argc == 3){
    btflash_delete_partition(argv[2]);
  }
  else{
    putstr("usage: partition delete part_name\r\n");
  }
}

void command_partition_reset(int argc, const char **argv)
{
  btflash_reset_partitions();
}

void command_partition_define(int argc, const char **argv)
{
  const char *namestr = argv[2];
  const char *basestr = argv[3];
  const char *sizestr = argv[4];
  const char *flagsstr = argv[5];
  char *name;
  unsigned long base, size, flags;
      
  if (argc < 6) {
    putstr("usage: partition define <name> <base> <size> <flags>\r\n");
    putstr("       flags  16 for JFFS2\r\n"); 
    return;
  }
  name = mmalloc(strlen(namestr)+1);
  if (name == NULL) {
    putstr("Error allocating space for name\r\n");
    return;
  }
  strcpy(name, namestr);

  if (strcmp(basestr, "+") == 0) {
    putstr("basestr is \"+\"\n\r");
    base = 0xffffffff;
  }
  else
    base = strtoul(basestr, NULL, 0);
  size = strtoul(sizestr, NULL, 0);
  if (strcmp(flagsstr, "jffs2") == 0) {
    flags = LFR_JFFS2;
  } else { 
    flags = strtoul(flagsstr, NULL, 0);
  }

  if (base < 128000 && ((flags&LFR_BOOTLDR) == 0)) {
    putstr("  bootldr sector must have flags set to 2\r\n"); 
    return;
  }

  btflash_define_partition(name, base, size, flags);
}



void
default_boot()
{
  while (1) {
     exec_string("boot");
     putstr("boot command failed, entering monitor\n\r");
     
     /* if default boot fails, drop into the bootmenu */
     bootmenu();
  }
}

#define N_MEM_SIZES 4
long mem_sizes[N_MEM_SIZES+1] = {
    SZ_128M,
    SZ_64M,
    SZ_32M,
    SZ_16M,
    0				/* endicator */
};

#if !(defined(CONFIG_MACH_SKIFF))
long
probe_ram_size(
    void)
{
    int	    i;
    long    mem_size;

    /* assumes one bank of DRAM and that DRAM_BASE0 is not in the cache */

    /*
     * Each bank on sa1110 is only 128M long.  if we write to 128M,
     * we really write to the second bank. So
     * we store a 128M val @ 0, since if we have 128M, the other
     * writes will succeed to their respective addresses and won't
     * appear at zero.  If we have 128M, then nothing will
     * clobber the 128M @ 0 and that is what we'll read
     * as the mem size.
     * If we add more banks, this will need to change.
     */
    *(long *)(DRAM_BASE0) = DRAM_MAX_BANK_SIZE;
    putstr("Probing bank0 memory size...\n\r");
    for (i = 0; ; i++) {
	mem_size = mem_sizes[i];
        if (mem_size >= DRAM_MAX_BANK_SIZE)
          continue;
	if (!mem_size)
	    break;
	*(long *)(DRAM_BASE0 + mem_size) = mem_size;
    }
    putLabeledWord(" bank0 memory size=", *(long *)(DRAM_BASE0));

    param_dram0_size.value = *(long *)(DRAM_BASE0);

    return (*(long *)DRAM_BASE0);
}
#endif // CONFIG_MACH_SKIFF
void
print_mem_size(
    char*   hdr,
    long    mem_size)
{
    char vbuf[32];
    putstr(hdr);
    putLabeledWord("0x", mem_size);
    dwordtodecimal(vbuf, mem_size >> 20);
    putstr(" in megs: "); putstr(vbuf); putstr("M\n\r");
}

#if !(defined(CONFIG_PXA1) || defined (CONFIG_MACH_SKIFF))
/* returns number of banks */
long
probe_ram_size2(long *dram_sizes)
{
     int	    i;
     long    mem_size;
     int     max_banks = DRAM_MAX_BANKS;
     int     n_banks = 0;
     int     bank;
     char    *dram_base;
     long    *dram_size_ptr = NULL;
     unsigned long mdcnfg = ABS_MDCNFG;
     long mem_saves[N_MEM_SIZES];
     long mem_save0;
     long mem_save4;
     putLabeledWord("MDCNFG=", mdcnfg);

     /* assumes one bank of DRAM and that DRAM_BASE0 is not in the cache */

     /*
      * Each bank is only 128M long.  if we write to 128M,
      * we really write to the second bank. So
      * we store a 128M val @ 0, since if we have 128M, the other
      * writes will succeed to their respective addresses and won't
      * appear at zero.  If we have 128M, then nothing will
      * clobber the 128M @ 0 and that is what we'll read
      * as the mem size.
      * If we add more banks, this will need to change.
      */
    
     for (bank = 0; bank < max_banks; bank++) {
	  int found = 0;
	  switch (bank) {
	  case 0:
	       dram_base = (char *)DRAM_BASE0;
	       dram_size_ptr = &param_dram0_size.value;
	       break;
#ifdef DRAM_BASE1
#warning DRAM_BASE1 defined
	  case 1:
	       if (!(mdcnfg&MDCNFG_BANK1_ENABLE))
		    continue;
	       dram_base = (char *)DRAM_BASE1;
	       dram_size_ptr = &param_dram1_size.value;
	       break;
#endif
	  default:
	       continue;
	  }
	  putLabeledWord("dram_size_ptr=", dram_size_ptr);

	  mem_save0 = *(long *)(dram_base);
	  mem_save4 = *(long *)(dram_base+4);
	  *(long *)(dram_base) = DRAM_MAX_BANK_SIZE;
	  *(long *)(dram_base+4) = 0xbeef;
	  putLabeledWord("Probing memory size bank=", bank);
	  putLabeledWord("  dram[0]=", mem_save0);
	  if (*(long *)(dram_base) != DRAM_MAX_BANK_SIZE) {
	       *(long *)(dram_base+0) = mem_save0;
	       *(long *)(dram_base+4) = mem_save4;
	       putLabeledWord("seems to be no dram in bank=", bank); 
	       break;
	  }
	  *(long *)(dram_base+4) = mem_save4;
	  /* probe, saving dram value in mem_saves */
	  for (i = 0; i < N_MEM_SIZES; i++) {
	       mem_size = mem_sizes[i];
	       if (mem_size < DRAM_MAX_BANK_SIZE) {
		    mem_saves[i] = *(long *)(dram_base + mem_size);
		    putLabeledWord("  mem_size=[i]", mem_size);
		    putLabeledWord("  mem_saves[i]=", mem_saves[i]);
		    *(long *)(dram_base + mem_size) = mem_size;
	       }
	  }
#if 0
	  /* flush the cache */
	  for (i = 0; i < 16384; i += 4) {
	       *((unsigned long*)CACHE_FLUSH_BASE) = 22;
	  }
#endif

	  /* now see if we read back a valid size */
	  mem_size = *(long *)dram_base;
	  putLabeledWord("maybe mem_size=", mem_size);
	  for (i = 0; i < N_MEM_SIZES; i++) {
	       if (mem_size == mem_sizes[i]) {
		    putLabeledWord("found=\r\n", mem_size); 
		    found = 1;
	       }
	  }

	  /* now restore the locations we poked */
	  for (i = 0; i < N_MEM_SIZES; i++) {
	       putLabeledWord("  mem_size[i]=", mem_sizes[i]);
	       putLabeledWord("  mem_saves[i]=", mem_saves[i]);
	       if (mem_sizes[i] < mem_size) {
		    *(long *)(dram_base + mem_size) = mem_saves[i];
	       }
	  }
	  *(long *)(dram_base) = mem_save0;
    
	  if (found) {
	       putLabeledWord(" memory size=", mem_size);

	       if (dram_sizes)
		    dram_sizes[bank] = mem_size;
	       if (dram_size_ptr)
		    *dram_size_ptr = mem_size;
	       n_banks++;
	  }
     }
     param_dram_n_banks.value = n_banks;
     return n_banks;
}

COMMAND(probe_ram, command_probe_ram, "-- probe for ram size and banks", BB_RUN_FROM_RAM);
void command_probe_ram(int argc, const char **argv)
{
  long dram_sizes[4];
  int n_banks = probe_ram_size2(dram_sizes);
  putLabeledWord("n_banks=", n_banks);
  putLabeledWord("dram_sizes[0]=", dram_sizes[0]);
  putLabeledWord("dram_sizes[1]=", dram_sizes[1]);
}
#endif



void bootldr_main(int call_bootmenu, int boot_flags)
{
     char	c;
     long icache_enabled, dcache_enabled;
     int need_to_enable_mmu = param_enable_mmu.value;
     extern void *_start;
     long dram_sizes[4];
     int n_banks = 1;

     if ((char*)&_start == (char*)FLASH_BASE) {
	  need_to_enable_mmu = 1;
     }
     // print assemblery
     //PrintHexWord2(0x0000dead);     
#if defined(CONFIG_MACH_SKIFF)
     boot_flags = BF_NORMAL_BOOT;
             
#endif //defined(CONFIG_MACH_SKIFF)
#if 0
     putLabeledWord("&_start=", (long)&_start);
     putLabeledWord("FLASH_BASE=", FLASH_BASE);
#endif

     *boot_flags_ptr = boot_flags; 
#if 0
     putLabeledWord("boot_flags_ptr=", (u32)boot_flags_ptr);
     putLabeledWord("*boot_flags_ptr=", *boot_flags_ptr);
     putLabeledWord("boot_flags=", boot_flags);
#endif


     /*
       At this stage, we assume:
       SDRAM is configured.
       UART is configured and ready to go.
       Initial PCI setup is done.
       Interrupts are OFF.
       MMU is DISABLED
     */  


#ifdef delete_me
     /* to allow wakeup from linux, the bootloader, and wince we use a ram sig.
      *  we need to put the bootloader sig here so it can wake up properly.  Linux
      * takes care of its own.
      */
  
     *p = 0x4242; // the number is meaningless, wince is recognized by 0x0 here.
#endif  

#ifdef CONFIG_MACH_JORNADA720
     (*((volatile int *)PPSR_REG)) &= ~(PPDR_LFCLK | 0x80);
     (*((volatile int *)PPDR_REG)) |= (PPDR_LFCLK | 0x80);
#endif

     /* 
      * set up MMU table:
      *  DRAM is cacheable
      *  cached and uncached images of Flash
      *  after MMU is enabled, bootldr is running in DRAM
      */
     if (need_to_enable_mmu) {
	  putstr("\r\nenabling mmu\r\n");
	  bootConfigureMMU();
#if !defined(CONFIG_PXA)
	  writeBackDcache(CACHE_FLUSH_BASE);
#endif 
	  enableMMU();
     }

     serial->enabled = !squelch_serial();

     /* flashword logically should be assigned in bootConfigureMMU,
      * but we can't do it until the MMU is enabled
      * because before then it resides in flash 
      * -Jamey 9/4/2000
      */
     if (need_to_enable_mmu || (long)&_start) {
	  putLabeledWord("setting flashword=", UNCACHED_FLASH_BASE);
	  flashword = (unsigned long *)UNCACHED_FLASH_BASE;
     } else {
	  putLabeledWord("setting flashword=", FLASH_BASE);
	  flashword = (unsigned long *)FLASH_BASE;
     }
     /* initialize the heap */
     mmalloc_init((unsigned char *)(HEAP_START), HEAP_SIZE);
  
#if !(defined(CONFIG_PXA) || defined(CONFIG_MACH_SKIFF))
     auxm_init();			/* init the aux microcontroller i/f */
#endif  
     /* figure out what kind of flash we have */
     btflash_init();
     // this must happen early, BEFORE the lcd is turned on
     discover_machine_type(boot_flags);

     initialize_by_mach_type();  
     if (need_to_enable_mmu) {
	  flashConfigureMMU(flash_size);
	  param_enable_mmu.value = 1;
     }

#ifdef CONFIG_PCI
     /* Configure the PCI devices */
#ifdef DEBUG
     putstr(" PCI");
#endif
     bootConfigurePCI();
#endif

     {
#ifdef CONFIG_MACH_SKIFF
	  long system_rev = get_system_rev();
#endif
	  long dram_size;
#ifdef CONFIG_MACH_SKIFF
	  dram_size = ((system_rev&SYSTEM_REV_MAJOR_MASK) == SYSTEM_REV_SKIFF_V2) ? DRAM_SIZE : SZ_16M;
#endif
#if defined(CONFIG_SA1100) || defined(CONFIG_PXA)
	  putstr("probing ram\r\n");
	  /* assumes one bank of DRAM and that DRAM_BASE0 is not in the cache */
	  dram_size = probe_ram_size();
	  if (dram_size == 0xffffffff)
	       putstr("DRAM size probe failed!\n\r");
	  /* if it is a normal boot, then we'll try probing two banks */
	  if (boot_flags & BF_NORMAL_BOOT){
	       n_banks = probe_ram_size2(dram_sizes);
	       putLabeledWord("n_banks=", n_banks);
	       putLabeledWord("dram_sizes[0]=", dram_sizes[0]);
	       putLabeledWord("dram_sizes[1]=", dram_sizes[1]);
	  }

#endif /* Bitsy, Assabet, Jornada720 */
#ifdef CONFIG_MACH_SKIFF
#if   defined(MEMCLK_33MHZ)
#warning 33MHZ override     
	  system_rev &= ~SYSTEM_REV_MEMCLK_MASK;
	  system_rev |= SYSTEM_REV_MEMCLK_33MHZ;
#elif defined(MEMCLK_48MHZ)
#warning 48MHz override     
	  system_rev &= ~SYSTEM_REV_MEMCLK_MASK;
	  system_rev |= SYSTEM_REV_MEMCLK_48MHZ;
#elif defined(MEMCLK_60MHZ)
#warning 60MHZ override     
	  system_rev &= ~SYSTEM_REV_MEMCLK_MASK;
	  system_rev |= SYSTEM_REV_MEMCLK_60MHZ;
#endif
	  param_system_rev.value = system_rev;
	  putLabeledWord("system_rev: ", system_rev);
	  switch (system_rev & SYSTEM_REV_MEMCLK_MASK) {
	  case SYSTEM_REV_MEMCLK_60MHZ: 
	       param_mem_fclk_21285.value = 60000000; break;
	  case SYSTEM_REV_MEMCLK_48MHZ: 
	       param_mem_fclk_21285.value = 48000000; break;
	  case SYSTEM_REV_MEMCLK_33MHZ: 
	  default:
	       param_mem_fclk_21285.value = 33333333; break;
	  }
          param_maclsbyte.value = get_maclsbyte();

	  putstr("memclk_hz: ");
	  putstr(((system_rev & SYSTEM_REV_MEMCLK_MASK) == SYSTEM_REV_MEMCLK_33MHZ) 
		 ? "33MHz\r\n" : "48MHz\r\n");
	  /* this string should agree with the MAC in btpci.c */

          putstr("MAC=08:00:2b:00:01:"); putHexInt8(param_maclsbyte.value); putstr("\r\n");

#endif /* CONFIG_MACH_SKIFF */
	  //param_dram_size.value = dram_size;
	  print_mem_size("SDRAM size: ", dram_size);
	 
	  bootinfo.bt_memend = dram_size;
     }
     param_boot_flags.value = boot_flags;

#ifdef CONFIG_MACH_JORNADA720
     /* Take the MCU out of reset mode */
     (*((volatile int *)PPSR_REG)) |= PPDR_LFCLK;
#endif

#ifdef CONFIG_LOAD_KERNEL
     if (!jffs_init_1pass_list()){
	  putstr("jffs_init_1pass_list failed to initialize\r\n");
     }
#endif //CONFIG_LOAD_KERNEL


     /* print the opening banner */
#ifdef CONFIG_LCD  
#if !defined (NO_SPLASH)
     if (squelch_serial() || do_splash)
	  splash();
#else
     // at least show something even if we didnt include a splash screen
     if (squelch_serial() || do_splash) {

	  lcd_default_init(lcd_type, NULL);
     }
#endif // no_splash
#endif // config_lcd  
     print_banner();

     if (normal_boot()) {
	  int suppress_splash = 0;
	  /* normal boot, eval params */
	  command_params_eval(0, NULL);
	  suppress_splash = param_suppress_splash.value;
	  if (!machine_is_jornada56x()) {
	       if (suppress_splash) {
#ifdef CONFIG_LCD
		    lcd_off(lcd_type);
#endif
	       } else {
#ifdef CONFIG_LCD
		    if (param_splash_filename.value)
			 splash();
		    lcd_on();
#endif
	       }
	  }
     }
     else {
	  putstr("DEBUG BOOT: not evaluating params\r\n");
	  putstr("DEBUG BOOT: use `params eval' to evaluate parameters.\r\n");
#ifdef CONFIG_REFLASH
	  reflash_enabled = 1;
#endif
     }

     icache_enabled = param_icache_enabled.value;
     dcache_enabled = param_dcache_enabled.value;

#ifdef CONFIG_SECURE
     //+
     // PocketPCSE - Erase memory and make sure that nothing is cached. The
     // caches get re-enabled afterwards if they are flagged to be.
     //-
     enable_caches(0, icache_enabled);
     eraseRAM( ERASE_PATTERN1, ERASE_PATTERN2 );
#endif

#if !defined(CONFIG_PXA)
     enable_caches(dcache_enabled, icache_enabled);
#endif

     /********************************************************************************
      * WARNING, IF YOU TURN ON THE D CACHE AND DONT CORRECTLY TURN IT OFF
      * DRAM GETS CORRUPTED, SINCE WE SEEM TO HANG WHEN WE FLUSH DCACHE, WE NEED TO
      * LOOK AT THIS MORE BEFORE RUNNING WITH DCACHE TURNED ON
      * 
      ********************************************************************************/  
     asmEnableCaches(dcache_enabled,icache_enabled); // turn on icache

     
     autoboot_timeout = param_autoboot_timeout.value;

#ifdef CONFIG_SA1100
     /* patch from Mayank Sharma <mayank@impulsesoft.com> */
     while(!CTL_REG_READ(POSR))
	  ;   /* Waiting for RCNR to stablize */
#endif

     /*
      * wait for a keystroke or a button press.
      * button presses cause commands to be run.
      */
     if (call_bootmenu || !normal_boot()
	 || ((c = awaitkey_seconds(autoboot_timeout, NULL))
	     && (c != '\r') && (c != '\n')) ) {
	  // do people care if this line comes out every time?  jca
	  if (autoboot_timeout != 0 && (!call_bootmenu) && squelch_serial())
	       putstr("type \"?\" or \"help\" for help.\r\n");
	  bootmenu(); /* does not return */
     }

     default_boot();
     return;
}

COMMAND(flash_type, command_flash_type, "[type] -- print available flash types or set the flash type", BB_RUN_FROM_RAM);
/* can have arguments or not */
void command_flash_type(int argc, const char **argv)
{
  if (argc == 1) {
     /* print the available flash types */
     btflash_print_types();
  } else {
    /* manually set the flash type */
     btflash_set_type(argv[1]);
  }
}

COMMAND(flash_width, command_flash_width, "[16|32] -- print available flash width or set the flash width", BB_RUN_FROM_RAM);
/* can have arguments or not */
void command_flash_width(int argc, const char **argv)
{
  if (argc == 1) {
     /* print the available flash types */
    putstr("flash_width="); putstr( BTF_2x16() ? "32\r\n" : "16\r\n" );
  } else {
    /* manually set the flash type */
    if (strcmp(argv[1], "16") == 0) {
      bt_flash_organization = BT_FLASH_ORGANIZATION_1x16;
    } else if (strcmp(argv[1], "32") == 0) {
      bt_flash_organization = BT_FLASH_ORGANIZATION_2x16;
    } else {
      putstr("Invalid width: must be 16 or 32\r\n");
    }
  }
}

#if defined(CONFIG_JFFS)

static int jffs_changed = 1;

COMMAND(jffsls, command_ls, "[path] -- display directory listing", BB_RUN_FROM_RAM);
/* can have arguments or not */
void command_ls(int argc, const char **argv)
{
  if(jffs_init(jffs_changed) < 0)
    return;

  jffs_changed = 0;

  jffs_list((argc == 1) ? "/" : (char *)argv[1]);
}

COMMAND(histogram, command_histogram, "-- display jffs statistics", BB_RUN_FROM_RAM);
/* no arguments */
void command_histogram(int argc, const char **argv)
{
  jffs_statistics();
}

#endif  /* CONFIG_JFFS */

#if defined(CONFIG_MD5)
COMMAND(md5sum, command_md5sum, "<address|partition> <len>", BB_RUN_FROM_RAM);
SUBCOMMAND(md5sum, file, command_md5sum, "<filename> [partition_name]", BB_RUN_FROM_RAM, 0);
void command_md5sum(int argc, const char **argv)
{
   void *base = 0;
   unsigned long size = 0;
   unsigned int sum[MD5_SUM_WORDS];
   char sizebuf[32];
   /* default place to unpack file */
   base = (void*)param_kernel_in_ram.value;

   if (!(argc >= 2 || argc <= 4)) {
      putstr("Usage: md5sum file <filename> [<partition>]\r\n");
      putstr("Usage: md5sum <partitionname> [<len>]\r\n");
	  putstr("Usage: md5sum <address> [<len>]\r\n");
      return;
   }

   if (strcmp(argv[1], "file") == 0) {
#ifdef CONFIG_JFFS
      /* using John Dorsey's JFFS implementation */
      if (jffs_init(jffs_changed) < 0)
         return;

      jffs_changed = 0;
      jffs_md5sum((char *)argv[1]);
#else
      const char *partname = (argc == 4) ? argv[3] : "root";
      const char *filename = argv[2];
      /* fetch the file contents */
      size = body_p1_load_file(partname, filename, base);
#endif
   } else 
      {
         struct FlashRegion *region = btflash_get_partition(argv[1]);
         if (region != NULL) {
            base = (void*)region->base;
            size = region->size;
         } else {
            base = (void*)strtoul(argv[1], NULL, 0);
            if (strtoul_err) {
               putstr("error parsing base addr: "); putstr(argv[1]); putstr("\r\n");
               return;
            }
         }
         if (argc == 3) {
            size = strtoul(argv[2], NULL, 0);
            if (strtoul_err) {
               putstr("error parsing size: "); putstr(argv[2]); putstr("\r\n");
               return;
            }
         }
      }

   if (size > 0) {
      md5_sum(base, size, sum);
      md5_display(sum); putstr("    "); putstr(argv[1]); 
      /* now put the size */
      dwordtodecimal(sizebuf, size); putstr("    "); putstr(sizebuf); putstr("\r\n");
   }
}
#endif  /* CONFIG_MD5 */

COMMAND(display, command_display, "display -- dumps the SA1100 registers", BB_RUN_FROM_RAM);
void command_display(int argc, const char **argv)
{
#if defined(CONFIG_SA1100)
  void *uart = serial->serbase(serial);

   putstr("\r\nSA1100 Registers:\r\n    UART" );
   putc((uart == Ser1Base) ? '1' : '3');
   putLabeledWord(":  ", (unsigned int)uart);
   putLabeledAddrWord( "      UTCR0   0x00 ", (long *)(uart + UTCR0));
   putLabeledAddrWord( "      UTCR1   0x04 ", (long *)(uart + UTCR1));
   putLabeledAddrWord( "      UTCR2   0x08 ", (long *)(uart + UTCR2));
   putLabeledAddrWord( "      UTCR3   0x0c ", (long *)(uart + UTCR3));
   putLabeledAddrWord( "      UTDR    0x10 ", (long *)(uart + UTDR));
   putLabeledAddrWord( "      UTSR0   0x14 ", (long *)(uart + UTSR0));
   putLabeledAddrWord( "      UTSR0+4 0x18 ", (long *)(uart + UTSR0+4));
   putLabeledAddrWord( "      UTSR0+8 0x1c ", (long *)(uart + UTSR0+8));
   putLabeledAddrWord( "      UTSR1   0x20 ", (long *)(uart + UTSR1));

#endif  /* defined(CONFIG_SA1100) */
}


/* requires arguments */
SUBCOMMAND(load, flash, command_load_flash, "<flash_addr> -- ymodem/xmodem receive to flash (see override param)", BB_RUN_FROM_RAM, 1);
void command_load_flash(int argc, const char **argv)
{
   unsigned long flash_dest = 0;
   int override;

   override = param_override.value;

   /* loading flash is different.. because writing flash is slow, we do
      not load it directly from the serial port. So we receive into
      memory first, and then program the flash... */
  
   if (argc < 2) { /* default to first command */
      putstr("usage: load flash <flashaddr>\r\n");
   } else {
      flash_dest = strtoul(argv[1], NULL, 0);
      if (strtoul_err) {
         putstr("error parsing flash_dest\r\n");
         return;
      }

      if (flash_dest < flashDescriptor->bootldr.size && !override) {
         putstr("That is bootloader space! Use load bootldr.  Operation canceled\r\n");
         return;
      }
      command_load_flash_region("flash", flash_dest, flash_size, 0);
   }
}

SUBCOMMAND(load, ram, command_load_ram, "<ramaddr>", BB_RUN_FROM_RAM, 1);
extern struct bootblk_param param_ymodem;
void command_load_ram(int argc, const char **argv)
{
#ifdef CONFIG_YMODEM
  unsigned long ymodem;
#endif
  unsigned long img_size = 0;
  unsigned long img_dest = 0;
  
  if (argc < 2) { /* default to first command */
    putstr("usage: load ram <ramaddr>\r\n");
  } else {
    /* parse the command line to fill img_size and img_dest */

    img_dest = strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing img_dest\r\n");
      return;
    }
    
#ifdef CONFIG_YMODEM
    ymodem = param_ymodem.value;

    if (ymodem)
      img_size = ymodem_receive((char*)img_dest, SZ_8M);
    else
#endif
      img_size = modem_receive((char*)img_dest, img_size);

    /* in case this is a kernel download, update bt_memavail */
    bootinfo.bt_memavail = ROUNDUP(img_size, SZ_1M);

    putHexInt32(img_size);
    putLabeledWord(" bytes loaded to ",img_dest);
    putstr("\r\n");


    last_ram_load_address = img_dest;
    last_ram_load_size = img_size;
    
  }
}

/**
 * does the work for command_load, etc.
 */

void program_flash_region(const char *regionName, 
                          unsigned long regionBase, size_t regionSize, 
                          unsigned long src, size_t img_size, int flags)
{
   unsigned long hdr_size = 0; /* size of header */
   unsigned long i;
   int remaining_bytes;
   long noerase;
   int isGZip = 0;
   unsigned char buf[64]; //extra space incase
   unsigned long compr_img_size = img_size;
   struct bz_stream z;
   int num_bytes = 0;
   int bytes_programmed = 0;
   unsigned char *p = buf;

   
   
   noerase = param_noerase.value;
   if (isGZipRegion(src))
       isGZip = 1;

   

   if (flags & LFR_BOOTLDR) {
       // in order to do the verify, bootldrs are special
       if (isGZip){
	   putstr("uncompressing gzipped bootldr\r\n");
	   putstr("uncompressing gzipped bootldr\r\n");	   
	   if (!gUnZip(src,&img_size,src + SZ_256K)){
	       putstr("bad unzip of gzipped bootldr\r\n");
	       return;
	   }
	   src += SZ_256K;
	   isGZip = 0;	   
       }
       
       if (!isValidBootloader(src,img_size)){
	   putstr("Not loading invalid bootldr into flash\r\n");
	   return;
       }
       
   }

   if (flags & LFR_SIZE_PREFIX) {
      hdr_size += 4;
      src -= 4;
      *(long *)src = img_size;
      img_size += hdr_size;
   }
   
   // let's see if we got a gzip image to play with.
   if (isGZip)
   {

       putstr("Looks like a gzipped image, let's verify it...\r\n");
       putstr("Looks like a gzipped image, let's verify it...\r\n");
       if (!verifyGZipImage(src,&img_size)){	   
	   putstr("invalid gzip image.  Sorry...\r\n");
	   return;
       }
       // img_size now has the uncomopressed size in it.
       gzInitStream(src,compr_img_size,&z);

   }
       

   

   if (img_size > regionSize) { // xple errors since minicom tends to eat some(all?)
      putLabeledWord("img_size is too large for region: ", regionSize);
      putLabeledWord("img_size is too large for region: ", regionSize);
      putLabeledWord("img_size is too large for region: ", regionSize);
      return;
   }

   putstr("\r\nprogramming flash...");

   set_vppen();
   if (flags & LFR_BOOTLDR) {
      putstr( "\r\nunlocking boot sector of flash\r\n" );
      protectFlashRange( 0x0, img_size+hdr_size, 0);
   }
   if (!noerase) {
     int eraseSize = img_size + hdr_size;
     if (flags & LFR_JFFS2)
       eraseSize = regionSize;
     putstr("erasing ...\r\n");
     if (eraseFlashRange(regionBase,eraseSize)) {
       putstr("erase error!\r\n");
       goto clear_VPPEN;
     }
   }

   /* the magic number.. */
   putstr("writing flash..\r\n");
   i = 0;
   remaining_bytes = img_size;
   while(remaining_bytes > 0) { 
      
      if (isGZip){
	  num_bytes -= bytes_programmed;
	  p += bytes_programmed;
	  //putLabeledWord("pre num_bytes = 0x",num_bytes);
	  //putLabeledWord("pre remaining_bytes = 0x",remaining_bytes);
	  
	  if (!num_bytes){
	      num_bytes = gzRead(&z,buf,64);
	      p = buf;
	      //putLabeledWord("gzRead gives num_bytes = 0x",num_bytes);
	      
	  }

      }
      else{
	  num_bytes = 64;
	  p = buf;	  
	  memcpy((void *)p,(const void *)(src+i),num_bytes);
      }
      
      if ((i % SZ_64K) == 0) {
         putstr("addr: ");
         putHexInt32(regionBase+i);
         putstr(" data: ");
         putHexInt32(*(unsigned long *)p);
         putstr("\r\n");
      }

      if (((i % 64) == 0) && (remaining_bytes > 64) && (num_bytes == 64)) {
         /* program a block */
         if (programFlashBlock(regionBase+i, (unsigned long*)p, 64)) {
            putstr("error while copying to flash!\r\n");
            goto clear_VPPEN;
            break;
         }
         bytes_programmed = 64;
      } else {
         if (programFlashWord(regionBase+i, *(unsigned long *)p)) {
            putstr("error while copying to flash!\r\n");
            goto clear_VPPEN;
            break;
         }
         bytes_programmed = 4;
      }
      i += bytes_programmed;
      remaining_bytes -= bytes_programmed;
      
   }
   putstr("verifying ... \r\n");

   if (isGZip){
       unsigned long calc_crc32;
       unsigned char *dst = (unsigned char *)&flashword[regionBase >> 2];       
       calc_crc32 = crc32(0,dst,img_size);
       putLabeledWord("calculated crc32 = 0x",calc_crc32);
       putLabeledWord("desired    crc32 = 0x",z.read_crc32);
       if (calc_crc32 != z.read_crc32){
	   putstr("error programming flash (crc check failed)\r\n");
	   goto clear_VPPEN;
       }
   }
   else
   {
      int i;
      int nwords = (img_size >> 2);
      unsigned long *srcwords = (unsigned long *)src;
      unsigned long *dstwords = (unsigned long *)&flashword[regionBase >> 2];

      for (i = 0; i < nwords; i++) {
         if (srcwords[i] != dstwords[i]) {
            putLabeledWord("error programming flash at offset=", i << 2);
            putLabeledWord("  src=", srcwords[i]);
            putLabeledWord("  flash=", dstwords[i]);
            putstr("not checking any more locations\r\n");
            goto clear_VPPEN;
         }
      }
   }   
   if (flags & LFR_JFFS2) {
     unsigned long jffs2_sector_marker0 = 0xFFFFFFFF;
     unsigned long jffs2_sector_marker1 = 0xFFFFFFFF;
     unsigned long jffs2_sector_marker2 = 0xFFFFFFFF;
     jffs2_sector_marker0 = param_jffs2_sector_marker0.value;
     jffs2_sector_marker1 = param_jffs2_sector_marker1.value;
     jffs2_sector_marker2 = param_jffs2_sector_marker2.value;
     putstr("formatting ... ");
     btflash_jffs2_format_region(regionBase + img_size + hdr_size, regionSize - img_size - hdr_size, jffs2_sector_marker0, jffs2_sector_marker1, jffs2_sector_marker2);
   }
   putstr("done.\r\n");
   if (flags & LFR_BOOTLDR) {
#ifdef CONFIG_MACH_JORNADA720 /* if we protect, then we can't rewrite using debug tools */
      protectFlashRange( 0x0, img_size+hdr_size, 0);
#else
      protectFlashRange( 0x0, img_size+hdr_size, 1);
#endif
   }

 clear_VPPEN:
   clr_vppen();
}

SUBCOMMAND(load, flashregion, command_load_flash, "-- upload an image to the flash. dangerous", BB_RUN_FROM_RAM, 0);
static void command_load_flash_region(const char *regionName, unsigned long regionBase, unsigned long regionSize, int flags)
{
   unsigned long img_size = 0; /* size of data */
   unsigned long img_dest = VKERNEL_BASE + 1024; /* just a temporary holding area */
   long ymodem = 0;


   if (0)
   if (amRunningFromRam() && regionBase >= flashDescriptor->bootldr.size) {
       putstr("Can't load to partition <");
       putstr(regionName);
       putstr("> while running from ram.  Operation canceled\r\n");
       return;
   }

   
   putstr("loading flash region "); putstr(regionName); putstr("\r\n");

#ifdef CONFIG_YMODEM
   ymodem = param_ymodem.value;
#endif

   if (ymodem) {
#ifdef CONFIG_YMODEM
      putstr("using ymodem\r\n");
      img_size = ymodem_receive((char*)img_dest, regionSize);
#endif
   } else {
      putstr("using xmodem\r\n");
      img_size = modem_receive((char*)img_dest, regionSize);
   }
   if (!img_size) {
      putstr("download error. aborting.\r\n");
      return;
   }
    
   putHexInt32(img_size);
   putLabeledWord(" bytes loaded to ",img_dest);
   if ((img_size % 4) != 0) {
      putstr("img_size is not a multiple of 4 -- are we sure that's OK?\r\n");
   }

   program_flash_region(regionName, regionBase, regionSize, img_dest, img_size, flags);
}

/* requires arguments */
COMMAND(load, command_load, "[partition] -- ymodem/xmodem receive to flash partition", BB_RUN_FROM_RAM);
SUBCOMMAND(load, params, command_load, "-- ymodem/xmodem receive to flash partition", BB_RUN_FROM_RAM, 0);
void command_load(int argc, const char **argv)
{
  const char *partname = argv[1];
  struct FlashRegion *partition = btflash_get_partition(partname);
  if (partition == NULL) {
    putstr("Error: no partition named "); putstr(partname); putstr("\r\n");
    putstr("Here are the defined partitions\r\n");
    command_partition_show(0, NULL);
    return;
  }
  if (partition->flags & LFR_BOOTLDR) {
    putstr("partition "); putstr(partname); putstr(" is a bootldr partition: \r\n requiring a bootldr or parrot image.\r\n");
  }
  if (partition->flags & LFR_JFFS2) {
    putstr("partition "); putstr(partname); putstr(" is a jffs2 partition: \r\n expecting .jffs2 or wince_image.gz.\r\n");
  }
  if (partition->flags & LFR_KERNEL) {
    putstr("partition "); putstr(partname); putstr(" is a kernel partition: \r\n probably expecting a zImage.\r\n");
  }
  putstr(" After receiving file, will automatically uncompress .gz images\r\n");
  command_load_flash_region(partname, partition->base, partition->size, partition->flags);
  if (strcmp(partname, "params") == 0) {
	params_eval(PARAM_PREFIX, 0);
  }
}

/* requires arguments */
COMMAND(program, command_program, "<flashpartname> <srcramaddr> <len>", BB_RUN_FROM_RAM);
void command_program(int argc, const char **argv)
{
    if (argc > 1) {
        const char *partname = argv[1];
        struct FlashRegion *partition = btflash_get_partition(partname);
        unsigned long src = 0;
        unsigned long len = 0;
        if (partition == NULL) {
            putstr("  could not find flash partition "); putstr(partname); putstr("\r\n");
            goto usage;
        }
        src = strtoul(argv[2], 0, 0);
        if (!src)
            goto usage;
        len = strtoul(argv[3], 0, 0);
        if (!len)
            goto usage;
        putstr("programming flash partition "); putstr(partname); putstr("\r\n");
        putLabeledWord(" ram src =", src);
        putLabeledWord("     len =", len);
        
        program_flash_region(partname, partition->base, partition->size, src, len, partition->flags);
        if (strcmp(partname, "params") == 0) {
            params_eval(PARAM_PREFIX, 0);
        }
    }
    return;
 usage:
    putstr("usage: program <flashpartname> <srcramaddr> <len>\r\n");
}

/* can have arguments or not */
COMMAND(save, command_save, "<partition_name>", BB_RUN_FROM_RAM);
void command_save(int argc, const char **argv)
{
  if (argc > 1) {
    const char *partname = argv[1];
    struct FlashRegion *partition = btflash_get_partition(partname);
    if (partition == NULL) {
	  putstr("Could not find partition ");
	  putstr(partname);
	  putstr("\r\n");
	  return;
	}
    putstr("About to xmodem send "); putstr(partition->name); putstr("\r\n");
    putLabeledWord("  flashword=", (unsigned long)flashword);
    putLabeledWord("  base=", partition->base);
    putLabeledWord("  nbytes=", partition->size);
    if (!modem_send(partition->base + (char *)flashword, partition->size)) {
      putstr("download error.\r\n");
      return;
    } 
  }
}

SUBCOMMAND(save, all, command_save_all, "-- upload all of flash via xmodem", BB_RUN_FROM_RAM, 0);
void command_save_all(int argc, const char **argv) 
{
   dword img_size;

   img_size = modem_send((char*)flashword, flash_size); 
   if (!img_size) {
      putstr("download error. aborting.\r\n");
      return;
   } 
}

SUBCOMMAND(save, flash, command_save_flash, "<flashaddr> <size> -- upload flash region via xmodem", BB_RUN_FROM_RAM, 0);
void command_save_flash(int argc, const char **argv) {

   dword         img_size;
   unsigned long flash_size = 0;
   unsigned long flash_dest = 0;

   if (argc < 3) { /* default to first command */
      putstr("usage: save flash <flashaddr> <size>\r\n");
   } else {
      flash_dest = strtoul(argv[1], NULL, 0);
      if (strtoul_err) {
         putstr("error parsing <flashaddr>\r\n");
         return;
      }
      flash_size = strtoul(argv[2], NULL, 0);
      if (strtoul_err) {
	putstr("error parsing <size>\r\n");
      }
      putLabeledWord("flash_dest=", flash_dest );
      putLabeledWord("flash_size=", flash_size );
      img_size = modem_send( flash_dest + (char*)flashword, flash_size );
      if (!img_size) {
         putstr("download error. aborting.\r\n");
         return;
      }
   }
   return;
}

SUBCOMMAND(save, ram, command_save_ram, "<ramaddr> <size> -- upload dram region via xmodem", BB_RUN_FROM_RAM, 0);
void command_save_ram(int argc, const char **argv) {

   dword         img_size;
   unsigned long ram_size = 0;
   unsigned long ram_dest = 0;

   if (argc < 3) { /* default to first command */
      putstr("usage: save ram <ramaddr> <size>\r\n");
   } else {
      ram_dest = strtoul(argv[2], NULL, 0);
      if (strtoul_err) {
         putstr("error parsing <ramaddr> "); putstr(argv[2]); putstr("\r\n");
         return;
      }
      ram_size = strtoul(argv[3], NULL, 0);
      if (strtoul_err) {
	putstr("error parsing <size>\r\n");
      }
      putLabeledWord("ram_dest=", ram_dest );
      putLabeledWord("ram_size=", ram_size );
      img_size = modem_send((char*)ram_dest, ram_size );
      if (!img_size) {
         putstr("download error. aborting.\r\n");
         return;
      }
   }
   return;
}

SUBCOMMAND(peek, ram, command_peek_ram, "<addr> -- reads 32 bits", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(peek, byte, command_peek_ram, "<addr> -- reads 8 bits", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(peek, short, command_peek_ram, "<addr> -- reads 16 bits", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(peek, int, command_peek_ram, "<addr> -- reads 32 bits", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(peek, flash, command_peek_ram, "<offset>", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(peek, gpio, command_peek_ram, "<offset>", BB_RUN_FROM_RAM, 1);
void command_peek_ram(int argc, const char **argv)
{
  unsigned long addr = 0;
  unsigned long result = 0;
  
  if (argc < 2) { /* default to first command */
    putstr("peek ram requires arguments!\r\n");
  } else {
    addr = strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing addr\r\n");
      return;
    }

    if (strcmp(argv[0], "byte") == 0)
       result = *(unsigned char *)(addr);
    else if (strcmp(argv[0], "short") == 0)
       result = *(unsigned short *)(addr);
    else if (strcmp(argv[0], "int") == 0)
       result = *(int *)(addr);
    else if (strcmp(argv[0], "flash") == 0)
       result = flashword[(addr&flash_address_mask) >> 2];
    else if (strcmp(argv[0], "gpio") == 0)
       /* read from the gpio read port */
       result = *(int *)(0x21800008 + addr);
    else 
       result = *(unsigned long *)(addr);

    putLabeledWord("  addr  = ",addr);
    putLabeledWord("  value = ",result);
  }
}

#if defined(CONFIG_JFFS)
SUBCOMMAND(peek, jffs, command_peek_jffs, "<inode>\r\npeek jffs <filename> <size>", BB_RUN_FROM_RAM, 1);
void command_peek_jffs(int argc, const char **argv)
{
  unsigned int inumber, size;

  switch(argc){

  case 2:

    inumber = strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing inumber\r\n");
      return;
    }
    
    jffs_dump_inode(inumber);

    break;

  case 3:
    size = strtoul(argv[2], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing size\r\n");
      return;
    }

    if(jffs_init(jffs_changed) < 0)
      return;
    
    jffs_changed = 0;
  
    jffs_dump_file((char *)argv[1], size);

    break;

  default:
    putstr("peek jffs requires inumber argument or filename and length\r\n");
  }

}

#endif  /* CONFIG_JFFS */

SUBCOMMAND(poke, ram, command_poke_ram, "<addr> <dword> -- reads 32 bits", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(poke, byte, command_poke_ram, "<addr> <byte> -- reads 8 bits", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(poke, short, command_poke_ram, "<addr> <short> -- reads 16 bits", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(poke, int, command_poke_ram, "<addr> <dword> -- reads 32 bits", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(poke, flash, command_poke_ram, "<offset> <value>", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(poke, gpio, command_poke_ram, "<offset> <value>", BB_RUN_FROM_RAM, 1);
void command_poke_ram(int argc, const char **argv)
{
  unsigned long addr = 0;
  unsigned long value = 0;
  
  if (argc < 3) { /* default to first command */
    putstr("poke ram requires arguments!\r\n");
  } else {

    addr = strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing addr\r\n");
      return;
    }

    value = strtoul(argv[2], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing value\r\n");
      return;
    }

    putstr("poke ram: \r\n");
    putLabeledWord(" addr=", addr);
    putLabeledWord(" value=", value);

    if (strcmp(argv[0], "byte") == 0) {
       *(unsigned char *)(addr) = value;
    } else if (strcmp(argv[0], "short") == 0) {
       *(unsigned short *)(addr) = value;
    } else if (strcmp(argv[0], "int") == 0) {
       *(int *)addr = value;
    } else if (strcmp(argv[0], "gpio") == 0) {
       /* write to the gpio write port */
       *(int *)(0x21800000 + addr) = value;
    } else if (strcmp(argv[0], "flash") == 0) {
       set_vppen();
       if (programFlashWord(addr & flash_address_mask, value))
          putstr("flash write failed!\r\n");
       clr_vppen();
    } else {
       *(unsigned long *)(addr) = value;
    }
  }
}


#if defined(CONFIG_SA1100)
extern unsigned long delayedBreakpointPC;
COMMAND(breakpoint, command_breakpoint, "[delayed] <pc> -- TODO", BB_RUN_FROM_RAM);
void command_breakpoint(int argc, const char **argv)
{
  if (argv[1] != NULL) {
    const char *pcstr = NULL;
    unsigned long pc;
    int delayed = 0;
    if (strcmp(argv[1], "delayed") == 0) {
      delayed = 1;
      pcstr = argv[2];
    } else {
      pcstr = argv[1];
    }
    pc = strtoul(pcstr, NULL, 0);
    pc &= 0xFFFFFFFC;
    pc |= 1;
    if (delayed) {
      putLabeledWord("PC breakpoint will be set after kernel unzip at: ", pc);
      delayedBreakpointPC = pc;
    } else {
      putLabeledWord("Setting hardware PC breakpoint at: ", pc);
      __asm__ ("mcr     p15, 0, %0, c14, c8, 0" : : "r" (pc));
    }
  } else {
    unsigned long pc = 0;
    putstr("Clearing PC breakpoint");
    __asm__ ("mcr     p15, 0, %0, c14, c8, 0" : : "r" (pc));
  }
}
#endif

COMMAND(qflash, command_qflash, "id | security | <addr>", BB_RUN_FROM_RAM);
void command_qflash(int argc, const char **argv)
{
  unsigned long addr = 0;
  unsigned long result = 0;
  
  if (argc < 2) { /* default to first command */
    putstr("qflash requires arguments!\r\n");
  } else {
    addr = strtoul(argv[argc-1], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing addr\r\n");
      return;
    }

    if (strcmp(argv[1], "id") == 0) {
       result = queryFlashID(addr);
    } else if (strcmp(argv[1], "security") == 0) {
       result = queryFlashSecurity(addr);
    } else {
       result = queryFlash(addr);
    }
    putLabeledWord("value = ",result);
  }
}

COMMAND(eflash, command_eflash, "<partition> | <addr> [<len>] | chip", BB_RUN_FROM_RAM);
void command_eflash(int argc, const char **argv)
{
  unsigned long addr = 0;
  unsigned long len = 0;
  int override;

  override = param_override.value;

  if (argc < 2) { /* default to first command */
    putstr("eflash requires arguments: <partition>|<addr> [<len>]|chip!\r\n");
    return;
  } else {
    struct FlashRegion *partition = btflash_get_partition(argv[1]);
    if (partition != NULL) {
      putstr("erasing partition "); putstr(argv[1]); putstr("\r\n");
      addr = partition->base;
      len = partition->size;
      goto erase;
    } else if (strncmp(argv[1], "chip", 4) == 0) {
      if (!override) {
	putstr("Cannot erase whole chip without setting override to 1.\r\n");
	return;
      }
      putstr("erasing flash chip\r\n");
      set_vppen();
      eraseFlashChip();
      clr_vppen();
      return;
    }
    addr = strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing addr: "); putstr(argv[1]); putstr("\r\n");
    }
    putLabeledWord("addr=", addr);

    if (argc > 2) {
      len = strtoul(argv[2], NULL, 0);
      if (strtoul_err) {
        putstr("error parsing len: "); putstr(argv[2]); putstr("\r\n");
      }
      putLabeledWord("len=", len);

    }
  erase:
    if (addr == 0 && !override) {
      putstr("Cannot erase first sector without setting override to 1.\r\n");
      return;
    }

    set_vppen();
    if (len == 0) 
      eraseFlashSector(addr);
    else
      eraseFlashRange(addr, len);
    clr_vppen();
  }
}

COMMAND(pflash, command_pflash, "<addr> <len> 0|1 -- (1 -> protect, 0 -> unprotect all)", BB_RUN_FROM_RAM);
void command_pflash(int argc, const char **argv)
{
  unsigned long addr = 0;
  unsigned long len = 0;
  unsigned long protect = 9;
  
  if (argc < 4) { /* default to first command */
    putstr("pflash requires arguments: <addr> <len> 0/1  (1 -> protect, 0 -> unprotect all)!\r\n");
  } else {
    addr = strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing addr: "); putstr(argv[1]); putstr("\r\n"); return;
    }
    len = strtoul(argv[2], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing len: "); putstr(argv[2]); putstr("\r\n"); return;
    }
    protect = strtoul(argv[3], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing protect: "); putstr(argv[3]); putstr("\r\n"); return;
    }
    /* be certain that protect is only 0/1 */
    if (!((protect == 0) || (protect == 1)) ) {
      putstr("error protect value must be 0/1: "); putstr(argv[3]); putstr("\r\n"); return;
    }
    
    if (verifiedFlashRange( addr, len )) {
       putLabeledWord("  addr=", addr);
       putLabeledWord("  len=", len);
       putLabeledWord("  protect=", protect);

       set_vppen();
       protectFlashRange(addr, len, protect);
       clr_vppen();
    } else { 
      putstr("Region specified is out of Range.\r\n" );
      putLabeledWord( " Please use a range less than:", flash_size );      
    }
  }
}

COMMAND(call, command_call, "<addr> [a0] [a1] [a2] [a3]", BB_RUN_FROM_RAM);
COMMAND(jump, command_call, "<addr> [a0] [a1] [a2] [a3]", BB_RUN_FROM_RAM);
void command_call(int argc, const char **argv)
{
  void (*fcn)(long a0, long a1, long a2, long a3) = NULL;
  
  if (argc < 2) {
    putstr("usage: call <addr> [a0] [a1] [a2] [a3]\r\n");
  } else {
    long a0 = 0;
    long a1 = 0;
    long a2 = 0;
    long a3 = 0;

    if (argv[1][0] == '.' || argv[1][0] == '=' || argv[1][0] == '-') {
	if (last_ram_load_address != 0) {
	    strtoul_err = 0;
	    fcn = (void*)last_ram_load_address;
	}
	else {
	    putstr("last_ram_load_address is 0.\n\r");
	    return;
	}
    }
    else
	fcn = (void*)strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing vaddr\r\n");
      return;
    }
    if (argc > 2) {
      a0 = strtoul(argv[2], NULL, 0);
    }
    if (argc > 3) {
      a1 = strtoul(argv[3], NULL, 0);
    }
    if (argc > 4) {
      a2 = strtoul(argv[4], NULL, 0);
    }
    if (argc > 5) {
      a3 = strtoul(argv[5], NULL, 0);
    }
    asmEnableCaches(0,0);
    putLabeledWord("Calling fcn=", (long)fcn);
    putLabeledWord("  a0=", a0);
    putLabeledWord("  a1=", a1);
    putLabeledWord("  a2=", a2);
    putLabeledWord("  a3=", a3);

    drain_uart();
    
    fcn(a0, a1, a2, a3);
  }
}

COMMAND(physaddr, command_physaddr, "<vaddr> -- returns physical address", BB_RUN_FROM_RAM);
void command_physaddr(int argc, const char **argv)
{
  unsigned long vaddr = 0;
  
  if (argc < 2) { /* default to first command */
    putstr("physaddr requires vaddr argument!\r\n");
  } else {
    vaddr = strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
      putstr("error parsing vaddr\r\n");
      return;
    }
    {
      unsigned long mmu_table_entry = mmu_table[vaddr >> 20];
      unsigned long paddr = mmu_table_entry & ~((1L << 20) - 1);
      unsigned long sectionDescriptor = mmu_table_entry & ((1L << 20) - 1);
      putLabeledWord("vaddr=", vaddr);
      putLabeledWord("paddr=", paddr);
      putLabeledWord("sectionDescriptor=", sectionDescriptor);
    }
  }
}

#ifdef UPDATE_BAUDRATE
void update_baudrate(struct bootblk_param *param, long paramoldvalue)
{
#ifdef CONFIG_MACH_SKIFF
   volatile unsigned long *csr = (volatile unsigned long *)DC21285_ARMCSR_BASE;
   unsigned long system_rev = get_system_rev();
   unsigned long fclk_hz = 33333333;
#endif
   int baudrate = param->value;
   unsigned long ubrlcr;
   unsigned long l_ubrlcr = 0;
   unsigned long m_ubrlcr = 0;
#ifdef CONFIG_MACH_SKIFF
   unsigned long h_ubrlcr = 0;
#endif


   
   switch ( baudrate ) {
#if defined(CONFIG_SA1100) || defined(CONFIG_PXA) 
   case 230400:
     break;
   case 115200:
     break;
#endif
   case 57600:
     break;
   case 38400:
     break;
   case 19200:
     break;
   case 9600:
     break;
   case 4800:
     break;
   case 2400:
     break;
   case 1200:
     break;
   case 300:
     break;
   case 110:
     break;
   default:
     param->value = paramoldvalue;
     putstr( "invalid baud rate:\r\n" );
     putstr( "     Please, try:   110,   300,  1200,   2400,  4800,  9600\r\n" );
     putstr( "                  19200, 38400, 57600, 115200 or 230,400\r\n" );
     return; 
   }
   
#ifdef CONFIG_MACH_SKIFF
   switch (system_rev&SYSTEM_REV_MEMCLK_MASK) {
   case SYSTEM_REV_MEMCLK_33MHZ:
      fclk_hz = 33333333;
      break;
   case SYSTEM_REV_MEMCLK_48MHZ:
      fclk_hz = 48000000;
      break;
   case SYSTEM_REV_MEMCLK_60MHZ:
      fclk_hz = 60000000;
      break;
   }

  fclk_hz = 48000000;

   ubrlcr = (fclk_hz / 64 / baudrate)-1;
   h_ubrlcr = csr[H_UBRLCR_REG];

#elif defined(CONFIG_SA1100) || defined(CONFIG_PXA)
   ubrlcr = (3686400 / 16 / baudrate) - 1;
#else
   #error no architecture defined in update_baudrate
#endif

   l_ubrlcr = ubrlcr & 0xFF;
   m_ubrlcr = (ubrlcr >> 8) & 0xFF;
   
   putLabeledWord("update_baudrate:  new baudrate=", baudrate);

   serial_set_baudrate(serial, baudrate);

   putLabeledWord(" baudrate changed to 0x", baudrate);

   /* set the value for the kernel too */

   bootinfo.bt_comspeed = baudrate;
}
#endif

void set_serial_number(struct bootblk_param *param, long paramoldvalue)
{
}


#ifdef CONFIG_MACH_SKIFF
long get_system_rev()
{
   long word1 = *(long *)FLASH_BASE;
   long word2 = *(long *)(FLASH_BASE+0x0080000C);
   long revision = SYSTEM_REV_SKIFF_V1;
   if (word1 != word2) {
      /* Skiff-V2 -- GPIO register 0 contains the revision mask */
      revision = (word2 & SYSTEM_REV_MASK);
   }
   return revision;
}

void set_system_rev(struct bootblk_param *param,const long oldVal)
{
   param->value = get_system_rev();
}
#endif

#ifdef CONFIG_MACH_SKIFF
/* seems not to work on sa1110 -- Brian Avery */
void flush_caches(void)
{
   writeBackDcache(CACHE_FLUSH_BASE);
   __asm__("  mcr p15, 0, r0, c7, c7, 0x00 /* flush I+D caches */\n"
           "  mcr p15, 0, r0, c7, c10, 4 /* drain the write buffer*/\n");
}
#else
void flush_caches(void)
{
}
#endif

long enable_caches(int dcache_enabled, int icache_enabled)
{
  long mmuc = 0;
#ifndef CONFIG_PXA 
  long enabled = (icache_enabled ? 0x1000 : 0x0);
  enabled |= (dcache_enabled ? 0xC : 0);
  flush_caches();
  __asm__("  mrc p15, 0, %0, c1, c0, 0\n"
	  "  bic %0, %0, #0x1000\n"
	  "  bic %0, %0, #0x000c\n"
	  "  orr %0, %0, %1\n"
	  "  mcr p15, 0, %0, c1, c0, 0\n"
	  : "=&r" (mmuc) : "r" (enabled));
  flush_caches();
#endif
  return mmuc;
}

void update_dcache_enabled(struct bootblk_param *param, long paramoldvalue)
{
   long dcache_enabled = param->value;
   long icache_enabled = 0;
   long mmuc;
   icache_enabled = param_icache_enabled.value;
   mmuc = enable_caches(dcache_enabled, icache_enabled);
   putLabeledWord("MMU Control word=", mmuc);
}

void update_icache_enabled(struct bootblk_param *param, long paramoldvalue)
{
   long dcache_enabled = 0;
   long icache_enabled = param->value;
   long mmuc;
   dcache_enabled = param_dcache_enabled.value;
   mmuc = enable_caches(dcache_enabled, icache_enabled);
   putLabeledWord("MMU Control word=", mmuc);
}

COMMAND(cmdex, command_cmdex, "[0|1] -- use extended commands", BB_RUN_FROM_RAM);
void command_cmdex(int argc, const char **argv)
{
    if (argc > 1)
	 use_extended_getcmd = strtoul(argv[1], NULL, 0);
    else
	use_extended_getcmd = !use_extended_getcmd;

    putLabeledWord("use_extended_getcmd=0x", use_extended_getcmd);    
}

void
update_autoboot_timeout(
    struct bootblk_param *param, long paramoldvalue)
{
    autoboot_timeout = (unsigned long)param->value;
    if (autoboot_timeout == 0)
	putstr("Autoboot is DISABLED\n\r");
}

void
update_cmdex(
    struct bootblk_param *param, long paramoldvalue)
{
    use_extended_getcmd = (unsigned long)param->value;
}

#if defined(CONFIG_MACH_IPAQ)
COMMAND(aux_sar, command_aux_ser, "-- aux serial dump", BB_RUN_FROM_RAM);
void command_aux_ser(
    int argc,
    const char **argv)
{
    auxm_serial_dump();
}
#endif

#ifndef CONFIG_MACH_H3900
COMMAND(ledblink, command_led_blink, "[on time] [off time] [off = 0] -- blink the LED", BB_RUN_FROM_RAM);
void
command_led_blink(
    int	    argc,
    const char*   argv[])
{
    char ledData[] = {0x01,0x00,0x05,0x05};
    
    if (argc > 1) 
	ledData[2] = (char) strtoul(argv[1], NULL, 0);
    
    if (argc > 2)
	ledData[2] = (char) strtoul(argv[2], NULL, 0);

    if (argc > 3)
	ledData[0] = (char) strtoul(argv[3], NULL, 0);
    
#if CONFIG_LCD
    led_blink(ledData[0],ledData[1],ledData[2],ledData[3]);
#endif
    
}
#endif

#if 1

voidpf
zcalloc(
    voidpf opaque,
    unsigned items,
    unsigned size)
{
    void*   p;
    int	    totsize;

    p = mmalloc(totsize = items * size);
    if (p) {
	memset(p, 0x00, totsize);
    }
    return(p);
}

void
zcfree(
    voidpf opaque,
    voidpf ptr)
{
    mfree(ptr);
}

#undef free
void
free(
    void*   p)
{
    mfree(p);
}

#endif


#if defined(CONFIG_PACKETIZE)

COMMAND(ttmode, command_ttmode, "[1,0] -- go into ttmode", BB_RUN_FROM_RAM);
void
command_ttmode(
    int		argc,
    const char*	argv[])
{
    static int	ttmode = 0;
    int	new_mode;

    if (argc > 1)
	new_mode = strtoul(argv[1], NULL, 0);
    else
	new_mode = 1;		/* no args turns on */

    if (new_mode) {
	packetize_output = 1;
	ack_commands = 1;
	no_cmd_echo = 1;
	use_extended_getcmd = 0;
    }
    else {
	packetize_output = 0;
	ack_commands = 0;
	no_cmd_echo = 0;
	use_extended_getcmd = 1;
    }

    ttmode = new_mode;
}

#if defined(CONFIG_USB)
void
command_usb_sercon(
    int 	argc,
    const char* argv[]);
#endif

COMMAND(ser_con, command_ser_con, "-- start a serial console session", BB_RUN_FROM_RAM );
void
command_ser_con(
    int		argc,
    const char* argv[])
{
#if defined(CONFIG_USB)
    // if there's USB support, we may really not be at a serial console.
    command_usb_sercon(argc,argv);
#else
    putstr("serial console at your service...\n\r");    
#endif
}

COMMAND(irda_con, command_irda_con, "irda_con -- start a irda console session", BB_RUN_FROM_RAM );
void
command_irda_con(
    int		argc,
    const char* argv[])
{
    putstr("irda not available yet, starting serial console.\n\r");
}


int
reboot_button_is_enabled(
    void)
{
    return (reboot_button_enabled > 0);
}

void
enable_reboot_button(
    void)
{
    reboot_button_enabled++;
}

void
disable_reboot_button(
    void)
{
    if (reboot_button_enabled > 0)
	reboot_button_enabled--;
}
#endif

COMMAND(memcpy, command_memcpy, "<dst> <src> <num> [size]", BB_RUN_FROM_RAM );
void
command_memcpy(
    int		argc,
    const char* argv[])
{
    void*   dst;
    void*   src;
    int	    num;
    int	    size = 1;
    
    if (argc < 4) {
	putstr("memcpy needs args: dst src num [size]\r\n");
	return;
    }

    dst = (void*)strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
	putstr("bad dst param\n\r");
	return;
    }
    src = (void*)strtoul(argv[2], NULL, 0);
    if (strtoul_err) {
	putstr("bad src param\n\r");
	return;
    }
    num = strtoul(argv[3], NULL, 0);
    if (strtoul_err) {
	putstr("bad num param\n\r");
	return;
    }
    if (argc > 4) {
	size = strtoul(argv[4], NULL, 0);
	if (strtoul_err) {
	    putstr("bad size param\n\r");
	    return;
	}
    }

    putstr("memcpy\n\r");
    putLabeledWord("src: 0x", (unsigned long)src);
    putLabeledWord("dst: 0x", (unsigned long)dst);
    putLabeledWord("num: 0x", num);
    putLabeledWord("size: 0x", size);
    
    switch (size) {
	case 1:
	{
	    char*   d = (char*)dst;
	    char*   s = (char*)src;

	    while (num--)
		*d++ = *s++;
	}
	break;

	case 2:
	{
	    short*   d = (short*)dst;
	    short*   s = (short*)src;

	    while (num--)
		*d++ = *s++;
	}
	break;

	case 4:
	{
	    int*   d = (int*)dst;
	    int*   s = (int*)src;

	    while (num--)
		*d++ = *s++;
	}
	break;

	default:
	    putLabeledWord("Bad size: 0x", size);
	    break;
    }
    
}

void  hex_dump(
    unsigned char   *data,
    size_t	    num)
{
    int     i;
    long    oldNum;
    char    buf[90];
    char*   bufp;
    int	    line_resid;
    
    while (num)
    {
	bufp = buf;
	binarytohex(bufp, (unsigned long)data, 4);
	bufp += 8;
	*bufp++ = ':';
	*bufp++ = ' ';
	
	oldNum = num;
	
	for (i = 0; i < 16 && num; i++, num--) {
	    binarytohex(bufp, (unsigned long)data[i], 1);
	    bufp += 2;
	    *bufp++ = (i == 7) ? '-' : ' ';
	}

	line_resid = (16 - i) * 3;
	if (line_resid) {
	    memset(bufp, ' ', line_resid);
	    bufp += line_resid;
	}
	
	memcpy(bufp, "| ", 2);
	bufp += 2;
	
	for (i = 0; i < 16 && oldNum; i++, oldNum--)
	    *bufp++ = BL_ISPRINT(data[i]) ? data[i] : '.';

	line_resid = 16 - i;
	if (line_resid) {
	    memset(bufp, ' ', 16-i);
	    bufp += 16 - i;
	}
	
	*bufp++ = '\r';
	*bufp++ = '\n';
	*bufp++ = '\0';
	putstr(buf);
	data += 16;
    }
}

COMMAND(hex_dump, command_hex_dump, "<addr> [size]", BB_RUN_FROM_RAM);
void
command_hex_dump(
    int		argc,
    const char*	argv[])
{
    size_t	    num;
    unsigned char*  p;

    if (argc == 3)		// size specified
	num = strtoul(argv[2], NULL, 0);
    else
	num = 16;

    p = (unsigned char*)strtoul(argv[1], NULL, 0);

    hex_dump(p, num);
}

void
bootldr_goto_sleep(
    void*   pspr)
{
#if !(defined(CONFIG_PXA)||defined(CONFIG_MACH_SKIFF))
    /*
     * shut down all of the peripherals in an clean and
     * orderly fashsion.
     * (later)
     */

    /* XXX do it!!!! */
#ifdef CONFIG_LCD
    if (!machine_is_jornada56x())
      lcd_off(lcd_type);
#endif
#ifdef CONFIG_MACH_IPAQ
    if (machine_has_auxm())
        auxm_fini_serial();
#endif
    CTL_REG_WRITE(UART2_UTCR3, 0);
    CTL_REG_WRITE(UART3_UTCR3, 0);
    
    
    /*  set pspr */
    if (pspr == NULL) {
	extern void SleepReset_Resume(void);
	extern void ResetEntryPoint(void);
	
	pspr = (void*)(SleepReset_Resume-ResetEntryPoint);
    }
	    
    CTL_REG_WRITE(PSPR, (unsigned long)pspr);
    
    /*  clear reset register status bits */
    CTL_REG_WRITE(RCSR, 0x0f);
    
#if 0 
    /*  setup GPIO outputs sleep state */
    /*  use current values ??? a good idea ??? */
    mask = CTL_REG_READ(GPIO_GPDR);
    v = CTL_REG_READ(GPIO_GPLR);
    v &= mask;
    CTL_REG_WRITE(PGSR, v);
#endif
    
    /*  set wakeup conditions */
    /*  any gpio edge */
    CTL_REG_WRITE(PWER, (1<<0));	/* power button */

    /* setup edge reggies
     * Wakeup on rising edge of power button.  It is inactive high
     * --------+        +-------
     *         +--------+
     *                  ^- wakeup here
     */
	
    CTL_REG_WRITE(GPIO_BASE+GPIO_GRER_OFF, 0x00000001);
    CTL_REG_WRITE(GPIO_BASE+GPIO_GFER_OFF, 0x00000000);

    /*  clear all previously detected edge bits */
    CTL_REG_WRITE(GPIO_BASE+GPIO_GEDR_OFF, 0xffffffff);

#if 0
    /*  set up an RTC int */
      v = regread(RCNR)
      v += 10                             # 1hz-->10seconds
      regwrite(RTAR, v)
      regread(RCNR)
      regwrite(RTSR, 1<<2)
#endif

#if 0				/* needed ??? */
    /*  enable some ints so we can wake up */
    CTL_REG_WRITE(ICLR, 0);	/* make 'em all irqs */
    
    CTL_REG_WRITE(ICMR,
#if 0
		  (1<<31)|	/* RTC match int */
		  (1<<17)|	/* gpio 27-11 (incl ACT button) */
#endif
		  (1<<0));	/* power button */
#endif
    
#if 0
    CTL_REG_WRITE(RTC_RTSR, 0);
#endif
    
    CTL_REG_WRITE(PCFR, PCFR_OPDE); /* kill the clock */
    
    /*  zzzzz */
    CTL_REG_WRITE(PMCR, (1<<0));
#endif // !(defined(CONFIG_PXA)||defined(CONFIG_MACH_SKIFF))
}

COMMAND(reset, command_reset, "-- software reset", BB_RUN_FROM_RAM);
void
command_reset(
    int		argc,
    const char*	argv[])
{
    bootldr_reset();
}

COMMAND(halt, command_halt, "-- power down", BB_RUN_FROM_RAM );
void
command_halt(
    int		argc,
    const char*	argv[])
{
#ifdef CONFIG_MACH_SPOT
    putstr("Power down.\r\n");
    while(1)
        GPIO_GPCR_WRITE(SPOT_POWER_LATCH);
#else
    putstr("Feature not available on this hardware.\r\n");
#endif
}

int
reportMismatch(
    void*	    addr1,
    void*	    addr2,
    unsigned long   w1,
    unsigned long   w2)
{
    putLabeledWord("addr1=0x", (unsigned long)addr1);
    putLabeledWord("addr2=0x", (unsigned long)addr2);
    putLabeledWord("w1=0x", w1);
    putLabeledWord("w2=0x", w2);

    /* signal no more comparisons */
    /* XXX add flag for 1 mismatch vs all mismatches */
    return(1);
}

COMMAND(memcmp, command_memcmp, "<addr1> <addr2> <num> [size]", BB_RUN_FROM_RAM );
void
command_memcmp(
    int		argc,
    const char* argv[])
{
    void*   dst;
    void*   src;
    int	    num;
    int	    size = 1;
    
    if (argc < 4) {
	putstr("memcpy needs args: dst src num [size]\r\n");
	return;
    }

    dst = (void*)strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
	putstr("bad addr1 param\n\r");
	return;
    }
    src = (void*)strtoul(argv[2], NULL, 0);
    if (strtoul_err) {
	putstr("bad addr2 param\n\r");
	return;
    }
    num = strtoul(argv[3], NULL, 0);
    if (strtoul_err) {
	putstr("bad num param\n\r");
	return;
    }
    if (argc > 4) {
	size = strtoul(argv[4], NULL, 0);
	if (strtoul_err) {
	    putstr("bad size param\n\r");
	    return;
	}
    }

    putstr("memcmp\n\r");
    putLabeledWord("a1: 0x", (unsigned long)src);
    putLabeledWord("a2: 0x", (unsigned long)dst);
    putLabeledWord("num: 0x", num);
    putLabeledWord("size: 0x", size);
    
    switch (size) {
	case 1:
	{
	    char*   d = (char*)dst;
	    char*   s = (char*)src;

	    while (num--) {
		if (*d != *s) {
		    if (reportMismatch(d, s, *d, *s))
			break;
		}
		s++;
		d++;
	    }
	}
	break;

	case 2:
	{
	    short*   d = (short*)dst;
	    short*   s = (short*)src;

	    while (num--) {
		if (*d != *s) {
		    if (reportMismatch(d, s, *d, *s))
			break;
		}
		s++;
		d++;
	    }
	}
	break;

	case 4:
	{
	    int*   d = (int*)dst;
	    int*   s = (int*)src;

	    while (num--) {
		if (*d != *s) {
		    if (reportMismatch(d, s, *d, *s))
			break;
		}
		s++;
		d++;
	    }
	}
	break;

	default:
	    putLabeledWord("Bad size: 0x", size);
	    break;
    }
    
}

COMMAND(ver, command_version, "-- display version info", BB_RUN_FROM_RAM );

void
command_version(
    int		argc,
    const char*	argv[])
{
    print_version_verbose(NULL);
}

COMMAND(mem, command_mem, "-- show info about memory", BB_RUN_FROM_RAM);
void
command_mem(
    int		argc,
    const char*	argv[])
{
    putstr("Flash memory info:\n\r");
    btflash_print_types();

    putstr("\n\rSDRAM memory info:\n\r");

    print_mem_size("SDRAM size:", bootinfo.bt_memend);
#if !defined(CONFIG_PXA)
    putstr("SDRAM bank0:\n\r");
    putLabeledWord("  mdcnfg = 0x",
		   CTL_REG_READ(DRAM_CONFIGURATION_BASE+MDCNFG));
    putLabeledWord("  mdrefr = 0x",
		   CTL_REG_READ(DRAM_CONFIGURATION_BASE+MDREFR));
#endif
    
}
    
static int rand_seed = 0;

void
srand(seed)
{
    seed += 0xabd826db;
    rand_seed = seed;
}

int
rand(void)
{
    int	ret = rand_seed;

    rand_seed += (rand_seed & 0x00010001) * 1000000;
    rand_seed += (rand_seed & 0x00100010) * 9999999;
    rand_seed += (rand_seed & 0x01100110) * 9999998;
    
    return (ret);
}

void
memtick(
    char*   start,
    char*   p,
    int	    tick_size,
    char*   tick_str)
{
    if ((p - start) % tick_size == 0)
	putstr(tick_str);
}
    
COMMAND(memtest2, command_memtest2, "<addr1> <addr2> -- another memory tester", BB_RUN_FROM_RAM);
void
command_memtest2(
    int		argc,
    const char* argv[])
{
	unsigned long* start;
	unsigned long* end;
	unsigned long* p;
	
	unsigned num_errs;
	unsigned max_errs = 16;
	
	if (argc < 3) {
		putstr("memtest needs args: addr1 addr2\r\n");
		return;
	}

	start = (unsigned long*)strtoul(argv[1], NULL, 0);
	if (strtoul_err) {
		putstr("bad addr1 param\n\r");
		return;
	}
	
	end = (unsigned long*)strtoul(argv[2], NULL, 0);
	if (strtoul_err) {
		putstr("bad addr2 param\n\r");
		return;
	}
	
	num_errs = 0;
	putstr("writing");
	for (p=start;p<end;p++)
	{
		*p=(unsigned long)p; //how convenient.
		if (((unsigned long)p % 0x10000) == 0)
			putstr(".");
	}
	putstr("\r\n");
	putstr("testing");
	for (p=start;p<end;p++)
	{
		if (*p != (unsigned long)p)
		{
			num_errs++;
			putLabeledWord("mismatch! wanted: 0x", (unsigned long)p);
			putLabeledWord("             got: 0x", *p);
			if (num_errs > max_errs)
			{
				putstr("max error count exceeded\n");
				goto die;
			};
		};
		if (((unsigned long)p % 0x10000) == 0)
			putstr(".");
	}
die:
	putLabeledWord("error count: ", num_errs);
};


COMMAND(memtest, command_memtest, "<addr1> <addr2> -- test mem between addrs", BB_RUN_FROM_RAM);
void
command_memtest(
    int		argc,
    const char* argv[])
{
    unsigned long*  start;
    unsigned long*  end;
    unsigned long*  p;
    int		    seed;
    int		    cmp;
    int		    rval;
    int		    tick_size = 64<<10;
    unsigned	    num_mismatches = 0;
    unsigned	    max_mismatches = 16;
    unsigned long   fillval = 0;
    int		    use_fillval = 0;
    
    if (argc < 4) {
	putstr("memtest needs args: addr1 addr2 seed\r\n");
	return;
    }

    start = (unsigned long*)strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
	putstr("bad addr1 param\n\r");
	return;
    }
    end = (unsigned long*)strtoul(argv[2], NULL, 0);
    if (strtoul_err) {
	putstr("bad addr2 param\n\r");
	return;
    }
    seed = (int)strtoul(argv[3], NULL, 0);
    if (strtoul_err) {
	putstr("bad seed param\n\r");
	return;
    }
    if (argc > 4) {
	fillval = strtoul(argv[4], NULL, 0);
	if (strtoul_err) {
	    putstr("bad fillval param\n\r");
	    return;
	}
	use_fillval = 1;
    }
    

    srand(seed);
    putstr("store vals\n\r");
    for (p = start; p <= end; p++) {
	*p = use_fillval ? fillval : rand();
	memtick((char*)start, (char*)p, tick_size, ".");
    }

    srand(seed);
    putstr("\n\rcmp vals\n\r");
    for (p = start; p <= end; p++) {
	rval = use_fillval ? fillval : rand();
	cmp = *p - rval;
	if (cmp) {
	    putstr("\n\r");
	    putLabeledWord("mismatch, want: 0x", rval);
	    putLabeledWord("           got: 0x", *p);
	    putLabeledWord("          addr: 0x", (unsigned long)p);
	    num_mismatches++;
	    if (num_mismatches > max_mismatches) {
		putstr("max_mismatches exceeded, aborting.\n\r");
		break;
	    }
	}
	memtick((char*)start, (char*)p, tick_size, ".");
    }

    putstr("\n\rcmp done\n\r");
    if (num_mismatches)
	putLabeledWord("*********num_mismatches 0x", num_mismatches);
}


COMMAND(testjffs2, command_testJFFS2, "testjffs2 <filename> <dst> -- TODO", BB_RUN_FROM_RAM);
void
command_testJFFS2(
    int		argc,
    const char* argv[])
{
    const char *filename;
    unsigned long * dst;
    
    if (argc > 1) {
	filename = argv[1];
    }
    else{
	putstr("read which file???\n\r");
	return;
    }
    if (argc > 2){
	
        dst = (void*)strtoul(argv[2], NULL, 0);
	if (strtoul_err) {
	    putstr("bad dst param\r\n");
	    return;
	}
    }
    else{
	putstr("copy the file to where???\r\n");
	return;
    }
    
    body_testJFFS2(filename,dst);
}

COMMAND(infojffs2, command_infoJFFS2, "[partition_name]", BB_RUN_FROM_RAM);
void
command_infoJFFS2(
    int		argc,
    const char* argv[])
{
    char partname[256];
    
    if (argc > 1) {
	strcpy(partname,argv[1]);	
    }
    else{
	strcpy(partname,"root");
    }
    
    body_infoJFFS2(partname);
}


COMMAND(timeflashread, command_timeFlashRead, "[partition_name]", BB_RUN_FROM_RAM);
void
command_timeFlashRead(
    int		argc,
    const char* argv[])
{
    char partname[256];
    
    if (argc > 1) {
	strcpy(partname,argv[1]);	
    }
    else{
	strcpy(partname,"root");
    }
    
    body_timeFlashRead(partname);
}

COMMAND(ls, command_p1_ls, "<dir> [partition]", BB_RUN_FROM_RAM);
void
command_p1_ls(
    int		argc,
    const char* argv[])
{
    char partname[256];
    char dirname[256];
#ifdef CONFIG_LOAD_KERNEL
    const char *kernel_part_name = (const char *)param_kernel_partition.value;
#else
    const char *kernel_part_name = "root";
#endif

    if (argc > 1) {
	strcpy(dirname,argv[1]);	
    }
    else{
	strcpy(dirname,"/");
    }
    if (argc > 2) {
	strcpy(partname,argv[2]);	
    }
    else{
	strcpy(partname,kernel_part_name);
    }

    body_p1_ls(dirname,partname);
}


SUBCOMMAND(load, file, command_p1_load_file, "[partition] [filename] [dest] -- jffs2 read file", BB_RUN_FROM_RAM, 1);
void
command_p1_load_file(
    int		argc,
    const char* argv[])
{
    char partname[256];
    char filename[256];
    unsigned char * dest = (unsigned char *) 0xc0008000;
    
    if (argc > 1) {
	strcpy(partname,argv[1]);	
    }
    else{
	strcpy(partname,"root");
    }
    if (argc > 2) {
	strcpy(filename,argv[2]);	
    }
    else{
	strcpy(filename,"cow");
    }

    if (argc > 3) {
	dest = (unsigned char *)strtoul(argv[3], NULL, 0);
	if (strtoul_err) {
	    putstr("error parsing <dest>\r\n");
	    return;
	}
    }
    body_p1_load_file(partname,filename,dest);
}



void
body_clearMem(unsigned long num,unsigned short *dst)
{
    unsigned short *ldst = dst;    
    int i;
    
    for (i=0; i < num/2; i++)
	*ldst++ = 0x0000;
}

COMMAND(clearmem, command_clearMem, "[num] [dst]", BB_RUN_FROM_RAM);
void
command_clearMem(
    int		argc,
    const char* argv[])
{

    unsigned long num = 310410; //=644*482+2
    unsigned short *dst = (unsigned short *)0xc0000000;

    
    if (argc > 1) {
	num = (unsigned long) strtoul(argv[1], NULL, 0);
	if (strtoul_err) {
	    putstr("bad num param\n\r");
	    return;
	}
    }
    if (argc > 2) {
	dst = (unsigned short *) strtoul(argv[2], NULL, 0);
	if (strtoul_err) {
	    putstr("bad dst param\n\r");
	    return;
	}
    }


    
    body_clearMem(num,dst);
    

}



COMMAND(cmpkernels, command_cmpKernels, "<partname> [dst] [src] [len]", BB_RUN_FROM_RAM);
void
command_cmpKernels(
    int		argc,
    const char* argv[])
{
    unsigned long * dst = 0;
    unsigned long * src = 0;
    unsigned long len = 0;
    const char *partname;

    if (argc > 1)
	partname = argv[1];
    else{
	putstr("which kernel partition?\n\r");
	return;
    }

    if (argc > 2){
	
        dst = (void*)strtoul(argv[2], NULL, 0);
	if (strtoul_err) {
	    putstr("bad dst param\n\r");
	    return;
	}
    }
    if (argc > 3){
	
        src = (void*)strtoul(argv[3], NULL, 0);
	if (strtoul_err) {
	    putstr("bad src param\n\r");
	    return;
	}
    }
    if (argc > 4){
	
        len = strtoul(argv[4], NULL, 0);
	if (strtoul_err) {
	    putstr("bad len param\n\r");
	    return;
	}
    }
    
    body_cmpKernels(partname,dst,src,len);
}

// This aviods the gcc assembler cache settings and seems to work more reliably.
//we should pull C-gccasm enabler if this proves solid
COMMAND(enable_caches, command_enable_caches, "[dcache] [icache]", BB_RUN_FROM_RAM);
void
command_enable_caches(
    int		argc,
    const char* argv[])
{
    int   icache_en = 0;
    int   dcache_en = 0;
    unsigned long ret;
    
    if (argc > 1) {
	dcache_en = (int) strtoul(argv[1], NULL, 0);
	if (strtoul_err) {
	    putstr("bad  dcache param\n\r");
	    return;
	}
    }

    if (argc > 2) {
	icache_en = (int) strtoul(argv[2], NULL, 0);
	if (strtoul_err) {
	    putstr("bad icache param\n\r");
	    return;
	}
    }

    ret = asmEnableCaches((unsigned long) dcache_en,(unsigned long) icache_en);
    putLabeledWord("  asmEnableCaches = ",ret);

}


void parseParamName(int argc,const char **argv,char *pName,char *pValue)
{
    int i;
    int len = 0;
    char *pAll;
    char *p;
    unsigned char canKill = 0;
    unsigned char valueSearch = 0;
    

    for (i=1; i < argc; i++){
#if 0
	putLabeledWord("i = ",i);
	putstr("argv = <");
	putstr(argv[i]);
	putstr(">\r\n");
#endif
	len+= strlen(argv[i]);
    }


    if ((pAll = (char *) mmalloc((len+1) * sizeof(char))) == NULL){
	putstr("out of room to build params list!\r\n");
	return;
    }
    pAll[0] = '\0';
    p = pAll;    
    for (i=1; i < argc; i++){
	strcpy(p,argv[i]);
	p += strlen(argv[i]);
	strcpy(p," ");
	p += 1;
    }
    // kill the last unneeded space
    *--p ='\0';
    

#if 0
    putstr("pAll = <");
    putstr(pAll);
    putstr(">\r\n");
#endif

    // eliminate = if it's there
    // we only kill the first one and only if its 
    p = pAll;
    while((p - pAll) < (strlen(argv[1]) + strlen(argv[2])+1)){

	if (canKill && (isblank(*p) || (*p == '=')))
	    valueSearch = 1;
	if (!canKill && isalnum(*p))
	    canKill = 1;
	else if (valueSearch && isalnum(*p))
	    break;
	if (canKill && (*p == '='))
	    *p = ' ';
	p++;
    }
    
#if 0
    putstr("pAll_post = <");
    putstr(pAll);
    putstr(">\r\n");
#endif

    p = pAll;
    while ((*p != ' ') && (*p != '\t'))
	p++;
    *p++ = '\0';
    
    while ((*p == ' ') || (*p == '\t'))
        p++;
    strcpy(pName,pAll);
    strcpy(pValue,p);
    
    free(pAll);
    return;
    
}

// this builds up the command buffer and either processes or prints it.
void parseParamFile(unsigned char *src,unsigned long size,int just_show)
{
    unsigned char *p = src;
    char cmdbuf[1024];
    char* cmdp;

    while ((p-src) < size){
	cmdp = cmdbuf;
	while (((p-src) < size) && (*p != '\r') && (*p != '\n'))
	    *cmdp++ = *p++;
	*cmdp = '\0';
	putstr("+ ");
	putstr(cmdbuf);
	putstr("\r\n");
	while ((*p == '\r') || (*p == '\n'))
	    p++;
	if (!just_show)
	    exec_string(cmdbuf);
    }
}

COMMAND(pef, command_pef, "[just_show]", BB_RUN_FROM_RAM);
void command_pef(int argc, const char* argv[])
{
    int just_show = 0;

    if (argc > 1) 
	just_show = strtoul(argv[1],NULL,0);	

    params_eval_file(just_show);
}

COMMAND(lli, command_lli, "-- low level coprocessor info", BB_RUN_FROM_RAM);
void command_lli(int argc, const char* argv[])
{

        putLabeledWord("ARCH INFO(CPR0)=", readCPR0());
        putLabeledWord("MMU Control (CPR1)=", readCPR1());
        putLabeledWord("TRANSLATION TABLE BASE (CPR2)=", readCPR2());
        putLabeledWord("DOMAIN ACCESS CTL (CPR3)=", readCPR3());
        putLabeledWord("FAULT STATUS (CPR5)=", readCPR5());
        putLabeledWord("FAULT ADDRESS (CPR6)=", readCPR6());
#ifdef CONFIG_PXA
        /* reading 7 and 8 is unpredictable */
        putLabeledWord("CACHE LOCK DOWN (CPR9)=", readCPR9());
        putLabeledWord("TLB LOCK DOWN (CPR10)=", readCPR10());
#endif
        putLabeledWord("MMU PROC ID  (CPR13)=", readCPR13());
        putLabeledWord("BREAKPOINT  (CPR14)=", readCPR14());
#ifdef CONFIG_PXA
        putLabeledWord("CP ACCESS (CPR15)=", readCPR15());
#endif
        putLabeledWord("Program CounterC  (PC)=", readPC());

    
}
	
COMMAND(cat, command_cat, "<file> [partname] -- show file from partition", BB_RUN_FROM_RAM);	
void command_cat(int argc, const char* argv[])
{
    char partname[256];
    char filename[256];
    long kernel_in_ram = 0;
    long size = 0;
    long i;
    
    kernel_in_ram = param_kernel_in_ram.value;

    if (argc > 1) {
	strcpy(filename,argv[1]);	
    }
    else{
	putstr("cat what file?\r\n");
    }

    
    if (argc > 2) {
	strcpy(partname,argv[2]);	
    }
    else{
	strcpy(partname,"root");
    }

    size = body_p1_load_file(partname,filename,(void*)kernel_in_ram);
    for (i=0; i < size; i++){
	if (*((char *)(kernel_in_ram + i)) == '\n')
	    putc('\r');
	putc(*((char *)(kernel_in_ram + i)));
    }
}

SUBCOMMAND(jffs2, read, command_jffs2_read, "<dstaddr> <file> [partname] -- reads jffs2 file into dram at dstaddr", BB_RUN_FROM_RAM, 2);
void command_jffs2_read(int argc, const char* argv[])
{
   const char *filename = NULL;
   const char *partname = "root";
   long buf = 0;
   long size = 0;
    
   if (argc > 0) {
      buf = strtoul(argv[0], 0, 0);
   } else {
      putstr("what dstaddr?\r\n");
   }
   if (argc > 1) {
      filename = argv[1]; 
   } else {
      putstr("what file?\r\n");
   }
   if (argc > 2) {
      partname = argv[2]; 
   }

   putstr("Reading file: "); putstr(filename); putstr(" from jffs2 filesystem in partition: "); putstr(partname); putstr(".\r\n");
   putLabeledWord("Writing contents of file to address ", buf); 
   size = body_p1_load_file(partname,filename,(void*)buf);
   putLabeledWord("Number of bytes read is ", size);
}


unsigned char isWincePartition(unsigned char *src)
{
    unsigned long *p = (unsigned long *)src;
    unsigned char ret = 1;
    int i;
    unsigned long tmp;
    
       
    //putLabeledWord("*p = ",*p);
    //putLabeledWord("WPM1 = ",WINCE_PARTITION_MAGIC_1);
    if (*p++ != WINCE_PARTITION_MAGIC_1)
	ret = 0;
    for (i=0; i < WINCE_PARTITION_LONG_0;i++){
	tmp = *p++;
	if ((tmp != 0x00000000) &&
	    (tmp != 0xFFFFFFFF))
	    ret = 0;
    }
    //putLabeledWord("*p = ",*p);
    //putLabeledWord("WPM2 = ",WINCE_PARTITION_MAGIC_2);
    if (*p++ != WINCE_PARTITION_MAGIC_2)
	ret = 0;
    
    
    return ret;
}
unsigned char amRunningFromRam(void)
{
#if defined (CONFIG_MACH_SKIFF)
        return 0;        

#else
        
        unsigned long pc = readPC();
        pc &= 0xF0000000;
        pc &= DRAM_BASE0;
        return (unsigned char) (pc>0);
#endif //defined (CONFIG_MACH_SKIFF)    
    


}


BOOL isValidBootloader(unsigned long p,unsigned long size)
{
	enum bootldr_image_status ret = isValidOHHImage(p, size);
	if (ret == BOOTLDR_IMAGE_OK)
		return TRUE;
	else if (ret != BOOTLDR_NOT_OHHIMAGE) 
		/* is ohh bootldr, so do not check for parrot */
		return FALSE;
		
	ret = isValidParrotImage(p,size);
	if (ret == BOOTLDR_IMAGE_OK)
		return TRUE;
	else
		return FALSE;
}

static enum bootldr_image_status isValidOHHImage(unsigned long p,unsigned long size)
{
	unsigned int bsd_sum;
	unsigned long start_addr;
	unsigned long mach_type = 0;
	unsigned long boot_caps;
	
	if (size < 0x2C) //exit early
		return BOOTLDR_BAD_SIZE;
	
	boot_caps = (unsigned long)(*(unsigned long *)(p+0x30));

	// right bootloader??
	if (*((unsigned long *)(p+0x20)) != BOOTLDR_MAGIC) {
		putLabeledWord("wrong magic: ", *((unsigned long *)(p+0x20)));
	        return BOOTLDR_NOT_OHHIMAGE;
	}
	
	// right arch??
	if (*((unsigned long *)(p+0x2C)) != ARCHITECTURE_MAGIC) {
		putLabeledWord("wrong arch: ", *((unsigned long *)(p+0x20)));
	        return BOOTLDR_WRONG_ARCH;
	}

#if defined(CONFIG_MACH_SKIFF)
	// linked at 0x41000000 or position independent??
	start_addr = (unsigned long)(*(unsigned long *)(p+0x28));
	if (start_addr != 0x41000000 && !(boot_caps & BOOTCAP_PIC)) {
		putLabeledWord("Not linked for flash, start_addr=", start_addr);
	        return BOOTLDR_WRONG_ADDR;
	}

#else
	// linked at 0x0 or position independent??
	start_addr = (unsigned long)(*(unsigned long *)(p+0x28));
	if (start_addr != 0x0 && !(boot_caps & BOOTCAP_PIC)) {
		putLabeledWord("Not linked for flash, start_addr=", start_addr);
	        return BOOTLDR_WRONG_ADDR;
	}
#endif // defined(CONFIG_MACH_SKIFF)
	// mach_type matches caps??
	mach_type = param_mach_type.value;
	if (((mach_type == MACH_TYPE_H3800) && !(boot_caps & BOOTCAP_H3800_SUPPORT))
	    || ((mach_type == MACH_TYPE_H3900) && !(boot_caps & BOOTCAP_H3900_SUPPORT))
	    || ((mach_type == MACH_TYPE_H5400) && !(boot_caps & BOOTCAP_H5400_SUPPORT))) {
		putLabeledWord("Does not support this CPU, boot_caps=", boot_caps);
	        return BOOTLDR_WRONG_ARCH;
	}

	//BSD Sum == 0??
	bsd_sum = bsd_sum_memory( p, size);
	if (bsd_sum != 0) {
		putstr("BSD checksum nonzero\r\n");
	        return BOOTLDR_BAD_CHECKSUM;
	}
	return BOOTLDR_IMAGE_OK;
}
static enum bootldr_image_status isValidParrotImage(unsigned long p,unsigned long size)
{
     BOOL ret = BOOTLDR_IMAGE_OK;
     unsigned char* p2;

	
     if (machine_is_jornada56x())
	  return ret;
     if (size < 0x1000)
	  return BOOTLDR_BAD_SIZE;
     if (size > 0x40000)
	  return BOOTLDR_BAD_SIZE;
	
     // we'll just check a couple of magic numbers
     if (param_mach_type.value != MACH_TYPE_H5400) {
	  if (*((unsigned long *)(p+0x0)) != PARROT_MAGIC_0){
	       putLabeledWord("parrot word 0 ->0x", *((unsigned long *)(p+0x0)));
	       putLabeledWord("Expected      ->0x", PARROT_MAGIC_0);
	       ret = BOOTLDR_BAD_MAGIC;
	  }
	
	
	  if ((*((unsigned long *)(p+0xFFC)) != PARROT_MAGIC_FFC) &&
	      (*((unsigned long *)(p+0xFFC)) != PARROT_MAGIC_FFC_ALT)){
	       putLabeledWord("parrot word 0xFFC ->0x", *((unsigned long *)(p+0xFFC)));
	       putLabeledWord("Expected      ->0x", PARROT_MAGIC_FFC);
	       putLabeledWord("or            ->0x", PARROT_MAGIC_FFC_ALT);
	       ret = BOOTLDR_BAD_MAGIC;
	  }
	
	  if (*((unsigned long *)(p+0x1000)) != PARROT_MAGIC_1000){
	       putLabeledWord("parrot word 0x1000 ->0x", *((unsigned long *)(p+0x1000)));
	       putLabeledWord("Expected           ->0x", PARROT_MAGIC_1000);
	       ret = BOOTLDR_BAD_MAGIC;
	  }
     } else {
	  /* is 5400 */
	  // we'll just check a couple of magic numbers
	  if (*((unsigned long *)(p+0)) != PARROT_MAGIC_0) {
	       putLabeledWord("wince firmware  0 ->0x", *((unsigned long *)(p+0x0)));
	       putLabeledWord("Expected          ->0x", PARROT_MAGIC_0);
	       ret = BOOTLDR_BAD_MAGIC;
	  }
	  if (*((unsigned long *)(p+0x40)) != H5400_FIRMWARE_MAGIC_40) {
	       putLabeledWord("firmware word 0x40 ->0x", *((unsigned long *)(p+0x40)));
	       putLabeledWord("Expected           ->0x", H5400_FIRMWARE_MAGIC_40);
	       ret = BOOTLDR_BAD_MAGIC;
	  }
	  if (*((unsigned long *)(p+0x1000)) != H5400_FIRMWARE_MAGIC_1000) {
	       putLabeledWord("firmware word 0x1000 ->0x", *((unsigned long *)(p+0x1000)));
	       putLabeledWord("Expected             ->0x", H5400_FIRMWARE_MAGIC_1000);
	       ret = BOOTLDR_BAD_MAGIC;
	  }
     }

     // ok installling parrot w/out having previously installed wince is bad
     // so we'll add the wince partition check to the test
     // XXX this presumes that the params sector follows the bootloader sector!!
     p2 = ((char*)flashword) + flashDescriptor->bootldr.base + flashDescriptor->bootldr.size;

     if (!isWincePartition(p2)){
	  putstr("You must load wince BEFORE loading Parrot\r\n");
	  return BOOTLDR_NOT_LOADABLE;
     }
	
     return ret;
}

COMMAND(discover, command_discover_machine_type, "-- guess what machine I am; see mach_type for result", BB_RUN_FROM_RAM);
void command_discover_machine_type(int argc, const char **argv)
{
   discover_machine_type(*boot_flags_ptr);
}


void discover_machine_type(long boot_flags)
{
#if defined(CONFIG_MACH_IPAQ)
    volatile unsigned short *p = (unsigned short *) IPAQ_EGPIO;
    volatile unsigned short *p2 = (unsigned short *) 0x0;
    volatile unsigned short tst;
 
    // first check to see if we are a 3800
    // on the 3800 this is the gpio dir register so it is safe to set it
    // we will be able to read it back if we are a 3800 and not else
    // we are setting the serial port on bit so it's a safe op
    // for the 3(1,6,7)xx models too
    set_h3600_egpio(IPAQ_EGPIO_RS232_ON);
    tst = *p2;
    tst = *p;

    if ((tst == EGPIO_IPAQ_RS232_ON)){
	param_mach_type.value = MACH_TYPE_H3800;
	putstr("Machine type--> iPAQ 3800\r\n");	
    }
    else if (machine_is_jornada56x()) {
	param_mach_type.value = MACH_TYPE_JORNADA56X;
	putstr("Machine type--> JORNADA56X\r\n");
    } else {
	if (is_flash_16bit()){
	    param_mach_type.value = MACH_TYPE_H3100;
	    putstr("Machine type--> iPAQ 3100\r\n");
	}	
	else {
	    param_mach_type.value = MACH_TYPE_H3600;
	    putstr("Machine type--> iPAQ 3600\r\n");
	}	

    }
    param_boot_flags.value = boot_flags;
#elif defined(CONFIG_MACH_H3900) || defined(CONFIG_MACH_H5400)
    if (boot_flags & BF_H3900) {
      param_mach_type.value = MACH_TYPE_H3900;
      putstr("Machine type--> iPAQ 3900\r\n");
      putLabeledWord("probing h3900 asic3  hwid[0]=", *(long*)H3900_ASIC3_ID_BASE);
    } else if (boot_flags & BF_H5400) {
      param_mach_type.value = MACH_TYPE_H5400;
      putstr("Machine type--> iPAQ3 \r\n");
    } else if (boot_flags & BF_H1900) {
      param_mach_type.value = MACH_TYPE_H1900;
      putstr("Machine type--> iPAQ 1900\r\n");
    };
#endif
}

void initialize_by_mach_type()
{
    unsigned long mach_type = param_mach_type.value;
#ifdef CONFIG_SA1100
    extern unsigned long msc1_config;
    extern unsigned long msc2_config;
    extern unsigned long msc1_config_jornada56x;
    extern unsigned long msc2_config_jornada56x;
#endif

    putLabeledWord("mach_type  ->", mach_type);

#if 0
    putLabeledWord("asic1 Gpio Mask addr show up as ->", H3800_ASIC1_GPIO_MASK_ADDR);
    putLabeledWord("asic1 Gpio Dir addr show up as ->", H3800_ASIC1_GPIO_DIR_ADDR);
    putLabeledWord("asic1 Gpio Out addr show up as ->", H3800_ASIC1_GPIO_OUT_ADDR);	
#endif

    if (machine_is_jornada56x()) {
#if defined(CONFIG_MACH_JORNADA720) || defined(CONFIG_MACH_JORNADA56X)
        ABS_MSC1 = msc1_config_jornada56x;
        ABS_MSC2 = msc2_config_jornada56x;
#endif	
    }
    else {
#if !(defined(CONFIG_PXA)|| defined(CONFIG_MACH_SKIFF))
      ABS_MSC1 = msc1_config;
      ABS_MSC2 = msc2_config;
#endif
    }
    
#if defined(CONFIG_HAL)
    hal_init(mach_type);
#endif
#if defined(CONFIG_MACH_IPAQ)  || defined(CONFIG_MACH_H3900)
    gpio_init(mach_type);
#endif
#ifdef CONFIG_LCD
    lcd_init(mach_type);
#endif
    btflash_reset_partitions();
#ifdef CONFIG_MACH_H5400
    /* turn off the vibrator */
    if (machine_is_h5400()) {
      putstr("poking gpdr to stop vibration\r\n");
      *(long*)0x40e00014 = 0x8000;
    }
#endif
    putstr("initialize_by_mach_type done\r\n");
}


/**************************************************************************************
 *
 * This is the section for our zlib experiments
 *
 * ***********************************************************************************/
COMMAND(tdz, command_tdz, "-- test decompress routines (best with ASCII) preflashing", BB_RUN_FROM_RAM);
void command_tdz(int argc, const char* argv[])
{

    size_t size;
    unsigned long address;
    unsigned long dAddress;
    int isGoodImg;
    
    size = last_ram_load_size;
	    
    
    if (argc < 3) {
	putstr("not enough args, need <srcAddress>  <destAddress>\n\r");
	return;
    }
    
    address = strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
	putstr("error parsing address\r\n");
	return;
    }
    dAddress = strtoul(argv[2], NULL, 0);
    if (strtoul_err) {
	putstr("error parsing dAddress\r\n");
	return;
    }



    isGoodImg = verifyGZipImage(address,&size);
    putLabeledWord("verify img = ",isGoodImg);
    putLabeledWord("uncompr size = ",size);

    size = last_ram_load_size;
    body_tdz(address,size,dAddress);
}


void body_tdz(unsigned long address,unsigned long size,unsigned long dAddress)
{    
    char tiny_buf[ZLIB_CHUNK_SIZE];
    struct bz_stream z;
    Byte *p;
    unsigned long uncomplen;
    
    
    gzInitStream(address,size,&z);
    
    
    
    p = (Byte *) dAddress;
    
    while ((size = gzReadChunk(&z,tiny_buf))) {
	memcpy(p,tiny_buf,size);
	p += size;	
	if (z.err != Z_OK) break;
	
    }
    putLabeledWord("finished loading with runningCRC = 0x",z.crc32);
    

    uncomplen = z.stream.total_out;
    
    putLabeledWord("uncompressed length = 0x", uncomplen);
    putLabeledWord("total_in = 0x", z.stream.total_in);
    putLabeledWord("read_crc returns = 0x", z.read_crc32);
    
    size = crc32(0,(const void *) dAddress,uncomplen);

    putLabeledWord("crc32 static calc= 0x", size);

}

//
// PocketPC(SE) - Function to erase the contents of RAM skipping the section
// that includes the bootloader code
//
void eraseRAM( unsigned long pattern1, unsigned long pattern2 )
{

  volatile unsigned long *	ptr;

  /* 
   * Pocket PCSE  - Blat the rest of the memory now that we have the splash
   * screen displayed. This may take a little time.
   */
  putstr("Erasing");
  ptr = (unsigned long *)DRAM_BASE0;
  while ((unsigned long)ptr < (DRAM_BASE0 + SZ_512M))
  {
     /*
      * Display some progress every so often
      */
     if (((unsigned long)ptr % SZ_1M) == 0)
     {
         putstr(".");

         /*
          * Check for looping in the memory and that although we are dealing
          * with a higher address value, the hardware is not looping it back
          * to a lower value that we have already erased.
          *
          * The test writes a different value and looks to see if it is
          * duplicated at the base of RAM. Any previous value is restored
          * before either contining or breaking the erase.
          */
         if (*ptr == pattern2)
         {
            *ptr = pattern1;
            if (*((unsigned long *)DRAM_BASE0) == pattern1)
            {
                *((unsigned long *)DRAM_BASE0) = pattern2;
                putstr("End Detected\r\n");
	        break;
            }
            *((unsigned long *)DRAM_BASE0) = pattern2;
         }

     }
     /*
      * Skip the section with the flash image loaded into it and variable 
      * space in 
      */
     if (((unsigned long)ptr) == (STACK_BASE - STACK_SIZE))
     {
         putstr("\r\nSkipping Copy of Flash\r\n");
         ptr = (unsigned long *)(FLASH_IMG_START + FLASH_IMG_SIZE);
     } 

     //+
     // Walk over the memory with an erase pattern. The ptr is
     // marked as volatile to stop the compiler optimising this out.
     //-
     *ptr = pattern1;
     *ptr = pattern2;
     *ptr = pattern1;
     *ptr = pattern2;
     *ptr = pattern1;
     *ptr = pattern2;
     ptr++;
  }
  putstr("\r\n");
}

#if defined(CONFIG_MACH_SKIFF)
// noop routine for skiffs as we are keeping their caches turned off !
int asmEnableCaches(int dcache_enabled, int icache_enabled)
{}
#endif //defined(CONFIG_MACH_SKIFF)
