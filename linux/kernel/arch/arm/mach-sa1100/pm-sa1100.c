/*
 * SA1100 Power Management Routines
 *
 * Copyright (c) 2001 Cliff Brake <cbrake@accelent.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2001-02-06:	Cliff Brake         Initial code
 *
 * 2001-02-25:	Sukjae Cho <sjcho@east.isi.edu> &
 * 		Chester Kuo <chester@linux.org.tw>
 * 			Save more value for the resume function! Support
 * 			Bitsy/Assabet/Freebird board
 *
 * 2001-08-29:	Nicolas Pitre <nico@cam.org>
 * 			Cleaned up, pushed platform dependent stuff
 * 			in the platform specific files.
 *
 * 2002-05-27:	Nicolas Pitre	Killed sleep.h and the kmalloced save array.
 * 				Storage is local on the stack now.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/sysctl.h>
#include <linux/errno.h>
#include <linux/cpufreq.h>

#include <asm/hardware.h>
#include <asm/memory.h>
#include <asm/system.h>
#include <asm/leds.h>


#ifdef CONFIG_IPAQ_HANDHELD
#include <asm/arch/h3600_asic.h>
#endif

#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>

extern void sa1100_cpu_suspend(void);
extern void sa1100_cpu_resume(void);
extern int debug_pm;

#define SAVE(x)		sleep_save[SLEEP_SAVE_##x] = x
#define RESTORE(x)	x = sleep_save[SLEEP_SAVE_##x]

/*
 * List of global SA11x0 peripheral registers to preserve.
 * More ones like CP and general purpose register values are preserved
 * with the stack location in sleep.S.
 */
enum {	SLEEP_SAVE_START = 0,

	SLEEP_SAVE_OSCR, SLEEP_SAVE_OIER,
	SLEEP_SAVE_OSMR0, SLEEP_SAVE_OSMR1, SLEEP_SAVE_OSMR2, SLEEP_SAVE_OSMR3,

	SLEEP_SAVE_GPDR, SLEEP_SAVE_GRER, SLEEP_SAVE_GFER, SLEEP_SAVE_GAFR,
	SLEEP_SAVE_PPDR, SLEEP_SAVE_PPSR, SLEEP_SAVE_PPAR, SLEEP_SAVE_PSDR,

	SLEEP_SAVE_ICMR,
	SLEEP_SAVE_Ser1SDCR0,

        SLEEP_SAVE_PWER,
        SLEEP_SAVE_MSC1, SLEEP_SAVE_MSC2,

	SLEEP_SAVE_SIZE
};


int pm_do_suspend(void)
{
	unsigned long sleep_save[SLEEP_SAVE_SIZE];

	cli();

	leds_event(led_stop);

	/* preserve current time */
	RCNR = xtime.tv_sec;

	/* save vital registers */
	SAVE(OSCR);
	SAVE(OSMR0);
	SAVE(OSMR1);
	SAVE(OSMR2);
	SAVE(OSMR3);
	SAVE(OIER);

	SAVE(GPDR);
	SAVE(GRER);
	SAVE(GFER);
	SAVE(GAFR);

	SAVE(PPDR);
	SAVE(PPSR);
	SAVE(PPAR);
	SAVE(PSDR);

	SAVE(Ser1SDCR0);

	SAVE(ICMR);
        SAVE(PWER);
        SAVE(MSC1);
        SAVE(MSC2);

	/* ... maybe a global variable initialized by arch code to set this? */
	GRER = PWER;
	// Ugly, but I need the AC inserted event
	// In the future, we're going to care about DCD and USB interrupts as well
	if ( machine_is_h3800()) {
#ifdef CONFIG_IPAQ_HANDHELD
		GFER = GPIO_H3800_AC_IN;
#endif
	} else {
		GFER = 0;
		if (machine_is_jornada56x()) {
			/* jca */
			GFER = PWER;
			ICMR |= PWER;
		}
	}
	GEDR = GEDR;

	/* Clear previous reset status */
	RCSR = RCSR_HWR | RCSR_SWR | RCSR_WDR | RCSR_SMR;

	/* set resume return address */
	PSPR = virt_to_phys(sa1100_cpu_resume);

	/* go zzz */
	sa1100_cpu_suspend();

	/* ensure not to come back here if it wasn't intended */
	PSPR = 0;

	if (debug_pm)
		printk(KERN_CRIT "*** made it back from resume\n");

#ifdef CONFIG_IPAQ_HANDHELD
	if ( machine_is_ipaq()) {
		ipaq_model_ops.gedr = GEDR;
		ipaq_model_ops.icpr = ICPR;
	}
#endif

	/* restore registers */
	RESTORE(GPDR);
	RESTORE(GRER);
	RESTORE(GFER);
	RESTORE(GAFR);

	/* clear any edge detect bit */
	GEDR = GEDR;

	RESTORE(PPDR);
	RESTORE(PPSR);
	RESTORE(PPAR);
	RESTORE(PSDR);

	RESTORE(Ser1SDCR0);

	PSSR = PSSR_PH;

	RESTORE(OSMR0);
	RESTORE(OSMR1);
	RESTORE(OSMR2);
	RESTORE(OSMR3);
	RESTORE(OSCR);
	RESTORE(OIER);

#ifdef CONFIG_IPAQ_HANDHELD
/* OSMR0 may have fired before we went to sleep, but after interrupts
   were shut off.  Set OSMR0 to something plausible */
	OSMR0 = OSCR + LATCH;
#endif
	ICLR = 0;
	ICCR = 1;
	RESTORE(ICMR);
	RESTORE(PWER);
	RESTORE(MSC1);
	RESTORE(MSC2);
	/* restore current time */
	xtime.tv_sec = RCNR;

	leds_event(led_start);
	
	sti();

	if (debug_pm)
		printk("interrupts are enabled\n");

	/*
	 * Restore the CPU frequency settings.
	 */
#ifdef CONFIG_CPU_FREQ
	cpufreq_restore();
#endif
	return 0;
}

unsigned long sleep_phys_sp(void *sp)
{
	return virt_to_phys(sp);
}

#include "pm-common.c"
