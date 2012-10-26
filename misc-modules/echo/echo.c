#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h> //MAJOR, MINOR
#include <linux/fs.h> //register_chrdev_region, file_operations
#include <linux/moduleparam.h>
#include <linux/kernel.h> //container_of
#include <linux/slab.h> //kmalloc
#include <linux/cdev.h> //struct cdev
#include <linux/version.h>
#include <linux/uaccess.h> //copy_from/to_user()
#include <linux/errno.h> //error code
#include "echo.h"

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("amit");

int nr_major;
module_param(nr_major, int, S_IRUGO);
MODULE_PARM_DESC(nr_major, "major number");

int nr_minor;
char *chrdev_name = "echo";

static dev_t device;
static int echo_dev_count = 1;
struct echo_cdev *echo_dev = NULL;

static struct file_operations echo_fs_ops = {
	.open = echo_open,
	.release = echo_release,
	.read = echo_read,
	.write = echo_write,
	.owner = THIS_MODULE,
};

int echo_open(struct inode *inode, struct file *filp)
{
	struct echo_cdev *dev;
	pr_debug("%s: f_flags: 0x%x\n",__FUNCTION__,filp->f_flags);
	//container_of(pointer, container_type, container_field);
	dev = container_of(inode->i_cdev, struct echo_cdev, cdev);
	filp->private_data = dev;
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		//trim the device size to 0
		dev->size = 0;
	}
	return 0;
}

int echo_release(struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t echo_read(struct file *filp, char __user *ubuff, size_t count, loff_t *poffset)
{
	struct echo_cdev *dev = filp->private_data;
	pr_debug("%s: f_flags: 0x%x\n",__FUNCTION__,filp->f_flags);
	
	//user trying to access an offset which is beyond the end of file 
	if (*poffset >= dev->size)
		return 0;

	//user trying to access more than eof, return bytes read till the eof
	if (*poffset + count >= dev->size) 
		//count = dev->size - *poffset;
		count = dev->size;
	//kspace --> uspace
	if (copy_to_user(ubuff, (dev->data + *poffset), count) < 0) {
		cdev_del(&echo_dev->cdev);
		unregister_chrdev_region(device, count);
		return -EFAULT;
	}
	//update the offset
	*poffset += count;
	return count;
}
//count is the size of requested data transfer
ssize_t echo_write(struct file *filp, const char __user *ubuff, size_t count, loff_t *poffset)
{
	int ret;
	struct echo_cdev *dev = filp->private_data;
	pr_debug("%s: f_flags: 0x%x\n",__FUNCTION__,filp->f_flags);
	dev->data = (char *)kmalloc(count, GFP_KERNEL);
	if (!(dev->data)) {
		printk(KERN_EMERG "%s: Not enough kernel buffers\n",__FUNCTION__);
		return 0;
	}
	dev->size = count;
	//uspace --> kspace
	if (copy_from_user(dev->data, ubuff, count) < 0) {
		cdev_del(&echo_dev->cdev);
		kfree(echo_dev);
		unregister_chrdev_region(device, count);
		return -EINVAL;
	}
	*poffset += count;
	ret = count;

	if (dev->size < *poffset)
		dev->size = *poffset;

	return ret;
}

static int __init echo_init(void)
{
	int ret;
	printk(KERN_EMERG "entering %s\n",__FUNCTION__);
	//let the user provide the major number.
	if (nr_major) { 
		device = MKDEV(nr_major, nr_minor);
		if ((ret = register_chrdev_region(device, echo_dev_count, chrdev_name)) < 0) {
			pr_debug("%s: failed to register %s\n",__FUNCTION__,
					chrdev_name);
			return ret;
		}
	} else {
		ret = alloc_chrdev_region(&device, 0, echo_dev_count, chrdev_name);
		if (ret < 0) {
			pr_debug("%s: failed to register %s\n",__FUNCTION__,
					chrdev_name);
			return ret;
		}
	}
	nr_major = MAJOR(device);
	nr_minor = MINOR(device);
	//print the major and minor numbers
	pr_debug("%s: major/minor:: %d/%d\n",__FUNCTION__,
			nr_major, nr_minor);
	echo_dev = (struct echo_cdev *)kmalloc(sizeof(struct echo_cdev), GFP_KERNEL);
	if (!echo_dev) {
		printk(KERN_EMERG "Not enough memory\n");
		unregister_chrdev_region(device, echo_dev_count);
		return -ENOMEM;
	}
	memset(echo_dev, 0, sizeof(struct echo_cdev));
	echo_dev->cdev.owner = THIS_MODULE;
	echo_dev->cdev.ops = &echo_fs_ops;
	cdev_init(&echo_dev->cdev, &echo_fs_ops);
	device = MKDEV(nr_major, nr_minor);
	//tell the kernel about this char device
	//telling the VFS layer to associate echo driver's fs operation for file r/w etc.
	ret = cdev_add(&echo_dev->cdev, device, 1);
	if (ret) {
		kfree(echo_dev);
		unregister_chrdev_region(device, echo_dev_count);
		return ret;
	}
	return 0;
}

static void __exit echo_exit(void)
{
	printk(KERN_EMERG "entering %s\n",__FUNCTION__);
	if (echo_dev->data) {
		printk("Inside %s: kfree()\n",__FUNCTION__);
		kfree(echo_dev->data);
	}
	cdev_del(&echo_dev->cdev);
	kfree(echo_dev);
	unregister_chrdev_region(device, echo_dev_count);
}

module_init(echo_init);
module_exit(echo_exit);
