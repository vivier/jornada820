/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
  . = 0xc0800000;
  _text = .;
  .text : {
   _stext = .;
   _start = .;
   *(.start)
   kernel_start = .;
   kernel.o
   kernel_len = . - kernel_start;
   . = ALIGN(32);
   initrd_start = .;
   initrd.o 
   . = ALIGN(32);
   initrd_end = .;
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
