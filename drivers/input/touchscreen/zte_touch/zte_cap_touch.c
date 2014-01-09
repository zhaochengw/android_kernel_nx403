/******************************************************************************

  Copyright (C), 2001-2011, ZTE. Co., Ltd.

******************************************************************************/

#include <linux/input/zte_cap_touch.h>
#include <linux/regulator/consumer.h>

#define GET_I2C_SDA_VAL	1	//show SDA value
#define CLR_I2C_SDA_VAL		2	//set SDA value to 0
#define SET_I2C_SDA_VAL	3	//set SDA value to 1
#define SET_I2C_SDA_I2C		4	//set SDA as i2c SDA port

#define GET_I2C_SCL_VAL		5	//show SCL value
#define CLR_I2C_SCL_VAL		6	//set SCL value to 0
#define SET_I2C_SCL_VAL		7	//set SCL value to 1
#define SET_I2C_SCL_I2C		8	//set SCL as i2c SCL port

#define GET_TP_INT_VAL		9	//show int value
#define GET_TP_RST_VAL		10	//show reset value
#define GET_TP_CS_VAL		11	//show cs value

#define ZTE_ANTI_CHARGER_NOISE

#ifdef CONFIG_HAS_EARLYSUSPEND
static void zte_touch_early_suspend(struct early_suspend *h);
static void zte_touch_late_resume(struct early_suspend *h);
#endif 

struct zte_ctp_info *zte_touch_info = NULL;
struct zte_ctp_dev *zte_ctp = NULL;
static struct kobject *zte_touch_board_attrs_kobj = NULL;
#ifdef ZTE_ANTI_CHARGER_NOISE
static int zte_charger_type = 0;
#endif

int zte_touch_i2c_read(char *reg_addr,char *buf, int size)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = zte_ctp->client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = reg_addr,
		 },
		{
		 .addr = zte_ctp->client->addr,
		 .flags = I2C_M_RD,
		 .len = size,
		 .buf = buf,
		 },
	};

	if(!buf)
		return 0;

	if (i2c_transfer(zte_ctp->client->adapter, msgs, 2) < 0) {
		return -EIO;
	} else
		return 0;
}

int zte_touch_i2c_write(char *buf,int size)
{
	struct i2c_msg msg[] = {
		{
		 .addr = zte_ctp->client->addr,
		 .flags = 0,
		 .len = size,
		 .buf = buf,
		 },
	};
	if (i2c_transfer(zte_ctp->client->adapter, msg, 1) < 0) {
		return -EIO;
	} else
		return 0;
}

static ssize_t zte_touch_virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int i,j;
	struct ts_i2c_platform_data *pdata = zte_ctp->pdata;

	for(i=0, j=0; i < pdata->nvitualkeys; i++) {
		j+=sprintf(buf, "%s%s:%d:%d:%d:%d:%d%s", buf,
					__stringify(EV_KEY), pdata->zte_virtual_key[i].keycode,
           				pdata->zte_virtual_key[i].x, pdata->zte_virtual_key[i].y,
           				pdata->zte_virtual_key[i].x_width, pdata->zte_virtual_key[i].y_width,
					(i == pdata->nvitualkeys - 1 ? "\n":":"));
	}

	return j;
}

static struct kobj_attribute zte_touch_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.zte_cap_touchscreen",
		.mode = S_IRUGO,
	},
	.show = &zte_touch_virtual_keys_show,
};

static struct attribute *zte_touch_virtual_keys_properties_attrs[] = {
	&zte_touch_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group zte_touch_virtual_keys_properties_attrs_group = {
	.attrs = zte_touch_virtual_keys_properties_attrs,
};

static ssize_t zte_touch_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (zte_touch_info->ts_name == NULL) {
		return snprintf(buf, PAGE_SIZE, "%s\n", "no device");
	}
	return snprintf(buf, PAGE_SIZE, "ZTE TOUCH IC: %s\nI2C ADDRESS: 0x%x\nVENDOR: %s\nFirmWare Ver: 0x%X\nRegInit Ver: 0x%X\n", \
		zte_touch_info->ts_name, zte_ctp->client->addr, zte_touch_info->vendor_name, zte_touch_info->firmwareVer, zte_touch_info->reginitVer);
}

static ssize_t zte_touch_dbg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, 
		"write 1 to enable ctp dbg,for example: echo 1 > ctp_dbg\n write 0 to disable ctp dbg,for example: echo 0 > ctp_dbg\n");
}

static ssize_t zte_touch_dbg_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)			
{
	unsigned int enable;
	
	sscanf(buf,"%d\n",&enable);
	zte_touch_enable_dbg(enable);
	
	return count;
}

static int zte_touch_atoi(const char *name)
{
	int val = 0;

	for (;; name++) {
		switch (*name) {
		case '0' ... '9':
			val = 10*val+(*name-'0');
			break;
		default:
			return val;
		}
	}
}

static ssize_t zte_touch_io_ctrl(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    
	int int_cmd = zte_touch_atoi(buf);
	struct zte_tp_intf tp_interface = zte_ctp->pdata->zte_tp_interface;

	u32 tp_sda = tp_interface.sda;
	u32 tp_scl = tp_interface.scl;
	u32 tp_int = tp_interface.irq;
	u32 tp_rst = tp_interface.reset;
	u32 tp_cs = tp_interface.cs;

	switch(int_cmd) {
	case GET_I2C_SDA_VAL:
		printk(KERN_INFO "touch: cmd is show SDA value");
		printk(KERN_INFO "touch: SDA = %d", gpio_get_value(tp_sda));
		break;
	case CLR_I2C_SDA_VAL:
		printk(KERN_INFO "touch: cmd is set SDA to 0");
		gpio_tlmm_config(GPIO_CFG(tp_sda, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		gpio_set_value(tp_sda, 0);
		printk(KERN_INFO "touch: SDA = %d", gpio_get_value(tp_sda));
		break;
	case SET_I2C_SDA_VAL:
		printk(KERN_INFO "touch: cmd is set SDA to 1");
		gpio_tlmm_config(GPIO_CFG(tp_sda, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		gpio_set_value(tp_sda, 1);
		printk(KERN_INFO "touch: SDA = %d", gpio_get_value(tp_sda));
		break;
	case SET_I2C_SDA_I2C:
		printk(KERN_INFO "touch: cmd is set SDA to i2c");
		gpio_tlmm_config(GPIO_CFG(tp_sda, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		printk(KERN_INFO "touch: SDA = %d", gpio_get_value(tp_sda));
		break;
	case GET_I2C_SCL_VAL:
		printk(KERN_INFO "touch: cmd is show SCL value");
		printk(KERN_INFO "touch: SCL = %d", gpio_get_value(tp_scl));
		break;
	case CLR_I2C_SCL_VAL:
		printk(KERN_INFO "touch: cmd is set SCL to 0");
		gpio_tlmm_config(GPIO_CFG(tp_scl, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		gpio_set_value(tp_scl, 0);
		printk(KERN_INFO "touch: SCL = %d", gpio_get_value(tp_scl));
		break;
	case SET_I2C_SCL_VAL:
		printk(KERN_INFO "touch: cmd is set SCL to 1");
		gpio_tlmm_config(GPIO_CFG(tp_scl, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		gpio_set_value(tp_scl, 1);
		printk(KERN_INFO "touch: SCL = %d", gpio_get_value(tp_scl));
		break;
	case SET_I2C_SCL_I2C:
		printk(KERN_INFO "touch: cmd is set SCL to i2c");
		gpio_tlmm_config(GPIO_CFG(tp_scl, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		printk(KERN_INFO "touch: SCL = %d", gpio_get_value(tp_scl));
		break;
	case GET_TP_INT_VAL:
		printk(KERN_INFO "touch: cmd is show INT value");
		printk(KERN_INFO "touch: INT = %d", gpio_get_value(tp_int));
		break;
	case GET_TP_RST_VAL:
		printk(KERN_INFO "touch: cmd is show RST value");
		printk(KERN_INFO "touch: RST = %d", gpio_get_value(tp_rst));
		break;
	case GET_TP_CS_VAL:
		printk(KERN_INFO "touch: cmd is show CS value");
		printk(KERN_INFO "touch: CS = %d", gpio_get_value(tp_cs));
		break;
	default:
		printk(KERN_INFO "touch: cmd unknown");
		break;
    }
    
    return count;
}

static DEVICE_ATTR(zte_touch_info, S_IRUGO | S_IWUSR, zte_touch_info_show, NULL);
static DEVICE_ATTR(zte_touch_dbg, S_IRUGO | S_IWUSR, zte_touch_dbg_show, zte_touch_dbg_store);
static DEVICE_ATTR(zte_touch_io_ctrl, S_IRUGO | S_IWUSR, NULL, zte_touch_io_ctrl);

static int zte_touch_virtual_keys_init(void)
{
	int ret;
	zte_touch_board_attrs_kobj = kobject_create_and_add("board_properties", NULL);
	if(zte_touch_board_attrs_kobj == NULL){
		ret = -ENOMEM;
		goto err_obj;
	}

	ret = sysfs_create_group(zte_touch_board_attrs_kobj, &zte_touch_virtual_keys_properties_attrs_group);
	if(ret){
		pr_err("Failed to create sys file.\n");
		goto err_file;
	}
	return 0;
err_file:
	kobject_del(zte_touch_board_attrs_kobj);
err_obj:
	return ret;	
}

static int zte_touch_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	static struct ts_i2c_platform_data *zte_ctp_pdata = NULL;

	zte_ctp_pdata = client->dev.platform_data;

	if (!zte_ctp_pdata) {
		dev_err(&client->dev, "platform data is required!\n");
		return -EINVAL;
	}
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA)) {
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -EIO;
	}
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "need I2C_FUNC_I2C\n");
		ret = -ENODEV;
	}
		
	zte_ctp = kzalloc(sizeof(struct zte_ctp_dev), GFP_KERNEL);
	if (!zte_ctp) {
		dev_err(&client->dev, "allocate zte_ctp failed\n");
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	zte_ctp->client = client;
	i2c_set_clientdata(client, zte_ctp);
	zte_ctp->pdata = zte_ctp_pdata;

	if (zte_ctp_pdata->init_tp_hw) {
		zte_ctp_pdata->init_tp_hw();
	}

	if (zte_ctp_pdata->power_on) {
		zte_ctp_pdata->power_on(client,true);
	}

	zte_ctp->ts_wq = create_singlethread_workqueue("touchscreen_wq");
	
	mutex_init(&zte_ctp->ts_lock);

	ret = sysfs_create_file(&client->dev.kobj,&dev_attr_zte_touch_info.attr);
	if (ret) {
		pr_err("create sysfs touch_info error!!\n");
		goto err_syscreate_file_failed;
	}

	ret = sysfs_create_file(&client->dev.kobj,&dev_attr_zte_touch_dbg.attr);
	if (ret) {
		pr_err("create sysfs dbg error!!\n");
		goto err_syscreate_file_failed;
	}

	ret = sysfs_create_file(&client->dev.kobj, &dev_attr_zte_touch_io_ctrl.attr);
	if (ret) {
		pr_err("create sysfs io_ctrl error!!\n");
	}
	
	return ret;
	
err_syscreate_file_failed:
	kfree(zte_ctp);
err_alloc_data_failed:
	return ret;
}

static int zte_touch_remove(struct i2c_client *client)
{
	struct zte_ctp_dev *ts = i2c_get_clientdata(client);
	
	free_irq(client->irq, ts);
	input_unregister_device(ts->input_dev);

	if (zte_ctp->pdata->exit_tp_hw)
			zte_ctp->pdata->exit_tp_hw();

	if (zte_ctp->pdata->power_on) {
		zte_ctp->pdata->power_on(client,false);
	}
	
	kfree(ts);

	return 0;
}

static int zte_touch_suspend(struct zte_ctp_dev *ts)
{
	disable_irq_nosync(gpio_to_irq(ts->pdata->zte_tp_interface.irq));
	
	if(zte_touch_info && zte_touch_info->ops->sleep) {
		zte_touch_info->ops->sleep(ts, true);
	}
	return 0;
}

static int zte_touch_resume(struct zte_ctp_dev *ts)
{

	if(zte_touch_info && zte_touch_info->ops->sleep) {
		zte_touch_info->ops->sleep(ts,false);
	}

	enable_irq(gpio_to_irq(ts->pdata->zte_tp_interface.irq));

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void zte_touch_early_suspend(struct early_suspend *h)
{
	struct zte_ctp_dev *ts = NULL;
	
	ts = container_of(h, struct zte_ctp_dev, early_suspend);
	zte_touch_suspend(ts);
}


static void zte_touch_late_resume(struct early_suspend *h)
{
	struct zte_ctp_dev *ts = NULL;

	ts = container_of(h, struct zte_ctp_dev, early_suspend);
	zte_touch_resume(ts);
}
#endif

#ifdef ZTE_ANTI_CHARGER_NOISE
static void zte_touch_anti_charger_work(struct work_struct *work)
{
	zte_touch_info->ops->tp_anti_charger(zte_ctp, zte_charger_type);
}

int zte_touch_change_charger_cfg(int charger_type)
{	
	if(!zte_touch_info || !zte_touch_info->ops->tp_anti_charger)
		return 0;
	zte_charger_type = charger_type;
	queue_work(zte_ctp->ts_wq, &zte_ctp->anti_work);

	return 0;
}
EXPORT_SYMBOL(zte_touch_change_charger_cfg);
#endif

int __init zte_touch_register(struct zte_ctp_info *ts_info)
{
	int i;
	int ret = -1;
	int irq;
	int try_time = 2;
	struct zte_tp_para tp_parameter;

	if (!ts_info) {
		TP_ERR("invalid ts_info\n");
		return -EINVAL;
	}

	mutex_lock(&zte_ctp->ts_lock);
	
	if (zte_touch_info) {
		TP_ERR("ts already exist\n");
		mutex_unlock(&zte_ctp->ts_lock);
		return -EEXIST;
	}

	if (ts_info->i2c_addr) {
		zte_ctp->client->addr = ts_info->i2c_addr;
	}else {
		TP_ERR("i2c_addr not defined\n");
		mutex_unlock(&zte_ctp->ts_lock);
		return -EINVAL;
	}

	while(try_time){
		if (ts_info->ops->isActive){
			ret = ts_info->ops->isActive(zte_ctp);
			if (!ret)
				break;
		}
		try_time--;
	}
	if (ret){
		TP_ERR("ctp not exist\n");
		mutex_unlock(&zte_ctp->ts_lock);
		return  -ENODEV;
	}
	
	zte_touch_info = ts_info;
	zte_touch_set_status(CTP_ONLINE);

	mutex_unlock(&zte_ctp->ts_lock);

	if (ts_info->ops->init_ts){
		ret = ts_info->ops->init_ts(zte_ctp);
		if (ret < 0){
			TP_ERR("Failed to init touchscrren!\n");
			return  -ENODEV;
		}
	}
	
	zte_ctp->input_dev = input_allocate_device();
	if (zte_ctp->input_dev == NULL) {
		ret = -ENOMEM;
		TP_ERR("Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}

	zte_ctp->input_dev->name = ZTE_CTP_NAME;

	set_bit(EV_SYN, zte_ctp->input_dev->evbit);
	set_bit(EV_KEY, zte_ctp->input_dev->evbit);
	set_bit(EV_ABS, zte_ctp->input_dev->evbit);
	set_bit(BTN_TOUCH, zte_ctp->input_dev->keybit);

	for(i=0; i < zte_ctp->pdata->nvitualkeys; i++) {
		set_bit(zte_ctp->pdata->zte_virtual_key[i].keycode, zte_ctp->input_dev->keybit);
	}

	tp_parameter = zte_ctp->pdata->zte_tp_parameter;

	/* Single touch */
	input_set_abs_params(zte_ctp->input_dev, ABS_X, tp_parameter.dis_min_x, tp_parameter.dis_max_x, 0, 0);
	input_set_abs_params(zte_ctp->input_dev, ABS_Y, tp_parameter.dis_min_y, tp_parameter.dis_max_y, 0, 0);
	input_set_abs_params(zte_ctp->input_dev, ABS_PRESSURE, tp_parameter.min_touch, tp_parameter.max_touch, 0, 0);
	input_set_abs_params(zte_ctp->input_dev, ABS_TOOL_WIDTH, tp_parameter.min_width, tp_parameter.max_width, 0, 0);

	/* Multitouch */
	input_set_abs_params(zte_ctp->input_dev, ABS_MT_POSITION_X, tp_parameter.dis_min_x, tp_parameter.dis_max_x, 0, 0);
	input_set_abs_params(zte_ctp->input_dev, ABS_MT_POSITION_Y, tp_parameter.dis_min_y, tp_parameter.dis_max_y, 0, 0);
	input_set_abs_params(zte_ctp->input_dev, ABS_MT_TOUCH_MAJOR, tp_parameter.min_touch, tp_parameter.max_touch, 0, 0);
	input_set_abs_params(zte_ctp->input_dev, ABS_MT_WIDTH_MAJOR, tp_parameter.min_width, tp_parameter.max_width, 0, 0);
	input_set_abs_params(zte_ctp->input_dev, ABS_MT_TRACKING_ID, tp_parameter.min_tid, tp_parameter.max_tid, 0, 0);

	ret = input_register_device(zte_ctp->input_dev);
	if (ret) {
		TP_ERR("Unable to register %s input device\n", zte_ctp->input_dev->name);
		goto err_input_register_device_failed;
	}

	zte_touch_virtual_keys_init();
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	zte_ctp->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	zte_ctp->early_suspend.suspend = zte_touch_early_suspend;
	zte_ctp->early_suspend.resume = zte_touch_late_resume;
	register_early_suspend(&zte_ctp->early_suspend);
#endif

	irq = gpio_to_irq(zte_ctp->pdata->zte_tp_interface.irq);

	INIT_WORK(&zte_ctp->ts_work, ts_info->ops->work_func);
#ifdef ZTE_ANTI_CHARGER_NOISE
	INIT_WORK(&zte_ctp->anti_work, zte_touch_anti_charger_work);
#endif
	
	ret = request_irq(irq,ts_info->ops->irq_handler, ts_info->irqflags, \
		zte_ctp->input_dev->name, zte_ctp);
	if (ret != 0) {
		TP_ERR("request_irq failed\n");
		goto err_request_irq_failed;
	}

	if (ts_info->ops->post_init){
		ret = ts_info->ops->post_init(zte_ctp);
		if (ret < 0){
			TP_ERR("Failed to post init touchscrren!\n");
			goto err_post_init;
		}
	}
	
	return 0;

err_post_init:
	free_irq(irq, zte_ctp);
err_request_irq_failed:
	input_unregister_device(zte_ctp->input_dev);
err_input_register_device_failed:
	input_free_device(zte_ctp->input_dev);
err_input_dev_alloc_failed:
	return ret;
}


static const struct i2c_device_id zte_touch_i2c_id[] = {
	{ ZTE_CTP_NAME, 0 },
};

static struct i2c_driver zte_touch_driver = {
	.id_table	= zte_touch_i2c_id,
	.probe	= zte_touch_probe,
	.remove	= zte_touch_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= zte_touch_suspend,
	.resume 	= zte_touch_resume,
#endif 
	.driver = {
		.name	= ZTE_CTP_NAME,
	},
};

static int __init zte_touch_init(void)
{
	return i2c_add_driver(&zte_touch_driver);
}

static void __exit zte_touch_exit(void)
{
	i2c_del_driver(&zte_touch_driver);
}

module_init(zte_touch_init);
module_exit(zte_touch_exit);

MODULE_DESCRIPTION("touchscreen driver ZTEMT");
MODULE_LICENSE("GPL");

