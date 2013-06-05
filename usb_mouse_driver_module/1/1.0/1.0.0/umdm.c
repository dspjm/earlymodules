/* umdm.c
 * main file for my usb mouse driver module
 * Author: Jimmy Pan
 * Email: dspjmt@gmail.com
 * Date:
 * 
 * Algorithm
 * Input:
 * Output:
 * Functions:
 * 	umdm_init
 * 	umdm_exit
 *
 */

#include <linux/usb.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#define VENDOR_ID 0x0458
#define DEVICE_ID 0x0007
#define MAX_DEV_CNT 3
#define DESC_BUF_SIZE 2048

#define UMDM_DEBUG

#undef DBPR
#ifdef UMDM_DEBUG
#define DBPR(fmt, ...) pr_alert("umdm: " fmt, ##__VA_ARGS__)
#else
#define DBPR(fmt, ...)
#endif

#define DBPRS(s) DBPR(#s " = %s\n", s)
#define DBPRI(n) DBPR(#n " = %d\n", n)
#define DBPRP(p) DBPR(#p " = %p\n", p)

static struct umdm_intfs{
	struct usb_interface *intfps[MAX_DEV_CNT];
	int least_avail_intfp;
	int intf_cnt;
} umdm_intfs;

struct umdm_cdev_file_priv {
	int intfs_index;
	char desc_buf[DESC_BUF_SIZE];
};

static ssize_t umdm_cdev_read(struct file *filp, char __user *buf, size_t n,
							loff_t *off)
{
	struct umdm_cdev_file_priv * tmp1;
	int tmp2, ret;
	DBPR("reading cdev\n");
	if (*off >= DESC_BUF_SIZE)
		return 0;
	if (DESC_BUF_SIZE - *off < n)
		n = DESC_BUF_SIZE - *off;
	tmp1 = filp->private_data;
	tmp2 = copy_to_user(buf, tmp1->desc_buf + *off, n);
	pr_alert("%s\n", tmp1->desc_buf + *off);
	ret = n - tmp2;
	*off += ret;
	return ret;
}

static ssize_t umdm_cdev_write(struct file *filp, const char __user *buf,
							size_t n, loff_t *off)
{
	return 0;
}

static int umdm_cdev_open(struct inode *inode, struct file *filp)
{
	int tmp1, tmp6;
	struct umdm_cdev_file_priv *tmp2;
	struct usb_interface *tmp3;
	char *tmp4;
	struct usb_device_descriptor *tmp5;
	tmp1 = MINOR(inode->i_rdev);
	tmp2 = kzalloc(sizeof *tmp2, GFP_KERNEL);
	tmp2->intfs_index = tmp1;
	if (!tmp2)
		return -ENOMEM;
	filp->private_data = tmp2;
	tmp3 = umdm_intfs.intfps[tmp2->intfs_index];
	if (!tmp3)
		return -ENODEV;
	tmp4 = tmp2->desc_buf;
	tmp5 = &interface_to_usbdev(tmp3)->descriptor;
	DBPRP(tmp4);
	DBPRP(tmp5);
	pr_alert("%d\n", tmp5->bDescriptorType);
	tmp6 = snprintf(tmp4, DESC_BUF_SIZE, "idVendor = 0x%x\n"
	  "idProduct = 0x%x\niSerialNumber = %d\nbDeviceClass = %d\n"
	  "bMaxPacketSize0 = %d\n", tmp5->idVendor, tmp5->idProduct,
	  tmp5->iSerialNumber, tmp5->bDeviceClass, tmp5->bMaxPacketSize0);
	if (tmp6 < 0)
		return tmp6;
	return 0;
}

static int umdm_cdev_release(struct inode *inode, struct file *filp)
{
	kfree(filp->private_data);
	return 0;
}

static struct file_operations umdm_cdev_fops = {
	.owner = THIS_MODULE,
	.read = umdm_cdev_read,
	.write = umdm_cdev_write,
	.open = umdm_cdev_open,
	.release = umdm_cdev_release
};

static struct {
	dev_t dev;
	struct cdev cdev;
	struct file_operations *fops;
	int dev_cnt;
} umdm_cdev = { .fops = &umdm_cdev_fops };


static void update_least_avail_intfp(void)
{
	int i;
	for (i = 0; i < MAX_DEV_CNT; i++)
		if (umdm_intfs.intfps[i] == NULL)
			break;
	umdm_intfs.least_avail_intfp = i;
}

static int add_intf_to_umdm_intfs(int *intf_num, struct usb_interface *intf)
{
	int tmp1;
	if (umdm_intfs.intf_cnt >= MAX_DEV_CNT)
		return -EBUSY;
	tmp1 = umdm_intfs.least_avail_intfp;
	*intf_num = tmp1;
	umdm_intfs.intfps[tmp1] = intf;
	umdm_intfs.intf_cnt++;
	update_least_avail_intfp();
	return 0;
}

static int umdm_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	int ret = 0;
	int tmp1;
	DBPR("Probing\n");
	DBPR("Hurrah, I am probed\n");
	ret = add_intf_to_umdm_intfs(&tmp1, intf);
	if (ret)
		goto out;
out:
	return ret;
}

static void umdm_disconnect(struct usb_interface *intf)
{
	
	DBPR("Device disconnected\n");
}

static int umdm_suspend(struct usb_interface *intf, pm_message_t message)
{
	DBPR("Device suspended\n");
	return 0;
}

static int umdm_resume(struct usb_interface *intf)
{
	DBPR("Device resumed\n");
	return 0;
}

static int umdm_reset_resume(struct usb_interface *inf)
{
	DBPR("Deviced reset from suspension\n");
	return 0;
}

static int umdm_pre_reset(struct usb_interface *intf)
{
	DBPR("Device about to reset\n");
	return 0;
}

static int umdm_post_reset(struct usb_interface *intf)
{
	DBPR("Device been reset\n");
	return 0;
}

static struct usb_device_id umdm_id_table[] = {
	{ USB_DEVICE(VENDOR_ID, DEVICE_ID) },
	{}
};

static struct usb_driver umdm_driver = {
	.name = "umdm_driver",
	.probe = umdm_probe,
	.disconnect = umdm_disconnect,
	.suspend = umdm_suspend,
	.resume = umdm_resume,
	.reset_resume = umdm_reset_resume,
	.pre_reset = umdm_pre_reset,
	.post_reset = umdm_post_reset,
	.id_table = umdm_id_table, 
};
MODULE_DEVICE_TABLE(usb, umdm_id_table);

static int umdm_init(void)
{
	int ret = 0;
	DBPR("umdm loaded\n");
	ret = usb_register(&umdm_driver);
	if (ret)
		goto out;
	DBPR("driver registered\n");
	ret = alloc_chrdev_region(&umdm_cdev.dev, 0, MAX_DEV_CNT, "umdm_cdev");
	if (ret)
		goto dereg_usb_driver;
	cdev_init(&umdm_cdev.cdev, umdm_cdev.fops);
	ret = cdev_add(&umdm_cdev.cdev, umdm_cdev.dev, MAX_DEV_CNT);
	if (ret)
		goto unreg_dev_region;
out:
	return ret;
unreg_dev_region:
	unregister_chrdev_region(umdm_cdev.dev, MAX_DEV_CNT);
dereg_usb_driver:
	usb_deregister(&umdm_driver);
	goto out;
}

static void umdm_exit(void)
{
	cdev_del(&umdm_cdev.cdev);
	unregister_chrdev_region(umdm_cdev.dev, MAX_DEV_CNT);
	usb_deregister(&umdm_driver);
	DBPR("driver unregistered\n");
	DBPR("umdm unloadedn");
}

module_init(umdm_init);
module_exit(umdm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jimmy Pan dspjmt@gmail.com");
