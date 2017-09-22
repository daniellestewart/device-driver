/* CSCI 5103 Fall 2016
 * Assignment # 7
 * name: Alexander Cina, Danielle Stewart
 * student id: 4515522, 4339650
 * x500 id: cinax020, dkstewar
 * CSELABS machine: csel-x07-umh, csel-x34-umh
 */
 
 /*
 * scullbuffer.h -- definitions for the char module
 * Modified for scullbuffer by Alexander Cina and Danielle Stewart
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
 * $Id: scull.h,v 1.15 2004/11/04 17:51:18 rubini Exp $
 */
 
#ifndef _SCULLBUFFER_H_
#define _SCULLBUFFER_H_
 
 
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>       /* printk(), min() */
#include <linux/slab.h>         /* kmalloc() */
#include <linux/fs.h>           /* everything... */
#include <linux/proc_fs.h>
#include <linux/errno.h>        /* error codes */
#include <linux/types.h>        /* size_t */
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */




/*
 * Macros to help debugging
 */

#undef PDEBUG             /* undef it, just in case */
#ifdef SCULL_DEBUG
#  ifdef __KERNEL__
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "scull: " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

#ifndef SCULL_MAJOR
#define SCULL_MAJOR 0   /* dynamic major by default */
#endif

#ifndef SCULL_B_NR_DEVS
#define SCULL_B_NR_DEVS 1  /* scullbuffer device */
#endif

#ifndef NITEMS
#define NITEMS 50  /* number of items in buffer */
#endif

/*
 * The pipe device is a simple circular buffer. Here its default size
 */
#ifndef SCULL_B_BUFFER
#define SCULL_B_BUFFER 4000
#endif


/*
 * Split minors in two parts
 */
#define TYPE(minor)     (((minor) >> 4) & 0xf)  /* high nibble */
#define NUM(minor)      ((minor) & 0xf)         /* low  nibble */

/*
 * The different configurable parameters in scullbuffer.c
 */
//extern int scull_b_buffer;     
//extern int nitems;
//extern static int scull_b_nr_devs;


/*
 * Prototypes for shared functions (in scullbuffer.c)
 */
 
 /* init, cleanup */ 
int     scull_b_init(void/*dev_t dev*/);
void    scull_b_cleanup(void);



#endif /* _SCULLBUFFER_H_ */
