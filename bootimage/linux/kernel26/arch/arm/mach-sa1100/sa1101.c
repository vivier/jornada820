/*
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Main driver for the SA1101 chip.
 * Modelled after the file sa1111.c
 *
 * Created for the Jornada820 port.
 * $Id: sa1101.c,v 1.1 2004/06/24 16:58:36 fare Exp $
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
#include <asm/hardware/sa1101.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/arch/irq.h>
#include <asm/uaccess.h>

#include <asm/io.h>

/* TODO: remove */
static void jornada820_init_proc (void);

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
  int irq;
  
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

static struct sa1101_pcmcia_irqs 
{
  int irq;
  const char *str;
} sa1101_pcmcia_irqs[] = {
  { IRQ_SA1101_S0_CDVALID,     "PCMCIA card detect" },
  { IRQ_SA1101_S0_BVD1_STSCHG, "PCMCIA BVD1" },
  { IRQ_SA1101_S1_CDVALID,     "CF card detect" },
  { IRQ_SA1101_S1_BVD1_STSCHG, "CF BVD1" },
};

/* ************************************************************************* */
/*  Initialize the PCMCIA subsystem: turn on interrupts, reserve memory      */
/* ************************************************************************* */

int sa1101_pcmcia_init(void *handler)
{
  int i, ret;

  if (!request_mem_region(_PCCR, 512, "PCMCIA")) return -1;
  
  for (i = ret = 0; i < ARRAY_SIZE(sa1101_pcmcia_irqs); i++)
    {
      ret = request_irq(sa1101_pcmcia_irqs[i].irq, 
			handler, 
			SA_INTERRUPT,
			sa1101_pcmcia_irqs[i].str, 
			NULL);
      if (ret)break;
    }


  if (i < ARRAY_SIZE(sa1101_pcmcia_irqs)) 
    {
      printk(KERN_ERR "sa1101_pcmcia: unable to grab IRQ%d (%d)\n",
	     sa1101_pcmcia_irqs[i].irq, ret);
      while (i--) 
       free_irq(sa1101_pcmcia_irqs[i].irq, NULL);
      release_mem_region(_PCCR, 16);
    }

  INTPOL1 |= (SA1101_IRQMASK_HI(IRQ_SA1101_S0_CDVALID) |
              SA1101_IRQMASK_HI(IRQ_SA1101_S1_CDVALID) |
              SA1101_IRQMASK_HI(IRQ_SA1101_S0_BVD1_STSCHG) |
              SA1101_IRQMASK_HI(IRQ_SA1101_S1_BVD1_STSCHG));

  return ret ? -1 : 2;
}

/* ************************************************************************* */
/*      Shut down PCMCIA: release memory, interrupts.                        */
/* ************************************************************************* */

int sa1101_pcmcia_shutdown(void)
{
  int i;
  for (i = 0; i < ARRAY_SIZE(sa1101_pcmcia_irqs); i++) 
   free_irq(sa1101_pcmcia_irqs[i].irq, NULL);

  INTPOL1 &= ~(SA1101_IRQMASK_HI(IRQ_SA1101_S0_CDVALID) |
	       SA1101_IRQMASK_HI(IRQ_SA1101_S1_CDVALID) |
	       SA1101_IRQMASK_HI(IRQ_SA1101_S0_BVD1_STSCHG) |
	       SA1101_IRQMASK_HI(IRQ_SA1101_S1_BVD1_STSCHG));

  release_mem_region(_PCCR, 512);
  return 0;
}

static int sa1101_usb_shutdown(void)
{
  /* not yet */
 return 0;
}

static int sa1101_vga_shutdown(void)
{
  /* not yet */
 return 0;
}

static int sa1101_usb_init(void)
{
  /* not yet */
 return 0;
}

static int sa1101_vga_init(void)
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

   /* TODO: remove */
  jornada820_init_proc();

}


/* ************************************************************************ */
/*        This is the place for PM code on this device.                     */
/* ************************************************************************ */

void sa1101_doze(void)
{
  /* not implemented */
	printk("SA1101 doze mode not implemented\n");
}

/* TODO: remove **********************************************************************************/

int j820_apm_get_power_status(u_char *ac_line_status, u_char *battery_status, u_char *battery_flag, 
                              u_char *battery_percentage, u_short *battery_life)
{
	*ac_line_status=(PBDRR & (1<<5)) ? 0:1;

	/* TODO: */
	*battery_status=0xff;
	*battery_flag=0xff;
	*battery_percentage=0xff;
	*battery_life=0xff;

 return 0;
}

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
  p += sprintf(p, "\t PADRR     = %08x\n", PADRR);
  p += sprintf(p, "\t PBDRR     = %08x\n", PBDRR);
  return (p-buf);
}

static int j820_write_proc (struct file *file,
			    const char *buffer,
			    unsigned long count,
			    void *data)
{
  char buf[260];
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

static void jornada820_init_proc (void)
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

/*********************************************************************************/

EXPORT_SYMBOL_GPL(sa1101_wake);
EXPORT_SYMBOL_GPL(sa1101_doze);
EXPORT_SYMBOL_GPL(sa1101_pcmcia_init);
EXPORT_SYMBOL_GPL(sa1101_pcmcia_shutdown);
EXPORT_SYMBOL_GPL(sa1101_usb_init);
EXPORT_SYMBOL_GPL(sa1101_usb_shutdown);
EXPORT_SYMBOL_GPL(sa1101_vga_init);
EXPORT_SYMBOL_GPL(sa1101_vga_shutdown);

MODULE_DESCRIPTION("Main driver for SA-1101.");
MODULE_LICENSE("GPL");
