/*
 *  linux/arch/arm/boot/j820/init.S
 *  George Almasi (galmasi@optonline.net)
 *  Based on Russel King's bootp/init.S
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Header file for moving kernel + initrd into place, and
 *  provide cmd line params to kernel. Note that we pass
 *  r0 through to r3 straight through.
 */
#define __ASSEMBLY__ 1
#include <asm/mach-types.h>
	
		.section .start,#alloc,#execinstr
		.type	_start, #function
		.globl	_start
_start:
		adr	r13, data
		ldmia	r13, {r4-r8}		@ get size and addr of initrd
						@ r5 = ATAG_INITRD
						@ r6 = initrd start
						@ r7 = initrd end
						@ r8 = param_struct address
		add	r8, r8, #16*4
		sub	r7,r7,r6
		stmia	r8, {r6,r7}		@ save in param_struct
		mov	r1, #MACH_TYPE_JORNADA820
		b       _kernel_start

data:
		.word	0x54410001		@ r4 = ATAG_CORE
		.word	0x54420005		@ r5 = ATAG_INITRD
		.word	initrd_start		@ r6
		.word	initrd_end		@ r7
		.word	params			@ r8

		.type	initrd_start,#object

_kernel_start:	
