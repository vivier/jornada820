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
 * $Id: sa1101.c,v 1.13 2004/07/11 13:02:31 oleg820 Exp $
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
#include <linux/device.h>

#include <asm/hardware.h>
#include <asm/arch/SA-1101.h>
#include <asm/hardware/sa1101.h>
#include <asm/mach/irq.h>
#include <asm/arch/irqs.h>
#include <asm/uaccess.h>
#include <asm/io.h>

/*
 * We keep the following data for the overall SA1101.  Note that the
 * struct device and struct resource are "fake"; they should be supplied
 * by the bus above us.  However, in the interests of getting all SA1101
 * drivers converted over to the device model, we provide this as an
 * anchor point for all the other drivers.
 */

static int sa1101_match(struct device *_dev, struct device_driver *_drv);
static int sa1101_bus_suspend(struct device *dev, u32 state);
static int sa1101_bus_resume(struct device *dev);

struct bus_type sa1101_bus_type = {
	.name           = "sa1101-bus",
	.match          = sa1101_match,
	.suspend        = sa1101_bus_suspend,
	.resume         = sa1101_bus_resume,
};

struct sa1101_dev_info {
	unsigned long	offset;
	unsigned long	skpcr_mask;
	unsigned int	devid;
	unsigned int	irq[6];
};

static struct sa1101_dev_info sa1101_devices[] = {
	{
		.offset		= SA1101_USB,
		.skpcr_mask	= SKPCR_UCLKEn,
		.devid		= SA1101_DEVID_USB,
		.irq = {
			IRQ_USBPWR,
			IRQ_HCIM,
			IRQ_HCIBUFFACC,
			IRQ_HCIRMTWKP,
			IRQ_NHCIMFCIR,
			IRQ_USB_PORT_RESUME
		},
	},
#if 0
	{
		.offset		= 0x0600,
		.skpcr_mask	= SKPCR_I2SCLKEN | SKPCR_L3CLKEN,
		.devid		= SA1111_DEVID_SAC,
		.irq = {
			AUDXMTDMADONEA,
			AUDXMTDMADONEB,
			AUDRCVDMADONEA,
			AUDRCVDMADONEB
		},
	},
	{
		.offset		= 0x0800,
		.skpcr_mask	= SKPCR_SCLKEN,
		.devid		= SA1111_DEVID_SSP,
	},
	{
		.offset		= SA1111_KBD,
		.skpcr_mask	= SKPCR_PTCLKEN,
		.devid		= SA1111_DEVID_PS2,
		.irq = {
			IRQ_TPRXINT,
			IRQ_TPTXINT
		},
	},
	{
		.offset		= SA1111_MSE,
		.skpcr_mask	= SKPCR_PMCLKEN,
		.devid		= SA1111_DEVID_PS2,
		.irq = {
			IRQ_MSRXINT,
			IRQ_MSTXINT
		},
	},
#endif
	{
		.offset		= SA1101_PCMCIA,
		.skpcr_mask	= 0,
		.devid		= SA1101_DEVID_PCMCIA,
		.irq = {
			IRQ_S0_READY_NINT,
			IRQ_S0_CD_VALID,
			IRQ_S0_BVD1_STSCHG,
			IRQ_S1_READY_NINT,
			IRQ_S1_CD_VALID,
			IRQ_S1_BVD1_STSCHG,
		},
	},
};

static void sa1101_dev_release(struct device *_dev)
{
	struct sa1101_dev *dev = SA1101_DEV(_dev);

	release_resource(&dev->res);
	kfree(dev);
}

static int
sa1101_init_one_child(struct device *parent, struct resource *parent_res,
		      struct sa1101_dev_info *info)
{
	struct sa1101_dev *dev;
	int ret;

	dev = kmalloc(sizeof(struct sa1101_dev), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		goto out;
	}
	memset(dev, 0, sizeof(struct sa1101_dev));

	snprintf(dev->dev.bus_id, sizeof(dev->dev.bus_id),
		 "%4.4lx", info->offset);

	dev->devid	 = info->devid;
	dev->dev.parent  = parent;
	dev->dev.bus     = &sa1101_bus_type;
	dev->dev.release = sa1101_dev_release;
	dev->dev.coherent_dma_mask = 0xffffffff;
	dev->res.start   = SA1101_BASE + info->offset;
	dev->res.end     = dev->res.start + 0x1ffff;
	dev->res.name    = dev->dev.bus_id;
	dev->res.flags   = IORESOURCE_MEM;
	dev->mapbase     = (u8 *)0xf4000000 + info->offset;
	dev->skpcr_mask  = info->skpcr_mask;
	memmove(dev->irq, info->irq, sizeof(dev->irq));
#if 1
	ret = request_resource(parent_res, &dev->res);
	if (ret) {
		printk("SA1101: failed to allocate resource for %s\n",
			dev->res.name);
		kfree(dev);
		goto out;
	}
#endif
	ret = device_register(&dev->dev);
	if (ret) {
		release_resource(&dev->res);
		kfree(dev);
		goto out;
	}

out:
	return ret;
}

static struct resource sa1101_resource = {
  .name   = "SA1101"
};

/*
 * Figure out whether we can see the SA1101
 */

int sa1101_probe(struct device * _dev)
{
	struct platform_device *pdev = to_platform_device(_dev);
	struct sa1101_dev *dev = SA1101_DEV(_dev);
	/* should check */
	int i;
	u32 has_devs;

	sa1101_wake();
	sa1101_init_irq(GPIO_JORNADA820_SA1101_CHAIN_IRQ);

	has_devs = 0xffffffff;

	for (i = 0; i < ARRAY_SIZE(sa1101_devices); i++)
		if (has_devs & (1 << i))
			sa1101_init_one_child(&dev->dev, &pdev->resource[0], &sa1101_devices[i]);

	return 0;
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
	INTSTATCLR1 = stat1;
	desc->chip->mask(irq);
	desc->chip->ack(irq);

	if (stat0 == 0 && stat1 == 0) {
		do_bad_IRQ(irq, desc, regs);
		return;
	}

	do {
	 for (i = IRQ_SA1101_START; stat0; i++, stat0 >>= 1)
		if (stat0 & 1)
			do_edge_IRQ(i, irq_desc + i, regs);

	 for (i = IRQ_SA1101_START + 32; stat1; i++, stat1 >>= 1)
		if (stat1 & 1)
			do_edge_IRQ(i, irq_desc + i, regs);

	 stat0 = INTSTATCLR0;
	 stat1 = INTSTATCLR1;	
	 INTSTATCLR0 = stat0;
	 INTSTATCLR1 = stat1;

	} while (stat0 | stat1); 

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
	INTENABLE1 &= ~SA1101_IRQMASK_HI(irq);
}
static void unmask_high_irq(unsigned int irq)
{
	INTENABLE1 |= SA1101_IRQMASK_HI(irq);
}
static int retrigger_high_irq(unsigned int irq)
{
	unsigned int mask = SA1101_IRQMASK_HI(irq);
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
	unsigned int mask = SA1101_IRQMASK_HI(irq);
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
	request_mem_region(SA1101_INTERRUPT, 0x1ffff, "irqs-1101"); // XXX - still needed???

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
//	request_irq(sa1101_irq, debug_sa1101_irq_handler, SA_INTERRUPT,
//		    "SA1101 chain interrupt", NULL); // DEBUG
	
	set_irq_type(sa1101_irq, IRQT_RISING);
	set_irq_data(sa1101_irq, (void *)SA1101_INTERRUPT);
	set_irq_chained_handler(sa1101_irq, sa1101_irq_handler); // NORMAL
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

	/* reset video memory controller */
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

#if 0
EXPORT_SYMBOL_GPL(sa1101_enable_device);
EXPORT_SYMBOL_GPL(sa1101_disable_device);
EXPORT_SYMBOL_GPL(sa1101_pll_clock);
#endif

static int sa1101_match(struct device *_dev, struct device_driver *_drv)
{
	struct sa1101_dev *dev = SA1101_DEV(_dev);
	struct sa1101_driver *drv = SA1101_DRV(_drv);
	
	return dev->devid == drv->devid;
}

static int sa1101_bus_suspend(struct device *dev, u32 state)
{
	struct sa1101_dev *sadev = SA1101_DEV(dev);
	struct sa1101_driver *drv = SA1101_DRV(dev->driver);
	int ret = 0;
	
	if (drv && drv->suspend)
		ret = drv->suspend(sadev, state);
	return ret;
}

static int sa1101_bus_resume(struct device *dev)
{
	struct sa1101_dev *sadev = SA1101_DEV(dev);
	struct sa1101_driver *drv = SA1101_DRV(dev->driver);
	int ret = 0;
	
	if (drv && drv->resume)
		ret = drv->resume(sadev);
	return ret;
}

static int sa1101_bus_probe(struct device *dev)
{
	struct sa1101_dev *sadev = SA1101_DEV(dev);
	struct sa1101_driver *drv = SA1101_DRV(dev->driver);
	int ret = -ENODEV;

	if (drv->probe)
		ret = drv->probe(sadev);
	return ret;
}

static int sa1101_bus_remove(struct device *dev)
{
	struct sa1101_dev *sadev = SA1101_DEV(dev);
	struct sa1101_driver *drv = SA1101_DRV(dev->driver);
	int ret = -ENODEV;
	
	if (drv->remove)
		ret = drv->remove(sadev);
	return ret;
}

static int sa1101_suspend(struct device *dev, u32 state, u32 level) {
	return 0;
}

static int sa1101_resume(struct device *dev, u32 level) {
	return 0;
}

static int sa1101_remove(struct device *dev)
{
	return 0;
}
	
static struct device_driver sa1101_device_driver = {
	.name       = "sa1101-bus",
	.bus        = &platform_bus_type,
	.probe      = sa1101_probe,
	.remove     = sa1101_remove,
	.suspend    = sa1101_suspend,
	.resume     = sa1101_resume,
};

int sa1101_driver_register(struct sa1101_driver *driver)
{
	WARN_ON(driver->drv.suspend || driver->drv.resume || driver->drv.probe || driver->drv.remove);
	driver->drv.probe = sa1101_bus_probe;
	driver->drv.remove = sa1101_bus_remove;
	driver->drv.bus = &sa1101_bus_type;
	return driver_register(&driver->drv);
}

void sa1101_driver_unregister(struct sa1101_driver *driver)
{
	driver_unregister(&driver->drv);
}

static int __init sa1101_init(void)
{
	int ret = bus_register(&sa1101_bus_type);

	if (ret == 0)
		driver_register(&sa1101_device_driver);
	return ret;
}

static void __exit sa1101_exit(void)
{
	driver_unregister(&sa1101_device_driver);
	bus_unregister(&sa1101_bus_type);
}

module_init(sa1101_init);
module_exit(sa1101_exit);

EXPORT_SYMBOL_GPL(sa1101_driver_register);
EXPORT_SYMBOL_GPL(sa1101_driver_unregister);

MODULE_DESCRIPTION("Main driver for SA-1101 companion chip.");
MODULE_LICENSE("GPL");
