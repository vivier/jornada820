/*
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
 * $Id: sa1101.c,v 1.6 2004/07/02 00:02:08 fare Exp $
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
		ret = -EBUSY;
		goto out;
	}

	printk(KERN_INFO "SA1101 Microprocessor Companion Chip mapped.\n");
        ret=0;

 out:
	return ret;
}

/*
 * SA1101 interrupt support
 */

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

static void irq_ack_low(unsigned int irq)
{
	INTSTATCLR0 = SA1101_IRQMASK_LO(irq);
}

static void irq_mask_low(unsigned int irq)
{
	INTENABLE0 &= ~SA1101_IRQMASK_LO(irq);
}

static void irq_unmask_low(unsigned int irq)
{
	INTENABLE0 |= SA1101_IRQMASK_LO(irq);
}

static struct irqchip irq_low = {
	.ack	= irq_ack_low,
	.mask	= irq_mask_low,
	.unmask = irq_unmask_low,
};

static void irq_ack_high(unsigned int irq)
{
	INTSTATCLR1 = SA1101_IRQMASK_HI(irq);
}

static void irq_mask_high(unsigned int irq)
{
	INTENABLE1 &= ~SA1101_IRQMASK_HI(irq);
}

static void irq_unmask_high(unsigned int irq)
{
	INTENABLE1 |= SA1101_IRQMASK_HI(irq);
}

static struct irqchip irq_high = {
	.ack	= irq_ack_high,
	.mask	= irq_mask_high,
	.unmask = irq_unmask_high,
};

extern asmlinkage void asm_do_IRQ(int irq, struct pt_regs *regs);

static irqreturn_t sa1101_IRQ_demux(int irq, void *dev_id, struct pt_regs *regs)
{
unsigned long stat0, stat1;
int i, found_one;

	do {
		found_one = 0;
		stat0 = INTSTATCLR0 & INTENABLE0 ;
		stat1 = INTSTATCLR1 & INTENABLE1 ;
		
#define CHECK_STAT(A,B) \
		for (i = A; B; i++, B >>= 1) \
			if (B & 1) { \
				found_one = 1; \
				asm_do_IRQ(i, regs); \
			}
		CHECK_STAT(IRQ_SA1101_START, stat0);
		CHECK_STAT(IRQ_SA1101_START+32, stat1);
	} while(found_one);
	
	return IRQ_HANDLED;
}

void __init sa1101_init_irq(int irq_nr)
{
  int irq;
  
  alloc_irq_space(64);

  request_mem_region(_INTTEST0, 512, "irqs"); // XXX - still needed???

  
  /* disable all IRQs */
  INTENABLE0 = 0;
  INTENABLE1 = 0;
  
  INTSTATCLR0 = -1;
  INTSTATCLR1 = -1;

  for (irq = IRQ_SA1101_GPAIN0; irq <= IRQ_SA1101_KPYIn7; irq++) {
	  set_irq_chip(irq, &irq_low);
	  set_irq_handler(irq, do_edge_IRQ);
	  set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
  }
	
  for (irq = IRQ_SA1101_KPYIn8; irq <= IRQ_SA1101_USBRESUME; irq++) {
	  set_irq_chip(irq, &irq_high);
	  set_irq_handler(irq, do_edge_IRQ);
	  set_irq_flags(irq, IRQF_VALID | IRQF_PROBE);
  }
  
  /* Register SA1101 interrupt */
  if (irq_nr < 0) return;
  if (request_irq(irq_nr, sa1101_IRQ_demux, SA_INTERRUPT, "SA1101", NULL) >= 0) {
	  set_irq_type(irq_nr,IRQT_RISING);
  } else {
	  printk(KERN_ERR "SA1101: unable to claim IRQ %d\n", irq_nr);
  }
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

/*
 * wake up the 1101
 */

void sa1101_wake(void)
{
  unsigned long flags;

  local_irq_save(flags);

  /* Control register setup */

  SKCR = SKCR_VCOON | SKCR_PLLEn | SKCR_IRefEn ;
  mdelay(100);
  SKCR |= SKCR_BCLKEn;
  udelay(100);

  /* Snoop register */

  SNPR &= ~SNPR_SnoopEn;	/* snoop		off */

  /* ---------------------------------------------------------- */
  /* Set up clocks                                              */
  /* ---------------------------------------------------------- */
   
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
