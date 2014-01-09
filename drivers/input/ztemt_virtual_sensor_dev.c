/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
 *
 * File Name          : lis3dh_acc.c
 * Authors            : MSH - Motion Mems BU - Application Team
 *		      : Matteo Dameno (matteo.dameno@st.com)
 *		      : Carmine Iascone (carmine.iascone@st.com)
 *                    : Samuel Huo (samuel.huo@st.com)
 * Version            : V.1.1.0
 * Date               : 07/10/2012
 * Description        : LIS3DH accelerometer sensor driver
 *
 *******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
 * OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
 * PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * THIS SOFTWARE IS SPECIFICALLY DESIGNED FOR EXCLUSIVE USE WITH ST PARTS.
 *
 ******************************************************************************
 Revision 1.0.0 14/06/2013
 ******************************************************************************/

#include	<linux/err.h>
#include	<linux/errno.h>
#include	<linux/delay.h>
#include	<linux/fs.h>
#include	<linux/i2c.h>
#include	<linux/input.h>
#include	<linux/uaccess.h>
#include	<linux/workqueue.h>
#include	<linux/irq.h>
#include	<linux/gpio.h>
#include	<linux/interrupt.h>
#include	<linux/slab.h>
#include	<linux/pm.h>
#include	<linux/input/ztemt_virtual_sensor_dev.h>
#include	<linux/module.h>
#include	<linux/regulator/consumer.h>

#define DEBUG_ON //DEBUG SWITCH
#define LOG_TAG "ZTEMT_VIRTUAL_SENSOR_DEV"
#ifdef  CONFIG_FEATURE_ZTEMT_SENSORS_LOG_ON
#define SENSOR_LOG_ERROR(fmt, args...) printk(KERN_ERR   "[%s] [%s: %d] "  fmt,\
                                              LOG_TAG,__FUNCTION__, __LINE__, ##args)
    #ifdef  DEBUG_ON
#define SENSOR_LOG_INFO(fmt, args...)  printk(KERN_INFO  "[%s] [%s: %d] "  fmt,\
                                              LOG_TAG,__FUNCTION__, __LINE__, ##args)
                                              
#define SENSOR_LOG_DEBUG(fmt, args...) printk(KERN_DEBUG "[%s] [%s: %d] "  fmt,\
                                              LOG_TAG,__FUNCTION__, __LINE__, ##args)
    #else
#define SENSOR_LOG_INFO(fmt, args...)
#define SENSOR_LOG_DEBUG(fmt, args...)
    #endif

#else
#define SENSOR_LOG_ERROR(fmt, args...)
#define SENSOR_LOG_INFO(fmt, args...)
#define SENSOR_LOG_DEBUG(fmt, args...)
#endif


static struct ztemt_virtual_sensor_dev_platform_data *pdata = NULL;


static int zte_sensors_power_control(struct i2c_client *client, struct ztemt_virtual_sensor_dev_platform_data *pdata, bool on)
{
	int rc = 0, i;
	struct regulator **vdd = NULL;
	const struct zte_sensors_power_regulator *reg_info = pdata->regulator_info;
	u8 num_reg = pdata->num_regulators;

    SENSOR_LOG_INFO("enter \n");
	if (!reg_info) 
    {
        SENSOR_LOG_INFO( "regulator pdata not specified\n");
		return -EINVAL;
	}
    
	if (false == on) /* Turn off the regulators */
    {
		goto sensors_reg_disable;
    }

	vdd = kzalloc(num_reg * sizeof(struct regulator *), GFP_KERNEL);
	if (!vdd) 
    {
        SENSOR_LOG_ERROR( "unable to allocate memory\n");
		return -ENOMEM;
	}
    
	for (i = 0; i < num_reg; i++)
    {
        SENSOR_LOG_INFO( "reg_info[i].name = %s\n",reg_info[i].name);
		vdd[i] = regulator_get(&client->dev, reg_info[i].name);
		if (IS_ERR(vdd[i])) 
        {
			rc = PTR_ERR(vdd[i]);
            SENSOR_LOG_ERROR( "%s: %d: Regulator get failed rc=%d\n",__func__,__LINE__,rc);
			goto error_vdd;
		}
        
		if (regulator_count_voltages(vdd[i]) > 0) 
        {
			rc = regulator_set_voltage(vdd[i], reg_info[i].min_uV, reg_info[i].max_uV);
			if (rc) {
                SENSOR_LOG_ERROR(" Regulator_set_voltage failed rc =%d\n",rc);
				regulator_put(vdd[i]);
				goto error_vdd;
			}
		}
        
		rc = regulator_set_optimum_mode(vdd[i], reg_info[i].hpm_load_uA);
		if (rc < 0) 
        {
            SENSOR_LOG_ERROR("Regulator_set_optimum_mode failed rc=%d\n",rc);
			regulator_set_voltage(vdd[i], 0, reg_info[i].max_uV);
			regulator_put(vdd[i]);
			goto error_vdd;
		}
        
		rc = regulator_enable(vdd[i]);
		if (rc) 
        {
            SENSOR_LOG_ERROR("Regulator_enable failed rc =%d\n",rc);
			regulator_set_optimum_mode(vdd[i], 0);
			regulator_set_voltage(vdd[i], 0, reg_info[i].max_uV);
			regulator_put(vdd[i]);
			goto error_vdd;
		}
	}
    SENSOR_LOG_INFO("exit\n");
	return rc;

sensors_reg_disable:
	i = pdata->num_regulators;
error_vdd:
	while (--i >= 0) 
    {
		if (regulator_count_voltages(vdd[i]) > 0)
        {
			regulator_set_voltage(vdd[i], 0, reg_info[i].max_uV);
		}
        regulator_set_optimum_mode(vdd[i], 0);
		regulator_disable(vdd[i]);
		regulator_put(vdd[i]);
	}
	kfree(vdd);
    SENSOR_LOG_ERROR("error!\n");
	return rc;
}


static int ztemt_virtual_sensor_dev_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = -1;

	SENSOR_LOG_INFO("probe start.\n");

    
    if (NULL != client->dev.platform_data)
    { 
        pdata = kzalloc(sizeof(struct ztemt_virtual_sensor_dev_platform_data), GFP_KERNEL);
        if (!pdata) 
        {
            SENSOR_LOG_ERROR( "unable to allocate memory\n");            
            goto prob_error;
		}
        memcpy(pdata, client->dev.platform_data, sizeof(struct ztemt_virtual_sensor_dev_platform_data));
    }
    else
    {
        SENSOR_LOG_INFO("platform data is NULL!\n");
        goto prob_error;
    }

    ret = zte_sensors_power_control(client, pdata, true);
    
    if (ret < 0)
    {
        SENSOR_LOG_INFO("zte_sensors_power_control failed!\n");
        kfree(pdata);
        pdata = NULL;
        goto prob_error;
    }
    else
    {
        SENSOR_LOG_INFO("zte_sensors_power_control success!\n");
    }

	SENSOR_LOG_INFO("probe sucess.\n");
    
    return 0;

prob_error:

	SENSOR_LOG_INFO("probe failed.\n");
    return -1;
}

static int __devexit ztemt_virtual_sensor_dev_remove(struct i2c_client *client)
{
	if (NULL!=pdata)
    {
	    kfree(pdata);
    }
	return 0;
}

static const struct i2c_device_id ztemt_virtual_sensor_dev_id[] =
{
    {ZTEMT_VIRTUAL_SENSOR_DEV_NAME, 0},
    {},
};

static struct i2c_driver ztemt_virtual_sensor_dev_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = ZTEMT_VIRTUAL_SENSOR_DEV_NAME,
		  },
	.probe = ztemt_virtual_sensor_dev_probe,
	.remove = __devexit_p(ztemt_virtual_sensor_dev_remove),
	.suspend = NULL,
	.resume = NULL,
	.id_table = ztemt_virtual_sensor_dev_id,
};

static int __init ztemt_virtual_sensor_dev_init(void)
{
	return i2c_add_driver(&ztemt_virtual_sensor_dev_driver);
}

static void __exit ztemt_virtual_sensor_dev_exit(void)
{
	i2c_del_driver(&ztemt_virtual_sensor_dev_driver);
}

module_init(ztemt_virtual_sensor_dev_init);
module_exit(ztemt_virtual_sensor_dev_exit);

MODULE_DESCRIPTION("ztemt_virtual_sensor_dev_remove");
MODULE_AUTHOR("ztemt zhubing");
MODULE_LICENSE("GPL");

