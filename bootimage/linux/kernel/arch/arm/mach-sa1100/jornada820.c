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
 * However, one important thing we have to reset is the coprocessor's
 * PID register - setting this to 0 allows us to use normal Linux-ish
 * paging.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/mm.h>

#include <asm/hardware.h>
#include <asm/setup.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/serial_sa1100.h>
#include <asm/mach-types.h>

#include <asm/pgtable.h>
#include <asm/pgalloc.h>
#include <asm/page.h>
#include <asm/setup.h>
#include <asm/irq.h>

#include "generic.h"

#include <linux/linux_logo.h>

/* *********************************************************************** */
/* Initialize the Jornada 820.                                             */
/* *********************************************************************** */

static int __init jornada820_init(void)
{
  printk("In jornada820_init\n");
  /* we need to fix the PID register, which was left on by WinCE */
  asm volatile ("mov r0, #0; mcr p15, 0, r0, c13, c0,0;");

  /* set GPIO edge detection - GPIO14 is the chained interrupt from SA-1101 */
  set_GPIO_IRQ_edge(GPIO_GPIO14, (GPIO_FALLING_EDGE | GPIO_RISING_EDGE));

  set_GPIO_IRQ_edge(GPIO_GPIO1, GPIO_RISING_EDGE);

  /* Initialize the 1101. */
  GAFR |= GPIO_32_768kHz;
  GPDR |= GPIO_32_768kHz;
  TUCR = TUCR_3_6864MHz | TUCR_MBGPIO ; /* should also set TUCR_MR */
  
  /* ------------------- */
  /* Initialize the SSP  */
  /* ------------------- */
  
  Ser4MCCR0 &= ~MCCR0_MCE;       /* disable MCP */
  PPAR |= PPAR_SSPGPIO;          /* assign alternate pins to SSP */
  GAFR |= (GPIO_GPIO10 | GPIO_GPIO11 | GPIO_GPIO12 | GPIO_GPIO13);
  GPDR |= (GPIO_GPIO10 | GPIO_GPIO12 | GPIO_GPIO13);
  GPDR &= ~GPIO_GPIO11;

  if (ssp_init()) printk("ssp_init() failed\n");
  
  Ser4SSCR0 = /* SSCR0_DataSize(8) | SSCR0_Motorola |  */
    0x0387; /* 8 bit Motorola, 460800 bit rate */
  Ser4SSCR1 = SSCR1_RIE | SSCR1_SClkIactH | SSCR1_SClk1_2P;
  ssp_enable();

  /*
  printk ("Old value of GPDR = %x\n", GPDR);
  printk ("Old value of VMCCR=%x\n", VMCCR);
  printk ("Old value of UFCR=%x\n", UFCR);

  printk ("Old value of VideoControl=%x\n", VideoControl);
  printk ("Old value of VgaTiming0=%x\n", VgaTiming0);
  printk ("Old value of VgaTiming1=%x\n", VgaTiming1);
  printk ("Old value of VgaTiming2=%x\n", VgaTiming2);
  printk ("Old value of VgaTiming3=%x\n", VgaTiming3);
  printk ("Old value of VgaBorder=%x\n", VgaBorder);
  printk ("Old value of VgaDBAR=%x\n", VgaDBAR);
  printk ("Old value of VgaDCAR=%x\n", VgaDCAR);
  printk ("Old value of VgaStatus=%x\n", VgaStatus);
  printk ("Old value of VgaInterruptMask=%x\n", VgaInterruptMask);
  printk ("Old value of VgaPalette=%x\n", VgaPalette);
  //   printk ("Old value of DacControl=%x\n", DacControl);
  printk ("Old value of VgaTest=%x\n", VgaTest);
  */


  /* This should reset the SA1101 
  GPDR |= GPIO_GPIO20;
  GPSR = GPIO_GPIO20;
  udelay(1);
  GPCR = GPIO_GPIO20;
  udelay(1);
  */
  
  sa1101_wake(SA1101_BASE);
  sa1101_init_irq (IRQ_JORNADA820_SA1101_CHAIN);

  jornada820_init_proc();

  /* Until I'm sure that the 1ms 16-bit counter setup by Wince at 0xc05c080
   * has been disabled, I'm keeping this page out of Linux' memory pool -- fare
   */
  // request_mem_region (0xc005c000, 0x1000, "memory fux0red by 1 kHz clock");
  /* galmasi declares the whole first 2 MBytes of RAM as dead */
  // request_mem_region (0xc0000000, 0x200000, "dead-memory");
  /* Actually, to be surer, he does it in a #ifdef CONFIG_SA1100_JORNADA820
   * within arch/arm/mm/init.c -- an ugly hack, but it's done early on
   */

  return 0;
}

__initcall(jornada820_init);

/* *********************************************************************** */
/*           temporary: setup the ramdisk                                  */
/* *********************************************************************** */

static void __init
fixup_jornada820(struct machine_desc *desc, struct param_struct *params,
		 char **cmdline, struct meminfo *mi)
{
  SET_BANK( 0, 0xc0000000, 16*1024*1024 );
  mi->nr_banks = 1;

  printk("fixup_jornada820: initrd at %x, %d bytes\n",
	 params->u1.s.initrd_start,
	 params->u1.s.initrd_size);
  setup_initrd(params->u1.s.initrd_start, params->u1.s.initrd_size);
}

/* *********************************************************************** */
/*              map Jornada 820-specific IO (think SA1101)                 */
/* *********************************************************************** */

static struct map_desc jornada820_io_desc[] __initdata = {
  /* virtual     physical    length      domain     r  w  c  b */
  { 0xf4000000, 0x18000000, 0x00400000, DOMAIN_IO, 0, 1, 0, 0 }, /* SA-1101 */
  LAST_DESC
};

static void __init jornada820_map_io(void)
{
  printk("In jornada820_map_io\n");
  sa1100_map_io();
  iotable_init(jornada820_io_desc);
  
  /* these are a wild guess */
  sa1100_register_uart(0, 3);
  sa1100_register_uart(1, 1);
  /*sa1100_register_uart(2, 2);*/
}

MACHINE_START(JORNADA820, "HP Jornada 820")
     BOOT_MEM(0xc0000000, 0x80000000, 0xf8000000)
     BOOT_PARAMS(0xc0200100)
     FIXUP(fixup_jornada820)
     MAPIO(jornada820_map_io)
     INITIRQ(sa1100_init_irq)
     SOFT_REBOOT
     MAINTAINER(galmasi@optonline.net)
MACHINE_END

/* *********************************************************************** */
/* machine specific proc file system                                       */
/* *********************************************************************** */


#include <linux/proc_fs.h>
#include <asm/uaccess.h>

static struct proc_dir_entry *j820_dir, *parent_dir = NULL;

#define PROC_NAME "j820"

static int j820_read_proc(char *buf,
			  char **start,
			  off_t pos,
			  int count,
			  int *eof,
			  void *data)
{
  char *p = buf;
  p += sprintf(p, "\t Contrast  = %u\n", DAC_JORNADA820_CONTRAST);
  p += sprintf(p, "\t Brightness= %u\n", DAC_JORNADA820_BRIGHTNESS);
  p += sprintf(p, "\t GPLR      = %08x\n", GPLR);
  p += sprintf(p, "\t PADRR     = %08x\n", PADRR);
  p += sprintf(p, "\t BBDRR     = %08x\n", PBDRR);
  return (p-buf);
}

static int j820_write_proc (struct file *file,
			    const char *buffer,
			    unsigned long count,
			    void *data)
{
  char buf[260]; int val;
  if (count > 258) return -EINVAL;
  if (copy_from_user(buf, buffer, count)) return -EFAULT;
  if (!strncmp(buf, "Contrast", 8))
    {
      unsigned val;
      sscanf(buf+8, "%d", &val);
      DAC_JORNADA820_CONTRAST = val;
    }
  if (!strncmp(buf, "Brightness", 10))
    {
      unsigned val;
      sscanf (buf+10, "%d", &val);
      DAC_JORNADA820_BRIGHTNESS = val;
    }
  if (!strncmp(buf, "Backlight", 9))
    {
      unsigned val;
      sscanf (buf+9, "%d", &val);
      if (val) GPSR = GPIO_JORNADA820_BACKLIGHTON;
      else     GPCR = GPIO_JORNADA820_BACKLIGHTON;
    }
  return count;
}

static void jornada820_init_proc ()
{
  j820_dir = create_proc_entry ("j820", 0, parent_dir);
  if (j820_dir == NULL)
    {
      printk("jornada820_init_proc failed\n");
      return;
    }
  j820_dir->read_proc = j820_read_proc;
  j820_dir->write_proc = j820_write_proc;
}
