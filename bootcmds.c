#include "params.h"
#include "commands.h"
#include "bootldr.h"
#include "bootlinux.h"
#include "btflash.h"
#include "cpu.h"
#if defined(CONFIG_JFFS)
#include "jffs.h"
#endif
#include "lcd.h"
#include "hal.h" /* for hal_get_option_detect */
#if defined(CONFIG_VFAT)
#include "vfat.h"
#endif
#if defined(CONFIG_IDE)
#include "ide.h"
#endif
#include "pcmcia.h"
#include "heap.h"
#include "serial.h"
#include "h3600_sleeve.h"
#include "fakelibc.h"
#include "cyclone_boot.h"
#include <string.h>

extern byte strtoul_err;
extern volatile unsigned long *flashword;

extern FlashDescriptor *flashDescriptor;
extern struct bootblk_param param_initrd_filename;
extern struct bootblk_param param_kernel_filename;
static struct ebsaboot bootinfo;


int isGZipRegion(unsigned long address);


COMMAND(boot, command_boot, "[partition] -- boots off the specified/default partition (see boot_type param)",
		BB_RUN_FROM_RAM);
/* can have arguments or not */
void command_boot(int argc, const char **argv)
{
  struct bootldr_command *command;
  unsigned char* p;
  char *wp = "wince";
  
  
  if (argv[1] == NULL) {
	char *boot_type;
	boot_type = (char*) param_boot_type.value;
	argv[1] = boot_type;
	argc++;
	if (argv[1] 
		&& ((strcmp(argv[1], "ide") == 0)
			|| (strcmp(argv[1], "vfat") == 0)))
	  goto skip_wince_check;

	/* since we cant have a params table in wince, we need to check for the
	 * wince partition here and force it
	 */
	p = ((char*)flashword) + flashDescriptor->bootldr.base + flashDescriptor->bootldr.size;
	if (isWincePartition(p)){
	  char *new_argv1 = (char *) mmalloc(strlen(wp)+1);
	  strcpy(new_argv1, wp);
	  argv[1] = new_argv1;
	}
	
  skip_wince_check:
	putstr("booting ");
	putstr(argv[1]);
	putstr("...\r\n");
	
	command = get_sub_command("boot", argv[1]);
	(*command->cmdfunc)(argc - command->offset, argv + command->offset);
  }
}

/*
 * boot_kernel
 *  root_filesystem_name:
 *  kernel_region_start: virtual address of region holding kernel image
 *  kernel_region_size:  size of region holding kernel image
 *  argc: number of arguments in argv (including argv[0], the command name)
 *  argv: argument vector (including argv[0], the command name)
 */
void boot_kernel(const char *root_filesystem_name, 
                        vaddr_t kernel_region_start, size_t kernel_region_size, int argc, const char **argv, int no_copy)
{
  unsigned long *kernel_region_words;
  unsigned long kernel_magic;
  unsigned long kernel_image_source = (unsigned long)kernel_region_start ;
  unsigned long kernel_image_size = kernel_region_size;
  unsigned long kernel_image_dest;
  int i;
  char *os;
  long linuxEntryPoint = 0;
#ifdef CONFIG_YMODEM
  long ymodem = 0;
#endif
  long n_banks = 1;
  long kernel_in_ram = 0;

  kernel_image_dest = DRAM_BASE;

  if (no_copy){
       // check from the ram copy       
      kernel_region_words = (unsigned long *) (0x8000 +
					       kernel_image_dest);      
  }
  else{
      kernel_region_words = (unsigned long *)kernel_region_start;// check the flash copy
  }
  kernel_magic = kernel_region_words[0];  
  
  n_banks = param_dram_n_banks.value;
  os = (char*)param_os.value;
  linuxEntryPoint = param_entry.value; 
  kernel_in_ram = param_kernel_in_ram.value;
#ifdef CONFIG_YMODEM
  ymodem = param_ymodem.value;
#endif

  putLabeledWord("kernel partition base ", (unsigned long)kernel_region_words);

  
  putLabeledWord("kernel_magic=", kernel_magic);
  if (kernel_magic == 0xFFFFFFFFL) {
    putstr("no boot image in flash\r\n");
    return;
  } 


  
  
  putLabeledWord("kernel_region_words[9]=", kernel_region_words[9]);
  
  if (kernel_region_words[9] == LINUX_ZIMAGE_MAGIC) {
    unsigned long kernel_image_offset = 0x0;
    unsigned long mach_type = MACH_TYPE;

#if 1
    if (   kernel_region_words[0] == SKIFF_ZIMAGE_MAGIC
           || kernel_region_words[9] == LINUX_ZIMAGE_MAGIC ) {
      /* standard Linux zImage gets loaded to phyasaddr 0x8000 */
      kernel_image_offset = 0x8000;
      linuxEntryPoint += 0x8000;
    }
#endif

    putstr("Linux ELF flash_imgstart=");
    putHexInt32((long)kernel_image_source);
    putstr(" size=");
    putHexInt32(kernel_image_size);
    putstr(" dest=");
    putHexInt32(kernel_image_dest);
    putstr(" offset=");
    putHexInt32(kernel_image_offset);
    putstr("\r\n");
     
    asmEnableCaches(0,0); // also flushes caches    
    putLabeledWord("MMU Control=", readCPR1());
    putLabeledWord("MMU PIDVAM=", readCPR13());

    if (((void *)kernel_image_dest != (void *)kernel_image_source) &&
	!no_copy) {
      putstr("copying Linux kernel ... ");
      memcpy((void*)(kernel_image_dest + kernel_image_offset),
	     (void*)kernel_image_source, kernel_image_size);
      putstr("done\r\n");
    }
    else if (no_copy)
	putstr("Skipping kernel copy by request.\n\r");

    for (i = 0; i < 40; i += 4) {
      /* we still have the MMU on, kernel_image_dest gets us to the right virtual address */
      putHexInt32(kernel_image_dest + kernel_image_offset + i);
      putstr(": ");
      putHexInt32(*(unsigned long *)(kernel_image_dest + kernel_image_offset +i));
      putstr("\r\n");
    }

    if (param_copy_initrd.value && param_initrd_size.value) {
      unsigned long current_initrd_start = param_initrd_start.value;
      unsigned long current_initrd_size = param_initrd_size.value;
      unsigned long initrd_size = current_initrd_size;
      unsigned long initrd_start = STACK_BASE - initrd_size;

      putstr("Copying compressed initrd from ");
      putHexInt32(current_initrd_start);
      putstr(" to ");
      putHexInt32(initrd_start);
      putstr("...");
      memcpy((char*)initrd_start,(char*)current_initrd_start,current_initrd_size); 
      putstr("Done \r\n");
      param_initrd_start.value = initrd_start;
    }

    if (1) {
      char boot_args[MAX_BOOTARGS_LEN];
      char *linuxargs = NULL;

      putstr("root_filesystem_name="); putstr(root_filesystem_name); putstr("\r\n");
      memset(boot_args, 0, MAX_BOOTARGS_LEN);
      linuxargs = (char *)param_linuxargs.value;
      putLabeledWord("Grabbed linuxargs, argc = ",argc);      
      mach_type = param_mach_type.value;
      putLabeledWord("Using mach_type ", mach_type);
      if (linuxargs != NULL)
        strcat(boot_args, linuxargs);
      if (argc > 1) {
	  putstr("pre unparse setting boot parameters to\r\n");
	  putstr(boot_args);putstr("\r\n");
	  unparseargs(boot_args, argc-1, argv+1);
      }

      
      putstr("setting boot parameters to\r\n");
      putstr(boot_args);putstr("\r\n");

      
      
#ifdef __linux__
      setup_linux_params(kernel_image_dest, param_memc_ctrl_reg.value, boot_args);
#endif
    }

#ifdef CONFIG_LCD
    lcd_off(lcd_type);
#endif

    putLabeledWord("linuxEntryPoint=", linuxEntryPoint);
    putstr("Booting Linux image\r\n");

#if 0
    putLabeledWord("CP15 r0=", readCPR0()); 
    putLabeledWord("CP15 r1=", readCPR1()); 
    putLabeledWord("CP15 r2=", readCPR2()); 
    putLabeledWord("CP15 r3=", readCPR3()); 
    /* no 4 */
    putLabeledWord("CP15 r5=", readCPR5()); 
    putLabeledWord("CP15 r6=", readCPR6()); 
#ifdef CONFIG_PXA
    putLabeledWord("CP15 r7=", readCPR7()); 
    putLabeledWord("CP15 r8=", readCPR8()); 
    putLabeledWord("CP15 r9=", readCPR9()); 
    putLabeledWord("CP15 r10=", readCPR10()); 
#endif
    /* no 11 or 12 */
    putLabeledWord("CP15 r13=", readCPR13()); 
    putLabeledWord("CP15 r14=", readCPR14()); 
#ifdef CONFIG_PXA
    putLabeledWord("CP15 r15=", readCPR15()); 
#endif

#endif

    bootLinux(&bootinfo,
              mach_type,
#ifdef CONFIG_MACH_SKIFF
              /* hack -- linux entry point virtual address is 0x10008000, physaddr is 0x00008000 */
              /* after we disable the MMU we have to use the physical address */
              linuxEntryPoint&0x00FFFFFF
#else
              linuxEntryPoint
#endif

              );

  } else if (kernel_magic == NETBSD_KERNEL_MAGIC) {
    char boot_args[MAX_BOOTARGS_LEN];

    putstr("copying NetBSD kernel ... ");
    memcpy((void*)kernel_image_dest, (void*)kernel_image_source, kernel_image_size);
    putstr("done\r\n");

    /*bootinfo.bt_memavail = ROUNDUP(kernel_image_size, SZ_1M);*/
    bootinfo.bt_memavail = SZ_2M+SZ_1M;
    /*putLabeledWord("bt_memavail = ",bootinfo.bt_memavail);*/
      
    boot_args[0] = 0;
    strcat(boot_args, "netbsd ");
    if (argc > 1) {
      unparseargs(boot_args, argc-1, argv+1);
    }
    bootinfo.bt_args = boot_args;
    bootinfo.bt_vargp = (u_int32_t)boot_args & PAGE_MASK;
    bootinfo.bt_pargp = (u_int32_t)boot_args & PAGE_MASK;
    for(i = 0 ; i < (DRAM_SIZE0 - SZ_2M) ; i += SZ_1M) {
      unsigned long dram_paddr = DRAM_BASE + i;
      unsigned long dram_vaddr = 0xf0000000 + i;
      mmu_table[dram_vaddr >> 20] = dram_paddr | MMU_SECDESC | MMU_CACHEABLE;
    }
    asmEnableCaches(0,0);    
    putstr("done!\r\nJumping to 0xF0000020..\r\n");
    
    boot(&bootinfo,0xF0000020);
  } else {
    putstr("Unrecognized kernel image\r\n");
    return;
  }
}

/* can have arguments or not */
SUBCOMMAND(boot, flash, command_boot_flash, "[partition]", BB_RUN_FROM_RAM, 1);
void command_boot_flash(int argc, const char **argv)
{
   const char *ipaddr = NULL;
   const char *serveraddr = NULL;
   const char *gatewayaddr = NULL;
   const char *netmask = NULL;
   const char *hostname = NULL;
   const char *nfsroot = NULL;
   const char *kernel_part_name = NULL;
   char bootargs[MAX_BOOTARGS_LEN];
   struct FlashRegion *kernelPartition;

   ipaddr = (const char *)param_ipaddr.value;
   serveraddr = (const char *)param_nfs_server_address.value;
   gatewayaddr = (const char *)param_gateway.value;
   netmask = (const char *)param_netmask.value;
   hostname = (const char *)param_hostname.value;
   nfsroot = (const char *)param_nfsroot.value;
#define KERNEL_PARAM_STR    "kpart="
#define KERNEL_PARAM_STR_LEN sizeof(KERNEL_PARAM_STR)-1
   if (argc > 1 &&
       memcmp(KERNEL_PARAM_STR, argv[1], KERNEL_PARAM_STR_LEN) == 0) {
       kernel_part_name = argv[1] + KERNEL_PARAM_STR_LEN;
       /* skip over this param */
       argv[1] = argv[0];
       argv++;
       argc--;
   }
   else
       kernel_part_name = (const char *)param_autoboot_kernel_part.value;

   kernelPartition = btflash_get_partition(kernel_part_name);

   if (kernelPartition == NULL) {
       putstr("cannot find kernel partition named >");
       putstr(kernel_part_name);
       putstr("<\r\n");
     return;
   }
   else {
       putstr("booting kernel from partition >");
       putstr(kernel_part_name);
       putstr("<\r\n");
   }
   
   if (nfsroot != NULL) {
     strcat(bootargs, " nfsroot="); strcat(bootargs, nfsroot);
   }
   if ((ipaddr != NULL) || (serveraddr != NULL) || (gatewayaddr != NULL) || (netmask != NULL) || (hostname != NULL)) {
      strcat(bootargs, " ip="); strcat(bootargs, (ipaddr != NULL) ? ipaddr : "");
      strcat(bootargs, ":"); strcat(bootargs, (serveraddr != NULL) ? serveraddr : "");
      strcat(bootargs, ":"); strcat(bootargs, (gatewayaddr != NULL) ? gatewayaddr : "");
      strcat(bootargs, ":"); strcat(bootargs, (netmask != NULL) ? netmask : "");
      strcat(bootargs, ":"); strcat(bootargs, (hostname != NULL) ? hostname : "");
      strcat(bootargs, ":eth0 ");
   }
   
   argv[argc++] = bootargs;
   argv[argc] = NULL;

   boot_kernel("initrd",
               (vaddr_t)(((unsigned long)flashword) + kernelPartition->base), kernelPartition->size, argc, argv, 0);
}

/****************************************************************************/
/*
 *
 * THIS IS A TEST FOR POSSIBLY BOOTING WINCE
 *
 * **************************************************************************/
SUBCOMMAND(boot, wince, command_boot_wince, "-- boots WinCE", BB_RUN_FROM_RAM, 0);
void command_boot_wince(int argc, const char **argv)
{

    extern long pspr_wince_cookie;
#ifdef CONFIG_LCD
    lcd_off(lcd_type);
#endif
#if !(defined(CONFIG_PXA) || defined(CONFIG_MACH_SKIFF))
    CTL_REG_WRITE(PSPR, pspr_wince_cookie);
#endif // !(defined(CONFIG_PXA) || defined(CONFIG_MACH_SKIFF))
    body_clearMem(0x100000,(unsigned short *)DRAM_BASE0);
    
    bootLinux(0,
              2,
	      0x00040000
	);
}

/*
 * Boot a QNX/Neutrino kernel.
 */
SUBCOMMAND(boot, qnx, command_boot_qnx, "-- boot QNX/Neutrino", BB_RUN_FROM_RAM, 2 );
void command_boot_qnx( int argc, const char **argv )
{
    unsigned long *qnx_image_words;
    void (*qnx_start_func)( int );
    unsigned long mach_type = MACH_TYPE;
    int i;

    putstr( "Booting QNX...\n\r" );
    mach_type = param_mach_type.value;

    qnx_image_words = (unsigned long *)QNX_FLASH_IMAGE_START;
    if( argc > 0 )
    {
        if( strcmp( argv[0], "flash" ) == 0 )
            qnx_image_words = (unsigned long *)QNX_FLASH_IMAGE_START;
        else if( strcmp( argv[0], "dram" ) == 0 )
            qnx_image_words = (unsigned long *)QNX_DRAM_IMAGE_START;
    }

    /* Search the first 32 1K word boundries */ 
    for( i=0; i<32; i++ )
    {
        if ( qnx_image_words[ 256 * i ] == QNX_IMAGE_MAGIC ) {
          putLabeledWord( "Found a QNX kernel @ 0x",
			  (unsigned long)(&qnx_image_words[256*i]));
          qnx_start_func = (void *)qnx_image_words;
          drain_uart();
          bootLinux( 0, mach_type, (void *)qnx_start_func );
          return;
        }
    }

    putLabeledWord( "ERROR: No QNX IFS found - searched from 0x", 
                    (unsigned long)qnx_image_words );
    return;
}



/*
 * Kernel is already loaded in dram
 */
SUBCOMMAND(boot, addr, command_boot_addr, "<address> <size> -- boots a kernel loaded in DRAM", BB_RUN_FROM_RAM, 1);
void command_boot_addr(
    int		    argc,
    const char**    argv)
{    const char *ipaddr = NULL;
    const char *serveraddr = NULL;
    const char *gatewayaddr = NULL;
    const char *netmask = NULL;
    const char *hostname = NULL;
    const char *nfsroot = NULL;
    char bootargs[MAX_BOOTARGS_LEN];
    unsigned long    img_dest;
    unsigned long    img_size;
    
    if (argc < 3) {
	putstr("not enough args, need <address> <size>\n\r");
	return;
    }
    
    img_dest = strtoul(argv[1], NULL, 0);
    if (strtoul_err) {
	putstr("error parsing img_dest\r\n");
	return;
    }
    img_size = strtoul(argv[2], NULL, 0);
    if (strtoul_err) {
	putstr("error parsing img_size\r\n");
	return;
    }
    
    ipaddr = (const char *)param_ipaddr.value;
    serveraddr = (const char *)param_nfs_server_address.value;
    gatewayaddr = (const char *)param_gateway.value;
    netmask = (const char *)param_netmask.value;
    hostname = (const char *)param_hostname.value;
    nfsroot = (const char *)param_nfsroot.value;
    
    if (nfsroot != NULL) {
	strcat(bootargs, " nfsroot="); strcat(bootargs, nfsroot);
    }
    if ((ipaddr != NULL) || (serveraddr != NULL) || (gatewayaddr != NULL) ||
	(netmask != NULL) || (hostname != NULL)) {
	strcat(bootargs, " ip=");
	strcat(bootargs, (ipaddr != NULL) ? ipaddr : "");
	strcat(bootargs, ":");
	strcat(bootargs, (serveraddr != NULL) ? serveraddr : "");
	strcat(bootargs, ":");
	strcat(bootargs, (gatewayaddr != NULL) ? gatewayaddr : "");
	strcat(bootargs, ":");
	strcat(bootargs, (netmask != NULL) ? netmask : "");
	strcat(bootargs, ":");
	strcat(bootargs, (hostname != NULL) ? hostname : "");
	strcat(bootargs, ":eth0 ");
    }

    argv[argc++] = bootargs;
    argv[argc] = NULL;
    
    boot_kernel("dram", (vaddr_t)img_dest, img_size, argc-2, argv+2, 1);
}

#if defined(CONFIG_JFFS)

/* read kernel from jffs2 file */
SUBCOMMAND(boot, jffs, command_boot_jffs, "[boot_file] -- read kernel from jffs2 file (see boot_file param)", BB_RUN_FROM_RAM, 1);
void command_boot_jffs(int argc, const char **argv)
{
   char *kernel;
   unsigned int length;
   const char *boot_file = NULL;
   const char *conf_file = NULL;
   int jffs_changed = 1;
   char bootargs[2048], imagename[256];
   struct jffs_conf *conf;

   if(argc < 2)
       boot_file = param_boot_file.value; 
   else
       boot_file = argv[1];

   conf_file = param_conf_file.value;

   bootargs[0] = '\0';

   /* Start building the kernel argument list now, since we might append
    * to it or overwrite it depending on what we see in the conf file.
    */

   if(jffs_init(jffs_changed) < 0)
     return;
   
   jffs_changed = 0;

   if(conf_file == NULL || jffs_conf((char *)conf_file, &conf) < 0){

     if(strcmp(boot_file, "?") == 0){  /* query for default boot file */

       putstr("Default boot_file is \""); putstr(boot_file); putstr("\"\r\n");

       return;

     } else {  /* command was not a query; try to boot something */

       if(boot_file == NULL){
	 putstr("No conf file or default boot_file available\r\n");
	 return;
       }

       strcpy(imagename, boot_file);

     }

   } else {  /* use conf file */

     if(strcmp(boot_file, "?") == 0){  /* query for list of boot images */

       jffs_conf_list(conf);

       return;

     } else {  /* command was not a query; try to boot something */

       if(jffs_conf_image(conf, boot_file, imagename, bootargs) < 0){
	 putstr("Unable to determine path of bootable image\r\n");
	 return;
       }

     }

   }

   /* We should now have the name of a file which the user believes is a
    * bootable image. The moment of truth:
    */
   if((kernel = jffs_file(imagename, &length)) == NULL){
     putstr("Unable to boot file \""); putstr(imagename); putstr("\"\r\n");
     return;
   }
   
   /* Rock. */

   putstr("Booting file \""); putstr(imagename); putstr("\"...\r\n");

   /* boot_kernel() considers nearly everything in argv to be a kernel
    * argument, so don't pass things (like the boot file/label) that
    * we wouldn't want to send to the kernel:
    */
   if(argc > 1)
     argc = 1;

   argv[argc++] = bootargs;

   /* boot_kernel() will only consider our kernel arguments if argv is
    * sufficiently large:
    */
   if(argc <= 2)
     argv[argc++] = "";

   argv[argc] = NULL;


   boot_kernel("jffs", kernel, length, argc, argv, 0);
}
#endif  /* CONFIG_JFFS */

#if defined(CONFIG_IDE)
/* read kernel from ide partition 0, ramdisk from partition 1 */
SUBCOMMAND(boot, ide, command_boot_ide, "[ide_num] [ram_num] -- read kernel from ide partition 0, ramdisk from partition 1", BB_RUN_FROM_RAM, 0);
void command_boot_ide(int argc, const char **argv)
{
  char *kernel;
  unsigned int length = 0;
  int kernel_partition_number = 0;
  int initrd_partition_number = 1;
  // const char *boot_file = NULL;
  const char *conf_file = NULL;

  if (argc > 2)
    kernel_partition_number = strtoul(argv[2], NULL, 0);
  if (argc > 3)
    initrd_partition_number = strtoul(argv[3], NULL, 0);

  conf_file = (const char *)param_conf_file.value;

  /* We should now have the name of a file which the user believes is a
   * bootable image. The moment of truth:
   */
   
  kernel = (char*)DRAM_BASE + 0x8000;
  {
    unsigned int detect = 0;
    char *initrd_start = (char *)(DRAM_BASE + SZ_2M);
    hal_get_option_detect(&detect);
    if (!detect) {
      putstr("warning, no sleeve detected.  attempting pcmcia insert anyway.\r\n");
    }
    {
#ifdef CONFIG_H3600_SLEEVE
      h3600_sleeve_insert();
#endif
      delay_seconds(1);
      pcmcia_detect( (u8*)&detect);
      if (detect) {
        size_t initrd_len = 0; 
        size_t one_sector_size = 0x200; /* just read first sector */
        pcmcia_insert();
        ide_read_ptable(NULL);
        /* read kernel from first partition */
        putLabeledWord(" Reading kernel from partition number ", kernel_partition_number); 
        ide_read_partition(kernel, kernel_partition_number, NULL);

	if (strstr( (char*)param_linuxargs.value, "noinitrd") == NULL) {
	  putLabeledWord(" Reading initrd from partition number ", initrd_partition_number); 
	  param_initrd_start.value = (unsigned long) initrd_start;
	  initrd_len = ide_get_partition_n_bytes(initrd_partition_number);
	  param_use_initrd.value = 1;
	  param_copy_initrd.value = 0;
	  /* let's take a peek at the head of the partition */
	  ide_read_partition(initrd_start, initrd_partition_number, &one_sector_size);
          /* only 7MB free for initrd on all models */
          if (initrd_len > 6*SZ_1M)
             initrd_len = 6*SZ_1M;
          param_initrd_size.value = initrd_len;
	  /* use_initrd is specified or if partition is gzip, then assume it's a initrd */
	  if (param_use_initrd.value || isGZipRegion((unsigned long)initrd_start)) {
	    ide_read_partition(initrd_start, initrd_partition_number, &initrd_len);
	    param_use_initrd.value = 1;
	    /* does not need to be moved */
	    param_copy_initrd.value = 0;
	  }
	}
      }
    }
  }

  putstr("Booting ide ... argv[2]= "); putstr(argv[2]); putstr("\r\n");

  /* boot_kernel() considers nearly everything in argv to be a kernel
   * argument, so don't pass things (like the boot file/label) that
   * we wouldn't want to send to the kernel:
   */
  /* 
   * argv[0] == boot
   * argv[1] == ide
   * argv[2] == zimage-filename
   * rest are kernel args
   */
  if(argc > 3) {
    argc -= 3;
    argv += 3;
  }

  boot_kernel("ide", kernel, length, argc, argv, 1);
}
#endif  /* CONFIG_IDE */

#if defined(CONFIG_VFAT)
/* 
 * does sleeve insert (on ipaqs)
 * does pcmcia insert
 * does ide attach
 * does vfat mount
 * read and eval /boot/params
 * read zImage from kernel_filename (default is /boot/zImage)
 * read initrd from initrd_filename (default is /boot/initrd)
 * boots
 */
SUBCOMMAND(boot, vfat, command_boot_vfat, " [filename] [args...] -- boots off VFAT partition 0 (on PCMCIA if possible)", BB_RUN_FROM_RAM, 2);

void command_boot_vfat(int argc, const char **argv)
{
   char *kernel;
   unsigned int length = 0;
   const char *conf_file = NULL;
   char *kernel_name;
   char bootopts[MAX_BOOTARGS_LEN];
   int boot_vfat_partition = (int)param_boot_vfat_partition.value;
   conf_file = (const char *)param_conf_file.value;

   /* We should now have the name of a file which the user believes is a
    * bootable image. The moment of truth:
    */
   
   kernel = (char*)DRAM_BASE + 0x8000;
   {
      unsigned int detect = 0;
      char *initrd_start = (char *)(DRAM_BASE + SZ_8M);
      hal_get_option_detect(&detect);
      if (!detect) {
         putstr("warning, no sleeve detected.  attempting pcmcia insert anyway.\r\n");
      }
      {
#ifdef CONFIG_H3600_SLEEVE
         h3600_sleeve_insert();
#endif
         delay_seconds(1);
         pcmcia_detect( (u8*)&detect);
         if (detect) {
            int rc;
            size_t initrd_len = 0; 
            size_t one_sector_size = 0x200; /* just read first sector */
            pcmcia_insert();
            ide_read_ptable(NULL);
            putLabeledWord("Mounting vfat on partition ", boot_vfat_partition);
            rc = vfat_mount_partition(boot_vfat_partition);
            if (rc < 0) {
               putLabeledWord("vfat mount failed rc=", rc);
               return;
            }

            putstr(" Reading params from file: /boot/params\r\n");
            rc = vfat_read_file(kernel, "boot/params", 0);
            if (rc > 0) {
              int just_show = 0;
              parseParamFile(kernel, rc, just_show);
            }

	    if (argv[0] && strcmp(argv[0], ".")) {
	      kernel_name = (char *) argv[0];
	    } else {
	       kernel_name = (char *) param_kernel_filename.value;
	    }
            putstr(" Reading kernel from file: "); putstr(kernel_name); putstr("\r\n");
            rc = vfat_read_file(kernel, kernel_name, 0);
            if (rc < 0) {
               putLabeledWord("read zimage failed rc=", rc);
               return;
            }

	    strncpy(bootopts, (char *)param_linuxargs.value, MAX_BOOTARGS_LEN);
	    unparseargs(bootopts, argc - 1, argv + 1);
            if (strstr(bootopts, "noinitrd") == 0) {
               int rc = 0;
	       /* finds initrd in param_initrd_filename */
               putstr(" Reading initrd from file: "); putstr((char*)param_initrd_filename.value); putstr("\r\n");

               param_initrd_start.value = (unsigned long) initrd_start;
               param_use_initrd.value = 1;
               param_copy_initrd.value = 0;
               /* let's zero initrd to avoid cramfs issues, in case initrd holds cramfs */
               memset(initrd_start, 0, 8*SZ_1M);
               /* let's take a peek at the head of the partition */
               rc = vfat_read_file((char *)initrd_start, (char *)param_initrd_filename.value, one_sector_size);
               /* use_initrd is specified or if partition is gzip, then assume it's a initrd */
               if (rc > 0) {
                  initrd_len = vfat_read_file(initrd_start, (char *)param_initrd_filename.value, 0);
                  hex_dump((char *)initrd_start, 0x40);
                  param_initrd_size.value = initrd_len;
                  param_use_initrd.value = 1;
                  param_copy_initrd.value = 0;
               }
            }
            /* now unmount the vfat because we are done reading from it */
            vfat_mount_partition(boot_vfat_partition);
            /* now eject the card */
            pcmcia_eject();
            /* now eject the sleeve */
#ifdef CONFIG_H3600_SLEEVE
            h3600_sleeve_eject();
#endif
         }
      }
   }

   

   putstr("Booting vfat...\r\n");

   /* boot_kernel() considers nearly everything in argv to be a kernel
    * argument, so don't pass things (like the boot file/label) that
    * we wouldn't want to send to the kernel:
    */
   /* 
    * argv[0] == filename
    * rest are kernel args
    */
   boot_kernel("ide", kernel, length, argc, argv, 1);
}
#endif  /* CONFIG_IDE */




/****************************************************************************/
/*
 *
 * THIS IS A TEST FOR BOOTLOADER HAPPINESS
 *
 * **************************************************************************/
SUBCOMMAND(boot, boot, command_boot_boot, "-- boots the bootloader, again", BB_RUN_FROM_RAM, 0);
void command_boot_boot(int argc, const char **argv)
{
    bootLinux(0,
              2,
	      0x0
	);
}

