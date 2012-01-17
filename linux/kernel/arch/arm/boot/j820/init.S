/*
 *  linux/arch/arm/boot/j820/init.S
 *  George Almasi (galmasi@optonline.net) and Fare Rideau (fare@tunes.org)
 *  Based on Russel King's bootp/init.S
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Header file for providing proper boot parameters to kernel.
 *  See http://www.arm.linux.org.uk/developer/booting.php
 */
#define __ASSEMBLY__ 1
#include <asm/mach-types.h>

		.section .start,#alloc,#execinstr
		.type	_start, #function
		.globl	_start
_start:

#define CONFIG_JORNADA820_F1267A_AUTODETECT 1
/* It's been tested to work, so we enable it by default. */

#ifdef CONFIG_JORNADA820_F1267A_AUTODETECT
@ detect whether or not we have memory expansion on RAM bank 2
/* Note:
* While autodetecting on my 32MB model,
* lack of alias testing leads to detecting 128MB -- bad!
* In other words, the F1267A *does* wrap around when you overflow.
* Note:	mis-discovering less then 3MB of RAM leads to the RAMdisk
* failing to uncompress, so that we can't see a full diagnostic.
*/

#if 0
@ turn off interrupts, just in case. (hpcboot already did it.)
	mrs	r2, cpsr
	orr	r2, r2, #0xc0
	msr	cpsr_c, r2
#endif

@ initialize registers with appropriate values for our detection hack
	mov	r5, #0xd0000000		@ memory address being tested
	mov	r6, #0			@ size found up to now
	mov	r7, #4*1024*1024	@ size increment
	mov	r8, #128*1024*1024	@ maximum size for expansion board
	mov	r9, #0xd0000000		@ base address for alias detection

detection_loop:
@ is there RAM at considered address?
		bl	testram
		bne	detection_done
@ is it but an alias to the base address?
		cmp	r5, r9
		beq	1f
		bl	testalias
		cmp	r0, #0
		bne	detection_done
1:
@ increment size
		add	r6, r6, r7
		cmp	r6, r8
		beq	detection_done
@ increment test address
		add	r5, r5, r7
@ loop
		b	detection_loop

detection_done:	
@ if we did find RAM, update the tags.
		cmp	r6, #0
		streq	r6, expansion_tag	@ mark as end of tags
		strne	r6, expansion_mem	@ fill the size

@ jump to the rest of the operations
		b	setup_kernel_params

@ Originally taken from blob/src/blob/memtest.S
	@ r5 = address to test
	@ returns eq = ram present, ne = no ram
	@ clobbers r1 - r4
testram:
	ldmia	r5, {r1, r2}    @ store current value in r1 and r2
	mov	r3, #0x55       @ write 0x55 to first word
	mov	r4, #0xaa       @ 0xaa to second
	stmia	r5, {r3, r4}
	ldmia	r5, {r3, r4}    @ read it back
	teq	r3, #0x55       @ do the values match
	teqeq	r4, #0xaa
	bne	bad             @ oops, no
	mov	r3, #0xaa       @ write 0xaa to first word
	mov	r4, #0x55       @ 0x55 to second
	stmia	r5, {r3, r4}
	ldmia	r5, {r3, r4}    @ read it back 
	teq	r3, #0xaa       @ do the values match
	teqeq	r4, #0x55
bad:    stmia	r5, {r1, r2}    @ in any case, restore old data
@	moveq	r0, #0          @ ok - all values matched
@	movne	r0, #1          @ no ram at this location
        mov     pc, lr

@ tests whether two valid RAM addresses are aliases one for the other
	@ r5, r9 = addresses to test
	@ returns r0 = 0 - no alias, r0 = 1 - alias
	@ clobbers r1 - r2
testalias:
	mov	r0, #0
	ldr	r1, [r5]	@ get current values at r5 and r9
	ldr	r2, [r9]
	cmp	r1, r2		@ if values differ, r5 and r9 are no alias
	movne	pc, lr
	add	r1, r1, #0xA5	@ change value at r9, see if r5 has changed
	str	r1, [r9]
	ldr	r1, [r5]
	str	r2, [r9]	@ restore value at r9
	cmp	r1, r2
	movne	r0, #1		@ if value at r9 changed, r5 and r9 are aliases
	mov	pc, lr

#endif

setup_kernel_params:
@ copy kernel ATAG params from our own
		adr	r4, data
		ldmia	r4!, {r0,r1,r2,r5,r6}
copy_loop:
	        ldmia   r4!, {r7-r10}	@ copy 4 words at a time
	        stmia   r5!, {r7-r10}
	        cmp     r4, r6		@ it is ok if we copy a bit more
	        ble     copy_loop

@ clear PID and enable clock switching
		mov r0, #0; mcr p15, 0, r0, c13, c0, 0;
		mov r0, #0; mcr p15, 0, r0, c15, c1, 2;

@ jump into (relocated) arch/arm/boot/compressed/head.S
		b       _kernel_start

data:
		@ entry values for r0-r2 as per booting.php
		.word	0, MACH_TYPE_JORNADA820, params
		@ initial values for the loop that copies tags to *params
		.word	params, end_of_tags
		@ tags
		.word	2, 0x54410001		@ ATAG_CORE
		.word	4, 0x54420005		@ ATAG_INITRD2
		.word	initrd_start, initrd_size
		.word   4, 0x54410002		@ ATAG_MEM: main memory
		.word	16*1024*1024, 0xC0000000
#ifdef CONFIG_JORNADA820_F1267A_AUTODETECT
expansion_tag:	.word   4, 0x54410002		@ ATAG_MEM: expansion board
expansion_mem:	.word	16*1024*1024, 0xD0000000
#endif
		.word	0,0			@ ATAG_NONE: end marker
end_of_tags:

_kernel_start:
