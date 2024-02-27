#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "simplechar"
#define CLASS_NAME "simplechar"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A simple Linux char driver");
MODULE_VERSION("0.1");

static int majorNumber;
static char message[256] = {0};
static short size_of_message;
static struct class *simplecharClass = NULL;
static struct device *simplecharDevice = NULL;

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
    {
        .open = dev_open,
        .read = dev_read,
        .write = dev_write,
        .release = dev_release,
};

static int __init simplechar_init(void)
{
    printk(KERN_INFO "SimpleChar: Initializing the SimpleChar LKM\n");

    // デバイス番号動的割り当て
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0)
    {
        printk(KERN_ALERT "SimpleChar failed to register a major number\n");
        return majorNumber;
    }
    printk(KERN_INFO "SimpleChar: registered correctly with major number %d\n", majorNumber);

    // デバイスクラスの作成
    simplecharClass = class_create(CLASS_NAME);
    if (IS_ERR(simplecharClass))
    {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to register device class\n");
        return PTR_ERR(simplecharClass);
    }
    printk(KERN_INFO "SimpleChar: device class registered correctly\n");

    // デバイスの作成
    simplecharDevice = device_create(simplecharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(simplecharDevice))
    {
        class_destroy(simplecharClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "Failed to create the device\n");
        return PTR_ERR(simplecharDevice);
    }
    printk(KERN_INFO "SimpleChar: device class created correctly\n");
    return 0;
}

static void __exit simplechar_exit(void)
{
    device_destroy(simplecharClass, MKDEV(majorNumber, 0));
    class_unregister(simplecharClass);
    class_destroy(simplecharClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "SimpleChar: Goodbye from the LKM!\n");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "SimpleChar: Device has been opened\n");
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    int error_count = 0;

    // 既に読み取り位置がメッセージの終わりを超えているかチェック
    if (*offset >= size_of_message)
    {
        return 0;
    }

    // 要求された長さが残りのメッセージサイズを超えていないか確認
    if (len > size_of_message - *offset)
    {
        len = size_of_message - *offset;
    }

    // ユーザー空間へのデータのコピー
    error_count = copy_to_user(buffer, message + *offset, len);

    if (error_count == 0)
    {
        // オフセット（読み取り位置）の更新
        *offset += len;
        printk(KERN_INFO "SimpleChar: Sent %zu characters to the user\n", len);
        return len;
    }
    else
    {
        printk(KERN_ERR "SimpleChar: Failed to send %d characters to the user\n", error_count);
        return -EFAULT;
    }
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    unsigned long not_copied;

    // 入力データがバッファサイズを超えていないかチェック
    if (len > sizeof(message) - 1)
    {
        printk(KERN_WARNING "SimpleChar: Input data is too long\n");
        return -EINVAL;
    }

    // ユーザー空間からのデータのコピー
    not_copied = copy_from_user(message, buffer, len);

    if (not_copied == 0)
    {
        // 終端文字の追加
        message[len] = '\0';
        // メッセージサイズの更新
        size_of_message = strlen(message);
        printk(KERN_INFO "SimpleChar: Received %zu characters from the user\n", len);
        return len;
    }
    else
    {
        return -EFAULT;
    }
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "SimpleChar: Device successfully closed\n");
    return 0;
}

module_init(simplechar_init);
module_exit(simplechar_exit);
