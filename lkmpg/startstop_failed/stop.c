#include <linux/kernel.h>
#include <linux/module.h>

int exit_hello_2()
{
	printk(KERN_INFO "Short is the life of a kernel module\n");
}
