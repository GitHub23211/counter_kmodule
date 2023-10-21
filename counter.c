#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your name and email address here");
MODULE_DESCRIPTION("Comp2291 Counter driver.");

int dev_open(struct inode* inde, struct file* fle);
int dev_rel(struct inode* inde, struct file* fle);
ssize_t dev_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
ssize_t dev_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);

static int times = 1;
dev_t dev;
int minor_num = 0;
int dev_count = 5;
int err = 0;
void* kern_mem;
int value_buf[255];

struct counter_dev {
    struct cdev cdev;
};

struct file_operations fops = {
    owner: THIS_MODULE,
    open: dev_open,
    release: dev_rel,
    read: dev_read,
    write: dev_write
};

struct counter_dev devs;
struct class* dev_class;
module_param(times, int, S_IRUGO);

static int hello_init(void) {
    if(alloc_chrdev_region(&dev, minor_num, dev_count, "counter") < 0) {
        printk(KERN_NOTICE "Cannot allocate major number...\n");
    }

    cdev_init(&(devs.cdev), &fops);
    devs.cdev.owner = THIS_MODULE;
    devs.cdev.ops = &fops;

    for(int i = 0; i < dev_count; i++) {
        err = cdev_add (&(devs.cdev), MKDEV(MAJOR(dev), MINOR(dev) + i), 1);
        /* Fail gracefully if need be */
       if (err) {
        printk(KERN_NOTICE "Error %d adding counter%d", err, i);
        }
    }

    dev_class = class_create(THIS_MODULE, "counter");
    for(int i = 0; i < dev_count; i++) {
        device_create(dev_class, NULL, MKDEV(MAJOR(dev), MINOR(dev) + i), NULL, "counter%d", i);
        printk(KERN_ALERT "MAJOR %d, MINOR %d\n", MAJOR(dev), MINOR(dev) + i);
    }
    return 0;
}

static void hello_exit(void) {
    for(int i = 0; i < dev_count; i++) {
        device_destroy(dev_class, MKDEV(MAJOR(dev), MINOR(dev) + i));
    }
    class_destroy(dev_class);
    cdev_del(&(devs.cdev));
    unregister_chrdev_region(dev, dev_count);
    printk(KERN_ALERT "Goodbye word \n");
}

int dev_open(struct inode* inde, struct file* fle) {
    if((kern_mem = kmalloc(255, GFP_KERNEL)) == 0) {
        printk(KERN_ALERT "Couldn't allocate memory in kernel\n");
        return -1;
    }
    printk(KERN_NOTICE "dev_open\n");
    return 0;
}

ssize_t dev_write(struct file* filp, const char __user* buff, size_t count, loff_t* offp) {
    printk(KERN_NOTICE "dev_write\n");
    copy_from_user(kern_mem, buff, count);
    printk(KERN_NOTICE "kern_mem %s\n", (char *) kern_mem);
    return count;
}


ssize_t dev_read(struct file* filp, char __user* buff, size_t count, loff_t* offp) {
    printk(KERN_NOTICE "dev_read\n");
    snprintf(kern_mem, 255, "%s snprintf", (char*) buff);
    copy_to_user(buff, kern_mem, 255);
    printk(KERN_NOTICE "read buffer %s\n", buff);
    return count;
}

int dev_rel(struct inode* inde, struct file* fle) {
    printk(KERN_NOTICE "dev_rel\n");
    kfree(kern_mem);
    return 0;
}
module_init(hello_init);
module_exit(hello_exit);