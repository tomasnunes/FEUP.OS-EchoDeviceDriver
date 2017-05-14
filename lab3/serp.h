/*
 * $Id: serp.c,v 1.0 2017/05/14 $
 */

#ifndef _ECHO_H
#define _ECHO_H

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

#define ECHO_MINOR 0
#define ECHO_MAJOR 0
#define ECHO_NUMDEVS 1

struct echo_dev {
  struct cdev cdev;
  int f_write;
  int f_read;
  ssize_t cwrite; //Counter of written chars
};

//extern int echo_numdevs;
extern int echo_minor;
extern int echo_major;

int echo_open(struct inode *inodeptr, struct file *fileptr);
int echo_release(struct inode *inodeptr, struct file *fileptr);
ssize_t echo_read(struct file *fileptr, char __user *buff, size_t cmax, loff_t *offptr);
ssize_t echo_write(struct file *fileptr, const char __user *buff, size_t cmax, loff_t *offptr);

#endif
