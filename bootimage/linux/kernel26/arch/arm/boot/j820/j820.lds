/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * $Id: j820.lds,v 1.3 2004/06/28 12:40:44 fare Exp $
 */
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
  . = 0xc0800000;
	/* base + 8MB: leave enough space for initrd
         * to still be there after kernel relocates and decompresses
	 * to its rightful place (base + 2MB + 32KB)
	 */
  _text = .;
  .text : {
   _stext = .;
   _start = .;
   *(.start)
   kernel_start = .;
   arch/arm/boot/j820/kernel.o
   kernel_len = . - kernel_start;
   . = ALIGN(32);
   initrd_start = .;
   arch/arm/boot/j820/initrd.o 
   . = ALIGN(32);
   initrd_end = .;
   initrd_size = initrd_end - initrd_start - 0xc0800000 ;
	/* somehow, the linker insist in adding 0xc0800000
	 * to the value of this symbol (!)
	 */
   _etext = .;
  }
  
  .stab 0 : { *(.stab) }
  .stabstr 0 : { *(.stabstr) }
  .stab.excl 0 : { *(.stab.excl) }
  .stab.exclstr 0 : { *(.stab.exclstr) }
  .stab.index 0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment 0 : { *(.comment) }
}
