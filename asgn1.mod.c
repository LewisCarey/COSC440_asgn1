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
	{ 0xda9e78e9, "module_layout" },
	{ 0x4b8b420c, "device_destroy" },
	{ 0x7485e15e, "unregister_chrdev_region" },
	{ 0x74738f6d, "cdev_del" },
	{ 0x85305f03, "class_destroy" },
	{ 0x148887b7, "device_create" },
	{ 0xb4c75863, "__class_create" },
	{ 0x39c19d5, "create_proc_entry" },
	{ 0xe3321d40, "cdev_add" },
	{ 0x60c2503e, "cdev_init" },
	{ 0x9d0d02cf, "cdev_alloc" },
	{ 0x29537c9e, "alloc_chrdev_region" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
	{ 0x37a0cba, "kfree" },
	{ 0x969267bd, "__free_pages" },
	{ 0x67c2fa54, "__copy_to_user" },
	{ 0x30407f, "contig_page_data" },
	{ 0xeed38436, "malloc_sizes" },
	{ 0xfa2a45e, "__memzero" },
	{ 0xfbc74f64, "__copy_from_user" },
	{ 0x2da2b3ae, "__alloc_pages_nodemask" },
	{ 0xd197d610, "kmem_cache_alloc" },
	{ 0x7ec1c780, "mem_map" },
	{ 0x5cb27d49, "remap_pfn_range" },
	{ 0x27e1a049, "printk" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "FAC3446361EE428C25A1056");
