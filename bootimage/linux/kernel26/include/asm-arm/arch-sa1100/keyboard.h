/*
 *  linux/include/asm-arm/arch-sa1100/keyboard.h
 *  Created 16 Dec 1999 by Nicolas Pitre <nico@cam.org>
 *  This file contains the SA1100 architecture specific keyboard definitions
 */
/* Jornada820 version based on keyboard.h 1.2 from cvs.handhelds.org
 * $Id: keyboard.h,v 1.1 2004/06/24 16:58:52 fare Exp $
 */

#ifndef _SA1100_KEYBOARD_H
#define _SA1100_KEYBOARD_H

#include <linux/config.h>
#include <asm/mach-types.h>

extern void gc_kbd_init_hw(void);
extern void smartio_kbd_init_hw(void);
extern void j820_kbd_init_hw(void);

static inline void kbd_init_hw(void)
{
	if (machine_is_graphicsclient())
		gc_kbd_init_hw();
	if (machine_is_adsbitsy())
		smartio_kbd_init_hw();
	if (machine_is_jornad820())
		j820_kbd_init_hw();
}

#endif  /* _SA1100_KEYBOARD_H */
