/* usb.h
 * USB support header file for SA-1110
 * handhelds.org open bootldr
 *
 * code that is unclaimed by others is
 *    Copyright (C) 2003 Joshua Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* Defines taken from SA-1100.h v1.2
 *	Author  	Copyright (c) Marc A. Viredaz, 1998
 *	        	DEC Western Research Laboratory, Palo Alto, CA
 */
 
/* Much code from the Linux kernel's arch/arm/mach-sa1100/usb* files.
 * Copyright (C) Compaq Computer Corporation, 1998, 1999
 * Copyright (C) Extenex Corporation, 2001
 * Copyright (c) 2001 by Nicolas Pitre
 */
 
#ifndef CONFIG_ACCEPT_GPL
#  error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#define VIO_BASE        0xf8000000      /* virtual start of IO space */
#define VIO_SHIFT       3               /* x = IO space shrink power */
#define PIO_START       0x80000000      /* physical start of IO space */

#ifndef io_p2v
#  define io_p2v( x )             \
   ( (((x)&0x00ffffff) | (((x)&0x30000000)>>VIO_SHIFT)) + VIO_BASE )
#endif

#define __REG(x)       (*((volatile u32 *)x))

#define ICPR            __REG(0x90050020)  /* IC Pending Reg.                 */
#define Ser0UDCCR        __REG(0x80000000)
#define Ser0UDCCS0       __REG(0x80000010)
#define Ser0UDCCS1       __REG(0x80000014)
#define Ser0UDCCS2       __REG(0x80000018)
#define Ser0UDCSR        __REG(0x80000030)
#define Ser0UDCAR        __REG(0x80000004)  /* Ser. port 0 UDC Address Reg. */
#define Ser0UDCOMP       __REG(0x80000008)  /* Ser. port 0 UDC Output Maximum Packet size reg. */
#define Ser0UDCIMP       __REG(0x8000000C)  /* Ser. port 0 UDC Input Maximum Packet size reg. */
#define Ser0UDCD0       __REG(0x8000001C)  /* Ser. port 0 UDC Data reg. end-point 0 */
#define Ser0UDCWC       __REG(0x80000020)  /* Ser. port 0 UDC Write Count reg. end-point 0 */
#define Ser0UDCDR       __REG(0x80000028)  /* Ser. port 0 UDC Data Reg. */
#define UDCCR_UDD        0x00000001
#define UDCCR_RESIM      0x00000004
#define UDCCR_SUSIM      0x00000040
#define UDCCR_ER29	 0x00000080
#define UDCCS0_OPR      0x00000001      /* Output Packet Ready (read)      */
#define UDCCS0_IPR      0x00000002      /* Input Packet Ready              */
#define UDCCS0_SST      0x00000004      /* Sent STall                      */
#define UDCCS0_FST      0x00000008      /* Force STall                     */
#define UDCCS0_DE       0x00000010      /* Data End                        */
#define UDCCS0_SE       0x00000020      /* Setup End (read)                */
#define UDCCS0_SO       0x00000040      /* Serviced Output packet ready    */
                                        /* (write)                         */
#define UDCCS0_SSE      0x00000080      /* Serviced Setup End (write)      */
#define UDCCS1_SST      0x00000008      /* Sent STall                      */
#define UDCCS1_FST       0x00000010
#define UDCCS2_FST	 0x00000020
#define UDCCS2_TFS      0x00000001      /* Transmit FIFO 8-bytes or less   */
#define UDCCS1_RPE       0x00000004
#define UDCCS2_TPE       0x00000004
#define UDCCS2_TUR      0x00000008      /* Transmit FIFO Under-Run         */
#define UDCCS2_SST      0x00000010      /* Sent STall                      */
#define UDCCS1_RPC       0x00000002
#define UDCCS2_TPC       0x00000002
#define UDCCS1_RNE      0x00000020      /* Receive FIFO Not Empty (read)   */
#define UDCCS1_RFS	0x00000001	/* >12 bytes in rfifo */
#define UDCSR_EIR        0x00000001      /* End-point 0 Interrupt Request   */
#define UDCSR_RIR        0x00000002      /* Receive Interrupt Request       */
#define UDCSR_TIR        0x00000004      /* Transmit Interrupt Request      */
#define UDCSR_SUSIR      0x00000008      /* SUSpend Interrupt Request       */
#define UDCSR_RESIR      0x00000010      /* RESume Interrupt Request        */
#define UDCSR_RSTIR      0x00000020      /* ReSeT Interrupt Request         */
#define UDCCR_REM        0x00000080

#define DescriptorHeader \
	u8 bLength;        \
	u8 bDescriptorType
	
// --- Device Descriptor -------------------

typedef struct {
	 DescriptorHeader;
	 u16 bcdUSB;		   	/* USB specification revision number in BCD */
	 u8  bDeviceClass;	/* USB class for entire device */
	 u8  bDeviceSubClass; /* USB subclass information for entire device */
	 u8  bDeviceProtocol; /* USB protocol information for entire device */
	 u8  bMaxPacketSize0; /* Max packet size for endpoint zero */
	 u16 idVendor;        /* USB vendor ID */
	 u16 idProduct;       /* USB product ID */
	 u16 bcdDevice;       /* vendor assigned device release number */
	 u8  iManufacturer;	/* index of manufacturer string */
	 u8  iProduct;        /* index of string that describes product */
	 u8  iSerialNumber;	/* index of string containing device serial number */
	 u8  bNumConfigurations; /* number fo configurations */
} __attribute__ ((packed)) device_desc_t;

// --- Configuration Descriptor ------------

typedef struct {
	 DescriptorHeader;
	 u16 wTotalLength;	    /* total # of bytes returned in the cfg buf 4 this cfg */
	 u8  bNumInterfaces;      /* number of interfaces in this cfg */
	 u8  bConfigurationValue; /* used to uniquely ID this cfg */
	 u8  iConfiguration;      /* index of string describing configuration */
	 u8  bmAttributes;        /* bitmap of attributes for ths cfg */
	 u8  MaxPower;		    /* power draw in 2ma units */
} __attribute__ ((packed)) config_desc_t;

// bmAttributes:
enum { USB_CONFIG_REMOTEWAKE=0x20, USB_CONFIG_SELFPOWERED=0x40,
	   USB_CONFIG_BUSPOWERED=0x80 };

// MaxPower:
#define USB_POWER( x)  ((x)>>1) /* convert mA to descriptor units of A for MaxPower */

// --- Interface Descriptor ---------------

typedef struct {
	 DescriptorHeader;
	 u8  bInterfaceNumber;   /* Index uniquely identfying this interface */
	 u8  bAlternateSetting;  /* ids an alternate setting for this interface */
	 u8  bNumEndpoints;      /* number of endpoints in this interface */
	 u8  bInterfaceClass;    /* USB class info applying to this interface */
	 u8  bInterfaceSubClass; /* USB subclass info applying to this interface */
	 u8  bInterfaceProtocol; /* USB protocol info applying to this interface */
	 u8  iInterface;         /* index of string describing interface */
} __attribute__ ((packed)) intf_desc_t;

// --- Endpoint  Descriptor ---------------

typedef struct {
	 DescriptorHeader;
	 u8  bEndpointAddress;  /* 0..3 ep num, bit 7: 0 = 0ut 1= in */
	 u8  bmAttributes;      /* 0..1 = 0: ctrl, 1: isoc, 2: bulk 3: intr */
	 u16 wMaxPacketSize;    /* data payload size for this ep in this cfg */
	 u8  bInterval;         /* polling interval for this ep in this cfg */
} __attribute__ ((packed)) ep_desc_t;

// bEndpointAddress:
enum { USB_OUT= 0, USB_IN=1 };
#define USB_EP_ADDRESS(a,d) (((a)&0xf) | ((d) << 7))
// bmAttributes:
enum { USB_EP_CNTRL=0, USB_EP_BULK=2, USB_EP_INT=3 };

// --- String Descriptor -------------------

typedef struct {
	 DescriptorHeader;
	 u8 bString[32];		  /* unicode string .. actaully 'n' __u16s. ... 32 max */
} __attribute__ ((packed)) string_desc_t;

typedef struct {
	 DescriptorHeader;
	 u16 bString[1];		  /* unicode string .. actaully 'n' __u16s. ... 32 max */
} __attribute__ ((packed)) string_desc_t_;

/*=======================================================
 * Handy helpers when working with above
 *
 */
// these are x86-style 16 bit "words" ...
#define make_word_c( w ) __constant_cpu_to_le16(w)
#define make_word( w )   __cpu_to_le16(w)

// descriptor types
enum { USB_DESC_DEVICE=1, USB_DESC_CONFIG=2, USB_DESC_STRING=3,
	   USB_DESC_INTERFACE=4, USB_DESC_ENDPOINT=5 };


/*=======================================================
 * Default descriptor layout for SA-1100 and SA-1110 UDC
 */

/* "config descriptor buffer" - that is, one config,
   ..one interface and 2 endpoints */
struct cdb {
	 config_desc_t cfg;
	 intf_desc_t   intf;
	 ep_desc_t     ep1;
	 ep_desc_t     ep2;
} __attribute__ ((packed));


/* all SA device descriptors */
typedef struct {
	 device_desc_t dev;   /* device descriptor */
	 struct cdb b;        /* bundle of descriptors for this cfg */
} __attribute__ ((packed)) desc_t;


enum {  kStateZombie  = 0,  kStateZombieSuspend  = 1,
                kStateDefault = 2,  kStateDefaultSuspend = 3,
                kStateAddr    = 4,  kStateAddrSuspend    = 5,
                kStateConfig  = 6,  kStateConfigSuspend  = 7
};

enum { USB_STATE_NOTATTACHED=0, USB_STATE_ATTACHED=1,USB_STATE_POWERED=2,
           USB_STATE_DEFAULT=3, USB_STATE_ADDRESS=4, USB_STATE_CONFIGURED=5,
                      USB_STATE_SUSPENDED=6};
                      
enum { kError=-1, kEvSuspend=0, kEvReset=1,
           kEvResume=2, kEvAddress=3, kEvConfig=4, kEvDeConfig=5 };

static int device_state_machine[8][6] = {
//                suspend               reset		resume     	adddr 	config 		deconfig
/* zombie */  {  kStateZombieSuspend,  	kStateDefault, 	kError,    	kError, kError, 	kError },
/* zom sus */ {  kError, 		kStateDefault, 	kStateZombie, 	kError, kError, 	kError },
/* default */ {  kStateDefaultSuspend, 	kStateDefault, 	kStateDefault, 	kStateAddr, kError, 	kError },
/* def sus */ {  kError, 		kStateDefault, 	kStateDefault, 	kError, kError, 	kError },
/* addr */    {  kStateAddrSuspend, 	kStateDefault, 	kError, 	kError, kStateConfig, 	kError },
/* addr sus */{  kError, 		kStateDefault, 	kStateAddr, 	kError, kError, 	kError },
/* config */  {  kStateConfigSuspend, 	kStateDefault, 	kError, 	kError, kError, 	kStateAddr },
/* cfg sus */ {  kError, 		kStateDefault, 	kStateConfig, 	kError, kError, 	kError }
};

static int sm_state_to_device_state[8] =
//  zombie           zom suspend          default            default sus
{ USB_STATE_POWERED, USB_STATE_SUSPENDED, USB_STATE_DEFAULT, USB_STATE_SUSPENDED,
// addr              addr sus             config                config sus
  USB_STATE_ADDRESS, USB_STATE_SUSPENDED, USB_STATE_CONFIGURED, USB_STATE_SUSPENDED
  };