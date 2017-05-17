/*
 * $Id: serp.c,v 1.0 2017/05/14 $
 */

#include "serp.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Mafalda Varela & Tomas Nunes");

const static char *name = "serp";
struct serp_dev *serp_devices;

int serp_minor 		= SERP_MINOR;
int serp_major 		= SERP_MAJOR;
int serp_numdevs  = SERP_NUMDEVS;
int serp_numports = SERP_NUMPORTS;
long serp_freq    = SERP_FREQ;
long serp_bitrate = SERP_BITRATE;
int serp_delay		= SERP_DELAY;

//Base address of each COM port
unsigned long coms_address[MAX_COMS] = { ADDRESS_COM1,
																				 ADDRESS_COM2,
																				 ADDRESS_COM3,
																				 ADDRESS_COM4};

//Get number of devices from script
module_param(serp_numdevs, int, S_IRUGO);
module_param(serp_freq, long, S_IRUGO);
module_param(serp_bitrate, long, S_IRUGO);
//module_param(serp_major, int, S_IRUGO);
//module_param(serp_minor, int, S_IRUGO);

struct file_operations serp_fops = {
	.owner 		= THIS_MODULE,
	.open 		= serp_open,
	.read 		= serp_read,
	.write 		= serp_write,
	.llseek 	= no_llseek,
	.release	= serp_release
};

int serp_module_init(void) {
	int ii;
	dev_t serp_dev = 0;

	if(alloc_chrdev_region(&serp_dev, serp_minor, serp_numdevs, name) < 0) {
		printk(KERN_ALERT "Failed alloc... :(");
		return -1;
	}
	serp_major 	= MAJOR(serp_dev);
	serp_minor	= MINOR(serp_dev);

	serp_devices = kzalloc(serp_numdevs * sizeof(struct serp_dev), GFP_KERNEL);
	if(!serp_devices) {
		printk(KERN_ALERT "Failed alloc memory for devices... :(");
		unregister_chrdev_region(serp_dev, serp_numdevs);
		return -1;
	}

	for(ii=0; ii<serp_numdevs; ++ii) {
		serp_dev = MKDEV(serp_major, serp_minor + ii);

		cdev_init(&(serp_devices[ii].cdev), &serp_fops);
		serp_devices[ii].cdev.owner = THIS_MODULE;
		serp_devices[ii].cdev.ops = &serp_fops;
		if(cdev_add(&(serp_devices[ii].cdev), serp_dev, 1))
			printk(KERN_ALERT "Failed cdev_add on %d device... :(", ii);

		serp_devices[ii].f_write = 0;
		serp_devices[ii].f_idle = 0;
		serp_devices[ii].cwrite = 0;
		serp_devices[ii].lcr = 0;
		serp_devices[ii].dll = 0;
		serp_devices[ii].dlm = 0;

		serp_devices[ii].uart = request_region(coms_address[ii], serp_numports, name);
  	if(!serp_devices[ii].uart) {
  		printk(KERN_ALERT "Failed request_region for device %d... :(", ii);
  		return -1;
  	}

		printk(KERN_ALERT "  -K- device %d was created with ports 0x%X-0x%X!\n", serp_dev, serp_devices[ii].uart->start, serp_devices[ii].uart->end);
	}

	printk(KERN_NOTICE "\n!Hello %d!\n\n", serp_major);

	return 0;
}

int serp_open(struct inode *inodeptr, struct file *fileptr) {
	struct serp_dev *serp_device;
	int dl = serp_freq/serp_bitrate;

	//Informs kernel that 'serp' is non seekable (Blocks lseek() calls for 'serp')
	nonseekable_open(inodeptr, fileptr);

	serp_device = container_of(inodeptr->i_cdev, struct serp_dev, cdev);
	fileptr->private_data = serp_device;

	serp_device->lcr = 0x0 | UART_LCR_WLEN8
						| UART_LCR_STOP
						| UART_LCR_PARITY | UART_LCR_EPAR
						| UART_LCR_DLAB;
	outb(serp_device->lcr, serp_device->uart->start + UART_LCR);

	serp_device->dll = 0x0 | (dl & 0x00FF);
	outb(serp_device->dll, serp_device->uart->start + UART_DLL);

	serp_device->dlm = 0x0 | (dl >> 8);
	outb(serp_device->dlm, serp_device->uart->start + UART_DLM);

	serp_device->lcr &= ~UART_LCR_DLAB;
	outb(serp_device->lcr, serp_device->uart->start + UART_LCR);

	printk(KERN_ALERT "  -K- serp_open initialized device with %ld frequency and %ld bit-rate\n", serp_freq, serp_bitrate);

	return 0;
}

ssize_t serp_read(struct file *fileptr, char __user *buff, size_t cmax, loff_t *offptr) {
	int ii = 0;
	ssize_t cread = 0;
	struct serp_dev *serp_device = fileptr->private_data;
	char *kbuff;

	printk(KERN_ALERT "  -K- serp_read was invoked!\n");

	kbuff = kzalloc(cmax-1, GFP_KERNEL);
	if(!kbuff) {
		printk(KERN_ALERT "  -K- serp_read could NOT alloc memory!\n");
		return -1;
	}

	serp_device->f_idle = 0;
	while(!serp_device->f_idle && cread < cmax) {
		if(inb(serp_device->uart->start + UART_LSR) & UART_LSR_OE) {
			printk(KERN_ALERT "  -K- serp_read there was an Override Error, is not sufficiently fast!\n");
			return -EIO;
		}
		else if(inb(serp_device->uart->start + UART_LSR) & UART_LSR_FE) {
			printk(KERN_ALERT "  -K- serp_read there was a Farming Error!\n");
			return -1;
		}
		else if(inb(serp_device->uart->start + UART_LSR) & UART_LSR_PE) {
			printk(KERN_ALERT "  -K- serp_read there was a Parity Error!\n");
			return -1;
		}
		else if(inb(serp_device->uart->start + UART_LSR) & UART_LSR_DR) {
			kbuff[cread++] = inb(serp_device->uart->start + UART_RX);
			ii = 0;
		}
		else {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(serp_delay);
			if(++ii == 50)
				serp_device->f_idle = 1;
		}
	}

	kbuff[cread] = '\0';
	if(copy_to_user(buff, kbuff, cread+1) > 0) {
		printk(KERN_ALERT "  -K- serp_read there was a problem copying to user space!\n");
		return -1;
	}

	printk(KERN_ALERT "    -KR- There was a total of %d chars written to this side!\n", cread);

	return cread;
}

ssize_t serp_write(struct file *fileptr, const char __user *buff, size_t cmax, loff_t *offptr) {
	//Keep declarations and code separated, ISO C90 forbis it!
	int ii;
	ssize_t cwrite = 0, error = 0;
	struct serp_dev *serp_device = fileptr->private_data;
	char *kbuff;

	printk(KERN_ALERT "  -K- serp_write was invoked!\n");
	if(serp_device->f_write) {
		error = serp_device->f_write;
		serp_device->f_write = 0;
		return error;
	}

	kbuff = kzalloc(cmax+1, GFP_KERNEL);
	if(!kbuff) {
		printk(KERN_ALERT "  -K- serp_write could NOT alloc memory!\n");
		return -1;
	}

	cwrite = cmax - copy_from_user(kbuff, buff, cmax);
	kbuff[cwrite] = '\0';

	serp_device->cwrite += cwrite;

	if(cwrite == 0) {
		serp_device->f_write = 0;
		return -EFAULT; //No char written, return error
	}
	else if(cwrite < cmax)
		serp_device->f_write = -EFAULT; //Flags error to return on next call
	else
		serp_device->f_write = 0;

	for(ii=0; ii<cwrite; ++ii) {
		serp_device->thr = kbuff[ii];

		while(!(inb(serp_devices->uart->start + UART_LSR) & UART_LSR_THRE)) {
			set_current_state(TASK_INTERRUPTIBLE);
			schedule_timeout(serp_delay);
		}

		outb((unsigned char)serp_device->thr, serp_device->uart->start + UART_TX);
	}

	kfree(kbuff);
	printk(KERN_ALERT "  -K- serp_write ended, buffer has been send to the otherside!\n");

	return cwrite;
}

int serp_release(struct inode *inodeptr, struct file *fileptr) {
	printk(KERN_ALERT "  -K- serp_release was invoked!\n");

	return 0;
}

void serp_module_exit(void) {
	int ii;
	dev_t serp_dev = MKDEV(serp_major, serp_minor);

	if(serp_devices) {
		for(ii=0; ii<serp_numdevs; ++ii) {
			printk(KERN_NOTICE "  - Goodbye port 0x%X-0x%X\n", serp_devices[ii].uart->start, serp_devices[ii].uart->end);
			cdev_del(&(serp_devices[ii].cdev));
	    release_region(coms_address[ii], serp_numports);
		}

		kfree(serp_devices);
		serp_devices = NULL;
	}

	printk(KERN_NOTICE "\n!Goodbye, my fellow device %d!\n\n", serp_major);
	unregister_chrdev_region(serp_dev, serp_numdevs);

	return;
}

module_init(serp_module_init);
module_exit(serp_module_exit);
