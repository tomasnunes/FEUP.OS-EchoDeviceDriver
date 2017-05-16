/*
 * $Id: echo.c,v 1.0 2017/04/30 $
 */

#include "echo.h"

int echo_numdevs 	= ECHO_NUMDEVS;
int echo_minor 		= ECHO_MINOR;
int echo_major 		= ECHO_MAJOR;
//dev_t echo_devnum = 0;

module_param(echo_numdevs, int, S_IRUGO);
//module_param(echo_major, int, S_IRUGO);
//module_param(echo_minor, int, S_IRUGO);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Mafalda Varela & Tomas Nunes");

const static char *name = "echo";
struct echo_dev *echo_devices;

struct file_operations echo_fops = {
	.owner 		= THIS_MODULE,
	.open 		= echo_open,
	.read 		= echo_read,
	.write 		= echo_write,
	.llseek 	= no_llseek,
	.release	= echo_release
};

int echo_module_init(void) {
	int ii;
	dev_t echo_dev = 0;

	if(alloc_chrdev_region(&echo_dev, echo_minor, echo_numdevs, name) < 0) {
		printk(KERN_ALERT "Failed alloc... :(");
		return -1;
	}
	echo_major 	= MAJOR(echo_dev);
	echo_minor	= MINOR(echo_dev);

	echo_devices = kzalloc(echo_numdevs * sizeof(struct echo_dev), GFP_KERNEL);
	if(!echo_devices) {
		printk(KERN_ALERT "Failed alloc memory for devices... :(");
		unregister_chrdev_region(echo_dev, echo_numdevs);
		return -1;
	}

	for(ii=0; ii<echo_numdevs; ++ii) {
		cdev_init(&(echo_devices[ii].cdev), &echo_fops);
		echo_devices[ii].cdev.owner = THIS_MODULE;
		echo_devices[ii].cdev.ops = &echo_fops;
		echo_devices[ii].f_write = 0;
		echo_devices[ii].f_read = 0;
		echo_devices[ii].cwrite = 0;

		echo_dev = MKDEV(echo_major, echo_minor + ii);
		if(cdev_add(&(echo_devices[ii].cdev), echo_dev, 1))
			printk(KERN_ALERT "Failed cdev_add on %d device... :(", ii);

		printk(KERN_INFO "  -K- device %d was created!\n", echo_dev);
	}

	printk(KERN_NOTICE "\n!Hello %d!\n\n", echo_major);
	return 0;
}

int echo_open(struct inode *inodeptr, struct file *fileptr) {
	struct echo_dev *echo_device;

	//Informs kernel that 'echo' is non seekable (Blocks lseek() calls for 'echo')
	nonseekable_open(inodeptr, fileptr);

	echo_device = container_of(inodeptr->i_cdev, struct echo_dev, cdev);
	fileptr->private_data = echo_device;
	printk(KERN_INFO "  -K- echo_open, echo_device can be accessed through fileptr!\n");

	return 0;
}

int echo_release(struct inode *inodeptr, struct file *fileptr) {
	printk(KERN_INFO "  -K- echo_release was invoked!\n");

	return 0;
}

ssize_t echo_read(struct file *fileptr, char __user *buff, size_t cmax, loff_t *offptr) {
	//static int f_error;
	//ssize_t cread = 0;
	struct echo_dev *echo_device = fileptr->private_data;

	printk(KERN_INFO "  -K- echo_read was invoked!\n");
	printk(KERN_INFO "    -KR- There was a total of %d chars written to 'echo'!\n", echo_device->cwrite);

	return 0;
}

ssize_t echo_write(struct file *fileptr, const char __user *buff, size_t cmax, loff_t *offptr) {
	//Keep declarations and code separated, ISO C90 forbis it!
	ssize_t cwrite = 0, error = 0;
	struct echo_dev *echo_device = fileptr->private_data;
	char *kbuff;

	printk(KERN_INFO "  -K- echo_write was invoked!\n");
	if(echo_device->f_write) {
		kfree(kbuff);
		error = echo_device->f_write;
		echo_device->f_write = 0;
		return error;
	}

	kbuff = kzalloc(cmax+1, GFP_KERNEL);
	if(!kbuff) {
		printk(KERN_ALERT "  -K- echo_write could NOT alloc memory!\n");
		return -1;
	}

	printk(KERN_INFO "  -K- echo_write was invoked!\n");
	cwrite = cmax - copy_from_user(kbuff, buff, cmax);
	//printk(KERN_INFO "-(%d)-\n", cwrite);
	//cwrite = cmax - cwrite;
	echo_device->cwrite += cwrite;
	kbuff[cwrite] = '\0';

	if(cwrite == 0) {
		echo_device->f_write = 0;
		return -EFAULT; //No char written, return error
	}
	else if(cwrite < cmax)
		echo_device->f_write = -EFAULT; //Flags error to return on next call
	else
		echo_device->f_write = 0;

	printk(KERN_NOTICE "      -KW- %s", kbuff);

	kfree(kbuff);
	return cwrite;
}

void echo_module_exit(void) {
	int ii;
	dev_t echo_dev = MKDEV(echo_major, echo_minor);

	if(echo_devices) {
		for(ii=0; ii<echo_numdevs; ++ii) {
			cdev_del(&(echo_devices[ii].cdev));
		}

		kfree(echo_devices);
		echo_devices = NULL;
	}

	printk(KERN_NOTICE "\n!Goodbye, my fellow device %d!\n\n", echo_major);
	unregister_chrdev_region(echo_dev, echo_numdevs);

	return(0);
}

module_init(echo_module_init);
module_exit(echo_module_exit);
