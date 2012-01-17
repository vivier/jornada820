/*
 * Jornada 820 keyboard driver (USAR 1855A01 chip - undocumented).
 *
 * 2004/06/30 Matan Ziv-Av <matan@svgalib.org>
 * port to kernel 2.6
 *
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Modelled after gc_keyb.c
 *
 * $Id: j820_keyb.c,v 1.8 2005/07/26 06:31:34 fare Exp $
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

static char kbmap[128] = {
/*00-07:*/ KEY_RESERVED, KEY_F1      , KEY_1       , KEY_Q       , KEY_A        , KEY_Z         , KEY_TAB       , KEY_RESERVED,
/*08-0F:*/ KEY_RESERVED, KEY_F2      , KEY_2       , KEY_W       , KEY_S        , KEY_X         , KEY_LEFTSHIFT , KEY_RESERVED,
/*10-17:*/ KEY_RESERVED, KEY_F3      , KEY_3       , KEY_E       , KEY_D        , KEY_C         , KEY_RESERVED  , KEY_LEFTALT,
/*18-1F:*/ KEY_LEFTMETA, KEY_F4      , KEY_4       , KEY_R       , KEY_F        , KEY_V         , KEY_RESERVED  , KEY_GRAVE,
/*20-27:*/ KEY_RESERVED, KEY_F5      , KEY_5       , KEY_T       , KEY_G        , KEY_B         , KEY_RESERVED  , KEY_RESERVED,
/*28-2F:*/ KEY_SPACE   , KEY_F6      , KEY_6       , KEY_Y       , KEY_H        , KEY_N         , KEY_RESERVED  , KEY_RESERVED,
/*30-37:*/ KEY_RIGHTALT, KEY_F7      , KEY_7       , KEY_U       , KEY_J        , KEY_M         , KEY_COMMA     , KEY_RESERVED,
/*38-3F:*/ KEY_DELETE  , KEY_F8      , KEY_8       , KEY_I       , KEY_K        , KEY_SEMICOLON , KEY_DOT       , KEY_RESERVED,
/*40-47:*/ KEY_RESERVED, KEY_F9      , KEY_9       , KEY_O       , KEY_L        , KEY_APOSTROPHE, KEY_SLASH     , KEY_UP,
/*48-4F:*/ KEY_DOWN    , KEY_F10     , KEY_0       , KEY_P       , KEY_LEFTBRACE, KEY_RIGHTBRACE, KEY_ENTER     , KEY_LEFT,
/*50-57:*/ KEY_RIGHT   , KEY_SYSRQ   , KEY_MINUS   , KEY_EQUAL   , KEY_BACKSPACE, KEY_BACKSLASH , KEY_RIGHTSHIFT, KEY_RESERVED,
/*58-5F:*/ KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED , KEY_RESERVED  , KEY_RESERVED  , KEY_RESERVED,
/*60-67:*/ KEY_ESC     , KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED , KEY_RESERVED  , KEY_RESERVED  , KEY_RESERVED,
/*68-6F:*/ KEY_LEFTCTRL, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED , KEY_RESERVED  , KEY_RESERVED  , KEY_RESERVED,
/*70-77:*/ KEY_POWER   , KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED , KEY_RESERVED  , KEY_RESERVED  , KEY_RESERVED,
/*78-7F:*/ KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED, KEY_RESERVED , KEY_RESERVED  , KEY_RESERVED  , KEY_RESERVED
};


/* TODO: linux/input.h */
#ifndef BUS_SPI
#define BUS_SPI		0x20
#endif

static struct input_dev dev;
static int open;
static struct timer_list   timer;

static irqreturn_t j820_kbd_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned scancode;
	int key;
	
	disable_irq(irq);
	
	/* we can hang the kernel here, because the ssp driver does not support timeouts (yet) */
	ssp_write_word(0x8200);
	scancode = ssp_read_word();

	key = kbmap[scancode&0x7f];
	if(key != KEY_RESERVED) {
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

	dev.name = "j820_keyb";
	dev.id.bustype = BUS_SPI;
	
	input_register_device(&dev);

	request_irq(GPIO_JORNADA820_KEYBOARD_IRQ, j820_kbd_irq, 0,
		    "j820_kbd_irq", NULL);
	set_irq_type(GPIO_JORNADA820_KEYBOARD_IRQ, IRQT_FALLING);

	init_timer(&timer);
	timer.function = j820_kbd_timer;
	mod_timer(&timer, jiffies+2);
	printk(KERN_INFO "input: %s\n", dev.name);

	return 0;
}

static void __exit j820_kbd_exit(void)
{

	free_irq(GPIO_JORNADA820_KEYBOARD_IRQ, 0);
	input_unregister_device(&dev);
}

module_init(j820_kbd_init);
module_exit(j820_kbd_exit);
MODULE_DESCRIPTION("Jornada 820 keyboard driver");
MODULE_LICENSE("GPL");
