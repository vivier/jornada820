	TTL	C:\pocket\test\asm.cpp

	AREA	|.drectve|, DRECTVE
	DCB	"-defaultlib:coredll.lib "
	DCB	"-defaultlib:corelibc.lib "

	EXPORT	|?do_it@@YAXXZ|				; do_it
	AREA	|.pdata|, PDATA
|$T222|	DCD	|?do_it@@YAXXZ|
	DCD	0x40000100
	AREA	|.text|, CODE
|?do_it@@YAXXZ| PROC					; do_it
|$M220|


		mcr     p15, 0, r0, c7, c5, 0 	;/* invalidate i cache & BTB */
		mrc	p15, 0, r0, c2, c0, 0
		mov	r0, r0
		sub 	pc, pc, #4		


; turn off LCD controller...
; I couldn't figure out how to use armasm.exe conditional with variables
; #if xscale
		ldr	r9, =0x44000000
		ldr	r8, [r9]
		orr	r8, r8, #0x400
		str	r8, [r9]

; #elif strongarm
;		ldr	r9, =0xb0100000
;		ldr	r8, [r9]
;		bic	r8, r8, #1
;		str	r8, [r9]
; #endif

		add	r9, r1, #0x100

; #if xscale
		ldr	r5, =0xa0000100
		ldr	r7, =0xa0400000

; #elif strongarm
;		ldr	r5, =0xc0000100
;		ldr	r7, =0xc0400000
; #endif


label		ldr     r8, [r9]
        	str     r8, [r5]
		add     r9, r9, #4 
		add     r5, r5, #4 
		cmp     r5, r7
        	blt     label

; #if xscale
		MOV	r4, #0xA0000000

; #elif strongarm
;		MOV	r4, #0xc0000000
; #endif

		add	r11,r4,#0x8000

		mov 	r10,r2		;machine number passed from DRAMloader

		mcr	p15, 0, r0, c8, c7, 0x00 ;/* invalidate I+D TLB */
		mrc	p15, 0, r0, c2, c0, 0
		mov	r0, r0
		sub 	pc, pc, #4		

; they skipped this, unnecessary? seems like we need to do this

		mcr     p15, 0, r0, c7, c5, 0 	;/* invalidate i cache & BTB */
	
		mov	r0, #0
		mcr	p15, 0, r0, c13, c0, 0
		mrc	p15, 0, r0, c2, c0, 0
		mov	r0, r0
		sub 	pc, pc, #4		

		mov     r0, #0
	        mov     r1, r10
		ldr     r2, [r2, #0]
		mov     r2, #0

; Jump into the kernel:
		mov		pc, r11	


|$M221|
	ENDP  ; |?do_it@@YAXXZ|, do_it
	END

