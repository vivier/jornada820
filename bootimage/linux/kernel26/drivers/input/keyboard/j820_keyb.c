/*
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/hardware.h>
#include <asm/hardware/ssp.h>
#include <asm/uaccess.h>

MODULE_DESCRIPTION("Jornada 820 keyboard driver");
MODULE_LICENSE("GPL");
/*
 * 00-07:  -   F1 1  q  a  z  \t -
 * 08-0F:  -   F2 2  w  s  x  ^1 -
 * 10-17:  -   F3 3  e  d  c  -  Alt
 * 18-1F:  Win F4 4  r  f  v  -  `
 * 20-27:  -   F5 5  t  g  b  -  -
 * 28-2F:  SPC F6 6  y  h  n  -  -
 * 30-37:  Fn  F7 7  u  j  m  <  -
 * 38-3F:  Del F8 8  i  k  ;  >  -
 * 40-47:  -   F9 9  o  l  '  ?  UP
 * 48-4F:  DWN FA 0  p  [  ]  En LFT
 * 50-57:  RGT FB -  +  BS \  ^2 -
 * 58-5F:  -   -  -  -  -  -  -  -
 * 60-67:  ESC -  -  -  -  -  -  -
 * 68-6F:  Ctl -  -  -  -  -  -  -
 * 70-7F:  POW
 * */

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

static struct input_dev dev;
static int open;
static struct timer_list   timer;

static irqreturn_t j820_kbd_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned scancode;
	int key;
	
	disable_irq(irq);
	
	ssp_write_word(0x8200);
	scancode = ssp_read_word();
	key = kbmap[scancode&0x7f];
	if(key) {
		input_report_key(&dev, key, (scancode & 0x80) ? 0 : 1);
	}
	
	enable_irq(irq);

	return IRQ_HANDLED;
}

static int j820_kbd_open(struct input_dev *dev)
{
	open++;
	return 0;
}

static void j820_kbd_close(struct input_dev *dev)
{
	open--;
}

static void j820_kbd_timer(unsigned long data) {
	j820_kbd_irq(0, NULL, NULL);
	mod_timer(&timer, jiffies+2);
}

static int __init j820_kbd_init(void)
{
	int i;

	dev.evbit[0] = BIT(EV_KEY) | BIT(EV_REP);

	init_input_dev(&dev);

	for (i=0; i<128; i++)
		set_bit(kbmap[i], dev.keybit);

	clear_bit(0, dev.keybit);

	dev.private = NULL;
	dev.open = j820_kbd_open;
	dev.close = j820_kbd_close;
	dev.event = NULL;

	dev.name = "J820";
	dev.id.bustype = BUS_ISA;
	
	input_register_device(&dev);

	set_irq_type(GPIO_JORNADA820_KEYBOARD_IRQ, IRQT_FALLING);
	request_irq(GPIO_JORNADA820_KEYBOARD_IRQ, j820_kbd_irq, 0,
		    "j820_kbd_irq", NULL);

	init_timer(&timer);
	timer.function = j820_kbd_timer;
	mod_timer(&timer, jiffies+2);
	printk(KERN_INFO "input: keyboard: %s\n", dev.name);

	return 0;
}


static void __exit j820_kbd_exit(void)
{

	free_irq(GPIO_JORNADA820_KEYBOARD_IRQ, 0);
	input_unregister_device(&dev);
}

module_init(j820_kbd_init);
module_exit(j820_kbd_exit);

