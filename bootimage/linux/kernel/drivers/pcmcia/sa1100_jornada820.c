/*
 * drivers/pcmcia/sa1100_jornada820.c
 *
 * Jornada820 PCMCIA specific routines
 * George Almasi (galmasi@optonline.net), 2004/1/24
 * Based on the sa1111_generic.c file.
 *
 */
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/ioport.h>

#include <asm/hardware.h>
#include <asm/irq.h>

#include "sa1100_generic.h"

#define SA1101_IRQMASK_LO(x)    (1 << (x - IRQ_SA1101_START))
#define SA1101_IRQMASK_HI(x)    (1 << (x - IRQ_SA1101_START - 32))

static struct irqs 
{
  int irq;
  const char *str;
} irqs[] = {
  { IRQ_SA1101_S0_CDVALID,     "PCMCIA card detect" },
  { IRQ_SA1101_S0_BVD1_STSCHG, "PCMCIA BVD1" },
  { IRQ_SA1101_S1_CDVALID,     "CF card detect" },
  { IRQ_SA1101_S1_BVD1_STSCHG, "CF BVD1" },
};

/* ************************************************************************* */
/*  Initialize the PCMCIA subsystem: turn on interrupts, reserve memory      */
/* ************************************************************************* */

int jornada820_pcmcia_init(struct pcmcia_init *init)
{
  int i, ret;

  if (!request_mem_region(_PCCR, 512, "PCMCIA")) return -1;
  
  for (i = ret = 0; i < ARRAY_SIZE(irqs); i++)
    {
      ret = request_irq(irqs[i].irq, 
			init->handler, 
			SA_INTERRUPT,
			irqs[i].str, 
			NULL);
      if (ret)break;
    }


  if (i < ARRAY_SIZE(irqs)) 
    {
      printk(KERN_ERR "Jornada820_pcmcia: unable to grab IRQ%d (%d)\n",
	     irqs[i].irq, ret);
      while (i--) free_irq(irqs[i].irq, NULL);
      release_mem_region(_PCCR, 16);
    }

  INTPOL1 |= (SA1101_IRQMASK_HI(IRQ_SA1101_S0_CDVALID) |
              SA1101_IRQMASK_HI(IRQ_SA1101_S1_CDVALID) |
              SA1101_IRQMASK_HI(IRQ_SA1101_S0_BVD1_STSCHG) |
              SA1101_IRQMASK_HI(IRQ_SA1101_S0_BVD1_STSCHG));

  return ret ? -1 : 2;
}

/* ************************************************************************* */
/*      Shut down PCMCIA: release memory, interrupts.                        */
/* ************************************************************************* */

int jornada820_pcmcia_shutdown(void)
{
  int i;
  for (i = 0; i < ARRAY_SIZE(irqs); i++) free_irq(irqs[i].irq, NULL);

  INTPOL1 &= ~(SA1101_IRQMASK_HI(IRQ_SA1101_S0_CDVALID) |
	       SA1101_IRQMASK_HI(IRQ_SA1101_S1_CDVALID) |
	       SA1101_IRQMASK_HI(IRQ_SA1101_S0_BVD1_STSCHG) |
	       SA1101_IRQMASK_HI(IRQ_SA1101_S0_BVD1_STSCHG));

  release_mem_region(_PCCR, 512);
  return 0;
}

/* ************************************************************************* */
/*             return socket status                                          */
/* ************************************************************************* */

int jornada820_pcmcia_socket_state(struct pcmcia_state_array *state)
{
  unsigned long status;
  if (state->size < 2)  return -1;

  status = PCSR;
  
  state->state[0].detect = (status & PCSR_S0_detected)     ? 0 : 1;
  state->state[0].ready  = (status & PCSR_S0_ready)        ? 1 : 0;
  state->state[0].vs_3v  = (status & PCSR_S0_VS1)          ? 1 : 0;
  state->state[0].vs_Xv  = (status & PCSR_S0_VS2)          ? 1 : 0;
  state->state[0].bvd1   = (status & PCSR_S0_BVD1_nSTSCHG) ? 1 : 0;
  state->state[0].bvd2   = (status & PCSR_S0_BVD2_nSPKR)   ? 1 : 0;
  state->state[0].wrprot = (status & PCSR_S0_WP)           ? 1 : 0;
  
  state->state[1].detect = (status & PCSR_S1_detected)     ? 0 : 1;
  state->state[1].ready  = (status & PCSR_S1_ready)        ? 1 : 0;
  state->state[1].vs_3v  = (status & PCSR_S1_VS1)          ? 0 : 1;
  state->state[1].vs_Xv  = (status & PCSR_S1_VS2)          ? 0 : 1;
  state->state[1].bvd1   = (status & PCSR_S1_BVD1_nSTSCHG) ? 1 : 0;
  state->state[1].bvd2   = (status & PCSR_S1_BVD2_nSPKR)   ? 1 : 0;
  state->state[1].wrprot = (status & PCSR_S1_WP)           ? 1 : 0;

  // printk ("pcmcia %x %x\n", state->state[0], state->state[1]);
  return 0;
}

/* ************************************************************************* */
/*                  return the current interrupt for each socket             */
/* ************************************************************************* */

int jornada820_pcmcia_get_irq_info(struct pcmcia_irq_info *info)
{
  int ret = 0;
  
  switch (info->sock)
    {
    case 0: info->irq = IRQ_SA1101_S0_READY_NIREQ;	break;
    case 1: info->irq = IRQ_SA1101_S1_READY_NIREQ;	break;
    default: ret = 1;
    }
  
  return ret;
}

/* ************************************************************************* */
/*           configure a socket                                              */
/* ************************************************************************* */

int jornada820_pcmcia_configure_socket(const struct pcmcia_configure *conf)
{
  unsigned int rst, flt, irq, vcc0, vcc1, vpp0, vpp1, mask0, mask1;
  unsigned long flags;

  switch (conf->sock)
    {
    case 0:
      rst  = PCCR_S0_reset;
      flt  = PCCR_S0_float;
      vcc0 = PCCR_S0_VCC0;
      vcc1 = PCCR_S0_VCC1;
      vpp0 = PCCR_S0_VPP0;
      vpp1 = PCCR_S0_VPP1;
      irq  = IRQ_SA1101_S0_READY_NIREQ;
      break;
      
    case 1:
      rst  = PCCR_S1_reset;
      flt  = PCCR_S1_float;
      vcc0 = PCCR_S1_VCC0;
      vcc1 = PCCR_S1_VCC1;
      vpp0 = PCCR_S1_VPP0;
      vpp1 = PCCR_S1_VPP1;
      irq  = IRQ_SA1101_S1_READY_NIREQ;
      break;
      
    default:  return -1;
    }
  
  mask0 = rst | flt | vcc0 | vcc1 | vpp0 | vpp1;
  mask1 = 0;

  switch (conf->vcc)
    {
    case 33: mask1 |= vcc0; break;      
    case 50: mask1 |= (vcc0 | vcc1); break;
    };

  switch (conf->vpp)
    {
    case 33: mask1 |= vpp0; break;      
    case 50: mask1 |= (vpp0 | vpp1); break;
    };

  if (conf->reset)  mask1 |= rst;
  if (conf->output) mask1 |= flt;
 
  local_irq_save(flags);
  PCCR = ((PCCR & ~mask0) | mask1);
  local_irq_restore(flags);
  
  if (conf->irq)  enable_irq(irq);
  else            disable_irq(irq);
  return 0;
}

int jornada820_pcmcia_socket_init(int sock)
{
  return 0;
}

int jornada820_pcmcia_socket_suspend(int sock)
{
  return 0;
}

struct pcmcia_low_level jornada820_pcmcia_ops = {
  init:			jornada820_pcmcia_init,
  shutdown:		jornada820_pcmcia_shutdown,
  socket_state:		jornada820_pcmcia_socket_state,
  get_irq_info:		jornada820_pcmcia_get_irq_info,
  configure_socket:	jornada820_pcmcia_configure_socket,

  socket_init:		jornada820_pcmcia_socket_init,
  socket_suspend:	jornada820_pcmcia_socket_suspend,
};

