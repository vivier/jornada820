/*
 * linux/arch/arm/mach-sa1100/jornada820.c
 * Jornada 820 fixup and initialization code.
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Modelled after the Jornada 720 code.
 * 
 * The Jornada 820 is never cold-booted into Linux. Thus, we do not
 * re-initialize everything, but lazily allow certain pre-set WinCE
 * things to continue functioning.
 *
 * This dependance will be removed soon ? (01.05.2004)
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/serial_sa1100.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/hardware/ssp.h>
#include <asm/delay.h>
#include "generic.h"

/* *********************************************************************** */
/* Initialize the Jornada 820.                                             */
/* *********************************************************************** */

static int __init jornada820_init(void)
{
  printk("In jornada820_init\n");

  /* allow interrupts: */
  /* audio et al. */
  set_GPIO_IRQ_edge(GPIO_JORNADA820_UCB1200,      GPIO_RISING_EDGE);
  /* sa1101 mux */
  set_GPIO_IRQ_edge(GPIO_JORNADA820_SA1101_CHAIN, GPIO_RISING_EDGE);

#if 0
  /* TODO: write the drivers to use these events */
  /*  ser1 */
  set_GPIO_IRQ_edge(GPIO_JORNADA820_POWERD,       GPIO_RISING_EDGE|GPIO_FALLING_EDGE);
  /*  serial port */
  set_GPIO_IRQ_edge(GPIO_JORNADA820_SERIAL,       GPIO_RISING_EDGE|GPIO_FALLING_EDGE);
  /*  serial what ? */
  set_GPIO_IRQ_edge(GPIO_GPIO(18),                GPIO_RISING_EDGE|GPIO_FALLING_EDGE);
  /*  ledbutton */
  set_GPIO_IRQ_edge(GPIO_JORNADA820_LEDBUTTON,    GPIO_RISING_EDGE);
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

  sa1101_probe(SA1101_BASE);
  sa1101_wake();
  sa1101_init_irq (GPIO_JORNADA820_SA1101_CHAIN_IRQ);


  /*
   * TODO: don't forget to remove the code from "arch/arm/mm/init.c".
   */

  return 0;
}

__initcall(jornada820_init);


int jornada_contrast(int arg_contrast)
{
        DAC_JORNADA820_CONTRAST = arg_contrast;
        return arg_contrast;
}
EXPORT_SYMBOL(jornada_contrast);

int jornada_brightness(int arg_brightness)
{
        DAC_JORNADA820_BRIGHTNESS = arg_brightness;
        return arg_brightness;
}
EXPORT_SYMBOL(jornada_brightness);


/* *********************************************************************** */
/*              map Jornada 820-specific IO (think SA1101)                 */
/* *********************************************************************** */

static struct map_desc jornada820_io_desc[] __initdata = {
  /* virtual     physical    length      domain     r  w  c  b */
  { 0xf4000000, 0x18000000, 0x00400000, DOMAIN_IO, 1, 1, 0, 0 }, /* SA-1101 */
  LAST_DESC
};

static void __init jornada820_map_io(void)
{
  sa1100_map_io();
  iotable_init(jornada820_io_desc);
  
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
