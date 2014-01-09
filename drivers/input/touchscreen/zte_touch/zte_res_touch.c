/******************************************************************************

  Copyright (C), 2001-2011, ZTE. Co., Ltd.

 ******************************************************************************
  File Name     : zte_res_touch.c
  Version       : Initial Draft
  Author        : LiuYongfeng
  Created       : 2011/10/13
  Last Modified :
  Function List :
              sys_init
              ts_exit
              ts_init
              ts_interrupt
              ts_probe
              ts_remove
              ts_timer
              ts_update_pen_state
              virtual_keys_show
  History       :
  1.Date        : 2011/10/13
    Author      : LiuYongfeng
    Modification: Created file

******************************************************************************/
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/earlysuspend.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/jiffies.h>
#include <linux/io.h>

#include <linux/input/zte_res_touch.h>

/* HW register map */
#define TSSC_CTL_REG      0x100
#define TSSC_SI_REG       0x108
#define TSSC_OPN_REG      0x104
#define TSSC_STATUS_REG   0x10C
#define TSSC_AVG12_REG    0x110

/* status bits */
#define TSSC_STS_OPN_SHIFT 0x6
#define TSSC_STS_OPN_BMSK  0x1C0
#define TSSC_STS_NUMSAMP_SHFT 0x1
#define TSSC_STS_NUMSAMP_BMSK 0x3E

/* CTL bits */
#define TSSC_CTL_EN		(0x1 << 0)
#define TSSC_CTL_SW_RESET	(0x1 << 2)
#define TSSC_CTL_MASTER_MODE	(0x3 << 3)
#define TSSC_CTL_AVG_EN		(0x1 << 5)
#define TSSC_CTL_DEB_EN		(0x1 << 6)
#define TSSC_CTL_DEB_12_MS	(0x2 << 7)	/* 1.2 ms */
#define TSSC_CTL_DEB_16_MS	(0x3 << 7)	/* 1.6 ms */
#define TSSC_CTL_DEB_2_MS	(0x4 << 7)	/* 2 ms */
#define TSSC_CTL_DEB_3_MS	(0x5 << 7)	/* 3 ms */
#define TSSC_CTL_DEB_4_MS	(0x6 << 7)	/* 4 ms */
#define TSSC_CTL_DEB_6_MS	(0x7 << 7)	/* 6 ms */
#define TSSC_CTL_INTR_FLAG1	(0x1 << 10)
#define TSSC_CTL_DATA		(0x1 << 11)
#define TSSC_CTL_SSBI_CTRL_EN	(0x1 << 13)

/* control reg's default state */
#define TSSC_CTL_STATE	  ( \
		TSSC_CTL_DEB_12_MS | \
		TSSC_CTL_DEB_EN | \
		TSSC_CTL_AVG_EN | \
		TSSC_CTL_MASTER_MODE | \
		TSSC_CTL_EN)

#define TSSC_NUMBER_OF_OPERATIONS 2
#define TS_PENUP_TIMEOUT_MS 20

struct ts {
	struct input_dev *input;
	struct timer_list timer;
	struct msm_tssc_platform_data *pdata;
#ifdef CONFIG_HAS_EARLYSUSPEND
/* ZTEMT Added by xuqi, 2011/12/2   PN: */
  struct early_suspend early_suspend;
/*ZTEMT END */
#endif 

	int irq;
};
/* ZTEMT Added by xuqi, 2011/12/2   PN: */
static void msm_touch_early_suspend(struct early_suspend *h);
static void msm_touch_late_resume(struct early_suspend *h);
/*ZTEMT END */

static struct ts *ts;

static void __iomem *virt;
#define TSSC_REG(reg) (virt + TSSC_##reg##_REG)

static struct kobject* virtual_keys_kobj = NULL;
static ssize_t virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct msm_tssc_platform_data *pdata = ts->pdata;

	/*four virtual keys,with search key*/
	if (pdata->num_virtual_keys == FourVirtualKeys) {
		return sprintf(buf,
				__stringify(EV_KEY)":"__stringify(KEY_HOME)":%d:%d:%d:%d" ":"
				__stringify(EV_KEY)":"__stringify(KEY_BACK)":%d:%d:%d:%d" ":"
				__stringify(EV_KEY)":"__stringify(KEY_MENU)":%d:%d:%d:%d" ":"
				__stringify(EV_KEY)":"__stringify(KEY_SEARCH)":%d:%d:%d:%d" "\n",\
				pdata->home.x,pdata->home.y,pdata->home.x_width,pdata->home.y_width,\
				pdata->back.x,pdata->back.y,pdata->back.x_width,pdata->back.y_width,\
				pdata->menu.x,pdata->menu.y,pdata->menu.x_width,pdata->menu.y_width,\
				pdata->search.x,pdata->search.y,pdata->search.x_width,pdata->search.y_width
			);
	} else if (pdata->num_virtual_keys == ThreeVirtualKeys) {
		/*only three virtual keys,without search key*/
		return sprintf(buf,
				__stringify(EV_KEY)":"__stringify(KEY_HOME)":%d:%d:%d:%d" ":"
				__stringify(EV_KEY)":"__stringify(KEY_BACK)":%d:%d:%d:%d" ":"
				__stringify(EV_KEY)":"__stringify(KEY_MENU)":%d:%d:%d:%d" "\n",\
				pdata->home.x,pdata->home.y,pdata->home.x_width,pdata->home.y_width,\
				pdata->back.x,pdata->back.y,pdata->back.x_width,pdata->back.y_width,\
				pdata->menu.x,pdata->menu.y,pdata->menu.x_width,pdata->menu.y_width
			);
	} else { //TwoVirtualKeys
		return sprintf(buf,
				__stringify(EV_KEY)":"__stringify(KEY_BACK)":%d:%d:%d:%d" ":"
				__stringify(EV_KEY)":"__stringify(KEY_MENU)":%d:%d:%d:%d" "\n",\
				pdata->back.x,pdata->back.y,pdata->back.x_width,pdata->back.y_width,\
				pdata->menu.x,pdata->menu.y,pdata->menu.x_width,pdata->menu.y_width
			);
	}
}

static struct kobj_attribute virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.zte_res_touchscreen",
		.mode = S_IRUGO,
	},
	.show = &virtual_keys_show,
};

static struct attribute *virtual_keys_properties_attrs[] = {
	&virtual_keys_attr.attr,
	NULL
};

static struct attribute_group virtual_keys_properties_attrs_group = {
	.attrs = virtual_keys_properties_attrs,
};

static int sys_init(void)
{
	int ret;
	virtual_keys_kobj = kobject_create_and_add("board_properties", NULL);
	if(virtual_keys_kobj == NULL){
		ret = -ENOMEM;
		goto err_obj;
	}

	ret = sysfs_create_group(virtual_keys_kobj, &virtual_keys_properties_attrs_group);
	if(ret){
		pr_err("Failed to create sys file.\n");
		goto err_file;
	}
	return 0;

err_file:
	kobject_del(virtual_keys_kobj);
err_obj:
	return ret;	
}


static ssize_t
show_TSSC_REG(struct device *dev, struct device_attribute *attr, char *buf)
{ 
	return snprintf(buf, PAGE_SIZE, "ctl:%x\nstatus:%x\navg12:%x\nopen:%x\nsample_int:%x\n",
        readl_relaxed(TSSC_REG(CTL)),
        readl_relaxed(TSSC_REG(STATUS)),readl_relaxed(TSSC_REG(AVG12)),
        readl_relaxed(TSSC_REG(OPN)),readl_relaxed(TSSC_REG(SI))); 
}

static ssize_t
set_TSSC_REG(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(TSSC_REG, S_IRUGO | S_IWUSR,
		show_TSSC_REG, set_TSSC_REG);
static struct attribute *dev_attrs[] = {
	&dev_attr_TSSC_REG.attr,
	NULL,
};
static struct attribute_group dev_attr_grp = {
	.attrs = dev_attrs,
};

static void ts_update_pen_state(struct ts *ts, int x, int y, int pressure)
{
	if (pressure) {
		input_report_abs(ts->input, ABS_X, x);
		input_report_abs(ts->input, ABS_Y, y);
		input_report_abs(ts->input, ABS_PRESSURE, pressure);
		input_report_key(ts->input, BTN_TOUCH, !!pressure);
	} else {
		input_report_abs(ts->input, ABS_PRESSURE, 0);
		input_report_key(ts->input, BTN_TOUCH, 0);
	}
	TP_DBG("x = %d, y = %d\n",x, y);
	input_sync(ts->input);
}

static void ts_timer(unsigned long arg)
{
	struct ts *ts = (struct ts *)arg;

	ts_update_pen_state(ts, 0, 0, 0);
}

static irqreturn_t ts_interrupt(int irq, void *dev_id)
{
	u32 avgs, x, y, lx, ly;
	u32 num_op, num_samp;
	u32 status;

	struct ts *ts = dev_id;
	struct msm_tssc_platform_data *pdata = ts->pdata;//yfliu
	
	status = readl_relaxed(TSSC_REG(STATUS));
	avgs = readl_relaxed(TSSC_REG(AVG12));
	x = avgs & 0xFFFF;
	y = avgs >> 16;

	TP_DBG("raw_x = %d, raw_y = %d\r\n",x, y);

	/* For pen down make sure that the data just read is still valid.
	 * The DATA bit will still be set if the ARM9 hasn't clobbered
	 * the TSSC. If it's not set, then it doesn't need to be cleared
	 * here, so just return.
	 */
	if (!(readl_relaxed(TSSC_REG(CTL)) & TSSC_CTL_DATA))
		goto out;

	/* Data has been read, OK to clear the data flag */
	writel_relaxed(TSSC_CTL_STATE, TSSC_REG(CTL));
	/* barrier: Write to complete before the next sample */
	mb();
	/* Valid samples are indicated by the sample number in the status
	 * register being the number of expected samples and the number of
	 * samples collected being zero (this check is due to ADC contention).
	 */
	num_op = (status & TSSC_STS_OPN_BMSK) >> TSSC_STS_OPN_SHIFT;
	num_samp = (status & TSSC_STS_NUMSAMP_BMSK) >> TSSC_STS_NUMSAMP_SHFT;

	if ((num_op == TSSC_NUMBER_OF_OPERATIONS) && (num_samp == 0)) {
		/* TSSC can do Z axis measurment, but driver doesn't support
		 * this yet.
		 */

		/*
		 * REMOVE THIS:
		 * These x, y co-ordinates adjustments will be removed once
		 * Android framework adds calibration framework.
		 */
		lx = x - pdata->x_min;	
	    ly = pdata->y_max - y;
		
		lx = lx * pdata->x_res / (pdata->x_max - pdata->x_min);
		ly = ly * pdata->y_res / (pdata->y_max - pdata->y_min);
		
		TP_DBG("%dx%d: X:(%d -> %d) Y:(%d->%d)\n",pdata->x_res,pdata->y_res,pdata->x_min,pdata->x_max,pdata->y_min,pdata->y_max);
		
		ts_update_pen_state(ts, lx, ly, 255);
		/* kick pen up timer - to make sure it expires again(!) */
		mod_timer(&ts->timer,
			jiffies + msecs_to_jiffies(TS_PENUP_TIMEOUT_MS));

	} else
		printk(KERN_INFO "Ignored interrupt: {%3d, %3d},"
				" op = %3d samp = %3d\n",
				 x, y, num_op, num_samp);

out:
	return IRQ_HANDLED;
}

void ts_power_off(void)
{
	writel(TSSC_CTL_STATE & ~TSSC_CTL_EN, TSSC_REG(CTL));
}

void ts_power_on(void)
{
	writel(TSSC_CTL_STATE, TSSC_REG(CTL));
}

static int msm_touch_suspend(struct ts *ts, pm_message_t mesg)
{
	disable_irq(ts->irq);
	ts_power_off();
	return 0;
}

static int msm_touch_resume(struct ts *ts)
{
	ts_power_on();
	enable_irq(ts->irq); 
	return 0;
}


static void msm_touch_early_suspend(struct early_suspend *h)
{
	struct ts *ts;
	ts = container_of(h, struct ts, early_suspend);
	msm_touch_suspend(ts, PMSG_SUSPEND);
}

static void msm_touch_late_resume(struct early_suspend *h)
{
	struct ts *ts;
	ts = container_of(h, struct ts, early_suspend);
	msm_touch_resume(ts);
}

static int __devinit ts_probe(struct platform_device *pdev)
{
	int result;
	struct input_dev *input_dev;
	struct resource *res, *ioarea;
	struct msm_tssc_platform_data *pdata = pdev->dev.platform_data;//yfliu

	/* The primary initialization of the TS Hardware
	 * is taken care of by the ADC code on the modem side
	 */

	/*if CTP exist just return, otherwise disconfig the ctp gpio and go ahead*/
	if (get_ctp_status() == CTP_ONLINE){
		dev_err(&pdev->dev, "CTP already exist, no need to regist the resistive touchscreen\n");
		result = -EINVAL;
		goto fail_get_pdata;
	}else{
		TP_INFO("No CTP detected,just regist the resistve touchscreen\n");	
	}
	
	if (!pdata){
		dev_err(&pdev->dev, "Platform data is invalid\n");
		result = -EINVAL;
		goto fail_get_pdata;
	}
	ts = kzalloc(sizeof(struct ts), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!input_dev || !ts) {
		result = -ENOMEM;
		goto fail_alloc_mem;
	}
	/*get the platform data*/
	ts->pdata = pdata;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
		result = -ENOENT;
		goto fail_alloc_mem;
	}

	ts->irq = platform_get_irq(pdev, 0);
	if (!ts->irq) {
		dev_err(&pdev->dev, "Could not get IORESOURCE_IRQ\n");
		result = -ENODEV;
		goto fail_alloc_mem;
	}

	ioarea = request_mem_region(res->start, resource_size(res), pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "Could not allocate io region\n");
		result = -EBUSY;
		goto fail_alloc_mem;
	}

	virt = ioremap(res->start, resource_size(res));
	if (!virt) {
		dev_err(&pdev->dev, "Could not ioremap region\n");
		result = -ENOMEM;
		goto fail_ioremap;
	}

	input_dev->name = ZTE_RTP_NAME;
	input_dev->phys = "msm_touch/input0";
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0002;
	input_dev->id.version = 0x0100;
	input_dev->dev.parent = &pdev->dev;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
	input_dev->absbit[BIT_WORD(ABS_MISC)] = BIT_MASK(ABS_MISC);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	
	if (pdata->num_virtual_keys == FourVirtualKeys) {
           input_dev->keybit[BIT_WORD(KEY_HOME)] = BIT_MASK(KEY_HOME);
           input_dev->keybit[BIT_WORD(KEY_MENU)] = BIT_MASK(KEY_MENU);
           input_dev->keybit[BIT_WORD(KEY_BACK)] = BIT_MASK(KEY_BACK);
	    input_dev->keybit[BIT_WORD(KEY_SEARCH)] = BIT_MASK(KEY_SEARCH);
	} else if (pdata->num_virtual_keys == ThreeVirtualKeys) {
	    input_dev->keybit[BIT_WORD(KEY_HOME)] = BIT_MASK(KEY_HOME);
           input_dev->keybit[BIT_WORD(KEY_MENU)] = BIT_MASK(KEY_MENU);
           input_dev->keybit[BIT_WORD(KEY_BACK)] = BIT_MASK(KEY_BACK);
	} else if (pdata->num_virtual_keys == TwoVirtualKeys) {
           input_dev->keybit[BIT_WORD(KEY_MENU)] = BIT_MASK(KEY_MENU);
           input_dev->keybit[BIT_WORD(KEY_BACK)] = BIT_MASK(KEY_BACK);
	}

    input_set_abs_params(input_dev, ABS_X, 0, pdata->x_res, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, pdata->y_res, 0, 0);

    input_set_abs_params(input_dev, ABS_PRESSURE, 0, pdata->pressure_max, 0, 0);
	
    result = input_register_device(input_dev);
	if (result)
		goto fail_ip_reg;

	ts->input = input_dev;

	setup_timer(&ts->timer, ts_timer, (unsigned long)ts);
	result = request_irq(ts->irq, ts_interrupt, IRQF_TRIGGER_RISING,
				 "touchscreen", ts);
	if (result)
		goto fail_req_irq;

#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level   = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = msm_touch_early_suspend;
	ts->early_suspend.resume  = msm_touch_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	platform_set_drvdata(pdev, ts); 
	sys_init();
    /*ztemt xu.xiaohua add 2011-11-9 15:54:48 
    for touchscreen attr **/
    result = sysfs_create_group(&pdev->dev.kobj, &dev_attr_grp);
    if (result)
		goto fail_req_irq;
    /* end modify */
	return 0;

fail_req_irq:
	input_unregister_device(input_dev);
	input_dev = NULL;
fail_ip_reg:
	iounmap(virt);
fail_ioremap:
	release_mem_region(res->start, resource_size(res));
fail_alloc_mem:
	input_free_device(input_dev);
	kfree(ts);
fail_get_pdata:
	return result;
}

static int __devexit ts_remove(struct platform_device *pdev)
{
	struct resource *res;
	struct ts *ts = platform_get_drvdata(pdev);

	free_irq(ts->irq, ts);
	del_timer_sync(&ts->timer);

	input_unregister_device(ts->input);
	iounmap(virt);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    /*ztemt xu.xiaohua add 2011-11-9 15:54:48 
    for touchscreen attr **/
    sysfs_remove_group(&pdev->dev.kobj, &dev_attr_grp);
    
	release_mem_region(res->start, resource_size(res));
	platform_set_drvdata(pdev, NULL);
	kfree(ts);
/*
sys/devices/platform/zte_res_touch/
sys/devices/zte_res_toutch/ ????

*/
	return 0;
}

static struct platform_driver ts_driver = {
	.probe		= ts_probe,
	.remove		= __devexit_p(ts_remove),
	.driver		= {
		.name = ZTE_RTP_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init ts_init(void)
{
	return platform_driver_register(&ts_driver);
}
module_init(ts_init);

static void __exit ts_exit(void)
{
	platform_driver_unregister(&ts_driver);
}
module_exit(ts_exit);

MODULE_DESCRIPTION("ZTEMT Touch Screen driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:zte_touchscreen");
