/*
 * $Id: seri.c,v 1.0 2017/05/14 $
 */

#include "seri.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Mafalda Varela & Tomas Nunes");

const static char *name = "seri";
static struct seri_dev_t *seri_devices;

int seri_minor 				= SERI_MINOR;
int seri_major 				= SERI_MAJOR;
int seri_numdevs  		= SERI_NUMDEVS;
int seri_numports 		= SERI_NUMPORTS;
long seri_freq    		= SERI_FREQ;
long seri_bitrate 		= SERI_BITRATE;
int seri_delay				= SERI_DELAY;
unsigned int seri_irq = SERI_IRQ;

//Base address of each COM port
unsigned long coms_address[MAX_COMS] = { ADDRESS_COM1,
																				 ADDRESS_COM2,
																				 ADDRESS_COM3,
																				 ADDRESS_COM4 };

//(Optional) Get number of devices, freq and bitrate from script
module_param(seri_numdevs, int, S_IRUGO);
module_param(seri_freq, long, S_IRUGO);
module_param(seri_bitrate, long, S_IRUGO);
module_param(seri_irq, int, S_IRUGO);

struct file_operations seri_fops = {
	.owner 		= THIS_MODULE,
	.open 		= seri_open,
	.read 		= seri_read,
	.write 		= seri_write,
	.llseek 	= no_llseek,
	.release	= seri_release
};

int seri_module_init(void) {
	//Keep declarations and code separated, ISO C90 forbis it!
	int ii;
	dev_t seri_dev = 0;

	if(alloc_chrdev_region(&seri_dev, seri_minor, seri_numdevs, name) < 0) {
		printk(KERN_ALERT "Failed alloc... :(");
		return -1;
	}
	seri_major 	= MAJOR(seri_dev);
	seri_minor	= MINOR(seri_dev);

	seri_devices = kzalloc(seri_numdevs * sizeof(struct seri_dev_t), GFP_KERNEL);
	if(!seri_devices) {
		printk(KERN_ALERT "Failed alloc memory for devices... :(");
		unregister_chrdev_region(seri_dev, seri_numdevs);
		return -ENOMEM;
	}

	for(ii=0; ii<seri_numdevs; ++ii) {
		seri_dev = MKDEV(seri_major, seri_minor + ii);

		cdev_init(&(seri_devices[ii].cdev), &seri_fops);
		seri_devices[ii].cdev.owner = THIS_MODULE;
		seri_devices[ii].cdev.ops = &seri_fops;

		spin_lock_init(&seri_devices[ii].lread);
		spin_lock_init(&seri_devices[ii].lwrite);

		if(cdev_add(&(seri_devices[ii].cdev), seri_dev, 1))
			printk(KERN_ALERT "Failed cdev_add on %d device... :(", ii);

		seri_devices[ii].uart = request_region(coms_address[ii], seri_numports, name);
		if(!seri_devices[ii].uart)
  		printk(KERN_ALERT "Failed request_region for device %d... :(", ii);

		printk(KERN_ALERT "  -K- device %d was created with ports 0x%X-0x%X!\n", seri_dev, seri_devices[ii].uart->start, seri_devices[ii].uart->end);
	}

	printk(KERN_NOTICE "\n!Hello %d!\n\n", seri_major);

	return 0;
}

int seri_open(struct inode *inodeptr, struct file *fileptr) {
	struct seri_dev_t *seri_device;
	int dl = seri_freq/seri_bitrate;

	//Informs kernel that 'seri' is non seekable (Blocks lseek() calls for 'seri')
	nonseekable_open(inodeptr, fileptr);

	seri_device = container_of(inodeptr->i_cdev, struct seri_dev_t, cdev);
	fileptr->private_data = seri_device;

	seri_device->kfread = kfifo_alloc(MAX_FIFO_BUFF, GFP_KERNEL, &seri_device->lread);
	seri_device->kfwrite = kfifo_alloc(MAX_FIFO_BUFF, GFP_KERNEL, &seri_device->lwrite);
	if(!seri_device->kfread || !seri_device->kfwrite) {
		printk(KERN_ALERT "  -K- seri_read could NOT alloc kfifos!\n");
		return -ENOMEM;
	}

	init_waitqueue_head(&seri_device->qread);
	init_waitqueue_head(&seri_device->qwrite);
	seri_device->f_write 	= 0;
	seri_device->f_read 	= 0;
	seri_device->f_idle 	= 0;
	seri_device->cwrite 	= 0;
	seri_device->cread 		= 0;
	seri_device->lcr 			= 0;
	seri_device->dll 			= 0;
	seri_device->dlm 			= 0;
	seri_device->lsr 			= 0;
	seri_device->ier			= 0;
	seri_device->iir			= 0;

	seri_device->lcr |= UART_LCR_WLEN8
									 | UART_LCR_STOP
									 | UART_LCR_PARITY | UART_LCR_EPAR
									 | UART_LCR_DLAB;
	outb(seri_device->lcr, seri_device->uart->start + UART_LCR);

	seri_device->dll = (dl & 0x00FF);
	outb(seri_device->dll, seri_device->uart->start + UART_DLL);

	seri_device->dlm = (dl >> 8);
	outb(seri_device->dlm, seri_device->uart->start + UART_DLM);

	seri_device->lcr &= ~UART_LCR_DLAB;
	outb(seri_device->lcr, seri_device->uart->start + UART_LCR);

	seri_device->ier |= UART_IER_THRI | UART_IER_RDI | UART_IER_RLSI;
	outb(seri_device->ier, seri_device->uart->start + UART_IER);

	//Cleans Receiver
	seri_device->lsr = inb(seri_device->uart->start + UART_LSR);
	while(seri_device->lsr & UART_LSR_DR) {
		inb(seri_device->uart->start + UART_RX);
		seri_device->lsr = inb(seri_device->uart->start + UART_LSR);
	}

	if(request_irq(seri_irq, seri_interrupt, SA_SHIRQ | SA_INTERRUPT, name, seri_device))
		printk(KERN_ALERT "Failed request_irq can't be assigned IRQ %d... :(", seri_irq);

	printk(KERN_ALERT "  -K- seri_open initialized device with %ld freq | %ld bit-rate | %d dl (dlm-%d  dll-%d)\n", seri_freq, seri_bitrate, dl, seri_device->dlm, seri_device->dll);

	return 0;
}

ssize_t seri_read(struct file *fileptr, char __user *buff, size_t cmax, loff_t *offptr) {
	struct seri_dev_t *seri_device = fileptr->private_data;
	char *tbuff;
	int length, result;
	ssize_t cread = 0;

	//printk(KERN_ALERT "  -K- seri_read was invoked!\n");

	//If file as ended, return immediately
	if(seri_device->f_read) {
		seri_device->f_read = 0;
		return 0;
	}

	tbuff = kzalloc(cmax, GFP_KERNEL);
	if(!tbuff) {
		printk(KERN_ALERT "  -K- seri_read could NOT alloc memory for tmp buff!\n");
		return -ENOMEM;
	}

	do {
		result = wait_event_interruptible_timeout(seri_device->qread, (seri_device->kfread->in != seri_device->kfread->out), seri_delay);
		length = seri_device->kfread->in - seri_device->kfread->out;
		if(length > cmax - cread)
			length = cmax - cread;

		//printk(KERN_ALERT "\n  -K- seri_read length = %d! | result = %d\n", length, result);

		if(result == -ERESTARTSYS) {
			kfree(tbuff);
			return -ERESTARTSYS;
		}
		else if(!result && cread) {
			seri_device->f_read = 1;
			break;
		}
		else {
			if(kfifo_get(seri_device->kfread, tbuff+cread, length) < length) {
				printk(KERN_ALERT "\n  -K- seri_read there was a problem copying from the fifo!\n");
				kfree(tbuff);
				return -1;
			}
			cread += length;
		}
	} while(cread < cmax);

	if(copy_to_user(buff, tbuff, cread) > 0) {
		printk(KERN_ALERT "\n  -K- serp_read there was a problem copying to user space!\n");
		kfree(tbuff);
		return -1;
	}

	//printk(KERN_ALERT "\n    -KR- There was a total of %d chars written to this side!\n", seri_device->cread);
	kfree(tbuff);
	seri_device->cread += cread;
	return cread;
}

ssize_t seri_write(struct file *fileptr, const char __user *buff, size_t cmax, loff_t *offptr) {
	struct seri_dev_t *seri_device = fileptr->private_data;
	ssize_t cwrite = 0, error = 0, cleft = 0;
	char tchar, *tbuff;
	int result, offset = 0;

	//printk(KERN_ALERT "  -K- seri_write was invoked!\n");

	if(seri_device->f_write) {
		error = seri_device->f_write;
		seri_device->f_write = 0;
		return error;
	}

	tbuff = kzalloc(cmax, GFP_KERNEL);
	if(!tbuff) {
		printk(KERN_ALERT "  -K- seri_write could NOT alloc memory!\n");
		return -ENOMEM;
	}

	cleft = cmax - copy_from_user(tbuff, buff, cmax);
	seri_device->cwrite += cleft;

	if(cleft == 0) {
		kfree(tbuff);
		return -EFAULT; //No char written, return error
	}
	else if(cleft < cmax)
		seri_device->f_write = -EFAULT; //Flags error to return on next call

	while(cleft > 0) {
		if(seri_device->kfwrite->in == seri_device->kfwrite->out) {
			if(cleft > seri_device->kfwrite->size)
				cwrite = seri_device->kfwrite->size;
			else
				cwrite = cleft;

			cleft -= cwrite;
			if(kfifo_put(seri_device->kfwrite, tbuff+offset, cwrite) < cwrite) {
				printk(KERN_ALERT "  -K- seri_write fail on copy to kfwrite!\n");
				kfree(tbuff);
				return -1;
			}
			offset += cwrite;

			if((inb(seri_device->uart->start + UART_LSR) & UART_LSR_THRE)) {
				if(kfifo_get(seri_device->kfwrite, &tchar, 1) < 1) {
					printk(KERN_ALERT "  -K- seri_write fail on getting from kfwrite!\n");
					kfree(tbuff);
					return -1;
				}

				outb(tchar, seri_device->uart->start + UART_TX);
			}
		}

		result = wait_event_interruptible_timeout(seri_device->qwrite, (seri_device->kfwrite->in == seri_device->kfwrite->out), seri_delay);
		if(result == -ERESTARTSYS) {
			kfree(tbuff);
			return -ERESTARTSYS;
		}
	}

	kfree(tbuff);
	//printk(KERN_ALERT "  -K- seri_write ended, buffer has been send to the otherside!\n");

	return cmax;
}

int seri_release(struct inode *inodeptr, struct file *fileptr) {
	struct seri_dev_t *seri_device = fileptr->private_data;
	printk(KERN_ALERT "  -K- seri_release was invoked!\n\n    -K- There was a total number of %d chars read and\n        a total of %d chars written to the otherside!!\n", seri_device->cread, seri_device->cwrite);

	kfifo_free(seri_device->kfread);
	kfifo_free(seri_device->kfwrite);

	return 0;
}

void seri_module_exit(void) {
	int ii;
	dev_t seri_dev = MKDEV(seri_major, seri_minor);

	if(seri_devices) {
		for(ii=0; ii<seri_numdevs; ++ii) {
			printk(KERN_NOTICE "  - Goodbye port 0x%X-0x%X\n", seri_devices[ii].uart->start, seri_devices[ii].uart->end);
			cdev_del(&(seri_devices[ii].cdev));
	    release_region(coms_address[ii], seri_numports);
			free_irq(seri_irq, &seri_devices[ii]);
		}

		kfree(seri_devices);
		seri_devices = NULL;
	}

	printk(KERN_NOTICE "\n!Goodbye, my fellow device %d!\n\n", seri_major);
	unregister_chrdev_region(seri_dev, seri_numdevs);

	return;
}

irqreturn_t seri_interrupt(int irq, void *dev_id) {
	struct seri_dev_t *seri_device = (struct seri_dev_t *)dev_id;
	char tchar;

	//printk(KERN_ALERT "  -K- seri_interrupt was invoked!\n"); //Slows down the program..

	seri_device->lsr = inb(seri_device->uart->start + UART_LSR);
	seri_device->iir = inb(seri_device->uart->start + UART_IIR);

	if(seri_device->iir & UART_IIR_RDI) {
		tchar = inb(seri_device->uart->start + UART_RX);
		if(kfifo_put(seri_device->kfread, &tchar, 1) < 1) {
			printk(KERN_ALERT "  -K- seri_interrupt kfread is full!\n");
		}

		wake_up_interruptible(&seri_device->qread); //Awake read function
		return IRQ_HANDLED;
	}
	else if(seri_device->iir & UART_IIR_THRI) {
		if(kfifo_get(seri_device->kfwrite, &tchar, 1) < 1) {
			//printk(KERN_ALERT "  -K- seri_interrupt kfwrite is empty!\n");
			wake_up_interruptible(&seri_device->qwrite); //Awake write function
			return IRQ_HANDLED;
		}

		outb(tchar, seri_device->uart->start + UART_TX);

		return IRQ_HANDLED;
	}

	return IRQ_NONE;
}

module_init(seri_module_init);
module_exit(seri_module_exit);
