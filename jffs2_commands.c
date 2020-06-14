#include "bootldr.h"
#include "btpci.h"
#include "btflash.h"
#include "btusb.h"
#include "heap.h"
#include "xmodem.h"
#include "jffs.h"
#include "lcd.h"
#include "params.h"
#include "pbm.h"
#include "commands.h"
#include "params.h"
#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900)
#include "aux_micro.h"
#include "zlib.h"
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

#include <string.h>

/* pretty simple, really.  we just need to
* 1) load the /boot/params file to the 0xC0008000 space (this should be more than large enough)
* 2) then go through and build up each line and parse it
*/
#ifdef CONFIG_LOAD_KERNEL
#define TMP_PARTITION_NAME "throwaway_bootldr_partition"
void params_eval_file(
    int just_show)
{

    long kernel_in_ram = DRAM_BASE0 + 0x8000;
    long size = 0;
    struct part_info part;
    struct FlashRegion *tmpPart;
    unsigned long bootldr_size  ;
    unsigned long bootldr_end ;
    
    if (!flashDescriptor) {
	putstr("flashDescriptor is NULL!\r\n");
	return;
    }
    
    bootldr_size = flashDescriptor->bootldr.size;
    bootldr_end = flashDescriptor->bootldr.base + bootldr_size;
    
    // given that there is no params sector, we need a temporary partition
    // covering the rest of flash to make a file system out of.
    btflash_define_partition(TMP_PARTITION_NAME, bootldr_end,
			     0x0,
			     LFR_EXPAND | LFR_JFFS2);
    
    tmpPart = btflash_get_partition(TMP_PARTITION_NAME);

    part.offset = ((char *) flashword) + tmpPart->base;
    part.erasesize = flashDescriptor->sectors[1] - flashDescriptor->sectors[0];
    part.size = tmpPart->size;
    
    putLabeledWord("params_eval_file: tmpPart->base  =",tmpPart->base);
    putLabeledWord("params_eval_file: tmpPart->size  =",tmpPart->size);
    
    size = jffs2_1pass_load(kernel_in_ram,&part,"/boot/params");

    putLabeledWord("pef: loaded a file of size ",size);
    
    
    if (size > 0)
	parseParamFile(kernel_in_ram,size,just_show);
    else
	putstr("failed to load params file /boot/params ...\r\n");


    // cleanup
    btflash_delete_partition(TMP_PARTITION_NAME);
}

#if 0
void
splash_file(const char * fileName, const char *partName)
{
    unsigned long size;
    unsigned long kernel_in_ram = 0xC0008000;
    char *kernel_part_name = NULL;
    struct FlashRegion *kernelPartition;
    struct part_info part;
    
    // working space
    kernel_in_ram = param_kernel_in_ram.value; 

    // partition to load file from
    if (partName){
	kernel_part_name = mmalloc(strlen(fileName)+1);
	strcpy(kernel_part_name,partName);
    }
    else{
      kernel_part_name = param_kernel_partition.value;
    }
    
    kernelPartition = btflash_get_partition(kernel_part_name);
    part.size = kernelPartition->size;
    /* for uniformly sized flash sectors */
    part.erasesize = flashDescriptor->sectors[1] - flashDescriptor->sectors[0];
    part.offset = ((char*)flashword) + kernelPartition->base;

    if (partName)
	mfree(kernel_part_name);
    
    if ((size = jffs2_1pass_load(kernel_in_ram,&part,fileName)) == 0){
	putstr("bad splash load for file: ");
	putstr(fileName);
	putstr("\r\n");
	return;
    }
    
    display_pbm(kernel_in_ram,size);
}
#endif


/* can have arguments or not */
SUBCOMMAND(boot, jffs2, command_boot_jffs2, "[boot_file] -- read kernel from jffs2 file (see boot_file param)", BB_RUN_FROM_RAM, 1);
void command_boot_jffs2(int argc, const char **argv)
{
   const char *ipaddr = NULL;
   const char *serveraddr = NULL;
   const char *gatewayaddr = NULL;
   const char *netmask = NULL;
   const char *hostname = NULL;
   const char *nfsroot = NULL;
   const char *kernel_file_name = NULL;
   const char *kernel_part_name = NULL;
   char bootargs[MAX_BOOTARGS_LEN];
   struct FlashRegion *kernelPartition;
   struct part_info part;
   unsigned long ret;
   long kernel_in_ram = 0;


   bootargs[0] = '\0';
   ipaddr = (char *) param_ipaddr.value;
   serveraddr = (char *) param_nfs_server_address.value;
   gatewayaddr = (char *) param_gateway.value;
   netmask = (char *) param_netmask.value;
   hostname = (char *) param_hostname.value;
   nfsroot = (char *) param_nfsroot.value;
   kernel_part_name = (char *) param_kernel_partition.value;


   kernelPartition = btflash_get_partition(kernel_part_name);
   if (argc > 1){
       kernel_file_name = argv[1];
       /* skip over this param */
       argv[1] = argv[0];
       argv++;
       argc--;
   }
   else kernel_file_name = (char *)param_kernel_filename.value;

   if (kernelPartition == NULL) {
       putstr("cannot find kernel partition named >");
       putstr(kernel_part_name);
       putstr("<\r\n");
     return;
   }
   else {
       putstr("booting ");
       putstr(kernel_file_name);
       putstr(" from partition >");
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
   
   if (strlen(bootargs)){
       argv[argc++] = bootargs;
       argv[argc] = NULL;
   }
   


   /* ok, so we copy the file to 32K ourselves and ask boot_kernel to skip the
    * copy.   that should be all that we need.
    */
   kernel_in_ram = param_kernel_in_ram.value;
   part.size = kernelPartition->size;
   /* for uniformly sized flash sectors */
   part.erasesize = flashDescriptor->sectors[1] - flashDescriptor->sectors[0];
   part.offset = ((char*)flashword) + kernelPartition->base;
    
   ret = jffs2_1pass_load((unsigned long *) kernel_in_ram,&part,kernel_file_name);   
   putstr("loaded file of size = 0x"); putHexInt32(ret);
   putstr(" at location 0x");putHexInt32(kernel_in_ram);putstr("\r\n");
   

   boot_kernel("ramdisk",
	       (vaddr_t)(((unsigned long)flashword) + kernelPartition->base), kernelPartition->size, argc, argv, 1);


}


int
body_testJFFS2(const char *filename,unsigned long *dest)
{
  struct part_info part;
  int i;
  const struct kernel_loader *the_loader = NULL;
  struct FlashRegion *flashRegion = btflash_get_partition("root");
  unsigned long ret;
  
  
    putstr("Attempting to output file " );
    putstr(filename);
    putstr("\n");

    if (!flashRegion) {
	putstr("could not find partition "); putstr("root"); putstr("\r\n");
	return -1;
    }
    part.size = flashRegion->size;
    /* for uniformly sized flash sectors */
    part.erasesize = flashDescriptor->sectors[1] - flashDescriptor->sectors[0];
    part.offset = ((char*)flashword) + flashRegion->base;
    
    putLabeledWord("root part size =", part.size);

  for (i = 0; loader[i]; i++) {
    if (loader[i]->check_magic(&part)) {
      the_loader = loader[i];
      break;
    }
  }
  if (!the_loader) {
    putstr("no kernel found\r\n");
    return -1;
  }
  else {
      putstr("partition is of type ");
      putstr(the_loader->name);
      putstr(" \r\n");
  }

  ret = jffs2_test_load((u32 *) dest,&part,filename);

  putLabeledWord("returned  = ", ret); putstr("\r\n");
  return 0;
}


int
body_infoJFFS2(char *partname)
{
    struct part_info part;
    struct FlashRegion *flashRegion= btflash_get_partition(partname);
    unsigned long ret;

  
  
    if (!flashRegion) {
	putstr("could not find partition "); putstr("root"); putstr("\r\n");
	return -1;
    }
    part.size = flashRegion->size;
    /* for uniformly sized flash sectors */
    part.erasesize = flashDescriptor->sectors[1] - flashDescriptor->sectors[0];
    part.offset = ((char*)flashword) + flashRegion->base;
    
    ret = jffs2_info(&part);
    return ret;
}


int
body_timeFlashRead(char *partname)
{
    struct part_info part;
    int i;
    const struct kernel_loader *the_loader = NULL;
    struct FlashRegion *flashRegion= btflash_get_partition(partname);
    unsigned long ret;

  
  
    if (!flashRegion) {
	putstr("could not find partition "); putstr("root"); putstr("\r\n");
	return -1;
    }
    part.size = flashRegion->size;
    /* for uniformly sized flash sectors */
    part.erasesize = flashDescriptor->sectors[1] - flashDescriptor->sectors[0];
    part.offset = ((char*)flashword) + flashRegion->base;
    
    for (i = 0; loader[i]; i++) {
	if (loader[i]->check_magic(&part)) {
	    the_loader = loader[i];
	    break;
	}
    }
    if (!the_loader) {
	putstr("no loader found for the partition\r\n");
	return -1;
    }
    ret = jffs2_scan_test(&part);    
    return ret;
}


int
body_p1_ls(char *dir,char *partname)
{
    struct part_info part;
    struct FlashRegion *flashRegion= btflash_get_partition(partname);

  
  
    if (!flashRegion) {
	putstr("could not find partition "); putstr(partname); putstr("\r\n");
	return -1;
    }
    part.size = flashRegion->size;
    /* for uniformly sized flash sectors */
    part.erasesize = flashDescriptor->sectors[1] - flashDescriptor->sectors[0];
    part.offset = ((char*)flashword) + flashRegion->base;
    
    jffs2_1pass_ls(&part,dir);
    return 0;
}


long
body_p1_load_file(const char *partname,const char *filename,unsigned char *dest)
{
    struct part_info part;
    struct FlashRegion *flashRegion= btflash_get_partition(partname);
    unsigned long ret;

  
  
    if (!flashRegion) {
	putstr("could not find partition "); putstr(partname); putstr("\r\n");
	return -1;
    }
    part.size = flashRegion->size;
    /* for uniformly sized flash sectors */
    part.erasesize = flashDescriptor->sectors[1] - flashDescriptor->sectors[0];
    part.offset = ((char*)flashword) + flashRegion->base;
    
    ret = jffs2_1pass_load(dest,&part,filename);
#if 0
    putLabeledWord("loaded file of size =", ret);    
#endif

    return ret;
    
}


void
body_cmpKernels(const char *partname,unsigned long *dstFlash,unsigned long *srcJFFS,unsigned long len)
{
  int i;
  struct FlashRegion *flashRegion = btflash_get_partition(partname);
  unsigned char *pF = (unsigned char *) dstFlash;
  unsigned char *pJ = (unsigned char *) srcJFFS;
  
  
  if (!flashRegion){
      putstr("invalid partition ");putstr(partname);putstr("\r\n");
      return;
  }
  
      
      
  // copy the flash to ram
  putstr("copying Linux kernel ... ");
  memcpy((void*)(dstFlash),
	 (void*)flashRegion->base, flashRegion->size);
  putstr("done\r\n");
  
  // now do a bytewise compare.
  for (i=0; i<len;i++){
      if (*pF++ != *pJ++){
	  putLabeledWord("cmp failed at i =", (long) i);
	  putLabeledWord("pF =", (long) pF-1);
	  putLabeledWord("*pF =", (long) *(pF-1));
	  putLabeledWord("pJ =", (long) pJ-1);
	  putLabeledWord("*pJ =", (long) *(pJ-1));
	  return;	  
      }
  }
  putLabeledWord("kernels match up to ", len);         
}

#else

void params_eval_file(
    int just_show)
{}
void
splash_file(char * fileName, char *partName)
{}
void command_boot_jffs2(int argc, const char **argv)
{}
int
body_testJFFS2(const char *filename,unsigned long *dest)
{ return 0; }
int
body_infoJFFS2(char *partname)
{ return 0; }
int
body_timeFlashRead(char *partname)
{ return 0; }
int
body_p1_ls(char *dir,char *partname)
{ return 0; }
long
body_p1_load_file(const char *partname,const char *filename,unsigned char *dest)
{
return 0;}


void body_cmpKernels(const char *partname,unsigned long *dstFlash,unsigned long *srcJFFS,unsigned long len)
{};

#endif //CONFIG_LOAD_KERNEL
