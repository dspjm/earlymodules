#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

#define BUF_SIZE 1024

int main(int argc, char **argv)
{
	int i;
	int tmp1;
	struct ioctl_isr_para tmp2;
	void *tmp3;
	tmp1 = open("/dev/scull0", O_RDONLY);
	if (argc == 1) {
		printf("Usage: test_prog 1|2 ...\n");
		return 0;
	}
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "1")) {
			ioctl(tmp1, IOCTL_TRIM);
		} else if (!strcmp(argv[i], "2")) {
			tmp3 = malloc(BUF_SIZE);
			tmp2.addr = tmp3;
			tmp2.buf_size = BUF_SIZE;
			tmp2.num = 0;
			ioctl(tmp1, IOCTL_SPEC_READ, &tmp2);
			printf("%s\n", tmp3);
			free(tmp3);
		} else {
		printf("Usage: test_prog 1|2 ...\n");
		return 0;
		}
	}
	return 0;
}
