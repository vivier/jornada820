	TTL	asmstuff.S

	AREA	|.drectve|, DRECTVE
	DCB	"-defaultlib:coredll.lib "
	DCB	"-defaultlib:corelibc.lib "

	EXPORT	|?read_mmu@@YAHXZ|			; read_mmu
	AREA	|.pdata|, PDATA
|$T231|	DCD	|?read_mmu@@YAHXZ|
	DCD	0x40000200
	AREA	|.text|, CODE
|?read_mmu@@YAHXZ| PROC					; read_mmu
|$M229|

	mrc	p15, 0, r0, c2, c0, 0 
	mov       pc, lr

|$M230|
	ENDP  ; |?read_mmu@@YAHXZ|, read_mmu

	EXPORT	|?IntOff@@YAXXZ|			; IntOff
	AREA	|.pdata|, PDATA
|$T235|	DCD	|?IntOff@@YAXXZ|
	DCD	0x40000100
	AREA	|.text|, CODE
|?IntOff@@YAXXZ| PROC					; IntOff
|$M233|

	mrs		r4,cpsr
	orr		r1,r4,#0xef
	msr		cpsr_c,r1
	mov		pc, lr

|$M234|
	ENDP  ; |?IntOff@@YAXXZ|, IntOff


	EXPORT	|?DRAMloader@@YAXII@Z|			; DRAMloader
	AREA	|.pdata|, PDATA
|$T239|	DCD	|?DRAMloader@@YAXII@Z|
	DCD	0x40001004
	AREA	|.text|, CODE
|?DRAMloader@@YAXII@Z| PROC				; DRAMloader

	mov       r12, sp
	stmdb     sp!, {r0, r1}  ; stmfd
	stmdb     sp!, {r12, lr}  ; stmfd
	sub       sp, sp, #8

|$M237|

	ldr		r3, [sp, #0x10]  ; 0x10 = 16
	str		r3, [sp]
	ldr		r2, [sp, #0x14]  ; 0x14 = 20
	str		r2, [sp, #4]

	ldr		r1, [sp]	   ; jump address
	ldr		r2, [sp, #4]	   ; machine number

	ldr		r0, =0x3fff
	mcr     p15, 0, r0, c7, c10, 4 ;Enable access
	mrc		p15, 0, r0, c2, c0, 0
	mov		r0, r0

	mrc     p15, 0, r3, c1, c0, 0   ;/* disable the MMU */
	and     r3, r3, #0xfffffffe 	; xscale says p needs to be 0 ???
	mov	r3, #0x120
	mcr     p15, 0, r3, c1, c0, 0   ;/* disable the MMU */
	mrc	p15, 0, r0, c2, c0, 0
	mov	r0, r0

; Jump into do_it and the kernel
	mov	pc,r1			; machine number is in register r2

|$M238|
	ENDP  ; |?DRAMloader@@YAXII@Z|, DRAMloader
	END

