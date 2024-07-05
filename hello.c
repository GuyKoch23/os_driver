#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple kernel module that prints 'hello' on load");
MODULE_VERSION("0.1");

static int __init hello_init(void) {
    printk(KERN_ERR "Hello, kernel!\n");
    return 0; // Non-zero return means that the module couldn't be loaded.
}

static void __exit hello_exit(void) {
    printk(KERN_ALERT "Goodbye, kernel!\n");
}

module_init(hello_init);
module_exit(hello_exit);