/* scull.c
 *
 * Author: Jimmy Pan
 * Email: dspjmt@gmail.com
 * Date:
 * 
 * Algorithm
 * Input:
 * Output:
 * Functions:
 *
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/kdev_t.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/sched.h>

#include "scull.h"

#define SCULL_DEBUG

#undef DBPRINT
#ifdef SCULL_DEBUG
#define DBPRINT(fmt, ...) printk(KERN_ALERT "scull: " fmt, ##__VA_ARGS__)
#else
#define DBPRINT(fmt, ...)
#endif
#define DBPRINTS(s) DBPRINT(#s ": %s\n", s)
#define DBPRINTI(n) DBPRINT(#n " = %i\n", n)
#define DBPRINTP(p) DBPRINT(#p " = %p\n", p)

static struct scull scull;


static void trim_scull_dev(struct scull_dev *sd)
{
	int i;
	struct quantum *tmp1, *tmp2;
	tmp1 = &sd->head;
	for (i = 0; i < sd->q_cnt; i++) {
		if (!tmp1)
			return;
		tmp2 = tmp1->next;
		if (tmp1->data)
			kfree(tmp1->data);
		if (i)
			kfree(tmp1);
		tmp1 = tmp2;
	}
	sd->q_cnt = 0;
	sd->sum_len = 0;
	sd->head.data = NULL;
	sd->head.len = 0;
	sd->head.next = NULL;
}
		
static int scull_open(struct inode *inode, struct file *filp)
{
	int tmp1;
	struct scull_dev *tmp2;
	tmp1 = MINOR(inode->i_rdev);
	if (tmp1 >= DEV_COUNT)
		return -EFAULT;
	tmp2 = scull.devs + tmp1;
	filp->private_data = tmp2;
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&tmp2->m))
			return -ERESTARTSYS;
		trim_scull_dev(tmp2);
		up(&tmp2->m);
	}
	return 0;
}

static int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

struct quantum *follow_quantums(struct quantum *head, int cnt)
{
	int i;
	for (i = 0; i < cnt && head; i++)
		head = head->next;
	return head;
}

static int scull_read(struct file *filp, char __user *buf, size_t size, 
							loff_t *f_off)
{
	struct scull_dev *tmp1;
	int tmp2, tmp3, tmp6, tmp7, tmp8, result;
	struct quantum *tmp4;
	void *tmp5;
	static int tmp = 0;
	tmp1 = filp->private_data;
	tmp2 = *f_off / QUANTUM_SIZE;
	tmp3 = *f_off % QUANTUM_SIZE;
	DBPRINTI(size);
	DBPRINTI(*f_off);
	if (down_interruptible(&tmp1->m))
		return -ERESTARTSYS;
	while (*f_off >= tmp1->sum_len) {
		DEFINE_WAIT(tmp9);
		DBPRINT("Wait queue");
		up(&tmp1->m);
		if (filp->f_flags == O_NONBLOCK)
			return -EAGAIN;
		prepare_to_wait_exclusive(&tmp1->q, &tmp9, TASK_INTERRUPTIBLE);
		DBPRINT("waiting");
		if (*f_off >= tmp1->sum_len)
			schedule();
		finish_wait(&tmp1->q, &tmp9);
		if (signal_pending(current))
			return -ERESTARTSYS;
		if (down_interruptible(&tmp1->m))
			return -ERESTARTSYS;
	}
/*
*/
	tmp4 = follow_quantums(&tmp1->head, tmp2);
	tmp5 = tmp4->data;
	tmp5 += tmp3;
	tmp6 = tmp4->len - tmp3;
	tmp8 = size > tmp6 ? tmp6 : size;
	tmp7 = copy_to_user(buf, tmp5, tmp8);
	result = tmp6 - tmp7;
	*f_off += result;
	DBPRINTI(tmp8);
	up(&tmp1->m);
	return result;
}

struct quantum *add_quantums(struct scull_dev *sd, int cnt)
{
	int i;
	struct quantum *tmp1;
	struct quantum *tmp3;
	void *tmp2;
	tmp1 = follow_quantums(&sd->head, sd->q_cnt - 1);
	for (i = 0; i < cnt; i++) {
		tmp2 = kmalloc(QUANTUM_SIZE, GFP_KERNEL);
		if (!tmp2)
			return NULL;
		memset(tmp2, 0, QUANTUM_SIZE);
		if (!sd->q_cnt)
			tmp1->data = tmp2;
		else {
			tmp3 = kmalloc(sizeof *tmp3, GFP_KERNEL);
			if (!tmp3) {
				kfree(tmp2);
				return NULL;
			}
			tmp1->next = tmp3;
			tmp3->data = tmp2;
			tmp3->len = 0;
			tmp3->next = NULL;
			tmp1 = tmp3;
		}
		sd->q_cnt++;
	}
	return tmp1;
}

static int scull_write(struct file *filp, const char __user *buf, size_t size,
							loff_t *f_off)
{
	struct scull_dev *tmp1;
	int tmp2, tmp3, tmp5, tmp7, tmp8, result;
	struct quantum *tmp4;
	void *tmp6;
	if (!size)
		return 0;
	tmp1 = filp->private_data;
	tmp2 = *f_off / QUANTUM_SIZE;
	tmp3 = *f_off % QUANTUM_SIZE;
	if (down_interruptible(&tmp1->m))
		return -ERESTARTSYS;
	tmp5 = tmp2 + 1 - tmp1->q_cnt;
	if (tmp5 < 0) {
		tmp4 = follow_quantums(&tmp1->head, tmp2);
	} else {
		tmp4 = add_quantums(tmp1, tmp5);
	}
	if (!tmp4) {
		result = -ENOMEM;
		goto out;
	}
	tmp6 = tmp4->data;
	tmp6 += tmp3;
	tmp7 = size < QUANTUM_SIZE - tmp3 ? size : QUANTUM_SIZE - tmp3;
	tmp8 = copy_from_user(tmp6, buf, tmp7);
	wake_up_interruptible(&tmp1->q);
	result = tmp7 - tmp8;
	tmp1->sum_len += result;
	tmp4->len += result;
	*f_off += result;
out:
	up(&tmp1->m);
	return result;
}

static int scull_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
							unsigned long arg)
{
	struct scull_dev *tmp1;
	struct ioctl_isr_para tmp2;
	struct quantum *tmp3;
	int tmp4;
	tmp1 = filp->private_data;
	switch (cmd) {
	case IOCTL_TRIM:
		trim_scull_dev(tmp1);
		return 0;
	case IOCTL_SPEC_READ:
		if (copy_from_user(&tmp2, (void *)arg, sizeof tmp2))
			return -EFAULT;
		if (!access_ok(VERIFY_WRITE, tmp2.addr, tmp2.buf_size))
			return -EFAULT;
		tmp3 = follow_quantums(&tmp1->head, tmp2.num);
		if (!tmp3)
			return 0;
		tmp4 = copy_to_user(tmp2.addr, tmp3->data,
							tmp2.buf_size);
		if (tmp4)
			return -EFAULT;
		return 0;
	}
	return -EINVAL;
}

static const struct file_operations scull_file_ops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.release = scull_release,
	.read = scull_read,
	.write = scull_write,
	.ioctl = scull_ioctl,
};

static void *scull_seq_start(struct seq_file *s, loff_t *pos)
{
	struct scull_dev *tmp;
	if (*pos >= DEV_COUNT)
		return NULL;
	tmp = scull.devs + *pos;
	++*pos;
	return tmp;
}

static void *scull_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	struct scull_dev *tmp1;
	if (*pos >= DEV_COUNT)
		return NULL;
	tmp1 = scull.devs + *pos;
	++*pos;
	return tmp1;
}

static void scull_seq_stop(struct seq_file *s, void *v)
{
}

static int scull_seq_show(struct seq_file *s, void *v)
{
	struct scull_dev *tmp1;
	struct quantum *tmp2;
	int i;
	tmp1 = (struct scull_dev *)v;
	if (v == scull.devs) {
		seq_printf(s, "scull:\n\tmajor: %i\n", scull.major);
	}
	seq_printf(s,"scull[%i]:\n\tquantum count: %i\n", tmp1->minor, tmp1->q_cnt);
	seq_printf(s, "\tquantums:\n");
	tmp2 = &tmp1->head;
	for (i = 0; i < tmp1->q_cnt; i++) {
		seq_printf(s, "\tquantum addr: %p\tdata: %p\n", tmp2, tmp2->data);
		tmp2 = tmp2->next;
	}
	return 0;
}

static const struct seq_operations scull_seq_ops = {
	.start = scull_seq_start,
	.next = scull_seq_next,
	.stop = scull_seq_stop,
	.show = scull_seq_show,
};

static int scull_seq_file_open(struct inode *inode, struct file *filp)
{
	return seq_open(filp, &scull_seq_ops);
}

static const struct file_operations scull_seq_fops = {
	.owner = THIS_MODULE,
	.open = scull_seq_file_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static void scull_dev_init(struct scull_dev *sd, dev_t dev)
{
	sd->head.data = NULL;
	sd->head.len = 0;
	sd->head.next = NULL;
	sd->q_cnt = 0;
	sd->minor = MINOR(dev);
	sd->sum_len = 0;
	init_waitqueue_head(&sd->q);
	sema_init(&sd->m, 1);
	cdev_init(&sd->cdev, &scull_file_ops);
	cdev_add(&sd->cdev, dev, 1);
}

static int scull_init(void)
{
	int i;
	dev_t tmp1;
	struct scull_dev *tmp2;

	alloc_chrdev_region(&tmp1, 0, DEV_COUNT, "scull");
	scull.major = MAJOR(tmp1);
	for (i = 0; i < DEV_COUNT; i++) {
		tmp1 = MKDEV(scull.major, i);
		tmp2 = scull.devs + i;
		scull_dev_init(tmp2, tmp1);
	}
	proc_create("scull", 0, NULL, &scull_seq_fops);
	DBPRINT("new scull\n");
	return 0;
}

static void scull_exit(void)
{
	int i;
	struct scull_dev *tmp1;
	for (i = 0; i < DEV_COUNT; i++) {
		tmp1 = scull.devs + i;
		cdev_del(&tmp1->cdev);
		trim_scull_dev(tmp1);
	}
	remove_proc_entry("scull", NULL);
	unregister_chrdev_region(MKDEV(scull.major, 0), DEV_COUNT);
}

module_init(scull_init);
module_exit(scull_exit);
