/*
 * linux/include/asm-arm/arch-sa1100/jornada820.h
 *
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Based on John Ankcorn's Jornada 720 file
 *
 * This file contains the hardware specific definitions for HP Jornada 820
 *
 */

#ifndef __ASM_ARCH_HARDWARE_H
#error "include <asm/hardware.h> instead"
#endif


/*
 *  The SA1101 on this machine - physical address
 */

#define SA1101_BASE		(0x18000000) 
#define SA1101_p2v( x )         ((x) - SA1101_BASE + 0xf4000000)
#define SA1101_v2p( x )         ((x) - 0xf4000000  + SA1101_BASE)
#ifndef Word
#define Word unsigned
#endif

/*
 * The keyboard's GPIO and IRQ
 */

#define GPIO_JORNADA820_KEYBOARD	GPIO_GPIO(0)
#define GPIO_JORNADA820_KEYBOARD_IRQ	IRQ_GPIO0

/*
 * GPIO 20 is the reset for the SA-1101. Hold to 1 to reset SA-1101.
 */

#define GPIO_JORNADA820_SA1101RESET     GPIO_GPIO(20)

/*
 * GPIO 23 is the backlight switch. Turn to 0.
 */

#define GPIO_JORNADA820_BACKLIGHTON     GPIO_GPIO(23)

/*
 * DACDR1 and DACDR2 are the knobs for brightness and contrast
 */

#define DAC_JORNADA820_CONTRAST         DACDR1
#define DAC_JORNADA820_BRIGHTNESS       DACDR2

/*
 *  The mouse GPIO and IRQ (not confirmed)
 */

#define GPIO_JORNADA820_MOUSE		GPIO_GPIO(9)
#define IRQ_JORNADA820_MOUSE	        IRQ_GPIO9

/*
 * The SA1101's chain interrupt.
 */

#define IRQ_JORNADA820_SA1101_CHAIN     IRQ_GPIO14