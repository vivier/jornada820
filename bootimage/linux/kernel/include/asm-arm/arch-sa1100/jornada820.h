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


/* TODO: remove */
#define SA1101_p2v( x )         ((x) - SA1101_BASE + 0xf4000000)
#define SA1101_v2p( x )         ((x) - 0xf4000000  + SA1101_BASE)
#ifndef Word
#define Word unsigned
#endif

/*------------------------- SA1100 ----------------------*/
/*
 * The keyboard's GPIO and IRQ
 */

#define GPIO_JORNADA820_KEYBOARD		GPIO_GPIO(0)
#define GPIO_JORNADA820_KEYBOARD_IRQ		IRQ_GPIO0

/*
 * UCB1200 GPIO and IRQ
 */

#define GPIO_JORNADA820_UCB1200			GPIO_GPIO(1)
#define GPIO_JORNADA820_UCB1200_IRQ		IRQ_GPIO1

/*
 * The SA1101's chain interrupt.
 */

#define GPIO_JORNADA820_SA1101_CHAIN		GPIO_GPIO(14)
#define GPIO_JORNADA820_SA1101_CHAIN_IRQ	IRQ_GPIO14

/*
 * SERIAL GPIO and IRQ
 */

#define GPIO_JORNADA820_SERIAL			GPIO_GPIO(15)
#define GPIO_JORNADA820_SERIAL_IRQ		IRQ_GPIO15

/*
 * POWERD GPIO and IRQ
 */

#define GPIO_JORNADA820_POWERD			GPIO_GPIO(16)
#define GPIO_JORNADA820_POWERD_IRQ		IRQ_GPIO16

/*
 * DTRDSR??? GPIO and IRQ
 */

#define GPIO_JORNADA820_DTRDSR			GPIO_GPIO(18)
#define GPIO_JORNADA820_DTRDSR_IRQ		IRQ_GPIO18

/*
 * GPIO 20 is the reset for the SA-1101. Hold to 1 to reset SA-1101.
 */

#define GPIO_JORNADA820_SA1101RESET		GPIO_GPIO(20)

/*
 * GPIO 23 is the backlight switch. Turn to 0.
 */

#define GPIO_JORNADA820_BACKLIGHTON     	GPIO_GPIO(23)

/*
 * LED GPIO
 */
#define GPIO_JORNADA820_LED			GPIO_GPIO(25)

/*
 * LEDBUTTON GPIO and IRQ
 */
#define GPIO_JORNADA820_LEDBUTTON		GPIO_GPIO(26)
#define GPIO_JORNADA820_LEDBUTTON_IRQ		IRQ_GPIO26

/*------------------------- SA1101 ----------------------*/
/*
 * DACDR1 and DACDR2 are the knobs for brightness and contrast
 */

#define DAC_JORNADA820_CONTRAST			DACDR1
#define DAC_JORNADA820_BRIGHTNESS		DACDR2

/*
 * PCMCIA/CF power...
 */
