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
00-07:  -  F1 1  q  a  z  \t -  
08-0F:  -  F2 2  w  s  x  ^1 -
10-17:  -  F3 3  e  d  c  -  Alt
18-1F:  Wn F4 4  r  f  v  -  `
20-27:  -  F5 5  t  g  b  -  -
28-2F:  SP F6 6  y  h  n  -  -  
30-37:  Fn F7 7  u  j  m  <  -
38-3F:  Dl F8 8  i  k  ;  >  -  
40-47:  -  F9 9  o  l  '  ?  UP
48-4F:  DN FA 0  p  [  ]  En LT
50-57:  RT FB -  +  BS \  ^2 - 
58-5F:  -  -  -  -  -  -  -  -  
60-67:  Es -  -  -  -  -  -  -  
68-6F:  Ctrl -  - - - - -
70-7F:  Off
*/

/* I fixed George's keymap, and put KP_mult in unmapped entries. -- fare */
static char kbmap[128] = {
/* 00-07: */  55, 59,  2, 16, 30, 44, 15, 55,
/* 08-0F: */  55, 60,  3, 17, 31, 45, 42, 55,
/* 10-17: */  55, 61,  4, 18, 32, 46, 55, 56,
/* 18-1F: */ 126, 62,  5, 19, 33, 47, 55, 41,
/* 20-27: */  55, 63,  6, 20, 34, 48, 55, 55,
/* 28-2F: */  57, 64,  7, 21, 35, 49, 55, 55,
/* 30-37: */ 100, 65,  8, 22, 36, 50, 51, 55,
/* 38-3F: */  83, 66,  9, 23, 37, 39, 52, 55,
/* 40-47: */  55, 67, 10, 24, 38, 40, 53,103,
/* 48-4F: */ 108, 68, 11, 25, 26, 27, 28,105,
/* 50-57: */ 106, 87, 12, 13, 14, 43, 54, 55,
/* 58-5F: */  55, 55, 55, 55, 55, 55, 55, 55,
/* 60-67: */   1, 55, 55, 55, 55, 55, 55, 55,
/* 68-6F: */  97, 55, 55, 55, 55, 55, 55, 55,
/* 70-77: */  88, 55, 55, 55, 55, 55, 55, 55,
/* 78-7F: */  55, 55, 55, 55, 55, 55, 55, 55
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
#if 1 /* XXX - debug code by fare */
  if ((*keycode) == 55) {
    printk("j820_keyb.c: Unexpected scancode 0x%02X received.\n", scancode);
  }
#endif
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
