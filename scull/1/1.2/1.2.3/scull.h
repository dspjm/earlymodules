/* scull.h
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

#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/wait.h>

#define DEV_COUNT 4
#define QUANTUM_SIZE 1024
#define SCULL_IOCTL_MAGIC 'm'

struct ioctl_isr_para {
	void *addr;
	int buf_size;
	int num;
};

enum scull_ioctl_enum {
	IOCTL_TRIM = _IO(SCULL_IOCTL_MAGIC, 1),
	IOCTL_SPEC_READ = _IOR(SCULL_IOCTL_MAGIC, 2, int),
};

struct quantum {
	void *data;
	int len;
	struct quantum *next;
};

struct scull_dev {
	struct cdev cdev;
	struct quantum head;
	int q_cnt;
	int sum_len;
	int minor;
	struct semaphore m;
	struct fasync_struct *faq;
	wait_queue_head_t q;
};

struct scull {
	struct scull_dev devs[DEV_COUNT];
	int major;
};
