/*
 * Jornada 820 keyboard driver.
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Modelled after gc_keyb.c
 *
 * Cannot (yet) handle Fn key combinations and Power On/Off
 * Depends on Russel King's SSP driver to work.
 *
 */

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kbd_ll.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kbd_kern.h>

#include <asm/keyboard.h>
#include <asm/irq.h>
#include <asm/hardware.h>

/*
00-07:  -   F1 1  q  a  z  \t -
08-0F:  -   F2 2  w  s  x  ^1 -
10-17:  -   F3 3  e  d  c  -  Alt
18-1F:  Win F4 4  r  f  v  -  `
20-27:  -   F5 5  t  g  b  -  -
28-2F:  SPC F6 6  y  h  n  -  -
30-37:  Fn  F7 7  u  j  m  <  -
38-3F:  Del F8 8  i  k  ;  >  -
40-47:  -   F9 9  o  l  '  ?  UP
48-4F:  DWN FA 0  p  [  ]  En LFT
50-57:  RGT FB -  +  BS \  ^2 -
58-5F:  -   -  -  -  -  -  -  -
60-67:  ESC -  -  -  -  -  -  -
68-6F:  Ctl -  -  -  -  -  -  -
70-7F:  POW
*/

/* I fixed George's keymap. Also mapped Power to F12, Fn to AltGr -- fare */
static char kbmap[128] = {
/* 00-07: */   0, 59,  2, 16, 30, 44, 15,  0,
/* 08-0F: */   0, 60,  3, 17, 31, 45, 42,  0,
/* 10-17: */   0, 61,  4, 18, 32, 46,  0, 56,
/* 18-1F: */ 126, 62,  5, 19, 33, 47,  0, 41,
/* 20-27: */   0, 63,  6, 20, 34, 48,  0,  0,
/* 28-2F: */  57, 64,  7, 21, 35, 49,  0,  0,
/* 30-37: */ 100, 65,  8, 22, 36, 50, 51,  0,
/* 38-3F: */  83, 66,  9, 23, 37, 39, 52,  0,
/* 40-47: */   0, 67, 10, 24, 38, 40, 53,103,
/* 48-4F: */ 108, 68, 11, 25, 26, 27, 28,105,
/* 50-57: */ 106, 87, 12, 13, 14, 43, 54,  0,
/* 58-5F: */   0,  0,  0,  0,  0,  0,  0,  0,
/* 60-67: */   1,  0,  0,  0,  0,  0,  0,  0,
/* 68-6F: */  97,  0,  0,  0,  0,  0,  0,  0,
/* 70-77: */  88,  0,  0,  0,  0,  0,  0,  0,
/* 78-7F: */   0,  0,  0,  0,  0,  0,  0,  0
};

int j820_kbd_setkeycode(unsigned int scancode, unsigned int keycode)
{
  kbmap[scancode&0x7F] = keycode;
  return 0;
}

int j820_kbd_getkeycode(unsigned int scancode)
{
  return kbmap[scancode &0x7F];
}

int j820_kbd_translate(unsigned char scancode, unsigned char *keycode,
		       char raw_mode)
{
  *keycode = kbmap[scancode & 0x7F];
  return 1;
}

char j820_kbd_unexpected_up(unsigned char keycode)
{
  return 0;
}

static void j820_kbd_irq(int irq, void *dev_id, struct pt_regs *regs)
{

#ifdef CONFIG_VT
  kbd_pt_regs = regs;
#endif
  disable_irq(irq);

  ssp_write_word(0x8200);
  unsigned scancode = ssp_read_word();
  handle_scancode(scancode, (scancode & 0x80) ? 0 : 1);

  enable_irq(irq);
}

void __init jornada820_kbd_init_hw(void)
{
  printk (KERN_INFO "Jornada 820 keyboard driver\n");

	/* init ? */
  
  k_setkeycode	  = j820_kbd_setkeycode;
  k_getkeycode	  = j820_kbd_getkeycode;
  k_translate	  = j820_kbd_translate;
  k_unexpected_up = j820_kbd_unexpected_up;
  
  /* ------------------------------ */
  /* Turn on the keyboard interrupt */
  /* ------------------------------ */

  set_GPIO_IRQ_edge (GPIO_JORNADA820_KEYBOARD, GPIO_FALLING_EDGE);
  if (request_irq(GPIO_JORNADA820_KEYBOARD_IRQ,
		  j820_kbd_irq,
		  0,
		  "j820_kbd_irq",
		  NULL) != 0)
    printk("Could not allocate IRQ for kbd!\n");
}
