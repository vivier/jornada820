/****************************************************************************

  regs-sa1101.c: Register monitor of SA-1101

Modified version of registers.c by Sukjae Cho <sjcho@east.isi.edu>

The IEEE* and keypad registers are omitted. The VGA palette is an array!

This module is made for
debugging and testing purpose.

Jornada820 version based on ???
$Id: regs-sa1101.c,v 1.2 2004/06/27 13:21:37 oleg820 Exp $

****************************************************************************/

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>               /* because we are a module */
#include <linux/init.h>                 /* for the __init macros */
#include <linux/proc_fs.h>              /* all the /proc functions */
#include <linux/ioport.h>
#include <asm/uaccess.h>                /* to copy to/from userspace */
#include <asm/arch/hardware.h>

#define MODULE_NAME "regmonsa1101"
#define DEV_DIRNAME "sa1101"
#define REG_DIRNAME "registers"

static ssize_t proc_read_reg(struct file * file, char * buf,
		size_t nbytes, loff_t *ppos);
static ssize_t proc_write_reg(struct file * file, const char * buffer,
		size_t count, loff_t *ppos);

static struct file_operations proc_reg_operations = {
	read:	proc_read_reg,
	write:	proc_write_reg
};

typedef struct sa1101_reg_entry {
	u32 phyaddr;
	char* name;
	char* description;
	unsigned short low_ino;
} sa1101_reg_entry_t;

static sa1101_reg_entry_t sa1101_regs[] =
{
/*	{ phyaddr,    name,     description } */
	{ _SKCR, "SKCR", "SA-1101 Control Reg." },
	{ _SMCR, "SMCR", "Shared Mem. Control Reg." },
	{ _SNPR, "SNPR", "Snoop Reg." },
	{ _VMCCR, "VMCCR", "VMC configuration register" },
	{ _VMCAR, "VMCAR", "VMC address register" },
	{ _VMCDR, "VMCDR", "VMC data register" },
	{ _UFCR, "UFCR", "Update FIFO Control Reg." },
	{ _UFSR, "UFSR", "Update FIFO Status Reg." },
	{ _UFLVLR, "UFLVLR", "Update FIFO level reg." },
	{ _UFDR, "UFDR", "Update FIFO data reg." },
	{ _SKPCR, "SKPCR", "Power Control Register" },
	{ _SKCDR, "SKCDR", "Clock Divider Register" },
	{ _DACDR1, "DACDR1", "DAC1 Data register" },
	{ _DACDR2, "DACDR2", "DAC2 Data register" },
     	{ _VideoControl	    ,"VideoControl", "    Video Control Register"},
     	{ _VgaTiming0	    ,"VgaTiming0", "	   VGA Timing Register 0"},
     	{ _VgaTiming1	    ,"VgaTiming1", "	     	VGA Timing Register 1"},
     	{ _VgaTiming2	    ,"VgaTiming2", "	     	VGA Timing Register 2"},
     	{ _VgaTiming3	    ,"VgaTiming3", "	     	VGA Timing Register 3"},
     	{ _VgaBorder	    ,"VgaBorder", "	     	VGA Border Color Register"},
     	{ _VgaDBAR	    ,"VgaDBAR", "	     	VGADMA Base Address Register"},
     	{ _VgaDCAR	    ,"VgaDCAR", "	     	VGADMA Channel Current Address Register"},
     	{ _VgaStatus	    ,"VgaStatus", "	     	VGA Status Register"},
     	{ _VgaInterruptMask ,"VgaInterruptMask", " 	VGA Interrupt Mask Register"},
     	{ _VgaPalette	    ,"VgaPalette", "	     	VGA Palette Registers"},
     	{ _DacControl	    ,"DacControl", "	     	DAC Control Register"},
     	{ _VgaTest	    ,"VgaTest", "	     	VGA Controller Test Register"},
 	{ _Revision         ,"Revision", ""},
 	{ _Control          ,"Control", ""},
 	{ _CommandStatus    ,"CommandStatus", ""},
 	{ _InterruptStatus  ,"InterruptStatus", ""},
 	{ _InterruptEnable  ,"InterruptEnable", ""},
 	{ _HCCA             ,"HCCA", ""},
 	{ _PeriodCurrentED  ,"PeriodCurrentED", ""},
 	{ _ControlHeadED    ,"ControlHeadED", ""},
 	{ _BulkHeadED       ,"BulkHeadED", ""},
 	{ _BulkCurrentED    ,"BulkCurrentED", ""},
 	{ _DoneHead         ,"DoneHead", ""},
 	{ _FmInterval       ,"FmInterval", ""},
 	{ _FmRemaining      ,"FmRemaining", ""},
 	{ _FmNumber         ,"FmNumber", ""},
 	{ _PeriodicStart    ,"PeriodicStart", ""},
 	{ _LSThreshold      ,"LSThreshold", ""},
 	{ _RhDescriptorA    ,"RhDescriptorA", ""},
 	{ _RhDescriptorB    ,"RhDescriptorB", ""},
 	{ _RhStatus         ,"RhStatus", ""},
 	{ _RhPortStatus     ,"RhPortStatus", ""},
 	{ _USBStatus        ,"USBStatus", ""},
 	{ _USBReset         ,"USBReset", ""},

 	{ _USTAR            ,"USTAR", ""},
 	{ _USWER            ,"USWER", ""},
 	{ _USRFR            ,"USRFR", ""},
 	{ _USNFR            ,"USNFR", ""},
 	{ _USTCSR           ,"USTCSR", ""},
 	{ _USSR             ,"USSR", ""},
     	{ _INTTEST0	,"INTTEST0", "     	Test register 0"},
     	{ _INTTEST1	,"INTTEST1", "     	Test register 1"},
     	{ _INTENABLE0	,"INTENABLE0", "   	Interrupt Enable register 0"},
     	{ _INTENABLE1	,"INTENABLE1", "   	Interrupt Enable register 1"},
     	{ _INTPOL0	,"INTPOL0", "	     	Interrupt Polarity selection 0"},
     	{ _INTPOL1	,"INTPOL1", "	     	Interrupt Polarity selection 1"},
     	{ _INTTSTSEL	,"INTTSTSEL", "    	Interrupt source selection"},
     	{ _INTSTATCLR0	,"INTSTATCLR0", "  	Interrupt Status 0"},
     	{ _INTSTATCLR1	,"INTSTATCLR1", "  	Interrupt Status 1"},
     	{ _INTSET0	,"INTSET0", "	     	Interrupt Set 0"},
     	{ _INTSET1	,"INTSET1", "	     	Interrupt Set 1"},
     	{ _KBDCR	,"KBDCR", "	     	KB Control Register"},
     	{ _KBDSTAT	,"KBDSTAT", "     	KB Status Register"},
     	{ _KBDDATA	,"KBDDATA", "	     	KB Transmit/Receive Data register"},
     	{ _KBDCLKDIV	,"KBDCLKDIV", "    	KB Clock Division Register"},
     	{ _KBDPRECNT	,"KBDPRECNT", "    	KB Clock Precount Register"},
     	{ _KBDTEST1	,"KBDTEST1", "     	KB Test register 1"},
     	{ _KBDTEST2	,"KBDTEST2", "     	KB Test register 2"},
     	{ _KBDTEST3	,"KBDTEST3", "     	KB Test register 3"},
     	{ _KBDTEST4	,"KBDTEST4", "     	KB Test register 4"},
     	{ _MSECR	,"MSECR", "	     	MS Control Register"},
     	{ _MSESTAT      ,"MSESTAT", "	           	MS Status Register"},
     	{ _MSEDATA      ,"MSEDATA", "	           	MS Transmit/Receive Data register"},
     	{ _MSECLKDIV    ,"MSECLKDIV", "          	MS Clock Division Register"},
     	{ _MSEPRECNT    ,"MSEPRECNT", "          	MS Clock Precount Register"},
     	{ _MSETEST1     ,"MSETEST1", "           	MS Test register 1"},
     	{ _MSETEST2     ,"MSETEST2", "          	MS Test register 2"},
     	{ _MSETEST3     ,"MSETEST3", "          	MS Test register 3"},
     	{ _MSETEST4     ,"MSETEST4", "          	MS Test register 4"},
     	{ _PADWR	,"PADWR", "	     	Port A Data Write Register"},
     	{ _PBDWR	,"PBDWR", "	     	Port B Data Write Register"},
     	{ _PADRR	,"PADRR", "	     	Port A Data Read Register"},
     	{ _PBDRR	,"PBDRR", "	     	Port B Data Read Register"},
     	{ _PADDR	,"PADDR", "	     	Port A Data Direction Register"},
     	{ _PBDDR	,"PBDDR", "	     	Port B Data Direction Register"},
     	{ _PASSR	,"PASSR", "	     	Port A Sleep State Register"},
     	{ _PBSSR	,"PBSSR", "	     	Port B Sleep State Register"},
     	{ _PCCR     	,"PCCR", "	     	PCMCIA Control Register"},
     	{ _PCSSR     	,"PCSSR", "	     	PCMCIA Sleep State Register"},
     	{ _PCSR     	,"PCSR", "	     	PCMCIA Status Register"}
};

#define NUM_OF_SA1101_REG_ENTRY	(sizeof(sa1101_regs)/sizeof(sa1101_reg_entry_t))

static int proc_read_reg(struct file * file, char * buf,
		size_t nbytes, loff_t *ppos)
{
	int i_ino = (file->f_dentry->d_inode)->i_ino;
	char outputbuf[15];
	int count;
	int i;
	sa1101_reg_entry_t* current_reg=NULL;
	if (*ppos>0) /* Assume reading completed in previous read*/
		return 0;
	for (i=0;i<NUM_OF_SA1101_REG_ENTRY;i++) {
		if (sa1101_regs[i].low_ino==i_ino) {
			current_reg = &sa1101_regs[i];
			break;
		}
	}
	if (current_reg==NULL)
		return -EINVAL;

	count = sprintf(outputbuf, "0x%08X\n",
			*((volatile unsigned int *) SA1101_p2v(current_reg->phyaddr)));
	*ppos+=count;
	if (count>nbytes)  /* Assume output can be read at one time */
		return -EINVAL;
	if (copy_to_user(buf, outputbuf, count))
		return -EFAULT;
	return count;
}

static ssize_t proc_write_reg(struct file * file, const char * buffer,
		size_t count, loff_t *ppos)
{
	int i_ino = (file->f_dentry->d_inode)->i_ino;
	sa1101_reg_entry_t* current_reg=NULL;
	int i;
	unsigned long newRegValue;
	char *endp;

	for (i=0;i<NUM_OF_SA1101_REG_ENTRY;i++) {
		if (sa1101_regs[i].low_ino==i_ino) {
			current_reg = &sa1101_regs[i];
			break;
		}
	}
	if (current_reg==NULL)
		return -EINVAL;

	newRegValue = simple_strtoul(buffer,&endp,0);
	*((volatile unsigned int *) SA1101_p2v(current_reg->phyaddr))=newRegValue;
	return (count+endp-buffer);
}

static struct proc_dir_entry *regdir;
static struct proc_dir_entry *cpudir;

static int __init init_regsa1101_monitor(void)
{
	struct proc_dir_entry *entry;
	int i;

	cpudir = proc_mkdir(DEV_DIRNAME, &proc_root);
	if (cpudir == NULL) {
		printk(KERN_ERR MODULE_NAME": can't create /proc/" DEV_DIRNAME "\n");
		return(-ENOMEM);
	}

	regdir = proc_mkdir(REG_DIRNAME, cpudir);
	if (regdir == NULL) {
		printk(KERN_ERR MODULE_NAME": can't create /proc/" DEV_DIRNAME "/" REG_DIRNAME "\n");
		return(-ENOMEM);
	}

	for(i=0;i<NUM_OF_SA1101_REG_ENTRY;i++) {
		entry = create_proc_entry(sa1101_regs[i].name,
				S_IWUSR |S_IRUSR | S_IRGRP ,
				regdir);
		if(entry) {
			sa1101_regs[i].low_ino = entry->low_ino;
			entry->proc_fops = &proc_reg_operations;
		} else {
			printk( KERN_ERR MODULE_NAME
				": can't create /proc/" REG_DIRNAME
				"/%s\n", sa1101_regs[i].name);
			return(-ENOMEM);
		}
	}
	printk("regsa1101 loaded.\n");
	return (0);
}

static void __exit cleanup_regsa1101_monitor(void)
{
	int i;
	for(i=0;i<NUM_OF_SA1101_REG_ENTRY;i++)
		remove_proc_entry(sa1101_regs[i].name,regdir);
	remove_proc_entry(REG_DIRNAME, cpudir);
	remove_proc_entry(DEV_DIRNAME, &proc_root);
}

module_init(init_regsa1101_monitor);
module_exit(cleanup_regsa1101_monitor);

MODULE_DESCRIPTION("SA1101 Register monitor");
MODULE_LICENSE("GPL");

EXPORT_NO_SYMBOLS;
