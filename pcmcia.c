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
 * PCMCIA/CF card support
 *
 */

#include "bootldr.h"
#if defined(__linux__) || defined(__QNXNTO__)
#ifdef CONFIG_MACH_IPAQ
#include <asm-arm/arch-sa1100/h3600_gpio.h>
#include <asm-arm/arch-sa1100/h3600_asic.h>	
#endif
#ifdef CONFIG_MACH_H3900
#include <asm-arm/arch-pxa/h3900-gpio.h>
#endif
#ifdef CONFIG_MACH_SKIFF
#include "skiff.h"
#endif
#endif
#include "bsdsum.h"
#include "commands.h"
#include "architecture.h"
#include "hal.h"
#include "ide.h"

#define __KERNEL__
#include "list.h"
#include <asm-arm/arch-sa1100/h3600-sleeve.h>
#include "pcmcia.h"
#include "cpu.h"

#ifdef CONFIG_PXA
#undef __REG
#define __REG(x) *((volatile unsigned long *) (x))
#endif

struct card_info card_info[2];

static int initialised = 0;

int generic_sa1100_pcmcia_map_mem(u8 socket, size_t len, int cis, char **mapping)
{
    if (mapping) {
        if (socket == 0)
            *mapping = (char*)0x28000000;
        else
            *mapping = (char*)0x38000000;
        if (cis == 0)
            *mapping += 0x04000000;
        return 0;
    } else {
        return -EINVAL;
    }
}

int generic_sa1100_pcmcia_map_io(u8 socket, size_t len, char **mapping)
{
    if (mapping) {
        *mapping = (char*)(socket ? 0x20000000 : 0x30000000 );
        return 0;
    } else {
        return -EINVAL;
    }
}

struct pcmcia_ops generic_pcmcia_ops_st = {
    name: "generic ops",
    map_mem: generic_sa1100_pcmcia_map_mem,
    map_io: generic_sa1100_pcmcia_map_io,
};

struct pcmcia_ops *generic_pcmcia_ops = &generic_pcmcia_ops_st;
struct pcmcia_ops *pcmcia_ops = NULL;





#if defined(CONFIG_MACH_SKIFF)
/* This is where all the skiff spec pcmcia ops code lives */

static unsigned char  *skiff_pcmcia_regs[2] = {
    (volatile unsigned char  *)(SKIFF_SOCKET_0_BASE),
    (volatile unsigned char  *)(SKIFF_SOCKET_1_BASE),
};


static int skiff_pcmcia_map_mem(u8 socket, size_t len, int cis, char **mapping)
{
        //putLabeledWord(__FUNCTION__": socket =",socket);
        //putLabeledWord(__FUNCTION__": cis =",cis);
        
        if (mapping) {
                // power/enable to the card
                skiff_pcmcia_regs[socket][SKIFF_DEVICE_PME] |= SKIFF_CARD_ENABLE|SKIFF_VCC_PWR_ENABLE|SKIFF_VPP_PWR_ENABLE;
                skiff_pcmcia_regs[socket][SKIFF_DEVICE_TIMER0_SETUP] = 0x1f;
                skiff_pcmcia_regs[socket][SKIFF_DEVICE_TIMER0_COMMAND] = 0x1f;
                skiff_pcmcia_regs[socket][SKIFF_DEVICE_TIMER0_RECOVERY] = 0x1f;
                //
                // for cis 
                // so sock 0 goes from 8010 0000 -> 801F F000
                // &  sock 1 goes from 8080 0000 -> 808F F000
                // for !cis 
                // so sock 0 goes from 8020 0000 -> 802F F000
                // &  sock 1 goes from 8090 0000 -> 809F F000

                if (cis == 0){
                        // NORMAL Memory space
                        // enable memory map 1
                        skiff_pcmcia_regs[socket][SKIFF_DEVICE_MAPPING] |= SKIFF_MM1_ENABLE;            
                        // end map 1 region
                        skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM1_END_LOW] = 0x0F;
                        // turn on REG# for cis
                        skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM1_OFF_HI] = 0x0;
                    
                        if (socket == 0){
                                //skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM1_HI] = SKIFF_16BIT | 0x02;
                                skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM1_HI] =  0x02;                                
                                *mapping = (char*)0x80200000;
                        }            
                        else{
                    
                                //skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM1_HI] = SKIFF_16BIT | 0x09;                                
                                skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM1_HI] =  0x09;                                
                                *mapping = (char*)0x80900000;
                        }

                }
                else {
                        // ATTRIBUTE Memory space                    
                        // enable memory map 0
                        skiff_pcmcia_regs[socket][SKIFF_DEVICE_MAPPING] |= SKIFF_MM0_ENABLE;            

                        // end map 0 region
                        skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM0_END_LOW] = 0x0F;
                        // turn on REG# for cis
                        skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM0_OFF_HI] = SKIFF_REG_ACTIVE;

                        if (socket == 0){
                                skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM0_HI] = SKIFF_16BIT | 0x01;                                
                                *mapping = (char*)0x80100000;
                        }            
                        else{
                                skiff_pcmcia_regs[socket][SKIFF_DEVICE_MM0_HI] = SKIFF_16BIT | 0x08;                                
                                *mapping = (char*)0x80800000;
                        }
                }

                // undo card reset
                skiff_pcmcia_regs[socket][SKIFF_DEVICE_PME2] = SKIFF_CARD_RESET_OFF;
                delay_seconds(1);
                return 0;
        } else {
                return -EINVAL;
        }

}

static int skiff_pcmcia_map_io(u8 socket, size_t len, char **mapping)
{
#if 0
    if (mapping) {
        *mapping = (char*)(socket ? 0x20000000 : 0x30000000 );
        return 0;
    } else {
        return -EINVAL;
    }
#endif
}



static int skiff_pcmcia_card_insert(u8 sock)
{

#if 0
        putLabeledWord("sock = ",sock);
        putLabeledWord("reg v socket0 state0 = 0x",skiff_pcmcia_regs[0][SKIFF_CARDBUS_STATE0]);
        putLabeledWord("reg v socket1 state0 = 0x",skiff_pcmcia_regs[1][SKIFF_CARDBUS_STATE0]);
        putLabeledWord("reg v socket0 state1 = 0x",skiff_pcmcia_regs[0][SKIFF_CARDBUS_STATE1]);
        putLabeledWord("reg v socket1 state1 = 0x",skiff_pcmcia_regs[1][SKIFF_CARDBUS_STATE1]);

        putLabeledWord("3v check: = ",skiff_pcmcia_regs[sock][SKIFF_CARDBUS_STATE1] & SKIFF_CARD_3V);
        putLabeledWord("5v check: = ",skiff_pcmcia_regs[sock][SKIFF_CARDBUS_STATE1] & SKIFF_CARD_5V);
#endif
        
        if (skiff_pcmcia_regs[sock][SKIFF_CARDBUS_STATE1] & SKIFF_CARD_3V) {
                skiff_pcmcia_regs[sock][SKIFF_CARDBUS_CONTROL0] = SKIFF_CARD_3V_VPP | SKIFF_CARD_3V_VCC;
                putLabeledWord("turning on 3 V for socket",sock);
                delay_seconds(1);
        
            
        }
        else if (skiff_pcmcia_regs[sock][SKIFF_CARDBUS_STATE1] & SKIFF_CARD_5V) {
                skiff_pcmcia_regs[sock][SKIFF_CARDBUS_CONTROL0] = SKIFF_CARD_5V_VPP | SKIFF_CARD_5V_VCC;
                putLabeledWord("turning on 5V for socket",sock);
                delay_seconds(1);
                        
        }
        else{
                //putstr("Error: card neither 5 nor 3 volts!!!\n");
                return 0;
        }
    
        return 0;
}

static int skiff_pcmcia_card_eject(u8 sock)
{

    putLabeledWord(__FUNCTION__ ": socket ",sock);
    // assert card reset
    skiff_pcmcia_regs[sock][SKIFF_DEVICE_PME2] = 0x0;
    delay_seconds(1);
    

    skiff_pcmcia_regs[sock][SKIFF_CARDBUS_CONTROL0] = 0x0;
    skiff_pcmcia_regs[sock][SKIFF_DEVICE_PME] = 0x0;
    
                    
    return 0;

}



struct pcmcia_ops skiff_pcmcia_ops = {
   name: "skiff pcmcia",
   card_insert: skiff_pcmcia_card_insert,
   card_eject: skiff_pcmcia_card_eject,
   map_mem: skiff_pcmcia_map_mem,
   map_io: skiff_pcmcia_map_io,

};




#endif //defined(CONFIG_MACH_SKIFF)











#ifdef CONFIG_PXA
#include "pcmcia-pxa.h"
#endif

static void pcmcia_init_module(void)
{
    unsigned int clock;

    putstr(__FUNCTION__ "\r\n");
#if defined(CONFIG_PXA)
    MECR = 3;

    clock = get_lclk_frequency_10khz();

    MCMEM0 = ((pxa_mcxx_setup(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	       & MCXX_SETUP_MASK) << MCXX_SETUP_SHIFT)
      | ((pxa_mcxx_asst(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	  & MCXX_ASST_MASK) << MCXX_ASST_SHIFT)
      | ((pxa_mcxx_hold(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	  & MCXX_HOLD_MASK) << MCXX_HOLD_SHIFT);
    MCMEM1 = ((pxa_mcxx_setup(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	       & MCXX_SETUP_MASK) << MCXX_SETUP_SHIFT)
      | ((pxa_mcxx_asst(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	  & MCXX_ASST_MASK) << MCXX_ASST_SHIFT)
      | ((pxa_mcxx_hold(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	  & MCXX_HOLD_MASK) << MCXX_HOLD_SHIFT);
    MCATT0 = ((pxa_mcxx_setup(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	       & MCXX_SETUP_MASK) << MCXX_SETUP_SHIFT)
      | ((pxa_mcxx_asst(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	  & MCXX_ASST_MASK) << MCXX_ASST_SHIFT)
      | ((pxa_mcxx_hold(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	  & MCXX_HOLD_MASK) << MCXX_HOLD_SHIFT);
    MCATT1 = ((pxa_mcxx_setup(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	       & MCXX_SETUP_MASK) << MCXX_SETUP_SHIFT)
      | ((pxa_mcxx_asst(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	  & MCXX_ASST_MASK) << MCXX_ASST_SHIFT)
      | ((pxa_mcxx_hold(PXA_PCMCIA_3V_MEM_ACCESS, clock)
	  & MCXX_HOLD_MASK) << MCXX_HOLD_SHIFT);
    MCIO0 = ((pxa_mcxx_setup(PXA_PCMCIA_IO_ACCESS, clock)
	      & MCXX_SETUP_MASK) << MCXX_SETUP_SHIFT)
      | ((pxa_mcxx_asst(PXA_PCMCIA_IO_ACCESS, clock)
	  & MCXX_ASST_MASK) << MCXX_ASST_SHIFT)
      | ((pxa_mcxx_hold(PXA_PCMCIA_IO_ACCESS, clock)
	  & MCXX_HOLD_MASK) << MCXX_HOLD_SHIFT);
    MCIO1 = ((pxa_mcxx_setup(PXA_PCMCIA_IO_ACCESS, clock)
	      & MCXX_SETUP_MASK) << MCXX_SETUP_SHIFT)
      | ((pxa_mcxx_asst(PXA_PCMCIA_IO_ACCESS, clock)
	  & MCXX_ASST_MASK) << MCXX_ASST_SHIFT)
      | ((pxa_mcxx_hold(PXA_PCMCIA_IO_ACCESS, clock)
	  & MCXX_HOLD_MASK) << MCXX_HOLD_SHIFT);
#elif defined(CONFIG_MACH_SKIFF)
    /* The skiff doesnt have any sleeves so the pcmcia registration takes place here :) */
    pcmcia_register_ops(&skiff_pcmcia_ops);

    delay_seconds(1);

    
#endif

 initialised = 1; 
}

void pcmcia_register_ops(struct pcmcia_ops *ops)
{
        //putLabeledWord(__FUNCTION__ ": ops=", (unsigned long)ops);
        pcmcia_ops = ops;
}

#define read_mapping_byte(m_, o_) (m_[o_] & 0xFF)
int pcmcia_read_cis(int sock)
{
    volatile unsigned short *mapping = 0;
    int i = 0;
    memset(&card_info[sock].funcid, 0, sizeof(struct card_info));
    pcmcia_map_mem(sock, SZ_1M, 1, &mapping);
    //putLabeledWord("cis mapping=", (long)mapping);
    putLabeledWord("cis[0] =", read_mapping_byte(mapping, 0));
    while (i < 256) {
        unsigned short type = (read_mapping_byte(mapping, i++));
        unsigned short len = (read_mapping_byte(mapping, i++));
        int j;
        if (type == CIS_TUPLE_END || len == 0) {
            putstr("end\r\n"); 
            break;
        }
        putstr("  "); putHexInt8(type); putstr(" "); putHexInt8(len);
        for (j = 0; j < len; j++) {
            putstr(" "); putHexInt8(read_mapping_byte(mapping, i+j)); 
        }
        putstr("\r\n");
        switch (type) {
        case CIS_TUPLE_MANFID:
            card_info[sock].manfid[0] = read_mapping_byte(mapping, i);
            card_info[sock].manfid[1] = read_mapping_byte(mapping, i+1);
            putLabeledWord("  manfid[0]=", card_info[sock].manfid[0]);
            putLabeledWord("  manfid[1]=", card_info[sock].manfid[1]);
            break;
        case CIS_TUPLE_FUNCID:
            putLabeledWord("  funcid=", read_mapping_byte(mapping, i));
            card_info[sock].funcid = read_mapping_byte(mapping, i);
            if (read_mapping_byte(mapping, i) == CIS_FUNCID_FIXED)
                putstr("    fixed disk\r\n");
            break;
        }
        i += len;
    }
    return 0;
}

int pcmcia_detect(u8 *detect)
{
        
  if (!initialised)
    pcmcia_init_module();

  if (detect) {
    *detect = 0;
#if defined(CONFIG_MACH_IPAQ)
    if (!(GPIO_GPLR_READ() & GPIO_H3600_PCMCIA_CD0))
      *detect |= 1;
    if (!(GPIO_GPLR_READ() & GPIO_H3600_PCMCIA_CD1))
      *detect |= 2;
#elif defined(CONFIG_MACH_H3900)
    if (!(GPLR0 & GPIO_H3900_PCMCIA_CD0))
      *detect |= 1;
    if (!(GPLR0 & GPIO_H3900_PCMCIA_CD1))
      *detect |= 2;
#elif defined(CONFIG_MACH_SKIFF)
    if (!(skiff_pcmcia_regs[0][SKIFF_CARDBUS_STATE0] & SKIFF_CARD_PRESENT))
            *detect |= 1;
    if (!(skiff_pcmcia_regs[1][SKIFF_CARDBUS_STATE0] & SKIFF_CARD_PRESENT))
            *detect |= 2;
    
#if 0
    putLabeledWord("socket0 iface= 0x",SKIFF_SOCKET_0(SKIFF_IFACE));
    
    putLabeledWord("socket1 iface= 0x",SKIFF_SOCKET_1(SKIFF_IFACE));
    
    putLabeledWord("socket0 state0 = 0x",SKIFF_SOCKET_0(SKIFF_CARDBUS_STATE0));
    putLabeledWord("socket1 state0 = 0x",SKIFF_SOCKET_1(SKIFF_CARDBUS_STATE0));

    putLabeledWord("reg v socket0 state0 = 0x",skiff_pcmcia_regs[0][SKIFF_CARDBUS_STATE0]);
    putLabeledWord("reg v socket1 state0 = 0x",skiff_pcmcia_regs[1][SKIFF_CARDBUS_STATE0]);
    putLabeledWord("reg v socket0 state1 = 0x",skiff_pcmcia_regs[0][SKIFF_CARDBUS_STATE1]);
    putLabeledWord("reg v socket1 state1 = 0x",skiff_pcmcia_regs[1][SKIFF_CARDBUS_STATE1]);


    
    putLabeledWord("socket0 state1 = 0x",SKIFF_SOCKET_0(SKIFF_CARDBUS_STATE1));
    putLabeledWord("socket1 state1 = 0x",SKIFF_SOCKET_1(SKIFF_CARDBUS_STATE1));
#endif
    


    
#else
#error What happens here?
#endif // defined(CONFIG_MACH_IPAQ)

  } else {
    return -EINVAL;
  }
  return 0;
}

int pcmcia_insert(void)
{
    int sock;
    
    if (!initialised)
      pcmcia_init_module();
    
    for (sock = 0; sock < 2; sock++) {
        pcmcia_card_insert(sock);
        delay_seconds(1);
        pcmcia_read_cis(sock);
        putLabeledWord(__FUNCTION__": funcid = ",card_info[sock].funcid);
        
        if (card_info[sock].funcid == CIS_FUNCID_FIXED) {
            char *mapping = 0;
            pcmcia_map_mem(sock, SZ_1M, 0, &mapping);
            ide_attach((volatile char *)mapping);
            delay_seconds(1);
        }
    }
    return 0;
}

int pcmcia_eject(void)
{
    int sock;
    for (sock = 0; sock < 2; sock++) {
        pcmcia_card_eject(sock);
    }
    return 0;
}

SUBCOMMAND(pcmcia, detect, command_pcmcia_detect, "          -- detect presence of pcmcia/cf cards", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(pcmcia, cis,    command_pcmcia_cis,    "[slotno]  -- show card information services (CIS) data ", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(pcmcia, insert, command_pcmcia_insert, "[slotno]  -- enable and bind driver to card(s)", BB_RUN_FROM_RAM, 0);
SUBCOMMAND(pcmcia, eject,  command_pcmcia_eject,  "[slotno]  -- disable card(s)", BB_RUN_FROM_RAM, 0);

void command_pcmcia_init(int argc, const char *argv[])
{
    pcmcia_init_module();
}

void command_pcmcia_detect(int argc, const char *argv[])
{
    u8 detect = 0;
    pcmcia_detect(&detect);
    putLabeledWord("pcmcia detect=", detect);
}

void command_pcmcia_cis(int argc, const char *argv[])
{
    volatile unsigned short *mapping;
    int i = 0;
    int sock = strtoul(argv[2], 0, 0);
    pcmcia_map_mem(sock, SZ_1M, 1, &mapping);
    //putLabeledWord("cis mapping=", (long)mapping);
    while (i < 256) {
        unsigned short type = (read_mapping_byte(mapping, i++));
        unsigned short len = (read_mapping_byte(mapping, i++)) ;
        int j;
        if (type == CIS_TUPLE_END || len == 0) {
            putstr("end\r\n"); 
            break;
        }
        putstr("  type="); putHexInt8(type); putstr(" len="); putHexInt8(len);
        for (j = 0; j < len; j++) {
            putstr(" "); putHexInt8(read_mapping_byte(mapping, i+j)); 
        }
        putstr("\r\n");
        switch (type) {
        case CIS_TUPLE_FUNCID:
            putLabeledWord("  funcid=", read_mapping_byte(mapping, i));
            if (read_mapping_byte(mapping, i) == CIS_FUNCID_FIXED)
                putstr("    fixed disk\r\n");
            break;
        }
        i += len;
    } 
}

void command_pcmcia_insert(int argc, const char *argv[])
{
    pcmcia_insert();
}

void command_pcmcia_eject(int argc, const char *argv[])
{
    pcmcia_eject();
}

