/*
 * linux/arch/arm/mach-sa1100/jornada820.c
 * Jornada 820 fixup and initialization code.
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Modelled after the Jornada 720 code.
 * 
 * $Id: jornada820.c,v 1.11 2004/07/14 20:21:42 fare Exp $
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/serial_sa1100.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/delay.h>
#include <linux/device.h>
#include "generic.h"


static struct resource sa1101_resources[] = {
	[0] = {
		.start  = JORNADA820_SA1101_BASE,
		.end    = 0x1bffffff,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device sa1101_device = {
	.name       = "sa1101-bus",
	.id     = 0,
	.num_resources  = ARRAY_SIZE(sa1101_resources),
	.resource   = sa1101_resources,
};

static struct platform_device *devices[] __initdata = {
	&sa1101_device,
};


/* *********************************************************************** */
/* Initialize the Jornada 820.                                             */
/* *********************************************************************** */

static int __init jornada820_init(void)
{
  printk("In jornada820_init\n");

  /* ------------------- */
  /* Initialize the SSP  */
  /* ------------------- */
  
  Ser4MCCR0 &= ~MCCR0_MCE;       /* disable MCP */
  PPAR |= PPAR_SSPGPIO;          /* assign alternate pins to SSP */
  GAFR |= (GPIO_GPIO10 | GPIO_GPIO11 | GPIO_GPIO12 | GPIO_GPIO13);
  GPDR |= (GPIO_GPIO10 | GPIO_GPIO12 | GPIO_GPIO13);
  GPDR &= ~GPIO_GPIO11;

  /* we mess with the SSCR0 directly, because there is no ssp_setreg() API, that
     can be called by the keyboard driver */

    /* 8 bit, Motorola, enable, 460800 bit rate */
  Ser4SSCR0 = SSCR0_DataSize(8)+SSCR0_Motorola+SSCR0_SSE+SSCR0_SerClkDiv(8);
//  Ser4SSCR1 = SSCR1_RIE | SSCR1_SClkIactH | SSCR1_SClk1_2P;

  Ser4MCCR0 |= MCCR0_MCE;       /* reenable MCP */

  /* we can't set the audio divisor here. It will be reset by the generic MCP code. */
  /* TODO: j820_audio.c driver should take care of this problem */

  return platform_add_devices(devices, ARRAY_SIZE(devices));

}

__initcall(jornada820_init);

/* *********************************************************************** */
/*              map Jornada 820-specific IO (think SA1101)                 */
/* *********************************************************************** */

static struct map_desc jornada820_io_desc[] __initdata = {
  /* virtual     physical    length      type */
  { 0xf5000000, 0, 0x01000000, MT_DEVICE }, /* Boot Rom */
  { 0xf4000000, JORNADA820_SA1101_BASE, 0x00400000, MT_DEVICE } /* SA-1101 */
};

static void __init jornada820_map_io(void)
{
  sa1100_map_io();
  iotable_init(jornada820_io_desc, ARRAY_SIZE(jornada820_io_desc));
  
  /* rs232 out */
  sa1100_register_uart(0, 3);

  /* internal diag. */
  sa1100_register_uart(1, 1);
}

MACHINE_START(JORNADA820, "HP Jornada 820")
     BOOT_MEM(0xc0000000, 0x80000000, 0xf8000000)
     BOOT_PARAMS(0xc0200100)
     MAPIO(jornada820_map_io)
     INITIRQ(sa1100_init_irq)
     SOFT_REBOOT
     MAINTAINER(galmasi@optonline.net)
MACHINE_END
