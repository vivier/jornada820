/*
 * linux/include/asm-arm/arch-sa1100/irq.h
 * 
 * Author: Nicolas Pitre
 */
/* Jornada820 version based on ???
 * $Id: irq.h,v 1.1 2004/06/24 19:57:38 fare Exp $
 */

#define fixup_irq(x)	(x)

/*
 * This prototype is required for cascading of multiplexed interrupts.
 * Since it doesn't exist elsewhere, we'll put it here for now.
 */
extern void do_IRQ(int irq, struct pt_regs *regs);
