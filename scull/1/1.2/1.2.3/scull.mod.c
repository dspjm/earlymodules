#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x41086e, "module_layout" },
	{ 0x581bf245, "seq_release" },
	{ 0x2b084b2b, "seq_read" },
	{ 0xa54015e2, "seq_lseek" },
	{ 0xe292647d, "kill_fasync" },
	{ 0x642e54ac, "__wake_up" },
	{ 0x105e2727, "__tracepoint_kmalloc" },
	{ 0x8fde5204, "slab_buffer_size" },
	{ 0xfc822e6e, "kmem_cache_alloc_notrace" },
	{ 0xc288f8ce, "malloc_sizes" },
	{ 0x589f30ed, "proc_create_data" },
	{ 0x40c497a1, "cdev_add" },
	{ 0xf28b622f, "cdev_init" },
	{ 0xffc7c184, "__init_waitqueue_head" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x9ccb2622, "finish_wait" },
	{ 0x4292364c, "schedule" },
	{ 0x1985ed3c, "prepare_to_wait_exclusive" },
	{ 0xc8b57c27, "autoremove_wake_function" },
	{ 0x7d0bea0b, "per_cpu__current_task" },
	{ 0xb72397d5, "printk" },
	{ 0x2da418b5, "copy_to_user" },
	{ 0xf2a644fb, "copy_from_user" },
	{ 0x3f1899f1, "up" },
	{ 0xfc4f55f3, "down_interruptible" },
	{ 0x4d80993a, "fasync_helper" },
	{ 0x79566c2d, "seq_printf" },
	{ 0x3d8e39e5, "seq_open" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x8d8aacb9, "remove_proc_entry" },
	{ 0xbef9fe65, "cdev_del" },
	{ 0x37a0cba, "kfree" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "82423DEE228282F38624B44");

static const struct rheldata _rheldata __used
__attribute__((section(".rheldata"))) = {
	.rhel_major = 6,
	.rhel_minor = 3,
};
