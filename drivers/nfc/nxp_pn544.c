/*
* pn544.c
* Copyright (C) 2011 NXP Semiconductors
*/

//ALERT:relocate pn544.c under .\kernel\drivers\misc


/*
* Makefile//TODO:Here is makefile reference
* obj-$(CONFIG_PN544)+= pn544.o
*/

#include <linux/fs.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/pn544.h>
#include <linux/fs.h>

//#define NXP_PN544_DEBUG

//#define NXP_PN544_STANDBY_MODE

#define DRIVER_DESC	"NFC driver for PN544"

#define MAX_BUFFER_SIZE		512
#define PN544_MSG_MAX_SIZE	0x21 /* at normal HCI mode */

/* Timing restrictions (ms) */
#define PN544_RESETVEN_TIME	35 /* 7 */

//#define PN544_NFC_SW_UPDATE 36
//#define PN544_NFC_nVEN      38

enum pn544_irq {
	PN544_NONE,
	PN544_INT,
};

struct pn544_dev	{
	wait_queue_head_t	read_wq;
	struct mutex read_mutex;
	struct i2c_client	*client;
	struct miscdevice	miscdev;
	bool irq_enabled;
	spinlock_t irq_enabled_lock;
	enum pn544_irq read_irq;
};

static struct pn544_dev *pn544_dev;

static int pn544_dev_enable(struct pn544_dev *dev, int mode)
{
	struct i2c_client *client = dev->client;
	struct pn544_nfc_platform_data *pdata = client->dev.platform_data;

#ifdef NXP_PN544_DEBUG
	dev_dbg(&client->dev, "%s: mode: %d\n", __func__, mode);
    printk(KERN_DEBUG "%s: mode: %d\n", __func__, mode);
#endif
	dev->read_irq = PN544_NONE;
	if (pdata->enable)
		pdata->enable(mode);
	usleep_range(10000, 15000);
	//usleep_range(20000, 35000);
	return 0;
}

static void pn544_dev_disable(struct pn544_dev *dev)
{
	struct i2c_client *client = dev->client;
	struct pn544_nfc_platform_data *pdata = client->dev.platform_data;

	if (pdata->disable)
		pdata->disable();

	msleep(PN544_RESETVEN_TIME);

	dev->read_irq = PN544_NONE;

#ifdef NXP_PN544_DEBUG
	dev_dbg(&client->dev, "%s: Now in OFF-mode\n", __func__);
    printk(KERN_DEBUG "%s: Now in OFF-mode\n", __func__);
#endif
}

static int pn544_irq_status(struct pn544_dev *dev)
{
	struct i2c_client *client = dev->client;
	struct pn544_nfc_platform_data *pdata = client->dev.platform_data;

	if ( pdata->irq_status ) {
		return 0;
	} else {
		return pdata->irq_status();
	}
}

static void pn544_disable_irq(struct pn544_dev *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->irq_enabled_lock, flags);
	if (dev->irq_enabled) {
		disable_irq_nosync(dev->client->irq);
		dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&dev->irq_enabled_lock, flags);
}

static irqreturn_t pn544_dev_irq_handler(int irq, void *dev_id)
{
	struct pn544_dev *dev = dev_id;

	pn544_disable_irq(dev);

	//dev_dbg(&dev->client->dev, "IRQ\n");

	dev->read_irq = PN544_INT;
	
	/* Wake up waiting readers */
	wake_up(&dev->read_wq);

	return IRQ_HANDLED;
}

static ssize_t pn544_dev_read(struct file *filp, char __user *buf,
		size_t count, loff_t *offset)
{
	//ztemt changed  by chengdongsheng 2012.12.20 avoid the filp->private_data was null
	//struct pn544_dev *dev = filp->private_data;	
	struct pn544_dev *dev = pn544_dev;
	//ztemt end
	struct i2c_client *client = dev->client;
	char tmp[MAX_BUFFER_SIZE];
	int ret;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

#ifdef NXP_PN544_DEBUG
	dev_dbg(&client->dev, "%s : reading %zu bytes.\n", __func__, count);
    printk(KERN_DEBUG "%s: reading %zu bytes.\n", __func__, count);
#endif
	mutex_lock(&dev->read_mutex);

	if ( !pn544_irq_status(dev) ) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto fail;
		}

		dev->irq_enabled = true;
		enable_irq(dev->client->irq);
		ret = wait_event_interruptible(dev->read_wq, 
			(dev->read_irq == PN544_INT));
		pn544_disable_irq(dev);

		if (ret)
			goto fail;
	}
	/* Read data */
	ret = i2c_master_recv(dev->client, tmp, count);
	dev->read_irq = PN544_NONE;
	mutex_unlock(&dev->read_mutex);

	if (ret < 0) {
		dev_err(&client->dev, "%s: i2c_master_recv returned %d\n", 
				__func__, ret);
		return ret;
	}
	if (ret > count) {
		dev_err(&client->dev, "%s: received too many bytes from i2c (%d)\n",
				__func__, ret);
		return -EIO;
	}
#ifdef NXP_PN544_DEBUG
	print_hex_dump(KERN_DEBUG, " read: ", DUMP_PREFIX_NONE, 16, 1, tmp, ret, false);
#endif
	if (copy_to_user(buf, tmp, ret)) {
		dev_err(&client->dev, "%s : failed to copy to user space\n", 
				__func__);
		return -EFAULT;
	}
	return ret;

fail:
	mutex_unlock(&dev->read_mutex);
	return ret;
}

static ssize_t pn544_dev_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *offset)
{
	//ztem changed  by chengdongsheng 2012.12.20 avoid the filp->private_data was null
	//struct pn544_dev *dev = filp->private_data;
	struct pn544_dev *dev = pn544_dev;
	//ztemt end
	struct i2c_client *client = dev->client;
	char tmp[MAX_BUFFER_SIZE];
	int ret;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	if (copy_from_user(tmp, buf, count)) {
		dev_err(&client->dev, "%s : failed to copy from user space\n", __func__);
		return -EFAULT;
	}

#ifdef NXP_PN544_DEBUG
	dev_dbg(&client->dev, "%s : writing %zu bytes.\n", __func__, count);
	print_hex_dump(KERN_DEBUG, "write: ", DUMP_PREFIX_NONE, 16, 1, tmp, count, false);
#endif
	/* Write data */
	ret = i2c_master_send(client, tmp, count);
	if (ret != count) {
		dev_err(&client->dev, "%s : addr is 0x%x, i2c_master_send returned %d\n", __func__, client->addr, ret);
		ret = -EIO;
	}

	return ret;
}

static int pn544_dev_open(struct inode *inode, struct file *filp)
{
#ifdef NXP_PN544_DEBUG
	struct i2c_client *client = pn544_dev->client;
#endif
	filp->private_data = pn544_dev;
#ifdef NXP_PN544_DEBUG
	dev_dbg(&client->dev, "%s : %d,%d\n", __func__, imajor(inode), iminor(inode));
    printk(KERN_DEBUG "%s\n", __func__);
#endif
	return pn544_dev_enable(pn544_dev, HCI_MODE);
}

static long pn544_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{	
	//ztemt changed  by chengdongsheng 2012.12.20 avoid the filp->private_data was null
	//struct pn544_dev *dev = filp->private_data;
	struct pn544_dev *dev = pn544_dev;	
	//ztemt end
	struct i2c_client *client = dev->client;
	unsigned int val;
	int r = 0;

#ifdef NXP_PN544_DEBUG
	dev_dbg(&client->dev, "%s: cmd: 0x%x,PN544_SET_PWR:0x%x\n", __func__, cmd, PN544_SET_PWR);
#endif
	switch (cmd) {
		case PN544_SET_PWR:
			val = arg;
#ifdef NXP_PN544_DEBUG
			dev_dbg(&client->dev, "%s:  PN544_SET_PWR: %d\n", __func__, val);
#endif
			switch (val) {
				case 0: // power off
					pn544_dev_disable(dev);
					break;
				case 1: // power on
					r = pn544_dev_enable(dev, HCI_MODE);
					if (r < 0)
						goto out;
					break;
				case 2: // reset and power on with firmware download enabled
					r = pn544_dev_enable(dev, FW_MODE);
					if (r < 0)
						goto out;
					break;
				default:
					r = -ENOIOCTLCMD;
					goto out;
					break;
			}
			break;
		default:
			dev_err(&client->dev, "%s bad ioctl %u\n", __func__, cmd);
			return -EINVAL;
	}
out:
	return r;
}

static const struct file_operations pn544_dev_fops = {
	.owner	= THIS_MODULE,
	.llseek	= no_llseek,
	.read	= pn544_dev_read,
	.write	= pn544_dev_write,
	.open	= pn544_dev_open,
	.unlocked_ioctl = pn544_dev_ioctl,
};

#ifdef NXP_PN544_STANDBY_MODE
static void set_standby_mode(struct pn544_dev *pn544dev)
{
	char standby_commands[] = {0x09, 0x9B, 0x82, 0x3F, 0x00, 0x9E, 0xAA, 0x01, 0x9B, 0x03};
	int ret = -1;

	if(pn544dev == NULL){
		printk(KERN_ERR "[%s]dev is NULL.\n", __func__);
		return;
	}
	
	//Reset and write standy mode commands
	ret = pn544_dev_enable(pn544dev, HCI_MODE);
	if (ret < 0){
		printk(KERN_ERR "[%s]enable nfc failed.\n", __func__);
		return;
	}
	
	pn544_dev_disable(pn544dev);

	ret = pn544_dev_enable(pn544dev, HCI_MODE);
	if (ret < 0){
		printk(KERN_ERR "[%s]enable nfc failed.\n", __func__);
		return;
	}

	ret = i2c_master_send(pn544dev->client , standby_commands, sizeof(standby_commands));
	if (ret != sizeof(standby_commands)) {
		printk(KERN_ERR "[%s]i2c_master_send failed. returned %d\n", __func__, ret);
		return;
	}
	pn544_dev_disable(pn544dev);
	
    printk(KERN_DEBUG "[%s]Reset and write standby commands successfully.write bytes:%d\n", __func__, ret);
}
#endif

static __devinit int pn544_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret;
	struct pn544_nfc_platform_data *pdata;

	dev_dbg(&client->dev, "IRQ: %d\n", client->irq);
	
    //pr_err("[%s]IRQ: %d\n", __func__, client->irq);

	if (pn544_dev != NULL) {
		dev_warn(&client->dev, "only one PN544 supported.\n");
		return -EBUSY;
	}

	pdata = client->dev.platform_data;

	if (pdata == NULL) {
		dev_err(&client->dev, "%s : nfc probe fail\n", __func__);
		return  -ENODEV;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "%s : need I2C_FUNC_I2C\n", __func__);
		return  -ENODEV;
	}
	
	pn544_dev = kzalloc(sizeof(struct pn544_dev), GFP_KERNEL);
	if (pn544_dev == NULL) {
		dev_err(&client->dev, "failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	pn544_dev->client = client;

	/* init mutex and queues */
	pn544_dev->read_irq = PN544_NONE;
	init_waitqueue_head(&pn544_dev->read_wq);
	mutex_init(&pn544_dev->read_mutex);
	spin_lock_init(&pn544_dev->irq_enabled_lock);
	
	i2c_set_clientdata(client, pn544_dev);

	if (!pdata->request_resources) {
		dev_err(&client->dev, "request_resources() missing\n");
		ret = -EINVAL;
		goto err_request_resources;
	}

	ret = pdata->request_resources(client);
	if (ret) {
		dev_err(&client->dev, "Cannot get platform resources\n");
		goto err_request_resources;
	}

	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	pn544_dev->irq_enabled = true;
	ret = request_irq(client->irq, pn544_dev_irq_handler,
			IRQF_TRIGGER_HIGH, client->name, pn544_dev);
	if (ret) {
		dev_err(&client->dev, "request_irq failed\n");
		goto err_request_irq;
	}
	pn544_disable_irq(pn544_dev);

	pn544_dev->miscdev.minor = MISC_DYNAMIC_MINOR;
	pn544_dev->miscdev.name = PN544_DRIVER_NAME;
	pn544_dev->miscdev.fops = &pn544_dev_fops;
	pn544_dev->miscdev.parent = &client->dev;
	ret = misc_register(&pn544_dev->miscdev);
	if (ret) {
		dev_err(&client->dev, "%s : misc_register failed\n", __func__);
		goto err_misc_register;
	}

	dev_dbg(&client->dev, "%s: dev: %p, pdata %p, client %p\n",
		__func__, pn544_dev, pdata, client);

#ifdef NXP_PN544_STANDBY_MODE
	set_standby_mode(pn544_dev);
#endif

	return 0;

err_misc_register:
	free_irq(client->irq, pn544_dev);
err_request_irq:
	if (pdata->free_resources)
		pdata->free_resources();
err_request_resources:
	mutex_destroy(&pn544_dev->read_mutex);
	kfree(pn544_dev);
err_exit:
	return ret;
}

static __devexit int pn544_remove(struct i2c_client *client)
{
	struct pn544_dev *dev = i2c_get_clientdata(client);
	struct pn544_nfc_platform_data *pdata = client->dev.platform_data;

	dev_dbg(&client->dev, "%s\n", __func__);

	misc_deregister(&dev->miscdev);
	if (pdata->disable)
		pdata->disable();
	dev->read_irq = PN544_NONE;
	free_irq(client->irq, dev);
	if (pdata->free_resources)
		pdata->free_resources();
	mutex_destroy(&dev->read_mutex);
	kfree(dev);

	pn544_dev = NULL;
	
	return 0;
}

static const struct i2c_device_id pn544_id[] = {
	{ PN544_DRIVER_NAME, 0 },
	{ }
};

static struct i2c_driver pn544_driver = {
	.id_table	= pn544_id,
	.probe		= pn544_probe,
	.remove		= pn544_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= PN544_DRIVER_NAME,
	},
};

/*
 * module load/unload record keeping
 */

static int __init pn544_dev_init(void)
{
	int ret;

	pr_debug(DRIVER_DESC ": %s\n", __func__);

	ret = i2c_add_driver(&pn544_driver);
	if (ret != 0) {
		pr_err(PN544_DRIVER_NAME ": driver registration failed\n");
		return ret;
	}
	return 0;
}
module_init(pn544_dev_init);

static void __exit pn544_dev_exit(void)
{
	i2c_del_driver(&pn544_driver);
	pr_debug(DRIVER_DESC ", Exiting.\n");
}
module_exit(pn544_dev_exit);

MODULE_AUTHOR("Sylvain Fonteneau");
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

