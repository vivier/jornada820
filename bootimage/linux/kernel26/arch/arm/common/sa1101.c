/*
 * 2004/07/02 Fare (fare@tunes.org)
 * Mimicked common/sa1111.c.
 *
 * 2004/07/01 Fare (fare@tunes.org)
 * Ported to 2.6, with inspiration from jornada56x.c.
 * See also Documentation/arm/Interrupts.
 *
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Main driver for the SA1101 chip.
 * Modelled after the file sa1111.c
 *
 * Created for the Jornada820 port.
 *
 * $Id: sa1101.c,v 1.1 2004/07/03 13:26:15 fare Exp $
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/ptrace.h>
#include <linux/errno.h>
#include <linux/ioport.h>
#include <linux/list.h>
#include <linux/timer.h>

#include <asm/hardware.h>
#include <asm/arch/SA-1101.h>
#include <asm/hardware/sa1101.h>
#include <asm/mach/irq.h>
#include <asm/arch/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/*
 * We keep the following data for the overall SA1111.  Note that the
 * struct device and struct resource are "fake"; they should be supplied
 * by the bus above us.  However, in the interests of getting all SA1101
 * drivers converted over to the device model, we provide this as an
 * anchor point for all the other drivers.
 */

static struct resource sa1101_resource = {
  .name   = "SA1101"
};

/*
 * Figure out whether we can see the SA1101
 */

int __init sa1101_probe(unsigned long phys_addr)
{
	int ret = -ENODEV;

	sa1101_resource.start = phys_addr;
	sa1101_resource.end = phys_addr + 0x00400000;

	if (request_resource(&iomem_resource, &sa1101_resource)) {
		printk(KERN_INFO "Failed to map SA1101 chip.\n");
		ret = -EBUSY;
		goto out;
	}

	printk(KERN_INFO "SA1101 Microprocessor Companion Chip mapped.\n");
        ret=0;

 out:
	return ret;
}

/*
 * A note about masking IRQs:
 *
 * The GPIO IRQ edge detection only functions while the IRQ itself is
 * enabled; edges are not detected while the IRQ is disabled.
 *
 * This is especially important for the PCMCIA signals, where we must
 * pick up every transition.  We therefore do not disable the IRQs
 * while processing them.
 *
 * However, since we are changed to a GPIO on the host processor,
 * all SA1101 IRQs will be disabled while we're processing any SA1101
 * IRQ.
 *
 * Note also that changing INTPOL while an IRQ is enabled will itself
 * trigger an IRQ.
 */

static void
sa1101_irq_handler(unsigned int irq, struct irqdesc *desc, struct pt_regs *regs)
{
	unsigned int stat0, stat1, i;

	stat0 = INTSTATCLR0;
	stat1 = INTSTATCLR1;
	INTSTATCLR0 = stat0;
	desc->chip->ack(irq);
	INTSTATCLR1 = stat1;

	if (stat0 == 0 && stat1 == 0) {
		do_bad_IRQ(irq, desc, regs);
		return;
	}

	for (i = IRQ_SA1101_START; stat0; i++, stat0 >>= 1)
		if (stat0 & 1)
			do_edge_IRQ(i, irq_desc + i, regs);

	for (i = IRQ_SA1101_START + 32; stat1; i++, stat1 >>= 1)
		if (stat1 & 1)
			do_edge_IRQ(i, irq_desc + i, regs);

	/* For level-based interrupts */
	desc->chip->unmask(irq);
}

static void sa1101_ack_irq(unsigned int irq)
{
}

static void mask_low_irq(unsigned int irq)
{
	INTENABLE0 &= ~SA1101_IRQMASK_LO(irq);
}
static void unmask_low_irq(unsigned int irq)
{
	INTENABLE0 |= SA1101_IRQMASK_LO(irq);
}
static int retrigger_low_irq(unsigned int irq)
{
	unsigned int mask = SA1101_IRQMASK_LO(irq);
	unsigned long ip0;
	int i;

	ip0 = INTPOL0;
	for (i = 0; i < 8; i++) {
		INTPOL0 = ip0 ^ mask;
		INTPOL0 = ip0;
		if (INTSTATCLR0 & mask) break;
	}

	if (i == 8)
		printk(KERN_ERR "Danger Will Robinson: failed to "
			"re-trigger IRQ%d\n", irq);
	return i == 8 ? -1 : 0;
}
static int type_low_irq(unsigned int irq, unsigned int flags)
{
	unsigned int mask = SA1101_IRQMASK_LO(irq);
	unsigned long ip0;

	if (flags == IRQT_PROBE)
		return 0;

	if ((!(flags & __IRQT_RISEDGE) ^ !(flags & __IRQT_FALEDGE)) == 0)
		return -EINVAL;

	ip0 = INTPOL0;
	if (flags & __IRQT_RISEDGE)
		ip0 &= ~mask;
	else
		ip0 |= mask;
	INTPOL0 = ip0;

	return 0;
}
/*
static int wake_low_irq(unsigned int irq, unsigned int on)
{
	return -1;
}
*/
static struct irqchip low_irq = {
	.ack		= sa1101_ack_irq,
	.mask		= mask_low_irq,
	.unmask		= unmask_low_irq,
	.retrigger	= retrigger_low_irq,
	.type		= type_low_irq,
/*	.wake		= wake_low_irq, */
};

static void mask_high_irq(unsigned int irq)
{
	INTENABLE1 &= ~SA1101_IRQMASK_LO(irq);
}
static void unmask_high_irq(unsigned int irq)
{
	INTENABLE1 |= SA1101_IRQMASK_LO(irq);
}
static int retrigger_high_irq(unsigned int irq)
{
	unsigned int mask = SA1101_IRQMASK_LO(irq);
	unsigned long ip1;
	int i;

	ip1 = INTPOL1;
	for (i = 0; i < 8; i++) {
		INTPOL1 = ip1 ^ mask;
		INTPOL1 = ip1;
		if (INTSTATCLR1 & mask) break;
	}

	if (i == 8)
		printk(KERN_ERR "Danger Will Robinson: failed to "
			"re-trigger IRQ%d\n", irq);
	return i == 8 ? -1 : 0;
}
static int type_high_irq(unsigned int irq, unsigned int flags)
{
	unsigned int mask = SA1101_IRQMASK_LO(irq);
	unsigned long ip1;

	if (flags == IRQT_PROBE)
		return 0;

	if ((!(flags & __IRQT_RISEDGE) ^ !(flags & __IRQT_FALEDGE)) == 0)
		return -EINVAL;

	ip1 = INTPOL1;
	if (flags & __IRQT_RISEDGE)
		ip1 &= ~mask;
	else
		ip1 |= mask;
	INTPOL1 = ip1;

	return 0;
}
/*
static int wake_high_irq(unsigned int irq, unsigned int on)
{
	return -1;
}
*/
static struct irqchip high_irq = {
	.ack		= sa1101_ack_irq,
	.mask		= mask_high_irq,
	.unmask		= unmask_high_irq,
	.retrigger	= retrigger_high_irq,
	.type		= type_high_irq,
/*	.wake		= wake_high_irq, */
};

void sa1101_init_irq(int sa1101_irq)
{
	unsigned int irq;

	alloc_irq_space(64); /* XXX - still needed??? */
	request_mem_region(_INTTEST0, 512, "irqs"); // XXX - still needed???

	/* disable all IRQs */
	INTENABLE0 = 0;
	INTENABLE1 = 0;
	
	/*
	 * detect on rising edge.  Note: Feb 2001 Errata for SA1101
	 * specifies that S0ReadyInt and S1ReadyInt should be '1'.
	 */
	INTPOL0 = 0;
	INTPOL1 = 
		SA1101_IRQMASK_HI(IRQ_SA1101_S0_READY_NIREQ) |
		SA1101_IRQMASK_HI(IRQ_SA1101_S1_READY_NIREQ);

	/* clear all IRQs */
	INTSTATCLR0 = -1;
	INTSTATCLR1 = -1;

	for (irq = IRQ_SA1101_GPAIN0; irq <= IRQ_SA1101_KPYIn7; irq++) {
		set_irq_chip(irq, &low_irq);
		set_irq_handler(irq, do_edge_IRQ);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	}
        
	for (irq = IRQ_SA1101_KPYIn8; irq <= IRQ_SA1101_USBRESUME; irq++) {
		set_irq_chip(irq, &high_irq);
		set_irq_handler(irq, do_edge_IRQ);
		set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
	}

	/*
	 * Register SA1101 interrupt
	 */
	set_irq_chained_handler(sa1101_irq, sa1101_irq_handler);
	set_irq_type(sa1101_irq, IRQT_RISING);
}

extern void sa1101_wake(void)
{
	unsigned long flags;

	local_irq_save(flags);

	/* Clock setup */
	GAFR |= GPIO_32_768kHz;
	GPDR |= GPIO_32_768kHz;
	TUCR = TUCR_3_6864MHz;

	/* sa1111.c does things quite differently;
	 * I copied over from the 2.4 sa1101.c... -- Fare
	 */

	/* Snoop register */

	SNPR &= ~SNPR_SnoopEn;	/* snoop		off */

	/* Set up clocks */
	SKPCR &=~SKPCR_UCLKEn;	/* USB                  off */
	SKPCR |= SKPCR_PCLKEn;	/* PS/2                 on  */
	SKPCR |= SKPCR_ICLKEn;	/* Interrupt controller on  */
	SKPCR &=~SKPCR_VCLKEn;	/* Video controller     off */
	SKPCR &=~SKPCR_PICLKEn;	/* parallel port        off */
	SKPCR |= SKPCR_nKPADEn;	/* multiplexer          on  */
	SKPCR |= SKPCR_DCLKEn; 	/* DACs                 on  */
  
	/* reset clock divider */
	SKCDR = 0x30000027;

	/* reset  video memory controller */
	VMCCR=0x100;

	/* setup shared memory controller */
	SMCR=SMCR_ColAdrBits(10)+SMCR_RowAdrBits(12)+SMCR_ArbiterBias;

	/* reset USB */
	USBReset |= USBReset_ForceIfReset;
	USBReset |= USBReset_ForceHcReset;

	local_irq_restore(flags);
}

extern int sa1101_usb_shutdown(void)
{
  /* not yet */
 return 0;
}

extern int sa1101_vga_shutdown(void)
{
  /* not yet */
 return 0;
}

extern int sa1101_usb_init(void)
{
  /* not yet */
 return 0;
}

extern int sa1101_vga_init(void)
{
  /* not yet */
 return 0;
}

/* ************************************************************************ */
/*        This is the place for PM code on this device.                     */
/* ************************************************************************ */

void sa1101_doze(void)
{
  /* not implemented */
	printk("SA1101 doze mode not implemented\n");
}

/*********************************************************************************/

EXPORT_SYMBOL_GPL(sa1101_wake);
EXPORT_SYMBOL_GPL(sa1101_doze);
EXPORT_SYMBOL_GPL(sa1101_usb_init);
EXPORT_SYMBOL_GPL(sa1101_usb_shutdown);
EXPORT_SYMBOL_GPL(sa1101_vga_init);
EXPORT_SYMBOL_GPL(sa1101_vga_shutdown);

MODULE_DESCRIPTION("Main driver for SA-1101.");
MODULE_LICENSE("GPL");
