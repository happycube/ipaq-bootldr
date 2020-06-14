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
/*	;; boot.s
	;; Compaq Personal Server Monitor Boot Code
        ;;      
        ;; Copyright 1999 Compaq Computer Corporation
        ;; All Rights Reserved
	;;
        ;; 
        ;; 
	;; This code is a very minimalist loader -- it jumps straight to
	;; main() after initalizing SDRAM,PCI,and the UART. =)
	;; boot.s from the Digital StrongARM uHAL provided inspiration for
	;; this loader.
*/
	
#ifdef __NetBSD__
#include <arm32/asm.h>
#else
#define __ASSEMBLY__	
#include <linux/linkage.h>
#define ASENTRY(x_) ENTRY(x_)        
#define _C_FUNC(x_) (/**/x_/**/)
#define _C_LABEL(x_) /**/x_/**/
	
#endif
#include "architecture.h"
#include "bootconfig.h"
#include "regs-21285.h"
#include "boot_flags.h"
// bavery -- may have wrong reg issues, be careful!
#include "sa1100.h"	

/*;; memory layout and setup constants*/
#define SDRAM_TREF_MIN	(1<<16)		/*min refresh wait (1 cycle)*/
#define PCI_DRAMBASE	DRAM_BASE	/* PCI maps into kernel space*/
#define SETUP_PCI_INT_ID	0x1	/* Init PCI Interrupt ID=1*/
	
/*;; SA_CONTROL (0x13C) control register constants*/
#define INIT_COMPLETE	1
#define PCI_NRESET	0x200		/* when 1 and pci_cfn asserted,*/
					/* pci_rst_l deasserted*/
					/* (page 7-51)*/

#define INITIAL_SDRAM_TIMING_REGISTER_VAL	(SDRAM_TRP|SDRAM_TDAL|\
	SDRAM_TRCD|SDRAM_TCAS|\
	SDRAM_TRC|SDRAM_CMD_DRV_TIME|/*no parity*/|\
	SDRAM_SA110_PRIME|SDRAM_TREF_MIN)

#define SENSIBLE_SDRAM_TIMING_REGISTER_VAL	(SDRAM_TRP|SDRAM_TDAL|\
	SDRAM_TRCD|SDRAM_TCAS|\
        SDRAM_TRC|SDRAM_CMD_DRV_TIME|/*no parity*/|\
	SDRAM_SA110_PRIME|SDRAM_TREF)		
		
/*;; PCI Command Register*/
#define PCI_IO_ENABLE		0x0001
#define PCI_MEM_ENABLE		0x0002
#define PCI_MASTER_ENABLE	0x0004
#define PCI_MEM_WRINVALID	0x0010
#define PCI_PERR_ENABLE 	0x0020
#define PCI_SERR_ENABLE 	0x0080
#define PCI_FAST_B2B		0x0200
#define PCI_CFN_INIT		(PCI_IO_ENABLE|PCI_MEM_ENABLE | \
	                         PCI_MASTER_ENABLE|PCI_MEM_WRINVALID | \
                                 PCI_PERR_ENABLE|PCI_SERR_ENABLE)



	
	/*; *************************/
	/*; Start of executable code*/
	/*; *************************/
	/*
	 * due to being an a.out, this is really offset 0x20 past the
	 * load address
	 * The first 32bits is patched to be a jump around the
	 * a.out header
	 */
ASENTRY(_start)	
ENTRY(ResetEntryPoint)
	
	/*; switch to high memory (since 21285 aliases to 0 from 0x4100)*/
	/*; Loading pc with HiReset makes sure we are running from real rom,*/
	/*; instead of a shadowed address. The 1st write after reset switches*/
	/*; the memory map for normal operation*/
	B	HiReset
	
UndefEntryPoint:	
	B	HandleUndef
	
SWIEntryPoint:	
	B	HandleSWI
	
PrefetchAbortEntryPoint:	
	B	HandlePrefetchAbort
	
DataAbortEntryPoint:	
	B	HandleDataAbort
	
NotUsedEntryPoint:      	
	b       HandleNotUsed
IRQEntryPoint:	
	B	HandleIRQ
	
FIQEntryPoint:	
	B	HandleFIQ
	
/* 0x20:        */	
	.long   BOOTLDR_MAGIC       /* magic number so we can verify that we only put bootldr's into flash in the bootldr region */
/* 0x24:        */	
	.long   BOOTLDR_VERSION
/* 0x28:        */	
        .long   _start              /* where this bootldr was linked, so we can put it in memory in the right place */
/* 0x2C:	*/
	.long	ARCHITECTURE_MAGIC       /* this contains the platform, cpu and machine id */
		
/* 0x30:        */
        .long   BOOTLDR_CAPS

/* 0x34:        */
	.ascii	BL_SIG

HandleUndef:	
	mov	r0, #0
	str	r14, [r0, #0x0]
	mrc	p15, 0, r1, c6, c0, 0 /* fault address */
	str	r1, [r0, #0x4]
	ORR	pc,pc,#FLASH_BASE/* jump to Flash*/
	ldr	r0,STR_UNDEF
	BL	PrintWord
	B	WaitForInput
	
HandleSWI:	
	ORR	pc,pc,#FLASH_BASE/* jump to Flash*/
	LDR	r0,STR_SWI
	BL	PrintWord
	B	WaitForInput
	
HandlePrefetchAbort:	
	mov	r0, #0
	str	r14, [r0, #0x0]
	mrc	p15, 0, r1, c6, c0, 0 /* fault address */
	str	r1, [r0, #0x4]
	ORR	pc,pc,#FLASH_BASE/* jump to Flash*/
	LDR	r0,STR_PREFETCH_ABORT
	BL	PrintWord
	B	WaitForInput
	
HandleDataAbort:	
	mov	r0, #0
	str	r14, [r0, #0x0]
	mrc	p15, 0, r1, c6, c0, 0 /* fault address */
	str	r1, [r0, #0x4]
	ORR	pc,pc,#FLASH_BASE/* jump to Flash*/
	LDR	r0,STR_DATA_ABORT
	MOV	r1, r14
	BL	PrintWord
	B	WaitForInput
	
HandleIRQ:	
	ORR	pc,pc,#FLASH_BASE/* jump to Flash*/
	LDR	r0,STR_IRQ
	BL	PrintWord
	B	WaitForInput
	
HandleFIQ:	
	ORR	pc,pc,#FLASH_BASE/* jump to Flash*/
	LDR	r0,STR_FIQ
	BL	PrintWord
	B	WaitForInput
	
HandleNotUsed:	
	ORR	pc,pc,#FLASH_BASE/* jump to Flash*/
	LDR	r0,STR_UNUSED
	BL	PrintWord
	B	WaitForInput

WaitForInput:	
	MOV	r1,#CSR_BASE
	LDR	r0,[r1,#UARTFLG_REG]
	TST	r0,#UART_RX_FIFO_EMPTY
	BNE	WaitForInput
	
ConsumeInput:	
	MOV	r1,#CSR_BASE
	LDR	r2,[r1,#UARTDR_REG]
	LDR	r0,[r1,#UARTFLG_REG]
	TST	r0,#UART_RX_FIFO_EMPTY
	BEQ	ConsumeInput
	
HiReset:	
	ORR	pc,pc,#FLASH_BASE/* make sure we're in Flash memory space*/
	MOV     r0,#DRAM_SIZE
	STR	r0,[r0,#-1]

	/*; Turn off all interrupts*/
	/*; page 7-61*/
	MOV	r1, #CSR_BASE
	MOV     r0, #0xFFFFFFFF	/* into r0 to clear all interrupt enables*/
	STR	r0, [r1, #IRQ_ENABLE_CLEAR_REG]
	STR	r0, [r1, #FIQ_ENABLE_CLEAR_REG]

	
	/*; see 7-38 of the 21285 spec */
	/*; faster flash speed.*/

	LDR	r0,[r1,#SA_CONTROL]
	ORR	r0,r0,#(FLASH_ACCESS_TIME<<16)
	ORR	r0,r0,#(FLASH_BURST_TIME<<20)
	ORR	r0,r0,#(FLASH_TRISTATE_TIME<<24)
	STR	r0,[r1,#SA_CONTROL]

	/*; Initialize Serial UART*/
	BL	InitUART
	
/*; debug print*/
	MOV	r0,#'\r'
	BL      PrintChar
	MOV	r0,#'\n'
	BL      PrintChar
	MOV	r0,#'*'
	BL      PrintChar
	MOV	r0,#'$'
	BL      PrintChar
		
	/*; Initialize SDRAM*/
	BL	InitMem
	
	LDR     r0, STR_MTST
	BL      PrintWord

TestDram:       	
	/* some test code */
	MOV	r5, #0
	MOV	r0, #0x00000001
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000002
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000004
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000008
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000010
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000020
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000040
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000080
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000100
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000200
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000400
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000800
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00001000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00002000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00004000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00008000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00010000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00020000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00040000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00080000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00100000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00200000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00400000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00800000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x01000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x02000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x04000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x08000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x10000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x20000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x40000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x80000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord

	LDR	r0, STR_MB2
	BL      PrintWord
	
TestDramBank2:       	
	/* some test code */
	MOV	r5, #0x1000000
	MOV	r0, #0x00000001
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000002
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000004
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000008
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000010
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000020
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000040
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000080
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000100
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000200
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000400
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00000800
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00001000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00002000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00004000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00008000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00010000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00020000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00040000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00080000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00100000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00200000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00400000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x00800000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x01000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x02000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x04000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x08000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x10000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x20000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x40000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord
	MOV	r0, #0x80000000
	STR	r0, [r5, #48]
	LDR	r0, [r5, #48]
	bl      PrintHexWord

/*; debug print*/
DebugPrintAfterTestDram:        	
	LDR	r0, STR_ENDM
	BL      PrintWord

#ifdef NOTDEF
memTest:                        

        /* Initial - fill memory */
        mov     r0,#DRAM_BASE
        add     r1,r0,#DRAM_SIZE
	ldr     r2,CRC32POLY
	mov     r3,#-1
        mov     r4,#1

10:     orrs    r3,r4,r3,lsl#1
        eorcs   r3,r3,r2
        str     r3,[r0], #4
        mov     r6,r0
	mov     r7,r1
	mov     r8,r2
	mov     r9,r3
	mov     r10,r4
	bl      PrintHexWord
	mov     r0,r6
	mov     r1,r7
	mov     r2,r8
	mov     r3,r9
	mov     r4,r10
        cmp     r0,r1
        bne     10b

        /* Phase two, check */
        mov     r0,#DRAM_BASE
        mov     r3,#-1

20:     orrs    r3,r4,r3,lsl #1
        eorcs   r3,r3,r2
        ldr     r5,[r0],#4
        cmp     r5,r3
        bne     memError
        mov     r6,r0
	mov     r7,r1
	mov     r8,r2
	mov     r9,r3
	mov     r10,r4
	bl      PrintHexWord
	mov     r0,r6
	mov     r1,r7
	mov     r2,r8
	mov     r3,r9
	mov     r4,r10
        cmp     r0,r1
        bne     20b
	b	InitializePCI
	
memError:	
	LDR	r0, STR_MEM_ERROR
	BL      PrintWord
memError1:	
	B	memError1
#endif	
		
/*; Initialize PCI*/
InitializePCI:	
	BL	InitPCI

/*; debug print*/
	MOV	r0,#'*'
	BL      PrintChar

Enable5V:	
        /*; turn on 5V supply (to get the motor drivers into reset state ASAP -Jamey 1/3/99 */
        LDR     r0,DW_USB_CONFIGURATION_BASE
	LDR     r1,[r0,#0x50]      /* read current value*/
	BIC     r1,r1,#(1<<(3+16))
        STR     r1,[r0,#0x50]
	/*; enable I2C port, turn on both I2C bits*/
        LDR     r1,[r0,#0x4c]
        ORR     r1,r1,#(0x27 << 16)
        STR     r1,[r0,#0x4c]

/*; debug print*/
	MOV	r0,#'*'
	BL      PrintChar

#ifdef CACHE_ENABLED
	/*; Enable the Icache*/
#ifdef ICACHE_ENABLED	
	MRC	p15, 0, r0, c1, c0, 0
	ORR	r0, r0, #0x1000  /* bit 12 is ICACHE enable*/
	MCR	p15, 0, r0, c1, c0, 0
#endif

	/*; flush the I/D caches*/
        /*;  c7 == cache control operation register*/
        /*;  crm==c7 opc2==0 indicates Flush I+D Cache*/
        /*;  r0 is ignored*/
	MCR	p15, 0, r0, c7, c7, 0x00 
        /*; flush the I+D TLB*/
        /*;  c8 is TLB operation register*/
        /*;  crm=c7 opc2==0 indicates Flush I+D TLB*/
        /*;  r0 is ignored*/
	MCR	p15, 0, r0, c8, c7, 0x00 

	/*; Flush the data cache*/
	LDR	r0,DW_CACHE_FLUSH_REGION
	BL	_C_FUNC(writeBackDcache)
#endif	

/*; debug print*/
	MOV	r0,#'*'
	BL      PrintChar
	
#if 0	
	/*; clear all of SDRAM to 0*/
	MOV	r1,#0x00
	MOV	r2,#DRAM_SIZE
	MOV	r3,#0x00
	BL	zi_init
#endif	

	MOV	r0,#'%'
	BL      PrintChar
	
#ifdef CACHE_ENABLED
	/*; Flush the data cache*/
	LDR	r0,DW_CACHE_FLUSH_REGION
	BL	_C_FUNC(writeBackDcache)
#endif
	
	MOV	r0,#'*'
	BL      PrintChar
		
	/*; Get ready to call C functions*/
	LDR	r0, DW_STACK_STR
	BL	PrintWord
	LDR	r0, DW_STACK_START
	BL	PrintHexWord

ASENTRY(call_main)
	LDR	sp,DW_STACK_START
	MOV	fp,#0		/* no previous frame, so fp=0*/

	MOV	a1, #0		/* set argc to 0*/
	MOV	a2, #0		/* set argv to NUL*/
	BL	bootldr_main		/* call main*/

	mov	pc, #FLASH_BASE	/* reboot */

	/*; *****************************************************************/
	/*; InitMem - initialize SDRAM*/
	/*; *****************************************************************/
ASENTRY(InitMem)

	/*
	;; SDRAM POWER ON SEQUENCE
	;; Mitsubishi M5M4V64Sx0 spec, page 13 
	
	;; Before starting normal operation, the following power on
	;; sequence is necessary to prevent a SDRAM from damaged or
	;; malfunctioning.
	;; 
	;; 1.Clock will be applied at power up along with
	;;    power. Attempt to maintain CKE high, DQM (x4,x8), DQMU/L
	;;    (x16) high and NOP condition at the inputs along with
	;;    power.
	;; 2. Maintain stable power, stable clock, and NOP input
	;;    conditions for a minimum of 200us.
	;; 3. Issue precharge commands for all banks. (PRE or PREA)
	;; 4. After all banks become idle state (after tRP), issue 8
	;;    or more auto-refresh commands.
	;; 5. Issue a mode register set command to initialize the mode
	;;    register.
	;; 
	;; After these sequence, the SDRAM is idle state and ready for
	;; normal operation.
	
	;; MAX708 power supervisor maintains reset for 200ms after
	;; poweron or reset switch.
	
	;; After reset, the SDRAM arrays are in an unknown state. To
	;; put them into a known state, force an all-banks precharge
	;; to each of the four possible arrays. You must access all
	;; four arrays for this even if all four are not fitted. This
	;; is necessary since the 21285 counts these precharge
	;; accesses, and inhibits access to the SDRAM until all four
	;; have been completed. Failure to perform four precharge
	;; access will result in unpredictable operation. An all-banks
	;; precharge is initiated by a read from any address in the
	;; mode register address space.
	*/
	MOV	r1,#DC21285_DRAM_A0MR	/* SDRAM array 0*/
	LDR	r0,[r1]			/* just read, don't care what returns*/
	ADD	r1,r1,#0x4000
	LDR	r0,[r1] /* SDRAM array 1*/
	ADD	r1,r1,#0x4000
	LDR	r0,[r1] /* SDRAM array 2*/
	ADD	r1,r1,#0x4000
	LDR	r0,[r1] /* SDRAM array 3*/


	/*
	;; Write to the SDRAM Timing Register. Set the refresh interval to the
	;; minimum because we have to wait for 8 refresh cycles to complete
	;; before we can rely on the SDRAMS operating normally.
	*/

	MOV	r1,#CSR_BASE
	LDR	r0,INITIAL_SDRAM_TIMING_REGISTER
	STR	r0,[r1,#SDRAM_TIMING]

	/*
	;; Wait for 8 refresh cycles to complete. The minimum refresh interval
	;; is 32 cycles and we are currently running with the Icache off, so
	;; the complete process will take 256 cycles (but make it 512)
	*/

	MOV	r0,#0x200
wait:	SUBS	r0,r0,#1
	BGT	wait

	/*
	;; Write to the SDRAM Mode Register. This requires one write operation
	;; for each SDRAM array. The address is important, not the data. The
	;; offset from the start of the mode space for each SDRAM array controls
	;; what data is written to the SDRAM mode register.
	*/


        MOV     r0,#0xFB
	MOV	r1,#(DC21285_DRAM_A0MR)
	STRB	r0,[r1,#((SDRAM_MODE_WM|SDRAM_MODE_LTMODE|SDRAM_MODE_BT|SDRAM_MODE_BL)<<2)]			/* doesn't matter what we write*/
        ADD     r1,r1,#0x4000     /*  DC21285_DRAM_A1MR*/
	STRB	r0,[r1,#((SDRAM_MODE_WM|SDRAM_MODE_LTMODE|SDRAM_MODE_BT|SDRAM_MODE_BL)<<2)]			
        ADD     r1,r1,#0x4000     /*  DC21285_DRAM_A2MR*/
	STRB	r0,[r1,#((SDRAM_MODE_WM|SDRAM_MODE_LTMODE|SDRAM_MODE_BT|SDRAM_MODE_BL)<<2)]			
        ADD     r1,r1,#0x4000     /*  DC21285_DRAM_A3MR*/
	STRB	r0,[r1,#((SDRAM_MODE_WM|SDRAM_MODE_LTMODE|SDRAM_MODE_BT|SDRAM_MODE_BL)<<2)]			

	/*
	;; Write to the four SDRAM Address and Size Registers. Note that we are
	;; NOT doing any kind of automatic memory detection and sizing here
	*/

	MOV     r1,#CSR_BASE
	LDR	r0,DW_SDRAM_ARRAY0_ADDRSIZE
	STR	r0,[r1,#SDRAM_ADDR_SIZE_0]
	LDR	r0,DW_SDRAM_ARRAY1_ADDRSIZE
	STR	r0,[r1,#SDRAM_ADDR_SIZE_1]
	MOV	r0,#SDRAM_ARRAY2_ADDRSIZE
	STR	r0,[r1,#SDRAM_ADDR_SIZE_2]
	MOV	r0,#SDRAM_ARRAY3_ADDRSIZE
	STR	r0,[r1,#SDRAM_ADDR_SIZE_3]

	/*
	;; Finally, reset the refresh interval to a sensible value. Continuing
	;; to run with a very short interval would waste memory bandwidth
	*/
	
	LDR	r0,SENSIBLE_SDRAM_TIMING_REGISTER
	STR	r0,[r1,#SDRAM_TIMING]

	MOV	pc, lr			/* All done, return*/

	/*
	;; ********************************************************************
	;; InitPCI - initialize PCI
	;; NOTE:	lr contains return address
	;;		a1 returns top of memory
	;; ********************************************************************
	;; This code does the bare minimum setup necessary to allow the PCI
	;; interface to be used.
	*/
ASENTRY(InitPCI)
	MOV	r1,#CSR_BASE

	MOV	r0,#0xC		/* disable outbound interrupts*/
	STR	r0,[r1,#PCI_OUT_INT_MASK]

	MOV	r0,#0		/* clear doorbell interrupts to PCI*/
	STR	r0,[r1,#DBELL_PCI_MASK]
	STR	r0,[r1,#DBELL_SA_MASK]	/* clear doorbell interrupts to SA110*/
	MOV     r0,#(0 << 15)
	STR	r0,[r1,#PCI_ADDR_EXT]	/* map PCI addr bit 31 to 0, as netbsd seems to expect -- Jamey 12/16/98*/

	MOV	r0,#SETUP_PCI_INT_ID	/* set int. ID=1 some sys can't handle 0*/
	STR	r0,[r1,#PCI_INT_LINE]   /* 21285 does not interpret this register*/

	LDR	r0,[r1,#SA_CONTROL]
	ORR	r0, r0,#PCI_NRESET	/* deassert PCI reset signal (page 7-51)*/
	/* leave these bits 0 -- set direction of xcs_l[2:0], used by PCI arbiter*/
	STR	r0,[r1,#SA_CONTROL]

	MOV	r0,#PCI_WINDOW_SIZE	/* open up window from PCI->SDRAM*/
	STR	r0,[r1,#DRAM_BASE_ADDR_MASK]
        MOV     r0,#0x00300000          /* 4MB expansion rom window size*/
        STR     r0,[r1,#ROM_BASE_ADDR_MASK]

	LDR	r0,[r1,#PCI_COMMAND]
	AND	r0,r0,#0		/* disable PCI for now, enabled below*/
	STR	r0,[r1,#PCI_COMMAND]
	LDR     r0,[r1,#PCI_COMMAND]     /* read PCI status register*/

	MOV	r0,#SZ_1G		/* CSR's appear much after DRAM  section 7.3.6, 7.1.11*/
	STR	r0,[r1,#PCI_CSR_MEM_BAR]
	MOV	r0,#0xf000		/* write 0xf000 to CSR I/O Base Addr Reg*/
	STR	r0,[r1,#PCI_IO_BAR]

	MOV	r0,#0			/* write 0x0 to SDRAM Base Addr. Reg.*/
	STR	r0,[r1,#PCI_DRAM_BAR]
	STR	r0,[r1,#MAILBOX_0]
	STR	r0,[r1,#MAILBOX_1]
	STR	r0,[r1,#MAILBOX_2]
	STR	r0,[r1,#MAILBOX_3]
	STR	r0,[r1,#DOORBELL_SETUP]

	/* Done: respond to I/O space & Memory transactions.*/
	/*	ALSO BE PCI MASTER*/
	MOV	r0, #PCI_CFN_INIT
	STR	r0, [r1, #PCI_COMMAND]

	/* Signal PCI_init_complete, won't hurt if it's already been done*/
	LDR	r0, [r1, #SA_CONTROL]
	ORR	r0, r0, #INIT_COMPLETE
	STR	r0, [r1, #SA_CONTROL]

	MOV	pc, lr			/* All done, return*/

	/*
	;; ********************************************************************
	;; InitUART - Initialize Serial Communications
	;; ********************************************************************
	;; Following reset, the UART is disabled. So, we do the following:
	*/
ASENTRY(InitUART)
	
	MOV	r1,#CSR_BASE
	ldr	r2,DW_GPIO_BASE
        MOV     r3,#FLASH_BASE
	
	/*; Program the UART control register*/
	MOV	r0,#0		/* disable UART completely, no HP SIR or IrDA*/
	STR	r0,[r1,#UARTCON_REG]

#if defined(MEMCLK_33MHZ)
#warning here	
        B Memclk33MHzInitUart
#elif defined(MEMCLK_48MHZ)
        B Memclk48MHzInitUart
#elif defined(MEMCLK_60MHZ)
        B Memclk60MHzInitUart
#endif	

	/* first, check for existence of GPIO registers */
	ldr     r3, [r3, #0xC] /* contents of third word of flash */
	/* if SkiffV2, check Memclk speed in GPIO register 0 */
	ldr     r4, [r2, #0xC] /* possible GPIO contents */
        cmp     r3, r4
        beq     Memclk33MHzInitUart /* SkiffV1 -- no GPIO */
	and     r4, r4, #SYSTEM_REV_MEMCLK_MASK
        cmp     r4, #SYSTEM_REV_MEMCLK_48MHZ
	beq     Memclk48MHzInitUart
        cmp     r4, #SYSTEM_REV_MEMCLK_60MHZ
	beq     Memclk60MHzInitUart
	/* default to 33MHz */
Memclk33MHzInitUart:
	/*; Write H_UBRLCR,L_UBRLCR,MUBRLCR to set up comm parameters*/
	/*; Note: MUST UPDATE IN THIS ORDER! (see sec. 6.6.5 in 21285 databook)*/
	MOV	r0,#UART_L_UBRLCR_33MHZ
	STR	r0,[r1,#L_UBRLCR_REG]
	MOV	r0,#UART_M_UBRLCR_33MHZ
	STR	r0,[r1,#M_UBRLCR_REG]
	MOV	r0,#UART_H_UBRLCR
	STR	r0,[r1,#H_UBRLCR_REG]
 	B       InitUartEnable

Memclk48MHzInitUart:    	
	/*; Write H_UBRLCR,L_UBRLCR,MUBRLCR to set up comm parameters*/
	/*; Note: MUST UPDATE IN THIS ORDER! (see sec. 6.6.5 in 21285 databook)*/
	MOV	r0,#UART_L_UBRLCR_48MHZ
	STR	r0,[r1,#L_UBRLCR_REG]
	MOV	r0,#UART_M_UBRLCR_48MHZ
	STR	r0,[r1,#M_UBRLCR_REG]
	MOV	r0,#UART_H_UBRLCR
	STR	r0,[r1,#H_UBRLCR_REG]
 	B       InitUartEnable

Memclk60MHzInitUart:    	
	/*; Write H_UBRLCR,L_UBRLCR,MUBRLCR to set up comm parameters*/
	/*; Note: MUST UPDATE IN THIS ORDER! (see sec. 6.6.5 in 21285 databook)*/
	MOV	r0,#UART_L_UBRLCR_60MHZ
	STR	r0,[r1,#L_UBRLCR_REG]
	MOV	r0,#UART_M_UBRLCR_60MHZ
	STR	r0,[r1,#M_UBRLCR_REG]
	MOV	r0,#UART_H_UBRLCR
	STR	r0,[r1,#H_UBRLCR_REG]
 	B       InitUartEnable

InitUartEnable:	
	/*; set enable bit in UARTCON register*/
	MOV	r0,#1
	STR	r0,[r1,#UARTCON_REG]

	MOV	pc, lr		/* All done, return*/

	/*
	;; ********************************************************************
	;; PrintHexNibble -- prints the least-significant nibble in R0 as a
	;; hex digit
        ;;   Reads r0, writes r0 with RXSTAT, modifies r0,r1,r2
	;;   Falls through to PrintChar
	;; ********************************************************************
	*/
PrintHexNibble:	
	LDR	r1, ADDR_HEX_TO_ASCII_TABLE
	AND	r0, r0, #0xF
	LDR	r0, [r1,r0]	/* convert to ascii */
				/* fall through to PrintChar*/
	/*
	;; ********************************************************************
	;; PrintChar -- prints the character in R0
        ;;   Reads r0, writes r0 with RXSTAT, modifies r0,r1,r2
	;;********************************************************************
	*/
PrintChar:	
	MOV	r1, #CSR_BASE
TXBusy:	
        LDR     r2,[r1,#UARTFLG_REG]/* check status*/
	TST     r2,#UART_TX_FIFO_BUSY
        BNE     TXBusy
	STR	r0,[r1,#UARTDR_REG]
	LDR     r0,[r1,#UARTFLG_REG]
        MOV     pc, lr

	/*
        ;; ********************************************************************
	;; PrintWord -- prints the 4 characters in R0
        ;;   Reads r0, writes r0 with RXSTAT, modifies r1,r2,r3,r4
	;; ********************************************************************
	*/
PrintWord:	
	MOV	r3, r0
	MOV     r4, lr
	BL      PrintChar     
	
	MOV     r0,r3,LSR #8    /* shift word right 8 bits*/
	BL      PrintChar
	
	MOV     r0,r3,LSR #16   /* shift word right 16 bits*/
	BL      PrintChar
	
	MOV     r0,r3,LSR #24   /* shift word right 24 bits*/
	BL      PrintChar

	MOV	r0, #'\r'
	BL	PrintChar

	MOV	r0, #'\n'
	BL	PrintChar
	
        MOV     pc, r4

	/*
        ;; ********************************************************************
	;; PrintHexWord -- prints the 4 bytes in R0 as 8 hex ascii characters
	;;   followed by a newline
        ;;   Reads r0, writes r0 with RXSTAT, modifies r1,r2,r3,r4
	;; ********************************************************************
	*/
PrintHexWord:	
	MOV     r4, lr
	MOV	r3, r0
	MOV	r0, r3,LSR #28
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #24
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #20
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #16
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #12
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #8
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #4
	BL      PrintHexNibble     
	MOV	r0, r3
	BL      PrintHexNibble     

	MOV	r0, #'\r'
	BL	PrintChar

	MOV	r0, #'\n'
	BL	PrintChar

        MOV     pc, r4

	/*
	;; ********************************************************************
	;; copy - copies are region from addr in r0 to r1. r2=stopping point
	;; ********************************************************************
	*/
copy:	
	CMP	r1,r2
	LDRCC	r3,[r0],#4
	STRCC	r3,[r1],#4
	BCC	copy
	MOV	pc,lr

	/*
	;; ********************************************************************
	;; zi_init - initializes region starting at r1-r2 to r3
	;; ********************************************************************
	*/
zi_init:	
	CMP	r1,r2
	STRCC	r3,[r1],#4
	BCC	zi_init
	MOV	PC,lr

	/*
	;; ********************************************************************
	;; enableMMU
	;; ********************************************************************
	*/
ENTRY(enableMMU)
	/*; r0 = translation table base*/
	LDR	r0,DW_MMU_TABLE
	MCR	p15, 0, r0, c2, c0, 0

	/*; r0 = domain access control*/
	LDR	r0,MMU_DOMCTRL
	MCR	p15, 0, r0, c3, c0, 0


	/*; enable the MMU*/
	MRC	p15, 0, r0, c1, c0, 0
	ORR	r0, r0, #1      /* bit 0 of c1 is MMU enable*/
#ifdef CACHE_ENABLED	
	ORR	r0, r0, #4      /* bit 2 of c1 is DCACHE enable*/
	ORR	r0, r0, #8      /* bit 3 of c1 is WB enable*/
#endif	
	MCR	p15, 0, r0, c1, c0, 0

	/*
	;; flush the I/D caches
        ;;  c7 == cache control operation register
        ;;  crm==c7 opc2==0 indicates Flush I+D Cache
        ;;  r0 is ignored
	*/
	MCR	p15, 0, r0, c7, c7, 0x00
	/*
        ;; flush the I+D TLB
        ;;  c8 is TLB operation register
        ;;  crm=c7 opc2==0 indicates Flush I+D TLB
        ;;  r0 is ignored
	*/
	MCR	p15, 0, r0, c8, c7, 0x00 
	
	/*; return*/
	MOV	pc,lr		

	/*
	;; ********************************************************************
	;; flushTLB
	;; ********************************************************************
	*/
ENTRY(flushTLB)
	/*
	;; flush the I/D caches
        ;;  c7 == cache control operation register
        ;;  crm==c7 opc2==0 indicates Flush I+D Cache
        ;;  r0 is ignored
	*/
	MCR	p15, 0, r0, c7, c7, 0x00
	/*
        ;; flush the I+D TLB
        ;;  c8 is TLB operation register
        ;;  crm=c7 opc2==0 indicates Flush I+D TLB
        ;;  r0 is ignored
	*/
	MCR	p15, 0, r0, c8, c7, 0x00 
	
/*;; return*/
	MOV	pc,lr		
ENTRY(writeBackDcache)
	/*
	;; r0 points to the start of a 16384 byte region of readable 
	;; data used only for this cache flushingroutine. If this area 
	;; is to be used by other code, then 32K must be loaded and the
	;; flush MCR is not needed.
	/*

	/*; return:	 r0,r1,r2 trashed. data cache is clean*/
	add r1,r0,#32768
l1:	
	ldr r2,[r0],#32
	teq r1, r0
	bne l1
	mcr p15,0,r0,c7,c6,0
	mov pc,lr

ENTRY(readPC)
        mov     r0,pc
        mov     pc,lr

ENTRY(readCPR0)
        mrc     p15,0,r0,c0,c0,0
        mov     pc,lr
		
ENTRY(readCPR1)
        MRC     p15,0,r0,c1,c0,0
        MOV     pc,lr

ENTRY(readCPR2)
        mrc     p15,0,r0,c2,c0,0
        mov     pc,lr
	
ENTRY(readCPR3)
        MRC     p15,0,r0,c3,c0,0
        MOV     pc,lr
ENTRY(readCPR5)
        mrc     p15,0,r0,c5,c0,0
        mov     pc,lr

ENTRY(readCPR6)
        mrc     p15,0,r0,c6,c0,0
        mov     pc,lr
	
	
ENTRY(readCPR13)
        mrc     p15,0,r0,c13,c0,0
        mov     pc,lr
		
ENTRY(readCPR14)
        mrc     p15,0,r0,c14,c0,0
        mov     pc,lr

/*
        ;; ********************************************************************
	;; PrintHexWord2 -- prints the 4 bytes in R0 as 8 hex ascii characters
	;;   followed by a newline
        ;;   Reads r0, writes r0 with RXSTAT, modifies r1,r2,r3,r4
	;; ********************************************************************
	*/
ENTRY(PrintHexWord2)
	MOV     r4, lr
	MOV	r3, r0
	MOV	r0, r3,LSR #28
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #24
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #20
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #16
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #12
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #8
	BL      PrintHexNibble     
	MOV	r0, r3,LSR #4
	BL      PrintHexNibble     
	MOV	r0, r3
	BL      PrintHexNibble     

	MOV	r0, #'\r'
	BL	PrintChar

	MOV	r0, #'\n'
	BL	PrintChar

        MOV     pc, r4
		
		
	/*
	;; ********************************************************************
	;; boot - boots into another image. DOES NOT RETURN!
	;; r0 = pointer to bootinfo
	;; r1 = entry point of kernel
	;; ********************************************************************
	*/
ENTRY(boot)
	/*
        ;; flush the I+D TLB
        ;;  c8 is TLB operation register
        ;;  crm=c7 opc2==0 indicates Flush I+D TLB
        ;;  r0 is ignored
	*/
	MCR	p15, 0, r0, c8, c7, 0x00 /* flush I+D TLB */
        MCR     p15, 0, r0, c7, c10, 4 /* drain the write buffer*/

        /*; make sure the pipeline is emptied*/
        MOV     r0,r0
        MOV     r0,r0
        MOV     r0,r0
        MOV     r0,r0

	MOV	pc,r1		/* jump to addr. bootloader is done*/

	/*
	;; ********************************************************************
	;; bootLinux - boots into another image. DOES NOT RETURN!
	;; r0 = must contain a zero or else the kernel loops
        ;; r1 = architecture type (6 for old kernels, 17 for new)
	;; r2 = entry point of kernel
	;; ********************************************************************
	*/
ENTRY(bootLinux)
	/*
        ;; flush the I+D TLB
        ;;  c8 is TLB operation register
        ;;  crm=c7 opc2==0 indicates Flush I+D TLB
        ;;  r0 is ignored
	*/
	MOV	r7,r1
	MOV	r8,r2

	// verify mmu is on	
	MRC     p15,0,r0,c1,c0,0
        bl	PrintHexWord
	mov	r0,r8
	bl	PrintHexWord

	MCR	p15, 0, r0, c8, c7, 0x00 /* flush I+D TLB */
        MCR     p15, 0, r0, c7, c10, 4 /* drain the write buffer*/
// dcache is off anyway
//	LDR	r0,DW_CACHE_FLUSH_REGION
//	BL	_C_FUNC(writeBackDcache)

	
	MOV     r3, #0x130
	MCR     p15, 0, r3, c1, c0, 0   /* disable the MMU */
        /*; make sure the pipeline is emptied*/
	mov	r0,r7
	bl	PrintHexWord
	mov	r0,r8
	bl	PrintHexWord
	// verify mmu is off
	MRC     p15,0,r0,c1,c0,0
        bl	PrintHexWord

	mov     r5,#0x8000
	ldr	r0,[r5,#0]	
	bl	PrintHexWord
	ldr	r0,[r5,#4]	
	bl	PrintHexWord
	ldr	r0,[r5,#0x800]	
	bl	PrintHexWord
	ldr	r0,[r5,#0xc00]	
	bl	PrintHexWord
	
	mov	r1,r7
	mov	r2,r8		
	/*mov	r2,#0x41000000 */
	MOV     r0,#0
        MOV     r0,r0
        MOV     r0,r0
        MOV     r0,r0

	MOV	pc,r2		/* jump to addr. bootloader is done*/

	.align 5
#ifdef HAS_REBOOT_COMMAND
	/* 
	 * remove reboot command until we make it *really* reboot
	 * things.  As it is, doesn't really work  or make much sense.
	 */
	/*
	;; ********************************************************************
	;; reboot - restarts the bootloader. DOES NOT RETURN!
	;; ********************************************************************
	*/
ENTRY(reboot)
       	/*; disable the instruction/data write buffer caches*/
	MRC	p15,0,r2,c1,c0,0
	MOV	r3,#~4
	AND	r2,r2,r3
	MOV	r3,#~8
	AND	r2,r2,r3
	MOV	r3,#~0x1000
	AND	r2,r2,r3
	MCR	p15,0,r2,c1,c0,0

	/*
	;; flush the caches
	;; flush the I/D caches
        ;;  c7 == cache control operation register
        ;;  crm==c7 opc2==0 indicates Flush I+D Cache
        ;;  r0 is ignored
	*/
	MCR	p15, 0, r0, c7, c7, 0x00
	/*
        ;; flush the I+D TLB
        ;;  c8 is TLB operation register
        ;;  crm=c7 opc2==0 indicates Flush I+D TLB
        ;;  r0 is ignored
	*/
	MCR	p15, 0, r0, c8, c7, 0x00
        MCR     p15, 0, r0, c7, c10, 4 /* drain the write buffer*/

        /*; make sure the pipeline is emptied*/
        MOV     r0,r0
        MOV     r0,r0
        MOV     r0,r0

	B	HiReset
#endif /* HAS_REBOOT_COMMAND */

	/*
	 * something that main calls
	 * what it wants ???
	 */
	
ENTRY(__main)
	mov	pc,lr		
	/*
	;; ********************************************************************
	;; Data Area
	;; ********************************************************************
	*/

	.align 2
CRC32POLY:	
	.word	0x04c11db7
	
	.align	2
addr_start:	
	.word	_C_FUNC(ResetEntryPoint)

	.align	2
SENSIBLE_SDRAM_TIMING_REGISTER:
	.word	SENSIBLE_SDRAM_TIMING_REGISTER_VAL

	.align	2
INITIAL_SDRAM_TIMING_REGISTER:
	.word	INITIAL_SDRAM_TIMING_REGISTER_VAL

	.align	2
ENTRY(HEX_TO_ASCII_TABLE)
	.ascii	"0123456789ABCDEF"

	.align	2
ADDR_HEX_TO_ASCII_TABLE:	
	.word	_C_FUNC(HEX_TO_ASCII_TABLE)

	.align	2
DW_STACK_STR:	
	.ascii	"STKP"

	.align	2
DW_GPIO_BASE:	
	.word   GPIO_BASE
	
	.align	2
#if 0
//old way w/o boot flags ptr	
DW_STACK_START:	
	.word	STACK_BASE+STACK_SIZE-4
	
	.align	2
#endif
#if 1
DW_STACK_START:	
	.word	STACK_BASE+STACK_SIZE-32
	
	.globl _C_LABEL(boot_flags_ptr)
_C_LABEL(boot_flags_ptr):
	.word   STACK_BASE+STACK_SIZE-4
	
	.align	2	
#endif
DW_MMU_TABLE:	
	.word	MMU_TABLE_START

	.align	2
DW_USB_CONFIGURATION_BASE:	
	.word	USB_CONFIGURATION_BASE

	.align	2
DW_CACHE_FLUSH_REGION:	
	.word	CACHE_FLUSH_BASE
	
	.align	2
MMU_DOMCTRL:	
	.word	0xFFFFFFFF
DW_SDRAM_ARRAY0_ADDRSIZE:       
        .word	SDRAM_ARRAY0_ADDRSIZE	
DW_SDRAM_ARRAY1_ADDRSIZE:       
        .word	SDRAM_ARRAY1_ADDRSIZE	

	.align	2
STR_MTST:	
        .ascii	"MTST"
	.align	2
STR_MB2:	
        .ascii	"MBK2"
	.align	2
STR_ENDM:	
        .ascii	"ENDM"
	.align	2
	
STR_UNDEF:	
        .ascii	"UNDF"

	.align	2
STR_SWI:	
        .ascii  "SWI "

	.align	2
STR_PREFETCH_ABORT:	
        .ascii	"PABT"

	.align	2
STR_DATA_ABORT:	
        .ascii	"DABT"
	.word   0

	.align	2
STR_IRQ:	
        .ascii	"IRQ "

	.align	2
STR_FIQ:	
        .ascii	"FIQ"

	.align	2
STR_UNUSED:	
        .ascii  "UNSD"

	.align	2
STR_MEM_ERROR:
	.ascii	"DRAM ERROR"
	/*; ******************************************************************/
