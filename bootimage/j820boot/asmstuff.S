/*	asmstuff.S	*/
/*
* insert directive to dynlink with coredll.lib, corelibc.lib
*/

	.globl read_mmu, IntOff, DRAMloader
@	.pdata
@	.word	read_mmu, 0x40000200
	.text
read_mmu:
	mrc	p15, 0, r0, c2, c0, 0 
	mov	pc, lr

@	.pdata
@	.word	IntOff, 0x40000100
	.text
IntOff:
	mrs	r4,cpsr
	orr	r1,r4,#0xef
	msr	cpsr_c,r1
	mov	pc, lr

@	.pdata
@	.word	DRAMloader, 0x40001004
	.text
DRAMloader:
	mov       r12, sp
	stmdb     sp!, {r0, r1}  /* stmfd */
	stmdb     sp!, {r12, lr}  /* stmfd */
	sub       sp, sp, #8

	ldr		r3, [sp, #0x10]  /* 0x10 = 16 */
	str		r3, [sp]
	ldr		r2, [sp, #0x14]  /* 0x14 = 20 */
	str		r2, [sp, #4]

	ldr		r1, [sp]	  /* jump address */
	ldr		r2, [sp, #4]	  /* machine number */

	ldr		r0, =0x3fff
	mcr     p15, 0, r0, c7, c10, 4  /* Enable access */
	mrc		p15, 0, r0, c2, c0, 0
	mov		r0, r0

	mrc     p15, 0, r3, c1, c0, 0   /* disable the MMU */
	and     r3, r3, #0xfffffffe 	/* xscale says p needs to be 0 ??? */
	mov	r3, #0x120
	mcr     p15, 0, r3, c1, c0, 0   /* disable the MMU */
	mrc	p15, 0, r0, c2, c0, 0
	mov	r0, r0

/* Jump into do_it and the kernel */
	mov	pc,r1			/* machine number is in register r2 */

