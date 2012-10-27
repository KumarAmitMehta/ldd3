#ifndef __echo_H
#define __echo_H

ssize_t echo_read(struct file *, char __user *, size_t, loff_t *);
ssize_t echo_write(struct file *, const char __user *, size_t, loff_t *);
int echo_open(struct inode *, struct file *);
int echo_release(struct inode *, struct file *);

struct echo_cdev {
	char *data;
	unsigned long size; //amount of data stored
	struct rw_semaphore sem;
	struct cdev cdev;
};
#endif //__echo_H
