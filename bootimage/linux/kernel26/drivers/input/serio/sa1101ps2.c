/*
 * linux/drivers/input/serio/sa1101ps2.c
 *
 * 2004/06/30 Matan Ziv-Av <matan@svgalib.org>
 *
 * Based on rmk's sa1111ps2.c from 2.6 and galmasi's j820_keyb.c from 2.4
 *
 * $Id: sa1101ps2.c,v 1.4 2005/07/25 09:09:14 fare Exp $
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/serio.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>

#include <asm/arch/SA-1101.h>
#include <asm/hardware/sa1101.h>

struct ps2if {
	struct serio		*io;
//	struct sa1101_dev	*dev;
	unsigned long		base;
	unsigned int		open;
	spinlock_t		lock;
	unsigned int		head;
	unsigned int		tail;
	unsigned char		buf[4];
} *sps2if;

/*
 * Read all bytes waiting in the PS2 port.  There should be
 * at the most one, but we loop for safety.  If there was a
 * framing error, we have to manually clear the status.
 */
static irqreturn_t ps2_rxint(int irq, void *dev_id, struct pt_regs *regs)
{
	struct ps2if *ps2if = dev_id;
	unsigned int scancode, flag, status;
	int handled = IRQ_NONE;

	status = sa1101_readl(ps2if->base + SA1101_PS2STAT);
	while (status & PS2STAT_RXF) {
/* 1101 does not have this bit 
		if (status & PS2STAT_STP)
			sa1101_writel(PS2STAT_STP, ps2if->base + SA1101_PS2STAT);
*/
		flag = /* (status & PS2STAT_STP ? SERIO_FRAME : 0) | */
		       (status & PS2STAT_RXP ? 0 : SERIO_PARITY);

		scancode = sa1101_readl(ps2if->base + SA1101_PS2DATA) & 0xff;

		if (hweight8(scancode) & 1)
			flag ^= SERIO_PARITY;

		serio_interrupt(ps2if->io, scancode, flag, regs);

		status = sa1101_readl(ps2if->base + SA1101_PS2STAT);

		handled = IRQ_HANDLED;
        }

        return handled;
}

/*
 * Completion of ps2 write
 */
static irqreturn_t ps2_txint(int irq, void *dev_id, struct pt_regs *regs)
{
	struct ps2if *ps2if = dev_id;
	unsigned int status;

	spin_lock(&ps2if->lock);
	status = sa1101_readl(ps2if->base + SA1101_PS2STAT);
	if (ps2if->head == ps2if->tail) {
		disable_irq(irq);
		/* done */
	} else if (status & PS2STAT_TXE) {
		sa1101_writel(ps2if->buf[ps2if->tail], ps2if->base + SA1101_PS2DATA);
		ps2if->tail = (ps2if->tail + 1) & (sizeof(ps2if->buf) - 1);
	}
	spin_unlock(&ps2if->lock);

	return IRQ_HANDLED;
}

/*
 * Write a byte to the PS2 port.  We have to wait for the
 * port to indicate that the transmitter is empty.
 */
static int ps2_write(struct serio *io, unsigned char val)
{
	struct ps2if *ps2if = io->port_data;
	unsigned long flags;
	unsigned int head;

	spin_lock_irqsave(&ps2if->lock, flags);

	/*
	 * If the TX register is empty, we can go straight out.
	 */
	if (sa1101_readl(ps2if->base + SA1101_PS2STAT) & PS2STAT_TXE) {
		sa1101_writel(val, ps2if->base + SA1101_PS2DATA);
	} else {
		if (ps2if->head == ps2if->tail)
			enable_irq(IRQ_SA1101_TPTXINT);
		head = (ps2if->head + 1) & (sizeof(ps2if->buf) - 1);
		if (head != ps2if->tail) {
			ps2if->buf[ps2if->head] = val;
			ps2if->head = head;
		}
	}

	spin_unlock_irqrestore(&ps2if->lock, flags);
	return 0;
}

static int ps2_open(struct serio *io)
{
	struct ps2if *ps2if = io->port_data;
	int ret;

	printk("Opening sa1101ps2 device...\n"); // DEBUG
	
	ret = request_irq(IRQ_SA1101_TPRXINT, ps2_rxint, 0,
			  "sa1101ps2rx", ps2if);
	if (ret) {
		printk(KERN_ERR "sa1101ps2: could not allocate IRQ%d: %d\n",
			IRQ_SA1101_TPRXINT, ret);
		return ret;
	}

	ret = request_irq(IRQ_SA1101_TPTXINT, ps2_txint, 0, "sa1101ps2tx", ps2if);
	if (ret) {
		printk(KERN_ERR "sa1101ps2: could not allocate IRQ%d: %d\n",
			IRQ_SA1101_TPTXINT, ret);
		free_irq(IRQ_SA1101_TPRXINT, ps2if);
		return ret;
	}
	printk("sa1101ps2: registered interrupts %d and %d\n",
	       IRQ_SA1101_TPRXINT, IRQ_SA1101_TPTXINT); // DEBUG

	ps2if->open = 1;

	enable_irq_wake(IRQ_SA1101_TPRXINT);

	sa1101_writel(PS2CR_ENA, ps2if->base + SA1101_PS2CR);
	return 0;
}

static void ps2_close(struct serio *io)
{
	struct ps2if *ps2if = io->port_data;

	sa1101_writel(0, ps2if->base + SA1101_PS2CR);

	disable_irq_wake(IRQ_SA1101_TPRXINT);

	ps2if->open = 0;

	free_irq(IRQ_SA1101_TPTXINT, ps2if);
	free_irq(IRQ_SA1101_TPRXINT, ps2if);

}

/*
 * Clear the input buffer.
 */
static void __init ps2_clear_input(struct ps2if *ps2if)
{
	int maxread = 100;

	while (maxread--) {
		if ((sa1101_readl(ps2if->base + SA1101_PS2DATA) & 0xff) == 0xff)
			break;
	}
}

static inline unsigned int
ps2_test_one(struct ps2if *ps2if, unsigned int mask)
{
	unsigned int val;

	sa1101_writel(PS2CR_ENA | mask, ps2if->base + SA1101_PS2CR);

	udelay(2);

	val = sa1101_readl(ps2if->base + SA1101_PS2STAT);
	return val & (PS2STAT_KBC | PS2STAT_KBD);
}

/*
 * Test the keyboard interface.  We basically check to make sure that
 * we can drive each line to the keyboard independently of each other.
 */
static int __init ps2_test(struct ps2if *ps2if)
{
	unsigned int stat;
	int ret = 0;

	printk("ps2_test base=%lx, ps2cr=%lx\n", ps2if->base, ps2if->base+SA1101_PS2CR); // DEBUG
	stat = ps2_test_one(ps2if, PS2CR_FKC);
	if (stat != PS2STAT_KBD) {
		printk("PS/2 interface test failed[1]: %02x\n", stat);
		ret = -ENODEV;
	}

	stat = ps2_test_one(ps2if, 0);
	if (stat != (PS2STAT_KBC | PS2STAT_KBD)) {
		printk("PS/2 interface test failed[2]: %02x\n", stat);
		ret = -ENODEV;
	}

	stat = ps2_test_one(ps2if, PS2CR_FKD);
	if (stat != PS2STAT_KBC) {
		printk("PS/2 interface test failed[3]: %02x\n", stat);
		ret = -ENODEV;
	}

	sa1101_writel(0, ps2if->base + SA1101_PS2CR);

	return ret;
}

/*
 * Add one device to this driver.
 */
static int __init ps2_probe(void) // struct sa1101_dev *dev
{
	struct ps2if *ps2if;
	struct serio *serio;
	int ret;

	ps2if = kmalloc(sizeof(struct ps2if), GFP_KERNEL);
	serio = kmalloc(sizeof(struct serio), GFP_KERNEL);
	if (!ps2if || !serio) {
		ret = -ENOMEM;
		goto free;
	}

	memset(ps2if, 0, sizeof(struct ps2if));
	memset(serio, 0, sizeof(struct serio));

	serio->type		= SERIO_8042;
	serio->write		= ps2_write;
	serio->open		= ps2_open;
	serio->close		= ps2_close;
//	strlcpy(serio->name, dev->dev.bus_id, sizeof(serio->name));
//	strlcpy(serio->phys, dev->dev.bus_id, sizeof(serio->phys));
	serio->port_data	= ps2if;
//	serio->dev.parent	= &dev->dev;
	ps2if->io		= serio;
//	ps2if->dev		= dev;
//	sa1101_set_drvdata(dev, ps2if);

	spin_lock_init(&ps2if->lock);

	/*
	 * Request the physical region for this PS2 port.
	 */
//	if (!request_mem_region(dev->res.start,
//				dev->res.end - dev->res.start + 1,
//				SA1101_DRIVER_NAME(dev))) {
//		ret = -EBUSY;
//		goto free;
//	}

	/*
	 * Our parent device has already mapped the region.
	 */
	ps2if->base = 0xf4000000 + __TRACK_INTERFACE;

//	sa1101_enable_device(ps2if->dev);

	/* Incoming clock is 8MHz */
	sa1101_writel(0, ps2if->base + SA1101_PS2CLKDIV);
	sa1101_writel(127, ps2if->base + SA1101_PS2PRECNT);

	/*
	 * Flush any pending input.
	 */
	ps2_clear_input(ps2if);

	/*
	 * Test the keyboard interface.
	 */
	ret = ps2_test(ps2if);
	if (ret)
		goto out;

	/*
	 * Flush any pending input.
	 */
	ps2_clear_input(ps2if);

//	sa1101_disable_device(ps2if->dev);
	printk("registering sa1101ps2 port...\n"); // DEBUG
	serio_register_port(ps2if->io);
	printk("registered sa1101ps2 port.\n"); // DEBUG
	return 0;

 out:
//	sa1101_disable_device(ps2if->dev);
//	release_mem_region(dev->res.start,
//			   dev->res.end - dev->res.start + 1);
 free:
//	sa1101_set_drvdata(dev, NULL);
	kfree(ps2if);
	kfree(serio);
	return ret;
}

/*
 * Remove one device from this driver.
 */
static void __exit ps2_remove(void)
{

	printk("Unregistering sa1101ps2 port...\n"); // DEBUG
	serio_unregister_port(sps2if->io);
//	release_mem_region(dev->res.start,
//			   dev->res.end - dev->res.start + 1);
//	sa1101_set_drvdata(dev, NULL);

	kfree(sps2if);
}

module_init(ps2_probe);
module_exit(ps2_remove);

MODULE_AUTHOR("Matan Ziv-Av <matan@svgalib.org>");
MODULE_DESCRIPTION("SA1101 PS2 controller driver");
MODULE_LICENSE("GPL");
