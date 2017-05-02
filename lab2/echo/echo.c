/*
 * $Id: echo.c,v 1.0 2017/04/30 $
 */
//MODULE_LICENSE("Dual BSD/GPL");
//MODULE_AUTHOR("Mafalda Varela & Tomas Nunes");

#include "echo.h"

int echo_numdevs = ECHO_NUMDEVS;
int echo_minor = ECHO_MINOR;
int echo_major = ECHO_MAJOR;
int echo_devnum = 0;

module_param(echo_numdevs, int, S_IRUGO);
module_param(echo_major, int, S_IRUGO);
module_param(echo_minor, int, S_IRUGO);

static dev_t dev;
static char *name = "echo";
static struct echo_dev echo_device;

struct file_operations echo_fops = {
	.owner = THIS_MODULE,
	.open = echo_open,
	.read = echo_read,
	.write = echo_write,
	.llseek = no_llseek,
	.release = echo_release
};

int echo_module_init(void) {
	if(alloc_chrdev_region(&dev, echo_minor, echo_numdevs, name)) {
		printk(KERN_ALERT "Failed alloc... :(");
		return -1;
	}
	echo_major = MAJOR(dev);
	echo_minor = MINOR(dev);
	echo_devnum = MKDEV(echo_major, echo_minor);

	if(!(echo_device.cdev = cdev_alloc())) {
		printk(KERN_ALERT "Failed alloc char device... :(");
		unregister_chrdev_region(echo_devnum, echo_numdevs);
		return -1;
	}
	cdev_init(echo_device.cdev, &echo_fops);
	echo_device.cdev->owner = THIS_MODULE;
	echo_device.cdev->ops = &echo_fops;

	if(cdev_add(echo_device.cdev, echo_devnum, 1) < 0) {
		printk(KERN_ALERT "Failed cdev_add... :(");
		unregister_chrdev_region(echo_devnum, echo_numdevs);
		return -1;
	}

	printk(KERN_NOTICE "\n!Hello %d!\n\n", echo_major);
	return 0;
}

int echo_open(struct inode *inodeptr, struct file *fileptr) {
	//Informs kernel that 'echo' is non seekable (Blocks lseek() calls for 'echo')
	nonseekable_open(inodeptr, fileptr);

	fileptr->private_data = echo_device.cdev;
	printk(KERN_INFO "  -K- echo_open can access cdev through file.private_data!\n");

	return 0;
}

int echo_release(struct inode *inodeptr, struct file *fileptr) {
	printk(KERN_INFO "  -K- echo_release was invoked!\n");

	return 0;
}

ssize_t echo_read(struct file *fileptr, char __user *buff, size_t cmax, loff_t *offptr) {
	static int f_error;
	ssize_t cread = 0;

	printk(KERN_INFO "  -K- echo_read was invoked!\n");

	return cread;
}

ssize_t echo_write(struct file *fileptr, const char __user *buff, size_t cmax, loff_t *offptr) {
	//Keep declarations and code separated, ISO C90 forbis it!
	static ssize_t f_error = 0;
	ssize_t cwrite = 0, error;
	char *kbuff = kmalloc(cmax+1, GFP_KERNEL);

	printk(KERN_INFO "  -K- echo_write was invoked!\n");
	/*if((f_error =! 0)) { //Double parentheses to avoid warning... (Don't ask me??)
		kfree(kbuff);
		error = f_error;
		f_error = 0;
		return error;
	}*/

	printk(KERN_INFO "  -K- echo_write was invoked!\n");
	cwrite = cmax - copy_from_user(kbuff, buff, cmax);
	kbuff[cwrite] = '\0';

	if(cwrite == 0) {
		f_error = 0;
		return -EFAULT; //No char written, return error
	}
	else if(cwrite < cmax)
		f_error = -EFAULT; //Flags error to return on next call
	else
		f_error = 0;

	printk(KERN_NOTICE "%s", kbuff);

	kfree(kbuff);
	return cwrite;
}

void echo_module_exit(void) {
	cdev_del(echo_device.cdev);
	printk(KERN_NOTICE "\n!Goodbye, my fellow device %d!\n\n", echo_major);
	unregister_chrdev_region(echo_devnum, echo_numdevs);
}

module_init(echo_module_init);
module_exit(echo_module_exit);
