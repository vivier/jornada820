/****************************************************************************

  regs-sa1100.c: Register monitor of SA-1100

Modified version of registers.c by Sukjae Cho <sjcho@east.isi.edu>

This module is made for
debugging and testing purpose.

****************************************************************************/

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>               /* because we are a module */
#include <linux/init.h>                 /* for the __init macros */
#include <linux/proc_fs.h>              /* all the /proc functions */
#include <linux/ioport.h>
#include <asm/uaccess.h>                /* to copy to/from userspace */
#include <asm/arch/hardware.h>

#define MODULE_NAME "regmonsa1100"
#define DEV_DIRNAME "sa1100"
#define REG_DIRNAME "registers"

static ssize_t proc_read_reg(struct file * file, char * buf,
		size_t nbytes, loff_t *ppos);
static ssize_t proc_write_reg(struct file * file, const char * buffer,
		size_t count, loff_t *ppos);

static struct file_operations proc_reg_operations = {
	read:	proc_read_reg,
	write:	proc_write_reg
};

typedef struct sa1100_reg_entry {
	u32 phyaddr;
	char* name;
	char* description;
	unsigned short low_ino;
} sa1100_reg_entry_t;

static sa1100_reg_entry_t sa1100_regs[] =
{
/*	{ phyaddr,    name,     description } */
	{ 0x80000000, "UDCCR", "UDC control register" },
	{ 0x80000004, "UDCAR", "UDC address register" },
	{ 0x80000008, "UDCOMP", "UDC OUT max packet register" },
	{ 0x8000000C, "UDCIMP", "UDC IN max packet register" },
	{ 0x80000010, "UDCCS0", "UDC endpoint 0 control/status register" },
	{ 0x80000014, "UDCCS1", "UDC endpoint 1 (out) control/status register" },
	{ 0x80000018, "UDCCS2", "UDC endpoint 2 (in) control/status register" },
	{ 0x8000001C, "UDCD0", "UDC endpoint 0 data register" },
	{ 0x80000020, "UDCWC", "UDC endpoint 0 write count register" },
	{ 0x80000028, "UDCDR", "UDC transmit/receive data register (FIFOs)" },
	{ 0x80000030, "UDCSR", "UDC status/interrupt register" },

	{ 0x80010000, "Ser1UTCR0", "UART control register 0" },
	{ 0x80010004, "Ser1UTCR1", "UART control register 1" },
	{ 0x80010008, "Ser1UTCR2", "UART control register 2" },
	{ 0x8001000C, "Ser1UTCR3", "UART control register 3" },
	{ 0x80010014, "Ser1UTDR", "UART data register" },
	{ 0x8001001C, "Ser1UTSR0", "UART status register 0" },
	{ 0x80010020, "Ser1UTSR1", "UART status register 1" },

	{ 0x80020060, "SDCR0", "SDLC Control Register 0" },
	{ 0x80020064, "SDCR1", "SDLC Control Register 1" },
	{ 0x80020068, "SDCR2", "SDLC Control Register 2" },
	{ 0x8002006C, "SDCR3", "SDLC Control Register 3" },
	{ 0x80020070, "SDCR4", "SDLC Control Register 4" },
	{ 0x80020078, "SDDR", "SDLC data register" },
	{ 0x80020080, "SDSR0", "SDLC status register 0" },
	{ 0x80020084, "SDSR1", "SDLC status register 1" },

	{ 0x80030000, "Ser2UTCR0", "UART control register 0" },
	{ 0x80030004, "Ser2UTCR1", "UART control register 1" },
	{ 0x80030008, "Ser2UTCR2", "UART control register 2" },
	{ 0x8003000C, "Ser2UTCR3", "UART control register 3" },
	{ 0x80030010, "Ser2UTCR4", "UART control register 4" },
	{ 0x80030014, "Ser2UTDR", "UART data register" },
	{ 0x8003001C, "Ser2UTSR0", "UART status register 0" },
	{ 0x80030020, "Ser2UTSR1", "UART status register 1" },

	{ 0x80040060, "HSCR0", "HSSP control register 0" },
	{ 0x80040064, "HSCR1", "HSSP control register 1" },
	{ 0x8004006C, "HSDR", "HSSP data register" },
	{ 0x80040074, "HSSR0", "HSSP status register 0" },
	{ 0x80040078, "HSSR1", "HSSP status register 1" },

	{ 0x80050000, "Ser3UTCR0", "UART control register 0" },
	{ 0x80050004, "Ser3UTCR1", "UART control register 1" },
	{ 0x80050008, "Ser3UTCR2", "UART control register 2" },
	{ 0x8005000C, "Ser3UTCR3", "UART control register 3" },
	{ 0x80050014, "Ser3UTDR", "UART data register" },
	{ 0x8005001C, "Ser3UTSR0", "UART status register 0" },
	{ 0x80050020, "Ser3UTSR1", "UART status register 1" },

	{ 0x80060000, "MCCR0", "MCP control register 0" },
	{ 0x80060008, "MCDR0", "MCP data register 0" },
	{ 0x8006000C, "MCDR1", "MCP data register 1" },
	{ 0x80060010, "MCDR2", "MCP data register 2" },
	{ 0x80060018, "MCSR", "MCP status register" },

	{ 0x80070060, "SSCR0", "SSP control register 0" },
	{ 0x80070064, "SSCR1", "SSP control register 1" },
	{ 0x8007006C, "SSDR", "SSP data register" },
	{ 0x80070074, "SSSR", "SSP status register" },

	{ 0x90000000, "OSMR0", "OS timer match registers 0" },
	{ 0x90000004, "OSMR1", "OS timer match registers 1" },
	{ 0x90000008, "OSMR2", "OS timer match registers 2" },
	{ 0x9000000C, "OSMR3", "OS timer match registers 3" },
	{ 0x90000010, "OSCR", "OS timer counter register" },
	{ 0x90000014, "OSSR", "OS timer status register" },
	{ 0x90000018, "OWER", "OS timer watchdog enable register" },
	{ 0x9000001C, "OIER", "OS timer interrupt enable register" },

	{ 0x90010000, "RTAR", "Real-time clock alarm register" },
	{ 0x90010004, "RCNR", "Real-time clock count register" },
	{ 0x90010008, "RTTR", "Real-time clock trim register" },
	{ 0x90010010, "RTSR", "Real-time clock status register" },

	{ 0x90020000, "PMCR", "Power manager control register" },
	{ 0x90020004, "PSSR", "Power manager sleep status register" },
	{ 0x90020008, "PSPR", "Power manager scratchpad register" },
	{ 0x9002000C, "PWER", "Power manager wakeup enable register" },
	{ 0x90020010, "PCFR", "Power manager configuration register" },
	{ 0x90020014, "PPCR", "Power manager PLL configuration register" },
	{ 0x90020018, "PGSR", "Power manager GPIO sleep state register" },
	{ 0x9002001C, "POSR", "Power manager oscillator status register" },

	{ 0x90030000, "RSRR", "Reset controller software reset register" },
	{ 0x90030004, "RCSR", "Reset controller status register" },
	{ 0x90030008, "TUCR", "Reserved for test" },

	{ 0x90040000, "GPLR", "GPIO pin level register" },
	{ 0x90040004, "GPDR", "GPIO pin direction register" },
	{ 0x90040008, "GPSR", "GPIO pin output set register" },
	{ 0x9004000C, "GPCR", "GPIO pin output clear register" },
	{ 0x90040010, "GRER", "GPIO rising-edge register" },
	{ 0x90040014, "GFER", "GPIO falling-edge register" },
	{ 0x90040018, "GEDR", "GPIO edge detect status register" },
	{ 0x9004001C, "GAFR", "GPIO alternate function register" },

	{ 0x90050000, "ICIP", "Interrupt controller irq pending register" },
	{ 0x90050004, "ICMR", "Interrupt controller mask register" },
	{ 0x90050008, "ICLR", "Interrupt controller FIQ level register" },
	{ 0x9005000C, "ICCR", "Interrupt controller control register" },
	{ 0x90050010, "ICFP", "Interrupt controller FIQ pending register" },
	{ 0x90050020, "ICPR", "Interrupt controller pending register" },

	{ 0x90060000, "PPDR", "PPC pin direction register" },
	{ 0x90060004, "PPSR", "PPC pin state register" },
	{ 0x90060008, "PPAR", "PPC pin assignment register" },
	{ 0x9006000C, "PSDR", "PPC sleep mode direction register" },
	{ 0x90060010, "PPFR", "PPC pin flag register" },
	{ 0x90060028, "HSCR2", "HSSP control register 2" },
	{ 0x90060030, "MCCR1", "MCP control register 1" },

	{ 0xA0000000, "MDCNFG", "DRAM configuration register" },
	{ 0xA0000004, "MDCAS0", "DRAM CAS waveform shift register 0" },
	{ 0xA0000008, "MDCAS1", "DRAM CAS waveform shift register 1" },
	{ 0xA000000C, "MDCAS2", "DRAM CAS waveform shift register 2" },
	{ 0xA0000010, "MSC0", "Static memory control register 0" },
	{ 0xA0000014, "MSC1", "Static memory control register 1" },
	{ 0xA0000018, "MECR", "Expansion bus configuration register" },

	{ 0xB0000000, "DDAR0", "DMA device address register" },
	{ 0xB0000004, "DCSR0set", "DMA control/status register 0 - write ones to set" },
	{ 0xB0000008, "DCSR0clr", "DMA control/status register 0 - write ones to clear" },
	{ 0xB000000C, "DCSR0rd", "DMA control/status register 0 - read only" },
	{ 0xB0000010, "DBSA0", "DMA buffer A start address 0" },
	{ 0xB0000014, "DBTA0", "DMA buffer A transfer count 0" },
	{ 0xB0000018, "DBSB0", "DMA buffer B start address 0" },
	{ 0xB000001C, "DBTB0", "DMA buffer B transfer count 0" },

	{ 0xB0000020, "DDAR1", "DMA device address register 1" },
	{ 0xB0000024, "DCSR1set", "DMA control/status register 1 - write ones to set" },
	{ 0xB0000028, "DCSR1clr", "DMA control/status register 1 - write ones to clear" },
	{ 0xB000002C, "DCSR1rd", "DMA control/status register 1 - read only" },
	{ 0xB0000030, "DBSA1", "DMA buffer A start address 1" },
	{ 0xB0000034, "DBTA1", "DMA buffer A transfer count 1" },
	{ 0xB0000038, "DBSB1", "DMA buffer B start address 1" },
	{ 0xB000003C, "DBTB1", "DMA buffer B transfer count 1" },

	{ 0xB0000040, "DDAR2", "DMA device address register 2" },
	{ 0xB0000044, "DCSR2set", "DMA control/status register 2 - write ones to set" },
	{ 0xB0000048, "DCSR2clr", "DMA control/status register 2 - write ones to clear" },
	{ 0xB000004C, "DCSR2rd", "DMA control/status register 2 - read only" },
	{ 0xB0000050, "DBSA2", "DMA buffer A start address 2" },
	{ 0xB0000054, "DBTA2", "DMA buffer A transfer count 2" },
	{ 0xB0000058, "DBSB2", "DMA buffer B start address 2" },
	{ 0xB000005C, "DBTB2", "DMA buffer B transfer count 2" },

	{ 0xB0000060, "DDAR3", "DMA device address register 3" },
	{ 0xB0000064, "DCSR3set", "DMA control/status register 3 - write ones to set" },
	{ 0xB0000068, "DCSR3clr", "DMA control/status register 3 - Write ones to clear" },
	{ 0xB000006C, "DCSR3rd", "DMA control/status register 3 - Read only" },
	{ 0xB0000070, "DBSA3", "DMA buffer A start address 3" },
	{ 0xB0000074, "DBTA3", "DMA buffer A transfer count 3" },
	{ 0xB0000078, "DBSB3", "DMA buffer B start address 3" },
	{ 0xB000007C, "DBTB3", "DMA buffer B transfer count 3" },

	{ 0xB0000080, "DDAR4", "DMA device address register 4" },
	{ 0xB0000084, "DCSR4set", "DMA control/status register 4 - write ones to set" },
	{ 0xB0000088, "DCSR4clr", "DMA control/status register 4 - write ones to clear" },
	{ 0xB000008C, "DCSR4rd", "DMA control/status register 4 - read only" },
	{ 0xB0000090, "DBSA4", "DMA buffer A start address 4" },
	{ 0xB0000094, "DBTA4", "DMA buffer A transfer count 4" },
	{ 0xB0000098, "DBSB4", "DMA buffer B start address 4" },
	{ 0xB000009C, "DBTB4", "DMA buffer B transfer count 4" },

	{ 0xB00000A0, "DDAR5", "DMA device address register 5" },
	{ 0xB00000A4, "DCSR5set", "DMA control/status register 5 - write ones to set" },
	{ 0xB00000A8, "DCSR5clr", "DMA control/status register 5 - write ones to clear" },
	{ 0xB00000AC, "DCSR5rd", "DMA control/status register 5 - read only" },
	{ 0xB00000B0, "DBSA5", "DMA buffer A start address 5" },
	{ 0xB00000B4, "DBTA5", "DMA buffer A transfer count 5" },
	{ 0xB00000B8, "DBSB5", "DMA buffer B start address 5" },
	{ 0xB00000BC, "DBTB5", "DMA buffer B transfer count 5" },

	{ 0xB0100000, "LCCR0", "LCD controller control register 0" },
	{ 0xB0100004, "LCSR", "LCD controller status register" },
	{ 0xB0100010, "DBAR1", "DMA channel 1 base address register" },
	{ 0xB0100014, "DCAR1", "DMA channel 1 current address register" },
	{ 0xB0100018, "DBAR2", "DMA channel 2 base address register" },
	{ 0xB010001C, "DCAR2", "DMA channel 2 current address register" },
	{ 0xB0100020, "LCCR1", "LCD controller control register 1" },
	{ 0xB0100024, "LCCR2", "LCD controller control register 2" },
	{ 0xB0100028, "LCCR3", "LCD controller control register 3" }
};

#define NUM_OF_SA1100_REG_ENTRY	(sizeof(sa1100_regs)/sizeof(sa1100_reg_entry_t))

static int proc_read_reg(struct file * file, char * buf,
		size_t nbytes, loff_t *ppos)
{
	int i_ino = (file->f_dentry->d_inode)->i_ino;
	char outputbuf[15];
	int count;
	int i;
	sa1100_reg_entry_t* current_reg=NULL;
	if (*ppos>0) /* Assume reading completed in previous read*/
		return 0;
	for (i=0;i<NUM_OF_SA1100_REG_ENTRY;i++) {
		if (sa1100_regs[i].low_ino==i_ino) {
			current_reg = &sa1100_regs[i];
			break;
		}
	}
	if (current_reg==NULL)
		return -EINVAL;

	count = sprintf(outputbuf, "0x%08X\n",
			*((volatile unsigned int *) io_p2v(current_reg->phyaddr)));
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
	sa1100_reg_entry_t* current_reg=NULL;
	int i;
	unsigned long newRegValue;
	char *endp;

	for (i=0;i<NUM_OF_SA1100_REG_ENTRY;i++) {
		if (sa1100_regs[i].low_ino==i_ino) {
			current_reg = &sa1100_regs[i];
			break;
		}
	}
	if (current_reg==NULL)
		return -EINVAL;

	newRegValue = simple_strtoul(buffer,&endp,0);
	*((volatile unsigned int *) io_p2v(current_reg->phyaddr))=newRegValue;
	return (count+endp-buffer);
}

static struct proc_dir_entry *regdir;
static struct proc_dir_entry *cpudir;

static int __init init_regsa1100_monitor(void)
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

	for(i=0;i<NUM_OF_SA1100_REG_ENTRY;i++) {
		entry = create_proc_entry(sa1100_regs[i].name,
				S_IWUSR |S_IRUSR | S_IRGRP,
				regdir);
		if(entry) {
			sa1100_regs[i].low_ino = entry->low_ino;
			entry->proc_fops = &proc_reg_operations;
		} else {
			printk( KERN_ERR MODULE_NAME
				": can't create /proc/" REG_DIRNAME
				"/%s\n", sa1100_regs[i].name);
			return(-ENOMEM);
		}
	}
	printk("regsa1100 loaded.\n");
	return (0);
}

static void __exit cleanup_regsa1100_monitor(void)
{
	int i;
	for(i=0;i<NUM_OF_SA1100_REG_ENTRY;i++)
		remove_proc_entry(sa1100_regs[i].name,regdir);
	remove_proc_entry(REG_DIRNAME, cpudir);
	remove_proc_entry(DEV_DIRNAME, &proc_root);
}

module_init(init_regsa1100_monitor);
module_exit(cleanup_regsa1100_monitor);

MODULE_DESCRIPTION("SA1100 Register monitor");
MODULE_LICENSE("GPL");

EXPORT_NO_SYMBOLS;
