/*
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Driver for the SA1101 chip in the Jornada820.
 * Modelled after the file sa1111.c
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
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/arch/irq.h>
#include <asm/uaccess.h>

struct resource sa1101_resource = {
  .name   = "SA1101"
};

EXPORT_SYMBOL_GPL(sa1101_resource);

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

void sa1101_IRQ_demux(int irq, void *dev_id, struct pt_regs *regs)
{
  unsigned long stat0, stat1;
  while (1)
    {
      int i;
      
      stat0 = INTSTATCLR0;
      stat1 = INTSTATCLR1;

      if (stat0 == 0 && stat1 == 0) break;
      
      for (i = IRQ_SA1101_START; stat0; i++, stat0 >>= 1)
	if (stat0 & 1) 
	{
	 do_IRQ(i, regs);
	}
      
      for (i = IRQ_SA1101_START + 32; stat1; i++, stat1 >>= 1)
	if (stat1 & 1) 
	{
	 do_IRQ(i, regs);
	}
    }
}

#define SA1101_IRQMASK_LO(x)	(1 << (x - IRQ_SA1101_START))
#define SA1101_IRQMASK_HI(x)	(1 << (x - IRQ_SA1101_START - 32))

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

static void sa1101_mask_and_ack_lowirq(unsigned int irq)
{
	unsigned int mask = SA1101_IRQMASK_LO(irq);

	//INTEN0 &= ~mask;
	INTSTATCLR0 = mask;
}

static void sa1101_mask_and_ack_highirq(unsigned int irq)
{
	unsigned int mask = SA1101_IRQMASK_HI(irq);

	//INTEN1 &= ~mask;
	INTSTATCLR1 = mask;
}

static void sa1101_mask_lowirq(unsigned int irq)
{
	INTENABLE0 &= ~SA1101_IRQMASK_LO(irq);
}

static void sa1101_mask_highirq(unsigned int irq)
{
	INTENABLE1 &= ~SA1101_IRQMASK_HI(irq);
}

static void sa1101_unmask_lowirq(unsigned int irq)
{
	INTENABLE0 |= SA1101_IRQMASK_LO(irq);
}

static void sa1101_unmask_highirq(unsigned int irq)
{
	INTENABLE1 |= SA1101_IRQMASK_HI(irq);
}

void __init sa1101_init_irq(int irq_nr)
{
  int irq, ret;
  
  request_mem_region(_INTTEST0, 512, "irqs");
  
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
  
  INTSTATCLR0 = -1;
  INTSTATCLR1 = -1;
  
  for (irq = IRQ_SA1101_GPAIN0; irq <= IRQ_SA1101_KPYIn7; irq++) {
    irq_desc[irq].valid		= 1;
    irq_desc[irq].probe_ok	= 0;
    irq_desc[irq].mask_ack	= sa1101_mask_and_ack_lowirq;
    irq_desc[irq].mask		= sa1101_mask_lowirq;
    irq_desc[irq].unmask	= sa1101_unmask_lowirq;
  }
  for (irq = IRQ_SA1101_KPYIn8; irq <= IRQ_SA1101_USBRESUME; irq++) {
    irq_desc[irq].valid		= 1;
    irq_desc[irq].probe_ok	= 0;
    irq_desc[irq].mask_ack	= sa1101_mask_and_ack_highirq;
    irq_desc[irq].mask		= sa1101_mask_highirq;
    irq_desc[irq].unmask	= sa1101_unmask_highirq;
  }
  
  /* Register SA1101 interrupt */
  if (irq_nr < 0) return;
  if (request_irq(irq_nr, sa1101_IRQ_demux, SA_INTERRUPT, "SA1101", NULL) < 0)
    printk(KERN_ERR "SA1101: unable to claim IRQ %d\n", irq_nr);
}

/*
 * wake up the 1101
 */

void sa1101_wake()
{
  unsigned long flags;
  extern int sa1101_init_proc();

  local_irq_save(flags);

  /* Control register setup */

  SKCR = SKCR_VCOON | SKCR_PLLEn | SKCR_IRefEn ;
  mdelay(100);
  SKCR |= SKCR_BCLKEn;
  udelay(100);

  /* Snoop register */

  SNPR &= ~SNPR_SnoopEn;

  /* ---------------------------------------------------------- */
  /* Set up clocks                                              */
  /* ---------------------------------------------------------- */
   
  SKPCR = 
    SKPCR_UCLKEn |             /* USB */
    SKPCR_PCLKEn |             /* PS/2 */
    SKPCR_ICLKEn |             /* Interrupt controller */
    SKPCR_VCLKEn |             /* Video controller */
    SKPCR_PICLKEn |            /* parallel port */
    SKPCR_nKPADEn |            /* multiplexer */
    SKPCR_DCLKEn;              /* DACs */
  
  SKCDR = 0x30000027;
  SKPCR=0xFFFFFFFF;
  VMCCR=0x100;
  SMCR = 0x1E;

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

EXPORT_SYMBOL_GPL(sa1101_wake);
EXPORT_SYMBOL_GPL(sa1101_doze);

MODULE_DESCRIPTION("Main driver for SA-1101.");
MODULE_LICENSE("GPL");
