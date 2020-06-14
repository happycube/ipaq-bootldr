/****************************************************************************/
/* Copyright 2001 Compaq Computer Corporation.                              */
/*                                           .                              */
/* Copying or modifying this code for any purpose is permitted,             */
/* provided that this copyright notice is preserved in its entirety         */
/* in all copies or modifications.  COMPAQ COMPUTER CORPORATION             */
/* MAKES NO WARRANTIES, EXPRESSED OR IMPLIED, AS TO THE USEFULNESS          */
/* OR CORRECTNESS OF THIS CODE OR ITS FITNESS FOR ANY PARTICULAR            */
/* PURPOSE.                                                                 */
/****************************************************************************/
/*
 * bootldr.h
 */
#ifndef _PARAMS_H_
#define _PARAMS_H_
#include "bootconfig.h"
#include "bootldr.h"

#define PT_TYPE 0x7
typedef enum ParamType {
   PT_NONE = 0,
   PT_INT = 1,
   PT_STRING = 2,
   PT_READONLY = 8
} ParamType;
typedef enum ParamFormat {
   PF_DECIMAL,
   PF_HEX,
   PF_STRING,
   PF_LOGICAL
} ParamFormat;

struct bootblk_param {
   const char *name;
   ParamType   paramType;
   ParamFormat paramFormat;
   long        value;
   void        (*update)(struct bootblk_param *param, const long paramoldvalue);
};

#define __paramdata	__attribute__ ((__section__ (".data.param")))

#define INTERNAL_PARAM(name_, pt_, pf_, init_, update_) \
  struct bootblk_param __paramdata param_ ## name_ = { #name_, pt_, pf_, init_, update_ } 

#ifdef REGISTER_PARAMS
#define PARAM(name_, pt_, pf_, init_, update_) \
  struct bootblk_param __paramdata param_ ## name_ = { #name_, pt_, pf_, init_, update_ } 
#else
#define PARAM(name_, pt_, pf_, init_, update_) \
  extern struct bootblk_param param_ ## name_
#endif

extern struct bootblk_param __params_begin;
extern struct bootblk_param __params_end;

extern struct bootblk_param *get_param(const char *s);
void update_baudrate(struct bootblk_param *param, const long paramoldvalue);
void update_dcache_enabled(struct bootblk_param *param, const long paramoldvalue);
void update_icache_enabled(struct bootblk_param *param, const long paramoldvalue);
void update_os(struct bootblk_param *param, const long paramoldvalue);
void update_linux_entry_point(struct bootblk_param *param, const long paramoldvalue);
void update_unzip_kernels(struct bootblk_param *param, const long paramoldvalue);
void update_xmodem_block_size(struct bootblk_param *param, const long paramoldvalue);
void update_unzip_ramdisk(struct bootblk_param *param, const long paramoldvalue);
void update_cmdex(struct bootblk_param *param, const long paramoldvalue);
void update_autoboot_timeout(struct bootblk_param *param, const long paramoldvalue);

void set_serial_number(struct bootblk_param *param, const long paramoldvalue);
void set_system_rev(struct bootblk_param *param, const long paramoldvalue);

void parseParamFile(unsigned char *src,unsigned long size,int just_show);
void params_eval_file(int just_show);
void parseParamName(int argc,const char **argv,char *pName,char *pValue);
void params_eval( const char* prefix_in, int just_show );
void command_params_eval(int argc, const char **argv);

#define PARAM_PREFIX ("bootldr:")

#define UPDATE_BAUDRATE
#ifdef UPDATE_BAUDRATE
PARAM(baudrate, PT_INT, PF_DECIMAL, UART_BAUD_RATE, update_baudrate );
#endif

#ifdef __QNXNTO__
PARAM( os, PT_STRING, PF_STRING, (long)"qnx", NULL );  
#else
PARAM( os, PT_STRING, PF_STRING, (long)"autoselect", NULL );  /* "linux", "netbsd", "autoselect", "qnx" */
#endif

#ifdef __QNXNTO__
PARAM( boot_type, PT_STRING, PF_STRING, (long)"qnx", NULL );
#else
PARAM( boot_type, PT_STRING, PF_STRING, (long)"jffs2", NULL );
#endif

/* kernel_in_ram is the address of where kernel is loaded in ram (base of dram + 32KB (0x8000) */
PARAM( kernel_in_ram, PT_INT, PF_HEX, (DRAM_BASE0+0x8000), NULL ); 
PARAM( autoboot_kernel_part, PT_STRING, PF_STRING, (long)"kernel", NULL );
PARAM( force_unzip, PT_INT, PF_HEX, 0, NULL ); 
PARAM( noerase, PT_INT, PF_HEX, 0, NULL ); 
PARAM( override, PT_INT, PF_HEX, 0, NULL ); 
PARAM( icache_enabled, PT_INT, PF_HEX, 1, update_icache_enabled ); 
#ifdef CONFIG_MACH_H3900
#define INITIAL_DCACHE_VALUE 1
#else
#define INITIAL_DCACHE_VALUE 0
#endif
PARAM( dcache_enabled, PT_INT, PF_HEX, INITIAL_DCACHE_VALUE, update_dcache_enabled ); 
PARAM( entry, PT_INT, PF_HEX, DRAM_BASE0, NULL ); 

PARAM( use_initrd, PT_INT, PF_HEX, 0, NULL );
PARAM( copy_initrd, PT_INT, PF_HEX, 0, NULL );
PARAM( initrd_start, PT_INT, PF_HEX, 0, NULL );
PARAM( initrd_size, PT_INT, PF_HEX, 0, NULL );
PARAM( initrd_partition, PT_STRING, PF_STRING, 0, NULL );

PARAM( dram0_size, PT_INT|PT_READONLY, PF_HEX, DRAM_SIZE, NULL );
PARAM( dram1_size, PT_INT|PT_READONLY, PF_HEX, 0, NULL );
PARAM( dram_n_banks, PT_INT|PT_READONLY, PF_HEX, 1, NULL );

PARAM( memc_ctrl_reg, PT_INT, PF_HEX, 0x110c, NULL );
#if defined(CONFIG_PCI) || defined(CONFIG_MACH_SKIFF)
PARAM( mem_fclk_21285, PT_INT|PT_READONLY, PF_DECIMAL, 48000000, NULL );
PARAM( maclsbyte, PT_INT, PF_HEX, 0xFF, program_all_eeprom );
#endif
#ifdef CONFIG_MACH_SKIFF
PARAM( serial_number, PT_INT|PT_READONLY, PF_HEX, 0xFF, set_serial_number );
PARAM( system_rev, PT_INT|PT_READONLY, PF_HEX, 0x01, set_system_rev );
//PARAM( linuxargs, PT_STRING, PF_STRING, (long)" root=/dev/ram initrd ramdisk_size=8192", NULL );
PARAM( linuxargs, PT_STRING, PF_STRING, (long)" noinitrd root=/dev/mtdblock1 init=/linuxrc ", NULL );
#else
PARAM( linuxargs, PT_STRING, PF_STRING, (long)" noinitrd root=/dev/mtdblock1 init=/linuxrc console=ttySA0", NULL );
#endif
PARAM( mach_type, PT_INT, PF_DECIMAL, MACH_TYPE, NULL );

//
// Keep these in even if CONFIG_LOAD_KERNEL isn't used since too many other places use them.
//
PARAM( kernel_partition, PT_STRING, PF_STRING, (long)"root", NULL );
PARAM( initrd_filename, PT_STRING, PF_STRING, (long)"boot/initrd", NULL );
PARAM( kernel_filename, PT_STRING, PF_STRING, (long)"boot/zImage", NULL );
PARAM( ptable_addr, PT_INT, PF_HEX, DRAM_BASE0 + 0x4242, NULL );
PARAM( hostname, PT_STRING, PF_STRING, 0, NULL );
PARAM( domainname, PT_STRING, PF_STRING, 0, NULL );
PARAM( ipaddr, PT_STRING, PF_STRING, 0, NULL );
PARAM( gateway, PT_STRING, PF_STRING, 0, NULL );
PARAM( netmask, PT_STRING, PF_STRING, 0, NULL );
PARAM( dns1, PT_STRING, PF_STRING, 0, NULL );
PARAM( dns2, PT_STRING, PF_STRING, 0, NULL );
PARAM( netcfg, PT_STRING, PF_STRING, 0, NULL ); /* manual/dhcp/... */
PARAM( nfs_server_address, PT_STRING, PF_STRING, 0, NULL );
PARAM( nfsroot, PT_STRING, PF_STRING, 0, NULL );
PARAM( verbose, PT_INT, PF_HEX, 0, NULL );
PARAM( cmdex, PT_INT, PF_HEX, 1, update_cmdex );
    
    /* for formatting JFFS2 filesystem sectors */
PARAM( jffs2_sector_marker0, PT_INT, PF_HEX, 0x20031985, NULL );
PARAM( jffs2_sector_marker1, PT_INT, PF_HEX, 0x0000000C, NULL );
PARAM( jffs2_sector_marker2, PT_INT, PF_HEX, 0xE41EB0B1, NULL );

PARAM( conf_file, PT_STRING, PF_STRING, (long)"/etc/bootldr.conf", NULL );

#if defined(CONFIG_JFFS)
PARAM( boot_file, PT_STRING, PF_STRING, 0, NULL );

#endif
PARAM( boot_vfat_partition, PT_INT, PF_DECIMAL, 0, NULL );

    /*
     * button macros.  the value is a string to be executed by the main
     * command processor
     */
PARAM( recb_cmd, PT_STRING, PF_STRING, (long) "reflash", NULL);
PARAM( calb_cmd, PT_STRING, PF_STRING, (long) "ser_con", NULL);
PARAM( conb_cmd, PT_STRING, PF_STRING, (long) "usb autoinit", NULL);
PARAM( qb_cmd, PT_STRING, PF_STRING, (long) "boot vfat", NULL);
PARAM( startb_cmd, PT_STRING, PF_STRING, (long) "boot", NULL);
PARAM( upb_cmd, PT_STRING, PF_STRING, 0, NULL);
PARAM( rightb_cmd, PT_STRING, PF_STRING, 0, NULL);
PARAM( leftb_cmd, PT_STRING, PF_STRING, 0, NULL);
PARAM( downb_cmd, PT_STRING, PF_STRING, 0, NULL);
PARAM( action_cmd, PT_STRING, PF_STRING, 0, NULL);

    /* suppress the splash screen */
PARAM( suppress_splash, PT_STRING, PF_STRING, 0, NULL );

    /* reboot after this interval */
PARAM( autoboot_timeout, PT_INT, PF_DECIMAL, 10, update_autoboot_timeout );
PARAM( splash_filename, PT_STRING, PF_STRING, 0, NULL );
PARAM( splash_partition, PT_STRING, PF_STRING, 0, NULL );

#if defined(CONFIG_YMODEM)
PARAM( ymodem, PT_INT, PF_HEX, 1, NULL );
PARAM( ymodem_g, PT_INT, PF_HEX, 0, NULL );
#endif

PARAM( boot_flags, PT_INT, PF_HEX, 0, 0 );

PARAM( enable_mmu, PT_INT, PF_DECIMAL, 0, NULL );

#endif // _PARAMS_H_
