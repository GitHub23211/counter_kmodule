#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/ioctl.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your name and email address here");
MODULE_DESCRIPTION("Comp2291 Counter driver.");

#define uint8_t unsigned char
#define DEV_IOC_MAGIC 0x5b
#define DEV_IOC_RST _IO(DEV_IOC_MAGIC, 0)
#define DEV_IOC_GET _IOR(DEV_IOC_MAGIC, 1, uint8_t*)
#define DEV_IOC_MOD _IOW(DEV_IOC_MAGIC, 2, uint8_t*)

int dev_open(struct inode* inde, struct file* fle);
int dev_rel(struct inode* inde, struct file* fle);
ssize_t dev_read(struct file *filp, char __user *buff, size_t count, loff_t *offp);
ssize_t dev_write(struct file *filp, const char __user *buff, size_t count, loff_t *offp);
static long ioctl(struct file *file, unsigned int cmd, unsigned long arg);

dev_t dev;
int minor_num = 0;
int dev_count = 5;
char* kern_mem = NULL;
int err = 0;
int res = 0;
int modulus = 255; //Hexadecimal natural maximum
int alloc_mem = 8; //in bytes

struct counter_dev {
    struct cdev cdev;
};

struct file_operations fops = {
    owner: THIS_MODULE,
    open: dev_open,
    release: dev_rel,
    read: dev_read,
    write: dev_write,
    unlocked_ioctl: ioctl
};

struct counter_dev devs;
struct class* dev_class;

static long ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
    switch(cmd) {
        case DEV_IOC_GET:
            copy_to_user((uint8_t*) arg, &res, sizeof(uint8_t));
            break;
        case DEV_IOC_MOD:
            char* temp = kmalloc(sizeof(uint8_t), GFP_KERNEL);
            copy_from_user(temp, (uint8_t*) arg, sizeof(uint8_t));
            modulus = *temp;
            printk(KERN_NOTICE "new modulus: %d\n", modulus);
            kfree(temp);
            break;
        case DEV_IOC_RST:
            res = 0;
            modulus = 255;
            break;
        default:
            printk(KERN_NOTICE "ran ioctl method");
            break;
    }
    return 0;
}

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
        printk(KERN_NOTICE "MAJOR %d, MINOR %d\n", MAJOR(dev), MINOR(dev) + i);
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
    printk(KERN_NOTICE "dev_open\n");
    if((kern_mem = kmalloc(alloc_mem, GFP_KERNEL)) == NULL) {
        printk(KERN_ALERT "Couldn't allocate memory in kernel\n");
        return -1;
    }
    return 0;
}

ssize_t dev_write(struct file* filp, const char __user* buff, size_t count, loff_t* offp) {
    if(count >= alloc_mem) {
        printk(KERN_ALERT "writing too many characters to device. Aborting... %s\n", kern_mem);
        return -1;
    }
    copy_from_user(kern_mem, buff, count);
    res = *kern_mem;
    if(res > modulus) {
        printk(KERN_ALERT "Value is greater than modulus. Aborting... %d\n", res);
        return -1;
    }
    printk(KERN_NOTICE "dev_write %s, res: %d\n", kern_mem, res);
    return count;
}

ssize_t dev_read(struct file* filp, char __user* buff, size_t count, loff_t* offp) {
    printk(KERN_NOTICE "dev_read %s\n", buff);
    copy_to_user(buff, &res, sizeof(int));
    if(res > 0) {
        res = res - 1;
    }
    return count;
}

int dev_rel(struct inode* inde, struct file* fle) {
    printk(KERN_NOTICE "dev_rel\n");
    kfree(kern_mem);
    kern_mem = NULL;
    return 0;
}

module_init(hello_init);
module_exit(hello_exit);