/*
 * linux/arch/arm/mach-sa1100/jornada820.c
 * Jornada 820 fixup and initialization code.
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Modelled after the Jornada 720 code.
 * 
 * $Id: jornada820.c,v 1.16 2005/07/29 11:09:24 fare Exp $
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/serial_sa1100.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/hardware/sa1101.h>
#include <asm/hardware/ssp.h>
#include <asm/delay.h>
#include "generic.h"


static struct resource sa1101_resources[] = {
	[0] = {
		.start  = JORNADA820_SA1101_BASE,
		.end    = JORNADA820_SA1101_BASE+0x3ffffff,
		.flags  = IORESOURCE_MEM,
	},
/* DO WE NEED SOMETHING LIKE THIS? (Excerpted from jornada720.c)
        [1] = {
                .start          = IRQ_GPIO1,
                .end            = IRQ_GPIO1,
                .flags          = IORESOURCE_IRQ,
        },
*/
};

/* jornada720 has something about dma mask... */

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

  /* allow interrupts: */
  /* audio et al. */
  set_irq_type(GPIO_JORNADA820_UCB1200_IRQ, IRQT_RISING);
  /* sa1101 mux */
  set_irq_type(GPIO_JORNADA820_SA1101_CHAIN_IRQ, IRQT_RISING);

#if 0
  /* TODO: write the drivers to use these events */
  /*  ser1 */
  set_irq_type(GPIO_JORNADA820_POWERD_IRQ, IRQT_PROBE);
  /*  serial port */
  set_irq_type(GPIO_JORNADA820_SERIAL_IRQ, IRQT_PROBE);
  /*  serial what ? */
  set_irq_type(GPIO_JORNADA820_DTRDSR_IRQ, IRQT_PROBE);
  /*  ledbutton */
  set_irq_type(GPIO_JORNADA820_LEDBUTTON_IRQ, IRQT_RISING);
#endif

#if 0
  /* TODO: This should reset the SA1101 
  GPDR |= GPIO_GPIO20;
  GPSR = GPIO_GPIO20;
  udelay(1);
  GPCR = GPIO_GPIO20;
  udelay(1);
  */
#endif

  /* ------------------- */
  /* Initialize the SSP  */
  /* ------------------- */
  
  Ser4MCCR0 &= ~MCCR0_MCE;       /* disable MCP */
  PPAR |= PPAR_SSPGPIO;          /* assign alternate pins to SSP */
  GAFR |= (GPIO_GPIO10 | GPIO_GPIO11 | GPIO_GPIO12 | GPIO_GPIO13);
  GPDR |= (GPIO_GPIO10 | GPIO_GPIO12 | GPIO_GPIO13);
  GPDR &= ~GPIO_GPIO11;

  if (ssp_init()) printk("ssp_init() failed.\n");

  /* we mess with the SSCR0 directly, because there is no ssp_setreg() API, that
     can be called by the keyboard driver */

    /* 8 bit, Motorola, enable, 460800 bit rate */
  Ser4SSCR0 = SSCR0_DataSize(8)+SSCR0_Motorola+SSCR0_SSE+SSCR0_SerClkDiv(8);
//  Ser4SSCR1 = SSCR1_RIE | SSCR1_SClkIactH | SSCR1_SClk1_2P;

  ssp_enable();

  Ser4MCCR0 |= MCCR0_MCE;       /* reenable MCP */
				/* XXX - Not in matan's kernel(?) */

  /* Initialize the 1101. */
  GAFR |= GPIO_32_768kHz;
  GPDR |= GPIO_32_768kHz;

  TUCR = TUCR_3_6864MHz; /* */

//  sa1101_probe(sa1101_device); // should be done by the device management
  sa1101_wake();
  sa1101_init_irq (GPIO_JORNADA820_SA1101_CHAIN_IRQ);

  return platform_add_devices(devices, ARRAY_SIZE(devices));

}

arch_initcall(jornada820_init);

/* *********************************************************************** */
/*              map Jornada 820-specific IO (think SA1101)                 */
/* *********************************************************************** */

static struct map_desc jornada820_io_desc[] __initdata = {
  /* virtual     physical    length      type */
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
//   .init_machine   = jornada820_init,
     .timer          = &sa1100_timer,
     SOFT_REBOOT
     MAINTAINER("http://jornada820.sourceforge.net/")
MACHINE_END
