/*
 * linux/drivers/pcmcia/sa1101_generic.c
 *
 * We implement the generic parts of a SA1101 PCMCIA driver.  This
 * basically means we handle everything except controlling the
 * power.  Power is machine specific...
 *
 *
 * Francois-Rene Rideau (fare@tunes.org), 2004/07/02
 * Port to 2.6 with inspiration from sa1100_jornada56x.c
 *
 * George Almasi (galmasi@optonline.net), 2004/1/24
 * Based on the sa1111_generic.c file.
 * $Id: sa1101_generic.c,v 1.5 2005/07/25 09:09:14 fare Exp $
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/init.h>

#include <pcmcia/ss.h>

#include <linux/interrupt.h>
#include <asm/mach/irq.h>
#include <asm/arch/irqs.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/irq.h>
#include <asm/arch/hardware.h>
#include <asm/hardware/sa1101.h>
//#include "soc_common.h"
//#include "sa11xx_core.h"
//#include "sa11xx_base.h"
#include "sa1100_generic.h"
#include "sa1101_generic.h"

/*
 * Initialize the PCMCIA subsystem: turn on interrupts, reserve memory
 */

static struct pcmcia_irqs irqs[] = {
	{ 0, IRQ_SA1101_S0_CDVALID,     "SA1101 PCMCIA card detect" },
	{ 0, IRQ_SA1101_S0_BVD1_STSCHG, "SA1101 PCMCIA BVD1" },
	{ 1, IRQ_SA1101_S1_CDVALID,     "SA1101 CF card detect" },
	{ 1, IRQ_SA1101_S1_BVD1_STSCHG, "SA1101 CF BVD1" },
};

static irqreturn_t debug_irq_handler(unsigned int irq, void* dev_id, struct pt_regs *regs)
{
	printk("Got interrupt (%d). Ignoring.\n", irq); // DEBUG
        return IRQ_HANDLED;
}

int sa1101_pcmcia_hw_init(struct soc_pcmcia_socket *skt)
{
	int ret;
	printk("sa1101_pcmcia_hw_init... "); // DEBUG
	if (skt->irq == NO_IRQ)
		skt->irq = skt->nr ? IRQ_SA1101_S1_READY_NIREQ :
			IRQ_SA1101_S0_READY_NIREQ;
//	request_irq(skt->irq, debug_irq_handler, SA_INTERRUPT, "SA1101 NIREQ", NULL);
//	set_irq_type(skt->irq, IRQT_BOTHEDGE);
	ret = soc_pcmcia_request_irqs(skt, irqs, ARRAY_SIZE(irqs));
	printk("%d\n", ret); // DEBUG
	return ret;
}

void sa1101_pcmcia_hw_shutdown(struct soc_pcmcia_socket *skt)
{
	soc_pcmcia_free_irqs(skt, irqs, ARRAY_SIZE(irqs));
}

void sa1101_pcmcia_socket_state(struct soc_pcmcia_socket *skt, struct pcmcia_state *state)
{
	unsigned long status = PCSR;

	switch (skt->nr) {
	case 0:
		state->detect = (status & PCSR_S0_detected)     ? 0 : 1;
		state->ready  = (status & PCSR_S0_ready)        ? 1 : 0;
		state->bvd1   = (status & PCSR_S0_BVD1_nSTSCHG) ? 1 : 0;
		state->bvd2   = (status & PCSR_S0_BVD2_nSPKR)   ? 1 : 0;
		state->wrprot = (status & PCSR_S0_WP)           ? 1 : 0;
		state->vs_3v  = (status & PCSR_S0_VS1)          ? 0 : 1;
		state->vs_Xv  = (status & PCSR_S0_VS2)          ? 0 : 1;
		break;
	case 1:
		state->detect = (status & PCSR_S1_detected)     ? 0 : 1;
		state->ready  = (status & PCSR_S1_ready)        ? 1 : 0;
		state->bvd1   = (status & PCSR_S1_BVD1_nSTSCHG) ? 1 : 0;
		state->bvd2   = (status & PCSR_S1_BVD2_nSPKR)   ? 1 : 0;
		state->wrprot = (status & PCSR_S1_WP)           ? 1 : 0;
		state->vs_3v  = (status & PCSR_S1_VS1)          ? 0 : 1;
		state->vs_Xv  = (status & PCSR_S1_VS2)          ? 0 : 1;
		break;
	}
}

/* this function is specific to jornada820.
   if you have other sa1101 based device, verify the settings yourself */ 

int sa1101_pcmcia_configure_socket(struct soc_pcmcia_socket *skt, const socket_state_t *state)
{
	unsigned int rst, flt, vcc0, vcc1, vpp0, vpp1, mask0, mask1; // irq
  unsigned long flags;

  switch (skt->nr)
    {
    case 0:
      rst  = PCCR_S0_reset;
      flt  = PCCR_S0_float;
      vcc0 = PCCR_S0_VCC0;
      vcc1 = PCCR_S0_VCC1;
      vpp0 = PCCR_S0_VPP0;
      vpp1 = PCCR_S0_VPP1;
//    irq  = IRQ_SA1101_S0_READY_NIREQ;
      break;

    case 1:
      rst  = PCCR_S1_reset;
      flt  = PCCR_S1_float;
      vcc0 = PCCR_S1_VCC0;
      vcc1 = PCCR_S1_VCC1;
      vpp0 = PCCR_S1_VPP0;
      vpp1 = PCCR_S1_VPP1;
//    irq  = IRQ_SA1101_S1_READY_NIREQ;
      break;

    default:  return -1;
    }

  mask0 = rst | flt | vcc0 | vcc1 | vpp0 | vpp1;
  mask1 = 0;

  switch (skt->nr)
    {
    case 0:
	  switch (state->Vcc)
	    {
	    case  0: break;
	    case 33: mask1 |= (vcc0|vcc1); break;
	    case 50: mask1 |=  vcc0      ; break;
	    default:
	            printk(KERN_ERR "sa1101_pcmcia: sock=0 unrecognised VCC %u\n",state->Vcc);
	            return -1;
	    };

          switch (state->Vpp)
            {
            case   0: break;
            case  50: mask1 |= vpp0; break;
            default:
                    printk(KERN_ERR "sa1101_pcmcia: sock=0 unrecognised VPP %u\n",state->Vpp);
                    return -1;
          };

    break;
    case 1:
	  switch (state->Vcc)
	    {
	    case  0: break;
	    case 33: mask1 |= vcc0; break;
	    default:
	            printk(KERN_ERR "sa1101_pcmcia: sock=1 unrecognised VCC %u\n",state->Vcc);
	            return -1;
	    };

          switch (state->Vpp)
            {
	    case  0: break;
            default:
                    printk(KERN_WARNING "sa1101_pcmcia: CF sock=1 VPP %u ???\n",state->Vpp);
          };
    break;

    default:  return -1;
    }

  if (state->flags & SS_RESET)  mask1 |= rst;
  if (state->flags & SS_OUTPUT_ENA)  mask1 |= flt;

  local_irq_save(flags);
  /* we have a minor timing problem here */
  PCCR = ((PCCR & ~mask0) | mask1);
  local_irq_restore(flags);

//  if (skt->irq)  enable_irq(irq);
//  else   	 disable_irq(irq);
  return 0;
}

void sa1101_pcmcia_socket_init(struct soc_pcmcia_socket *skt)
{
	soc_pcmcia_enable_irqs(skt, irqs, ARRAY_SIZE(irqs));
}

void sa1101_pcmcia_socket_suspend(struct soc_pcmcia_socket *skt)
{
	soc_pcmcia_disable_irqs(skt, irqs, ARRAY_SIZE(irqs));
}

struct pcmcia_low_level sa1101_pcmcia_ops = {
	.owner			= THIS_MODULE,
	.hw_init		= sa1101_pcmcia_hw_init,
	.hw_shutdown		= sa1101_pcmcia_hw_shutdown,
	.socket_state		= sa1101_pcmcia_socket_state,
	.configure_socket	= sa1101_pcmcia_configure_socket,
	.socket_init		= sa1101_pcmcia_socket_init,
	.socket_suspend		= sa1101_pcmcia_socket_suspend,
/*
	.socket_get_timing	= 
*/
};

static int pcmcia_probe(struct sa1101_dev *dev)
{
	printk("sa1101 pcmcia: probe...\n"); // DEBUG

	if (!request_mem_region(dev->res.start, 512,
				SA1101_DRIVER_NAME(dev)))
		return -EBUSY;

#if 0
	base = dev->mapbase;

	/*
	 * Initialise the suspend state.
	 */
	sa1101_writel(PCSSR_S0_SLEEP | PCSSR_S1_SLEEP, base + SA1101_PCSSR);
	sa1101_writel(PCCR_S0_FLT | PCCR_S1_FLT, base + SA1101_PCCR);
#endif

#ifdef CONFIG_SA1100_JORNADA820
	pcmcia_jornada820_init(&dev->dev);
#endif

	return 0;
}
static int __devexit pcmcia_remove(struct sa1101_dev *dev)
{
	soc_common_drv_pcmcia_remove(&dev->dev);
	release_mem_region(dev->res.start, 512);
	return 0;
}
static int pcmcia_suspend(struct sa1101_dev *dev, u32 state)
{
	return pcmcia_socket_dev_suspend(&dev->dev, state);
}

static int pcmcia_resume(struct sa1101_dev *dev)
{
	return pcmcia_socket_dev_resume(&dev->dev);
}

static struct sa1101_driver pcmcia_driver = {
	.drv = {
		.name	= "sa1101-pcmcia",
	},
	.devid		= SA1101_DEVID_PCMCIA,
	.probe		= pcmcia_probe,
	.remove		= __devexit_p(pcmcia_remove),
	.suspend	= pcmcia_suspend,
	.resume		= pcmcia_resume,
};

static int __init sa1101_drv_pcmcia_init(void)
{
	printk("init sa1101 pcmcia driver...\n"); // DEBUG
	return sa1101_driver_register(&pcmcia_driver);
}

static void __exit sa1101_drv_pcmcia_exit(void)
{
	sa1101_driver_unregister(&pcmcia_driver);
}

module_init(sa1101_drv_pcmcia_init);
module_exit(sa1101_drv_pcmcia_exit);

MODULE_DESCRIPTION("SA1101 PCMCIA card socket driver");
MODULE_LICENSE("GPL");
