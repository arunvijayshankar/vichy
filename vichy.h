/*
 * vichy.h - local definitions for vichy char module
 *
 * This source code is a minimal version of the 
 * Simple Character Utility for Loading Localities (scull) char 
 * module, which comes from the book "Linux Device Drivers" 
 * by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.
 *
 */
#ifndef _VICHY_H_
#define _VICHY_H_

#ifndef VICHY_MAJOR
#define VICHY_MAJOR 0 /* Dynamic majob by default */
#endif

#ifndef VICHY_NR_DEVS
#define VICHY_NR_DEVS 2 /* vichy0 to vichy1 */
#endif

/*
 * The base device is a variable length memory region
 * A linked list of indirect blocks is used
 *
 * "vichy_dev->data points to a memory area of 
 * VICHY_QUANTUM bytes" 
 */
#ifndef VICHY_QUANTUM
#define VICHY_QUANTUM 4000
#endif


/* Representation of data in a device */
struct vichy_qdata {
	void *data;
	struct vichy_qdata *next;
};

struct vichy_dev {
	struct vichy_qdata *data; /* Pointer to first quantum data block */
	int quantum; /* Current quantum size */
	unsigned long size; /* amount of data stored */
	struct cdev cdev;
};

/* Configurable params */
extern int vichy_major;
extern int vichy_nr_devs;
extern int vichy_quantum;

/* Prototypes */
int vichy_trim(struct vichy_dev *dev);


#endif /* _VICHY_H_ */
