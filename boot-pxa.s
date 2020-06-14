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
	;; main() after initalizing SDRAM,PCI,and the UART.
	;; boot.s from the Digital StrongARM uHAL provided inspiration for
	;; this loader.
*/
	
#ifdef __QNXNTO__
#define __ASSEMBLY__
#define ASENTRY(x_) ENTRY(x_)
#define _C_FUNC(x_) /**/x_/**/
#define _C_LABEL(x_) /**/x_/**/
#define ENTRY(name) .globl name; \
                    name:
#else /* not QNX */
#ifdef __NetBSD__
#include <arm32/asm.h>
#else /* not NetBSD */
#define __ASSEMBLY__	
#include <linux/linkage.h>
#define ASENTRY(x_) ENTRY(x_)        
#define _C_FUNC(x_) /**/x_/**/
#define _C_LABEL(x_) /**/x_/**/
#endif /* NetBSD */
#endif /* QNX */	 
#include "architecture.h"
#include "bootconfig.h"
#include "asm-arm/arch-pxa/h3900_asic.h"
#include "asm-arm/arch-pxa/pxa-regs.h"
#include "asm-arm/arch-pxa/h3900-init.h"
#include "pxa.h"	
#include "arm.h"	
#include "boot_flags.h"

/*
 * This macro is used to wait for a CP15 write and is needed
 * when we have to ensure that the last operation to the co-pro
 * was completed before continuing with operation.
 */
	.macro	cpwait, rd
	mrc	p15, 0, \rd, c2, c0, 0		@ arbitrary read of cp15
	mov	\rd, \rd			@ wait for completion
	sub 	pc, pc, #4			@ flush instruction pipeline
	.endm	

	.macro	cpwait_ret, lr, rd
	mrc	p15, 0, \rd, c2, c0, 0		@ arbitrary read of cp15
	sub	pc, \lr, \rd, LSR #32		@ wait for completion and
						@ flush instruction pipeline
	.endm
	
/* probly lives somewhere in kernel/include/asm-arm/arch-pxa  looks like hardware.h */ 
#undef __REG
#define __REG(x) (x)
#define __BASE(x) (x & 0xffff0000)
#define __OFFSET(x) (x & 0x0000ffff)
	
	/*; *************************/
	/*; Start of executable code*/
	/*; *************************/
ENTRY(_start)	
ENTRY(ResetEntryPoint) /* 0x00:  */
	
	b	HiReset
	
/* 0x04:         */	
UndefEntryPoint:	
	b	HandleUndef
	
/* 0x08:         */	
SWIEntryPoint:	
	b	HandleSWI
	
/* 0x0c:         */	
PrefetchAbortEntryPoint:	
	b	HandlePrefetchAbort
	
/* 0x10:         */	
DataAbortEntryPoint:	
	b	HandleDataAbort
	
/* 0x14:         */	
NotUsedEntryPoint:      	
	b       HandleNotUsed
/* 0x18:         */	
IRQEntryPoint:	
	b	HandleIRQ
	
/* 0x1c:         */	
FIQEntryPoint:	
	b	HandleFIQ
	
_bootldr_magic:		
/* 0x20:        */
	.long   BOOTLDR_MAGIC       /* magic number so we can verify that we only put bootldr's into flash in the bootldr region */

_bootldr_version:		
/* 0x24:        */
	.long   BOOTLDR_VERSION
	
_bootldr_start:
/* 0x28:        */	
        .long   _C_LABEL(_start)              /* where this bootldr was linked, so we can put it in memory in the right place */
	
_bootldr_architecture_magic:		
/* 0x2C:	*/
	.long	ARCHITECTURE_MAGIC  /* this contains the platform, cpu and machine id */
	
_bootldr_capabilities:		
/* 0x30 */
	/*
	 * bootldr capabilities
	 */
	.long	BOOTLDR_CAPS

_bootldr_signature:
/* 0x34 */	
	/*
	 * bootldr sig
	 */
        .ascii	BL_SIG
	
_bootldr_end:
/* 0x38 */
        /* bootldr size */
	.globl  __commands_end
        .long   __commands_end

HandleUndef:	
        /* save all user registers */
        adr     r13, AbortStorage
        stmia   r13, {r0-r12}
        mov     r12, r14
        bl      InitUARTFF
	ldr	r0,STR_UNDEF
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintWord
	mov     r0, r12
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	mrc	p15, 0, r0, c6, c0, 0 /* fault address */
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	
	mov     r14, r12
        /* restore registers */
        ldmia   r13, {r0-r12}
        /* now loop */
1:      b 1b
	
HandleSWI:	
        /* save all user registers */
        adr     r13, AbortStorage
        stmia   r13, {r0-r12}
        mov     r12, r14
        bl      InitUARTFF
	ldr	r0,STR_SWI
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintWord
	mov     r0, r12
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	mrc	p15, 0, r0, c6, c0, 0 /* fault address */
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	
	mov     r14, r12
        /* restore registers */
        ldmia   r13, {r0-r12}
        /* now loop */
1:      b 1b
	
HandlePrefetchAbort:	
        /* save all user registers */
#if 0
        adr     r13, AbortStorage
        stmia   r13, {r0-r12}
        mov     r12, r14
        bl      InitUARTFF
#endif
	ldr	r0,STR_PREFETCH_ABORT
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintWord
	mov     r0, r14
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	mov     r0, r13
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	mov     r0, r12
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
#if 0
	mrc	p15, 0, r0, c6, c0, 0 /* fault address */
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	
	mov     r14, r12
        /* restore registers */
        ldmia   r13, {r0-r12}
#endif	
        /* now loop */
1:      b 1b
	
AbortStorage:
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
        .word 0
	
HandleDataAbort:	
        /* save all user registers */
        adr     r13, AbortStorage
        stmia   r13, {r0-r12}
        mov     r12, r14
        bl      InitUARTFF
	ldr	r0,STR_DATA_ABORT
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintWord
	mov     r0, r12
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	mrc	p15, 0, r0, c6, c0, 0 /* fault address */
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	
	mov     r14, r12
        /* restore registers */
        ldmia   r13, {r0-r12}
        /* now loop */
1:      b 1b

HandleIRQ:	
	b       HiReset
                
        /* save all user registers */
        adr     r13, AbortStorage
        stmia   r13, {r0-r12}
        mov     r12, r14
        bl      InitUARTFF
	ldr	r0,STR_IRQ
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintWord
	mov     r0, r12
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	
	mov     r14, r12
        /* restore registers */
        ldmia   r13, {r0-r12}
        /* now loop */
1:      b 1b
	
HandleFIQ:	
        /* save all user registers */
        adr     r13, AbortStorage
        stmia   r13, {r0-r12}
        mov     r12, r14
        bl      InitUARTFF
	ldr	r0,STR_FIQ
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintWord
	mov     r0, r12
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	
	mov     r14, r12
        /* restore registers */
        ldmia   r13, {r0-r12}
        /* now loop */
1:      b 1b
	
HandleNotUsed:	
        /* save all user registers */
        adr     r13, AbortStorage
        stmia   r13, {r0-r12}
        mov     r12, r14
        bl      InitUARTFF
	ldr	r0,STR_NOT_USED
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintWord
	mov     r0, r12
	ldr     r1,_C_LABEL(SerBase)
	BL	PrintHexWord
	
	mov     r14, r12
        /* restore registers */
        ldmia   r13, {r0-r12}
        /* now loop */
1:      b 1b
	

//;	Reset entry point
HiReset:
	//; Workaround for PXA250 early termination of SDRAM auto-refresh
	//; (Erratum #114)
	mov	r0, #0x48000000			//; MEM_BASE

	mov	r1, #0x03800000
	orr	r1, r1, #0x000ff000
	str	r1, [r0, #__OFFSET(MDREFR)]
	mov	r0, #0xa0000000
	ldr	r1, [r0]
	ldr	r1, [r0]

	mov	r2, #0x2000
1:
	ldr	r1, [r0]
	subs	r2, r2, #1
	bpl	1b
	//; end of workaround for erratum 114

        //; keep track of whether button was pushed in r12
	ldr     r12, boot_flags_init_val
	//; btw, r12 is sp

	//; must configure memory interface properly before accessing ASICs
	ldr	r0, GPIO_BASE
	ldr	r1, GPSRx_Init_H5400
	str	r1, [r0, #__OFFSET(GPSR0)]	
	ldr	r1, GPSRy_Init_H5400
	str	r1, [r0, #__OFFSET(GPSR1)]	
	ldr	r1, GPSRz_Init_H5400
	str	r1, [r0, #__OFFSET(GPSR2)]	
	
	ldr	r1, GPCRx_Init_H5400
	str	r1, [r0, #__OFFSET(GPCR0)]	
	ldr	r1, GPCRy_Init_H5400
	str	r1, [r0, #__OFFSET(GPCR1)]	
	ldr	r1, GPCRz_Init_H5400
	str	r1, [r0, #__OFFSET(GPCR2)]	

	ldr	r1, GPDRx_Init_H5400
	str	r1, [r0, #__OFFSET(GPDR0)]	
	ldr	r1, GPDRy_Init_H5400
	str	r1, [r0, #__OFFSET(GPDR1)]	
	ldr	r1, GPDRz_Init_H5400
	str	r1, [r0, #__OFFSET(GPDR2)]	

	//; set up GPIOs
	ldr	r1, GAFR0x_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR0_L)]	
	ldr	r1, GAFR1x_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR0_U)]	
	ldr	r1, GAFR0y_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR1_L)]	
	ldr	r1, GAFR1y_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR1_U)]	
	ldr	r1, GAFR0z_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR2_L)]	
	ldr	r1, GAFR1z_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR2_U)]	

	//; let GPIOS be GPIOS
	ldr	r1,PMCR_BASE
	add	r2,r1,#__OFFSET(PSSR)
	mov	r0,#0x30
	str	r0,[r2]

	//; set up MSC2
	ldr	r0, MEM_BASE
	ldr	r1,MSC2_INIT
	str	r1,[r0,#__OFFSET(MSC2)] //; spec says to ldr the mscs after a set
	ldr	r2,[r0,#__OFFSET(MSC2)]
	
probe_machine_type:
	bl	H1900_Ident	
	bl	H3900_Ident
	bl	H5400_Ident /* must be after H3900_Ident */	

init_by_machine_type:	
	bl	H3900_InitGPIO
	bl	H5400_InitGPIO	
	
check_for_action_button:	
	/* if any machine types defined, then call the machine specific button check */
	tst	r12, #0xFF000000
	bne	1f
	tst	r12, #0x00FF0000
	bne	1f
	b	generic_power_button_test
1:		
	bl	H3900_CheckActionButton	
	bl	H5400_CheckActionButton	
	bl	H1900_CheckActionButton
	b	MaybeForceBootldrStart
generic_power_button_test:		
	/* otherwise try a generic check */
	/* lets try the power button     */
	ldr	r1, GPIO_BASE
	add	r1,r1,#__OFFSET(GPLR0)
	ldr	r0,[r1]
	tst	r0, #0x1
	bne	1f //; assume 0 means button is down
	bic	r12, r12, #BF_NORMAL_BOOT	
1:		
MaybeForceBootldrStart:		
	tst	r12, #BF_NORMAL_BOOT
	beq	BootldrStartForced  //; pressed -> Z high so skip this

//;	Check for a wince img and jump to it if we find one.
//;	otherwise we write stuff in ram and bad shit happens.
CheckForWince:
	mov	r0,#FLASH_BASE
	add	r0,r0,#0x40000
//; look for wince image	
	ldr	r1,[r0],#4
	ldr	r2,WINCE_MAGIC_1
	cmp	r1,r2
	bne	noWince
//;	ok, some wince images have 0's where they are erased and
//;	others have F's
	ldr	r3,WINCE_MAGIC_LONG_0
wince_loop:	
	ldr	r1,[r0],#4
	cmp	r1,#0
	bne	10f
	b	20f
10:	
	cmp	r1,#0xFFFFFFFF
	bne	noWince
20:	
	subs	r3,r3,#1
	bne	wince_loop	

	ldr	r1,[r0],#4
	ldr	r2,WINCE_MAGIC_2
	cmp	r1,r2
	bne	noWince

WakeUpWince:
	@ now jump to wince
	ldr	r1,wince_jp
	nop
	nop
	nop
	nop
	mov	pc,r1

noWince:
	//; check to see whether this was a wake up from sleep
        ldr     r1, PMCR_BASE
        ldr     r0, [r1, #__OFFSET(RCSR)]
        and     r0, r0, #(RCSR_HWR + RCSR_GPR + RCSR_WDR + RCSR_SMR)

	//; clear whatever status we found
	str	r0, [r1, #__OFFSET(RCSR)]

        teq     r0, #RCSR_SMR
        beq     WakeupStart

	b	BootldrStart

BootldrStartForced:
	//; we got here cuz they held the action button dowm,
	//; therefore no squelchies
#if defined(CONFIG_MACH_H3900) || defined(CONFIG_MACH_H5400)
	bic	r12, r12, #BF_SQUELCH_SERIAL
#endif	
	bic     r12, r12, #BF_NORMAL_BOOT
BootldrStart:
        bl      H3900_RS232_On 
        bl      InitUARTFF

	mov	r0, #'{'
	ldr     r1, _C_LABEL(SerBase)
        bl      PrintChar
	
	mov	r0, r12
	ldr     r1, _C_LABEL(SerBase)
        bl      PrintHexWord
	
	ldr	r0, H3900_ASIC3_BASE
	add	r0, r0, #0x00001000
	ldrh	r0, [r0, #0]
	ldr     r1, _C_LABEL(SerBase)
        bl      PrintHexWord
	
	ldr	r0, H3900_ASIC3_BASE
	add	r0, r0, #0x00001000
	ldrh	r0, [r0, #4]
	ldr     r1, _C_LABEL(SerBase)
        bl      PrintHexWord
	
	mov	r0, #'}'
	ldr     r1, _C_LABEL(SerBase)
        bl      PrintChar
	
	/* check to see if we're operating out of DRAM */
        bic     r4, pc, #0x000000FF
        bic     r4, r4, #0x0000FF00
        bic     r4, r4, #0x00FF0000	
        cmp     r4, #DRAM_BASE0
	bne     RunningInFlash

	mov	r0, r12
	ldr     r1, _C_LABEL(SerBase)
        bl      PrintHexWord
	
	mov     r0, pc
	ldr     r1, _C_LABEL(SerBase)
        bl      PrintHexWord
	
RunningInDRAM:  
	mov     r0, #'D'
	ldr     r1, _C_LABEL(SerBase)
	bl      _C_FUNC(PrintChar)
	mov     r0, #'\r'
	ldr     r1, _C_LABEL(SerBase)
	bl      _C_FUNC(PrintChar)
	mov     r0, #'\n'
	ldr     r1, _C_LABEL(SerBase)
	bl      _C_FUNC(PrintChar)
	//; no low level setup needed :	)
	b	HANG
		
RunningInFlash: 	
	mov     r0, #'F'
	ldr     r1, _C_LABEL(SerBase)
	bl      _C_FUNC(PrintChar)
	mov     r0, #'\r'
	ldr     r1, _C_LABEL(SerBase)
	bl      _C_FUNC(PrintChar)
	mov     r0, #'\n'
	ldr     r1, _C_LABEL(SerBase)
	bl      _C_FUNC(PrintChar)

MAIN_SETUP:	
	bl	SetupClocks
	bl	SetupMemory
	bl	TestMemory
	ldr	r0,STR_DUCK
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord
	
        bl      MaybeCopyToDRAM
	
HANG:	

		/*; Get ready to call C functions*/
	ldr	r0, STR_STACK
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord
	ldr	r0, DW_STACK_START
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
	
CALL_MAIN:      
        //; pass boot flags as second arg                 
	orr     a2, r12, r12
	ldr	sp,DW_STACK_START
	mov	fp,#0		/* no previous frame, so fp=0*/

	/*
	 * check to see if this is a sleep reset.
	 * if so, then we want to enter the bootmenu directly.
	 * otherwise, we want to proceed as before, allowing 
	 * a key or timeout to control the boot action
	 */	 		
	tst	a2, #BF_SLEEP_RESET
	movne	a1, #1
	moveq	a1, #0
	
	bl	_C_FUNC(bootldr_main)	/* call the main C entry point */
	nop
	nop
	nop
	nop
	mov	pc, #FLASH_BASE	/* reboot */
	b	HANG



WakeupStart:
        ldr     r1, CLOCK_BASE
        mov	r0, #CKEN6_FFUART
        str     r0, [r1,#__OFFSET(CKEN)]		//; re-enable UART clock
	
	bl	InitUARTFF

	# get SDRAM going again
	bl	SetupMemory

	# retrieve the scratchpad value and jump to that address
	ldr     r7, PMCR_BASE
        ldr     r6, [r7, #__OFFSET(PSPR)]
	mov     pc, r6
	
/*	;; ********************************************************************
	;; SetupClocks -- inits the clock registers
	;; 
        ;;   modifies r0,r1 
	;;   
	;; ********************************************************************
*/	
SetupClocks:
	//; this initializes the clocks to valid values 
	ldr	r1,CLOCK_BASE
	ldr	r0,CCCR_INIT
	str	r0,[r1,#__OFFSET(CCCR)]
	ldr	r0,CKEN_INIT
	str	r0,[r1,#__OFFSET(CKEN)]
	ldr	r0,OSCC_INIT
	str	r0,[r1,#__OFFSET(OSCC)]
	
	mrc	p14, 0, r0, c6, c0, 0
	orr	r0, r0, #3			//; select turbo mode, start FCS
	mcr	p14, 0, r0, c6, c0, 0
	mov	pc, lr	
	
/*	;; ********************************************************************
	;; SetupMemory -- inits the SDRAM and MSC registers
	;; 
        ;;   modifies r0,r1 
	;;   
	;; ********************************************************************
*/
SetupMemory:
	mov	r6,lr

	//; we are following the sequence in the os developers guide pg 13-14
	//;Step 0:	 print out step 0
	ldr	r0,STR_STEP
	ldr	r3,=0xffffff00
	and	r0,r0,r3
	orr	r0,r0,#'0'
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord
	
	//; Step 1:	 wait 200 usec for the internal clocks to stabilize.
	ldr	r0,OSCR_BASE
	mov	r1,#0
	str	r1,[r0,#__OFFSET(OSCR)]
	ldr	r3,=0x300 //; 200 usec is x2e1 so this shouldbe fine
10:	
	ldr	r2,[r0,#__OFFSET(OSCR)]
	cmp	r3,r2
	bgt	10b

	ldr	r0,STR_STEP
	ldr	r3,=0xffffff00
	and	r0,r0,r3
	orr	r0,r0,#'1'
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord
	
			
	//; Step 2:	Set up the MSC registers	
	ldr	r0,MEM_BASE
	ldr	r1,MSC0_INIT
	str	r1,[r0,#__OFFSET(MSC0)] //; spec says to ldr the mscs after a set
	ldr	r2,[r0,#__OFFSET(MSC0)]

#if 0	//; not clear what the right value for this would be, or if it even matters
	ldr	r1,MSC1_INIT
	str	r1,[r0,#__OFFSET(MSC1)] //; spec says to ldr the mscs after a set
	ldr	r2,[r0,#__OFFSET(MSC1)]
#endif
	ldr	r0,MEM_BASE
	ldr	r1,MSC2_INIT
	str	r1,[r0,#__OFFSET(MSC2)] //; spec says to ldr the mscs after a set
	ldr	r2,[r0,#__OFFSET(MSC2)]


	
	ldr	r0,STR_STEP
	ldr	r3,=0xffffff00
	and	r0,r0,r3
	orr	r0,r0,#'2'
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord
	
	
	
	//; Step 3:	configure the PCMCIA card using  mecr, mcmem0,mcatt0,mcatt1,mcio0,mcio133 
	//; we'll do this later....
	ldr	r0,STR_STEP
	ldr	r3,=0xffffff00
	and	r0,r0,r3
	orr	r0,r0,#'3'
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord

	//; step 4:  configure FLYCNFG -- a fly by dma register that has apparently disappeared...
	ldr	r0,STR_STEP
	ldr	r3,=0xffffff00
	and	r0,r0,r3
	orr	r0,r0,#'4'
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord

	//; Step 5:	initial configure of mdrefr k0run,e0pin,k0db2,kxfree off,apd off, slfrefresh must stay on !
	ldr	r0,MEM_BASE
	ldr	r1,MDREFR_STEP_5_INIT
	tst	r12, #BF_H5400
	beq	1f
	ldr	r1,MDREFR_STEP_5_INIT_H5400
1:	str	r1,[r0,#__OFFSET(MDREFR)]

	ldr	r0,STR_STEP
	ldr	r3,=0xffffff00
	and	r0,r0,r3
	orr	r0,r0,#'5'
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord

	//;Step 6:	synchronous static memory -- we dont have any
	ldr	r0,MEM_BASE
	mov	r1,#0
	str	r1,[r0,#__OFFSET(SXCNFG)]

	ldr	r0,STR_STEP
	ldr	r3,=0xffffff00
	and	r0,r0,r3
	orr	r0,r0,#'6'
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord
		
	//;Step 7:	SDRAM again
	ldr	r0,MEM_BASE
	//; MDREFR self refresh turned off
	ldr	r1,[r0,#__OFFSET(MDREFR)]
	orr	r1,r1,#MDREFR_K1RUN
	orr	r1,r1,#MDREFR_K2RUN
	bic	r1,r1,#MDREFR_SLFRSH
	orr	r1,r1,#MDREFR_E1PIN
	str	r1,[r0,#__OFFSET(MDREFR)]
	//; MDCONFIG setup up but not enabled							
	ldr	r1,MDCNFG_STEP_7_INIT
	tst	r12, #BF_H5400
	beq	1f
	ldr	r1,MDCNFG_STEP_7_INIT_H5400
1:	str	r1,[r0,#__OFFSET(MDCNFG)]
	
	//;Step 8:	wait for the sdclocks to stabilize (200 usec)
	ldr	r0,OSCR_BASE
	mov	r1,#0
	str	r1,[r0,#__OFFSET(OSCR)]
	ldr	r3,=0x300 //; 200 usec is x2e1 so this shouldbe fine
20:	
	ldr	r2,[r0,#__OFFSET(OSCR)]
	cmp	r3,r2
	bgt	20b

	//;Step 9:	trigger refresshes by reading sdram
	mov	r0,#0xa0000000
	ldr	r3,=0x200
	mov	r2,#0
20:
	ldr	r1,[r0]
	add	r2,r2,#1
	cmp	r3,r2
	bgt	20b

	//;Step 10:	enable the sdram in the mdconfig register
	ldr	r0,MEM_BASE
	ldr	r1,[r0,#__OFFSET(MDCNFG)]
	orr	r1,r1,#MDCNFG_DE0
	orr	r1,r1,#MDCNFG_DE1
	str	r1,[r0,#__OFFSET(MDCNFG)]

	//;Step 11:	//; write out the register mode offset value to the sdram
	ldr	r0,MEM_BASE
	mov	r1,#0
	str	r1,[r0,#__OFFSET(MDMRS)]		
	mov	pc, r6


/*	;; ********************************************************************
	;; TestMemory -- do we have dram?
	;; 
        ;;   modifies r0,r1,r2,r3,r4,
	;;
	;;	r5 -- dram_ptr
	;;	r6 lr
	;;	r7 -- when to print ptr
	;;	r8 -- value
	;;	r9 -- saved value
	;;	r10 -- loop end
	;;	r11 cmp value
	;;   
	;; ********************************************************************
*/	
TestMemory:
	mov	r6,lr
	ldr     r0, STR_MTST
	ldr     r1, _C_LABEL(SerBase)
	bl      PrintWord

TestDramBank0:       	
	/* some test code */
	mov	r5, #DRAM_BASE0
	mov	r7, #0x00000001
TestDramBank0Loop:
	mov     r0, r7
	ldr	r2,[r5, #48]
	str	r0, [r5, #48]
	ldr	r0, [r5, #48]
	str	r2, [r5, #48]	
	ldr     r1, _C_LABEL(SerBase)
	bl      PrintHexWord
	tst     r7, #0x80000000
	bne     TestDramBank0Done
	mov     r7, r7, lsl #1
        b       TestDramBank0Loop
TestDramBank0Done:

		
	//; randomish number
	mov	r4,#0xa0000000
	ldr	r5,[r4]
	ldr	r1,DEADBEEF_S
	str	r1,[r4]
	mov	r0,r4
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
	mov	r4,#0xa0000000
	ldr	r0,[r4]
	ldr     r1,_C_LABEL(SerBase) 
	bl	PrintHexWord
	mov	r4,#0xa0000000
	str	r5,[r4]
	
		
	ldr	r0,MEM_BASE
	add	r0,r0,#__OFFSET(MDCNFG)
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
	ldr	r4,MEM_BASE
	ldr	r0,[r4]
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord

	ldr	r0,MEM_BASE
	add	r0,r0,#__OFFSET(MDREFR)
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
	ldr	r4,MEM_BASE
	ldr	r0,[r4,#__OFFSET(MDREFR)]
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord

	ldr	r0,MEM_BASE
	add	r0,r0,#__OFFSET(MSC0)
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
	ldr	r4,MEM_BASE
	ldr	r0,[r4,#__OFFSET(MSC0)]
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
	
	ldr	r0,MEM_BASE
	add	r0,r0,#__OFFSET(MSC1)
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
	ldr	r4,MEM_BASE
	ldr	r0,[r4,#__OFFSET(MSC1)]
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord

	ldr	r0,MEM_BASE
	add	r0,r0,#__OFFSET(MSC2)
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
	ldr	r4,MEM_BASE
	ldr	r0,[r4,#__OFFSET(MSC2)]
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord

	/*; debug print*/
DebugPrintAfterTestDram:        	
	ldr	r0, STR_ENDM
	ldr     r1, _C_LABEL(SerBase)
	bl      PrintWord
		
	mov	pc, r6	
		
/************************************************************************************/
/* Copy to dram if link address is not same as flash address */
/************************************************************************************/
MaybeCopyToDRAM:
        mov     r6, lr        
        mov     r4, #FLASH_BASE
        ldr     r5, _bootldr_start
        ldr     r7, _bootldr_end
	
        cmp     r4, r5
        beq     nocpy
	
	ldr	r0,STR_RELO
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintWord
		
copy:   	
	mov	r9, #FLASH_BASE
	/* r9 has flash base */
	/* r5 has link-time start address of bootldr */
	/* r7 gets link-time end address of bootldr */
1:      ldr     r8, [r9]
        str     r8, [r5]
	
	add     r9, r9, #4 
	add     r5, r5, #4 
        cmp     r5, r7
        blt     1b
	
	/* adjust the link register */
        mov     r4, #FLASH_BASE
        ldr     r5, _bootldr_start
        sub     r3, r5, r4
        add     r6, r6, r3
	
        mov     lr, r6	
	
nocpy:  mov     pc, lr
/************************************************************************************/

/************************************************************************************
	;;  Printing Functions
	;; 
/************************************************************************************
	/*
	;; ********************************************************************
	;; InitUARTFF - Initialize Serial Communications
	;; ********************************************************************
	;; Following reset, the UART is disabled. So, we do the following:
	;; 1) set the uart gpio lines in their alternate reg state
	;; 2) set the right baud rate (115200) and set to 8 bit/char mode
	;; 3) enable the fifos 
	;; 4) turn on the uart
	*/
InitUARTFF:
	mov	r6,lr
	ldr	r1,FFUART_BASE
	//; disable the ffuart just in case
	add	r2,r1,#__OFFSET(FFIER)
	mov	r0,#0x0
	str	r0,[r2]

#if 1					//; is redundant on h3900 and h5400
	tst	r12, #(BF_H3900|BF_H5400)
	bne	1f
	//; GPIOS
	ldr	r1,GPIO_BASE 
	//; the direction register
	add	r2,r1,#__OFFSET(GPDR1)
	ldr	r0,GPDR1_S //; settings
	str	r0,[r2]
	//; alt fxn register for gpio 32-47
	add	r2,r1,#__OFFSET(GAFR1_L)
	ldr	r0,GAFR1_L_S //; settings
	str	r0,[r2]

	//; let GPIOS be GPIOS
	ldr	r1,PMCR_BASE
	add	r2,r1,#__OFFSET(PSSR)
	mov	r0,#0x20
	str	r0,[r2]
1:	
#endif
	 		
	/* now we start setting up the uart  */
	//; lets set up the baud rate 
	//; baud rate = 14.7456 * 10^6 / (16 * x) = 921600/x
	//; 115200 => x = 8
	ldr	r1,FFUART_BASE
	//; get acc3ess to baud control registers
	add	r2,r1,#__OFFSET(FFLCR)
	mov	r0,#LCR_DLAB
	str	r0,[r2]
	//; set the baud rate
	add	r2,r1,#__OFFSET(FFDLL)
	mov	r0,#0x8
	str	r0,[r2]
	add	r2,r1,#__OFFSET(FFDLH)
	mov	r0,#0x0
	str	r0,[r2]
	//; clear the latch bit and set to 8 bit per character mode
	add	r2,r1,#__OFFSET(FFLCR)
	mov	r0,#LCR_WLS1
	orr	r0,r0,#LCR_WLS0
	str	r0,[r2]
	//; turn off modem stuff
	//;add	r2,r1,#__OFFSET(FFMCR)
	//;mov	r0,#0
	//;str	r0,[r2]
	//; enable the fifos  and reset them for good measure
#if 1
	add	r2,r1,#__OFFSET(FFFCR)
	mov	r0,#FCR_TRFIFOE
	//;orr	r0,r0,#FCR_ITL_32
	orr	r0,r0,#FCR_RESETTF
	orr	r0,r0,#FCR_RESETRF
	str	r0,[r2]
#endif
				

InitUartFFEnable:
	//; enable the uart		
	ldr	r1,FFUART_BASE
	add	r2,r1,#__OFFSET(FFIER)
	mov	r0,#IER_UUE
	str	r0,[r2]

	tst	r12, #BF_SQUELCH_SERIAL
	bne	10f
	
        /* transmit a character or two */
	mov	r2, #'U'
	str	r2, [r1, #__OFFSET(FFTHR)]
	mov	r2, #'F'
	str	r2, [r1, #__OFFSET(FFTHR)]
	mov	r2, #'F'
	str	r2, [r1, #__OFFSET(FFTHR)]
	mov	r2, #'\r'
	str	r2, [r1, #__OFFSET(FFTHR)]
	mov	r2, #'\n'
	str	r2, [r1, #__OFFSET(FFTHR)]


#if 0
	ldr	r0,FFUART_BASE
	add	r0,r0,#__OFFSET(FFLCR)
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
	ldr	r4,FFUART_BASE
	ldr	r0,[r4,#__OFFSET(FFLCR)]
	ldr     r1,_C_LABEL(SerBase)
	bl	PrintHexWord
#endif

	
10:		
	mov	pc, r6		/* All done, return*/

/*
	;; ********************************************************************
	;; PrintHexNibble -- prints the least-significant nibble in R0 as a
	;; hex digit
        ;;   r0 contains nibble to write as Hex
        ;;   r1 contains base of serial port
        ;;   writes r0 with RXSTAT, modifies r0,r2 
	;;   Falls through to PrintChar
	;; ********************************************************************
	*/
PrintHexNibble:	
	adr	r2, _C_LABEL(HEX_TO_ASCII_TABLE)
	and	r0, r0, #0xF
	ldr	r0, [r2,r0]	/* convert to ascii */
	b       _C_FUNC(PrintChar)

	/*
	;; ********************************************************************
	;; PrintChar -- prints the character in R0
        ;;   r0 contains the character
        ;;   r1 contains base of serial port
        ;;   writes r0 with UTSR1, modifies r0,r1,r2
	;;********************************************************************
	*/

PrintChar:      
	# see if printing is disabled
	tst	r12, #BF_SQUELCH_SERIAL
	movne	pc, lr		/* return if squelched */

TXBusy:	
        ldr     r2,[r1,#__OFFSET(FFLSR)]/* check status*/
	tst     r2,#LSR_TEMT /* TX empty */
        beq     TXBusy
	str	r0,[r1,# __OFFSET(FFTHR)]
	ldr     r0,[r1,#__OFFSET(FFLSR)]
        mov     pc, lr
	
	/*
        ;; ********************************************************************
	;; PrintWord -- prints the 4 characters in R0
        ;;   r0 contains the binary word
        ;;   r1 contains the base of the serial por
        ;;   writes r0 with RXSTAT, modifies r1,r2,r3,r4
	;; ********************************************************************
	*/
PrintWord:	
	mov	r3, r0
	mov     r4, lr
	bl      _C_FUNC(PrintChar)     
	
	mov     r0,r3,LSR #8    /* shift word right 8 bits*/
	bl      _C_FUNC(PrintChar)
	
	mov     r0,r3,LSR #16   /* shift word right 16 bits*/
	bl      _C_FUNC(PrintChar)
	
	mov     r0,r3,LSR #24   /* shift word right 24 bits*/
	bl      _C_FUNC(PrintChar)

	mov	r0, #'\r'
	bl	_C_FUNC(PrintChar)

	mov	r0, #'\n'
	bl	_C_FUNC(PrintChar)
	
        mov     pc, r4

	/*
        ;; ********************************************************************
	;; PrintHexWord -- prints the 4 bytes in R0 as 8 hex ascii characters
	;;   followed by a newline
        ;;   r0 contains the binary word
        ;;   r1 contains the base of the serial port
        ;;   Writes r0 with RXSTAT, modifies r1,r2,r3,r4
ENTRY(PrintHex)
	;; ********************************************************************
	*/
PrintHexWord:	
	mov     r4, lr
	mov	r3, r0
	mov	r0, r3,LSR #28
	bl      PrintHexNibble     
	mov	r0, r3,LSR #24
	bl      PrintHexNibble     
	mov	r0, r3,LSR #20
	bl      PrintHexNibble     
	mov	r0, r3,LSR #16
	bl      PrintHexNibble     
	mov	r0, r3,LSR #12
	bl      PrintHexNibble     
	mov	r0, r3,LSR #8
	bl      PrintHexNibble     
	mov	r0, r3,LSR #4
	bl      PrintHexNibble     
	mov	r0, r3
	bl      PrintHexNibble     

	mov	r0, #'\r'
	bl	_C_FUNC(PrintChar)

	mov	r0, #'\n'
	bl	_C_FUNC(PrintChar)

        mov     pc, r4



	
//; Utility fxns
/*
	;; ********************************************************************
	;; bootLinux - boots into another image. DOES NOT RETURN!
	;; r0 = must contain a zero or else the kernel loops
        ;; r1 = architecture type (6 for old kernels, 17 for new)
	;; r2 = entry point of kernel
	;; ********************************************************************
*/
ENTRY(bootLinux)
	mov     r10, r1
	mov     r11, r2
	/*
        ;; flush the I+D TLB
        ;;  c8 is TLB operation register
        ;;  crm=c7 opc2==0 indicates Flush I+D TLB
        ;;  r0 is ignored
	*/

	// clean data cache	
	mov	r0, #0xa0000000
	mov	ip, #1024
2:
	mcr	p15, 0, r0, c7, c2, 5		// allocate line
	add	r0, r0, #32
	subs	ip, ip, #1
	bne	2b
	
	mcr     p15, 0, r0, c7, c10, 4 		// drain WB

	mcr	p15, 0, r0, c8, c7, 0x00 	/* invalidate I+D TLB */
	cpwait  r0

	mov     r3, #0x120 			// xscale says p needs to be 0 ???
	mcr     p15, 0, r3, c1, c0, 0   	// disable mmu
	cpwait  r0

        // be sure pipeline is clear
        mov     r0,r0
        mov     r0,r0
        mov     r0,r0

	// flush Dcache
	mcr	p15, 0, r0, c7, c6, 0

	// flush Icache and BTB
	mcr     p15, 0, r0, c7, c5, 0
	
	/* zero PID in Process ID Virtual Address Map register. */
	mov	r0, #0
	mcr	p15, 0, r0, c13, c0, 0
	cpwait  r0	

	mov     r0, #0
        mov     r1, r10
	ldr     r2, [r2, #0]
	mov     r2, #0
	mov	pc, r11		/* jump to addr. bootloader is done*/


	/*
	;; ********************************************************************
	;; boot - boots into another image. DOES NOT RETURN!
	;; r0 = pointer to bootinfo
	;; r1 = entry point of kernel
	;; ********************************************************************

	//; This fxn does not turn off the MMU prior to jumping!  seems like a bug to me  (bavery) !!!
		
	*/
ENTRY(boot)
	/*
        ;; flush the I+D TLB
        ;;  c8 is TLB operation register
        ;;  crm=c7 opc2==0 indicates Flush I+D TLB
        ;;  r0 is ignored
	*/
	mcr	p15, 0, r0, c8, c7, 0x00	/* invalidate I+D TLB */
        mcr     p15, 0, r0, c7, c10, 4 /* drain the write buffer*/

        /*; make sure the pipeline is emptied*/
        mov     r0,r0
        mov     r0,r0
        mov     r0,r0
        mov     r0,r0

	mov	pc,r1		/* jump to addr. bootloader is done*/


ENTRY(asmEnableCaches)
	/*
	;; params:	 r0 = dcache, r1 = icache
	/*

	/*; return:	 r0,r1,r2,r3  trashed. */

	mov	r3,#0
	orr	r3,r0,r3
	orr	r3,r3,r1,lsl #1

	/* get a picture of the current cache situation */
	mrc	p15, 0, r2, c1, c0, 0  /* control address */

	/* set dcache, clear it if requested */
	tst	r3,#(1 << 0)
	orrne	r2,r2,#0x04
	biceq	r2,r2,#0x04 /* bit 2 is dcache */

	// clean data cache	
	mov	r0, #0xa0000000
	mov	ip, #1024
2:
	mcr	p15, 0, r0, c7, c2, 5		// allocate line
	add	r0, r0, #32
	subs	ip, ip, #1
	bne	2b
	
	// now flush the dcache
	mcr p15,0,r0,c7,c6,0

	// also the tlb dcache entries
	mcr p15,0,r0,c8,c6,0

	/* set icache, clear it if requested */
	tst	r3,#(1<<1)
	orrne	r2,r2,#0x1000
	biceq	r2,r2,#0x1000  /* bit 12 is icache */

	// flush icache
	mcr	p15, 0, r0, c7,c5,0

	// also the tlb icache entries
	mcr p15,0,r0,c8,c5,0

	/* store the new values */	
	mcr	p15, 0, r2, c1, c0, 0
	mov	pc,lr
	
ENTRY(readPC)
        mov     r0,pc
        mov     pc,lr
		
ENTRY(readCPR0)
        mrc     p15,0,r0,c0,c0,0
        mov     pc,lr
		
ENTRY(readCPR1)
        mrc     p15,0,r0,c1,c0,0
        mov     pc,lr

ENTRY(readCPR2)
        mrc     p15,0,r0,c2,c0,0
        mov     pc,lr

ENTRY(readCPR3)
        mrc     p15,0,r0,c3,c0,0
        mov     pc,lr

/* no CPR4 */	

ENTRY(readCPR5)
        mrc     p15,0,r0,c5,c0,0
        mov     pc,lr

ENTRY(readCPR6)
        mrc     p15,0,r0,c6,c0,0
        mov     pc,lr
	
ENTRY(readCPR7)
        mrc     p15,0,r0,c7,c0,0
        mov     pc,lr
	
ENTRY(readCPR8)
        mrc     p15,0,r0,c8,c0,0
        mov     pc,lr
	
ENTRY(readCPR9)
        mrc     p15,0,r0,c9,c0,0
        mov     pc,lr
	
ENTRY(readCPR10)
        mrc     p15,0,r0,c10,c0,0
        mov     pc,lr
	
/* no 11 or 12 */
	
ENTRY(readCPR13)
        mrc     p15,0,r0,c13,c0,0
        mov     pc,lr
		
ENTRY(readCPR14)
        mrc     p15,0,r0,c14,c0,0
        mov     pc,lr
	
ENTRY(readCPR15)
        mrc     p15,0,r0,c15,c0,0
        mov     pc,lr
	

ENTRY(writeBackDcache)
	/*
	;; r0 points to the start of a 16384 byte region of readable 
	;; data used only for this cache flushingroutine. If this area 
	;; is to be used by other code, then 32K must be loaded and the
	;; flush mcr is not needed.
	/*

	/*; return:	 r0,r1,r2 trashed. data cache is clean*/
	add r1,r0,#32768
l1:	
	ldr r2,[r0],#32
	teq r1, r0
	bne l1
	mcr p15,0,r0,c7,c6,0
	mov pc,lr


ENTRY(flushTLB)
		/*
	;; flush the I/D caches
        ;;  c7 == cache control operation register
        ;;  crm==c7 opc2==0 indicates Flush I+D Cache
        ;;  r0 is ignored
	*/
	mcr	p15, 0, r0, c7, c7, 0x00	@ invalidate I, D caches & BTB
	/*
        ;; flush the I+D TLB
        ;;  c8 is TLB operation register
        ;;  crm=c7 opc2==0 indicates Flush I+D TLB
        ;;  r0 is ignored
	*/
	mcr	p15, 0, r0, c8, c7, 0x00        @ invalidate I & D TLBs

	mcr	p15, 0, r0, c7, c5, 0           @ invalidate I cache & BTB
	cpwait  r0     

        @ invalidate all tlb entries 
	mcr	p15, 0, ip, c7, c10, 4		@ Drain Write (& Fill) Buffer
	mcr	p15, 0, ip, c8, c7, 0		@ invalidate I & D TLBs
/*;; return*/
	mov	pc,lr		

ENTRY(enableMMU)
	/*; r0 = translation table base*/
	ldr	r0,DW_MMU_TABLE
	mcr	p15, 0, r0, c2, c0, 0

	/*; r0 = domain access control*/
	ldr	r0,MMU_DOMCTRL
	mcr	p15, 0, r0, c3, c0, 0

	/*; enable the MMU*/
	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #1      /* bit 0 of c1 is MMU enable*/
	
	/* make sure Virtual interrupt vector adjust is OFF */
	//;bic	r0, r0, #(1<<13)
	
#ifdef CACHE_ENABLED	
	//;orr	r0, r0, #4      /* bit 2 of c1 is DCACHE enable*/
	//;orr	r0, r0, #8      /* bit 3 of c1 is WB enable*/
#endif	
	mcr	p15, 0, r0, c1, c0, 0
        cpwait  r0
	/*
	;; flush the I/D caches
        ;;  c7 == cache control operation register
        ;;  crm==c7 opc2==0 indicates Flush I+D Cache
        ;;  r0 is ignored
	*/
	//;mcr	p15, 0, r0, c7, c7, 0x00
	/*
        ;; flush the I+D TLB
        ;;  c8 is TLB operation register
        ;;  crm=c7 opc2==0 indicates Flush I+D TLB
        ;;  r0 is ignored
	*/
	//;mcr	p15, 0, r0, c8, c7, 0x00 	@ invalidate I & D TLBs
	nop
	nop
	nop
	nop
	nop
	nop
	nop	
	/*; return*/
	mov	pc,lr		

	

//;STUBS	
ENTRY(SleepReset_Resume)
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
/*ENTRY(HEX_TO_ASCII_TABLE)*/
	.globl _C_LABEL(HEX_TO_ASCII_TABLE)
_C_LABEL(HEX_TO_ASCII_TABLE):
	.ascii	"0123456789ABCDEF"

	.align	2
STR_STACK:	
	.ascii	"STKP"


	.align	2
DW_STACK_START:	
	.word	STACK_BASE+STACK_SIZE-16
	
	.align	2
DW_MMU_TABLE:	
	.word	MMU_TABLE_START

	.align	2
DW_CACHE_FLUSH_REGION:	
	.word	CACHE_FLUSH_BASE

	.align	2
MMU_DOMCTRL:	
	.word	0xFFFFFFFF


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
STR_PRECACHE:
        .ascii  "PRE$"	
	.align 2
STR_POSTCACHE:
        .ascii  "PST$"	
	.align 2
STR_PREZERO:
        .ascii  "PREZ"	
	.align 2
STR_POSTZERO:
        .ascii  "PSTZ"	
	.align 2
STR_POSTFLUSH:
        .ascii  "FLSH"	
	.align 2
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
STR_NOT_USED:	
        .ascii	"NUSD"

	.align	2
STR_MEM_ERROR:
	.ascii	"DRAM ERROR"

	.align	2
STR_WAKEUP:	
        .ascii	"WKUP"

	.align	2
STR_WAKEUP_R:	
        .ascii	"GOGO"

	.align	2
STR_WAKEUP_SKIP:	
        .ascii	"SKWU"

	.align	2
STR_COWS:	
        .ascii	"COWS"
	
	.align	2
STR_DUCK:	
        .ascii	"DUCK"
	
	.align	2
STR_RELO:	
        .ascii	"RELO"
	
STR_STEP:	
        .ascii	"_STP"

	.globl _C_LABEL(boot_flags_ptr)
_C_LABEL(boot_flags_ptr):
	.word   STACK_BASE+STACK_SIZE-4
		
_C_LABEL(SerBase):
	.long __BASE(FFUART)
			
	.globl _C_FUNC(delayedBreakpointPC)
_C_FUNC(delayedBreakpointPC):
        .long   0

	//; ******************************************************************/
.align 4	
MEM_BASE:	
	.long 	__BASE(MDCNFG)
OSCR_BASE:	
	.long 	__BASE(OSCR)
CLOCK_BASE:	
	.long 	__BASE(CCCR)
GPIO_BASE:
        .long __BASE(GPLR0)
FFUART_BASE:
	.long __BASE(FFUART)
PMCR_BASE:
	.long __BASE(PMCR)
CCCR_INIT:	//; turbo = 2* run = 2* mem = 99.53 Mhz pg 3-33
	.long ((4<<7)|(2<<5)|(1)) 	
CKEN_INIT:	
	.long (CKEN16_LCD | CKEN6_FFUART | CKEN5_STUART)
OSCC_INIT:  //;32 K clock on	
	.long (OSCC_OON)
MDREFR_STEP_5_INIT:  //; os guide pg 13 part 0 running and enabled at full speed refresh rate = 0x18
	.long (MDREFR_K0RUN | MDREFR_E0PIN | MDREFR_SLFRSH | (0x18))
MDREFR_STEP_5_INIT_H5400:  //; os guide pg 13 part 0 running and enabled at full speed refresh rate = 0x18
	.long (MDREFR_K0DB2 | MDREFR_K0RUN | MDREFR_E0PIN | MDREFR_SLFRSH | (0x18))
MDCNFG_STEP_7_INIT:  //; set up but not enabled !!!
	.long (MDCNFG_DCAC0 | (2*MDCNFG_DRAC0) | MDCNFG_DNB0 | MDCNFG_DTC0 | MDCNFG_DLATCH0 |    \
		MDCNFG_DCAC2 | (2*MDCNFG_DRAC2) | MDCNFG_DNB2 | MDCNFG_DTC2 | MDCNFG_DLATCH2)
MDCNFG_STEP_7_INIT_H5400:  //; set up but not enabled !!!
	.long (MDCNFG_DSA1111_0 | MDCNFG_DCAC0 | (2*MDCNFG_DRAC0) | MDCNFG_DNB0 | 2*MDCNFG_DTC0 | MDCNFG_DLATCH0 |    \
		MDCNFG_DCAC2 | (2*MDCNFG_DRAC2) | MDCNFG_DNB2 | 2*MDCNFG_DTC2 | MDCNFG_DLATCH2)

/* ipaq3/h5400	
 * ----Memory Controller----
 *	0x48000000->0x1ac9	MDCNFG
 *	    000 1 1 0 10 |  1 10 01 0 0 1	
 *         (  1*DSA1111 | 1*DLATCH0 | 0*DADDR0 | 2*DTC0 | DNB0 | 2*DRAC0 | 1*DCAC0 | DE0 )
 *	0x48000004->0x020df018	MDREFR
 *          0000 00 1 0 0 0 00 1 1 0 1 | 1 1 1 1 0000 0001 1000
 *          DRI=0x018 E0PIN=1 K0RUN=1 K0DB2=1 E1PIN=1 K1RUN=1 K1DB2=0 K2RUN=1 K2DB2=1 APD=00  	
 *	0x48000008->0x129c24f2
 *	0x4800000c->0x7ff424fa
 *	0x48000010->0x7ff47ff4
 *	0x48000014->0x3
 *	0x48000018->0x0
 *	0x4800001c->0x40004
 *	0x48000020->0x1fe01fe
 *	0x48000024->0x0
 *	0x48000028->0x10504
 *	0x4800002c->0x10504
 *	0x48000030->0x10504
 *	0x48000034->0x10504
 *	0x48000038->0x4715
 *	0x4800003c->0x4715
 *	0x48000040->0x220032
 *	0x48000044->0x8
 */

H5400_Ident:
	//; for now, it's H5400 if H5400 is supported and it's not H3900 (or h1900)
#ifdef CONFIG_MACH_H5400
#warning here	
	tst	r12, #BF_H3900	
	bne	1f
	tst	r12, #BF_H1900
	bne	1f
	orr	r12, r12, #BF_H5400
#endif	
1:	mov	pc, lr	

H5400_InitGPIO:		
	tst	r12, #BF_H5400
	beq	1f
#ifdef CONFIG_MACH_H5400
	/* set up GPIOs for H5400 */
	ldr	r0, GPIO_BASE
	
	//; set up GPIOs for H5400
	ldr	r0, GPIO_BASE
	ldr	r1, GAFR0x_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR0_L)]	
	ldr	r1, GAFR1x_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR0_U)]	
	ldr	r1, GAFR0y_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR1_L)]	
	ldr	r1, GAFR1y_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR1_U)]	
	ldr	r1, GAFR0z_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR2_L)]	
	ldr	r1, GAFR1z_Init_H5400
	str	r1, [r0, #__OFFSET(GAFR2_U)]	

	ldr	r1, GPDRx_Init_H5400
	str	r1, [r0, #__OFFSET(GPDR0)]	
	ldr	r1, GPDRy_Init_H5400
	str	r1, [r0, #__OFFSET(GPDR1)]	
	ldr	r1, GPDRz_Init_H5400
	str	r1, [r0, #__OFFSET(GPDR2)]	

#endif	
1:	mov	pc, lr
	
H5400_CheckActionButton:
	tst	r12, #BF_H5400
	beq	1f
#ifdef CONFIG_MACH_H5400
	bic	r12, r12, #BF_ACTION_BUTTON
	orr	r12, r12, #BF_NORMAL_BOOT

	ldr	r0, GPIO_BASE
	ldr	r1, [r0, #__OFFSET(GPLR0)]
	tst	r1, #1 << 4
	orreq	r12, r12, #BF_ACTION_BUTTON		// button was pressed
	biceq	r12, r12, #BF_NORMAL_BOOT
	
	orr	r12, r12, #BF_BUTTON_CHECKED
#endif
1:	mov	pc, lr

#if 1
GAFR0x_Init_H5400:	.long 0x80000001
GAFR1x_Init_H5400:	.long 0x00000010
GAFR0y_Init_H5400:	.long 0x999A8558
GAFR1y_Init_H5400:	.long 0xAAA5AAAA
GAFR0z_Init_H5400:	.long 0xAAAAAAAA
GAFR1z_Init_H5400:	.long 0x00000002
GPDRx_Init_H5400:	.long  0xd3e3b940
GPDRy_Init_H5400:	.long  0x7cffab83
GPDRz_Init_H5400:	.long  0x0001ebdf
GPSRx_Init_H5400:	.long  0xD320A800 /* was 0x00200000 */
GPSRy_Init_H5400:	.long  0x64FFAB83 /* was 0x00000000 */
GPSRz_Init_H5400:	.long  0x0001C8C9 /* was 0x00000800 */
GPCRx_Init_H5400:	.long  0x00C31140 /* was 0x00000000 */
GPCRy_Init_H5400:	.long  0x98000000 /* was 0x00000000 */
GPCRz_Init_H5400:	.long  0x00002336 /* was 0x00000000 */
#endif	
	

H1900_Ident:
	//; we detect h1900 by asic3 being at 0x0C001000
	ldr	r0, H1900_ASIC3_BASE
	add	r0, r0, #0x00001000
	ldrh	r1, [r0, #0]
	ldr	r2, ASIC3_PART_NUMBER_1
	cmp	r1, r2
	bne	1f
	ldrh	r1, [r0, #4]
	ldr	r2, ASIC3_PART_NUMBER_2
	cmp	r1, r2
	bne	1f
	orr	r12, r12, #BF_H1900
1:	mov pc, lr
H1900_ASIC3_BASE:
	.long	0x0C000000

H1900_CheckActionButton:
	tst	r12, #BF_H1900
	beq	1f
	bic	r12, r12, #BF_ACTION_BUTTON
	orr	r12, r12, #BF_NORMAL_BOOT

	ldr	r0, GPIO_BASE
	ldr	r1, [r0, #__OFFSET(GPLR0)]
	tst	r1, #1 << 21
	orreq	r12, r12, #BF_ACTION_BUTTON		// button was pressed
	biceq	r12, r12, #BF_NORMAL_BOOT
	
	orr	r12, r12, #BF_BUTTON_CHECKED
1:	mov	pc, lr

H1900_InitGPIO:		
	tst	r12, #BF_H1900
	beq	1f
	/* set up GPIOs for H1900 */
	ldr	r0, GPIO_BASE
	
	ldr	r0, GPIO_BASE
	ldr	r1, GAFR0x_Init_H1900
	str	r1, [r0, #__OFFSET(GAFR0_L)]	
	ldr	r1, GAFR1x_Init_H1900
	str	r1, [r0, #__OFFSET(GAFR0_U)]	
	ldr	r1, GAFR0y_Init_H1900
	str	r1, [r0, #__OFFSET(GAFR1_L)]	
	ldr	r1, GAFR1y_Init_H1900
	str	r1, [r0, #__OFFSET(GAFR1_U)]	
	ldr	r1, GAFR0z_Init_H1900
	str	r1, [r0, #__OFFSET(GAFR2_L)]	
	ldr	r1, GAFR1z_Init_H1900
	str	r1, [r0, #__OFFSET(GAFR2_U)]	

	ldr	r1, GPDRx_Init_H1900
	str	r1, [r0, #__OFFSET(GPDR0)]	
	ldr	r1, GPDRy_Init_H1900
	str	r1, [r0, #__OFFSET(GPDR1)]	
	ldr	r1, GPDRz_Init_H1900
	str	r1, [r0, #__OFFSET(GPDR2)]	
1:	mov	pc, lr

GAFR0x_Init_H1900:	.long 0x00000004 
GAFR1x_Init_H1900:	.long 0x591a8012
GAFR0y_Init_H1900:	.long 0x60000009
GAFR1y_Init_H1900:	.long 0xAAA10008
GAFR0z_Init_H1900:	.long 0xAAAAAAAA
GAFR1z_Init_H1900:	.long 0x00000000
GPDRx_Init_H1900:	.long  0xd3d39000
GPDRy_Init_H1900:	.long  0xfc7785df
GPDRz_Init_H1900:	.long  0x0001ffff
GPSRx_Init_H1900:	.long  0x00200000 /* was 0x00200000 */
GPSRy_Init_H1900:	.long  0x01CB7916 /* was 0x00000000 */
GPSRz_Init_H1900:	.long  0x00000800 /* was 0x00000800 */
GPCRx_Init_H1900:	.long  0x00000000 /* was 0x00000000 */
GPCRy_Init_H1900:	.long  0xFE3486E9 /* was 0x00000000 */
GPCRz_Init_H1900:	.long  0x00000000 /* was 0x00000000 */


H3900_Ident:		
	//; h3900 has asic3 part number at 0x14801000
	//; now that gpios and MSC2 are configured, we can probe asic3
	ldr	r0, H3900_ASIC3_BASE
	add	r0, r0, #0x00001000
	ldrh	r1, [r0, #0]
	ldrh	r2, ASIC3_PART_NUMBER_1
	cmp	r1, r2
	bne	1f
	ldrh	r1, [r0, #4]
	ldrh	r2, ASIC3_PART_NUMBER_2
	cmp	r1, r2
	bne	1f
	orr	r12, r12, #BF_H3900
1:	mov	pc, lr	
ASIC3_PART_NUMBER_1:	
	.long	0x4854
ASIC3_PART_NUMBER_2:	
	.long	0x432d
	
H3900_InitGPIO:		
	tst	r12, #BF_H3900
	beq	1f
	//; set up GPIOs for H3900
	ldr	r0, GPIO_BASE
	ldr	r1, GAFR0x_Init_H3900
	str	r1, [r0, #__OFFSET(GAFR0_L)]	
	ldr	r1, GAFR1x_Init_H3900
	str	r1, [r0, #__OFFSET(GAFR0_U)]	
	ldr	r1, GAFR0y_Init_H3900
	str	r1, [r0, #__OFFSET(GAFR1_L)]	
	ldr	r1, GAFR1y_Init_H3900
	str	r1, [r0, #__OFFSET(GAFR1_U)]	
	ldr	r1, GAFR0z_Init_H3900
	str	r1, [r0, #__OFFSET(GAFR2_L)]	
	ldr	r1, GAFR1z_Init_H3900
	str	r1, [r0, #__OFFSET(GAFR2_U)]	

	ldr	r1, GPDRx_Init_H3900
	str	r1, [r0, #__OFFSET(GPDR0)]	
	ldr	r1, GPDRy_Init_H3900
	str	r1, [r0, #__OFFSET(GPDR1)]	
	ldr	r1, GPDRz_Init_H3900
	str	r1, [r0, #__OFFSET(GPDR2)]	
1:	mov	pc, lr
	
H3900_CheckActionButton:
	tst	r12, #BF_H3900
	beq	1f
#ifdef CONFIG_MACH_H3900
	//; see if the user is holding down the action button
	ldr	r6, H3900_ASIC2_BASE
	add	r6,r6,#_H3900_ASIC2_KPIO_Base
	//; set up the asic2 buttons in alt mode to be read
	add	r1,r6,#_H3900_ASIC2_KPIO_Alternate
	mov	r0,#0x3e
	str	r0,[r1]
	//; read the action button = kpio5
	add	r1,r6,#_H3900_ASIC2_KPIO_Data
	ldr	r0,[r1]
	tst	r0, #(1<<5)	/* action button on 3800, 0 is pressed, 1 is up */
	beq	2f
	/* was up */
	bic	r12, r12, #BF_ACTION_BUTTON
	orr	r12, r12, #BF_NORMAL_BOOT
	b	1f
	/* was down */
	orr	r12, r12, #BF_ACTION_BUTTON
2:	bic	r12, r12, #BF_NORMAL_BOOT
#endif
1:	orr	r12, r12, #BF_BUTTON_CHECKED
	mov	pc, lr

H3900_RS232_On:	
	tst	r12, #BF_H3900
	beq	1f
#ifdef CONFIG_MACH_H3900	
	ldr	r3,H3900_ASIC3_BASE
	//; set up the mask register
	add	r1,r3,#_H3900_ASIC3_GPIOB_Base
	add	r1,r1,#_H3900_ASIC3_GPIO_Mask
	ldr	r0,_H3900_ASIC3_GPIO_MASK_INIT
	str	r0,[r1]
	//; set up the out register	
	add	r1,r3,#_H3900_ASIC3_GPIOB_Base
	add	r1,r1,#_H3900_ASIC3_GPIO_Out
	ldr	r0,_H3900_ASIC3_GPIO_OUT_INIT
	str	r0,[r1]
	//; set up dir register
	add	r1,r3,#_H3900_ASIC3_GPIOB_Base
	add	r1,r1,#_H3900_ASIC3_GPIO_Direction
	ldr	r0,_H3900_ASIC3_GPIO_DIR_INIT
	str	r0,[r1]

	//; set up the out register	
	add	r1,r3,#_H3900_ASIC3_GPIOB_Base
	add	r1,r1,#_H3900_ASIC3_GPIO_Out
	ldr	r0,_H3900_ASIC3_GPIO_OUT_INIT
	str	r0,[r1]
#endif /* CONFIG_MACH_H3900 */	
1:	mov	pc, lr
	
#if defined(CONFIG_MACH_H3900) /* for now, use the macros from h3900-init.h */	
GAFR0x_Init_H3900:	.long GAFR0x_InitValue
GAFR1x_Init_H3900:	.long GAFR1x_InitValue
GAFR0y_Init_H3900:	.long GAFR0y_InitValue
GAFR1y_Init_H3900:	.long GAFR1y_InitValue
GAFR0z_Init_H3900:	.long GAFR0z_InitValue
GAFR1z_Init_H3900:	.long GAFR1z_InitValue
GPDRx_Init_H3900:	.long GPDRx_InitValue
GPDRy_Init_H3900:	.long GPDRy_InitValue
GPDRz_Init_H3900:	.long GPDRz_InitValue
GPSRx_Init_H3900:	.long  0x00000000 /* we don't seem to need to do anything here for h3900 */
GPSRy_Init_H3900:	.long  0x00000000
GPSRz_Init_H3900:	.long  0x00000000
GPCRx_Init_H3900:	.long  0x00000000
GPCRy_Init_H3900:	.long  0x00000000
GPCRz_Init_H3900:	.long  0x00000000
#endif	
	
#if defined(CONFIG_MACH_H3900)
H3900_ASIC3_BASE:
        .long _H3900_ASIC3_Base
H3900_ASIC2_BASE:	
	.long 	_H3900_ASIC2_Base
#endif /* config_h3900 */	
	

DEADBEEF_S:
	.long 0xdeadbeef
	
MSC0_INIT:	
	.long 0x26e026e0
MSC1_INIT:	
	.long 0x26e826e8
MSC2_INIT:	
	.long 0x74a42494
		
_H3900_ASIC3_GPIO_OUT_INIT:	
	.long 0x0004
_H3900_ASIC3_GPIO_MASK_INIT:	
	.long   0xffff
_H3900_ASIC3_GPIO_DIR_INIT:	
	.long   0xffff
	
	//; alternate fxn reg settings to enable the ff uart
GPDR1_S:
	.long 0xfcffab82	
	//;.long (GPIO34_FFRXD_MD | GPIO35_FFCTS_MD | GPIO36_FFDCD_MD | GPIO37_FFDSR_MD | GPIO38_FFRI_MD    \
	//;       | GPIO39_FFTXD_MD | GPIO40_FFDTR_MD | GPIO41_FFRTS_MD)
GAFR1_L_S:
	.long 0x999a8558
	//;.long ( (1<< GPIO39_FFTXD) | (1<<GPIO40_FFDTR) | (1<<GPIO41_FFRTS))

#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900) || defined(CONFIG_MACH_H5400)
wince_jp:
	.long	0x00041000
WINCE_MAGIC_1:	
	.long WINCE_PARTITION_MAGIC_1
WINCE_MAGIC_LONG_0:	
	.long WINCE_PARTITION_LONG_0
WINCE_MAGIC_2:	
	.long WINCE_PARTITION_MAGIC_2
bootldr_magic:  
	.long BOOTLDR_MAGIC
#endif

boot_flags_init_val:
#if defined(CONFIG_MACH_IPAQ) || defined(CONFIG_MACH_H3900) || defined(CONFIG_MACH_H5400)
        .long	BF_NORMAL_BOOT|BF_SQUELCH_SERIAL
#else
	.long	BF_NORMAL_BOOT
#endif

#if 0
mdrefr_valid_bits_after_reset:	
	.long MDREFR_SLFRSH|MDREFR_KAPD|MDREFR_EAPD|MDREFR_K2DB2|MDREFR_K1RUN|MDREFR_E1PIN|MDREFR_K0DB2
#endif


	
