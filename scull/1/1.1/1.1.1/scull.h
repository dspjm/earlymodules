/* scull.h
 *
 * Author: Jimmy Pan
 * Email: dspjmt@gmail.com
 * Date: Fri Dec  7 13:07:57 CST 2012
 * 
 * Algorithm
 * Input:
 * Output:
 * Functions:
 *
 */

#ifndef SCULL_H_
#define SCULL_H_

#define DEV_NAME "scull"
#define DEV_COUNT 4
#define QUANTUM_SIZE 256
#define MAX_TRY 10

struct quantum {
	void *data;
	struct quantum *next;
};

struct scull_dev {
	struct cdev cdev;
	struct quantum head;
	int q_cnt;
};

struct scull {
	struct scull_dev dev[DEV_COUNT];
};

#endif
