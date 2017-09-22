/* CSCI 5103 Fall 2016
 * Assignment # 7
 * name: Alexander Cina, Danielle Stewart
 * student id: 4515522, 4339650
 * x500 id: cinax020, dkstewar
 * CSELABS machine: csel-x07-umh
 */
 
 /* The following code is modified by Alexander Cina 
  * and Danielle Stewart for the project
  * scullbuffer. 
  */
 
 
 /*
 * scullbuffer.c -- fifo driver for scull
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>	/* printk(), min() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/proc_fs.h>
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/init.h>
#include "scullbuffer.h"		/* local definitions */

#define init_MUTEX(_m) sema_init(_m, 1);

#define SCULLDEBUG // Comment this out when finished with debugging

#ifdef SCULLDEBUG
#define KDEBUG(...) printk(KERN_DEBUG __VA_ARGS__)
#else
#define KDEBUG(...)
#endif

// Module parameters
static int n_items = 20;
module_param(n_items,int,S_IRUGO);

// The 32 byte item struct
typedef struct item_s {
	char data[32];
} item_t;

// The scullbuffer struct
struct scull_buffer {
        wait_queue_head_t inq, outq;       /* read and write queues */
        char *buffer, *end;                /* begin of buf, end of buf */
        int buffersize;                    /* used in pointer arithmetic */
        char *rp, *wp;                     /* where to read, where to write */
        int nreaders, nwriters;            /* number of openings for r/w */
        struct semaphore sem;              /* mutual exclusion semaphore */
        struct cdev cdev;                  /* Char device structure */
};


/* parameters */
static int scull_b_nr_devs = SCULL_B_NR_DEVS;   /* number of pipe devices */
int scull_b_buffer =  SCULL_B_BUFFER;
int scull_major =   SCULL_MAJOR;
static int nitems = NITEMS;
int scull_minor =   0;
dev_t scull_b_devno;                    /* Our first device number */

// Set parameters for device
module_param(scull_b_nr_devs, int, 0);  /* FIXME check perms */
module_param(scull_b_buffer, int, 0);
module_param(nitems, int, 0);
module_param(scull_major, int, 0);
module_param(scull_minor, int, 0);



/* scull_b_devices:
 * The scullbuffer devices we will be using. 
 */
static struct scull_buffer *scull_b_devices;

static int spacefree(struct scull_buffer *dev);

/* scull_b_open:
 * Parameters: inode struct pointer inode
 *						   file struct pointer filp
 *
 * - Get ptr to scull_buffer (use container_of inode)
 * - Store in file struct private_data section
 * - Lock semaphore in interruptible mode
 * - Allocate buffer located in scull_buffer struct (if not yet done)
 * - Set r/w pointers to start at beginning of buffer
 * - If reader called open, increment reader count
 *     Else increment writer count
 * - Release semaphore
 */
static int scull_b_open(struct inode *inode, struct file *filp)
{
        struct scull_buffer *dev;

	KDEBUG("Entered open in scullbuffer.\n");

        dev = container_of(inode->i_cdev, struct scull_buffer, cdev);
        filp->private_data = dev;

	PDEBUG("scull_b_open locking sem\n");
        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;
        if (!dev->buffer) {
                /* allocate the buffer */
                dev->buffer = kmalloc(sizeof(item_t)*NITEMS, GFP_KERNEL);
                if (!dev->buffer) {
			PDEBUG("scull_b_open unlocking sem\n");
                        up(&dev->sem);
                        return -ENOMEM;
                }
        }
        
        dev->end = dev->buffer + (NITEMS-1) * sizeof(item_t);
        dev->rp = dev->wp = dev->buffer; /* rd and wr from the beginning */

        /* use f_mode,not  f_flags: it's cleaner (fs/open.c tells why) */
        if (filp->f_mode & FMODE_READ)
                dev->nreaders++;
        if (filp->f_mode & FMODE_WRITE)
                dev->nwriters++;
        PDEBUG("scull_b_open unlocking sem\n");
	up(&dev->sem);

        return nonseekable_open(inode, filp);
}



/* scull_b_release:
 * Parameters: inode struct pointer
 * 						 file struct pointer
 *
 * - Get ptr to scull_buffer (use container_of inode)
 * - Store in file struct private_data section
 * - Lock semaphore 
 * - If a reader called this, decrement reader count
 *     Else decrement writer count
 * - If there are no more readers/writers, free buffer memory
 * - Unlock semaphore
 */
static int scull_b_release(struct inode *inode, struct file *filp)
{
        struct scull_buffer *dev = filp->private_data;
	
	PDEBUG("scull_b_release locking sem\n");
        down_interruptible(&dev->sem);
        if (filp->f_mode & FMODE_READ) {
                dev->nreaders--;
	}
        if (filp->f_mode & FMODE_WRITE) {
                dev->nwriters--;
	}
        if (dev->nreaders + dev->nwriters == 0) {
                kfree(dev->buffer);
                dev->buffer = NULL; /* the other fields are not checked on open */
        }
	PDEBUG("scull_b_release unlocking sem\n");
        up(&dev->sem);
	PDEBUG("scull_b_release finished for process %d\n",current->pid);
	if (dev->nreaders <= 0) {
		wake_up_all(&dev->outq);
	}
	if (dev->nwriters <= 0) {
		wake_up_all(&dev->inq);
	}
        return 0;
}

/* scull_b_read:
 * Parameters: file struct pointer
 * user space buffer pointer
 * size_t count (used to return amt read from buffer)
 * loff_t file position pointer
 *
 * - Get ptr to scull_buffer (use container_of inode)
 * - Store in file struct private_data section
 * - Lock semaphore 
 * - While there is nothing to read (consume) wait here
 * - If wp > rp then return difference between wp and rp
 *     Else return the data up to the end of buffer (end - rp)
 * - Try to copy_to_user the data consumed from buffer
 *     (Fail gracefully if unsuccessful)
 * - Increment rp by count consumed
 *     If this case ends up reaching the end of the buffer, 
 *     we wrapped around, so set rp equal to beginning of buffer
 * - Unlock semaphore
 * - Wake up writers (producers)
 * - Return amount consumed (count)
*/
static ssize_t scull_b_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
        struct scull_buffer *dev = filp->private_data;

	PDEBUG("scull_b_read locking sem\n");
        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;

        PDEBUG("Beginning read:\" (scull_b_read) dev->wp:%p    dev->rp:%p\" \n",dev->wp,dev->rp);

        while (dev->rp == dev->wp) { /* nothing to read */
                // If there are no writers, return 0
		if (dev->nwriters == 0) {
			// First release the semaphore
			PDEBUG("scull_b_read unlocking sem, no writers\n");
			up(&dev->sem);
			return 0;
		}
		PDEBUG("scull_b_read unlocking sem\n");
		up(&dev->sem); /* release the lock */
                if (filp->f_flags & O_NONBLOCK)
                        return -EAGAIN;
                PDEBUG("Process %d reading: going to sleep\n", current->pid);
                if (wait_event_interruptible(dev->inq, ((dev->rp != dev->wp) || (dev->nwriters != 0))))
                        return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
                
                //if(dev->nwriters <= 0) {
                //  return 0;	
                //}
                
                
                /* otherwise loop, but first reacquire the lock */
		PDEBUG("scull_b_read locking sem\n");
                if (down_interruptible(&dev->sem))
                        return -ERESTARTSYS;
        }
        /* ok, data is there, return something */
        if (dev->wp > dev->rp)
                count = min(count, (size_t)(dev->wp - dev->rp));
        else /* the write pointer has wrapped, return data up to dev->end */
                count = min(count, (size_t)(dev->end - dev->rp));
        PDEBUG("About to copy an item of size %d to the reader %d\n",(int)count,current->pid);
	if (copy_to_user(buf, dev->rp, count)) {
		PDEBUG("scull_b_read unlocking sem\n");
                up (&dev->sem);
                return -EFAULT;
        }
	PDEBUG("Just copied an item of size %d to the reader %d\n",(int)count,current->pid);
        dev->rp += count;
        if (dev->rp == dev->end)
                dev->rp = dev->buffer; /* wrapped */
	PDEBUG("scull_b_read unlocking sem\n");
        up (&dev->sem);



	PDEBUG("scull_b_read waking up writers\n");
        /* finally, awake any writers and return */
        wake_up_all(&dev->outq);
        PDEBUG("Process %d did read %li bytes\n",current->pid, (long)count);
        return count;
}

/* scull_getwritespace:
 * Parameters: scull_buffer struct pointer
 * 						 file struct pointer
 *
 *   Producer calls this to wait for space in buffer
 *   Instead of busy waiting, we will call the scheduler
 *   in order to not waste CPU cycles. 
 *   It's assumed that the caller (producer) holds the lock 
 *   on a semaphore before calling this function. 
 * - If the buffer is full:
 *     - Release semaphore
 *     - If NONBLOCK is specified, behavior is slightly different:
 *       We just return -EAGAIN which really means "try it again"
 *     - Else (NONBLOCK not specified) call prepare_to_wait
 *       This will specify the wait queue (producer->outq) and that we 
 *       want to wait in interruptible mode
 *     - If there is still no space in the buffer, call the scheduler and
 *       get put on the wait queue
 *     - Once we are out of the wait queue, call finish_wait - cleanup after wait
 *     - Check for pending signals 
 *     - Lock the semaphore again
 * - This all occurs in a while loop, thus it will continue until space is 
 *   available in the buffer
 */
static int scull_getwritespace(struct scull_buffer *dev, struct file *filp)
{
        while (spacefree(dev) == 0) { /* full */
                DEFINE_WAIT(wait);
                // Check if there are no readers
		if (dev->nreaders == 0) {
			PDEBUG("scull_getwritespace releasing sem\n");
			up(&dev->sem);
			return 1;
		}
		PDEBUG("scull_getwritespace releasing sem\n");
                up(&dev->sem);
                if (filp->f_flags & O_NONBLOCK) {
                        return -EAGAIN;
		}
                PDEBUG("Process %d writing: going to sleep\n",current->pid);
                prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
                if (spacefree(dev) == 0) {
                        schedule();
		}
                finish_wait(&dev->outq, &wait);
		if (signal_pending(current)) {
                        return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
		}
		PDEBUG("scull_getwritespace locking sem\n");
                if (down_interruptible(&dev->sem)) {
                        return -ERESTARTSYS;
		}
        }
        return 0;
}       

/* spacefree:
 * Parameter: scull_buffer struct pointer
 *
 * - If the rp, wp pointers are equal, then the buffer is empty
 *   so the space available is equal to buffersize-1 (we leave one
 *   slot empty). 
 *   Return this value
 * - Else, we return a messy pointer arithmatic statement
 *   that just calculates how much space is available in buffer 
 */
static int spacefree(struct scull_buffer *dev)
{
        if (dev->rp == dev->wp)
                return dev->buffersize - 1*sizeof(item_t);
	PDEBUG("spacefree, dev->buffersize = %d\n",dev->buffersize);
        return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) - 1*sizeof(item_t);
}


/* scull_b_write:
 * Parameters: file struct pointer
 * user space buffer pointer
 * size_t count (used to return amt written to buffer)
 * loff_t file position poninter
 *
 * - Get ptr to scull_buffer (use container_of inode)
 * - Store in file struct private_data section
 * - Lock semaphore
 * - Make sure there is space to write by a call to scull_getwritespace
 *   If there is, we get amount specified by a call to spacefree
 * - If wp >= rp then we write the difference between end and wp (to end of buffer)
 *     Else the write pointer has wrapped around, so we will up to rp-1
 * - Try to copy_from_user
 *     (Fail gracefully if unsuccessful)
 * - Increment wp pointer by value we wrote (count)
 * - Release semaphore
 * - Wake up readers (consumers)
 * - Return count (what we read/produced)
 */
static ssize_t scull_b_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
        struct scull_buffer *dev = filp->private_data;
        int result;

	PDEBUG("scull_b_write locking sem\n");
        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;

        /* Make sure there's space to write */
        result = scull_getwritespace(dev, filp);
        if (result == 1) // there are no readers, so exit with 0
		return 0;
	else if (result)
                return result; /* scull_getwritespace called up(&dev->sem) */

        /* ok, space is there, accept something */
        count = min(count, (size_t)spacefree(dev));
        if (dev->wp >= dev->rp)
                count = min(count, (size_t)(dev->end - dev->wp)); /* to end-of-buf */
        else /* the write pointer has wrapped, fill up to rp-1 */
                count = min(count, (size_t)(dev->rp - dev->wp - 1));
        PDEBUG("Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);
        if (copy_from_user(dev->wp, buf, count)) {
		PDEBUG("scull_b_write unlocking sem\n");
                up (&dev->sem);
                return -EFAULT;
        }
        dev->wp += count;
        if (dev->wp == dev->end)
                dev->wp = dev->buffer; /* wrapped */
        PDEBUG("\" (scull_b_write) dev->wp:%p    dev->rp:%p\" \n",dev->wp,dev->rp);
        PDEBUG("scull_b_write unlocking sem\n");
	up(&dev->sem);

        /* finally, awake any reader */
	PDEBUG("Waking up readers\n");
        wake_up_all(&dev->inq);  /* blocked in read() and select() */

        //PDEBUG("\"%s\" did write %li bytes\n",current->comm, (long)count);
        PDEBUG("Process %d did write %li bytes\n", current->pid, (long)count);
	return count;
}


/* scull_b_poll:
 * Parameters: file struct pointer
 *             poll_table pointer
 *
 * - Get the scull_buffer struct pointer
 * - Lock semaphore
 * - Call poll_wait on both inq and outq
 *   This call will add the device to the list of those that can 
 *   wake the process up. Once the kernel blocks the process, 
 *   we know that the producer or consumer can wake it up again. 
 * - If the rp, wp pointers are not equal (buffer is nonempty)
 *   then we have a readable situation - the consumer can be awakened
 * - If there is space free in the buffer, we have a writable situation - 
 *   the producer can be awakened
 * - Unlock semaphore
 * - Return mask value (readable/writable)
 */
static unsigned int scull_b_poll(struct file *filp, poll_table *wait)
{
        struct scull_buffer *dev = filp->private_data;
        unsigned int mask = 0;

        /*
         * The buffer is circular; it is considered full
         * if "wp" is right behind "rp" and empty if the
         * two are equal.
         */
	PDEBUG("scull_b_poll locking sem\n");
        down_interruptible(&dev->sem);
        poll_wait(filp, &dev->inq,  wait);
        poll_wait(filp, &dev->outq, wait);
        if (dev->rp != dev->wp)
                mask |= POLLIN | POLLRDNORM;    /* readable */
        if (spacefree(dev))
                mask |= POLLOUT | POLLWRNORM;   /* writable */
        PDEBUG("scull_b_poll unlocking sem\n");
	up(&dev->sem);
        return mask;
}

/* File operations:
 * Will redirect system calls to the appropriate
 * functions defined for this device
 *
 * We need owner, read, write, poll, open, release
 *
 */
struct file_operations scull_buffer_fops = {
        .owner =        THIS_MODULE,
        .read =         scull_b_read,
        .write =        scull_b_write,
        .poll =         scull_b_poll,
        .open =         scull_b_open,
        .release =      scull_b_release,
};

/* scull_b_setup_entry:
 * Parameters: scull_buffer struct pointer
 *             integer index
 *
 * Set up cdev entry
 * - Initialize cdev
 * - Set owner as THIS MODULE
 * - Add the device number to this cdev device
 * - Fail gracefully if needed 
 *
 */
static void scull_b_setup_cdev(struct scull_buffer *dev, int index)
{
        // Changed this from devno = scull_b_devno
	int err, devno = MKDEV(scull_major, scull_minor + index);
    
        cdev_init(&dev->cdev, &scull_buffer_fops);
        dev->cdev.owner = THIS_MODULE;
	dev->buffersize = n_items * sizeof(item_t);
        err = cdev_add (&dev->cdev, devno, 1);
        /* Fail gracefully if need be */
        if (err)
                printk(KERN_NOTICE "Error %d adding scullpipe%d", err, index);
}


/* scull_b_init:
 * Parameter: dev_t the first device number (major/minor)
 *
 * - Register the device region (check return value for error)
 * - Allocate memory for all devices
 * - For each device:
 *     - Initialize wait queues (inq, outq)
 *     - Initialize semaphore
 *     - Setup cdev device
 * - Return the number of devices set up
 *
 */
int scull_b_init(void /*dev_t firstdev*/)
{
        int result;

	KDEBUG("Initializing device\n");


        if (scull_major) {
                scull_b_devno = MKDEV(scull_major, scull_minor);
                result = register_chrdev_region(scull_b_devno, scull_b_nr_devs, "scullbuffer");
        } else {
                result = alloc_chrdev_region(&scull_b_devno, scull_minor, scull_b_nr_devs,
                                "scullbuffer");
                scull_major = MAJOR(scull_b_devno);
		scull_minor = MINOR(scull_b_devno);
        }




        if (result < 0) {
                printk(KERN_NOTICE "Unable to get scullp region, error %d\n", result);
                // Return error code here
		return result;
        }
        //scull_b_devno = firstdev;
        scull_b_devices = kmalloc(SCULL_B_NR_DEVS * sizeof(struct scull_buffer), GFP_KERNEL);
        if (scull_b_devices == NULL) {
                unregister_chrdev_region(scull_b_devno, SCULL_B_NR_DEVS);
                return -ENOMEM; // No memory error?
        }
        memset(scull_b_devices, 0, SCULL_B_NR_DEVS * sizeof(struct scull_buffer));
        
        init_waitqueue_head(&(scull_b_devices->inq));
        init_waitqueue_head(&(scull_b_devices->outq));
        init_MUTEX(&scull_b_devices->sem);
        scull_b_setup_cdev(scull_b_devices, 1);

        //return SCULL_B_NR_DEVS;
	//Return 0 if no errors
	KDEBUG("Finished initializing with no errors!\n");
	return 0;
}


/* scull_b_cleanup:
 * Parameter: None (void)
 * 
 * This is called by cleanup_module or on failure.
 * - If no devices were initialized, there is nothing to release
 * - Else for each device created:
 *      - Delete the device 
 *      - Free the memory for each device
 * - Free the memory for the devices pointer
 * - Unregister device region
 * - Set device pointer to null
 */
void scull_b_cleanup(void)
{
	KDEBUG("cleaning up the device\n");
        if (!scull_b_devices)
                return; /* nothing else to release */

        
        cdev_del(&scull_b_devices->cdev);
        kfree(scull_b_devices->buffer);
        
        kfree(scull_b_devices);
        unregister_chrdev_region(scull_b_devno, SCULL_B_NR_DEVS);
        scull_b_devices = NULL; /* pedantic */
}

// Init and exit calls 
module_init(scull_b_init);
module_exit(scull_b_cleanup);
