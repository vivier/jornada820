/*
 *  linux/drivers/usb/usb-ohci-sa1101.c
 *
 *  Largely inspired by the usb-ohci-sa1111 driver.
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/usb.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include "../../../arch/arm/mach-sa1100/pcipool.h"

#include "usb-ohci.h"

#define SA1101_FAKE_PCIDEV ((struct pci_dev *) 1101)

int __devinit
hc_add_ohci(struct pci_dev *dev, int irq, void *membase, unsigned long flags,
	    ohci_t **ohci, const char *name, const char *slot_name);
extern void hc_remove_ohci(ohci_t *ohci);

static ohci_t *sa1101_ohci;

static void __init sa1101_ohci_configure(void)
{
	unsigned int usb_rst = 0;

	
	USBReset = usb_rst | USBReset_ForceIfReset | USBReset_ForceHcReset;
	
	/*
	 * Now, carefully enable the USB clock, and take
	 * the USB host controller out of reset.
	 */
	
	SKPCR |= SKPCR_UCLKEn;
	udelay(11);
	
	/*
	 * Stop the USB clock.
	 */
	
	SKPCR &= ~SKPCR_UCLKEn;
	USBReset &= ~USBReset_ForceIfReset;
	
	SKPCR |= SKPCR_UCLKEn;
	USTCSR = 0x00000800;
	USBReset &= ~USBReset_ForceHcReset;

	INTPOL1 |= 0x100000;
}

static int __init sa1101_ohci_init(void)
{
	int ret;

	/*
	 * Request memory resources.
	 */
//	if (!request_mem_region(_USB_OHCI_OP_BASE, _USB_EXTENT, "usb-ohci"))
//		return -EBUSY;

	sa1101_ohci_configure();

	/*
	 * Initialise the generic OHCI driver.
	 */
	ret = hc_add_ohci(SA1101_FAKE_PCIDEV, NIRQHCIM,
			  (void *)0, 0, &sa1101_ohci,
			  "usb-ohci", "sa1101");

//	if (ret)
//		release_mem_region(_USB_OHCI_OP_BASE, _USB_EXTENT);


	return ret;
}

static void __exit sa1101_ohci_exit(void)
{
	hc_remove_ohci(sa1101_ohci);

	/*
	 * Put the USB host controller into reset.
	 */
	USBReset |= USBReset_ForceIfReset | USBReset_ForceHcReset;

	/*
	 * Stop the USB clock.
	 */
	SKPCR &= ~SKPCR_UCLKEn;

	/*
	 * Release memory resources.
	 */
//	release_mem_region(_USB_OHCI_OP_BASE, _USB_EXTENT);

}

module_init(sa1101_ohci_init);
module_exit(sa1101_ohci_exit);
MODULE_DESCRIPTION("SA1101 USB driver");
MODULE_LICENSE("GPL");
