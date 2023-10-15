#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your name and email address here");
MODULE_DESCRIPTION("Comp2291 Counter driver.");


static char* whom = "world";
static int times = 1;
module_param(times, int, S_IRUGO);
module_param(whom, charp, S_IRUGO);

dev_t dev;
int minor_num = 0;
int count = 5;

static int dev_open(struct inode* inde, struct file* fle);
static int dev_rel(struct inode* inde, struct file* fle);

struct counter_dev {
    struct cdev cdev;
};

struct file_operations fops = {
    owner: THIS_MODULE,
    open: dev_open,
    release: dev_rel
};

struct counter_dev devs;
int err = 0;

static int dev_open(struct inode* inde, struct file* fle) {
    printk(KERN_NOTICE "dev_open\n");
    return 0;
}

static int dev_rel(struct inode* inde, struct file* fle) {
    printk(KERN_NOTICE "dev_rel\n");
    return 0;
}

static int hello_init(void) {
    if(alloc_chrdev_region(&dev, minor_num, count, "counter") < 0) {
        printk(KERN_NOTICE "Cannot allocate major number...\n");
    }
    printk(KERN_ALERT "MAJOR %d, MINOR %d\n", MAJOR(dev), MINOR(dev));


    cdev_init(&(devs.cdev), &fops);
    devs.cdev.owner = THIS_MODULE;
    devs.cdev.ops = &fops;
    err = cdev_add (&(devs.cdev), dev, 1);
    /* Fail gracefully if need be */
    if (err)
    printk(KERN_NOTICE "Error %d adding counter0", err);


    for(int i = 0; i < times; i++) {
        printk(KERN_ALERT "Hello %s\n", whom);
    }

    return 0;
}


static void hello_exit(void) {
    printk(KERN_ALERT "Goodbye word \n");
    unregister_chrdev_region(dev, count);
    cdev_del(&(devs.cdev));
}

module_init(hello_init);
module_exit(hello_exit);

/* vi: set ts=4 sw=4 et ic nows: */
