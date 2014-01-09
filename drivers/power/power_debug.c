/* Copyright (c) 2011-2012, ZTEMT. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 //if you want to control it, please echo 0/1 > /sys/zte_power_debug/switch, 
 // 0->close, 1-> open. now the default is ON, line 153.
#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/power_supply.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <../../arch/arm/mach-msm/clock.h>


#include "power_debug.h"

static struct power_debug_info power_debug_information;
static int power_debug_switch=1;
extern int msm_show_resume_irq_mask; //used to print the resume irq

static void print_battery_information(void)
{
	printk("BMS capacity=%d current=%d ibat_ma=%d vbat_ma=%d usb_in=%d\n",
			power_debug_information.battery_information.percent_of_battery,
			power_debug_information.battery_information.battery_current/1000,
			power_debug_information.battery_information.ibat_ua/1000,
			power_debug_information.battery_information.vbat_uv/1000,
			power_debug_information.battery_information.usb_chg_in);
}
static void update_battery_information(void)
{
	power_debug_information.battery_information.percent_of_battery = pm8921_bms_get_percent_charge();
	pm8921_bms_get_battery_current(&power_debug_information.battery_information.battery_current); 
	pm8921_bms_get_simultaneous_battery_voltage_and_current
		(&power_debug_information.battery_information.ibat_ua,&power_debug_information.battery_information.vbat_uv);
	power_debug_information.battery_information.dc_chg_in = pm8921_is_dc_chg_plugged_in();
	power_debug_information.battery_information.usb_chg_in = pm8921_is_usb_chg_plugged_in();
}

static void power_debug_work_func(struct work_struct *work)
{
	update_battery_information();
	printk("power_debug_work_func_______start!\n");
	//print wakelocks
	global_print_active_locks(WAKE_LOCK_SUSPEND);
	//wakelock_stats_show_debug();
	//print battery related information
	print_battery_information();
	hrtimer_start(&power_debug_information.timer,ktime_set(10,0),HRTIMER_MODE_REL);	
	printk("power_debug_work_func_________over!\n");

}

static enum hrtimer_restart power_debug_timer(struct hrtimer *timer)
{
	schedule_work(&power_debug_information.work);
	return HRTIMER_NORESTART;
}

static int power_debug_timer_control(int on)
{
	int ret;
	if(1==on)
	{
		if(1==power_debug_switch)
		{
			printk("The power_debu_timer is already on\n");
			ret=1;
		}
		else
		{
			ret=hrtimer_start(&power_debug_information.timer, ktime_set(5,0),HRTIMER_MODE_REL);
			power_debug_switch=1;
			msm_show_resume_irq_mask=1;
		}
	}
	else
	{

		if(0==power_debug_switch)
		{
			printk("The power_debu_timer is already off\n");
			ret=1;
		}
		else
		{
			ret=hrtimer_cancel(&power_debug_information.timer);
			power_debug_switch=0;
			msm_show_resume_irq_mask=0;
		}

	}
	return ret;
}


static ssize_t po_info_show(struct device *dev, 
		struct device_attribute *attr, char *buf)
{

	sprintf(buf, "%u\n", power_debug_switch);
	return 1;
}
static ssize_t po_info_store(struct device *dev, 
		struct device_attribute *attr, const char *buf, size_t count)
{

	unsigned int val;

	if (sscanf(buf, "%u", &val) == 1) {
		if (power_debug_timer_control(val))
			return count;
	}
	return -EINVAL;
        
}

static ssize_t clock_dump_show(struct device *dev, 
		struct device_attribute *attr, char *buf)
{

	sprintf(buf, "%u\n", power_debug_switch);
	clock_debug_print_enabled();
	return 1;
}

static DEVICE_ATTR(switch, 0644, po_info_show, po_info_store);
static DEVICE_ATTR(clock_dump, 0644,  clock_dump_show, NULL);
static struct kobject *po_kobject = NULL;


static int __init power_debug_init(void)
{
	int ret;

	INIT_WORK(&power_debug_information.work, power_debug_work_func);
	hrtimer_init(&power_debug_information.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	power_debug_information.timer.function = power_debug_timer;
	hrtimer_start(&power_debug_information.timer, ktime_set(5,0),HRTIMER_MODE_REL);

	po_kobject = kobject_create_and_add(DRV_NAME, NULL);
	if(po_kobject == NULL) {
		ret = -ENOMEM;
		goto err1;
	}

	ret = sysfs_create_file(po_kobject, &dev_attr_switch.attr);
	ret |= sysfs_create_file(po_kobject, &dev_attr_clock_dump.attr);
	if(ret){
		goto err;
	}
	msm_show_resume_irq_mask=1; //on in default, deleted is allow.

	return 0;

err:
	kobject_del(po_kobject);
err1:
	printk(DRV_NAME": Failed to create sys file\n");
	return ret;
}

static void __exit power_debug_exit(void)
{
	hrtimer_cancel(&power_debug_information.timer);
	kobject_del(po_kobject);
}

late_initcall(power_debug_init);
module_exit(power_debug_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ZTEMT power debug feature");
MODULE_VERSION("1.0");



