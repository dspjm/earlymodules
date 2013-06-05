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
 *
 */

#ifndef SCULL_H_
#define SCULL_H_

#include <linux/types.h>
#include <linux/semaphore.h>

struct data_set {
	int set_num;
	void *data;
	struct list_head list;
};

struct scull_dev {
	struct data_set data_set;
	int set_cnt;
	int minor;
	struct cdev *cdev;
	struct semaphore sem;
};

struct scull {
	struct scull_dev *scull_dev;
	int major;
	int dev_count;
	unsigned int dev_size;
};

#endif
