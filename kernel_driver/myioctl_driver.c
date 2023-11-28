#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>

#define MYIOCTL_MAGIC 'k'
#define MYIOCTL_RESET _IO(MYIOCTL_MAGIC, 0)
#define MYIOCTL_GET_COUNT _IOR(MYIOCTL_MAGIC, 1, int)
#define MYIOCTL_INCREMENT _IOW(MYIOCTL_MAGIC, 2, int)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("B11001035_劉家成");
MODULE_DESCRIPTION("Simple IOCTL Example");

static int myioctl_major;
static long count = 0;

static int myioctl_open(struct inode *inode, struct file *filp);
static int myioctl_release(struct inode *inode, struct file *filp);
static long myioctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static const struct file_operations myioctl_fops = {
    .open = myioctl_open,
    .release = myioctl_release,
    .unlocked_ioctl = myioctl_ioctl,
};

static const char *my_file_path = "/etc/my_variable_file";

static void save_variable(void) {
    struct file *file;
    loff_t pos = 0;

    file = filp_open(my_file_path, O_WRONLY | O_CREAT, 0644);
    if (IS_ERR(file)) {
        printk(KERN_ERR "Error opening file %s\n", my_file_path);
        return;
    }

    kernel_write(file, &count, sizeof(count), &pos);

    filp_close(file, NULL);
}

static void load_variable(void) {
    struct file *file;
    loff_t pos = 0;

    file = filp_open(my_file_path, O_RDONLY, 0);
    if (IS_ERR(file)) {
        printk(KERN_ERR "Error opening file %s\n", my_file_path);
        return;
    }

    kernel_read(file, &count, sizeof(count), &pos);

    filp_close(file, NULL);
}


static int __init myioctl_init(void) {
    myioctl_major = register_chrdev(0, "myioctl", &myioctl_fops);

    if (myioctl_major < 0) {
        pr_err("Failed to register character device\n");
        return myioctl_major;
    }
    load_variable();
    pr_info("my ioctl module loaded. Major number: %d\n", myioctl_major);

    return 0;
}

static void __exit myioctl_exit(void) {
    save_variable();
    unregister_chrdev(myioctl_major, "myioctl");
    
    pr_info("myioctl module unloaded\n");
}

static int myioctl_open(struct inode *inode, struct file *filp) {
    pr_info("myioctl device opened\n");
    return 0;
}

static int myioctl_release(struct inode *inode, struct file *filp) {
    pr_info("myioctl device closed\n");
    return 0;
}

static long myioctl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    int err = 0;
    int tmp;

    if (_IOC_TYPE(cmd) != MYIOCTL_MAGIC) {
        pr_err("Invalid magic number\n");
        return -ENOTTY;
    }

    switch(cmd) {
        case MYIOCTL_RESET:
            pr_info("IOCTL: Resetting counter\n");
            count = 0;
            break;
        
        case MYIOCTL_GET_COUNT:
            pr_info("IOCTL: Getting counter value\n");
            err = copy_to_user((int *)arg, &count, sizeof(long));
            break;
        
        case MYIOCTL_INCREMENT:
            pr_info("IOCTL: Incrementing counter\n");
            err = copy_from_user(&tmp, (int *)arg, sizeof(int));
            if (err == 0) {
                count += tmp;
            }
            break;
        
        default:
            pr_err("Unknown IOCTL command\n");
            return -ENOTTY;
    }

    return err;
}

module_init(myioctl_init);
module_exit(myioctl_exit);
