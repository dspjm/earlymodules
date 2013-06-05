/* main.c
 * a test module
 * Author: Jimmy Pan
 * Email: dspjmt@gmail.com
 * Date: Sun Dec  2 09:03:35 CST 2012
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
#define DATA_SET_SIZE 1024

#define SCULL_DEBUG

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

void *follow_set(int s_pos, struct scull_dev *sd)
{
	int i;
	struct data_set *ds;
	struct list_head *list;
	ds = &sd->data_set;
	
	list = ds->list.next;
	for (i = 0; i < s_pos; i++)
		list = list->next; 
	ds = list_entry(list, struct data_set, list);
	DEB_PRINT("set_num = %i\n followed", ds->set_num);
	return ds;
}

ssize_t
scull_read(struct file *filp, char __user *dest, size_t size, loff_t *f_off)
{
	int transfered;
	struct scull_dev *sd;
	int set_off, quantum_off, max_size;
	void *src;
	sd = filp->private_data;
	/*determine offsets and maximum size*/
	set_off = *f_off / DATA_SET_SIZE;
	quantum_off = *f_off % DATA_SET_SIZE;
	max_size = DATA_SET_SIZE - quantum_off;
	/*determine size*/
	size = size > max_size ? max_size : size;

	src = follow_set(set_off, sd);
	src += quantum_off;
	transfered = size - copy_to_user(dest, src, size);
	*f_off += transfered;
	DEB_PRINT("%i bytes read\n", transfered);
	return transfered;
}

struct data_set *alloc_and_add_data_set(struct scull_dev *sd)
{
	struct data_set *ds;
	ds = kmalloc(sizeof *ds, GFP_KERNEL);
	if (!ds)
		return NULL;
	ds->data = kmalloc(DATA_SET_SIZE, GFP_KERNEL);
	if (!ds->data) {
		kfree(ds);
		return NULL;
	}
	memset(ds->data, 0, DATA_SET_SIZE);
	list_add_tail(&ds->list, &sd->data_set.list);
	ds->set_num = sd->set_cnt;
	sd->set_cnt++;
	DEB_PRINT("ds = %p, ds->data = %p, set_num = %i added\n",ds, ds->data, ds->set_num);
	return ds;
}

void truncate_data_set(struct scull_dev *sd)
{
	struct data_set *p, *np;
	list_for_each_entry_safe(p, np, &sd->data_set.list, list) {
		if (p->data)
			kfree(p->data);
		if (p)
			kfree(p);
	}
}
	

ssize_t
scull_write(struct file *filp, const char __user *src, size_t size, loff_t *f_off)
{
	int written;
	struct scull_dev *sd;
	struct data_set *ds;
	int set_off, quantum_off;
	size_t max_size;
	void * dest;
	sd = filp->private_data;
/*	truncate_data_set(sd);*/
	/*determine set_off and quantum_off*/
	set_off = *f_off / DATA_SET_SIZE;
	quantum_off = *f_off % DATA_SET_SIZE;
	DEB_PRINT("set_off %i, quantum_off %i", set_off, quantum_off);
	/*determine whether we should allocate a page and set ds*/
	if (set_off < sd->set_cnt) {
		ds = follow_set(set_off, sd);
		DEB_PRINT("when writing, set exist");
	}
	else {
		ds = alloc_and_add_data_set(sd);
		DEB_PRINT("when writing, set allocated ds = %p", ds);
	}
	if (!ds)
		goto error;
	/*determine copy size*/
	max_size = DATA_SET_SIZE - quantum_off;
	size = size > max_size ? max_size : size;

	dest = ds->data + quantum_off;
	written = copy_from_user(dest, src, size);
	written = size - written;
	*f_off += written;
	DEB_PRINT("%i bytes written\n", written);
	return written;
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
	struct data_set *ds;
	cnt += sprintf(page + cnt, "scull:\ndevice major: %i\ndevice amount:"
			"%i\ndevice data block size: %i\nDevices:\n", 
			scull->major,scull->dev_count, scull->dev_size);
	sd = scull->scull_dev;
		
	for (i = 0; i < DEV_COUNT; sd++, i++) {
		int set_num = 0;
		cnt += sprintf(page + cnt, " minor: %i\n data:\n", sd->minor);
		ds = &sd->data_set;
		list_for_each_entry(ds, &sd->data_set.list, list) {
			cnt += sprintf(page + cnt, "set %d\nds: %p\nds->data:%p\n%s",
				set_num, ds, ds->data, (char *)ds->data);
		}
	}
	*eof = 1;
	return cnt;
}

int scull_stat_read_proc(char *page, char **start, off_t off, int count,
						int *eof, void *data)
{
	struct scull_dev *sd;
	int i;
	static int cnt = 0;
	struct data_set *ds, *tmp;
	sd = scull->scull_dev;
	tmp = &sd->data_set; 
	DEB_PRINT("ds = %p, set_num = %d, ds->data = %p %i\n", tmp, tmp->set_num, tmp->data, cnt);
	for (i = 0; i < DEV_COUNT; sd++, i++) {
		sd = scull->scull_dev + i;
		list_for_each_entry(ds, &tmp->list, list) {
			DEB_PRINT("ds = %p, set_num = %d\n", ds, ds->set_num);
		}
	}
	cnt++;
	*eof = 1;
	return 0;
}

int init(void)
{
	int i;
	struct cdev *cdev;
	dev_t tmp;
	struct scull_dev *tmp3;
	if (alloc_chrdev_region(&dev_num, 0, DEV_COUNT, DEV_NAME) != 0)
		goto error;
	major = MAJOR(dev_num);
	scull = kmalloc(sizeof *scull, GFP_KERNEL);
/*memset please*/
	if (!scull)
		goto error;
	scull->major = major;
	scull->dev_count = DEV_COUNT;
	scull->dev_size = DATA_SET_SIZE;
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
		tmp3 = &scull->scull_dev[i];
		tmp3->cdev = cdev;
		tmp3->minor = i;
		init_MUTEX(&tmp3->sem);
		INIT_LIST_HEAD(&tmp3->data_set.list);
		DEB_PRINT("list_prev = %p, list_next = %p", tmp3->data_set.list.prev, tmp3->data_set.list.next);
		tmp3->data_set.set_num = 0;
		tmp3->data_set.data = NULL;
		tmp3->set_cnt = 1;
	}
	create_proc_read_entry("scull", 0, NULL, scull_read_proc, NULL);
	create_proc_read_entry("scullstat", 0, NULL, scull_stat_read_proc, NULL);
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
	remove_proc_entry("scullstat", NULL);
	for (i = 0; i < DEV_COUNT; i++) {
		truncate_data_set(&scull->scull_dev[i]);
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
