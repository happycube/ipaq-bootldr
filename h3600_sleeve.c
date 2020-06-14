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
 * h3600 sleeve driver
 *
 */

#include "bootldr.h"
#if defined(__linux__) || defined(__QNXNTO__)
#include <asm-arm/arch-sa1100/h3600_gpio.h>
#include <asm-arm/arch-sa1100/h3600_asic.h>	
#endif
#include "cpu.h"
#include "bsdsum.h"
#include "architecture.h"
#include "hal.h"
#include "heap.h"
#include "pcmcia.h"
#include "commands.h"

#define __KERNEL__
#include "list.h"
#include <asm-arm/arch-sa1100/h3600-sleeve.h>
#include <asm-arm/arch-sa1100/backpaq.h>

static int initialized = 0;
static struct sleeve_dev *sleeve_dev;

static int h3600_sleeve_init_module(void)
{
    int result = 0;

    if (!initialized) {
        putstr(__FUNCTION__ "\r\n");

        clr_h3600_egpio(IPAQ_EGPIO_OPT_NVRAM_ON);
        clr_h3600_egpio(IPAQ_EGPIO_OPT_ON);

        sleeve_dev = (struct sleeve_dev *)mmalloc(sizeof(struct sleeve_dev));
        if (sleeve_dev == NULL) {
            putstr(__FUNCTION__ ": malloc failed\r\n");
            return -ENOMEM;
        }
        memset(sleeve_dev, 0, sizeof(struct sleeve_dev));

        h3600_generic_pcmcia_init_module();

        initialized = 1;
    }

    return result;
}

void h3600_sleeve_cleanup_module(void)
{
}

void oscrsleep(unsigned int oscrtime)
{
#ifdef CONFIG_SA1100
   OSCR = 0;
   while (OSCR < oscrtime)
      ;
#else
#  warning no definition for OSCR in msleep
#endif
}

void msleep(unsigned int msec)
{
   oscrsleep(msec*3686);
}

/*******************************************************************************/


struct {
   char start_of_id; /* 0xaa */
   int data_len;
   char version;
   short vendor_id;
   short device_id;
} sleeve_header;


/*******************************************************************************/



static struct list_head sleeve_drivers;

static const struct sleeve_device_id *
h3600_sleeve_match_device(const struct sleeve_device_id *ids, struct sleeve_dev *dev)
{
   while (ids->vendor) {
      if ((ids->vendor == SLEEVE_ANY_ID || ids->vendor == dev->vendor) &&
          (ids->device == SLEEVE_ANY_ID || ids->device == dev->device))
         return ids;
      ids++;
   }
   return NULL;
}

static int h3600_sleeve_announce_device(struct sleeve_driver *drv, struct sleeve_dev *dev)
{
   const struct sleeve_device_id *id = NULL;

   if (drv->id_table) {
      id = h3600_sleeve_match_device(drv->id_table, dev);
      if (!id)
         return 0;
   }

   dev->driver = drv;
   if (drv->probe(dev, id) >= 0) {
      return 1;
   }

   return 0;
}

void h3600_sleeve_insert(void) 
{
   struct list_head *item;

   if (!initialized)
     h3600_sleeve_init_module();

   set_h3600_egpio(IPAQ_EGPIO_OPT_NVRAM_ON);

   msleep(250);

   if (hal_spi_read(6, (char*)&sleeve_dev->vendor, 2) != 0) {
      putstr(__FUNCTION__ ": no spi read, defaulting sleeve vendor\r\n");
      sleeve_dev->vendor = COMPAQ_VENDOR_ID;
   }
   if (hal_spi_read(8, (char*)&sleeve_dev->device, 2) != 0) {
      putstr(__FUNCTION__ ": no spi read, defaulting sleeve deviceid\r\n");
      sleeve_dev->device = SINGLE_PCMCIA_SLEEVE;
   }

   putLabeledWord(" sleeve vendorid=", sleeve_dev->vendor);
   putLabeledWord(" sleeve deviceid=", sleeve_dev->device);
			
   for (item=sleeve_drivers.next; item != &sleeve_drivers; item=item->next) {
      struct sleeve_driver *drv = list_entry(item, struct sleeve_driver, node);
      if (h3600_sleeve_announce_device(drv, sleeve_dev)) {
         putstr(__FUNCTION__ ": matched driver "); putstr(drv->name); putstr("\r\n");
         return;
      }
   }

}

void h3600_sleeve_eject(void) 
{
   
   if (!initialized)
     h3600_sleeve_init_module();

   if (sleeve_dev->driver && sleeve_dev->driver->remove) {
      sleeve_dev->driver->remove(sleeve_dev);
   }

   memset(sleeve_dev, 0, sizeof(struct sleeve_dev));
   clr_h3600_egpio(IPAQ_EGPIO_OPT_NVRAM_ON);
}

int h3600_current_sleeve( void )
{

   if (!initialized)
     h3600_sleeve_init_module();

   return H3600_SLEEVE_ID( sleeve_dev->vendor, sleeve_dev->device );
}

/*******************************************************************************/

int h3600_sleeve_register_driver(struct sleeve_driver *drv)
{
   putLabeledWord("registering sleeve driver ", (long)drv);
   list_add_tail(&drv->node, &sleeve_drivers);
   return 0;
}

void
h3600_sleeve_unregister_driver(struct sleeve_driver *drv)
{
   list_del(&drv->node);
}

struct h3600_backpaq_fpga_dev_struct {
	unsigned int  usage_count;     /* Number of people currently using this device */
	unsigned long busy_count;      /* Number of times we've had to wait for EMPTY bit */
	unsigned long bytes_written;   /* Bytes written in the most recent open/close */
};

static struct h3600_backpaq_fpga_dev_struct  h3600_backpaq_fpga_data;


struct h3600_backpaq_eeprom h3600_backpaq_eeprom_shadow;

void h3600_sleeve_fpga_load(int argc, const char **argv)
{
  const char *sleeve = argv[0];
  const char *cmd = argv[1];
  char *pbuf = (char *)DRAM_BASE0;
  unsigned long bytes_to_write = 0;
  unsigned int i;
  if (argv[2]) {
    pbuf = (unsigned char *)strtoul(argv[2], 0, 0);
  }
  if (argv[3]) {
    bytes_to_write = strtoul(argv[3], 0, 0);
  }
  
  h3600_backpaq_eeprom_shadow.sysctl_start = 0x02000000ul;

  /* turn on 3.3V */
  set_h3600_egpio(IPAQ_EGPIO_OPT_ON);
  /* turn on 1.8V and clocks */
  BackpaqSysctlGenControl = 0x13;
  msleep(100);
  /* Clear the control registers to wipe out memory */
  BackpaqSysctlFPGAControl = BACKPAQ_FPGACTL_M0 | BACKPAQ_FPGACTL_M1 | BACKPAQ_FPGACTL_M2;
  msleep( 1 );       /* Wait for 1 ms */
  /* Put the FPGA into program mode */
  BackpaqSysctlFPGAControl =
    (BACKPAQ_FPGACTL_M0 | BACKPAQ_FPGACTL_M1 | BACKPAQ_FPGACTL_M2 
     | BACKPAQ_FPGACTL_PROGRAM);

  putLabeledWord("BackpaqSysctlGenControl=", BackpaqSysctlGenControl);
  putLabeledWord("BackpaqSysctlFPGAControl=", BackpaqSysctlFPGAControl);
  putLabeledWord("BackpaqSysctlFPGAStatus=", BackpaqSysctlFPGAStatus);

  /* now send the data */
  for ( i = 0 ; i < bytes_to_write ; pbuf++, i++ ) {
    int bcount = 0;
    /* Wait for the CPLD to signal it's ready for the next byte */
    while (!(BackpaqSysctlFPGAStatus & BACKPAQ_FPGASTATUS_EMPTY)) {
      if (bcount > 10) {
        putLabeledWord("  busy: status=", BackpaqSysctlFPGAStatus);
        putLabeledWord("        pbuf=", pbuf);
      }
      bcount++;
      h3600_backpaq_fpga_data.busy_count++;
    }
		
    if ((((long)pbuf) &0xFFFF) == 0) 
      putLabeledWord("  pbuf=", pbuf);
    /* Write *pbuf to the FPGA */
    BackpaqSysctlFPGAProgram = *pbuf;
  }

  h3600_backpaq_fpga_data.bytes_written = bytes_to_write;

  /* turn off programming mode */
  BackpaqSysctlFPGAControl = BACKPAQ_FPGACTL_PROGRAM | BACKPAQ_FPGACTL_M0 | BACKPAQ_FPGACTL_M2;

  putLabeledWord("BackpaqSysctlFPGAStatus=", BackpaqSysctlFPGAStatus);
  putLabeledWord("bytes_written=", h3600_backpaq_fpga_data.bytes_written);
  putLabeledWord("busy_count=", h3600_backpaq_fpga_data.busy_count);

}

/*******************************************************************************/


SUBCOMMAND(sleeve, detect, command_sleeve, "-- detects h3600 sleeve", BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(sleeve, insert, command_sleeve, "-- detects and enables h3600 sleeve", BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(sleeve, eject,  command_sleeve, "-- disables h3600 sleeve", BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(sleeve, on,     command_sleeve, "-- enables OPT_ON, does not attach driver", BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(sleeve, reset,  command_sleeve, "-- asserts OPT_RESET for 200ms", BB_RUN_FROM_RAM, 0); 
SUBCOMMAND(sleeve, fpga,   command_sleeve, "<srcaddr> <nbytes> -- load fpga from dram", BB_RUN_FROM_RAM, 0);

void
command_sleeve(
               int		argc,
               const char*	argv[])
{
  if (!initialized) {
    h3600_sleeve_init_module();
    initialized = 1;
  }
  if (strcmp(argv[1], "init") == 0) {
    h3600_sleeve_init_module();
  } else if (strcmp(argv[1], "detect") == 0) {
    int detect = 0;
    hal_get_option_detect(&detect);
    putLabeledWord("sleeve detect=", detect);
  } else if (strcmp(argv[1], "insert") == 0) {
    h3600_sleeve_insert();
  } else if (strcmp(argv[1], "eject") == 0) {
    h3600_sleeve_eject();
  } else if (strcmp(argv[1], "on") == 0) {
    set_h3600_egpio(IPAQ_EGPIO_OPT_ON);
  } else if (strcmp(argv[1], "reset") == 0) {
    set_h3600_egpio(IPAQ_EGPIO_OPT_RESET);
    msleep(200);
    clr_h3600_egpio(IPAQ_EGPIO_OPT_RESET);
  } else if (strcmp(argv[1], "fpga") == 0) {
    h3600_sleeve_fpga_load(argc, argv); 
  } else {
    putstr("usage: sleeve detect | insert | eject \r\n");
  }
}
