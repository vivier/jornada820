/*
 * drivers/pcmcia/sa1100_jornada820.c
 *
 * Jornada820 PCMCIA specific routines
 *
 * $Id: sa1100_jornada820.c,v 1.9 2004/07/11 14:39:42 oleg820 Exp $
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/init.h>

#include <linux/interrupt.h>
#include <asm/mach/irq.h>
#include <asm/arch/irqs.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/irq.h>
#include <asm/arch/hardware.h>

#include <asm/hardware/sa1101.h>
#include "soc_common.h"
#include "sa11xx_core.h"
#include "sa11xx_base.h"
#include "sa1100_generic.h"

#include "sa1101_generic.h"

static int jornada820_pcmcia_configure_socket(struct soc_pcmcia_socket *skt, const socket_state_t *state)
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
      break;

    case 1:
      rst  = PCCR_S1_reset;
      flt  = PCCR_S1_float;
      vcc0 = PCCR_S1_VCC0;
      vcc1 = PCCR_S1_VCC1;
      vpp0 = PCCR_S1_VPP0;
      vpp1 = PCCR_S1_VPP1;
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

  return 0;
}

static struct pcmcia_low_level jornada820_pcmcia_ops = {
  .owner		= THIS_MODULE,
  .hw_init		= sa1101_pcmcia_hw_init,
  .hw_shutdown		= sa1101_pcmcia_hw_shutdown,
  .socket_state		= sa1101_pcmcia_socket_state,
  .configure_socket	= jornada820_pcmcia_configure_socket,

  .socket_init		= sa1101_pcmcia_socket_init,
  .socket_suspend	= sa1101_pcmcia_socket_suspend,
};

int __init pcmcia_jornada820_init(struct device *dev)
{
	int ret = -ENODEV;

	if (machine_is_jornada820())
		ret = sa11xx_drv_pcmcia_probe(dev, &jornada820_pcmcia_ops, 0, 2);

	return ret;
}

