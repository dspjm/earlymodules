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

struct data_set {
	void *data;
	struct list_head list;
}

struct scull_dev {
	void *data;
	int minor;
	struct cdev *cdev;
};

struct scull {
	struct scull_dev *scull_dev;
	int major;
	int dev_count;
	unsigned int dev_size;
};

#endif
