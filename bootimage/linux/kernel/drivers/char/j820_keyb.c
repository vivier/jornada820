/*
 * Jornada 820 keyboard driver.
 * 2004/01/22 George Almasi (galmasi@optonline.net)
 * Modelled after gc_keyb.c
 *
 * Cannot (yet) handle Fn key combinations and Power On/Off
 * Depends on Russel King's SSP driver to work.
 *
 */

#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kbd_ll.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/kbd_kern.h>
#include <linux/pc_keyb.h>
#include <linux/random.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <asm/keyboard.h>
#include <asm/irq.h>
#include <asm/hardware.h>
#include <asm/hardware/ssp.h>
#include <asm/uaccess.h>

/*
00-07:  -   F1 1  q  a  z  \t -
08-0F:  -   F2 2  w  s  x  ^1 -
10-17:  -   F3 3  e  d  c  -  Alt
18-1F:  Win F4 4  r  f  v  -  `
20-27:  -   F5 5  t  g  b  -  -
28-2F:  SPC F6 6  y  h  n  -  -
30-37:  Fn  F7 7  u  j  m  <  -
38-3F:  Del F8 8  i  k  ;  >  -
40-47:  -   F9 9  o  l  '  ?  UP
48-4F:  DWN FA 0  p  [  ]  En LFT
50-57:  RGT FB -  +  BS \  ^2 -
58-5F:  -   -  -  -  -  -  -  -
60-67:  ESC -  -  -  -  -  -  -
68-6F:  Ctl -  -  -  -  -  -  -
70-7F:  POW
*/

/* I fixed George's keymap. Also mapped Power to F12, Fn to AltGr -- fare */
static char kbmap[128] = {
/* 00-07: */   0, 59,  2, 16, 30, 44, 15,  0,
/* 08-0F: */   0, 60,  3, 17, 31, 45, 42,  0,
/* 10-17: */   0, 61,  4, 18, 32, 46,  0, 56,
/* 18-1F: */ 126, 62,  5, 19, 33, 47,  0, 41,
/* 20-27: */   0, 63,  6, 20, 34, 48,  0,  0,
/* 28-2F: */  57, 64,  7, 21, 35, 49,  0,  0,
/* 30-37: */ 100, 65,  8, 22, 36, 50, 51,  0,
/* 38-3F: */  83, 66,  9, 23, 37, 39, 52,  0,
/* 40-47: */   0, 67, 10, 24, 38, 40, 53,103,
/* 48-4F: */ 108, 68, 11, 25, 26, 27, 28,105,
/* 50-57: */ 106, 87, 12, 13, 14, 43, 54,  0,
/* 58-5F: */   0,  0,  0,  0,  0,  0,  0,  0,
/* 60-67: */   1,  0,  0,  0,  0,  0,  0,  0,
/* 68-6F: */  97,  0,  0,  0,  0,  0,  0,  0,
/* 70-77: */  88,  0,  0,  0,  0,  0,  0,  0,
/* 78-7F: */   0,  0,  0,  0,  0,  0,  0,  0
};

int j820_kbd_setkeycode(unsigned int scancode, unsigned int keycode)
{
  kbmap[scancode&0x7F] = keycode;
  return 0;
}

int j820_kbd_getkeycode(unsigned int scancode)
{
  return kbmap[scancode &0x7F];
}

int j820_kbd_translate(unsigned char scancode, unsigned char *keycode,
		       char raw_mode)
{
  *keycode = kbmap[scancode & 0x7F];
  return 1;
}

char j820_kbd_unexpected_up(unsigned char keycode)
{
  return 0;
}

static void j820_kbd_irq(int irq, void *dev_id, struct pt_regs *regs)
{
  unsigned scancode;

#ifdef CONFIG_VT
  kbd_pt_regs = regs;
#endif
  disable_irq(irq);

  ssp_write_word(0x8200);
  scancode = ssp_read_word();
  handle_scancode(scancode, (scancode & 0x80) ? 0 : 1);

  enable_irq(irq);
}

static int  psaux_init(void);

void __init jornada820_kbd_init_hw(void)
{
  printk (KERN_INFO "Jornada 820 keyboard driver\n");

	/* init ? */
  
  k_setkeycode	  = j820_kbd_setkeycode;
  k_getkeycode	  = j820_kbd_getkeycode;
  k_translate	  = j820_kbd_translate;
  k_unexpected_up = j820_kbd_unexpected_up;
  
  /* ------------------------------ */
  /* Turn on the keyboard interrupt */
  /* ------------------------------ */

  set_GPIO_IRQ_edge (GPIO_JORNADA820_KEYBOARD, GPIO_FALLING_EDGE);
  if (request_irq(GPIO_JORNADA820_KEYBOARD_IRQ,
		  j820_kbd_irq,
		  0,
		  "j820_kbd_irq",
		  NULL) != 0)
    printk("Could not allocate IRQ for kbd!\n");

#ifdef CONFIG_PSMOUSE
  psaux_init();
#endif

}

#ifdef CONFIG_PSMOUSE


spinlock_t kbd_controller_lock = SPIN_LOCK_UNLOCKED;

static unsigned char mouse_reply_expected = 0;
static struct aux_queue *queue; /* Mouse data buffer. */
static int aux_count = 0;

static inline void handle_mouse_event(unsigned char scancode)
{
  if (mouse_reply_expected) 
    {
      if (scancode == AUX_ACK) 
	{
	  mouse_reply_expected--;
	  return;
	}
      mouse_reply_expected = 0;
    }

  add_mouse_randomness(scancode);
  if (aux_count)
    {
      int head = queue->head;
      queue->buf[head] = scancode;
      head = (head + 1) & (AUX_BUF_SIZE - 1);
      if (head != queue->tail)
	{
	  queue->head = head;
	  if (queue->fasync) kill_fasync(&queue->fasync, SIGIO, POLL_IN);
	  wake_up_interruptible(&queue->proc_list);
	}
    }
}

static void handle_mse_event(void)
{
  unsigned int msests = KBDSTAT;
  unsigned int work = 10000;
  unsigned char scancode;

  while (msests & KBDSTAT_RXF)
    {
      while (msests & KBDSTAT_RXF)
        {
          scancode = KBDDATA & 0xff;
          /* if (!(msests & KBDSTAT_STP)) */ handle_mouse_event(scancode);
          if (!--work)
            {
              printk(KERN_ERR
                     "pc_keyb: mouse controller jammed (0x%02X).\n",
                     msests);
              return;
            }
          msests = KBDSTAT;
        }
      work = 10000;
    }
}

static void ms_wait(void)
{
  unsigned long timeout = KBC_TIMEOUT;

  do 
    {
      handle_mse_event();
      if (KBDSTAT & KBDSTAT_TXE) return;
      mdelay(1);
      timeout--;
    }
  while (timeout);
#ifdef KBD_REPORT_TIMEOUTS
  printk(KERN_WARNING "Mouse timed out[1]\n");
#endif
}

static void mouse_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
  unsigned long flags;
  spin_lock_irqsave(&kbd_controller_lock, flags);
  handle_mse_event();
  spin_unlock_irqrestore(&kbd_controller_lock, flags);
}

static int __init detect_auxiliary_port(void)
{
  unsigned long flags;
  int loops = 10;
  int retval = 0;

  /* Check if the BIOS detected a device on the auxiliary port. */
  if (aux_device_present == 0xaa)
    return 1;

  spin_lock_irqsave(&kbd_controller_lock, flags);
  SKPCR |= SKPCR_PCLKEn;

  KBDCLKDIV = 0;
  KBDPRECNT = 127;
  KBDCR = KBDCR_ENA;
  mdelay(50);
  KBDDATA = 0xf4;
  mdelay(50);
  do
    {
      unsigned int msests = KBDSTAT;
      if (msests & KBDSTAT_RXF)
        {
          do
            {
              msests = KBDDATA;       /* dummy read */
              mdelay(50);
              msests = KBDSTAT;
            }
          while (msests & KBDSTAT_RXF);
          printk(KERN_INFO "Detected PS/2 Mouse Port.\n");
          retval = 1;
          break;
        }
      mdelay(1);
    }
  while (--loops);
  spin_unlock_irqrestore(&kbd_controller_lock, flags);
  return retval;
}

static void aux_write_dev(int val)
{
  unsigned long flags;
  spin_lock_irqsave(&kbd_controller_lock, flags);
  ms_wait();
  KBDDATA = val;
  spin_unlock_irqrestore(&kbd_controller_lock, flags);
}

static void aux_write_ack(int val)
{
  unsigned long flags;
  spin_lock_irqsave(&kbd_controller_lock, flags);
  ms_wait();
  KBDDATA = val;
  /* we expect an ACK in response. */
  mouse_reply_expected++;
  ms_wait();
  spin_unlock_irqrestore(&kbd_controller_lock, flags);
}

static unsigned char get_from_queue(void)
{
  unsigned char result;
  unsigned long flags;

  spin_lock_irqsave(&kbd_controller_lock, flags);
  result = queue->buf[queue->tail];
  queue->tail = (queue->tail + 1) & (AUX_BUF_SIZE - 1);
  spin_unlock_irqrestore(&kbd_controller_lock, flags);
  return result;
}

static inline int queue_empty(void)
{
  return queue->head == queue->tail;
}

static int fasync_aux(int fd, struct file *filp, int on)
{
  int retval;
  retval = fasync_helper(fd, filp, on, &queue->fasync);
  if (retval < 0) return retval;
  return 0;
}

#define SA1101_IRQMASK_LO(x)    (1 << (x - IRQ_SA1101_START))
#define SA1101_IRQMASK_HI(x)    (1 << (x - IRQ_SA1101_START - 32))


#define AUX_DEV ((void *)queue)
static int release_aux(struct inode *inode, struct file *file)
{
  fasync_aux(-1, file, 0);
  if (--aux_count) return 0;
  aux_write_ack(AUX_DISABLE_DEV); /* Disable aux device */
  KBDCR &= ~KBDCR_ENA;
  free_irq(IRQ_SA1101_TPRXINT, AUX_DEV);
  return 0;
}

static int open_aux(struct inode *inode, struct file *file)
{
  if (aux_count++) { return 0; }
  queue->head = queue->tail = 0;  /* Flush input queue */

  if (request_irq(IRQ_SA1101_TPRXINT,
                  mouse_interrupt, SA_SHIRQ, "PS/2 Trackpad",
                  AUX_DEV))
    {
      aux_count--;
      return -EBUSY;
    }

  KBDCLKDIV = 0;
  KBDPRECNT = 127;
  KBDCR &= ~KBDCR_ENA;
  mdelay(50);
  KBDCR = KBDCR_ENA;
  mdelay(50);
  KBDDATA = 0xf4;
  mdelay(50);
  if (KBDSTAT & 0x0100) { KBDSTAT = 0x0100; }
  aux_write_ack(AUX_ENABLE_DEV);  /* Enable aux device */
  return 0;
}

static ssize_t
read_aux(struct file *file, char *buffer, size_t count, loff_t * ppos)
{
  DECLARE_WAITQUEUE(wait, current);
  ssize_t i = count;
  unsigned char c;

  if (queue_empty())
    {
      if (file->f_flags & O_NONBLOCK) return -EAGAIN;
      add_wait_queue(&queue->proc_list, &wait);
    repeat:
      set_current_state(TASK_INTERRUPTIBLE);
      if (queue_empty() && !signal_pending(current))
        {
          schedule();
          goto repeat;
        }
      current->state = TASK_RUNNING;
      remove_wait_queue(&queue->proc_list, &wait);
    }
  while (i > 0 && !queue_empty())
    {
      c = get_from_queue();
      put_user(c, buffer++);
      i--;
    }
  if (count - i)
    {
      file->f_dentry->d_inode->i_atime = CURRENT_TIME;
      return count - i;
    }
  if (signal_pending(current)) return -ERESTARTSYS;
  return 0;
}

static ssize_t
write_aux(struct file *file, const char *buffer, size_t count,
          loff_t * ppos)
{
  ssize_t retval = 0;
  if (count)
    {
      ssize_t written = 0;
      if (count > 32) count = 32;     /* Limit to 32 bytes. */
      do
        {
          char c;
          get_user(c, buffer++);
          aux_write_dev(c);
          written++;
        }
      while (--count);
      retval = -EIO;
      if (written)
        {
          retval = written;
          file->f_dentry->d_inode->i_mtime = CURRENT_TIME;
        }
    }
  return retval;
}

static unsigned int aux_poll(struct file *file, poll_table * wait)
{
  poll_wait(file, &queue->proc_list, wait);
  if (!queue_empty()) return POLLIN | POLLRDNORM;
  return 0;
}

struct file_operations psaux_fops = {
  read:           read_aux,
  write:          write_aux,
  poll:           aux_poll,
  open:           open_aux,
  release:        release_aux,
  fasync:         fasync_aux,
};

static struct miscdevice psaux_mouse = {
  PSMOUSE_MINOR, "psaux", &psaux_fops
};

static int __init psaux_init(void)
{
  int ret;

  printk("Jornada820 mouse driver\n");

  if (!request_mem_region(_KBDCR, 512, "psaux"))
    return -EBUSY;

  if (!detect_auxiliary_port()) {
    ret = -EIO;
    goto out;
  }

  misc_register(&psaux_mouse);
  queue = (struct aux_queue *) kmalloc(sizeof(*queue), GFP_KERNEL);
  memset(queue, 0, sizeof(*queue));
  queue->head = queue->tail = 0;
  init_waitqueue_head(&queue->proc_list);

  aux_write_ack(AUX_SET_SAMPLE);
  aux_write_ack(100);     /* 100 samples/sec */
  aux_write_ack(AUX_SET_RES);
  aux_write_ack(3);       /* 8 counts per mm */
  aux_write_ack(AUX_SET_SCALE21); /* 2:1 scaling */
  ret = 0;

  printk("Jornada820 mouse driver registered\n");


 out:
  if (ret)
    release_mem_region(_KBDCR, 512);
  return ret;
}

#endif                          /* CONFIG_PSMOUSE */
