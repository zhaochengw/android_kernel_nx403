/*
 * Atmel maXTouch Touchscreen driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Copyright (C) 2011 Atmel Corporation
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>

#include <linux/input/zte_cap_touch.h>
#include <linux/regulator/consumer.h>

/* Family ID */
#define MXT224_ID		0x80
#define MXT768E_ID		0xA1
#define MXT1386_ID		0xA0

/* Version */
#define MXT336_VER_16		16
#define MXT336_VER_20		20
#define MXT336_VER_21		21
#define MXT336_VER_22		22
#define MXT336_VER_32		32

#define MXT336_YILI			22
#define MXT336_MUTTO		11

/* Firmware files */
#define MXT336_FW_NAME		"maxtouch.fw"
#define MXT336_CFG_NAME		"atmel_mxt336.cfg"
#define MXT336_CFG_MAGIC		"ZTEMTN8300"

#define MXT336_FAMILY_ID_VAL		0x82
#define MXT336_VARIANT_ID_VAL		0x19
#define MXT336_VERSION_VAL			0x01
#define MXT336_BUILD_VAL			0xaa
#define MXT336_OBJECT_NUM_VAL		0x16

/* Registers */
#define MXT336_FAMILY_ID		0x00
#define MXT336_VARIANT_ID		0x01
#define MXT336_VERSION			0x02
#define MXT336_BUILD			0x03
#define MXT336_MATRIX_X_SIZE	0x04
#define MXT336_MATRIX_Y_SIZE	0x05
#define MXT336_OBJECT_NUM		0x06
#define MXT336_OBJECT_START	0x07

#define MXT336_OBJECT_SIZE			6

#define MXT336_MAX_BLOCK_WRITE	256

/* Object types */
#define MXT336_DEBUG_DIAGNOSTIC_T37		37
#define MXT336_GEN_MESSAGE_T5			5
#define MXT336_GEN_COMMAND_T6			6
#define MXT336_GEN_POWER_T7				7
#define MXT336_GEN_ACQUIRE_T8				8
#define MXT336_GEN_DATASOURCE_T53		53
#define MXT336_TOUCH_MULTI_T9				9
#define MXT336_TOUCH_KEYARRAY_T15		15
#define MXT336_TOUCH_PROXIMITY_T23		23
#define MXT336_TOUCH_PROXKEY_T52			52
#define MXT336_PROCI_GRIPFACE_T20			20
#define MXT336_PROCG_NOISE_T22			22
#define MXT336_PROCI_ONETOUCH_T24		24
#define MXT336_PROCI_TWOTOUCH_T27		27
#define MXT336_PROCI_GRIP_T40				40
#define MXT336_PROCI_PALM_T41				41
#define MXT336_PROCI_TOUCHSUPPRESSION_T42	42
#define MXT336_PROCI_STYLUS_T47				47
#define MXT336_PROCG_NOISESUPPRESSION_T48	48
#define MXT336_SPT_COMMSCONFIG_T18			18
#define MXT336_SPT_GPIOPWM_T19		19
#define MXT336_SPT_SELFTEST_T25		25
#define MXT336_SPT_CTECONFIG_T28		28
#define MXT336_SPT_USERDATA_T38		38
#define MXT336_SPT_DIGITIZER_T43		43
#define MXT336_SPT_MESSAGECOUNT_T44	44
#define MXT336_SPT_CTECONFIG_T46		46
#define MXT336_PROCI_ADAPTIVETHRESHOLD_T55		55
#define MXT336_PROCI_SHIELDLESS_T56				56
#define MXT336_PROCI_EXTRATOUCHSCREENDATA_T57	57
#define MXT336_PROCG_NOISESUPPRESSION_T62		62

/* MXT336_GEN_MESSAGE_T5 object */
#define MXT336_RPTID_NOMSG		0xff
#define MXT336_MSG_MAX_SIZE		9

/* MXT336_GEN_COMMAND_T6 field */
#define MXT336_COMMAND_RESET			0
#define MXT336_COMMAND_BACKUPNV		1
#define MXT336_COMMAND_CALIBRATE		2
#define MXT336_COMMAND_REPORTALL		3
#define MXT336_COMMAND_DIAGNOSTIC	5

/* MXT336_GEN_POWER_T7 field */
#define MXT336_POWER_IDLEACQINT		0
#define MXT336_POWER_ACTVACQINT		1
#define MXT336_POWER_ACTV2IDLETO		2

#define MXT336_POWER_CFG_RUN			0
#define MXT336_POWER_CFG_DEEPSLEEP	1

/* MXT336_GEN_ACQUIRE_T8 field */
#define MXT336_ACQUIRE_CHRGTIME		0
#define MXT336_ACQUIRE_TCHDRIFT		2
#define MXT336_ACQUIRE_DRIFTST		3
#define MXT336_ACQUIRE_TCHAUTOCAL	4
#define MXT336_ACQUIRE_SYNC			5
#define MXT336_ACQUIRE_ATCHCALST		6
#define MXT336_ACQUIRE_ATCHCALSTHR	7
#define MXT336_ACQUIRE_ATCHFRCCALTHR		8
#define MXT336_ACQUIRE_ATCHFRCCALRATIO	9

/* MXT336_TOUCH_MULTI_T9 field */
#define MXT336_TOUCH_CTRL			0
#define MXT336_TOUCH_XORIGIN		1
#define MXT336_TOUCH_YORIGIN		2
#define MXT336_TOUCH_XSIZE			3
#define MXT336_TOUCH_YSIZE			4
#define MXT336_TOUCH_BLEN			6
#define MXT336_TOUCH_TCHTHR		7
#define MXT336_TOUCH_TCHDI		8
#define MXT336_TOUCH_ORIENT		9
#define MXT336_TOUCH_MOVHYSTI		11
#define MXT336_TOUCH_MOVHYSTN	12
#define MXT336_TOUCH_NUMTOUCH	14
#define MXT336_TOUCH_MRGHYST		15
#define MXT336_TOUCH_MRGTHR		16
#define MXT336_TOUCH_AMPHYST		17
#define MXT336_TOUCH_XRANGE_LSB	18
#define MXT336_TOUCH_XRANGE_MSB	19
#define MXT336_TOUCH_YRANGE_LSB	20
#define MXT336_TOUCH_YRANGE_MSB	21
#define MXT336_TOUCH_XLOCLIP		22
#define MXT336_TOUCH_XHICLIP		23
#define MXT336_TOUCH_YLOCLIP		24
#define MXT336_TOUCH_YHICLIP		25
#define MXT336_TOUCH_XEDGECTRL	26
#define MXT336_TOUCH_XEDGEDIST	27
#define MXT336_TOUCH_YEDGECTRL	28
#define MXT336_TOUCH_YEDGEDIST	29
#define MXT336_TOUCH_JUMPLIMIT	30

/* MXT336_PROCI_GRIPFACE_T20 field */
#define MXT336_GRIPFACE_CTRL		0
#define MXT336_GRIPFACE_XLOGRIP	1
#define MXT336_GRIPFACE_XHIGRIP	2
#define MXT336_GRIPFACE_YLOGRIP	3
#define MXT336_GRIPFACE_YHIGRIP	4
#define MXT336_GRIPFACE_MAXTCHS	5
#define MXT336_GRIPFACE_SZTHR1	7
#define MXT336_GRIPFACE_SZTHR2	8
#define MXT336_GRIPFACE_SHPTHR1	9
#define MXT336_GRIPFACE_SHPTHR2	10
#define MXT336_GRIPFACE_SUPEXTTO	11

/* MXT336_PROCI_NOISE field */
#define MXT336_NOISE_CTRL				0
#define MXT336_NOISE_OUTFLEN			1
#define MXT336_NOISE_GCAFUL_LSB		3
#define MXT336_NOISE_GCAFUL_MSB		4
#define MXT336_NOISE_GCAFLL_LSB		5
#define MXT336_NOISE_GCAFLL_MSB		6
#define MXT336_NOISE_ACTVGCAFVALID	7
#define MXT336_NOISE_NOISETHR			8
#define MXT336_NOISE_FREQHOPSCALE	10
#define MXT336_NOISE_FREQ0				11
#define MXT336_NOISE_FREQ1				12
#define MXT336_NOISE_FREQ2				13
#define MXT336_NOISE_FREQ3				14
#define MXT336_NOISE_FREQ4				15
#define MXT336_NOISE_IDLEGCAFVALID	16

/* MXT336_SPT_COMMSCONFIG_T18 */
#define MXT336_COMMS_CTRL			0
#define MXT336_COMMS_CMD			1

/* MXT336_SPT_CTECONFIG_T28 field */
#define MXT336_CTE_CTRL			0
#define MXT336_CTE_CMD				1
#define MXT336_CTE_MODE			2
#define MXT336_CTE_IDLEGCAFDEPTH	3
#define MXT336_CTE_ACTVGCAFDEPTH	4
#define MXT336_CTE_VOLTAGE		5

/* MXT336_SPT_USERDATA_T38 */
#define MXT336_USERDATA_VENDOR	0
#define MXT336_USERDATA_UPDATE	1
#define MXT336_USERDATA_VERSION 	5

#define MXT336_VOLTAGE_DEFAULT	2700000
#define MXT336_VOLTAGE_STEP		10000

/* Define for MXT336_GEN_COMMAND_T6 */
#define MXT336_BOOT_VALUE		0xa5
#define MXT336_RESET_VALUE		0x01
#define MXT336_BACKUP_VALUE	0x55

/* Define for T8 Flag */
#define MXT336_PALM_RECOVERY		1
#define MXT336_PALM_NORMAL_USE	0

/* Delay times */
#define MXT336_BACKUP_TIME		25	/* msec */
#define MXT224_RESET_TIME			65	/* msec */
#define MXT768E_RESET_TIME			250	/* msec */
#define MXT1386_RESET_TIME			200	/* msec */
#define MXT336_RESET_TIME			200	/* msec */
#define MXT336_RESET_NOCHGREAD	400	/* msec */
#define MXT336_FWRESET_TIME		1000	/* msec */

/* Command to unlock bootloader */
#define MXT336_UNLOCK_CMD_MSB	0xaa
#define MXT336_UNLOCK_CMD_LSB		0xdc

/* Bootloader mode status */
#define MXT336_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define MXT336_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define MXT336_FRAME_CRC_CHECK		0x02
#define MXT336_FRAME_CRC_FAIL			0x03
#define MXT336_FRAME_CRC_PASS			0x04
#define MXT336_APP_CRC_FAIL			0x40	/* valid 7 8 bit only */
#define MXT336_BOOT_STATUS_MASK		0x3f
#define MXT336_BOOT_EXTENDED_ID		(1 << 5)
#define MXT336_BOOT_ID_MASK			0x1f

/* Command process status */
#define MXT336_STATUS_CFGERROR	(1 << 3)
#define MXT336_STATUS_CALIBRATE	(1 << 4)
#define MXT336_STATUS_RESET		(1 << 7)

/* Touch status */
#define MXT336_SUPPRESS	(1 << 1)
#define MXT336_AMP			(1 << 2)
#define MXT336_VECTOR		(1 << 3)
#define MXT336_MOVE		(1 << 4)
#define MXT336_RELEASE		(1 << 5)
#define MXT336_PRESS		(1 << 6)
#define MXT336_DETECT		(1 << 7)

/* Touch orient bits */
#define MXT336_XY_SWITCH		(1 << 0)
#define MXT336_X_INVERT		(1 << 1)
#define MXT336_Y_INVERT		(1 << 2)

/* Touchscreen absolute values */
#define MXT336_MAX_AREA		0xff

#define MXT336_MAX_FINGER		5

/* Orient */
#define MXT336_NORMAL					0x0
#define MXT336_DIAGONAL				0x1
#define MXT336_HORIZONTAL_FLIP		0x2
#define MXT336_ROTATED_90_COUNTER	0x3
#define MXT336_VERTICAL_FLIP			0x4
#define MXT336_ROTATED_90				0x5
#define MXT336_ROTATED_180			0x6
#define MXT336_DIAGONAL_COUNTER		0x7

#define MXT336_BOOTLOADER_ADDR		0x25

#define MXT336_FW_UPGRADE					0
#define MXT336_CFG_UPDATE					0
#define MXT336_CFG_UPDATE_WITH_ARRAY	1

#define MXT336_CALIBRATE	1

struct mxt336_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

struct mxt336_object {
	u8 type;
	u16 start_address;
	u16 size;
	u16 instances;
	u8 num_report_ids;
	/* to map object and message */
	u8 min_reportid;
	u8 max_reportid;
};

struct mxt336_message {
	u8 reportid;
	u8 message[MXT336_MSG_MAX_SIZE - 2];
};

struct mxt336_finger {
	int status;
	int x;
	int y;
	int area;
	int pressure;
};

enum mxt336_device_state { INIT, APPMODE, BOOTLOADER, FAILED };

struct mxt336_device_info {
	bool debug_enabled;
	bool driver_paused;
	u8 is_stopped;
	u16 mem_size;
	unsigned int max_x;
	unsigned int max_y;
	u8 actv_cycle_time;
	u8 idle_cycle_time;
	u8 mxt336_max_reportid;
	u8 bootloader_addr;
	struct mxt336_info info;
	enum mxt336_device_state state;
	struct mxt336_object *object_table;
	struct mxt336_finger mxt336_finger[MXT336_MAX_FINGER];
#if MXT336_FW_UPGRADE
	struct bin_attribute mem_access_attr;
#endif
};

#if MXT336_CALIBRATE
#define RECOVERY_MESSAGE_COUNT	1000
static unsigned int g_count_T9_message = 0;
static int atmel_mxt336_T8_set_palm(struct zte_ctp_dev *ts, u8 flag);
#endif

static struct mxt336_device_info *atmel_mxt336_dev_info;
static void atmel_mxt336_release_all_finger(struct zte_ctp_dev *ts);

#if MXT336_CFG_UPDATE_WITH_ARRAY
static const u8 mxt336_cfg_init_ver_16_yili[] = {
	/*[GEN_POWERCONFIG_T7 INSTANCE 0] */    
	0x32, 0xff, 0xff, 0x03,
	
	/*[GEN_ACQUISITIONCONFIG_T8 INSTANCE 0]*/    
	0x14, 0x00, 0x14, 0x14, 0x00, 0x00, 0xff, 0x01, 0x00, 0x00,  
 
	/*[TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0] */    
	0x8b, 0x00, 0x01, 0x16, 0x0d, 0x01, 0x80, 0x50, 0x02, 0x07,
	0x00, 0x05, 0x02, 0x01, 0x0a, 0x0a, 0x0a, 0x0a, 0x20, 0x03,
	0xe0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x0f, 0x32, 0x32, 0x00, 0x00, 
 
	/*[TOUCH_KEYARRAY_T15 INSTANCE 0] Object*/    
	0x03, 0x00, 0x00, 0x02, 0x01, 0x01, 0x40, 0x28, 0x00, 0x00, 
	0x00,   
 
	/*[SPT_COMMSCONFIG_T18 INSTANCE 0] */    
	0x00, 0x00,    
 
	/*[SPT_GPIOPWM_T19 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    
 
	/*[TOUCH_PROXIMITY_T23 INSTANCE 0] Object*/    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     
	0x00, 0x00, 0x00, 0x00, 0x00,    
 
	/*[SPT_SELFTEST_T25 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00,
 
	/*[PROCI_GRIPSUPPRESSION_T40 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00,    
 
	/*[PROCI_TOUCHSUPPRESSION_T42 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 
	/*[SPT_CTECONFIG_T46 INSTANCE 0] */    
	0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  
 
	/*[PROCI_STYLUS_T47 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    
	0x00, 0x00, 0x00,
      
	/*[PROCI_ADAPTIVETHRESHOLD_T55 INSTANCE 0]*/
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 
	/*[PROCI_SHIELDLESS_T56 INSTANCE 0]*/
	0x03, 0x00, 0x01, 0x29, 0x2c, 0x0e, 0x28, 0x05, 0x27, 0x27, 
	0x11, 0x27, 0x01, 0x23, 0x23, 0x0e, 0x23, 0x05, 0x23, 0x01,       
	0x1f, 0x1f, 0x1f, 0x20, 0x1f, 0x0f, 0x00, 0x00, 0x03, 0x64, 
	0x01, 0x01, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00,
 
	/*[PROCI_EXTRATOUCHSCREENDATA_T57 INSTANCE 0]*/
	0x00, 0x00, 0x00, 
 
	/*[PROCG_NOISESUPPRESSION_T62 INSTANCE 0] */    
	0x7f, 0x03, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00,     
	0x06, 0x0b, 0x10, 0x14, 0x05, 0x00, 0x0a, 0x05, 0x05, 0x60,     
	0x1e, 0x64, 0x34, 0x1a, 0x64, 0x00, 0x00, 0x04, 0x40, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x70, 0x2d, 0x02, 0x02, 0x01, 0x20,        
	0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     
	0x00, 0x00, 0x0e, 0x00,
};

static const u8 mxt336_cfg_init_ver_16_mutto[] = {   
	/*[GEN_POWERCONFIG_T7 INSTANCE 0] */    
	0x32, 0xff, 0xff, 0x03,
	
	/*[GEN_ACQUISITIONCONFIG_T8 INSTANCE 0]*/    
	0x14, 0x00, 0x14, 0x14, 0x00, 0x00, 0xff, 0x01, 0x00, 0x00,  
 
	/*[TOUCH_MULTITOUCHSCREEN_T9 INSTANCE 0] */    
	0x8b, 0x00, 0x00, 0x18, 0x0d, 0x00, 0x80, 0x50, 0x02, 0x05,
	0x00, 0x05, 0x02, 0x01, 0x05, 0x0a, 0x0a, 0x0a, 0x20, 0x03,
	0xe0, 0x01, 0x08, 0x0a, 0x12, 0x12, 0x8f, 0x28, 0x99, 0x3c,
	0x00, 0x0f, 0x32, 0x32, 0x00, 0x00, 
 
	/*[TOUCH_KEYARRAY_T15 INSTANCE 0] Object*/    
	0x03, 0x16, 0x0d, 0x02, 0x01, 0x01, 0x60, 0x32, 0x00, 0x00, 
	0x00,   
 
	/*[SPT_COMMSCONFIG_T18 INSTANCE 0] */    
	0x00, 0x00,    
 
	/*[SPT_GPIOPWM_T19 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    
 
	/*[TOUCH_PROXIMITY_T23 INSTANCE 0] Object*/    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     
	0x00, 0x00, 0x00, 0x00, 0x00,    
 
	/*[SPT_SELFTEST_T25 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00,
 
	/*[PROCI_GRIPSUPPRESSION_T40 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00,    
 
	/*[PROCI_TOUCHSUPPRESSION_T42 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
 
	/*[SPT_CTECONFIG_T46 INSTANCE 0] */    
	0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,  
 
	/*[PROCI_STYLUS_T47 INSTANCE 0] */    
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,    
	0x00, 0x00, 0x00,
      
	/*[PROCI_ADAPTIVETHRESHOLD_T55 INSTANCE 0]*/
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 
	/*[PROCI_SHIELDLESS_T56 INSTANCE 0]*/
	0x03, 0x00, 0x01, 0x28, 0x19, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 
	0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x21, 0x21, 0x21,       
	0x21, 0x21, 0x21, 0x21, 0x21, 0x21, 0x25, 0x25, 0x03, 0x64, 
	0x01, 0x01, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00,
 
	/*[PROCI_EXTRATOUCHSCREENDATA_T57 INSTANCE 0]*/
	0x00, 0x00, 0x00, 
 
	/*[PROCG_NOISESUPPRESSION_T62 INSTANCE 0] */    
	0x7f, 0x03, 0x00, 0x16, 0x00, 0x19, 0x00, 0x00, 0x1e, 0x00,     
	0x06, 0x0b, 0x10, 0x14, 0x05, 0x00, 0x0a, 0x05, 0x05, 0x60,     
	0x1e, 0x64, 0x34, 0x1a, 0x64, 0x00, 0x00, 0x04, 0x40, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x70, 0x2d, 0x02, 0x02, 0x01, 0x20,        
	0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,     
	0x00, 0x00, 0x0e, 0x00,
};
#endif

#if MXT336_FW_UPGRADE
static int atmel_mxt336_fw_read(struct zte_ctp_dev *ts, u8 *val, unsigned int count)
{
	struct i2c_msg msg;

	msg.addr = atmel_mxt336_dev_info->bootloader_addr;
	msg.flags = ts->client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = val;

	return i2c_transfer(ts->client->adapter, &msg, 1);
}

static int atmel_mxt336_fw_write(struct zte_ctp_dev *ts, const u8 * const val,
	unsigned int count)
{
	struct i2c_msg msg;

	msg.addr = atmel_mxt336_dev_info->bootloader_addr;
	msg.flags = ts->client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (u8 *)val;

	return i2c_transfer(ts->client->adapter, &msg, 1);
}

static int atmel_mxt336_get_bootloader_address(struct zte_ctp_dev *ts)
{
	atmel_mxt336_dev_info->bootloader_addr = MXT336_BOOTLOADER_ADDR;
	TP_DBG("Bootloader i2c address: 0x%02x", atmel_mxt336_dev_info->bootloader_addr);
	return 0;
}

static int atmel_mxt336_probe_bootloader(struct zte_ctp_dev *ts)
{
	int ret;
	u8 val;

	ret = atmel_mxt336_get_bootloader_address(ts);
	if (ret)
		return ret;

	if (atmel_mxt336_fw_read(ts, &val, 1) != 1) {
		TP_ERR("atmel_mxt336_fw_read failed\n");
		return -EIO;
	}

	if (val & MXT336_BOOT_STATUS_MASK)
		TP_ERR("Application CRC failure\n");
	else
		TP_DBG("Device in bootloader mode\n");

	return 0;
}

static int atmel_mxt336_get_bootloader_version(struct zte_ctp_dev *ts, u8 val)
{
	u8 buf[3];

	if (val | MXT336_BOOT_EXTENDED_ID) {
		TP_DBG("Getting extended mode ID information");

		if (atmel_mxt336_fw_read(ts, &buf[0], 3) != 3) {
			TP_ERR("atmel_mxt336_fw_read failed!\n");
			return -EIO;
		}

		TP_DBG("Bootloader ID = %d, Version = %d", buf[1], buf[2]);

		return buf[0];
	} else {
		TP_DBG("Bootloader ID = %d", val & MXT336_BOOT_ID_MASK);
		return val;
	}
}

static int atmel_mxt336_check_bootloader(struct zte_ctp_dev *ts,
				unsigned int state)
{
	u8 val;

recheck:
	if (atmel_mxt336_fw_read(ts, &val, 1) != 1) {
		TP_ERR("atmel_mxt336_fw_read failed\n");
		return -EIO;
	}

	switch (state) {
	case MXT336_WAITING_BOOTLOAD_CMD:
		val = atmel_mxt336_get_bootloader_version(ts, val);
		val &= ~MXT336_BOOT_STATUS_MASK;
		break;
	case MXT336_WAITING_FRAME_DATA:
	case MXT336_APP_CRC_FAIL:
		val &= ~MXT336_BOOT_STATUS_MASK;
		break;
	case MXT336_FRAME_CRC_PASS:
		if (val == MXT336_FRAME_CRC_CHECK)
			goto recheck;
		if (val == MXT336_FRAME_CRC_FAIL) {
			TP_ERR("Bootloader CRC fail\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		TP_ERR("Invalid bootloader mode state 0x%X\n", val);
		return -EINVAL;
	}

	return 0;
}

static int atmel_mxt336_unlock_bootloader(struct zte_ctp_dev *ts)
{
	u8 buf[2];

	buf[0] = MXT336_UNLOCK_CMD_LSB;
	buf[1] = MXT336_UNLOCK_CMD_MSB;

	if (atmel_mxt336_fw_write(ts, buf, 2) != 2) {
		TP_ERR("atmel_mxt336_fw_write failed\n");
		return -EIO;
	}

	return 0;
}
#endif

static int __atmel_mxt336_read_reg(struct i2c_client *client,
			       u16 reg, u16 len, void *val)
{
	struct i2c_msg xfer[2];
	u8 buf[2];

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;

	/* Write register */
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;
	xfer[0].buf = buf;

	/* Read data */
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = len;
	xfer[1].buf = val;

	if (i2c_transfer(client->adapter, xfer, 2) != 2) {
		TP_ERR("i2c transfer failed\n");
		return -EIO;
	}

	return 0;
}

static int atmel_mxt336_read_reg(struct i2c_client *client, u16 reg, u8 *val)
{
	return __atmel_mxt336_read_reg(client, reg, 1, val);
}

static int atmel_mxt336_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	u8 buf[3];

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;
	buf[2] = val;

	if (i2c_master_send(client, buf, 3) != 3) {
		TP_ERR("i2c_master_send failed!\n");
		return -EIO;
	}

	return 0;
}

#if MXT336_FW_UPGRADE
static int atmel_mxt336_write_block(struct i2c_client *client, u16 addr, u16 length, u8 *value)
{
	int i;
	struct {
		__le16 le_addr;
		u8  data[MXT336_MAX_BLOCK_WRITE];
	} i2c_block_transfer;

	if (length > MXT336_MAX_BLOCK_WRITE) {
		TP_ERR("Write block length exceed!\n");
		return -EINVAL;
	}

	memcpy(i2c_block_transfer.data, value, length);

	i2c_block_transfer.le_addr = cpu_to_le16(addr);

	i = i2c_master_send(client, (u8 *) &i2c_block_transfer, length + 2);

	if (i == (length + 2))
		return 0;
	else {
		TP_ERR("i2c_master_send failed!\n");
		return -EIO;
	}
}
#endif

static int atmel_mxt336_read_object_table(struct i2c_client *client,
				      u16 reg, u8 *object_buf)
{
	return __atmel_mxt336_read_reg(client, reg, MXT336_OBJECT_SIZE,
				   object_buf);
}

static struct mxt336_object *atmel_mxt336_get_object(struct zte_ctp_dev *ts, u8 type)
{
	struct mxt336_object *object;
	int i;

	for (i = 0; i < atmel_mxt336_dev_info->info.object_num; i++) {
		object = atmel_mxt336_dev_info->object_table + i;
		if (object->type == type)
			return object;
	}

	TP_ERR("Invalid object type = %u\n", type);
	return NULL;
}

static int atmel_mxt336_check_message_length(struct zte_ctp_dev *ts)
{
	struct mxt336_object *object;

	object = atmel_mxt336_get_object(ts, MXT336_GEN_MESSAGE_T5);
	if (!object) {
		TP_ERR("atmel_mxt336_get_object failed!\n");
		return -EINVAL;
	}

	if (object->size > MXT336_MSG_MAX_SIZE) {
		TP_ERR("MXT336_MSG_MAX_SIZE exceeded");
		return -EINVAL;
	}

	return 0;
}

static int atmel_mxt336_read_message(struct zte_ctp_dev *ts,
				 struct mxt336_message *message)
{
	struct mxt336_object *object;
	u16 reg;
	int ret;

	object = atmel_mxt336_get_object(ts, MXT336_GEN_MESSAGE_T5);
	if (!object) {
		TP_ERR("atmel_mxt336_get_object failed!\n");
		return -EINVAL;
	}

	reg = object->start_address;

	/* Do not read last byte which contains CRC */
	ret = __atmel_mxt336_read_reg(ts->client, reg, object->size - 1, message);

	if (ret == 0 && message->reportid != MXT336_RPTID_NOMSG
	    && atmel_mxt336_dev_info->debug_enabled)
		print_hex_dump(KERN_DEBUG, "MXT MSG:", DUMP_PREFIX_NONE, 16, 1,
			       message, object->size - 1, false);
	
	return ret;
}

#if MXT336_CFG_UPDATE
static int atmel_mxt336_read_message_reportid(struct zte_ctp_dev *ts,
	struct mxt336_message *message, u8 reportid)
{
	int try = 0;
	int error;
	int fail_count;

	fail_count = atmel_mxt336_dev_info->mxt336_max_reportid * 2;

	while (++try < fail_count) {
		error = atmel_mxt336_read_message(ts, message);
		if (error) {
			TP_ERR("atmel_mxt336_read_message failed!\n");
			return error;
		}

		if (message->reportid == 0xff) {
			TP_ERR("reportid is 0xff!\n");
			return -EINVAL;
		}

		if (message->reportid == reportid)
			return 0;
	}

	return -EINVAL;
}
#endif

static int atmel_mxt336_read_object(struct zte_ctp_dev *ts, u8 type, u8 offset, u8 *val)
{
	struct mxt336_object *object;
	u16 reg;

	object = atmel_mxt336_get_object(ts, type);
	if (!object) {
		TP_ERR("atmel_mxt336_get_object failed!\n");
		return -EINVAL;
	}

	reg = object->start_address;
	return __atmel_mxt336_read_reg(ts->client, reg + offset, 1, val);
}

static int atmel_mxt336_write_object(struct zte_ctp_dev *ts, u8 type, u8 offset, u8 val)
{
	struct mxt336_object *object;
	u16 reg;

	object = atmel_mxt336_get_object(ts, type);
	if (!object) {
		TP_ERR("atmel_mxt336_get_object failed!\n");
		return -EINVAL;
	}

	if (offset >= object->size * object->instances) {
		TP_ERR("Tried to write outside object type = %d,"
			" offset = %d, size = %d\n", type, offset, object->size);
		return -EINVAL;
	}

	reg = object->start_address;
	return atmel_mxt336_write_reg(ts->client, reg + offset, val);
}

static void atmel_mxt336_input_report(struct zte_ctp_dev *ts, int single_id)
{
	struct mxt336_finger *finger = atmel_mxt336_dev_info->mxt336_finger;
	struct input_dev *input_dev = ts->input_dev;
	int status = finger[single_id].status;
	int finger_num = 0;
	int id;

	for (id = 0; id < MXT336_MAX_FINGER; id++) {
		if (!finger[id].status)
			continue;

		input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
				finger[id].status != MXT336_RELEASE ?
				finger[id].area : 0);
		input_report_abs(input_dev, ABS_MT_POSITION_X,
				finger[id].x);
		input_report_abs(input_dev, ABS_MT_POSITION_Y,
				finger[id].y);
		input_report_abs(input_dev, ABS_MT_PRESSURE,
				finger[id].pressure);
		input_mt_sync(input_dev);

		if (finger[id].status == MXT336_RELEASE)
			finger[id].status = 0;
		else
			finger_num++;
	}

	input_report_key(input_dev, BTN_TOUCH, finger_num > 0);

	if (status != MXT336_RELEASE) {
		input_report_abs(input_dev, ABS_X, finger[single_id].x);
		input_report_abs(input_dev, ABS_Y, finger[single_id].y);
		input_report_abs(input_dev, ABS_PRESSURE, finger[single_id].pressure);
	}

	input_sync(input_dev);
}

static void atmel_mxt336_input_touchevent(struct zte_ctp_dev *ts,
				      struct mxt336_message *message, int id)
{
	struct mxt336_finger *finger = atmel_mxt336_dev_info->mxt336_finger;
	u8 status = message->message[0];
	int x;
	int y;
	int area;
	int pressure;

	if (atmel_mxt336_dev_info->driver_paused)
		return;

	if (id >= MXT336_MAX_FINGER) {
		TP_ERR("MXT336_MAX_FINGER exceeded!\n");
		return;
	}

	/* Check the touch is present on the screen */
	if (!(status & MXT336_DETECT)) {
		if (status & MXT336_SUPPRESS) {
			TP_DBG("[%d] suppressed\n", id);
			finger[id].status = MXT336_RELEASE;
			atmel_mxt336_input_report(ts,id);
		} else if (status & MXT336_RELEASE) {
			TP_DBG("[%d] released\n", id);
			finger[id].status = MXT336_RELEASE;
			atmel_mxt336_input_report(ts, id);
		}
		return;
	}

	/* Check only AMP detection */
	if (!(status & (MXT336_PRESS | MXT336_MOVE)))
		return;

	x = (message->message[1] << 4) | ((message->message[3] >> 4) & 0xf);
	y = (message->message[2] << 4) | ((message->message[3] & 0xf));
	if (ts->pdata->zte_tp_parameter.res_x < 1024)
		x = x >> 2;
	if (ts->pdata->zte_tp_parameter.res_y < 1024)
		y = y >> 2;

	area = message->message[4];
	pressure = message->message[5];

	TP_DBG("[%d] %s, x = %d, y = %d, area = %d\n", id,
		status & MXT336_MOVE ? "moved" : "pressed", x, y, area);

	finger[id].status = status & MXT336_MOVE ? MXT336_MOVE : MXT336_PRESS;
	finger[id].x = x;
	finger[id].y = y;
	finger[id].area = area;
	finger[id].pressure = pressure;

	atmel_mxt336_input_report(ts, id);
}

static void atmel_mxt336_input_keyevent(struct zte_ctp_dev *ts, struct mxt336_message *message)
{
	u8 status = message->message[0];
	u8 keys = message->message[1];

	if(status && 0x80) {
		switch(keys) {
		case 0x01:
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, 0);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->pdata->zte_virtual_key[1].x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->pdata->zte_virtual_key[1].y);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 255);
			input_report_key(ts->input_dev, BTN_TOUCH, 1);
			input_sync(ts->input_dev);
			break;
		case 0x02:
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, 0);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, ts->pdata->zte_virtual_key[0].x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, ts->pdata->zte_virtual_key[0].y);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 1);
			input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 255);
			input_report_key(ts->input_dev, BTN_TOUCH, 1);
			input_sync(ts->input_dev);
			break;
		default:
			TP_ERR("Keys not support!\n");
			break;
		}
	} else {
		input_report_key(ts->input_dev, BTN_TOUCH, 0);
		input_sync(ts->input_dev);
	}
	
	return;
}

static void atmel_mxt336_work_func(struct work_struct *ts_work)
{
	struct zte_ctp_dev *ts =  container_of(ts_work, struct zte_ctp_dev, ts_work);
	struct mxt336_message message;
	struct mxt336_object *object;
	int touchid;
	u8 reportid;

	do {
		if (atmel_mxt336_read_message(ts, &message)) {
			TP_ERR("Failed to read message\n");
			goto end;
		}

		reportid = message.reportid;

		object = atmel_mxt336_get_object(ts, MXT336_TOUCH_MULTI_T9);
		if (!object) {
			TP_ERR("atmel_mxt336_get_object failed!\n");
			goto end;
		}

		if (reportid >= object->min_reportid && reportid <= object->max_reportid) {
		#if MXT336_CALIBRATE
			g_count_T9_message++;
			if(g_count_T9_message > RECOVERY_MESSAGE_COUNT) {
				atmel_mxt336_T8_set_palm(ts, MXT336_PALM_NORMAL_USE);
			}
		#endif
		
			touchid = reportid - object->min_reportid;
			atmel_mxt336_input_touchevent(ts, &message, touchid);
		} else if (reportid == 12) {
		#if MXT336_CALIBRATE
			g_count_T9_message++;
			if(g_count_T9_message > RECOVERY_MESSAGE_COUNT) {
				atmel_mxt336_T8_set_palm(ts, MXT336_PALM_NORMAL_USE);
			}
		#endif
		
			atmel_mxt336_input_keyevent(ts, &message);
		} else if(reportid == 1) {
			object = atmel_mxt336_get_object(ts, MXT336_GEN_COMMAND_T6);
			if (!object) {
				TP_ERR("atmel_mxt336_get_object failed!\n");
				goto end;
			}

			if ((reportid == object->max_reportid) && (message.message[0] & MXT336_STATUS_CFGERROR))
				TP_ERR("Configuration error\n");
			
		#if MXT336_CALIBRATE
			if((message.message[0] & MXT336_STATUS_CALIBRATE) || (message.message[0] & MXT336_STATUS_RESET)) {
				TP_DBG("mxt336 status calibrate : %d, reset : %d\n", (message.message[0] & MXT336_STATUS_CALIBRATE), (message.message[0] & MXT336_STATUS_RESET));
				g_count_T9_message = 0;
				atmel_mxt336_release_all_finger(ts);
			}
		#endif
		} else {
			//other object message process
		}
	} while (reportid != MXT336_RPTID_NOMSG);

end:
	enable_irq(gpio_to_irq(ts->pdata->zte_tp_interface.irq));
}

static irqreturn_t atmel_mxt336_irq_handler(int irq, void *dev_id)
{
	struct zte_ctp_dev *ts = dev_id;

	disable_irq_nosync(gpio_to_irq(ts->pdata->zte_tp_interface.irq));
	queue_work(ts->ts_wq, &ts->ts_work);

	return IRQ_HANDLED;
}

static int atmel_mxt336_read_chg(struct zte_ctp_dev *ts)
{
	return gpio_get_value(ts->pdata->zte_tp_interface.irq);
}

static int atmel_mxt336_make_highchg(struct zte_ctp_dev *ts)
{
	struct mxt336_message message;
	int count;
	int error;

	if(atmel_mxt336_dev_info->state != APPMODE) {
		TP_ERR("IC is not in APPMODE!\n");
		return -EBUSY;
	}

	/* If all objects report themselves then a number of messages equal to
	 * the number of report ids may be generated. Therefore a good safety
	 * heuristic is twice this number */
	count = atmel_mxt336_dev_info->mxt336_max_reportid * 2;

	/* Read dummy message to make high CHG pin */
	do {
		error = atmel_mxt336_read_message(ts, &message);
		if (error) {
			TP_ERR("atmel_mxt336_read_message failed!\n");
			return error;
		}
	} while (message.reportid != MXT336_RPTID_NOMSG && --count);

	if (!count) {
		TP_ERR("CHG pin isn't cleared\n");
		return -EBUSY;
	}

	return 0;
}

#if MXT336_CFG_UPDATE
static int atmel_mxt336_read_current_crc(struct zte_ctp_dev *ts, unsigned long *crc)
{
	int error;
	struct mxt336_message message;
	struct mxt336_object *object;

	object = atmel_mxt336_get_object(ts, MXT336_GEN_COMMAND_T6);
	if (!object) {
		TP_ERR("atmel_mxt336_get_object failed!\n");
		return -EIO;
	}

	/* Try to read the config checksum of the existing cfg */
	atmel_mxt336_write_object(ts, MXT336_GEN_COMMAND_T6, MXT336_COMMAND_REPORTALL, 1);

	msleep(30);

	/* Read message from command processor, which only has one report ID */
	error = atmel_mxt336_read_message_reportid(ts, &message, object->max_reportid);
	if (error) {
		TP_ERR("Failed to retrieve CRC\n");
		return error;
	}

	/* Bytes 1-3 are the checksum. */
	*crc = message.message[1] | (message.message[2] << 8) | (message.message[3] << 16);

	return 0;
}

static int atmel_mxt336_download_config(struct zte_ctp_dev *ts, const char *fn)
{
	struct device *dev = &ts->client->dev;
	struct mxt336_info cfg_info;
	struct mxt336_object *object;
	const struct firmware *cfg = NULL;
	int ret;
	int offset;
	int pos;
	int i;
	unsigned long current_crc, info_crc, config_crc;
	unsigned int type, instance, size;
	u8 val;
	u16 reg;

	ret = request_firmware(&cfg, fn, dev);
	if (ret < 0) {
		TP_ERR("Failure to request config file %s\n", fn);
		return -EINVAL;
	}

	ret = atmel_mxt336_read_current_crc(ts, &current_crc);
	if (ret) {
		TP_ERR("atmel_mxt336_read_current_crc failed!\n");
		return ret;
	}

	if (strncmp(cfg->data, MXT336_CFG_MAGIC, strlen(MXT336_CFG_MAGIC))) {
		TP_ERR("Unrecognised config file\n");
		ret = -EINVAL;
		goto release;
	}

	pos = strlen(MXT336_CFG_MAGIC);

	/* Load information block and check */
	for (i = 0; i < sizeof(struct mxt336_info); i++) {
		ret = sscanf(cfg->data + pos, "%hhx%n", (unsigned char *)&cfg_info + i, &offset);
		if (ret != 1) {
			TP_ERR("Bad format\n");
			ret = -EINVAL;
			goto release;
		}

		pos += offset;
	}

	if (cfg_info.family_id != atmel_mxt336_dev_info->info.family_id) {
		TP_ERR("Family ID mismatch!\n");
		ret = -EINVAL;
		goto release;
	}

	if (cfg_info.variant_id != atmel_mxt336_dev_info->info.variant_id) {
		TP_ERR("Variant ID mismatch!\n");
		ret = -EINVAL;
		goto release;
	}

	if (cfg_info.version != atmel_mxt336_dev_info->info.version)
		TP_ERR("Warning: version mismatch!\n");

	if (cfg_info.build != atmel_mxt336_dev_info->info.build)
		TP_ERR("Warning: build num mismatch!\n");

	ret = sscanf(cfg->data + pos, "%lx%n", &info_crc, &offset);
	if (ret != 1) {
		TP_ERR("Bad format\n");
		ret = -EINVAL;
		goto release;
	}
	pos += offset;

	/* Check config CRC */
	ret = sscanf(cfg->data + pos, "%lx%n", &config_crc, &offset);
	if (ret != 1) {
		TP_ERR("Bad format\n");
		ret = -EINVAL;
		goto release;
	}
	pos += offset;

	if (current_crc == config_crc) {
		TP_DBG("Config CRC 0x%X: OK\n", (unsigned int) current_crc);
		ret = 0;
		goto release;
	} else {
		TP_DBG("Config CRC 0x%X: does not match 0x%X, writing config\n",
				(unsigned int) current_crc, (unsigned int) config_crc);
	}

	while (pos < cfg->size) {
		/* Read type, instance, length */
		ret = sscanf(cfg->data + pos, "%x %x %x%n", &type, &instance, &size, &offset);
		if (ret == 0) {
			/* EOF */
			TP_ERR("sscanf EOF!\n");
			ret = 1;
			goto release;
		} else if (ret < 0) {
			TP_ERR("Bad format\n");
			ret = -EINVAL;
			goto release;
		}
		pos += offset;

		object = atmel_mxt336_get_object(ts, type);
		if (!object) {
			TP_ERR("atmel_mxt336_get_object failed!\n");
			ret = -EINVAL;
			goto release;
		}

		if (size > object->size) {
			TP_ERR("Object length exceeded!\n");
			ret = -EINVAL;
			goto release;
		}

		if (instance >= object->instances) {
			TP_ERR("Object instances exceeded!\n");
			ret = -EINVAL;
			goto release;
		}

		reg = object->start_address + object->size * instance;

		for (i = 0; i < size; i++) {
			ret = sscanf(cfg->data + pos, "%hhx%n", &val, &offset);
			if (ret != 1) {
				TP_ERR("Bad format\n");
				ret = -EINVAL;
				goto release;
			}

			ret = atmel_mxt336_write_reg(ts->client, reg + i, val);
			if (ret) {
				TP_ERR("atmel_mxt336_write_reg failed!\n");
				goto release;
			}

			pos += offset;
		}

		/* If firmware is upgraded, new bytes may be added to end of
		 * objects. It is generally forward compatible to zero these
		 * bytes - previous behaviour will be retained. However
		 * this does invalidate the CRC and will force a config
		 * download every time until the configuration is updated */
		if (size < object->size) {
			TP_DBG("Warning: zeroing %d byte(s) in T%d\n", object->size - size, type);

			for (i = size + 1; i < object->size; i++) {
				ret = atmel_mxt336_write_reg(ts->client, reg + i, 0);
				if (ret) {
					TP_ERR("atmel_mxt336_write_reg failed!\n");
					goto release;
				}
			}
		}
	}

release:
	release_firmware(cfg);
	return ret;
}

static ssize_t atmel_mxt336_update_cfg(struct device *dev,
											struct device_attribute *attr,
											const char *buf, size_t count)
{
	struct zte_ctp_dev *ts = dev_get_drvdata(dev);
	unsigned int version;
	int ret;

	TP_DBG("\n\n [Atmel_mxt336 cfg upgrade] Start ...\n\n");

	if (sscanf(buf, "%u", &version) != 1) {
		dev_err(dev, "Invalid values\n");
		return -EINVAL;
	}
	
	disable_irq(gpio_to_irq(ts->pdata->zte_tp_interface.irq));

	ret = atmel_mxt336_download_config(ts, MXT336_CFG_NAME);
	if (ret < 0) {
		TP_ERR("atmel_mxt336_download_config error\n");
		return ret;
	} else if (ret == 0) {
		/* CRC matched, or no config file, no need to reset */
		TP_DBG("No config file find!\n");
		return 0;
	}

	/* Backup to memory */
	atmel_mxt336_write_object(ts, MXT336_GEN_COMMAND_T6,
						MXT336_COMMAND_BACKUPNV, MXT336_BACKUP_VALUE);
	msleep(MXT336_BACKUP_TIME);

	enable_irq(gpio_to_irq(ts->pdata->zte_tp_interface.irq));

	TP_DBG("[Atmel_mxt336 cfg upgrade] End !!\n\n");

	return ret;	
}

static DEVICE_ATTR(atmel_mxt336_update_cfg, S_IRUGO|S_IWUSR, NULL, atmel_mxt336_update_cfg);	

static struct attribute *mxt336_attrs[] = {
	&dev_attr_atmel_mxt336_update_cfg.attr,
	NULL
};

static const struct attribute_group mxt336_attr_group = {
	.attrs = mxt336_attrs,
};
#endif

#if MXT336_CFG_UPDATE_WITH_ARRAY
static bool atmel_mxt336_object_writable(unsigned int type)
{
	switch (type) {
	case MXT336_GEN_POWER_T7:
	case MXT336_GEN_ACQUIRE_T8:
	case MXT336_TOUCH_MULTI_T9:
	case MXT336_TOUCH_KEYARRAY_T15:
	case MXT336_SPT_COMMSCONFIG_T18:
	case MXT336_SPT_GPIOPWM_T19:
	case MXT336_TOUCH_PROXIMITY_T23:
	case MXT336_SPT_SELFTEST_T25:
	case MXT336_PROCI_GRIP_T40:
	case MXT336_PROCI_TOUCHSUPPRESSION_T42:
	case MXT336_SPT_CTECONFIG_T46:
	case MXT336_PROCI_STYLUS_T47:
	case MXT336_PROCI_ADAPTIVETHRESHOLD_T55:
	case MXT336_PROCI_SHIELDLESS_T56:
	case MXT336_PROCI_EXTRATOUCHSCREENDATA_T57:
	case MXT336_PROCG_NOISESUPPRESSION_T62:
		return true;
	default:
		return false;
	}
}

static int atmel_mxt336_update_cfg_with_array(struct zte_ctp_dev *ts)
{
	struct mxt336_object *object; 
	int index = 0;
	int i, j;
	u8 version = atmel_mxt336_dev_info->info.version;
	u8 *init_vals = NULL;
	u8 vendor;
	u8 update;
	int ret;

	ret = atmel_mxt336_read_object(ts,MXT336_SPT_USERDATA_T38,MXT336_USERDATA_UPDATE,&update);
	if(ret < 0) {
		TP_ERR("atmel_mxt336_read_object failed!\n");
		return ret;
	}


	if(update) {
		TP_DBG("cfg has updated!\n");
		return 0;
	}


	ret = atmel_mxt336_read_object(ts,MXT336_SPT_USERDATA_T38,MXT336_USERDATA_VENDOR,&vendor);
	if(ret < 0) {
		TP_ERR("atmel_mxt336_read_object failed!\n");
		return ret;
	}

	TP_INFO("Version = %d, Vendor = %d\n",version, vendor);
	
	switch (version) {
	case MXT336_VER_16:	
		if (vendor == MXT336_YILI) {
			/*BYD TP 32 with ID*/
			init_vals = (u8 *)mxt336_cfg_init_ver_16_yili;
		} else if (vendor == MXT336_MUTTO) {
			/*Goworld TP 32 with ID*/
			init_vals = (u8 *)mxt336_cfg_init_ver_16_mutto;
		} else {
			init_vals = (u8 *)mxt336_cfg_init_ver_16_yili;
		}
		break;	
	default:
		TP_INFO("Firmware version %d doesn't support\n", version);
		return -EINVAL;
	}

	for (i = 0; i < atmel_mxt336_dev_info->info.object_num; i++) 
	{
		object = atmel_mxt336_dev_info->object_table + i;

		if (!atmel_mxt336_object_writable(object->type))
			continue;

		for (j = 0; j < object->size; j++)
			atmel_mxt336_write_object(ts, object->type, j, init_vals[index + j]);

		index += object->size;
	}

	/* Backup to memory */
	ret = atmel_mxt336_write_object(ts, MXT336_GEN_COMMAND_T6,
						MXT336_COMMAND_BACKUPNV, MXT336_BACKUP_VALUE);
	if(ret < 0) {
		TP_ERR("atmel_mxt336_write_object failed!\n");
		return ret;
	}
	
	ret = atmel_mxt336_write_object(ts, MXT336_SPT_USERDATA_T38, MXT336_USERDATA_UPDATE, 1);
	if(ret < 0) {
		TP_ERR("atmel_mxt336_write_object failed!\n");
		return ret;
	}
	msleep(MXT336_BACKUP_TIME);
	
	return 0;
}
#endif

static int atmel_mxt336_soft_reset(struct zte_ctp_dev *ts, u8 value)
{
	int timeout_counter = 0;

	TP_DBG("atmel_mxt336_soft_reset\n");

	atmel_mxt336_write_object(ts, MXT336_GEN_COMMAND_T6,
			MXT336_COMMAND_RESET, value);

	switch (atmel_mxt336_dev_info->info.family_id) {
	case MXT224_ID:
		msleep(MXT224_RESET_TIME);
		break;
	case MXT768E_ID:
		msleep(MXT768E_RESET_TIME);
		break;
	case MXT1386_ID:
		msleep(MXT1386_RESET_TIME);
		break;
	default:
		msleep(MXT336_RESET_TIME);
	}
	
	timeout_counter = 0;
	while ((timeout_counter++ <= 100) && !!atmel_mxt336_read_chg(ts))
		msleep(20);
	if (timeout_counter > 100) {
		TP_ERR("No response after reset!\n");
		return -EIO;
	}

	return 0;
}

static int atmel_mxt336_set_power_cfg(struct zte_ctp_dev *ts, u8 mode)
{
	int error;
	u8 actv_cycle_time;
	u8 idle_cycle_time;

	if (atmel_mxt336_dev_info->state != APPMODE) {
		TP_ERR("Not in APPMODE\n");
		return -EINVAL;
	}

	switch (mode) {
	case MXT336_POWER_CFG_DEEPSLEEP:
		actv_cycle_time = 0;
		idle_cycle_time = 0;
		break;
	case MXT336_POWER_CFG_RUN:
	default:
		actv_cycle_time = atmel_mxt336_dev_info->actv_cycle_time;
		idle_cycle_time = atmel_mxt336_dev_info->idle_cycle_time;
		break;
	}

	error = atmel_mxt336_write_object(ts, MXT336_GEN_POWER_T7, MXT336_POWER_ACTVACQINT,
	actv_cycle_time);
	if (error)
		goto i2c_error;

	error = atmel_mxt336_write_object(ts, MXT336_GEN_POWER_T7, MXT336_POWER_IDLEACQINT,
				idle_cycle_time);
	if (error)
		goto i2c_error;

	TP_DBG("Set actv_cycle_time = %d, idle_cycle_time = %d", actv_cycle_time, idle_cycle_time);

	atmel_mxt336_dev_info->is_stopped = (mode == MXT336_POWER_CFG_DEEPSLEEP) ? 1 : 0;

	return 0;

i2c_error:
	TP_ERR("Failed to set power cfg");
	return error;
}

static int atmel_mxt336_read_power_cfg(struct zte_ctp_dev *ts, u8 *actv_cycle_time,
				u8 *idle_cycle_time)
{
	int error;

	error = atmel_mxt336_read_object(ts, MXT336_GEN_POWER_T7,
				MXT336_POWER_ACTVACQINT,
				actv_cycle_time);
	if (error) {
		TP_ERR("Read power cfg ACTVACQINT error\n");
		return error;
	}

	error = atmel_mxt336_read_object(ts, MXT336_GEN_POWER_T7,
				MXT336_POWER_IDLEACQINT,
				idle_cycle_time);
	if (error) {
		TP_ERR("Read power cfg IDLEACQINT error\n");
		return error;
	}

	return 0;
}

static int atmel_mxt336_check_power_cfg_post_reset(struct zte_ctp_dev *ts)
{
	int error;

	error = atmel_mxt336_read_power_cfg(ts, &atmel_mxt336_dev_info->actv_cycle_time,
				   &atmel_mxt336_dev_info->idle_cycle_time);
	if (error) {
		TP_ERR("atmel_mxt336_read_power_cfg error\n");
		return error;
	}

	/* Power config is zero, select free run */
	if (atmel_mxt336_dev_info->actv_cycle_time == 0 || atmel_mxt336_dev_info->idle_cycle_time == 0) {
		TP_DBG("Overriding power cfg to free run\n");
		atmel_mxt336_dev_info->actv_cycle_time = 255;
		atmel_mxt336_dev_info->idle_cycle_time = 255;

		error = atmel_mxt336_set_power_cfg(ts, MXT336_POWER_CFG_RUN);
		if (error) {
			TP_ERR("atmel_mxt336_set_power_cfg error\n");
			return error;
		}
	}

	return 0;
}

static int atmel_mxt336_probe_power_cfg(struct zte_ctp_dev *ts)
{
	int error;

	error = atmel_mxt336_read_power_cfg(ts, &atmel_mxt336_dev_info->actv_cycle_time,
				   &atmel_mxt336_dev_info->idle_cycle_time);
	if (error) {
		TP_ERR("atmel_mxt336_read_power_cfg error\n");
		return error;
	}

	/* If in deep sleep mode, attempt reset */
	if (atmel_mxt336_dev_info->actv_cycle_time == 0 || atmel_mxt336_dev_info->idle_cycle_time == 0) {
		error = atmel_mxt336_soft_reset(ts, MXT336_RESET_VALUE);
		if (error) {
			TP_ERR("atmel_mxt336_soft_reset error\n");
			return error;
		}

		error = atmel_mxt336_check_power_cfg_post_reset(ts);
		if (error) {
			TP_ERR("atmel_mxt336_check_power_cfg_post_reset error\n");
			return error;
		}
	}

	return 0;
}

#if MXT336_CALIBRATE
static int atmel_mxt336_T8_set_palm(struct zte_ctp_dev *ts, u8 flag)
{
	u8 error = 0;
	
	if (flag) {		
		//Set to 0, 0, 1, 0 for Palm recovery
		error = atmel_mxt336_write_object(ts, MXT336_GEN_ACQUIRE_T8, MXT336_ACQUIRE_ATCHCALST, 0);
		if (error) {
			TP_ERR("Write T8 error\n");
			return 1;
		}
		
		error = atmel_mxt336_write_object(ts, MXT336_GEN_ACQUIRE_T8, MXT336_ACQUIRE_ATCHCALSTHR, 0);
		if (error) {
			TP_ERR("Write T8 error\n");
			return 1;
		}
		
		error = atmel_mxt336_write_object(ts, MXT336_GEN_ACQUIRE_T8, MXT336_ACQUIRE_ATCHFRCCALTHR, 1);
		if (error) {
			TP_ERR("Write T8 error\n");
			return 1;
		}
		
		error = atmel_mxt336_write_object(ts, MXT336_GEN_ACQUIRE_T8, MXT336_ACQUIRE_ATCHFRCCALRATIO, 0);
		if (error) {
			TP_ERR("Write T8 error\n");
			return 1;
		}
	} else {
		//set to 255, 1, 0, 0 for Normal use
		error = atmel_mxt336_write_object(ts, MXT336_GEN_ACQUIRE_T8, MXT336_ACQUIRE_ATCHCALST, 255);
		if (error) {
			TP_ERR("Write T8 error\n");
			return 1;
		}
		
		error = atmel_mxt336_write_object(ts, MXT336_GEN_ACQUIRE_T8, MXT336_ACQUIRE_ATCHCALSTHR, 1);
		if (error) {
			TP_ERR("Write T8 error\n");
			return 1;
		}
		
		error = atmel_mxt336_write_object(ts, MXT336_GEN_ACQUIRE_T8, MXT336_ACQUIRE_ATCHFRCCALTHR, 0);
		if (error) {
			TP_ERR("Write T8 error\n");
			return 1;
		}
		
		error = atmel_mxt336_write_object(ts, MXT336_GEN_ACQUIRE_T8, MXT336_ACQUIRE_ATCHFRCCALRATIO, 0);
		if (error) {
			TP_ERR("Write T8 error\n");
			return 1;
		}		
	}

	return 0;	
}
#endif

static int atmel_mxt336_check_reg_init(struct zte_ctp_dev *ts)
{
	int ret;
	int timeout_counter = 0;
	u8 command_register;

#if MXT336_CFG_UPDATE
	ret = atmel_mxt336_download_config(ts, MXT336_CFG_NAME);
	if (ret < 0) {
		TP_ERR("atmel_mxt336_download_config error\n");
		return ret;
	} else if (ret == 0) {
		/* CRC matched, or no config file, no need to reset */
		TP_DBG("No config file find!\n");
		return 0;
	}

	/* Backup to memory */
	atmel_mxt336_write_object(ts, MXT336_GEN_COMMAND_T6,
						MXT336_COMMAND_BACKUPNV, MXT336_BACKUP_VALUE);
	msleep(MXT336_BACKUP_TIME);
#endif

#if MXT336_CFG_UPDATE_WITH_ARRAY
	ret = atmel_mxt336_update_cfg_with_array(ts);
	if (ret < 0) {
		TP_ERR("atmel_mxt336_update_cfg_with_array error\n");
		return ret;
	}
#endif

	do {
		ret =  atmel_mxt336_read_object(ts, MXT336_GEN_COMMAND_T6,
						MXT336_COMMAND_BACKUPNV, &command_register);
		if (ret) {
			TP_ERR("Backup error\n");
			return ret;
		}
		msleep(20);
	} while ((command_register != 0) && (timeout_counter++ <= 100));
	if (timeout_counter > 100) {
		TP_ERR("No response after backup!\n");
		return -EIO;
	}

	ret = atmel_mxt336_soft_reset(ts, MXT336_RESET_VALUE);
	if (ret) {
		TP_ERR("atmel_mxt336_soft_reset error\n");
		return ret;
	}

	ret = atmel_mxt336_check_power_cfg_post_reset(ts);
	if (ret) {
		TP_ERR("atmel_mxt336_check_power_cfg_post_reset error\n");
		return ret;
	}

#if MXT336_CALIBRATE
	g_count_T9_message = 0;
	ret = atmel_mxt336_T8_set_palm(ts, MXT336_PALM_RECOVERY);
	if (ret) {
		TP_ERR("atmel_mxt336_T8_set_palm error\n");
		return ret;
	}
#endif

	return 0;
}

static int atmel_mxt336_get_info(struct zte_ctp_dev *ts)
{
	struct i2c_client *client = ts->client;
	struct mxt336_info *info = &atmel_mxt336_dev_info->info;
	int error;
	u8 val;

	error = atmel_mxt336_read_reg(client, MXT336_FAMILY_ID, &val);
	if (error) {
		TP_ERR("Read FAMILY_ID error\n");
		return error;
	}
	if (val != MXT336_FAMILY_ID_VAL) {
		TP_ERR("IC FAMILY_ID dismatch = %d\n", val);
		return 1;
	}
	info->family_id = val;

	error = atmel_mxt336_read_reg(client, MXT336_VARIANT_ID, &val);
	if (error) {
		TP_ERR("Read VARIANT_ID error\n");
		return error;
	}
	if (val != MXT336_VARIANT_ID_VAL) {
		TP_ERR("IC VARIANT_ID dismatch = %d\n", val);
		return 1;
	}
	info->variant_id = val;

	error = atmel_mxt336_read_reg(client, MXT336_VERSION, &val);
	if (error) {
		TP_ERR("Read VERSION error\n");
		return error;
	}
	info->version = val;

	error = atmel_mxt336_read_reg(client, MXT336_BUILD, &val);
	if (error) {
		TP_ERR("Read BUILD error\n");
		return error;
	}
	info->build = val;

	error = atmel_mxt336_read_reg(client, MXT336_OBJECT_NUM, &val);
	if (error) {
		TP_ERR("Read OBJECT_NUM error\n");
		return error;
	}
	info->object_num = val;

	TP_DBG("Family ID = %d, Variant ID = %d, Version = %d.%d, "
		"Build = 0x%02X, Object Num = %d\n",
		info->family_id, info->variant_id, info->version >> 4, info->version & 0xf,
		info->build, info->object_num);

	return 0;
}

static int atmel_mxt336_get_object_table(struct zte_ctp_dev *ts)
{
	int i;
	u16 reg;
	int error;
	u8 reportid = 0;
	u16 end_address;
	u8 buf[MXT336_OBJECT_SIZE];
	
	atmel_mxt336_dev_info->mem_size = 0;

	for (i = 0; i < atmel_mxt336_dev_info->info.object_num; i++) {
		struct mxt336_object *object = atmel_mxt336_dev_info->object_table + i;

		reg = MXT336_OBJECT_START + MXT336_OBJECT_SIZE * i;
		error = atmel_mxt336_read_object_table(ts->client, reg, buf);
		if (error) {
			TP_ERR("atmel_mxt336_read_object_table error\n");
			return error;
		}

		object->type = buf[0];
		object->start_address = (buf[2] << 8) | buf[1];
		object->size = buf[3] + 1;
		object->instances = buf[4] + 1;
		object->num_report_ids = buf[5];

		if (object->num_report_ids) {
			reportid += object->num_report_ids * object->instances;
			object->max_reportid = reportid;
			object->min_reportid = object->max_reportid -object->instances * object->num_report_ids + 1;
		}

		end_address = object->start_address + object->size * object->instances - 1;

		if (end_address >= atmel_mxt336_dev_info->mem_size)
			atmel_mxt336_dev_info->mem_size = end_address + 1;

		TP_DBG("Type = %u, start_address = %u, size = %u, instances = %u, "
			"min_reportid = %u, max_reportid = %u\n",
			object->type, object->start_address, object->size, object->instances,
			object->min_reportid, object->max_reportid);
	}

	/* Store maximum reportid */
	atmel_mxt336_dev_info->mxt336_max_reportid = reportid;

	return 0;
}

static int atmel_mxt336_read_resolution(struct zte_ctp_dev *ts)
{
	int error;
	unsigned char val;
	unsigned char orient;
	unsigned int x_range, y_range;
	struct i2c_client *client = ts->client;

	/* Update matrix size in info struct */
	error = atmel_mxt336_read_reg(client, MXT336_MATRIX_X_SIZE, &val);
	if (error) {
		TP_ERR("Read X_SIZE error\n");
		return error;
	}
	atmel_mxt336_dev_info->info.matrix_xsize = val;

	error = atmel_mxt336_read_reg(client, MXT336_MATRIX_Y_SIZE, &val);
	if (error) {
		TP_ERR("Read Y_SIZE error\n");
		return error;
	}
	atmel_mxt336_dev_info->info.matrix_ysize = val;

	/* Read X/Y size of touchscreen */
	error =  atmel_mxt336_read_object(ts, MXT336_TOUCH_MULTI_T9,
			       MXT336_TOUCH_XRANGE_MSB, &val);
	if (error) {
		TP_ERR("Read XRANGE_MSB error\n");
		return error;
	}
	x_range = val << 8;

	error =  atmel_mxt336_read_object(ts, MXT336_TOUCH_MULTI_T9,
			       MXT336_TOUCH_XRANGE_LSB, &val);
	if (error) {
		TP_ERR("Read XRANGE_LSB error\n");
		return error;
	}
	x_range |= val;

	error =  atmel_mxt336_read_object(ts, MXT336_TOUCH_MULTI_T9,
			       MXT336_TOUCH_YRANGE_MSB, &val);
	if (error) {
		TP_ERR("Read YRANGE_MSB error\n");
		return error;
	}
	y_range = val << 8;

	error =  atmel_mxt336_read_object(ts, MXT336_TOUCH_MULTI_T9,
			       MXT336_TOUCH_YRANGE_LSB, &val);
	if (error) {
		TP_ERR("Read YRANGE_LSB error\n");
		return error;
	}
	y_range |= val;

	error =  atmel_mxt336_read_object(ts, MXT336_TOUCH_MULTI_T9,
			       MXT336_TOUCH_ORIENT, &orient);
	if (error) {
		TP_ERR("Read ORIENT error\n");
		return error;
	}

	/* Handle default values */
	if (x_range == 0)
		x_range = 1023;

	if (y_range == 0)
		y_range = 1023;

	if (orient & MXT336_XY_SWITCH) {
		atmel_mxt336_dev_info->max_x = y_range;
		atmel_mxt336_dev_info->max_y = x_range;
	} else {
		atmel_mxt336_dev_info->max_x = x_range;
		atmel_mxt336_dev_info->max_y = y_range;
	}

	TP_DBG("Matrix Size X = %u, Y = %u, Touchscreen size X = %u, Y = %u\n",
			atmel_mxt336_dev_info->info.matrix_xsize, atmel_mxt336_dev_info->info.matrix_ysize,
			atmel_mxt336_dev_info->max_x, atmel_mxt336_dev_info->max_y);

	return 0;
}

static int atmel_mxt336_initialize(struct zte_ctp_dev *ts)
{
	int error;
	struct mxt336_info *info = &atmel_mxt336_dev_info->info;

	error = atmel_mxt336_get_info(ts);
	if (error < 0) {
	#if MXT336_FW_UPGRADE
		error = atmel_mxt336_probe_bootloader(ts);
		if (error) {
			TP_ERR("atmel_mxt336_probe_bootloader failed!\n");
			return error;
		} else {
			atmel_mxt336_dev_info->state = BOOTLOADER;
			return 0;
		}		
	#endif
		TP_ERR("Read reg failed!\n");
		return error;
	} else if (error > 0) {
 		TP_ERR("IC FAMILY_ID or VARIANT_ID dismatch!\n");
		return error;
	}

	atmel_mxt336_dev_info->state = APPMODE;
	atmel_mxt336_dev_info->driver_paused = 0;
	atmel_mxt336_dev_info->debug_enabled = 0;

	atmel_mxt336_dev_info->object_table = kcalloc(info->object_num,
				     sizeof(struct mxt336_object), GFP_KERNEL);
	if (!atmel_mxt336_dev_info->object_table) {
		TP_ERR("Kcalloc object_table failed!\n");
		return -ENOMEM;
	}

	/* Get object table information */
	error = atmel_mxt336_get_object_table(ts);
	if (error) {
		TP_ERR("atmel_mxt336_get_object_table failed!\n");
		goto err_init;
	}

	error = atmel_mxt336_check_message_length(ts);
	if (error) {
		TP_ERR("atmel_mxt336_check_message_length failed!\n");
		goto err_init;
	}

	error = atmel_mxt336_probe_power_cfg(ts);
	if (error) {
		TP_ERR("atmel_mxt336_probe_power_cfg failed!\n");
		goto err_init;
	}

	/* Check register init values */
	error = atmel_mxt336_check_reg_init(ts);
	if (error) {
		TP_ERR("atmel_mxt336_check_reg_init failed!\n");
		goto err_init;
	}

	error = atmel_mxt336_read_resolution(ts);
	if (error) {
		TP_ERR("atmel_mxt336_read_resolution failed!\n");
		goto err_init;
	}

	return 0;

err_init:
	kfree(atmel_mxt336_dev_info->object_table);
	return error;
}

#if MXT336_FW_UPGRADE
static int atmel_mxt336_load_fw(struct device *dev, const char *fn)
{
	struct zte_ctp_dev *ts = dev_get_drvdata(dev);
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	unsigned int retry = 0;
	int ret;

	ret = request_firmware(&fw, fn, dev);
	if (ret < 0) {
		TP_ERR("Unable to open firmware %s\n", fn);
		return ret;
	}

	if (atmel_mxt336_dev_info->state != BOOTLOADER) {
		/* Change to the bootloader mode */
		ret = atmel_mxt336_soft_reset(ts, MXT336_BOOT_VALUE);
		if (ret)
			goto release_firmware;

		ret = atmel_mxt336_get_bootloader_address(ts);
		if (ret)
			goto release_firmware;

		atmel_mxt336_dev_info->state = BOOTLOADER;
	}

	ret = atmel_mxt336_check_bootloader(ts, MXT336_WAITING_BOOTLOAD_CMD);
	if (ret) {
		/* Bootloader may still be unlocked from previous update
		 * attempt */
		ret = atmel_mxt336_check_bootloader(ts, MXT336_WAITING_FRAME_DATA);
		if (ret) {
			atmel_mxt336_dev_info->state = FAILED;
			goto release_firmware;
		}
	} else {
		TP_DBG("Unlocking bootloader\n");

		/* Unlock bootloader */
		atmel_mxt336_unlock_bootloader(ts);
	}

	while (pos < fw->size) {
		ret = atmel_mxt336_check_bootloader(ts, MXT336_WAITING_FRAME_DATA);
		if (ret) {
			atmel_mxt336_dev_info->state = FAILED;
			goto release_firmware;
		}

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* Take account of CRC bytes */
		frame_size += 2;

		/* Write one frame to device */
		atmel_mxt336_fw_write(ts, fw->data + pos, frame_size);

		ret = atmel_mxt336_check_bootloader(ts, MXT336_FRAME_CRC_PASS);
		if (ret) {
			retry++;

			/* Back off by 20ms per retry */
			msleep(retry * 20);

			if (retry > 20) {
				atmel_mxt336_dev_info->state = FAILED;
				goto release_firmware;
			}
		} else {
			retry = 0;
			pos += frame_size;
			TP_DBG("Updated %d/%zd bytes\n", pos, fw->size);
		}
	}

	atmel_mxt336_dev_info->state = APPMODE;

release_firmware:
	release_firmware(fw);
	return ret;
}

static ssize_t atmel_mxt336_update_fw_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct zte_ctp_dev *ts = dev_get_drvdata(dev);
	int error;

	disable_irq(gpio_to_irq(ts->pdata->zte_tp_interface.irq));

	error = atmel_mxt336_load_fw(dev, MXT336_FW_NAME);
	if (error) {
		TP_ERR("The firmware update failed(%d)\n", error);
		count = error;
	} else {
		TP_DBG("The firmware update succeeded\n");

		/* Wait for reset */
		msleep(MXT336_FWRESET_TIME);

		atmel_mxt336_dev_info->state = INIT;
		kfree(atmel_mxt336_dev_info->object_table);
		atmel_mxt336_dev_info->object_table = NULL;

		atmel_mxt336_initialize(ts);
	}

	if (atmel_mxt336_dev_info->state == APPMODE) {
		enable_irq(gpio_to_irq(ts->pdata->zte_tp_interface.irq));

		error = atmel_mxt336_make_highchg(ts);
		if (error)
			return error;
	}

	return count;
}

static ssize_t atmel_mxt336_pause_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count;
	char c;

	c = atmel_mxt336_dev_info->driver_paused ? '1' : '0';
	count = sprintf(buf, "%c\n", c);

	return count;
}

static ssize_t atmel_mxt336_pause_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int i;

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		atmel_mxt336_dev_info->driver_paused = (i == 1);
		TP_DBG("%s\n", i ? "paused" : "unpaused");
		return count;
	} else {
		TP_DBG("pause_driver write error\n");
		return -EINVAL;
	}
}

static ssize_t atmel_mxt336_debug_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int count;
	char c;

	c = atmel_mxt336_dev_info->debug_enabled ? '1' : '0';
	count = sprintf(buf, "%c\n", c);

	return count;
}

static ssize_t atmel_mxt336_debug_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int i;

	if (sscanf(buf, "%u", &i) == 1 && i < 2) {
		atmel_mxt336_dev_info->debug_enabled = (i == 1);

		TP_DBG("%s\n", i ? "debug enabled" : "debug disabled");
		return count;
	} else {
		TP_DBG("debug_enabled write error\n");
		return -EINVAL;
	}
}

static int atmel_mxt336_check_mem_access_params(struct zte_ctp_dev *data, loff_t off,
				       size_t *count)
{
	if (atmel_mxt336_dev_info->state != APPMODE) {
		TP_ERR("Not in APPMODE\n");
		return -EINVAL;
	}

	if (off >= atmel_mxt336_dev_info->mem_size)
		return -EIO;

	if (off + *count > atmel_mxt336_dev_info->mem_size)
		*count = atmel_mxt336_dev_info->mem_size - off;

	if (*count > MXT336_MAX_BLOCK_WRITE)
		*count = MXT336_MAX_BLOCK_WRITE;

	return 0;
}

static ssize_t atmel_mxt336_mem_access_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct zte_ctp_dev *ts = dev_get_drvdata(dev);
	int ret = 0;

	ret = atmel_mxt336_check_mem_access_params(ts, off, &count);
	if (ret < 0)
		return ret;

	if (count > 0)
		ret = __atmel_mxt336_read_reg(ts->client, off, count, buf);

	return ret == 0 ? count : ret;
}

static ssize_t atmel_mxt336_mem_access_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct zte_ctp_dev *ts = dev_get_drvdata(dev);
	int ret = 0;

	ret = atmel_mxt336_check_mem_access_params(ts, off, &count);
	if (ret < 0)
		return ret;

	if (count > 0)
		ret = atmel_mxt336_write_block(ts->client, off, count, buf);

	return ret == 0 ? count : 0;
}

static DEVICE_ATTR(atmel_mxt336_update_fw, 0664, NULL, atmel_mxt336_update_fw_store);
static DEVICE_ATTR(atmel_mxt336_debug_enable, 0664, atmel_mxt336_debug_enable_show,
		   atmel_mxt336_debug_enable_store);
static DEVICE_ATTR(atmel_mxt336_pause_driver, 0664, atmel_mxt336_pause_show, atmel_mxt336_pause_store);

static struct attribute *atmel_mxt336_attrs[] = {
	&dev_attr_atmel_mxt336_update_fw.attr,
	&dev_attr_atmel_mxt336_debug_enable.attr,
	&dev_attr_atmel_mxt336_pause_driver.attr,
	NULL
};

static const struct attribute_group atmel_mxt336_attr_group = {
	.attrs = atmel_mxt336_attrs,
};
#endif

static void atmel_mxt336_release_all_finger(struct zte_ctp_dev *ts)
{
	int id;
	struct mxt336_finger *finger = atmel_mxt336_dev_info->mxt336_finger;

	/*clear all the touches after resume just in case*/
	for(id=0; id < MXT336_MAX_FINGER; id++){
		if (!finger[id].status)
			continue;
		
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, finger[id].x);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, finger[id].y);
		input_report_abs(ts->input_dev, ABS_MT_PRESSURE, 0);
		input_mt_sync(ts->input_dev);
		finger[id].status = 0;
	}
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_sync(ts->input_dev);
}

static void atmel_mxt336_start(struct zte_ctp_dev *ts)
{
	int error;

	if (!atmel_mxt336_dev_info->is_stopped)
		return;

	error = atmel_mxt336_set_power_cfg(ts, MXT336_POWER_CFG_RUN);
	if (!error)
		TP_DBG("atmel_mxt336_start\n");

	atmel_mxt336_release_all_finger(ts);
}

static void atmel_mxt336_stop(struct zte_ctp_dev *ts)
{
	int error;

	if (atmel_mxt336_dev_info->is_stopped)
		return;

	error = atmel_mxt336_set_power_cfg(ts, MXT336_POWER_CFG_DEEPSLEEP);
	if (!error)
		TP_DBG("atmel_mxt336_stop\n");

	atmel_mxt336_release_all_finger(ts);
}

static int atmel_mxt336_suspend(struct zte_ctp_dev *ts)
{
	struct input_dev *input_dev = ts->input_dev;

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		atmel_mxt336_stop(ts);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static int atmel_mxt336_resume(struct zte_ctp_dev *ts)
{
#if MXT336_CALIBRATE
	int ret = 0;
#endif
	struct input_dev *input_dev = ts->input_dev;

	/* Soft reset */
	atmel_mxt336_soft_reset(ts, MXT336_RESET_VALUE);

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		atmel_mxt336_start(ts);

#if MXT336_CALIBRATE
	g_count_T9_message = 0;
	ret = atmel_mxt336_T8_set_palm(ts, MXT336_PALM_RECOVERY);
	if (ret) {
		TP_ERR("atmel_mxt336_T8_set_palm error\n");
		return ret;
	}
#endif

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static int atmel_mxt336_isActive(struct zte_ctp_dev *ts)
{
	int ret;

	atmel_mxt336_dev_info = kzalloc(sizeof(struct mxt336_device_info), GFP_KERNEL);
	if (!atmel_mxt336_dev_info) {
		TP_ERR("allocate atmel_mxt336_dev_info failed!\n");
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	ret = atmel_mxt336_initialize(ts);
	if (ret){
		TP_ERR("atmel_mxt336_initialize failed!\n");
		ret = -ENODEV;
		goto err_init_failed;
	}

#if MXT336_FW_UPGRADE
	ret = sysfs_create_group(&ts->client->dev.kobj, &atmel_mxt336_attr_group);
	if (ret) {
		TP_ERR("Failure %d creating sysfs group\n", ret);
		goto err_init_failed;
	}

	sysfs_bin_attr_init(&atmel_mxt336_dev_info->mem_access_attr);
	atmel_mxt336_dev_info->mem_access_attr.attr.name = "mem_access";
	atmel_mxt336_dev_info->mem_access_attr.attr.mode = S_IRUGO | S_IWUSR;
	atmel_mxt336_dev_info->mem_access_attr.read = atmel_mxt336_mem_access_read;
	atmel_mxt336_dev_info->mem_access_attr.write = atmel_mxt336_mem_access_write;
	atmel_mxt336_dev_info->mem_access_attr.size = atmel_mxt336_dev_info->mem_size;

	if (sysfs_create_bin_file(&ts->client->dev.kobj, &atmel_mxt336_dev_info->mem_access_attr) < 0) {
		TP_ERR("Failed to create %s\n", atmel_mxt336_dev_info->mem_access_attr.attr.name);
		goto err_remove_sysfs_group;
	}
#endif
	
#if MXT336_CFG_UPDATE
	ret = sysfs_create_group(&ts->client->dev.kobj,&mxt336_attr_group);
	if (ret) {
		pr_err("create sysfs mxt336_attr_group error!!\n");
		goto err_remove_bin_file;
	}
#endif
	
	TP_INFO("atmel mxt touch detect success!\n");
	return 0;

#if MXT336_CFG_UPDATE
err_remove_bin_file:
#if MXT336_FW_UPGRADE
	sysfs_remove_bin_file(&ts->client->dev.kobj, &atmel_mxt336_dev_info->mem_access_attr);
#endif
#endif
#if MXT336_FW_UPGRADE
err_remove_sysfs_group:
	sysfs_remove_group(&ts->client->dev.kobj, &atmel_mxt336_attr_group);	
#endif
err_init_failed:
	kfree(atmel_mxt336_dev_info);	
err_alloc_data_failed:
	return ret;

}

static int atmel_mxt336_sleep(struct zte_ctp_dev *ts, bool sleep)
{
	int ret;
	
	if (sleep){
		ret = atmel_mxt336_suspend(ts);
		if (ret){
			TP_ERR("atmel_mxt336 failed to suspend\n");
			goto atmel_sleep_failed;
		}
	}else{
		ret = atmel_mxt336_resume(ts);
		if (ret){
			TP_ERR("atmel_mxt336 failed to resume\n");
			goto atmel_sleep_failed;
		}
	}
	TP_DBG("atmel_mxt336 %s success\n",sleep == true ? "suspend":"resume");
	return 0;
	
atmel_sleep_failed:
	return ret;
}

static int atmel_mxt336_change_charger_cfg(struct zte_ctp_dev *ts, int charger_type)
{
/*
	if(charger_type == 1){
		atmel_mxt336_write_object(ts, MXT336_TOUCH_MULTI_T9, MXT336_TOUCH_TCHTHR, 0x80);
	} else {
		atmel_mxt336_write_object(ts, MXT336_TOUCH_MULTI_T9, MXT336_TOUCH_TCHTHR, 0x80);
	}
*/
	return 0;
}

static struct zte_ctp_ops atmel_mxt336_ops = {
	.irq_handler = atmel_mxt336_irq_handler,
	.work_func = atmel_mxt336_work_func,
	.init_ts = NULL,
	.isActive = atmel_mxt336_isActive,
	.sleep = atmel_mxt336_sleep, 
	.post_init = atmel_mxt336_make_highchg,
	.tp_anti_charger = atmel_mxt336_change_charger_cfg,
};

static struct zte_ctp_info atmel_mxt336_info = {
	.ts_name = "atmel_mxt336",//TP IC 
	.i2c_addr = 0x4a,
//	.vendor_name = "",  //TP Module Vindor
//	.reginitVer = 0,        //Chip register init value version
	.irqflags = IRQF_TRIGGER_LOW,
	.ops = &atmel_mxt336_ops,
};

static int __init atmel_mxt336_init(void)
{
	TP_DBG("atmel_mxt336_init\n");
	return zte_touch_register(&atmel_mxt336_info);
}

module_init(atmel_mxt336_init);

/* Module information */
MODULE_AUTHOR("ZTEMT <ZTEMT@zte.com.cn>");
MODULE_DESCRIPTION("Atmel maXTouch Touchscreen driver");
MODULE_LICENSE("GPL");
