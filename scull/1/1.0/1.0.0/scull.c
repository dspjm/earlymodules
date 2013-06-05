/* main.c
 * a test module
 * Author: Jimmy Pan
 * Email: dspjmt@gmail.com
 * Date: Sat Sep 22 10:46:29 CST 2012
 * 
 * Algorithm
 * Input:
 * Output:
 * Functions:
 *
 */

#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>

#include "scull.h"

#define DEV_COUNT 4
#define DEV_NAME "scull"
#define DEV_SIZE 1024

/*#define SCULL_DEBUG*/

#undef DEB_PRINT
#ifdef SCULL_DEBUG
#define DEB_PRINT(fmt, args...) printk(KERN_ALERT "scull: " fmt, ## args)
#else
#define DEB_PRINT(fmt, args...)
#endif



dev_t dev_num;
int major;
struct scull *scull;

int scull_open(struct inode *inode, struct file *filp)
{
	filp->private_data = scull->scull_dev + MINOR(inode->i_rdev);
	return 0;
}

ssize_t
scull_read(struct file *filp, char __user *dest, size_t size, loff_t *offset)
{
	int result;
	struct scull_dev *sd;
	sd = filp->private_data;
	if (*offset + size > DEV_SIZE)
		size = DEV_SIZE - *offset;
	result = copy_to_user(dest, sd->data + *offset, size);
	result = size - result;
	*offset += result;
	DEB_PRINT("sd->data: %s, off: %i\n", (char *)sd->data, *offset);
	DEB_PRINT("dest: %p, dest data: %s\n", dest, dest);
	DEB_PRINT("read %i bytes", result);
	return result;
error:
	return -ENOMEM;
}

ssize_t
scull_write(struct file *filp, const char __user *src, size_t size, loff_t *off)
{
	int result;
	struct scull_dev *sd;
	sd = filp->private_data;
	DEB_PRINT("filp: %p, src: %p, size: %i, off: %i\n", filp, src,
								size, *off);
	DEB_PRINT("scull_dev *sd: %p, sd->data: %p\n", sd, sd->data);
	DEB_PRINT("sizeof sd->data: %i\n", sizeof *sd->data);
	if (*off + size > DEV_SIZE)
		goto error;
	result = copy_from_user(sd->data + *off, src, size);
	result = size - result;
	*off += result;
	DEB_PRINT("written %i bytes", result);
	DEB_PRINT("%s", sd->data);
	schedule();
	return result;
error:
	return -ENOMEM;
}

int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct file_operations scull_fops = {
		.owner = THIS_MODULE,
		.open = scull_open,
		.read = scull_read,
		.write = scull_write,
		.release = scull_release,
};

int scull_read_proc(char *page, char **start, off_t offset, int count,
						int *eof, void *data)
{
	struct scull_dev *sd;
	int i;
	int cnt = 0;
	cnt += sprintf(page + cnt, "scull:\ndevice major: %i\ndevice amount:"
			"%i\ndevice data block size: %i\nDevices:\n", 
			scull->major,scull->dev_count, scull->dev_size);
	sd = scull->scull_dev;
	for (i = 0; i < DEV_COUNT; sd++, i++) {
		cnt += sprintf(page + cnt, " minor: %i\n data:%s\n",
					sd->minor, (char *)sd->data);
	}
	*eof = 1;
	return cnt;
}

int init(void)
{
	int i;
	struct cdev *cdev;
	dev_t tmp;
	void *tmp2;
	if (alloc_chrdev_region(&dev_num, 0, DEV_COUNT, DEV_NAME) != 0)
		goto error;
	major = MAJOR(dev_num);
	scull = kmalloc(sizeof *scull, GFP_KERNEL);
/*memset please*/
	if (!scull)
		goto error;
	scull->major = major;
	scull->dev_count = DEV_COUNT;
	scull->dev_size = DEV_SIZE;
	scull->scull_dev = kmalloc(sizeof(struct scull_dev) * DEV_COUNT,
								GFP_KERNEL);
	memset(scull->scull_dev, 0, sizeof(struct scull_dev) * DEV_COUNT);
	if (!scull->scull_dev)
		goto error;
	for (i = 0; i < DEV_COUNT; i++) {
		tmp = MKDEV(major, i);
		cdev = cdev_alloc();
		cdev_init(cdev, &scull_fops);
		if (cdev_add(cdev, tmp, 1) != 0)
			goto error;
		tmp2 = kmalloc(DEV_SIZE, GFP_KERNEL);
		DEB_PRINT("%i\n", sizeof *tmp2);
		if (!tmp2)
			goto error;
		memset(tmp2, 0, DEV_SIZE);
		scull->scull_dev[i].cdev = cdev;
		scull->scull_dev[i].minor = i;
		scull->scull_dev[i].data = tmp2;
	}
	create_proc_read_entry("scull", 0, NULL, scull_read_proc, NULL);
	DEB_PRINT("hello, scull");
	return 0;
error:
	printk(KERN_ALERT "error");
	return 1;
}

void exit(void)
{
	int i;
	remove_proc_entry("scull", NULL);
	for (i = 0; i < DEV_COUNT; i++) {
		kfree(scull->scull_dev[i].data);
		cdev_del(scull->scull_dev[i].cdev);
		kfree(scull->scull_dev[i].cdev);
	}
	kfree(scull->scull_dev);
	kfree(scull);
	unregister_chrdev_region(MKDEV(major, 0), DEV_COUNT);
	DEB_PRINT("good bye, scull!");
}

module_init(init);
module_exit(exit);
