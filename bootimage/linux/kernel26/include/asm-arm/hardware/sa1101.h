/*
 * File created for Jornada 820... (?)
 * $Id: sa1101.h,v 1.4 2004/07/01 21:50:18 fare Exp $
 */
#ifndef _ASM_ARCH_SA1101
#define _ASM_ARCH_SA1101

#include <asm/arch/bitfield.h>

#define SA1101_IRQMASK_LO(x)	(1 << (x - IRQ_SA1101_START))
#define SA1101_IRQMASK_HI(x)	(1 << (x - IRQ_SA1101_START - 32))

#ifndef __ASSEMBLY__

extern int sa1101_probe(unsigned long phys_addr);

extern void sa1101_wake(void);
extern void sa1101_doze(void);

extern void sa1101_init_irq(int irq_nr);

extern int sa1101_pcmcia_init(void *handler);
extern int sa1101_pcmcia_shutdown(void);

extern int sa1101_usb_init(void);
extern int sa1101_usb_shutdown(void);

extern int sa1101_vga_init(void);
extern int sa1101_vga_shutdown(void);

#define sa1101_writel(val,addr)	({ *(volatile unsigned int *)(addr) = (val); })
#define sa1101_readl(addr)	(*(volatile unsigned int *)(addr))

#endif

#endif  /* _ASM_ARCH_SA1101 */
