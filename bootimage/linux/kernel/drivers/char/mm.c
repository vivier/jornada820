/*
 *  linux/drivers/char/memaccess.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include <asm/irq.h>
#include <asm/uaccess.h>

#define MEMACCESS_NAME "Mem"

static int memaccess_users = 0;

static int memaccess_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long a) {
    unsigned int i, j;
    void * arg = (void *)a;
    
    if(cmd==197) {
        if(verify_area(VERIFY_READ, arg, sizeof(int)))
            return -EFAULT;
        copy_from_user(&i, arg, sizeof(int));
        j=*(unsigned char *)i;
        return j;
    } else 
    
    if(cmd==198) {
        if(verify_area(VERIFY_READ, arg, sizeof(int)))
            return -EFAULT;
        copy_from_user(&i, arg, sizeof(int));
        j=*(unsigned short *)i;
        return j;
    } else 
    
    if(cmd==199) {
        if(verify_area(VERIFY_READ, arg, sizeof(int)))
            return -EFAULT;
        copy_from_user(&i, arg, sizeof(int));
        j=*(unsigned int *)i;
        return j;
    } else 
    
    if(cmd == 297) {
        if(verify_area(VERIFY_READ, arg, 2*sizeof(int)))
            return -EFAULT;
        copy_from_user(&i, arg, sizeof(int));
        copy_from_user(&j, arg+sizeof(int), sizeof(int));
        *(unsigned char *)i = j;
    } else
    
    if(cmd == 298) {
        if(verify_area(VERIFY_READ, arg, 2*sizeof(int)))
            return -EFAULT;
        copy_from_user(&i, arg, sizeof(int));
        copy_from_user(&j, arg+sizeof(int), sizeof(int));
        *(unsigned short *)i = j;
    } else 
    
    if(cmd == 299) {
        if(verify_area(VERIFY_READ, arg, 2*sizeof(int)))
            return -EFAULT;
        copy_from_user(&i, arg, sizeof(int));
        copy_from_user(&j, arg+sizeof(int), sizeof(int));
        *(unsigned int *)i = j;
    } 
    return 0;

}

static int memaccess_open(struct inode *inode, struct file *file){
  MOD_INC_USE_COUNT;
  ++memaccess_users;
  return 0;
}

static int memaccess_release(struct inode *inode, struct file *file){
  --memaccess_users;
  MOD_DEC_USE_COUNT;
  return 0;
}

static struct file_operations memaccess_ops = {
  open:    memaccess_open,
  release: memaccess_release,
  ioctl:   memaccess_ioctl
};

static struct miscdevice memaccess_misc = {
  97 , MEMACCESS_NAME, &memaccess_ops
};
    

static int __init memaccess_init(void){

  if(misc_register(&memaccess_misc)<0){
    printk(KERN_ERR "%s: unable to register misc device\n",
           MEMACCESS_NAME);
    return -EIO;
  }
 
  return 0;
}  

static void __exit memaccess_exit(void){

  if(misc_deregister(&memaccess_misc)<0)
    printk(KERN_ERR "%s: unable to deregister misc device\n",
           MEMACCESS_NAME);
}

module_init(memaccess_init);
module_exit(memaccess_exit);

MODULE_LICENSE("GPL");
