/* asm.S */
/*
* insert directive to dynlink with coredll.lib, corelibc.lib
*/
	
	.globl	do_it
@	.pdata
@	.word	do_it, 0x40000100
	.text
do_it:
	mcr     p15, 0, r0, c7, c5, 0 	/* invalidate i cache & BTB */
	mrc	p15, 0, r0, c2, c0, 0
	mov	r0, r0
	sub 	pc, pc, #4

/* turn off LCD controller... */
	ldr	r9, =0xb0100000
	ldr	r8, [r9]
	bic	r8, r8, #1
	str	r8, [r9]

	add	r9, r1, #0x100

	ldr	r5, =0xc0000100
	ldr	r7, =0xc0400000

1:	ldr     r8, [r9]
        str     r8, [r5]
	add     r9, r9, #4 
	add     r5, r5, #4 
	cmp     r5, r7
        blt     1b

	mov	r4, #0xc0000000

	add	r11,r4,#0x8000

	mov 	r10,r2		/* machine number passed from DRAMloader */

	mcr	p15, 0, r0, c8, c7, 0x00 /* invalidate I+D TLB */
	mrc	p15, 0, r0, c2, c0, 0
	mov	r0, r0
	sub 	pc, pc, #4		

/* they skipped this, unnecessary? seems like we need to do this */

	mcr     p15, 0, r0, c7, c5, 0 	/* invalidate i cache & BTB */
	
	mov	r0, #0
	mcr	p15, 0, r0, c13, c0, 0
	mrc	p15, 0, r0, c2, c0, 0
	mov	r0, r0
	sub 	pc, pc, #4		

	mov     r0, #0
	mov     r1, r10
	ldr     r2, [r2, #0]
	mov     r2, #0

/* Jump into the kernel: */
	mov		pc, r11	
