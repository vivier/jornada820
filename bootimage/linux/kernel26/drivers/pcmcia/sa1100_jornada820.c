/*
 * drivers/pcmcia/sa1100_jornada820.c
 *
 * Jornada820 PCMCIA specific routines
 *
 * $Id: sa1100_jornada820.c,v 1.6 2004/07/07 16:57:24 oleg820 Exp $
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/init.h>

#include <linux/interrupt.h>
#include <asm/mach/irq.h>
#include <asm/arch/irq.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/irq.h>
#include <asm/arch/hardware.h>

#include <asm/hardware/sa1101.h>
#include "soc_common.h"
#include "sa11xx_core.h"
#include "sa11xx_base.h"
#include "sa1100_generic.h"


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
