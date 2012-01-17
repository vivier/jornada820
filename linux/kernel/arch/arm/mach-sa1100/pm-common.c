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
#include <asm/uaccess.h>


#ifdef CONFIG_IPAQ_HANDHELD
#include <asm/arch-sa1100/h3600_asic.h>
#endif

#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>

/*
 * Debug macros
 */
#undef DEBUG



static char pm_helper_path[128] = "/sbin/pm_helper";
extern int exec_usermodehelper(char *path, char **argv, char **envp);
int debug_pm = 0;
static int pm_helper_veto = 0;

static int
run_sbin_pm_helper( pm_request_t action )
{
	int i;
	char *argv[3], *envp[8];

	if (!pm_helper_path[0])
		return 2;

	if ( action != PM_SUSPEND && action != PM_RESUME )
		return 1;

	/* Be root */
	current->uid = current->gid = 0;

	i = 0;
	argv[i++] = pm_helper_path;
	argv[i++] = (action == PM_RESUME ? "resume" : "suspend");
	argv[i] = 0;

	i = 0;
	/* minimal command environment */
	envp[i++] = "HOME=/";
	envp[i++] = "PATH=/sbin:/bin:/usr/sbin:/usr/bin";
	envp[i] = 0;

	/* other stuff we want to pass to /sbin/pm_helper */
	return exec_usermodehelper (argv [0], argv, envp);
}

/*
 * If pm_suggest_suspend_hook is non-NULL, it is called by pm_suggest_suspend.
 */
int (*pm_suggest_suspend_hook)(int state);
EXPORT_SYMBOL(pm_suggest_suspend_hook);

/*
 * If pm_use_sbin_pm_helper is nonzero, then run_sbin_pm_helper is called before suspend and after resume
 */
int pm_use_sbin_pm_helper = 1;
EXPORT_SYMBOL(pm_use_sbin_pm_helper);

/*
 * If sysctl_pm_do_suspend_hook is non-NULL, it is called by sysctl_pm_do_suspend.
 * If it returns a true value, then pm_suspend is not called. 
 * Use this to hook in apmd, for now.
 */
int (*pm_sysctl_suspend_hook)(int state);
EXPORT_SYMBOL(pm_sysctl_suspend_hook);

int pm_suspend(void);

int pm_suggest_suspend(void)
{
	int retval;

	if (pm_suggest_suspend_hook) {
		if (pm_suggest_suspend_hook(PM_SUSPEND))
			return 0;
	}
	
	if (pm_use_sbin_pm_helper) {
		pid_t pid;
		int res;
		int status = 0;
		unsigned int old_fs;
        	
		pid = kernel_thread ((int (*) (void *)) run_sbin_pm_helper, (void *) PM_SUSPEND, 0 );
		if ( pid < 0 )
			return pid;

		if (debug_pm)
			printk(KERN_CRIT "%s:%d got pid=%d\n", __FUNCTION__, __LINE__, pid);	
        		
		old_fs = get_fs ();
		set_fs (get_ds ());
		res = waitpid(pid, &status, __WCLONE);
		set_fs (old_fs);
	
		if ( pid != res ) {
			if (debug_pm)
				printk(KERN_CRIT ": waitpid returned %d (exit_code=%d); not suspending\n", res, status );
	        	
			return -1;
		}
        		
		/*if ( WIFEXITED(status) && ( WIFEXITSTATUS(status) != 0 )) {*/
		if (( status & 0xff7f ) != 0 ) {
			if (pm_helper_veto) {
				if (debug_pm)
					printk(KERN_CRIT "%s: SUSPEND WAS CANCELLED BY pm_helper (exit status %d)\n", __FUNCTION__, status >> 8);
				return -1;
			} else {
				if (debug_pm)
					printk(KERN_CRIT "%s: pm_helper returned %d, but going ahead anyway\n", __FUNCTION__, status >> 8);
			}
		}
	}

	if (debug_pm)
		printk(KERN_CRIT "%s: REALLY SUSPENDING NOW\n", __FUNCTION__ );

	if (pm_sysctl_suspend_hook) {
		if (pm_sysctl_suspend_hook(PM_SUSPEND))
			return 0;
	}

	retval = pm_suspend();
	if (retval) {
		if (debug_pm)
			printk(KERN_CRIT "pm_suspend returned %d\n", retval);
		return retval;
	}

	if (pm_use_sbin_pm_helper) {
		pid_t pid;
        	
		if (debug_pm)
			printk(KERN_CRIT "%s: running pm_helper for wakeup\n", __FUNCTION__);

		pid = kernel_thread ((int (*) (void *)) run_sbin_pm_helper, (void *) PM_RESUME, 0 );
		if ( pid < 0 )
			return pid;
        		
		if ( pid != waitpid ( pid, NULL, __WCLONE ))
			return -1;
	}

	return 0;
}

EXPORT_SYMBOL(pm_suggest_suspend);


/*
 * Send us to sleep.
 */
int pm_suspend(void)
{
	int retval;

	retval = pm_send_all(PM_SUSPEND, (void *)3);
	if ( retval )
		return retval;

#ifdef CONFIG_IPAQ_HANDHELD
	retval = h3600_power_management(PM_SUSPEND);
	if (retval) {
		pm_send_all(PM_RESUME, (void *)0);
		return retval;
	}
#endif

	retval = pm_do_suspend();

#ifdef CONFIG_IPAQ_HANDHELD
	/* Allow the power management routines to override resuming */
	while ( h3600_power_management(PM_RESUME) )
		retval = pm_do_suspend();
#endif

	pm_send_all(PM_RESUME, (void *)0);

	return retval;
}
EXPORT_SYMBOL(pm_suspend);

#ifdef CONFIG_SYSCTL
/*
 * ARGH!  ACPI people defined CTL_ACPI in linux/acpi.h rather than
 * linux/sysctl.h.
 *
 * This means our interface here won't survive long - it needs a new
 * interface.  Quick hack to get this working - use sysctl id 9999.
 */
#warning ACPI broke the kernel, this interface needs to be fixed up.
#define CTL_ACPI 9999
#define ACPI_S1_SLP_TYP 19

static struct ctl_table pm_table[] =
{
/*	{ACPI_S1_SLP_TYP, "suspend", NULL, 0, 0600, NULL, (proc_handler *)&sysctl_pm_suspend},*/
	{2, "helper", pm_helper_path, sizeof(pm_helper_path), 0644, NULL, (proc_handler *)&proc_dostring},
	{3, "debug", &debug_pm, sizeof(debug_pm), 0644, NULL, (proc_handler *)&proc_dointvec},
	{4, "helper_veto", &pm_helper_veto, sizeof(pm_helper_veto), 0644, NULL, (proc_handler *)&proc_dointvec},
	{0}
};

static struct ctl_table pm_dir_table[] =
{
	{CTL_ACPI, "pm", NULL, 0, 0555, pm_table},
	{0}
};

/*
 * Initialize power interface
 */
static int __init pm_init(void)
{
	register_sysctl_table(pm_dir_table, 1);
	return 0;
}

__initcall(pm_init);

#endif

