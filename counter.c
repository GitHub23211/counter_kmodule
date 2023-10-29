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
char* device_name = "counter";
char* kern_mem = NULL;
int res = 0;
int modulus = 255; //1 byte
int alloc_mem = 8; //in bytes

int minor_start_num = 0;
static int major_num = -1;
static int minor_count = 5;
module_param(major_num, int, S_IRUGO);
module_param(minor_count, int, S_IRUGO);

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

static int hello_init(void) {
    int alloc_err;
    int cdev_add_err;

    if(major_num == -1) {
        if((alloc_err = alloc_chrdev_region(&dev, minor_start_num, minor_count, device_name)) != 0) {
            pr_notice("Error %d: Cannot allocate major number...\n", alloc_err);
            return -1;
        }
    }
    else {
        dev = MKDEV(major_num, minor_start_num);
        if((alloc_err = register_chrdev_region(dev, minor_count, device_name)) != 0) {
            pr_notice("Error %d: Cannot allocate major number...\n", alloc_err);
            return -1;
        }
    }

    pr_info("Device %s inserted\n", device_name);

    cdev_init(&(devs.cdev), &fops);
    devs.cdev.owner = THIS_MODULE;
    devs.cdev.ops = &fops;
    dev_class = class_create(THIS_MODULE, device_name);
    if((cdev_add_err = cdev_add(&(devs.cdev), dev, minor_count)) < 0) {
        pr_notice("Error %d adding %s%d\n", cdev_add_err, device_name, MINOR(dev));
    }

    for(int i = 0; i < minor_count; i++) {
        device_create(dev_class, NULL, MKDEV(MAJOR(dev), MINOR(dev) + i), NULL, "%s%d", device_name, i);
        pr_notice("MAJOR %d, MINOR %d\n", MAJOR(dev), MINOR(dev) + i);
    }
    return 0;
}

static void hello_exit(void) {
    for(int i = 0; i < minor_count; i++) {
        device_destroy(dev_class, MKDEV(MAJOR(dev), MINOR(dev) + i));
    }
    class_destroy(dev_class);
    cdev_del(&(devs.cdev));
    unregister_chrdev_region(dev, minor_count);
    pr_info("Device %s removed\n", device_name);
}

int dev_open(struct inode* inde, struct file* fle) {
    if((kern_mem = kmalloc(alloc_mem, GFP_KERNEL)) == NULL) {
        pr_notice("Couldn't allocate memory in kernel\n");
        return -1;
    }
    return 0;
}

ssize_t dev_write(struct file* filp, const char __user* buff, size_t count, loff_t* offp) {
    if(count >= alloc_mem) {
        pr_notice("Bytes to write: %lu > allocated memory: %d bytes. Aborting...\n", count, alloc_mem);
        return -1;
    }

    if(copy_from_user(kern_mem, buff, count) != 0) {
        pr_notice("Could not copy data from user space to kernel space");
        return -1;
    }

    if((int) (*kern_mem) > modulus) {
        pr_notice("%s%d: overflow %d > %d\n", device_name, MINOR(dev), res, modulus);
        return -1;
    }
    res = (int) *kern_mem;
    pr_debug("%s%d: wrote %ld\n", device_name, MINOR(dev), count);
    return count;
}

ssize_t dev_read(struct file* filp, char __user* buff, size_t count, loff_t* offp) {
    if(copy_to_user(buff, &res, sizeof(int)) != 0) {
        pr_notice("Could not copy data from kernel space to user space");
        return -1;
    }
    if(res > 0) {
        res = res - 1;
    }
    pr_debug("%s%d: read %ld\n", device_name, MINOR(dev), count);
    return count;
}

int dev_rel(struct inode* inde, struct file* fle) {
    kfree(kern_mem);
    kern_mem = NULL;
    return 0;
}

static long ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
    switch(cmd) {
        case DEV_IOC_GET:
            if(copy_to_user((uint8_t*) arg, &res, sizeof(uint8_t)) != 0) {
                pr_notice("Could not copy data from kernel space to user space");
                break;
            }
            pr_debug("%s%d: DEV_IOC_GET: %d\n", device_name, MINOR(dev), res);
            break;
        case DEV_IOC_MOD:
            char* temp = kmalloc(sizeof(uint8_t), GFP_KERNEL);
            if(copy_from_user(temp, (uint8_t*) arg, sizeof(uint8_t)) != 0) {
                pr_notice("Could not copy data from user space to kernel space");
                break;
            }
            if(temp) {
                modulus = *temp;
                pr_debug("%s%d: DEV_IOC_MOD: %d\n", device_name, MINOR(dev), modulus);
                break;
            }
            pr_notice("%s%d: DEV_IOC_MOD: error\n", device_name, MINOR(dev));
            kfree(temp);
            break;
        case DEV_IOC_RST:
            res = 0;
            modulus = 255;
            pr_debug("%s%d: DEV_IOC_RST\n", device_name, MINOR(dev));
            break;
        default:
            pr_notice("%s%d: DEV_IOC_ERR\n", device_name, MINOR(dev));
            break;
    }
    return 0;
}

module_init(hello_init);
module_exit(hello_exit);