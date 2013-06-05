#include <linux/kernel.h>
#include <linux/module.h>

int init_hello_2(void)
{
	printk(KERN_INFO "Hello, world - this is the kernel speaking\n");
	return 0;
}
