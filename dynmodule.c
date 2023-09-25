#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#include "dynmodule.h"

MODULE_LICENSE("DUAL BSD/GPL");
MODULE_AUTHOR("BAIBRAHIM with help of scull in LDD3 ");


int dynmodul_major = DYNMODUL_MAJOR;
int dynmodul_minor = 0;
int dynmodul_nr_devs = DYNMODUL_NR_DEVS;
int dynmodul_quantum = DYNMODUL_QUANTUM;
int dynmodul_qset = DYNMODUL_QSET;


struct dynmodul_dev *dynmodul_devices;


int dynmodul_trim(struct dynmodul_dev *dev) {
	struct dynmodul_qset *next, *dptr;
	int qset = dev->qset; /*dev is nit null*/
	int i;
	
	for (dptr = dev->data; dptr; dptr = next) {/*all list items*/
		if (dptr->data) {
			for (i = 0; i < qset; i++)
				kfree(dptr->data[i]);
			kfree(dptr->data);
			dptr->data = NULL;
		}
		next =dptr->next;
		kfree(dptr);
	}
	dev->size = 0;
	dev->quantum = dynmodul_quantum;
	dev->qset = dynmodul_qset;
	dev->data = NULL;
	return 0;
}
	

int dynmodul_open(struct inode *inode, struct file *filp) {
	struct dynmodul_dev *dev; /*device infos*/
	
	dev = container_of(inode->i_cdev, struct dynmodul_dev, cdev);
	filp->private_data = dev; /*for other methids*/
	
	printk(KERN_ALERT "Device opened\n");
	return 0;
}


int dynmodul_mrelease(struct inode *inode, struct file *filp) {
	return 0;
	printk(KERN_ALERT "Device Released");
}


struct dynmodul_qset *dynmodul_follow(struct dynmodul_dev *dev, int n){
	struct dynmodul_qset *qs = dev->data;

        /* Allocate first qset explicitly if need be */
	if (! qs) {
		qs = dev->data = kmalloc(sizeof(struct dynmodul_qset), GFP_KERNEL);
		if (qs == NULL)
			return NULL;  /* Never mind */
		memset(qs, 0, sizeof(struct dynmodul_qset));
	}

	/* Then follow the list */
	while (n--) {
		if (!qs->next) {
			qs->next = kmalloc(sizeof(struct dynmodul_qset), GFP_KERNEL);
			if (qs->next == NULL)
				return NULL;  /* Never mind */
			memset(qs->next, 0, sizeof(struct dynmodul_qset));
		}
		qs = qs->next;
		continue;
	}
	return qs;
}


static ssize_t dynmodul_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	
	struct dynmodul_dev *dev = filp->private_data;
	struct dynmodul_qset *dptr; /*first list item*/
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset; /*how many bytes in the listitem*/
	int item, s_pos, q_pos, rest;
	ssize_t retval = 0;
	
	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;
	if (*f_pos >= dev->size)
		goto out;
	if (*f_pos + count > dev->size)
		count = dev->size - *f_pos;

	/* find listitem, qset index, and offset in the quantum */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; q_pos = rest % quantum;

	/* follow the list up to the right position (defined elsewhere) */
	dptr = dynmodul_follow(dev, item);

	if (dptr == NULL || !dptr->data || ! dptr->data[s_pos])
		goto out; /* don't fill holes */

	/* read only up to the end of this quantum */
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		retval = -EFAULT;
		goto out;
	}
	
	*f_pos += count;
	retval = count;
	
	//printk(KERN_NOTICE"QSET %s /n",&__user buf);
	
	
	
	
	

  out:
	mutex_unlock(&dev->lock);
	return retval;
	
}


static ssize_t dynmodul_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
	
	struct dynmodul_dev *dev = filp->private_data;
	struct dynmodul_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	/* find listitem, qset index and offset in the quantum */
	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	s_pos = rest / quantum; q_pos = rest % quantum;

	/* follow the list up to the right position */
	dptr = dynmodul_follow(dev, item);
	if (dptr == NULL)
		goto out;
	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data)
			goto out;
		memset(dptr->data, 0, qset * sizeof(char *));
	}
	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		if (!dptr->data[s_pos])
			goto out;
	}
	/* write only up to the end of this quantum */
	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_from_user(dptr->data[s_pos]+q_pos, buf, count)) {
		retval = -EFAULT;
		goto out;
	}
	*f_pos += count;
	retval = count;

        /* update the size */
	if (dev->size < *f_pos)
		dev->size = *f_pos;

  out:
	mutex_unlock(&dev->lock);
	return retval;
	
	//text[len] = '\0';
	//text_size = len;
	
	//printk(KERN_INFO "Recieved text %s\n", text);
	//return len;
}


 /* The "extended" operations -- only seek*/

loff_t dynmodul_llseek(struct file *filp, loff_t off, int whence)
{
	struct dynmodul_dev *dev = filp->private_data;
	loff_t newpos;

	switch(whence) {
	  case 0: /* SEEK_SET */
		newpos = off;
		break;

	  case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	  case 2: /* SEEK_END */
		newpos = dev->size + off;
		break;

	  default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0) return -EINVAL;
	filp->f_pos = newpos;
	return newpos;
}




/*File Opereation with the libaray linux/fs.h*/
struct file_operations dynmodul_fops = {
	.read = dynmodul_read,
	.write = dynmodul_write,
	.open = dynmodul_open,
	.release = dynmodul_mrelease,
	.llseek = dynmodul_llseek
	
};



void dynmodul_cleanup (void)
{
	int i;
	dev_t devno = MKDEV(dynmodul_major, dynmodul_minor);

	/* Get rid of our char dev entries */
	if (dynmodul_devices) {
		for (i = 0; i < dynmodul_nr_devs; i++) {
			dynmodul_trim(dynmodul_devices + i);
			cdev_del(&dynmodul_devices[i].cdev);
		}
		kfree(dynmodul_devices);
	}


	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, dynmodul_nr_devs);
	printk(KERN_NOTICE "Modul removed");

	
}




/* Set up the char_dev structure for this device.*/

static void dynmodul_setup_cdev(struct dynmodul_dev *dev, int index)
{
	int err, devno = MKDEV(dynmodul_major, dynmodul_minor + index);
    
	cdev_init(&dev->cdev, &dynmodul_fops); //refering to the file operations in the libary
	dev->cdev.owner = THIS_MODULE; // refering to the modul itself and macroized in the libary modul.h 
	dev->cdev.ops = &dynmodul_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding cdevice%d", err, index);
}


int dynmodul_init(void) {
	
	int result, i;
	dev_t dev = 0;
	
	printk(KERN_INFO "Message Module initialized.\n");
	
	if (dynmodul_major) {
		dev = MKDEV(dynmodul_major, dynmodul_minor);
		result = register_chrdev_region(dev, dynmodul_nr_devs, "dynmodul");
	} else {
		printk (KERN_ALERT "MODUL ALLOCATED");
		result = alloc_chrdev_region(&dev, dynmodul_minor, dynmodul_nr_devs, "dynmodul");
		dynmodul_major = MAJOR(dev);
	}
	if (result < 0) {
		printk(KERN_WARNING "dynmodul: can't get Major numbre %d\n", dynmodul_major);
		return result;
	}
	
	dynmodul_devices = kmalloc(dynmodul_nr_devs * sizeof(struct dynmodul_dev), GFP_KERNEL);
	
	if (!dynmodul_devices) {
		result = -ENOMEM;
		//*goto fail;
	}
	memset(dynmodul_devices, 0, dynmodul_nr_devs * sizeof(struct dynmodul_dev));
	
	/*intilize each device.*/
	
	for (i = 0; i < dynmodul_nr_devs; i++) {
		dynmodul_devices[i].quantum = dynmodul_quantum;
		dynmodul_devices[i].qset = dynmodul_qset;
		mutex_init(&dynmodul_devices[i].lock);
		dynmodul_setup_cdev(&dynmodul_devices[i], i);
	}
	
	/*call the init func for any freinds devices*/
	dev = MKDEV(dynmodul_major, dynmodul_minor + dynmodul_nr_devs);
	//dev += dynmodul_p_init(dev);
	//dev += dynmodul_access_init(dev);
	return result;
}


module_init(dynmodul_init);
module_exit(dynmodul_cleanup);
