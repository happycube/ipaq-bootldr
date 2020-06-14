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
 * drivers/pcmcia/h3600_generic
 *
 * PCMCIA implementation routines for H3600 iPAQ standards sleeves
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

#define __KERNEL__
#include "list.h"
#include <asm-arm/arch-sa1100/h3600-sleeve.h>
#include "linkup-l1110.h"
#include "pcmcia.h"

#ifdef CONFIG_PXA
struct linkup_l1110 *dual_pcmcia_linkup[2] = {
    (struct linkup_l1110 *)(0x1a000000 / 2),
    (struct linkup_l1110 *)(0x19000000 / 2)
};
#else
struct linkup_l1110 *dual_pcmcia_linkup[2] = {
    (struct linkup_l1110 *)0x1a000000,
    (struct linkup_l1110 *)0x19000000
};
#endif

/*************************************************************************************/
/*     
       Compaq Dual Sleeve
*/
/*************************************************************************************/


static int dual_pcmcia_card_insert(u8 sock)
{
    int vs_3v = 0; /* otherwise 5V */
    int prs = dual_pcmcia_linkup[sock]->prc; /* read prc returns status */
    int prc;
    if ((prs & LINKUP_PRS_VS1) == 0)
        vs_3v = 1;

    /* Linkup Systems L1110 with TI TPS2205 PCMCIA Power Switch */
    /* S1 is VCC5#, S2 is VCC3# */ 
    /* S3 is VPP_VCC, S4 is VPP_PGM */
    /* PWR_ON is wired to #SHDN */
    prc = (LINKUP_PRC_APOE | LINKUP_PRC_SOE | LINKUP_PRC_S1 | LINKUP_PRC_S2
           | (sock * LINKUP_PRC_SSP));
    /* assume vpp is same as vcc */
    if (vs_3v)
      prc &= ~LINKUP_PRC_S2;
    else
      prc &= ~LINKUP_PRC_S1;

    prc |= LINKUP_PRC_S3;
    dual_pcmcia_linkup[sock]->prc = prc;
    return 0;
}

static int dual_pcmcia_card_eject(u8 sock)
{
    int vs_3v = 0; /* otherwise 5V */
    int prs = dual_pcmcia_linkup[sock]->prc; /* read prc returns status */
    int prc;
    if ((prs & LINKUP_PRS_VS1) == 0)
        vs_3v = 1;
    /* Linkup Systems L1110 with TI TPS2205 PCMCIA Power Switch */
    /* S1 is VCC5#, S2 is VCC3# */ 
    /* S3 is VPP_VCC, S4 is VPP_PGM */
    /* PWR_ON is wired to #SHDN */
    prc = (LINKUP_PRC_APOE | LINKUP_PRC_SOE | LINKUP_PRC_S1 | LINKUP_PRC_S2
           | (sock * LINKUP_PRC_SSP));

    dual_pcmcia_linkup[sock]->prc = prc;
    return 0;
}

struct pcmcia_ops dual_pcmcia_ops = {
   name: "dual pcmcia",
   card_insert: dual_pcmcia_card_insert,
   card_eject: dual_pcmcia_card_eject
};


static int dual_pcmcia_probe_sleeve(struct sleeve_dev *sleeve_dev, 
                                    const struct sleeve_device_id *ent)
{
   putstr(__FUNCTION__ "\r\n");
   set_h3600_egpio(IPAQ_EGPIO_OPT_ON);

   /* initialize the linkup chips */
   dual_pcmcia_linkup[0]->prc = (LINKUP_PRC_S2|LINKUP_PRC_S1);
   dual_pcmcia_linkup[0]->prc = (LINKUP_PRC_S2|LINKUP_PRC_S1|LINKUP_PRC_SSP);

   pcmcia_register_ops(&dual_pcmcia_ops);
   return 0;
}

static void dual_pcmcia_remove_sleeve(struct sleeve_dev *sleeve_dev)
{
   putstr(__FUNCTION__ "\r\n");
   clr_h3600_egpio(IPAQ_EGPIO_OPT_ON);
}

static struct sleeve_device_id dual_pcmcia_tbl[] = {
   { COMPAQ_VENDOR_ID, DUAL_PCMCIA_SLEEVE },
   { 0, }
};

static struct sleeve_driver dual_pcmcia_driver = {
   name:     "Compaq Dual PC Card Sleeve",
   id_table: dual_pcmcia_tbl,
   probe: dual_pcmcia_probe_sleeve,
   remove: dual_pcmcia_remove_sleeve,
};

/*************************************************************************************/
/*     
       Compact Flash sleeve 
*/
/*************************************************************************************/

static struct pcmcia_ops cf_pcmcia_ops = {
   name: "single cf",
};

static int cf_probe_sleeve(struct sleeve_dev *sleeve_dev, const struct sleeve_device_id *ent)
{
   putstr(__FUNCTION__ "\r\n");
   set_h3600_egpio(IPAQ_EGPIO_OPT_ON);
   pcmcia_register_ops(&cf_pcmcia_ops);
   return 0;
}

static void cf_remove_sleeve(struct sleeve_dev *sleeve_dev)
{
   putstr(__FUNCTION__ "\r\n");
   clr_h3600_egpio(IPAQ_EGPIO_OPT_ON);
}

static struct sleeve_device_id cf_tbl[] = {
   { COMPAQ_VENDOR_ID, SINGLE_COMPACTFLASH_SLEEVE },
   { 0, }
};

static struct sleeve_driver cf_driver = {
   name:     "Compaq Compact Flash Sleeve",
   id_table: cf_tbl,
   probe:    cf_probe_sleeve,
   remove:   cf_remove_sleeve,
};


/*************************************************************************************/
/*     
       Single slot PCMCIA sleeve 
*/
/*************************************************************************************/

static struct pcmcia_ops single_pcmcia_ops = {
   name: "single pcmcia",
};

static int pcmcia_probe_sleeve(struct sleeve_dev *sleeve_dev, 
                               const struct sleeve_device_id *ent)
{
    putstr(__FUNCTION__ "\r\n");
    set_h3600_egpio(IPAQ_EGPIO_OPT_ON);
    delay_seconds(1);
    /* probe for dual pcmcia sleeve, until spi is working */
    putstr("  probing for dual pcmcia sleeve\r\n");
    dual_pcmcia_linkup[0]->prc = 0;
    putLabeledWord("  dual_pcmcia_linkup[0]->prc=", dual_pcmcia_linkup[0]->prc);
    if (dual_pcmcia_linkup[0]->prc != 0) {
        dual_pcmcia_probe_sleeve(sleeve_dev, ent);
    } else {
        pcmcia_register_ops(&single_pcmcia_ops);
    }
    return 0;
}

static void pcmcia_remove_sleeve(struct sleeve_dev *sleeve_dev)
{
   putstr(__FUNCTION__ "\r\n");
   clr_h3600_egpio(IPAQ_EGPIO_OPT_ON);
}

static struct sleeve_device_id pcmcia_tbl[] = {
   { COMPAQ_VENDOR_ID, SINGLE_PCMCIA_SLEEVE },
   { 0, }
};

static struct sleeve_driver pcmcia_driver = {
   name:     "Compaq PC Card Sleeve",
   id_table: pcmcia_tbl,
   probe:    pcmcia_probe_sleeve,
   remove:   pcmcia_remove_sleeve,
};


/*************************************************************************************/

int h3600_generic_pcmcia_init_module(void)
{
   putstr(__FUNCTION__ ": registering sleeve drivers\r\n");
   h3600_sleeve_register_driver(&cf_driver);
   h3600_sleeve_register_driver(&pcmcia_driver);
   h3600_sleeve_register_driver(&dual_pcmcia_driver);
   return 0;
}

void h3600_generic_pcmcia_exit_module(void)
{
   putstr(__FUNCTION__ ": unregistering sleeve drivers\r\n");
   h3600_sleeve_unregister_driver(&cf_driver);
   h3600_sleeve_unregister_driver(&pcmcia_driver);
   h3600_sleeve_unregister_driver(&dual_pcmcia_driver);
} 

