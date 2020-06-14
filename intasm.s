/* intasm.S
 * assembler interrupt support file
 * handhelds.org open bootldr
 *
 * code that is unclaimed by others is
 *    Copyright (C) 2003 Joshua Wise
 *
 * lots of help from Philip Blundell, thanks :)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

	.globl irq_stub

irq_stub:
	ldr pc, irq_vec
irq_vec: .word entry

entry:
	stmfd sp!, {r0-r14}
	bl irq_handle
	nop
	ldmfd sp!, {r0-r14}
	subs pc, lr, #4

