/* .c
 *
 * Author: Jimmy Pan
 * Email: dspjmt@gmail.com
 * Date:
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
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>

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

void truncate_quantums(struct scull_dev *sd)
{
	struct quantum *tmp1, *tmp3;
	void *tmp2;
	int i;
	tmp1 = &sd->head;
	tmp2 = tmp1->data; if (tmp2)
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
	sd->head.next = NULL;
	sd->q_cnt = 0;
}

int scull_open(struct inode *inode, struct file *filp)
{
	int minor;
	struct scull_dev *tmp1;
	minor = MINOR(inode->i_rdev);
	tmp1 = &scull.dev[minor];
	filp->private_data = tmp1;
	if (down_interruptible(&tmp1->sem))
		return -ERESTARTSYS;
	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		truncate_quantums(&scull.dev[minor]);
	}
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
	struct scull_dev *tmp = filp->private_data;
	up(&tmp->sem);
	return 0;
}

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.open = scull_open,
	.read = scull_read,
	.write = scull_write,
	.release = scull_release,
};

void scull_dev_init(struct scull_dev *sd, int minor)
{
	cdev_init(&sd->cdev, &scull_fops);
	sd->head.data = NULL;
	sd->head.next = NULL;
	sd->q_cnt = 0;
	sd->minor = minor;
	init_MUTEX(&sd->sem);
}

int scull_read_proc(char *buf, char **start, off_t off, int count, int *eof,
								void *data)
{
	int tmp1 = 0;
	int i;
	struct scull_dev *tmp2;
	tmp1 += sprintf(buf + tmp1, "scull:\n");
	tmp1 += sprintf(buf + tmp1, "\tquantum size: %i\n", QUANTUM_SIZE);
	for (i = 0; i < DEV_COUNT; i++) {
		tmp2 = &scull.dev[i];
		tmp1 += sprintf(buf + tmp1, "dev[%i]:\n", i);
		tmp1 += sprintf(buf + tmp1, "\tq_cnt = %i\n", tmp2->q_cnt);
	}
	*eof = 1;
	return tmp1;
}

void *scull_seq_start(struct seq_file *sf, loff_t *pos)
{
	if (*pos >= DEV_COUNT)
		return NULL;
	return &scull.dev[*pos];
}

void *scull_seq_next(struct seq_file *sf, void *v, loff_t *pos)
{
	if (++*pos >= DEV_COUNT)
		return NULL;
	return &scull.dev[*pos];
}

void scull_seq_stop(struct seq_file *sf, void *v)
{
}

int scull_seq_show(struct seq_file *sf, void *v)
{
	struct scull_dev *tmp1 = (struct scull_dev *)v;
	struct quantum *tmp2;
	int i;
	seq_printf(sf, "scull_dev[%i]:\n", tmp1->minor);
	seq_printf(sf, "\tq_cnt = %i\n", tmp1->q_cnt);
	seq_printf(sf, "\tquantums:\n");
	tmp2 = &tmp1->head;
	for (i = 0; i < tmp1->q_cnt; i++) {
		seq_printf(sf, "\taddr: %p, data: %p\n", tmp2, tmp2->data);
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

/*Take notice the following function cannot replace seq_read, or an error would
 * occur*/
/*static int scull_req_file_read(struct file *fp, char __user *buf, size_t cnt,
							loff_t *f_off)
{
	struct seq_file *tmp1 = fp->private_data;
	seq_printf(tmp1, "scull:\n\tquantum size: %i\n\n", QUANTUM_SIZE);
	return seq_read(fp, buf, cnt, f_off);
}
*/

static struct file_operations scull_seq_file_ops = {
	.owner = THIS_MODULE,
	.open = scull_seq_file_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

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
		scull_dev_init(tmp, i);
		cdev_add(&tmp->cdev, dev_num, 1);
	}
	create_proc_read_entry("scull", 0, NULL, scull_read_proc, NULL);
	proc_create("scullseq", 0, NULL, &scull_seq_file_ops);
	return 0;
}

void scull_exit(void)
{
	int i;
	for (i = 0; i < DEV_COUNT; i++) {
		cdev_del(&scull.dev[i].cdev);
		truncate_quantums(&scull.dev[i]);
	}
	remove_proc_entry("scull", NULL);
	remove_proc_entry("scullseq", NULL);
	unregister_chrdev_region(MKDEV(major, 0), DEV_COUNT);
}

module_init(scull_init);
module_exit(scull_exit);
