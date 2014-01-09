/******************************************************************************

  Copyright (C), 2001-2012, ZTE. Co., Ltd.

******************************************************************************/
#include <linux/input/zte_cap_touch.h>

/*----------------------------------------------*
 * macros                                       *
 *----------------------------------------------*/

#define SLOT_TYPE				0 		// touch event type choise

#define I2C_RETRY_CNT 			5 		//total i2c retry count 
#define DOWNLOAD_RETRY_CNT 	5 		//unuse
#define MELFAS_FW_UPDATE 		1 		//melfas firmware update

#define TS_READ_LEN_ADDR 		0x0F	//the register expr the total packages
#define TS_READ_START_ADDR 	0x10 	//the status register
#define TS_READ_REGS_LEN 		31 		//read regs len
#define TS_WRITE_REGS_LEN 		16 		//write regs len

#define TS_MAX_TOUCH 			5 		//max touch point
#define TS_READ_HW_VER_ADDR 	0xC1	//hw version address
#define TS_READ_SW_VER_ADDR 	0xC3	//sw version address
#define TS_READ_IC_ID_ADDR  	0xC4	//ic id address
#define TS_READ_Vindor_ADDR	0xC5	//vendor address

#define MELFAS_MMS144_IC_ID 	0x08	//ic id

#define MELFAS_MMS144_HW_VER 	0x01 	//hw version
#define MELFAS_MMS144_FW_VER 	0x06 	//sw version

#if MELFAS_FW_UPDATE
#include "melfas_mms144_isc_bin.h"
//#include "melfas_mms144_isp.h"
//#include "melfas_mms144_isp_bin.h"
#endif

#if SLOT_TYPE
#define REPORT_MT(touch_number, x, y, area, pressure) \
do {     \
	input_mt_slot(ts->input_dev, touch_number);	\
	input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, true);	\
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);             \
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);             \
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, area);         \
	input_report_abs(ts->input_dev, ABS_MT_PRESSURE, pressure); \
} while (0)
#else
#define REPORT_MT(touch_number, x, y, area, pressure) \
do {     \
	input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, touch_number);\
	input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);             \
	input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);             \
	input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, area);         \
	input_report_abs(ts->input_dev, ABS_MT_PRESSURE, pressure); \
	input_mt_sync(ts->input_dev);                                      \
} while (0)
#endif

enum
{
	None = 0,
	TOUCH_SCREEN,
	TOUCH_KEY
};

struct muti_touch_info
{
	int pressure;
	int area;	
	int posX;
	int posY;
};

static struct muti_touch_info g_Mtouch_info[TS_MAX_TOUCH];
static struct zte_ctp_info melfas_mms144_info;

#if MELFAS_FW_UPDATE
extern int melfas_mms144_isc_fw_download(struct zte_ctp_dev *ts, const u8 *data, size_t len);
#endif

static int melfas_mms144_i2c_read(struct i2c_client *client, u16 addr, u16 length, u8 *value)
{
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg;
	int ret = -1;

	msg.addr = client->addr;
	msg.flags = 0x00;
	msg.len = 1;
	msg.buf = (u8 *) & addr;

	ret = i2c_transfer(adapter, &msg, 1);

	if (ret >= 0)
	{
		msg.addr = client->addr;
		msg.flags = I2C_M_RD;
		msg.len = length;
		msg.buf = (u8 *) value;

		ret = i2c_transfer(adapter, &msg, 1);
	} 
	
	if(ret < 0)
	{
		pr_err("[TP] : melfas_mms144_i2c_read error = [%d]", ret);
	}

	return ret;
}

static void melfas_mms144_get_data(struct zte_ctp_dev *ts, uint8_t *buf)
{
	int ret = 0, i, read_num;
	int fingerID, Touch_Type = 0, touchState = 0, pressure;

	if (ts == NULL)
		pr_info("[TP] %s: ts data is NULL \n", __FUNCTION__);

	/*Read the touch num*/
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = melfas_mms144_i2c_read(ts->client, TS_READ_LEN_ADDR, 1, buf);
		if (ret >= 0 && buf[0] <= TS_READ_REGS_LEN) {
//		if (ret >= 0 && ret <= TS_READ_REGS_LEN) {
			TP_DBG("TS_READ_LEN_ADDR succeed = [%d] \n", ret);
			break; // i2c success
		}
	}

	if (ret < 0 || buf[0] > TS_READ_REGS_LEN) {
//	if (ret < 0 || ret > TS_READ_REGS_LEN) {
		TP_DBG("TS_READ_LEN_ADDR fail = [%d] \n", ret);
		return;
	} else {
		read_num = buf[0];
		TP_DBG("Read_num = [%d] \n", read_num);
	}

	/*Read the touch info*/
	if (read_num > 0) {
		for (i = 0; i < I2C_RETRY_CNT; i++) {
			ret = melfas_mms144_i2c_read(ts->client, TS_READ_START_ADDR, read_num, buf);
			if (ret > 0 && buf[0] != 0xFF) {
//			if (ret >= 0 && buf[0] != 0xFF) {
				TP_DBG("TS_READ_START_ADDR succeed = [%d] \n", ret);
				break;//i2c success
			}
		}
	}

	if (ret < 0 || buf[0] == 0xFF) {
//	if (ret < 0) {
		TP_DBG("TS_READ_START_ADDR fail = [%d] \n", ret);
		return;
	}
			
	/*Parase the touch info*/
	for (i = 0; i < read_num; i = i + 6) {
		Touch_Type = (buf[i] >> 5) & 0x03;

		TP_DBG("Touch_Type = %d\n",Touch_Type);

		/* touch type is panel */
		if (Touch_Type == TOUCH_SCREEN)	{
			fingerID = (buf[i] & 0x0F) - 1;
			touchState = (buf[i] & 0x80);

			g_Mtouch_info[fingerID].posX = (uint16_t)(buf[i + 1] & 0x0F) << 8 | buf[i + 2];
			g_Mtouch_info[fingerID].posY = (uint16_t)(buf[i + 1] & 0xF0) << 4 | buf[i + 3];
			g_Mtouch_info[fingerID].area = buf[i + 4];

			if (touchState)
				g_Mtouch_info[fingerID].pressure = buf[i + 5];
			else
				g_Mtouch_info[fingerID].pressure = 0;
			TP_DBG("Screen pressed, x = %d, y = %d, p = %d\n", g_Mtouch_info[fingerID].posX, g_Mtouch_info[fingerID].posY, g_Mtouch_info[fingerID].pressure);
		} else if (Touch_Type == TOUCH_KEY) {
			fingerID = (buf[i] & 0x0F) - 1;
			touchState = (buf[i] & 0x80);
			if (touchState)
				pressure = buf[i + 5];
			else 
				pressure = 0;
			TP_DBG("Key pressed, ID = %d, State = %d, Prssure = %d\n",fingerID, touchState, pressure);
			switch (fingerID) {			
			case 0:
				g_Mtouch_info[fingerID].posX = ts->pdata->zte_virtual_key[0].x;
				g_Mtouch_info[fingerID].posY = ts->pdata->zte_virtual_key[0].y;
				g_Mtouch_info[fingerID].pressure = pressure;
				break;
			case 1:
				g_Mtouch_info[fingerID].posX = ts->pdata->zte_virtual_key[1].x;
				g_Mtouch_info[fingerID].posY = ts->pdata->zte_virtual_key[1].y;
				g_Mtouch_info[fingerID].pressure = pressure;
				break;	
			default:
				TP_ERR("Key not support\n");
				break;
			}
		}
	}
}

static void melfas_mms144_process_data(struct zte_ctp_dev *ts, uint8_t *buf)
{
	int i,finger_num = 0;;
	
	for (i = 0; i < TS_MAX_TOUCH; i++)
	{
		if (g_Mtouch_info[i].pressure == -1)
			continue;

		if(g_Mtouch_info[i].pressure == 0)
		{
			// release event	  
			input_mt_sync(ts->input_dev);  
		}
		else
		{
			REPORT_MT(i, g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].area, g_Mtouch_info[i].pressure);
			finger_num++;
		}
		TP_DBG("Touch ID = %d, State = %d, x = %d, y = %d, z = %d w = %d\n", 
						i, (g_Mtouch_info[i].pressure > 0), g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, g_Mtouch_info[i].pressure, g_Mtouch_info[i].area);
		if (g_Mtouch_info[i].pressure == 0)
			g_Mtouch_info[i].pressure = -1;
	}
	input_report_key(ts->input_dev, BTN_TOUCH, (finger_num == 0)? 0:1);
	input_sync(ts->input_dev);
}

static irqreturn_t melfas_mms144_irq_handler(int irq, void *dev_id)
{
	struct zte_ctp_dev *melfas_data = (struct zte_ctp_dev *)dev_id;

	TP_DBG("melfas_mms100_irq_handler\n");
	
	disable_irq_nosync(gpio_to_irq(melfas_data->pdata->zte_tp_interface.irq));
	queue_work(melfas_data->ts_wq, &melfas_data->ts_work);
	
	return IRQ_HANDLED;
}

static void melfas_mms144_work_func(struct work_struct *work)
{
	struct zte_ctp_dev *melfas_data = container_of(work,struct zte_ctp_dev,ts_work);
	uint8_t buf[TS_READ_REGS_LEN] = { 0, };
	
	if (!work) {
		TP_ERR("NULL Pintor\n");
		goto out;
	}
	melfas_mms144_get_data(melfas_data,buf);
	melfas_mms144_process_data(melfas_data, buf);
out:
	enable_irq(gpio_to_irq(melfas_data->pdata->zte_tp_interface.irq));
	return;
}

static int melfas_mms144_isActive(struct zte_ctp_dev *ts)
{
	int ret;
	int i;
	u8 val[3];
	
	if (!ts) {
		TP_ERR("NULL Pointer\n");
		ret = -1;
		goto out;
	}

	for (i = 0; i < I2C_RETRY_CNT; i++)
	{
		ret = melfas_mms144_i2c_read(ts->client, TS_READ_SW_VER_ADDR, 1, &val[0]);
		ret = melfas_mms144_i2c_read(ts->client, TS_READ_IC_ID_ADDR, 1, &val[1]);
		ret = melfas_mms144_i2c_read(ts->client, TS_READ_Vindor_ADDR, 1, &val[2]);
		
		if (ret >= 0)
		{
			break; // i2c success
		}
	}

	if (ret < 0)
	{
		TP_ERR("[TP] ID read fail = [%d] \n", ret);
		goto out;
	}

	melfas_mms144_info.firmwareVer = val[0];
	if (val[1] == MELFAS_MMS144_IC_ID) {
		TP_INFO("[TP] : SW_VER[0x%02x] IC_ID[0x%02x] VINDOR_ID[0x%02x] \n", val[0], val[1], val[2]);
		return 0;
	}else {
		TP_INFO("[TP] : Melfas_mms100 detect failed, id_data = %d\ns",val[1]);
		ret = -1;
	}
	
out:
	return ret;
}

static void melfas_ts_release_all_finger(struct zte_ctp_dev *ts)
{
	int i;
	for(i=0; i<TS_MAX_TOUCH; i++)
	{
		if(-1 == g_Mtouch_info[i].pressure)
			continue;

		if(0 == g_Mtouch_info[i].pressure) {
			g_Mtouch_info[i].pressure = -1;
			input_mt_sync(ts->input_dev);
		} else {
			g_Mtouch_info[i].pressure = -1;
			REPORT_MT(i, g_Mtouch_info[i].posX, g_Mtouch_info[i].posY, 0, 0);
		}
	}
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_sync(ts->input_dev);
}

#if MELFAS_FW_UPDATE
static ssize_t melfas_mms144_firmware_update(struct device *dev,
											struct device_attribute *attr,
											const char *buf, size_t count)
{
	u8 val[3];
	int ret = 0;
	unsigned int version;
	struct zte_ctp_dev *data = dev_get_drvdata(dev);

	if (sscanf(buf, "%u", &version) != 1) {
		dev_err(dev, "Version is an Invalid value !\n");
		return -EINVAL;
	}

	disable_irq(gpio_to_irq(data->pdata->zte_tp_interface.irq));

	if (melfas_mms144_info.firmwareVer <= MELFAS_MMS144_FW_VER)
	{
		TP_INFO("Updata FirmwareVer %d to %d\n",melfas_mms144_info.firmwareVer,MELFAS_MMS144_FW_VER);
		ret = melfas_mms144_isc_fw_download(data, MELFAS_isc_binary, MELFAS_isc_binary_nLength);
		if (ret < 0) {
			TP_ERR("Updating firmware failed!\n");
			ret = -1;
		} else {
			TP_INFO("Updating firmware succeed!\n");
		}
	}

	enable_irq(gpio_to_irq(data->pdata->zte_tp_interface.irq));

	if(!ret) {
		mdelay(100);
		melfas_mms144_i2c_read(data->client, TS_READ_SW_VER_ADDR, 1, &val[0]);
		TP_INFO("Read sw ver = %x\n", val[0]);
	}

	return count;
}

static DEVICE_ATTR(melfas_mms144_update_fw, S_IRUGO|S_IWUSR, NULL, melfas_mms144_firmware_update);	

static struct attribute *melfas_mms144_attrs[] = {
	&dev_attr_melfas_mms144_update_fw.attr,
	NULL
};

static const struct attribute_group melfas_mms144_attr_group = {
	.attrs = melfas_mms144_attrs,
};

static int melfas_mms144_initialize(struct zte_ctp_dev *ts)
{
 	int ret;
	
 	ret = sysfs_create_group(&ts->client->dev.kobj,&melfas_mms144_attr_group);
	if (ret) {
		pr_err("create sysfs melfas_mms144_attr_group error!!\n");
	}

	return ret;
 }
#else
static int melfas_mms144_initialize(struct zte_ctp_dev *ts)
{
	return 1;
 }
#endif

static int melfas_mms144_sleep(struct zte_ctp_dev *ts, bool sleep)
{
	if (!ts) {
		TP_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if(ts->pdata->zte_tp_interface.reset == 0) {
		pr_err("The gpio_reset pin is error\n");
		return -EINVAL;
	}
	if(sleep) {
		/*suspend*/
		gpio_set_value(ts->pdata->zte_tp_interface.reset,0);
		disable_irq(gpio_to_irq(ts->pdata->zte_tp_interface.irq));
		melfas_ts_release_all_finger(ts);
	} else {
		/*resume*/
		gpio_set_value(ts->pdata->zte_tp_interface.reset,1);
		enable_irq(gpio_to_irq(ts->pdata->zte_tp_interface.irq));
	}
	return 0;
}

static struct zte_ctp_ops melfas_mms144_ops = {
	.irq_handler = melfas_mms144_irq_handler,
	.work_func = melfas_mms144_work_func,
	.init_ts = melfas_mms144_initialize,
	.isActive = melfas_mms144_isActive,
	.sleep = melfas_mms144_sleep,
	.post_init = NULL,
	.tp_anti_charger = NULL,
};

static struct zte_ctp_info melfas_mms144_info = {
	.ts_name = "melfas_mms144",
	.vendor_name = "EELY",
	.i2c_addr = 0x4b,
	.irqflags = IRQF_TRIGGER_FALLING,
	.ops = &melfas_mms144_ops,
};

static int __init melfas_mms144_init(void)
{
	TP_INFO("melfas_mms144_init\n");
	return zte_touch_register(&melfas_mms144_info);
}

module_init(melfas_mms144_init);

MODULE_LICENSE("GPL");
