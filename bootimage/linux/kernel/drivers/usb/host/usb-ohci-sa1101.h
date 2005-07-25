/*
 * usb-ohci-sa1101.h
 *
 * definitions and special code for Intel SA-1101 USB OHCI host controller
 *
 * Largely inspired by the SA-1111 USB OHCI driver.
 *
 */

/* arch/arm/mach-sa1100/sa1111.c */
void sa1101_ohci_hcd_cleanup(void);

static void
ohci_non_pci_cleanup(void)
{
	sa1101_ohci_hcd_cleanup();
}

/* Make the remaining of the code happy */
#ifndef CONFIG_PCI
#define CONFIG_PCI
#include "../../arch/arm/mach-sa1100/pcipool.h"
#endif

