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
	//.read 		= serp_read,
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
		//serp_devices[ii].f_read = 0;
		serp_devices[ii].cwrite = 0;
		serp_devices[ii].lcr = 0;
		serp_devices[ii].dll = 0;
		serp_devices[ii].dlm = 0;

		serp_devices[ii].uart = request_region(coms_address[ii], serp_numports, name);
  	if(!serp_devices[ii].uart) {
  		printk(KERN_ALERT "Failed request_region for device %d... :(", ii);
  		return -1;
  	}

		printk(KERN_INFO "  -K- device %d was created with ports 0x%X-0x%X!\n", serp_dev, serp_devices[ii].uart->start, serp_devices[ii].uart->end);
	}

	printk(KERN_NOTICE "\n!Hello %d!\n\n", serp_major);

	return 0;
}

/*{
	unsigned char lcr = 0;
  int dl = serp_freq/serp_bitrate;
  unsigned char dll = 0;
  unsigned char dlm = 0;
  char thr = 0;

  serp_devices = kzalloc(serp_numdevs * sizeof(struct resource *), GFP_KERNEL);
	if(!serp_devices) {
		printk(KERN_ALERT "Failed alloc memory for devices... :(");
		return -1;
	}

  for(ii=0; ii<serp_numdevs; ++ii) {
    serp_devices[ii] = request_region(coms_address[ii], serp_numports, name);
  	if(!serp_devices[ii]) {
  		printk(KERN_ALERT "Failed request_region for device %d... :(", ii);
  		return -1;
  	}

    lcr = 0x0 | UART_LCR_WLEN8
              | UART_LCR_STOP
              | UART_LCR_PARITY | UART_LCR_EPAR
              | UART_LCR_DLAB;
    outb(lcr, serp_devices[ii]->start + UART_LCR);

    dl = serp_freq/serp_bitrate;
    dll = 0x0 | (dl & 0x00FF);
    outb(dll, serp_devices[ii]->start + UART_DLL);

    dlm = 0x0 | (dl >> 8);
    outb(dlm, serp_devices[ii]->start + UART_DLM);

    lcr &= ~UART_LCR_DLAB;
    outb(lcr, serp_devices[ii]->start + UART_LCR);

    thr = 'M';

		for(ii=0; ii<50 ; ++ii) {
    	if(!(inb(serp_devices[ii]->start + UART_LSR) & UART_LSR_THRE)) {
        schedule();
    	}

			outb((unsigned char)thr, serp_devices[ii]->start + UART_TX);
		}

    printk(KERN_NOTICE "\n!Device %d (port 0x%X-0x%X) send a char to the otherside!\n\n", ii, serp_devices[ii]->start, serp_devices[ii]->end);
	}

	return 0;
}*/

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

	printk(KERN_INFO "  -K- serp_open initialized device with %ld frequency and %ld bit-rate\n", serp_freq, serp_bitrate);

	return 0;
}

/*ssize_t serp_read(struct file *fileptr, char __user *buff, size_t cmax, loff_t *offptr) {
	//static int f_error;
	//ssize_t cread = 0;
	struct serp_dev *serp_device = fileptr->private_data;

	printk(KERN_INFO "  -K- serp_read was invoked!\n");
	printk(KERN_INFO "    -KR- There was a total of %d chars written to 'serp'!\n", serp_device->cwrite);

	return 0;
}*/

ssize_t serp_write(struct file *fileptr, const char __user *buff, size_t cmax, loff_t *offptr) {
	//Keep declarations and code separated, ISO C90 forbis it!
	int ii;
	ssize_t cwrite = 0, error = 0;
	struct serp_dev *serp_device = fileptr->private_data;
	char *kbuff;

	printk(KERN_INFO "  -K- serp_write was invoked!\n");
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

	printk(KERN_INFO "  -K- serp_write was invoked!\n");
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
			; //schedule();
		}

		outb((unsigned char)serp_device->thr, serp_device->uart->start + UART_TX);
	}

	kfree(kbuff);
	printk(KERN_INFO "  -K- serp_write has been send to the otherside!\n");

	return cwrite;
}

int serp_release(struct inode *inodeptr, struct file *fileptr) {
	printk(KERN_INFO "  -K- serp_release was invoked!\n");

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

/*{
	printk(KERN_NOTICE "\n!Goodbye, my fellow devices!\n");

  for(ii=0; ii<serp_numdevs; ++ii) {
  	printk(KERN_NOTICE "  - Goodbye port 0x%X-0x%X\n", serp_devices[ii]->start, serp_devices[ii]->end);
    release_region(coms_address[ii], serp_numports);
  }
  printk("\n");

  kfree(serp_devices);

	return(0);
}*/

module_init(serp_module_init);
module_exit(serp_module_exit);
