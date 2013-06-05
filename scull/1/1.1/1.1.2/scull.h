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

#define DEV_NAME "scull"
#define DEV_COUNT 4
#define QUANTUM_SIZE 1024
#define MAX_TRY 10

struct quantum {
	void *data;
	struct quantum *next;
};

struct scull_dev {
	struct cdev cdev;
	struct quantum head;
	int q_cnt;
	int minor;
	struct semaphore sem;
};

struct scull {
	struct scull_dev dev[DEV_COUNT];
};

#endif
