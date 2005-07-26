/*
 * 
 */
#ifndef _ASM_ARCH_SA1101
#define _ASM_ARCH_SA1101

#include <asm/arch/bitfield.h>

#define SA1101_IRQMASK_LO(x)	(1 << (x - IRQ_SA1101_START))
#define SA1101_IRQMASK_HI(x)	(1 << (x - IRQ_SA1101_START - 32))

extern int sa1101_probe(unsigned long phys_addr);

extern void sa1101_wake(void);
extern void sa1101_doze(void);

extern void sa1101_init_irq(int irq_nr);
extern void sa1101_IRQ_demux(int irq, void *dev_id, struct pt_regs *regs);

extern int sa1101_pcmcia_init(void *handler);
extern int sa1101_pcmcia_shutdown(void);

//static int sa1101_usb_init(void);
//static int sa1101_usb_shutdown(void);

//static int sa1101_vga_init(void);
//static int sa1101_vga_shutdown(void);

#define sa1101_writereg(val,addr)	({ *(volatile unsigned int *)(addr) = (val); })
#define sa1101_readreg(addr)	(*(volatile unsigned int *)(addr))

#undef readl
#undef writel
#define readl(a) sa1101_readreg(SA1101_p2v( _USB( 0x100 * (int)(a) )))
#define writel(d, a) sa1101_writereg(d, SA1101_p2v( _USB( 0x100 * (int)(a) )))

#endif  /* _ASM_ARCH_SA1101 */
