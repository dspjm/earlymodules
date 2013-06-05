#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

void io_sig_handler(int sig)
{
	printf("IO signal recieved\n");
}

struct sigaction io_sa = { 
	.sa_handler = io_sig_handler,
};

int main(int argc, char **argv)
{
	int tmp1, tmp2;
	tmp1 = open("/dev/scull0", O_RDONLY);
	tmp2 = fcntl(tmp1, F_GETFL);
	fcntl(tmp1, F_SETOWN, getpid());
	fcntl(tmp1, F_SETFL, tmp2 | FASYNC);
	sigaction(SIGIO, &io_sa, NULL);
	while (1)
		sleep(10);
	printf("exiting");
	return 0;
}
