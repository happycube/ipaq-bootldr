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

/*  21285reg.s
 *  memory and register map for the 21285
 * 
 */

/* DC21285 Addresses */

/* ; these 4 are actually ranges of addresses -- the SDRAM mode register is updated
 * ; with the low address bits.
 */
#define DC21285_DRAM_A0MR               0x40000000
#define DC21285_DRAM_A1MR               0x40004000
#define DC21285_DRAM_A2MR               0x40008000
#define DC21285_DRAM_A3MR               0x4000C000
	
#define DC21285_XBUS_XCS0               0x40010000
#define DC21285_XBUS_XCS1               0x40011000
#define DC21285_XBUS_XCS2               0x40012000
#define DC21285_XBUS_NOCS               0x40014000

#define DC21285_FLASH                   0x41000000

#define DC21285_ARMCSR_BASE             0x42000000

#define DC21285_SA_CACHE_FLUSH          0x50000000

#define DC21285_OUTBOUND_WRITE_FLUSH    0x78000000

#define DC21285_PCI_IACK                0x79000000
	
#define DC21285_PCI_TYPE_1_CONFIG       0x7A000000

#define DC21285_PCI_TYPE_0_CONFIG       0x7B000000

#define DC21285_PCI_IO                  0x7C000000

#define DC21285_PCI_MEMORY              0x80000000

/* See 21285 Page 7-31 of data sheet: */
#define DC21285_Trp1                    0x0
#define DC21285_Trp2                    0x1
#define DC21285_Trp3                    0x2
#define DC21285_Trp4                    0x3
#define DC21285_Tdal2                   (0x0 << 2)
#define DC21285_Tdal3                   (0x1 << 2)
#define DC21285_Tdal4                   (0x2 << 2)
#define DC21285_Tdal5                   (0x3 << 2)
#define DC21285_Trcd2                   (0x2 << 4)
#define DC21285_Trcd3                   (0x3 << 4)
#define DC21285_Tcas2                   (0x2 << 6)
#define DC21285_Tcas3                   (0x3 << 6)
#define DC21285_Trc4                    (0x1 << 8)
#define DC21285_Trc5                    (0x2 << 8)
#define DC21285_Trc6                    (0x3 << 8)
#define DC21285_Trc7                    (0x4 << 8)
#define DC21285_Trc8                    (0x5 << 8)
#define DC21285_Trc9                    (0x6 << 8)
#define DC21285_Trc10                   (0x7 << 8)
#define DC21285_Tcd0                    (0x0 << 11)
#define DC21285_Tcd1                    (0x1 << 11)
#define DC21285_Parity0                 (0x0 << 12)
#define DC21285_Parity1                 (0x1 << 12)
	
/*   21285 register memory map -- offsets relative to CSR_BASE
 */
#define CSR_BASE                        DC21285_ARMCSR_BASE

#define PCI_COMMAND                     0x04
#define PCI_STATUS                      0x06
#define PCI_REVISION_ID                 0x08
#define PCI_CLASSCODE                   0x0A
#define PCI_CACHE_LINE_SIZE             0x0C
#define PCI_LATENCY_TIMER               0x0D
#define PCI_CSR_MEM_BAR                 0x10
#define PCI_IO_BAR                      0x14
#define PCI_DRAM_BAR                    0x18
#define PCI_VENDOR_ID                   0x2C
#define PCI_OUT_INT_STATUS              0x30
#define PCI_OUT_INT_MASK                0x34
#define PCI_INT_LINE                    0x3C
#define MAILBOX_0                       0x50
#define MAILBOX_1                       0x54
#define MAILBOX_2                       0x58
#define MAILBOX_3                       0x5C
#define DOORBELL                        0x60
#define DOORBELL_SETUP                  0x64
#define ROM_BYTE_WRITE                  0x68
#define CHAN_1_BYTE_COUNT               0x80
#define CHAN_1_PCI_ADDR                 0x84
#define CHAN_1_DRAM_ADDR                0x88
#define CHAN_1_DESC_PTR                 0x8C
#define CHAN_1_CONTROL                  0x90
#define CHAN_2_BYTE_COUNT               0xA0
#define CHAN_2_PCI_ADDR                 0xA4
#define CHAN_2_DRAM_ADDR                0xA8
#define CHAN_2_DESC_PTR                 0xAC
#define CHAN_2_CONTROL                  0xB0
#define DRAM_BASE_ADDR_MASK             0x100
#define DRAM_BASE_ADDR_OFF              0x104
#define ROM_BASE_ADDR_MASK              0x108
#define SDRAM_TIMING                    0x10C
#define SDRAM_ADDR_SIZE_0               0x110
#define SDRAM_ADDR_SIZE_1               0x114
#define SDRAM_ADDR_SIZE_2               0x118
#define SDRAM_ADDR_SIZE_3               0x11C
#define I2O_IFH                         0x120
#define I2O_IPT                         0x124
#define I2O_OPH                         0x128
#define I2O_OFT                         0x12C
#define I2O_IFC                         0x130
#define I2O_OPC                         0x134
#define I2O_IPC                         0x138
#define SA_CONTROL                      0x13C
#define PCI_ADDR_EXT                    0x140
#define PREFETCH_RANGE                  0x144
#define XBUS_CYCLE                      0x148
#define XBUS_STROBE                     0x14C
#define DBELL_PCI_MASK                  0x150
#define DBELL_SA_MASK                   0x154
#define UARTDR_REG                      0x160
#define UMSEOI_REG                      0x164
#define RXSTAT_REG                      0x164 
#define H_UBRLCR_REG                    0x168 
#define M_UBRLCR_REG                    0x16C 
#define L_UBRLCR_REG                    0x170 
#define UARTCON_REG                     0x174 
#define UARTFLG_REG                     0x178 
#define IRQ_STATUS_REG                  0x180 
#define IRQ_RAW_STATUS_REG              0x184 
#define IRQ_ENABLE_REG                  0x188 
#define IRQ_ENABLE_SET_REG              0x188 
#define IRQ_ENABLE_CLEAR_REG            0x18C 
#define IRQ_SOFT_REG                    0x190 

/* positions of the interrupts in the IRQ regiters */
#define IRQ_INTA_BITNUM			8
#define IRQ_INTB_BITNUM			9
#define IRQ_INTC_BITNUM			10
#define IRQ_INTD_BITNUM			11
#define IRQ_PCIINT_BITNUM		18


#define FIQ_STATUS_REG                  0x280 
#define FIQ_RAW_STATUS_REG              0x284 
#define FIQ_ENABLE_REG                  0x288 
#define FIQ_ENABLE_SET_REG              0x288 
#define FIQ_ENABLE_CLEAR_REG            0x28C 
#define FIQ_SOFT_REG                    0x290 
#define TIMER_1_LOAD_REG                0x300 
#define TIMER_1_VALUE_REG               0x304 
#define TIMER_1_CONTROL_REG             0x308 
#define TIMER_1_CLEAR_REG               0x30C 

#define TIMER_2_LOAD_REG                0x320 
#define TIMER_2_VALUE_REG               0x324 
#define TIMER_2_CONTROL_REG             0x328 
#define TIMER_2_CLEAR_REG               0x32C 

#define TIMER_3_LOAD_REG                0x340 
#define TIMER_3_VALUE_REG               0x344 
#define TIMER_3_CONTROL_REG             0x348 
#define TIMER_3_CLEAR_REG               0x34C 

#define TIMER_4_LOAD_REG                0x360 
#define TIMER_4_VALUE_REG               0x364 
#define TIMER_4_CONTROL_REG             0x368 
#define TIMER_4_CLEAR_REG               0x36C 

/* 	END */
	
