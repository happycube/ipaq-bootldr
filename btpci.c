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
 * btpci.c - Firmware configuration of PCI for Compaq Personal Server
 */

#include "bootldr.h"
#include "btpci.h"
#include "commands.h"
#include "params.h"

/* some typedefs required by pcireg.h */
//typedef dword u_int32_t;
typedef word u_int16_t;
typedef byte u_int8_t;
#include "pcireg.h"

#define readInt8(a,o) (*(volatile byte*)(((char*)(a))+o))
#define readInt16(a,o) (*(volatile word*)(((char*)(a))+o))
#define readInt32(a,o) (*(volatile dword*)(((char*)(a))+o))

#define writeInt8(a,o,v) (*(volatile byte*)(((char*)(a))+o) = (v))
#define writeInt16(a,o,v) (*(volatile word*)(((char*)(a))+o) = (v))
#define writeInt32(a,o,v) (*(volatile dword*)(((char*)(a))+o) = (v))

static void bset8(volatile char *addr, u_int32_t offset, u_int8_t mask)
{
   u_int32_t temp;
   temp = readInt8(addr, offset);
   temp |= mask;
   writeInt8(addr, offset, temp);
}

static void bclr8(volatile char *addr, u_int32_t offset, u_int8_t mask)
{
   u_int32_t temp;
   temp = readInt8(addr, offset);
   temp &= ~mask;
   writeInt8(addr, offset, temp);
}

static void bset16(volatile char *addr, u_int32_t offset, u_int16_t mask)
{
   u_int32_t temp;
   temp = readInt16(addr, offset);
   temp |= mask;
   writeInt16(addr, offset, temp);
}

static void bclr16(volatile char *addr, u_int32_t offset, u_int16_t mask)
{
   u_int32_t temp;
   temp = readInt16(addr, offset);
   temp &= ~mask;
   writeInt16(addr, offset, temp);
}

static void bset32(volatile char *addr, u_int32_t offset, u_int32_t mask)
{
   u_int32_t temp;
   temp = readInt32(addr, offset);
   temp |= mask;
   writeInt32(addr, offset, temp);
}

static void bclr32(volatile char *addr, u_int32_t offset, u_int32_t mask)
{
   u_int32_t temp;
   temp = readInt32(addr, offset);
   temp &= ~mask;
   writeInt32(addr, offset, temp);
}


#define makePCIConfigurationAddress(dev_, fcn_, offset_) ((char*)(0x7b000000L | (3<<22) | ((dev_)<<11) | ((fcn_)<<8) | (offset_)))

#if 0
#define STANDARD_COMMAND (PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_MASTER_ENABLE|PCI_COMMAND_INVALIDATE_ENABLE|PCI_COMMAND_PARITY_ENABLE|PCI_COMMAND_SERR_ENABLE|PCI_COMMAND_BACKTOBACK_ENABLE)
#else
#define STANDARD_COMMAND (PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_MASTER_ENABLE|PCI_COMMAND_INVALIDATE_ENABLE|PCI_COMMAND_PARITY_ENABLE|PCI_COMMAND_SERR_ENABLE|PCI_COMMAND_BACKTOBACK_ENABLE)
#endif

#define USB_DELAY 1000000

/*
 * Command and status register.
 */

struct PciConfigurationValues {
  const char *name;
  int   pciDevice;                   /* which PCI device this info is for */
  int   pciFunction;                 /* which PCI function in a multifunction device this info is for */
  dword interruptPin;                /* which interrupt pin on the chip to use */
  dword interruptLine;               /* which interrupt line on 21285 it's tied to -- 0x40 + 21285.IRQEnable.bitnum 
                                      *   -- the 0x40 is so that we can use netbsd interrupt steering unmodified.
                                      */   
  dword BAR0;			     /* base address register 0 */
  dword BAR1;			     /* base address register 1 */
  dword commandEnables;              /* these bits get OR'd into the command CSR of the device */
  void (*handleQuirks)(struct PciConfigurationValues *); 
};

static void handleCirrusCardbusBridgeQuirks(struct PciConfigurationValues *);
static void handleOptiUSBControllerQuirks(struct PciConfigurationValues *);
static void handlePCNetQuirks(struct PciConfigurationValues *);

#define CARDBUS_DEVICE_NUMBER 0
#define OHCI_DEVICE_NUMBER 1
#define PCNET_DEVICE_NUMBER 2

struct PciConfigurationValues pciConfigurationValues [] = {
   { "Cardbus Socket1", CARDBUS_DEVICE_NUMBER, 0, 
     PCI_INTERRUPT_PIN_A,  
     0x40 + IRQ_INTA_BITNUM,
     CARDBUS_SOCKET1_MEMORY_BASE,
     0,
     STANDARD_COMMAND,
     handleCirrusCardbusBridgeQuirks },
   { "Cardbus Socket2", CARDBUS_DEVICE_NUMBER, 1, 
     PCI_INTERRUPT_PIN_B, 
     0x40 + IRQ_INTB_BITNUM, 
     CARDBUS_SOCKET2_MEMORY_BASE, 
     0, 
     STANDARD_COMMAND,
     handleCirrusCardbusBridgeQuirks },
   { "USB Controller", OHCI_DEVICE_NUMBER, 0, 
     PCI_INTERRUPT_PIN_A,
     0x40 + IRQ_INTC_BITNUM,            /* PCI_INTERRUPT_PIN_A on OPTi is wired to 21285's PCI_INTERRUPT_PIN_C */ 
     USB_MEMORY_BASE, /* first block of PCI memory space */
     0,
     STANDARD_COMMAND,
     handleOptiUSBControllerQuirks },
   { "Ethernet Controller", PCNET_DEVICE_NUMBER, 0, 
     PCI_INTERRUPT_PIN_A,
     0x40 + IRQ_INTD_BITNUM,            /* PCI_INTERRUPT_PIN_A on Ethernet chip is wired to 21285's PCI_INTERRUPT_PIN_D */ 
     ETHERNET_IO_BASE,
     0,  /* was ETHERNET_MEMORY_BASE, for memory mapped IO, but seems to be hardwired in driver to expect an IO and not a Memory address */
     STANDARD_COMMAND,
     handlePCNetQuirks },
   { NULL, 0, 0, 0, 0, 0, 0, 0, NULL }
};

/* 
 *  ad[1:0] 
 *  ad[7:2] configuration doubleword
 *  ad[10:8] fcn number
 *  ad[15:11] device
 *  ad[23:16] bus number
 */



void formatCardbusPresentState(long reg)
{
   
}


typedef struct RegDescr {
  short size;
  short offset;
  const char *name;
  void  (*format)(long value);
} RegDescr;

RegDescr pciConfigRegDescrs[] = {
  { 4, 0x00, "ID", NULL },
  { 4, 0x04, "StatusCommand", NULL },
  { 4, 0x08, "ClassCode:Revision", NULL },
  { 4, 0x0c, "BIST:Header:Latency:CLS", NULL },
  { 4, 0x10, "MemBAR", NULL },
  { 4, 0x14, "CardbusStatus", NULL },
  { 4, 0x18, "=LatTimr:SubBusNum:CardbusNum:PCINum", NULL },
  { 4, 0x1c, "MemBase0", NULL },
  { 4, 0x20, "MemLimit0", NULL },
  { 4, 0x24, "MemBase1", NULL },
  { 4, 0x28, "MemLimit1", NULL },
  { 4, 0x2c, "IOBase0", NULL },
  { 4, 0x30, "IOLimit0", NULL },
  { 4, 0x34, "IOBase1", NULL },
  { 4, 0x38, "IOLimit1", NULL },
  { 4, 0x3c, "BridgeCtl:IntPin:IntLine", NULL },
  { 4, 0x40, "SubsystemID", NULL },
  { 4, 0x44, "LegacyBAR", NULL },
  { 4, 0x48, "DMASlaveReg", NULL },
  { 4, 0x4c, "SocketNumber", NULL },
  { 4, 0x80, "PowerManagement", NULL },
  { 4, 0x90, "DMASlaveConfig", NULL },
  { 4, 0x94, "SocketNumber", NULL },
  { 4, 0x98, "MiscConfig1", NULL },
  { 0, 0, "", NULL } 
};

RegDescr cardbusRegDescrs[] = {
  { 4, 0x0, "StatusEvent", NULL },
  { 4, 0x4,  "StatusMask", NULL },
  { 4, 0x8, "PresentState", formatCardbusPresentState },
  { 4, 0xc, "EventForce", NULL },
  { 4, 0x10, "Control", NULL },
  { 0, 0, ", NULL" }
};

RegDescr excaRegDescrs[] = {
  { 1, 0x00, "0x800 ChipRev", NULL },
  { 1, 0x01, "=>InterfaceStatus", NULL },
  { 1, 0x02, "=>PowerCtl", NULL },
  { 1, 0x03, "IntGenCtl", NULL },
  { 1, 0x04, "CardStatusChange", NULL },
  { 1, 0x05, "MgmtIntCfg", NULL },
  { 1, 0x06, "MappingEnable", NULL },
  { 1, 0x07, "IOWindowCtl", NULL },

  { 1, 0x16, "=>MiscCtl1", NULL },
  { 1, 0x1e, "MiscCtl2", NULL },
  { 1, 0x125, "=>MiscCtl3(0xf)", NULL },
  { 1, 0x12f, "MiscCtl4", NULL },
  { 1, 0x130, "MiscCtl5", NULL },
  { 1, 0x131, "MiscCtl6", NULL },
  { 1, 0x126, "=>SMB Socket Power Control Addr", NULL },
  { 1, 0x17, "FIFO Ctl", NULL },
  { 1, 0x1f, "ChipInfo", NULL },

  { 1, 0x26, "ATA Ctl", NULL },
  { 1, 0x27, "Scratch", NULL },

  { 2, 0x10, "GenMap0 StartAddr", NULL },
  { 2, 0x12, "GenMap0 EndAddr", NULL },
  { 1, 0x40, "GenMap0 UpperAddr", NULL },
  { 2, 0x14, "GenMap0 OffsetAddr", NULL },

  { 2, 0x18, "GenMap1 StartAddr", NULL },
  { 2, 0x1a, "GenMap1 EndAddr", NULL },
  { 1, 0x41, "GenMap1 UpperAddr", NULL },
  { 2, 0x1c, "GenMap1 OffsetAddr", NULL },

  { 2, 0x20, "GenMap2 StartAddr", NULL },
  { 2, 0x22, "GenMap2 EndAddr", NULL },
  { 1, 0x42, "GenMap2 UpperAddr", NULL },
  { 2, 0x24, "GenMap2 OffsetAddr", NULL },

  { 2, 0x20, "GenMap3 StartAddr", NULL },
  { 2, 0x22, "GenMap3 EndAddr", NULL },
  { 1, 0x43, "GenMap3 UpperAddr", NULL },
  { 2, 0x24, "GenMap3 OffsetAddr", NULL },

  { 2, 0x30, "GenMap4 StartAddr", NULL },
  { 2, 0x32, "GenMap4 EndAddr", NULL },
  { 1, 0x44, "GenMap4 UpperAddr", NULL },
  { 2, 0x34, "GenMap4 OffsetAddr", NULL },

  { 2, 0x08, "GenMap5 StartAddr", NULL },
  { 2, 0x0a, "GenMap5 EndAddr", NULL },
  { 1, 0x45, "GenMap5 UpperAddr", NULL },
  { 2, 0x36, "GenMap5 OffsetAddr", NULL },

  { 2, 0x0c, "GenMap6 StartAddr", NULL },
  { 2, 0x0e, "GenMap6 EndAddr", NULL },
  { 1, 0x46, "GenMap6 UpperAddr", NULL },
  { 2, 0x38, "GenMap6 OffsetAddr", NULL },

  { 1, 0x3a, "SetupTiming0", NULL },
  { 1, 0x3b, "CommandTiming0", NULL },
  { 1, 0x3c, "RecoveryTiming0", NULL },
  { 1, 0x3d, "SetupTiming1", NULL },
  { 1, 0x3e, "CommandTiming1", NULL },
  { 1, 0x3f, "RecoveryTiming1", NULL },

  { 1, 0x103, "ExtensionCtl1", NULL },
  { 1, 0x10a, "ExternalData", NULL },
  { 1, 0x10b, "ExtensionCtl2", NULL },

  { 1, 0x122, "PCI Space Ctl", NULL },
  { 1, 0x123, "PC Card Space Ctl", NULL },
  { 1, 0x124, "WindowTypeSelect", NULL },

  { 1, 0x127, "GenMap0 ExtraCtl", NULL },
  { 1, 0x128, "GenMap1 ExtraCtl", NULL },
  { 1, 0x129, "GenMap2 ExtraCtl", NULL },
  { 1, 0x12a, "GenMap3 ExtraCtl", NULL },
  { 1, 0x12b, "GenMap4 ExtraCtl", NULL },
  { 1, 0x12c, "GenMap5 ExtraCtl", NULL },
  { 1, 0x12d, "GenMap6 ExtraCtl", NULL },

  { 1, 0x12e, "ExtCardStatusChange", NULL },

  { 1, 0x134, "MaskRevision", NULL },
  { 1, 0x135, "ProdID", NULL },
  { 2, 0x136, "DevCapA", NULL },
  { 4, 0x138, "=>DevImpl", NULL },

  { 0, 0, ", NULL" }
};

static void showRegs(const char *regClass, volatile dword *regs, RegDescr *regDescrs)
{
  long lregs = (long)regs;
  int i = 0;
  RegDescr *regDescr = &regDescrs[i];
  while (regDescr->size != 0) {
    int offset = regDescr->offset;
    int reg = 0;
    putstr(regClass); putstr("\t"); 
    putHexInt32(lregs+offset); putstr(": "); 
    switch (regDescr->size) {
    case 1: reg = readInt8(regs, offset); putHexInt8(reg); break;
    case 2: reg = readInt16(regs, offset); putHexInt16(reg); break;
    case 4: reg = readInt32(regs, offset); putHexInt32(reg); break;
    }
    putstr("\t"); putstr(regDescr->name);
    if (regDescr->format != NULL) {
       putstr("\t");
       (regDescr->format)(reg);
    }
    putstr("\r\n");

    i++;
    regDescr = &regDescrs[i];
  }
}

enum {
  CCB_CONFIGURE,
  CCB_REGS,
  CCB_RESET_CARD,
  CCB_ENABLE_CARD,
  CCB_DISABLE_CARD,
  CCB_CONFIGURATION_READ,
  CCB_CONFIGURATION_WRITE,
  CCB_POWER,
  CCB_NUM
};

static struct {
  char *name;
  int tag;
} cardbusSubcommands[] = {
  { "configure", CCB_CONFIGURE },
  { "config", CCB_CONFIGURE },
  { "regs", CCB_REGS },
  { "reset", CCB_RESET_CARD },
  { "enable", CCB_ENABLE_CARD },
  { "disable", CCB_DISABLE_CARD },
  { "read", CCB_CONFIGURATION_READ },
  { "write", CCB_CONFIGURATION_WRITE },
  { "power", CCB_POWER },
  { NULL, CCB_NUM },
};


#if 0
COMMAND(cardbus, testCirrusCardbusBridge, "-- tests cirrus cardbus bridge", BB_RUN_FROM_RAM);
void testCirrusCardbusBridge(char *args)
{
   struct PciConfigurationValues *pcv = &pciConfigurationValues[0];
   volatile char *socketConfig = (volatile char *)makePCIConfigurationAddress(pcv->pciDevice, pcv->pciFunction, 0);
   long socketOperationPCIAddr = (long)readInt32(socketConfig,PCI_MAPREG_START);
   volatile byte *socketOperationPhysAddr = (volatile byte *)(((long)socketOperationPCIAddr)|0x80000000L);
   int socketNumber = readInt32(socketConfig, 0x4c);
   long cardPCIAddr = readInt32(socketConfig, 0x1c);
   long cardPCIEndAddr = readInt32(socketConfig, 0x20);
   long cardOffset = 0;
   char *cardPhysAddr = (char *)(cardPCIAddr | 0x80000000L);
   int i;

   for (i = 0; i < CCB_NUM; i++) {
     if (strncmp(args, cardbusSubcommands[i].name, strlen( cardbusSubcommands[i].name)) == 0) {
       break;
     }
   }
   putstr("args=");
   putstr(args); putstr("\r\n");
   putstr("cmd="); putstr(cardbusSubcommands[i].name); putstr("\r\n");
   putstr("tag="); putHexInt8(cardbusSubcommands[i].tag); putstr("\r\n");
   if (cardbusSubcommands[i].tag == CCB_NUM)
     return;
   switch (cardbusSubcommands[i].tag) {
   case CCB_CONFIGURE: {
      /* disable write posting */
      bclr8(socketConfig, 0x3f, 4);

     /* set up card configuration memory space */
     /* start addr */
     writeInt8(socketOperationPhysAddr, 0x810, ((cardPCIAddr >> 12) & 0xff));
     writeInt8(socketOperationPhysAddr, 0x811, ((cardPCIAddr >> 20) & 0x0f) | 0x80);
     writeInt8(socketOperationPhysAddr, 0x840, ((cardPCIAddr >> 24) & 0xff));

     /* end addr */
     writeInt8(socketOperationPhysAddr, 0x812, ((cardPCIEndAddr >> 12) & 0xff));
     writeInt8(socketOperationPhysAddr, 0x813, ((cardPCIEndAddr >> 20) & 0x0f) | 0x0);

   /* cardmem offset */
     writeInt8(socketOperationPhysAddr, 0x814, ((cardOffset >> 12) & 0xff));
     writeInt8(socketOperationPhysAddr, 0x815, ((cardOffset >> 20) & 0x3f) | 0x40);

     writeInt8(socketOperationPhysAddr, 0x817, 0x70);

   /* enable window */
     bset8(socketOperationPhysAddr, 0x806, 1);

     putstr("\r\nMapping Registers Configured\r\n");
   }
   break;

   case CCB_POWER: {
      putLabeledWord("PowerControl: ", readInt8(socketOperationPhysAddr, 0x802));
      putLabeledWord("MiscCtl1[3Vbit1]: ", readInt8(socketOperationPhysAddr, 0x816));
      showRegs("Cardbus", (volatile dword *)socketOperationPhysAddr, cardbusRegDescrs);
      putstr("\r\n");

      /* turn on the power */
      putstr("turning on the power: ... "); putHexInt32(socketOperationPhysAddr[9]); 

      if (readInt8(socketOperationPhysAddr, 9) & 8) {
         /* 3.3V card */
         if (0) bset8(socketOperationPhysAddr, 0x10, (3 << 4));
         writeInt8(socketOperationPhysAddr, 0x816, 1); /* 3V card */
         putstr("  3.3V card\r\n");
      }
      if (readInt8(socketOperationPhysAddr, 9) & 4) {
         /* 5V card */
         if (0) bset8(socketOperationPhysAddr, 0x10, (2 << 4));
         writeInt8(socketOperationPhysAddr, 0x816, 0); /* 5V card */
         putstr("  5V card\r\n");
      }
      
      /* turn on power and enable the card */
      writeInt8(socketOperationPhysAddr, 0x802, 0x10);
      {
         int i;
         /* delay loop */
         for (i = 0; i < 1000; i++)
            writeInt8(socketOperationPhysAddr, 0x802, 0x10);
      }

      /* turn on power and enable the card */
      writeInt8(socketOperationPhysAddr, 0x802, 0x90);
      writeInt8(socketOperationPhysAddr, 0x803, 0x60);

      putstr("  done\r\n");
   } break;

   case CCB_REGS: {
     showRegs("PCIConf", (volatile dword *)socketConfig, pciConfigRegDescrs);
     putstr("\r\n");
     showRegs("Cardbus", (volatile dword *)socketOperationPhysAddr, cardbusRegDescrs);
     putstr("\r\n");
     showRegs("   exCa", (volatile dword *)(socketOperationPhysAddr+0x800), 
	      excaRegDescrs);
     putstr("\r\n");
   
   }
   break;

   case CCB_RESET_CARD: {
     putstr("Issuing card reset\r\n");
     {
       int i;
       bclr8(socketOperationPhysAddr, 0x803, 0x40);
       /* delay 20 ms */
       for (i = 0; i < 200 * 1000 * 20; i++)
	 /* spin */;
       /* turn off reset */
       bset8(socketOperationPhysAddr, 0x803, 0x40);
     }
   }
   break;

   case CCB_ENABLE_CARD: {
     putstr("Enabling card\r\n");
     {
       int i;
       bset8(socketOperationPhysAddr, 0x802, 0x80);
     }
   }
   break;

   case CCB_DISABLE_CARD: {
     putstr("Disabling card\r\n");
     {
       int i;
       bclr8(socketOperationPhysAddr, 0x802, 0x80);
     }
   }
   break;

   case CCB_CONFIGURATION_READ: {
     u_int32_t data[0x10];
     /* look at card registers */
     int i;
     for (i = 0; i < 0x40; i += 4) {
        data[i >> 2] = readInt32(cardPhysAddr, i);
     }
     for (i = 0; i < 0x40; i += 4) {
       putHexInt32((long)cardPhysAddr + i); putstr(":\t"); 
       putHexInt32(data[i>>2]);
       putstr("\r\n");
     }
   }
   case CCB_CONFIGURATION_WRITE: {
     u_int32_t data[0x10];
     /* look at card registers */
     int i;
     for (i = 0; i < 0x40; i += 4) {
        data[i >> 2] = readInt32(cardPhysAddr, i);
     }
     for (i = 0; i < 0x40; i += 4) {
       putHexInt32((long)cardPhysAddr + i); putstr(":\t"); 
       putHexInt32(data[i>>2]);
       putstr("\r\n");
     }
   }
   break;
   }
}
#endif

static void handleCirrusCardbusBridgeQuirks(struct PciConfigurationValues *pcv)
{
   volatile char *socketConfig = (volatile char *)makePCIConfigurationAddress(pcv->pciDevice, pcv->pciFunction, 0);
   long socketOperationPCIAddr = (long)readInt32(socketConfig,PCI_MAPREG_START);
   volatile byte *socketOperationPhysAddr = (volatile byte *)(((long)socketOperationPCIAddr)|0x80000000L);
   int socketNumber = readInt32(socketConfig, 0x4c);
   
   /* latency timer */
   bset32(socketConfig, 0x0c, 0x78 << 8);
   bset32(socketConfig, 0x18, 0x78 << 24);

   /* footbridge latency timers */
   bset32((char*)0x42000000, 0x0c, 0x78 << 8);

   /* bus numbers */
   writeInt32(socketConfig, 0x18, 
              (((1+socketNumber) << 16) /* subordinate bus number */
               | ((1+socketNumber) << 8) /* cardbus number */
               | (0 << 0) /* pci bus number */
               ));
#ifdef DeadCode
   /* memory range 0 */
   writeInt32(socketConfig, 0x1c, (long)socketOperationPCIAddr + 0x60000);     /* base */
   writeInt32(socketConfig, 0x20, (long)socketOperationPCIAddr + 0x70000 - 1); /* limit */
   /* memory range 1 */
   writeInt32(socketConfig, 0x24, (long)socketOperationPCIAddr + 0x70000);     /* base */
   writeInt32(socketConfig, 0x28, (long)socketOperationPCIAddr + 0x80000 - 1); /* limit */

   /* IO range 0 */
   writeInt32(socketConfig, 0x2c, 0x0);     /* base */
   writeInt32(socketConfig, 0x30, 255); /* limit */
   /* IO range 1 */
   writeInt32(socketConfig, 0x34, 256);     /* base */
   writeInt32(socketConfig, 0x38, 511); /* limit */
#endif

   bclr32(socketConfig, 0x3c, (1 << 26)); /* disable write posting, p58 of 6832 spec */

   /* setup legacy mode base address (IO space to talk to this controller), p62 of 6832 spec.*/
   writeInt32(socketConfig, 0x44, (0x3e0 + socketNumber) | /* inuse */ 0x01);

   /* power management register, p67 of PD6833 spec.*/
   if ((readInt32(socketConfig, 0x80) & 0xff) == 0x01) {
      /* supports power management */
      putLabeledWord("PowerManagementCapabilities: ", readInt32(socketConfig, 0x80));
      writeInt32(socketConfig, 0x84, 0x0);
   }

#ifdef Broken
   /* disable fifo's */
   bset32(socketOperationPhysAddr, 0x814, 0x6f << 24);
#endif

   /* device implementation byte A, p138 of 6832 spec */
   socketOperationPhysAddr[0x938] |= 0xf;

   /* configuring power control, pp128-129 of 6832 spec */
   socketOperationPhysAddr[0x926] = 0xa1;
   socketOperationPhysAddr[0x925] = 0xf;

#if 0
   putstr("\r\n");

   putLabeledWord("PowerControl: ", socketOperationPhysAddr[0x802]);
   putLabeledWord("MiscCtl1[3Vbit1]: ", socketOperationPhysAddr[0x816]);
   showRegs("Cardbus", (volatile dword *)socketOperationPhysAddr, cardbusRegDescrs);
   putstr("\r\n");

   putstr("\r\n");
   showRegs("PCIConf", (volatile dword *)socketConfig, pciConfigRegDescrs);
   putstr("\r\n");
   showRegs("Cardbus", (volatile dword *)socketOperationPhysAddr, cardbusRegDescrs);
   putstr("\r\n");
   showRegs("   exCa", (volatile dword *)(socketOperationPhysAddr+0x800), 
	    excaRegDescrs);
   putstr("\r\n");
#endif
   
}

static void handleOptiUSBControllerQuirks(struct PciConfigurationValues *pcv)
{
   /* some hacks to set up the USB controller properly */
   volatile dword *usbcfg = (volatile dword *)makePCIConfigurationAddress(pcv->pciDevice, pcv->pciFunction, 0);
   volatile dword *usbmem = (volatile dword *)(readInt32(usbcfg, PCI_MAPREG_START) | 0x80000000L);

   /* a hack to test connectivity to USB controller */
   writeInt32(usbcfg, 0x50, readInt32(usbcfg, 0x50) | (1 << 3)); /* enable writability of subsystem ID register */
   putstr("TSTUSB(");
   writeInt32(usbcfg, 0x2c, 0xAAAA5555L);
   putHexInt32(readInt32(usbcfg, 0x2c));
   putc(',');
   writeInt32(usbcfg, 0x2c, 0x5555AAAAL);
   putHexInt32(readInt32(usbcfg, 0x2c));
   putc(',');
   writeInt32(usbcfg, 0x2c, 0xFFFFFFFFL);
   putHexInt32(readInt32(usbcfg, 0x2c));
   putstr(")");

   /* cache size, latency timer */
   putstr("USBCSTM(");
   putHexInt32(readInt32(usbcfg, 0x0C));
   writeInt32(usbcfg, 0x0C, 0x0d08);
   putc(',');
   putHexInt32(readInt32(usbcfg, 0x0C));
   putc(')');

     /* setting the subsystem vendor ID register */
   writeInt32(usbcfg, 0x2C, 0xc8611045L);

   /* put the controller into reset state */
   putstr("USBHCCR[0](");
   putHexInt32(readInt32(usbmem, 0x4));
   writeInt32(usbmem, 0x4, 0);
   putc(',');
   putHexInt32(readInt32(usbmem, 0x4));
   putc(')');

     /* put it into reset state, in case it wasn't there */

           
     /* interrupt assignment and strap option overrides */
#if 1
   /* enable 5V supply on rev A skfmlb */

     /* disable all interrupts */
   writeInt32(usbmem, 0x14, 0x00);
   putstr("USBSTRP(");
   putHexInt32(readInt32(usbcfg, 0x50));
   writeInt32(usbcfg, 0x50, 0x00060108);
   putc(',');
   putHexInt32(readInt32(usbcfg, 0x50));
   putstr(")");
   writeInt32(usbcfg, 0x4c, 
	  ((1 << 16)  /* enable I2C port (test0/test1) */
	   | (3 << 17) /* drive a 1 on both outputs */
	   | (1 << 21) /* enable output */
	   ));

   writeInt32(usbmem, 0x48, readInt32(usbmem, 0x48) | 0x00000200);
#endif
   putstr("]");
}



void bootConfigurePCI(void)
{
   int i;
   /*   putstr("Configuring PCI\r\n");*/
   putstr("[");
   for (i = 0; i < 256; i++) {
      struct PciConfigurationValues *pcv = &pciConfigurationValues[i];
      byte *csrBase = (byte*)makePCIConfigurationAddress(pcv->pciDevice, pcv->pciFunction, 0);
      if (pcv->name == NULL)
         break;
      
#ifdef BTPCI_VERBOSE
      putstr("Configuring "); putstr(pcv->name); putstr("\r\n");
#endif /* BTPCI_VERBOSE */
      putstr(pcv->name);
      putstr("(");

      /* configure the interrupt register */
      *(volatile dword *)(csrBase+PCI_INTERRUPT_REG) = 
         (pcv->interruptPin << PCI_INTERRUPT_PIN_SHIFT)
         | (pcv->interruptLine << PCI_INTERRUPT_LINE_SHIFT);
#if BTPCI_DEBUG
      *(volatile dword *)(csrBase+PCI_MAPREG_START) = 0xFFFFFFFFL;
      putLabeledWord("   mapreg request: ",
		     *(volatile dword *)(csrBase+PCI_MAPREG_START));
      putHexInt32(*(volatile dword *)(csrBase+PCI_MAPREG_START)); putstr(",");
#endif

      /* configure base address registers */
      if (pcv->BAR0 != 0)
         *(volatile dword *)(csrBase+PCI_MAPREG_START) = pcv->BAR0;
      if (pcv->BAR1 != 0)
         *(volatile dword *)(csrBase+PCI_MAPREG_START+4) = pcv->BAR1;
      putstr(", MaxLat:"); putHexInt8(PCI_LATTIMER(readInt32(csrBase, PCI_BHLC_REG)));
      putstr(", MinGnt:"); putHexInt8((readInt32(csrBase, PCI_INTERRUPT_REG) >> 16) & 0xFF);
      bset32(csrBase, PCI_BHLC_REG, (0xFF << PCI_LATTIMER_SHIFT));

#if 1
      *(volatile dword *)(csrBase+PCI_COMMAND_STATUS_REG) = pcv->commandEnables;
#endif
#ifdef BTPCI_VERBOSE
      putLabeledWord("   cmdstatus: ",
		     *(volatile dword *)(csrBase+PCI_COMMAND_STATUS_REG));
#endif /* BTPCI_VERBOSE */
      putHexInt32(*(volatile dword *)(csrBase+PCI_COMMAND_STATUS_REG));
      putstr(",");
      
#ifdef BTPCI_VERBOSE
      putLabeledWord("   class: ", *(volatile dword *)(csrBase+PCI_CLASS_REG));
#endif /* BTPCI_VERBOSE */
      putHexInt32(*(volatile dword *)(csrBase+PCI_CLASS_REG));
      putstr(",");
      
#ifdef BTPCI_VERBOSE
      putLabeledWord("   mapreg: ", *(volatile dword *)(csrBase+PCI_MAPREG_START));
#endif /* BTPCI_VERBOSE */
      putHexInt32(*(volatile dword *)(csrBase+PCI_MAPREG_START));
      putstr(")");
      if (pcv->handleQuirks != NULL) {
         pcv->handleQuirks(pcv);
      }
   }
}




#define EEPROM_N_ABITS 6
#define EEPROM_N_DBITS 16

#define PCNET_RAP_WIO_REG 0x12
#define PCNET_RDP_WIO_REG 0x10
#define PCNET_BDP_WIO_REG 0x16
#define PCNET_RESET_WIO_REG 0x14

#define PCNET_RAP_DWIO_REG 0x14
#define PCNET_RDP_DWIO_REG 0x10
#define PCNET_BDP_DWIO_REG 0x1C
#define PCNET_RESET_DWIO_REG 0x18

static enum e_pcnet_io_mode {
  PCNET_UNKNOWN_MODE = 0,
  PCNET_WIO_MODE = 1,
  PCNET_DWIO_MODE = 2
} pcnet_io_mode = PCNET_DWIO_MODE;

static void pcnet_determine_io_mode(char *ioaddr)
{
  int csr0;
  int readback;

  return;
  writeInt16(ioaddr, PCNET_RAP_WIO_REG, 0);
  csr0 = readInt16(ioaddr, PCNET_RDP_WIO_REG);
  writeInt16(ioaddr, PCNET_RAP_WIO_REG, 88);
  readback = readInt16(ioaddr, PCNET_RAP_WIO_REG);
  if (csr0 == 4 && readback == 88) {
    pcnet_io_mode = PCNET_WIO_MODE;
    return;
  }
  putLabeledWord("csr0: ", csr0);
  putLabeledWord("readback: ", readback);

  writeInt32(ioaddr, PCNET_RAP_DWIO_REG, 0);
  csr0 = readInt32(ioaddr, PCNET_RDP_DWIO_REG)&0xFFFF;
  writeInt32(ioaddr, PCNET_RAP_DWIO_REG, 88);
  readback = readInt32(ioaddr, PCNET_RAP_DWIO_REG)&0xFFFF;
  if (csr0 == 4 && readback == 88) {
    pcnet_io_mode = PCNET_DWIO_MODE;
    return;
  }

  putLabeledWord("csr0: ", csr0);
  putLabeledWord("readback: ", readback);
  putstr("unknown pcnet io mode\r\n");
}

/* assumes WIO mode, which is the default mode of the PCNet controller after reset */
static void pcnet_write_csr (char *ioaddr, int regnum, int regval)
{
  writeInt32(ioaddr, PCNET_RAP_DWIO_REG, regnum);
  writeInt32(ioaddr, PCNET_RDP_DWIO_REG, regval&0xFFFF);
}

static int pcnet_read_csr (char *ioaddr, int regnum)
{
  writeInt32(ioaddr, PCNET_RAP_DWIO_REG, regnum);
  return readInt32(ioaddr, PCNET_RDP_DWIO_REG)&0xFFFF;
}

/* assumes WIO mode, which is the default mode of the PCNet controller after reset */
static void pcnet_write_bcr (char *ioaddr, int regnum, int regval)
{
  writeInt32(ioaddr, PCNET_RAP_DWIO_REG, regnum);
  writeInt32(ioaddr, PCNET_BDP_DWIO_REG, regval&0xFFFF);
}

static int pcnet_read_bcr (char *ioaddr, int regnum)
{
  writeInt32(ioaddr, PCNET_RAP_DWIO_REG, regnum);
  return readInt32(ioaddr, PCNET_BDP_DWIO_REG)&0xFFFF;
}




#define EEPROM_BCR 19
#define EEPROM_EDI (1<<0)
#define EEPROM_EDO (1<<0)
#define EEPROM_ESK (1<<1)
#define EEPROM_ECS (1<<2)
#define EEPROM_EEN (1<<4)
#define EEPROM_EEDET (1<<13)
#define EEPROM_PREAD (1<<14)
#define EEPROM_PVALID (1<<15)

static int eeprom_clk(char *ioaddr, int regval)
{
  int result;
  regval &= ~EEPROM_ESK;
  pcnet_write_bcr (ioaddr, 19, regval);
  regval |= EEPROM_ESK;
  pcnet_write_bcr (ioaddr, 19, regval);
#if 0
  regval &= ~EEPROM_ESK;
  pcnet_write_bcr (ioaddr, 19, regval);
#endif
  result = pcnet_read_bcr (ioaddr, 19);
  return result;
}

/* enable erase and write operations */
static void ewen_eeprom(char *ioaddr)
{
  int regval = EEPROM_EEN | EEPROM_ECS; /* enable manual eeprom operations */
  int abitnum;

  /* send a few zeros */
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  /* send start bit */
  eeprom_clk(ioaddr, regval | EEPROM_EDI);
  /* send ewen opcode */
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  for (abitnum = 0; abitnum < EEPROM_N_ABITS; abitnum++)
	eeprom_clk(ioaddr, regval | EEPROM_EDI);

  eeprom_clk(ioaddr, EEPROM_EEN & ~EEPROM_ECS);
  pcnet_write_bcr (ioaddr, 19, 0);
}

/* enable disable and write operations */
static void ewds_eeprom(char *ioaddr)
{
  int regval = EEPROM_EEN | EEPROM_ECS; /* enable manual eeprom operations */
  int abitnum;

  /* send a few zeros */
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  /* send start bit */
  eeprom_clk(ioaddr, regval | EEPROM_EDI);
  /* send ewds opcode */
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  for (abitnum = 0; abitnum < EEPROM_N_ABITS; abitnum++)
	eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  eeprom_clk(ioaddr, EEPROM_EEN & ~EEPROM_ECS);
  pcnet_write_bcr (ioaddr, 19, 0);

}

static void erase_eeprom(char *ioaddr, int word_number)
{
  int regval = EEPROM_EEN; /* enable manual eeprom operations */
  int abitnum;

  /* set CS to one */
  regval |= EEPROM_ECS;

  /* send a few zeros */
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  /* send start bit */
  eeprom_clk(ioaddr, regval | EEPROM_EDI);
  /* send 'erase' opcode */
  eeprom_clk(ioaddr, regval | EEPROM_EDI);
  eeprom_clk(ioaddr, regval | EEPROM_EDI);

  for (abitnum = EEPROM_N_ABITS-1; abitnum >= 0; abitnum--) {
	/* set DI to address bit, starting with MSB */
	int edi = ((word_number >> abitnum)&1) ? EEPROM_EDI : 0;
	/* clock */
	eeprom_clk(ioaddr, regval | edi);
  }

  /* set CS TO zero */
  eeprom_clk(ioaddr, regval & ~EEPROM_ECS);
  eeprom_clk(ioaddr, regval & ~EEPROM_ECS);
  eeprom_clk(ioaddr, regval & ~EEPROM_ECS);

  /* now poll for #BSY */
  for (abitnum = 0; abitnum < 10; abitnum++) {
	int edo = eeprom_clk(ioaddr, regval) & EEPROM_EDO;
	if (!edo) {
	  break;
	}
  }
  /* now poll for ready */
  for (abitnum = 0; abitnum < 1000; abitnum++) {
	int edo = eeprom_clk(ioaddr, regval) & EEPROM_EDO;
	if (edo) {
	  if (1) {
             putstr("*** eeprom address ");
             putHexInt8(word_number);
             putstr(" erased ***\r\n");
          }
	  break;
	}
  }

  /* set CS TO zero again */
  eeprom_clk(ioaddr, regval & ~EEPROM_ECS);

  /* disable EEPROM mode */
  pcnet_write_bcr (ioaddr, EEPROM_BCR, 0);
}

static void write_eeprom(char *ioaddr, int word_number, int value)
{
  int regval = EEPROM_EEN | EEPROM_ECS; /* enable manual eeprom operations */
  int abitnum, dbitnum;

  /* send a few zeros */
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  /* send start bit */
  eeprom_clk(ioaddr, regval | EEPROM_EDI);
  /* send 'write' opcode */
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  eeprom_clk(ioaddr, regval | EEPROM_EDI);

  for (abitnum = EEPROM_N_ABITS-1; abitnum >= 0; abitnum--) {
	/* set DI to address bit, starting with MSB */
	int edi = ((word_number >> abitnum)&1) ? EEPROM_EDI : 0;
	/* clock */
	eeprom_clk(ioaddr, regval | edi);
  }

  /* now send data bits */
  for (dbitnum = EEPROM_N_DBITS-1; dbitnum >= 0; dbitnum--) {
	int edi = ((value >> dbitnum)&1) ? EEPROM_EDI : 0;
	eeprom_clk(ioaddr, regval | edi);
  }

  /* set CS TO zero */
  eeprom_clk(ioaddr, regval & ~EEPROM_ECS);
  eeprom_clk(ioaddr, regval & ~EEPROM_ECS);
  eeprom_clk(ioaddr, regval & ~EEPROM_ECS);

  /* now poll for #BSY */
  for (abitnum = 0; abitnum < 10; abitnum++) {
	int edo = eeprom_clk(ioaddr, regval) & EEPROM_EDO;
	if (!edo) {
	  break;
	}
  }
  /* now poll for ready */
  for (abitnum = 0; abitnum < 1000; abitnum++) {
	int edo = eeprom_clk(ioaddr, regval) & EEPROM_EDO;
	if (edo) {
	  if (1) {
             putstr("*** eeprom address ");
             putHexInt8(word_number);
             putstr(" written ***\r\n");
          }
	  break;
	}
  }

  /* set CS TO zero again */
  eeprom_clk(ioaddr, regval & ~EEPROM_ECS);

  /* disable EEPROM mode */
  pcnet_write_bcr (ioaddr, EEPROM_BCR, 0);
}

static int read_eeprom(char *ioaddr, int word_number)
{
  int regval = EEPROM_EEN; /* enable manual eeprom operations */
  int abitnum, dbitnum;
  int data = 0;

  /* set CS to one */
  regval |= EEPROM_ECS;

  /* send a few zeros */
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
  /* send start bit */
  eeprom_clk(ioaddr, regval | EEPROM_EDI);
  /* send read opcode */
  eeprom_clk(ioaddr, regval | EEPROM_EDI);
  eeprom_clk(ioaddr, regval & ~EEPROM_EDI);

  for (abitnum = EEPROM_N_ABITS-1; abitnum >= 0; abitnum--) {
	/* set DI to address bit, starting with MSB */
	int edi = ((word_number >> abitnum)&1) ? EEPROM_EDI : 0;
	/* clock */
	eeprom_clk(ioaddr, regval | edi);
  }

  for (dbitnum = EEPROM_N_DBITS-1; dbitnum >= 0; dbitnum--) {
	int edo;
	/* clock */
	regval = eeprom_clk(ioaddr, regval & ~EEPROM_EDI);
	/* read DI, starting with msb */
	edo = (regval & EEPROM_EDO) ? 1 : 0;
	data |= (edo << dbitnum);
  }

  /* set CS TO zero */
  eeprom_clk(ioaddr, regval & ~EEPROM_ECS);

  /* disable EEPROM mode */
  pcnet_write_bcr (ioaddr, EEPROM_BCR, 0);
  return data;
}


#define EEPROM_MAC0_OFFSET		0
#define EEPROM_MAC1_OFFSET		1
#define EEPROM_MAC2_OFFSET		2
#define EEPROM_CSR116_OFFSET	3
#define EEPROM_HWID_OFFSET		4
#define EEPROM_RESVD_OFFSET		5
#define EEPROM_CKSUM1_OFFSET    6
#define EEPROM_WW_OFFSET		7
#define EEPROM_BCR2_OFFSET		8
#define EEPROM_BCR4_OFFSET		9
#define EEPROM_BCR5_OFFSET		10
#define EEPROM_BCR6_OFFSET		11
#define EEPROM_BCR7_OFFSET		12
#define EEPROM_BCR9_OFFSET		13
#define EEPROM_BCR18_OFFSET		14
#define EEPROM_BCR22_OFFSET		15
#define EEPROM_BCR23_OFFSET		16
#define EEPROM_BCR24_OFFSET		17
#define EEPROM_BCR25_OFFSET		18
#define EEPROM_BCR26_OFFSET		19
#define EEPROM_BCR27_OFFSET		20
#define EEPROM_BCR32_OFFSET		21
#define EEPROM_BCR33_OFFSET		22
#define EEPROM_BCR35_OFFSET		23
#define EEPROM_BCR36_OFFSET		24
#define EEPROM_BCR37_OFFSET		25
#define EEPROM_BCR38_OFFSET		26
#define EEPROM_BCR39_OFFSET		27
#define EEPROM_BCR40_OFFSET		28
#define EEPROM_BCR41_OFFSET		29
#define EEPROM_BCR42_OFFSET		30
#define EEPROM_BCR43_OFFSET		31
#define EEPROM_BCR44_OFFSET		32
#define EEPROM_BCR48_OFFSET		33
#define EEPROM_BCR49_OFFSET		34
#define EEPROM_BCR50_OFFSET		35
#define EEPROM_BCR51_OFFSET		36
#define EEPROM_BCR52_OFFSET		37
#define EEPROM_BCR53_OFFSET		38
#define EEPROM_BCR54_OFFSET		39
#define EEPROM_CKSUM2_OFFSET	40

/*
 * Reprograms the ethernet attached to the PCNet ethernet controller
 * Assumes that there is only one of these on the PCI bus.
 */ 
void program_all_eeprom()
{
   int i;
   long maclsbyte;
   short eeprom_data[82];
   unsigned char *bytes = (unsigned char *)eeprom_data;
   char *configBase = (char *)makePCIConfigurationAddress(PCNET_DEVICE_NUMBER, 0, 0);
   char *ioaddr = (char *)0x7c000000 + (readInt16(configBase, PCI_MAPREG_START)&0xFFFE);

   memset(eeprom_data, 0, sizeof(eeprom_data));


   /* uses MAC addresses starting with 08-00-2B-00-01-00 */
   /* get per-unit part of the MAC address */
   maclsbyte = param_maclsbyte.value;
   

   /* fill in the data */
   eeprom_data[EEPROM_MAC0_OFFSET] = 0x0008; /* byte swapped */
   eeprom_data[EEPROM_MAC1_OFFSET] = 0x002b; /* byte swapped */
   eeprom_data[EEPROM_MAC2_OFFSET] = 0x0001 | ((maclsbyte&0xFF)<<8); /* byte swapped */
   eeprom_data[EEPROM_HWID_OFFSET] = 0x11;
   eeprom_data[EEPROM_WW_OFFSET] = 0x5757;
   eeprom_data[EEPROM_BCR2_OFFSET] = 0x3100;
   eeprom_data[EEPROM_BCR4_OFFSET] = 0x00c0;
   eeprom_data[EEPROM_BCR5_OFFSET] = 0x0084;
   eeprom_data[EEPROM_BCR6_OFFSET] = 0x0088;
   eeprom_data[EEPROM_BCR7_OFFSET] = 0x0090;
   eeprom_data[EEPROM_BCR18_OFFSET] = 0x9001; /* WIO mode */
   eeprom_data[EEPROM_BCR22_OFFSET] = 0xFF06;
   eeprom_data[EEPROM_BCR23_OFFSET] = 0xaa22;
   eeprom_data[EEPROM_BCR24_OFFSET] = 0x1717;
   eeprom_data[EEPROM_BCR35_OFFSET] = 0x1022;
   eeprom_data[EEPROM_BCR36_OFFSET] = 0xc811;
   eeprom_data[EEPROM_BCR48_OFFSET] = 0x00a0;
   eeprom_data[EEPROM_BCR49_OFFSET] = 0x0080;
   
   /* compute the MAC checksum */
   {
      int mac_checksum = 0;
      int i;
      for (i = 0; i < 16; i++)
         mac_checksum += bytes[i];
      /* eeprom_data[EEPROM_CKSUM1_OFFSET] needs to match this checksum */
      eeprom_data[EEPROM_CKSUM1_OFFSET] = mac_checksum;
   }
   /* compute the eeprom checksum */
   {
      int eeprom_checksum = 0;
      int i;
      for (i = 0; i < 82; i++)
         eeprom_checksum += bytes[i];
      /* (eeprom_data[EEPROM_CKSUM2_OFFSET] + eeprom_cksum) & 0xFF should be 0xFF */
      eeprom_data[EEPROM_CKSUM2_OFFSET] = (0xFF - eeprom_checksum)&0xFF;
   }
   /* now program the eeprom */
   {
      int i;
      ewen_eeprom(ioaddr);
      for (i = 0; i < 41; i++) {
         short oldvalue = read_eeprom(ioaddr, i);
         short newvalue = eeprom_data[i];
         if (oldvalue != newvalue) {
            putstr("reprogramming eeprom["); putHexInt8(i); putstr("] from "); putHexInt16(oldvalue);
            putstr(" to "); putHexInt16(newvalue); putstr("\r\n");
            erase_eeprom(ioaddr, i);
            write_eeprom(ioaddr, i, newvalue);
         }
      }
      ewds_eeprom(ioaddr);
   }
}



void handlePCNetQuirks(struct PciConfigurationValues *pciConfigurationValue)
{
  char *configBase = makePCIConfigurationAddress(pciConfigurationValue->pciDevice, 
						 pciConfigurationValue->pciFunction,
						 0);
  char *ioaddr = (char*)0x7c000000 + (readInt16(configBase, PCI_MAPREG_START)&0xFFFE);
  int valid_eeprom;
  int maclsbyte;
  struct bootblk_param *maclsbyte_param;
   
  /* one more reset to be sure */
  readInt16(ioaddr, PCNET_RESET_WIO_REG);
  readInt32(ioaddr, PCNET_RESET_DWIO_REG);
  /* put into DWIO mode */
  writeInt32(ioaddr, PCNET_RDP_DWIO_REG, 0x0);
  readInt32(ioaddr, PCNET_RDP_DWIO_REG);

  pcnet_io_mode = PCNET_DWIO_MODE;

  if (0) {
    putstr("\r\n\r\n");
    putLabeledWord("CONFIG: ", (int)configBase);
    putLabeledWord("IOADDR: ", (int)ioaddr);
    putLabeledWord("APROM0: ", readInt8(ioaddr, 0));
    putLabeledWord("APROM1: ", readInt8(ioaddr, 1));
    putLabeledWord("APROM2: ", readInt8(ioaddr, 2));
    putLabeledWord("APROM3: ", readInt8(ioaddr, 3));
    putLabeledWord("APROM4: ", readInt8(ioaddr, 4));
    putLabeledWord("APROM5: ", readInt8(ioaddr, 5));
    putLabeledWord("RAP16:  ", readInt16(ioaddr, PCNET_RAP_WIO_REG));
    putLabeledWord("RDP16:  ", readInt16(ioaddr, PCNET_RDP_WIO_REG));
    putLabeledWord("RAP32:  ", readInt32(ioaddr, PCNET_RAP_DWIO_REG));
    putLabeledWord("RDP32:  ", readInt32(ioaddr, PCNET_RDP_DWIO_REG));
    putLabeledWord("CSR0:  ", pcnet_read_csr(ioaddr, 0));
    putLabeledWord("BCR19: ", pcnet_read_bcr(ioaddr, 19));

  }
  valid_eeprom = pcnet_read_bcr(ioaddr, 19) & (1<<15); /* BCR19: page 161 of Am79C978 manual */
  maclsbyte = readInt8(ioaddr, 5) & 0xFF;

  putLabeledWord("maclsbyte", maclsbyte);

}

int get_maclsbyte(void)
{
  char *configBase = makePCIConfigurationAddress(PCNET_DEVICE_NUMBER, 0, 0);
  char *ioaddr = (char*)0x7c000000 + (readInt16(configBase, PCI_MAPREG_START)&0xFFFE);
  int maclsbyte;
  maclsbyte = readInt8(ioaddr, 5) & 0xFF;
  return maclsbyte;
}
