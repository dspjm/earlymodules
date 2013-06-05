#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>

int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

#define SUCCESS 0
#define DEVICE_NAME "chardev"
#define BUF_LEN 80

static dev_t dev;
static int Major;
static int Device_Open = 0;
static struct cdev *cdev;

static char msg[BUF_LEN];
static char *msg_Ptr;

static struct file_operations fops = {
	.read = device_read,
	.write = device_write,
	.open = device_open,
	.release = device_release
};

int init_module(void)
{
	int result;
	if (result = alloc_chrdev_region(&dev, 0, 4, DEVICE_NAME)) {
		printk(KERN_ALERT "Registering char device failed\n");
		return result;
	}
	Major = MAJOR(dev);
	cdev = cdev_alloc();
	cdev_init(cdev, &fops);
	if (cdev_add(cdev, dev, 4)) {
		printk(KERN_ALERT "Unable to add cdev.\n");
		return -1;
	}
	printk(KERN_INFO "I was assigned major number %d. To talk to \n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");

	return SUCCESS;
}

void cleanup_module(void)
{
}

static int device_open(struct inode *inode, struct file *file)
{
	static int counter = 0;
	if (Device_Open)
		return -EBUSY;
	Device_Open++;
	sprintf(msg, "I already told you %d times Hello world!\n", counter++);
	msg_Ptr = msg;
	try_module_get(THIS_MODULE);
	return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;
	module_put(THIS_MODULE);
	return 0;
}

static ssize_t device_read(struct file *filp, char *buffer, size_t length,
			     loff_t *offset)
{
	int bytes_read = 0;
	if (*msg_Ptr == 0)
		return 0;
	while (length && *msg_Ptr) {
		put_user(*(msg_Ptr++), buffer++);
		length--;
		bytes_read++;
	}
	return bytes_read;
}

static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
	return -EINVAL;
}
