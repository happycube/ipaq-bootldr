/* int.c
 * interrupt support for the handhelds.org bootldr to provide interrupts for USB
 * handhelds.org open bootldr
 *
 * code that is unclaimed by others is
 *    Copyright (C) 2003 Joshua Wise
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Changelog:
 *   ?/??/?? 0.1: does USB interrupt handling.
 *   ?/??/?? 0.2: DMA interrupt handling.
 *   3/09/03 0.3: DMA send interrupt also.
 */

#include "commands.h"
#include "bootldr.h"
#include "serial.h"

#ifndef CONFIG_ACCEPT_GPL
#  error This file covered by GPL but CONFIG_ACCEPT_GPL undefined.
#endif

#define INTVERSION "0.3sa"

extern void irq_stub();
extern void usb_poll();
extern int usb_getinitted();

#define REG(x)       (*((volatile u32 *)x))

/* following code is from asm/system.h in the kernel.
 *
 *  Copyright (C) 1996 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define __sti()                                                 \
        ({                                                      \
                unsigned long temp;                             \
        __asm__ __volatile__(                                   \
        "mrs    %0, cpsr                @ sti\n"                \
"       bic     %0, %0, #128\n"                                 \
"       msr     cpsr_c, %0"                                     \
        : "=r" (temp)                                           \
        :                                                       \
        : "memory");                                            \
        })


/* from the iPAQ Linxu kernel's "SA-1100.h". info from header:
 *      Author          Copyright (c) Marc A. Viredaz, 1998
 *                      DEC Western Research Laboratory, Palo Alto, CA
 */
   
#define ICIP            REG(0x90050000)  /* IC IRQ Pending reg.             */
#define ICMR            REG(0x90050004)  /* IC Mask Reg.                    */
#define ICLR            REG(0x90050008)  /* IC Level Reg.                   */
#define ICCR            REG(0x9005000C)  /* IC Control Reg.                 */
#define ICFP            REG(0x90050010)  /* IC FIQ Pending reg.             */
#define ICPR            REG(0x90050020)  /* IC Pending Reg.                 */

#define IC_Ser0UDC      0x00002000      /* Ser. port 0 UDC                 */
#define IC_DMA0		(0x1 << 20)
#define IC_DMA1		(0x1 << 21)
/* need space for an intmode stack */
unsigned char intstack[1024];

static int int_isrunning = 0;

extern void usb_int();
extern void dma_int();
extern void dma_sendint();
void set_irqstack(unsigned char* stack);

SUBCOMMAND(int, init, command_int_init, "-- starts interrupts", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(int, version, command_int_version, "-- does a interrupt version check", BB_RUN_FROM_RAM, 1);
SUBCOMMAND(int, readcpsr, command_int_readcpsr, "-- read cpsr", BB_RUN_FROM_RAM, 1);

void command_int_version(int argc, const char** argv)
{
	putstr("version " INTVERSION "\r\n");
};

void command_int_init(int argc, const char** argv)
{
	if (int_isrunning)
	{
		putstr("Interrupts are already running. Not reenabling.\n");
		return;
	};
	int_isrunning = 1;
	putstr("enabling interrupts: ");
	
	memcpy ((void*)0x00000018, irq_stub, 8);
	putstr("handler, ");
	
	set_irqstack(&intstack[1024]);
	putstr("stack, ");
	
	__sti();
	ICLR = 0;
	ICCR = 1;
	putstr("irq, ");
	
	ICMR = IC_Ser0UDC | IC_DMA0 | IC_DMA1; 
	putstr("unmask, ");
	
	putstr("done\r\n");
};

int int_getinitted() { return int_isrunning; }

static int read_cpsr () {
	register int r;
	asm ("mrs %0, cpsr" : "=r" (r));
	return r;
}

void command_int_readcpsr(int argc, const char** argv)
{
	putLabeledWord("CPSR: ", read_cpsr());
};

void irq_handle()
{
	if (ICIP & IC_Ser0UDC)
		usb_int();
	if (ICIP & IC_DMA0)
		dma_int();
	if (ICIP & IC_DMA1)
		dma_sendint();
};

/*
 *  from linux/arch/arm/kernel/fiq.c and linux/include/asm-arm/proc-armv/ptrace.h
 *
 *  Copyright (C) 1998 Russell King
 *  Copyright (C) 1998, 1999 Phil Blundell
 */

#define T_BIT           0x20
#define F_BIT           0x40
#define I_BIT           0x80
#define IRQ_MODE        0x12

void set_irqstack(unsigned char* stack)
{
	register unsigned long tmp, tmp2;
	__asm__ volatile (
	"mrs	%0, cpsr
	mov	%1, %3
	msr	cpsr_c, %1	@ select IRQ mode
	mov	r0, r0
	mov	r13, %2
	msr	cpsr_c, %0	@ return to SVC mode
	mov	r0, r0"
	: "=&r" (tmp), "=&r" (tmp2)
	: "r" (stack), "I" (I_BIT | F_BIT | IRQ_MODE)
	/* These registers aren't modified by the above code in a way
	   visible to the compiler, but we mark them as clobbers anyway

	   so that GCC won't put any of the input or output operands in
	   them.  */
	: "r13");
}
