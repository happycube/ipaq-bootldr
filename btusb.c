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
 * btusb.c - Test file for USB
 *
 */

#include "bootconfig.h"
#include "bootldr.h"
#include "ohci.h"

static HcEndpointDescriptor hced0;
static HcTransferDescriptor hctd0;
static HcTransferDescriptor hctd1;

static HCCA hcca;
static char hctdata0[8];

void testUSB(char *args)
{
   int i;
   volatile dword *usb_config = (dword *)0x7bc00800;
   volatile dword *usb_mem = (dword *)0x00000000;

   memset((char*)&hcca, 0, sizeof(hcca));
   memset((char*)&hced0, 0, sizeof(hced0));
   memset((char*)&hctd0, 0, sizeof(hctd0));
   memset((char*)&hctd1, 0, sizeof(hctd1));
   hced0.control = 0 & 0x7f; /* FA: function address */
   hced0.control |= ((0 & 0xF) << 7); /* EN: endpoint number */
   hced0.control |= ((1 & 0x3) << 11); /* D: direction  -- 1 == OUT */
   hced0.control |= ((0 & 0x1) << 13); /* S: speed -- 0 == full-speed */
   hced0.control |= ((0 & 0x1) << 14); /* K: skip -- 0 == no skip */
   hced0.control |= ((0 & 0x1) << 15); /* F: format -- 0 == general */
   hced0.control |= ((8 & 0x7F) << 16); /* MPS: maximum packet size -- 8 */
   hced0.tailp = (dword) &hctd1;
   hced0.headp = (dword) &hctd0;
   hced0.nextED = 0;

   for (i = 0; i < 8; i++)
      hctdata0[i] = 0x5a;

   hctd0.control = 0;
   hctd0.control |= ((1 & 0x1) << 18); /* DR: data rounding */
   hctd0.control |= ((00 & 0x3) << 19); /* DP: direction/pid -- 00==setup */
   hctd0.control |= ((00 & 0x7) << 21); /* DI: DelayInterrupt */
   hctd0.control |= ((00 & 0x3) << 24); /* T: DataToggle */
   hctd0.control |= ((00 & 0x3) << 26); /* EC: ErrorCount */
   hctd0.control |= ((00 & 0xF) << 28); /* CC: Condition Code */
   hctd0.cbp = &hctdata0[0];
   hctd0.nextTD = (dword) &hctd1;
   hctd0.BE = &hctdata0[7];


   /* make sure HC can be PCI master */

   putLabeledWord("21285 SDRAM BAR: ", *(dword *)0x42000018);
   usb_mem[0x18 >> 2] = (dword)&hcca; /* tell OPTi where the hcca structure is in memory */
   usb_mem[0x20] = (dword)&hced0; /* control endpoint descriptor list */
   usb_mem[0x4 >> 2] = 
      ((2 << 6) /* USB operational */
       | (1 << 4) /* control endpoint enabled */
       );
   usb_mem[0x8 >> 2] = 
      (1 << 2); /* enable the control endpoint list */
   putLabeledWord("USB StatusCommand: ", usb_config[4 >> 2]);
   *(((byte*)usb_config)+0x4e) = 7; /* turn on test0 and test1 outputs */
   putLabeledWord("USB I2C: ", usb_config[0x4C >> 2]);
   usb_config[0x50 >> 2] &= 0xFF0FFFFFl;
   putLabeledWord("USB FeatureControl: ", usb_config[0x50 >> 2]);

#if 0
   putLabeledWord("USB MemOfst4: ", usb_mem[0x4 >> 2]);
   putLabeledWord("USB MemOfst8: ", usb_mem[0x8 >> 2]);
#endif
   putLabeledWord("USB MemofstC: ", usb_mem[0xC >> 2]);
   putLabeledWord("USB Memofst10: ", usb_mem[0x10 >> 2]);
   putLabeledWord("USB Memofst14: ", usb_mem[0x14 >> 2]);
   putLabeledWord("USB Memofst1C: ", usb_mem[0x1C >> 2]);
   usb_mem[0x48 >> 2] |= (2 << 8); /* turn on power switches */
   putLabeledWord("USB Memofst48: ", usb_mem[0x48 >> 2]);
   usb_mem[0x4C >> 2] = 0xFFFFFFFFl;
   

   usb_mem[0x50 >> 2] = (1 << 16) | (0x80 << 8); /* set global power on */
   putLabeledWord("USB Memofst50: ", usb_mem[0x50 >> 2]);
   putLabeledWord("USB Memofst54: ", usb_mem[0x54 >> 2]);
   putLabeledWord("USB Memofst58: ", usb_mem[0x58 >> 2]);

   {
      dword hcFrameNumber = usb_mem[0x3c >> 2];
      dword hccaFrameNumber = hcca.frameNumber;
      dword hcStatusCommand = usb_config[4 >> 2];
      dword hcControl = usb_mem[4 >> 2];
      dword hcStatus = usb_mem[0xC >> 2];
      dword hcMem50 = usb_mem[0x50 >> 2];
      dword hcMem54 = usb_mem[0x54 >> 2];
      dword hcMem58 = usb_mem[0x58 >> 2];
      dword hcMem20 = usb_mem[0x20 >> 2];
      dword hcMem24 = usb_mem[0x24 >> 2];
      int c;
      for (c = 0; c < 1000000; c++) {
         dword newHcFrameNumber = usb_mem[0x3c >> 2];
         dword newHccaFrameNumber = hcca.frameNumber;
         dword newHcStatusCommand = usb_config[4 >> 2];
         dword newHcControl = usb_mem[4 >> 2];
         dword newHcStatus = usb_mem[0xC >> 2];
         dword newHcMem50 = usb_mem[0x50 >> 2];
         dword newHcMem54 = usb_mem[0x54 >> 2];
         dword newHcMem58 = usb_mem[0x58 >> 2];
         dword newHcMem20 = usb_mem[0x20 >> 2];
         dword newHcMem24 = usb_mem[0x24 >> 2];
         if (hcFrameNumber != newHcFrameNumber
             || hccaFrameNumber != newHccaFrameNumber
             || hcStatusCommand != newHcStatusCommand
             || hcControl != newHcControl
             || hcStatus != newHcStatus
             || hcMem50 != newHcMem50
             || hcMem54 != newHcMem54
             || hcMem58 != newHcMem58
             || hcMem20 != newHcMem20
             || hcMem24 != newHcMem24
             ) {
            putstr("\r\n");
            putLabeledWord("wait count: ", c);
            putLabeledWord("HcFrameNumber: ", newHcFrameNumber);
            putLabeledWord("HcFrameRemaining: ", usb_mem[0x38 >> 2]);
            putLabeledWord("HccaFrameNumber: ", newHccaFrameNumber);
            putLabeledWord("HcStatusCommand: ", newHcStatusCommand);
            putLabeledWord("HcMem50: ", newHcMem50);
            putLabeledWord("HcMem54: ", newHcMem54);
            putLabeledWord("HcMem58: ", newHcMem58);
            putLabeledWord("HcMem20: ", newHcMem20);
            putLabeledWord("HcMem24: ", newHcMem24);

            hcFrameNumber = newHcFrameNumber;
            hccaFrameNumber = newHccaFrameNumber;
            hcStatusCommand = newHcStatusCommand;
            hcControl = newHcControl;
            hcStatus = newHcStatus;
            hcMem50 = newHcMem50;
            hcMem54 = newHcMem54;
            hcMem58 = newHcMem58;
            hcMem20 = newHcMem20;
            hcMem24 = newHcMem24;
         }
      }
   }
}
