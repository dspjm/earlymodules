/* scull.c
 *
 * Author: Jimmy Pan
 * Email: dspjmt@gmail.com
 * Date: Fri Dec  7 13:07:32 CST 2012
 * 
 * Algorithm
 * Input:
 * Output:
 * Functions:
 * 	write()
 * 	read()
 * 	init()
 * 	exit()
 *
 */

#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/string.h>

#include "scull.h"

#undef DEBUG_SCULL
#define DEBUG_SCULL

#undef DB_PRINT
#ifdef DEBUG_SCULL
#define DB_PRINT(fmt, args...) printk(KERN_ALERT "scull: " fmt, ##args)
#define DB_PRINTP(ptr) printk(KERN_ALERT "scull: " #ptr " = %p\n", ptr)
#define DB_PRINTI(n) DB_PRINT(#n " = %i\n", n)
#define DB_PRINTS(s) DB_PRINT("%s\n", s);
#else
#define DB_PRINT(fmt, args...)
#define DB_PRINTP(ptr)
#endif

struct scull scull;
int major;


void traverse_device(struct scull_dev *sd)
{
	int i;
	struct quantum *tmp = &sd->head;
	DB_PRINTS("traversing scull_dev:")
	for (i = 0; i < sd->q_cnt; i++) {
		DB_PRINTI(i);
		DB_PRINTP(tmp->data);
		DB_PRINTP(tmp->next);
		tmp = tmp->next;
	}
}

int scull_open(struct inode *inode, struct file *filp)
{
	int minor;
	minor = MINOR(inode->i_rdev);
	filp->private_data = &scull.dev[minor];
	return 0;
}

struct quantum *follow_quantum(struct quantum *q, int cnt)
{
	int i;
	for (i = 0; i < cnt; i++) {
		q = q->next;
		if (!q)
			return NULL;
	}
	return q;
}

ssize_t scull_read(struct file *fp, char __user *buf, size_t cnt, loff_t *f_off)
{
	int tmp1, tmp2;
	struct quantum *tmp3;
	struct scull_dev *tmp4 = fp->private_data;
	unsigned long tmp5;
	long tmp6;
	void *tmp7;
	ssize_t result;

	tmp1 = *f_off / QUANTUM_SIZE;
	tmp2 = *f_off % QUANTUM_SIZE;
	tmp3 = follow_quantum(&tmp4->head, tmp1);
	if (!tmp3)
		return 0;
	tmp5 = QUANTUM_SIZE - tmp2;
	tmp7 = tmp3->data;
	tmp6 = copy_to_user(buf, tmp7 + tmp2, tmp5);
	result = tmp5 - tmp6;
	*f_off += result;
	return result;
}

void truncate_quantum_datas(struct scull_dev *sd)
{
	struct quantum *tmp1, *tmp3;
	void *tmp2;
	int i;
	tmp1 = &sd->head;
	tmp2 = tmp1->data;
	if (tmp2)
		kfree(tmp2);
	tmp1 = tmp1->next;
	for (i = 1; i < sd->q_cnt; i++) {
		tmp3 = tmp1->next;
		tmp2 = tmp1->data;
		if (tmp2)
			kfree(tmp2);
		if (tmp1)
			kfree(tmp1);
		tmp1 = tmp3;
	}
	sd->q_cnt = 0;
}

struct quantum *quantum_alloc(void)
{
	struct quantum *tmp1;
	void *tmp2;
	tmp1 = kmalloc(sizeof *tmp1, GFP_KERNEL);
	if (!tmp1)
		return NULL;
	tmp2 = kmalloc(QUANTUM_SIZE, GFP_KERNEL);
	if (!tmp2) {
		kfree(tmp1);
		return NULL;
	}
	memset(tmp1, 0, sizeof *tmp1);
	memset(tmp2, 0, QUANTUM_SIZE);
	tmp1->data = tmp2;
	tmp1->next = NULL;
	return tmp1;
}

int add_quantums(struct scull_dev *sd, int cnt)
{
	struct quantum *tmp1, *tmp2;
	void *tmp3;
	int i;
	if (sd->q_cnt == 0 && cnt > 0) {
		tmp3 = kmalloc(QUANTUM_SIZE, GFP_KERNEL);
		if (!tmp3)
			return 1;
		memset(tmp3, 0, QUANTUM_SIZE);
		sd->head.data = tmp3;
		sd->head.next = NULL;
		sd->q_cnt = 1;
		cnt--;
	}
	tmp1 = follow_quantum(&sd->head, sd->q_cnt - 1);
	for (i = 0; i < cnt; i++) {
		tmp2 = quantum_alloc();
		if (!tmp2)
			return 1;
		tmp1->next = tmp2;
		sd->q_cnt++;
		tmp1 = tmp2;
	}
	return 0;
}
	

int write_in_old_space(struct scull_dev *sd, const char __user *buf, size_t cnt,
						int q_num, int q_off)
{
	struct quantum *tmp1;
	void *tmp2;
	int tmp3, tmp4, tmp5, result;
	tmp1 = follow_quantum(&sd->head, q_num);
	tmp2 = tmp1->data;
	if (!tmp2)
		return -ENOMEM;
	tmp3 = QUANTUM_SIZE - q_off;
	tmp4 = cnt > tmp3 ? tmp3 : cnt;
	tmp5 = copy_from_user(tmp2 + q_off, buf, tmp4);
	result = tmp4 - tmp5;
	return result;
}
	
ssize_t write_in_new_space(struct scull_dev *sd, const char __user* buf,
					size_t cnt, int q_num, int q_off)
{
	int tmp1;
	int result;

	tmp1 = q_num - (sd->q_cnt - 1);
	if (add_quantums(sd, tmp1))
		return -ENOMEM;
	result = write_in_old_space(sd, buf, cnt, q_num, q_off);
	return result;
}

ssize_t
scull_write(struct file *fp, const char __user *buf, size_t cnt, loff_t *f_off)
{
	int tmp1, tmp2;
	struct scull_dev *tmp3;
	ssize_t result;

	tmp1 = *f_off / QUANTUM_SIZE;
	tmp2 = *f_off % QUANTUM_SIZE;
	tmp3 = fp->private_data;
	if (tmp1 > tmp3->q_cnt - 1)
		result = write_in_new_space(tmp3, buf, cnt, tmp1, tmp2);
	else
		result = write_in_old_space(tmp3, buf, cnt, tmp1, tmp2);
	*f_off += result;
	return result;
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

void scull_dev_init(struct scull_dev *sd)
{
	cdev_init(&sd->cdev, &scull_fops);
	sd->head.data = NULL;
	sd->head.next = NULL;
	sd->q_cnt = 0;
}

int scull_init(void)
{
	dev_t dev_num;
	int i;
	struct scull_dev *tmp;

	DB_PRINTS("\nThis is a new scull");
	alloc_chrdev_region(&dev_num, 0, DEV_COUNT, DEV_NAME);
	major = MAJOR(dev_num);
	for (i = 0; i < DEV_COUNT; i++) {
		tmp = &scull.dev[i];
		dev_num = MKDEV(major, i);
		scull_dev_init(tmp);
		cdev_add(&tmp->cdev, dev_num, 1);
	}
	return 0;
}

void scull_exit(void)
{
	int i;
	for (i = 0; i < DEV_COUNT; i++) {
		cdev_del(&scull.dev[i].cdev);
		truncate_quantum_datas(&scull.dev[i]);
	}
	unregister_chrdev_region(MKDEV(major, 0), DEV_COUNT);
}

module_init(scull_init);
module_exit(scull_exit);
