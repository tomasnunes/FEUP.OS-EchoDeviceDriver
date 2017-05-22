/*
 * $Id: seri.c,v 1.0 2017/05/14 $
 */

#ifndef _SERI_H
#define _SERI_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#include "serial_reg.h"
#include <linux/ioport.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/delay.h>

#include <linux/interrupt.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
//#include <linux/workqueue.h>

#define SERI_MINOR    0
#define SERI_MAJOR    0
#define SERI_NUMDEVS  1
#define SERI_NUMPORTS 8
#define SERI_FREQ     115200
#define SERI_BITRATE  1200
#define MAX_COMS      4
#define ADDRESS_COM1  0x3F8
#define ADDRESS_COM2  0x2F8 //NOT IN USE
#define ADDRESS_COM3  0x3E8 //NOT IN USE
#define ADDRESS_COM4  0x2E8 //NOT IN USE
#define SERI_DELAY    100
#define SERI_IRQ      4
#define MAX_FIFO_BUFF 4096

struct seri_dev_t {
  struct cdev cdev;
  int f_write; //Flags error for next call in seri_write
  int f_read;  //Flags error for next call in seri_read
  int f_idle;  //Flags user idle in read
  ssize_t cwrite; //Total of written chars
  ssize_t cread;  //Total of read chars
  struct resource *uart;
  unsigned char lcr;
  unsigned char dll;
  unsigned char dlm;
  unsigned char lsr;
  unsigned char ier;
  unsigned char iir;
  //struct semaphore *sem; //No need, using kfifo lock!
  size_t cmax;
  char *tbuff;
  spinlock_t lread, lwrite; //Read and Write locks for kfifos
  struct kfifo *kfread, *kfwrite; //Read and Write kfifos
  wait_queue_head_t qread, qwrite; //Read and Write Wait Queues
};

extern int seri_minor;
extern int seri_major;
extern int seri_numdevs;
extern int seri_numports;
extern long seri_bitrate;
extern long seri_freq;
extern unsigned long coms_address[MAX_COMS];
extern int seri_delay;

int seri_open(struct inode *inodeptr, struct file *fileptr);
int seri_release(struct inode *inodeptr, struct file *fileptr);
ssize_t seri_write(struct file *fileptr, const char __user *buff, size_t cmax, loff_t *offptr);
ssize_t seri_read(struct file *fileptr, char __user *buff, size_t cmax, loff_t *offptr);
irqreturn_t seri_interrupt(int irq, void *dev_id);

#endif
