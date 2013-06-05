/* scull.h
 *
 * Author: Jimmy Pan
 * Email: dspjmt@gmail.com
 * Date: Sat Dec 15 06:38:05 CST 2012
 * 
 * Algorithm
 * Input:
 * Output:
 * Functions:
 *
 */

#include <linux/semaphore.h>
#include <linux/cdev.h>

#define DEV_COUNT 4
#define QUANTUM_SIZE 1024

struct quantum {
	void *data;
	struct quantum *next;
};

struct scull_dev {
	struct cdev cdev;
	struct quantum head;
	int q_cnt;
	int minor;
	struct semaphore m;
};

struct scull {
	struct scull_dev devs[DEV_COUNT];
	int major;
};
