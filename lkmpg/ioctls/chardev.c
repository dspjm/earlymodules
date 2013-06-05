#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/types.h>
#include <linux/cdev.h>

#include "chardev.h"

#define SUCCESS 0
#define DEVICE_NAME "char_dev"
#define BUF_LEN 80

static int Device_Open = 0;
static char Message[BUF_LEN];
static char *Message_Ptr;
static dev_t dev;
static struct cdev *cdev;
static int major_num;

static int device_open(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printf(KERN_INFO "device_open(%p)\n", file);
#endif

	if (Device_Open)
		return -EBUSY;
	Device_Open++;
	Message_Ptr = Message;
	try_module_get(THIS_MODULE);
	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG
	printk(KERN_INFO "device_release(%p,%p)\n", inode, file);
#endif
	Device_Open--;
	module_put(THIS_MODULE);
	return SUCCESS;
}

static ssize_t device_read(struct file *file, char __user *buffer,
			     size_t length, loff_t *offset)
{
	int bytes_read = 0;
#ifdef DEBUG
	printk(KERN_INFO "device_read(%p,%p,%d)\n", file, buffer, length);
#endif
	if (*Message_Ptr == 0)
		return 0;
	while (length && *Message_Ptr) {
		put_user(*(Message_Ptr++), buffer++);
		length--;
		bytes_read++;
	}
#ifdef DEBUG
	printk(KERN_INFO "Read %d bytes, %d left\n", bytes_read, length);
#endif
	return bytes_read;
}

static ssize_t device_write(struct file *file, const char __user *buffer,
			      size_t length, loff_t *offset)
{
	int i;
#ifdef DEBUG
	printk(KERN_INFO "device_write(%p,%s,%d)", file, buffer, length);
#endif
	for (i = 0; i < length && i < BUF_LEN; i++)
		get_user(Message[i], buffer + i);
	Message_Ptr = Message;
	return i;
}

int device_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num,
		   unsigned long ioctl_param)
{
	int i;
	char *temp;
	char ch;
	switch (ioctl_num) {
	case IOCTL_SET_MSG:
		temp = (char *)ioctl_param;
		get_user(ch, temp);
		for (i = 0; ch && i < BUF_LEN; i++, temp++)
			get_user(ch, temp);
		device_write(file, (char *)ioctl_param, i, 0);
		break;

	case IOCTL_GET_MSG:
		i = device_read(file, (char *)ioctl_param, 99, 0);
		put_user('\0', (char *)ioctl_param+i);
		break;

	case IOCTL_GET_NTH_BYTE:
		return Message[ioctl_param];
		break;
	}
	return SUCCESS;
}

struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,
};

int init_module(void)
{
	int ret_val;
	ret_val = alloc_chrdev_region(&dev, 0, 4, DEVICE_NAME);
	if (ret_val) {
		printk(KERN_ALERT "%s failed with %d\n",
			 "Sorry, registering the character device ", ret_val);
		return ret_val;
	}
	major_num = MAJOR(dev);
	cdev = cdev_alloc();
	cdev_init(cdev, &fops);
	if (cdev_add(cdev, dev, 4)) {
		printk(KERN_ALERT "Unable to add cdev.\n");
		return -1;
	}
	printk(KERN_INFO "%s The major device number is %d.\n",
		 "Registeration is a success", major_num);
	printk(KERN_INFO "If you want to talk to the device driver,\n");
	printk(KERN_INFO "you'll have to create a device file.\n");
	printk(KERN_INFO "We suggest you use:\n");
	printk(KERN_INFO "mknod %s c %d 0\n", DEVICE_NAME, major_num);
	printk(KERN_INFO "The device file name is important, because\n");
	printk(KERN_INFO "the ioctl program assumes that's the\n");
	printk(KERN_INFO "file you'll use.\n");
	return 0;
}

void cleanup_module()
{
}
