/*
 * drivers/pcmcia/sa1100_jornada820.c
 *
 * Jornada820 PCMCIA specific routines
 *
 * Francois-Rene Rideau (fare@tunes.org), 2004/07/02
 * Port to 2.6 with inspiration from sa1100_jornada56x.c
 *
 * George Almasi (galmasi@optonline.net), 2004/1/24
 * Based on the sa1111_generic.c file.
 * $Id: sa1100_jornada820.c,v 1.4 2004/07/03 14:04:50 fare Exp $
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/irq.h>
#include <asm/arch/hardware.h>
#include "sa1100_generic.h"


/*
 * Initialize the PCMCIA subsystem: turn on interrupts, reserve memory
 */

static struct pcmcia_irqs irqs[] = {
//	{ 0, IRQ_SA1101_S0_READY_NIREQ, "SA1101 PCMCIA ready" },
	{ 0, IRQ_SA1101_S0_CDVALID,     "SA1101 PCMCIA card detect" },
	{ 0, IRQ_SA1101_S0_BVD1_STSCHG, "SA1101 PCMCIA BVD1" },
//	{ 1, IRQ_SA1101_S1_READY_NIREQ, "SA1101 CF ready" },
	{ 1, IRQ_SA1101_S1_CDVALID,     "SA1101 CF card detect" },
	{ 1, IRQ_SA1101_S1_BVD1_STSCHG, "SA1101 CF BVD1" },
};
static int sa1101_pcmcia_hw_init(struct sa1100_pcmcia_socket *skt)
{
	if (skt->irq == NO_IRQ)
		skt->irq = skt->nr ? IRQ_SA1101_S0_READY_NIREQ :
			IRQ_SA1101_S1_READY_NIREQ;
	return sa11xx_request_irqs(skt, irqs, ARRAY_SIZE(irqs));
}

static void sa1101_pcmcia_hw_shutdown(struct sa1100_pcmcia_socket *skt)
{
	sa11xx_free_irqs(skt, irqs, ARRAY_SIZE(irqs));
}

static void sa1101_pcmcia_socket_state(struct sa1100_pcmcia_socket *skt, struct pcmcia_state *state)
{
	unsigned long status = PCSR;

	/* XXX - as compared to what we do here, jornada56x does as follows:
	 * _detected is inverted and _VS2 is 2 rather than 1.
	 * If there is trouble, try that.
	 */
	switch (skt->nr) {
	case 0:
		state->detect = (status & PCSR_S0_detected)     ? 0 : 1;
		state->ready  = (status & PCSR_S0_ready)        ? 1 : 0;
		state->vs_3v  = (status & PCSR_S0_VS1)          ? 0 : 1;
		state->vs_Xv  = (status & PCSR_S0_VS2)          ? 0 : 1;
		state->bvd1   = (status & PCSR_S0_BVD1_nSTSCHG) ? 1 : 0;
		state->bvd2   = (status & PCSR_S0_BVD2_nSPKR)   ? 1 : 0;
		state->wrprot = (status & PCSR_S0_WP)           ? 1 : 0;
		break;
	case 1:
		state->detect = (status & PCSR_S1_detected)     ? 0 : 1;
		state->ready  = (status & PCSR_S1_ready)        ? 1 : 0;
		state->vs_3v  = (status & PCSR_S1_VS1)          ? 0 : 1;
		state->vs_Xv  = (status & PCSR_S1_VS2)          ? 0 : 1;
		state->bvd1   = (status & PCSR_S1_BVD1_nSTSCHG) ? 1 : 0;
		state->bvd2   = (status & PCSR_S1_BVD2_nSPKR)   ? 1 : 0;
		state->wrprot = (status & PCSR_S1_WP)           ? 1 : 0;
		break;
	}
}

static int sa1101_pcmcia_configure_socket(struct sa1100_pcmcia_socket *skt, const socket_state_t *state)
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
    break;

    default:  return -1;
    }

    /* TODO: how to verify ? */
  switch (state->Vpp)
    {
    case   0: break;
    case  33: mask1 |= vpp0; break;
    case  50: mask1 |= (vpp0|vpp1); break;
    default:
            printk(KERN_ERR "sa1101_pcmcia: unrecognised VPP %u\n",state->Vpp);
            return -1;
    };

  if (state->flags & SS_RESET)  mask1 |= rst;
  if (state->flags & SS_OUTPUT_ENA)  mask1 |= flt;

  local_irq_save(flags);
  PCCR = ((PCCR & ~mask0) | mask1);
  local_irq_restore(flags);

//  if (skt->irq)  enable_irq(irq);
//  else   	 disable_irq(irq);
  return 0;
}

void sa1101_pcmcia_socket_init(struct sa1100_pcmcia_socket *skt)
{
	sa11xx_enable_irqs(skt, irqs, ARRAY_SIZE(irqs));
}

void sa1101_pcmcia_socket_suspend(struct sa1100_pcmcia_socket *skt)
{
	sa11xx_disable_irqs(skt, irqs, ARRAY_SIZE(irqs));
}

struct pcmcia_low_level sa1101_pcmcia_ops = {
	.owner			= THIS_MODULE,
	.hw_init		= sa1101_pcmcia_hw_init,
	.hw_shutdown		= sa1101_pcmcia_hw_shutdown,
	.socket_state		= sa1101_pcmcia_socket_state,
	.configure_socket	= sa1101_pcmcia_configure_socket,
	.socket_init		= sa1101_pcmcia_socket_init,
	.socket_suspend		= sa1101_pcmcia_socket_suspend,
};

int __init pcmcia_sa1101_init(struct device *dev)
{
	return sa11xx_drv_pcmcia_probe(dev, &sa1101_pcmcia_ops, 0, 2);
}
