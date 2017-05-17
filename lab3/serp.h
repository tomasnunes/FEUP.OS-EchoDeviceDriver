/*
 * $Id: serp.c,v 1.0 2017/05/14 $
 */

#ifndef _SERP_H
#define _SERP_H

#include "serial_reg.h"
#include <linux/ioport.h>
#include <asm/io.h>
#include <linux/sched.h>

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

#define SERP_NUMDEVS  1
#define SERP_NUMPORTS 8
#define SERP_FREQ     115200
#define SERP_BITRATE  1200
#define ADDRESS_COM1  0x3f8
#define ADDRESS_COM2  0x0 //NOT IN USE
#define ADDRESS_COM3  0x0 //NOT IN USE
#define ADDRESS_COM4  0x0 //NOT IN USE
#define MAX_COMS      4

#define SERP_MINOR 0
#define SERP_MAJOR 0
#define SERP_NUMDEVS 1

struct serp_dev {
  struct cdev cdev;
  int f_write;
  //int f_read;
  ssize_t cwrite; //Counter of written chars
  struct resource *uart;
  unsigned char lcr;
  unsigned char dll;
  unsigned char dlm;
  char thr;
};

extern int serp_minor;
extern int serp_major;
extern int serp_numdevs;
extern long serp_bitrate;
extern long serp_freq;
extern unsigned long coms_address[MAX_COMS];

int serp_open(struct inode *inodeptr, struct file *fileptr);
int serp_release(struct inode *inodeptr, struct file *fileptr);
ssize_t serp_write(struct file *fileptr, const char __user *buff, size_t cmax, loff_t *offptr);
ssize_t serp_read(struct file *fileptr, char __user *buff, size_t cmax, loff_t *offptr);

#endif
