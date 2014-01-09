/*******************************************************************************
*                                                                              *
*   File Name:    taos.c                                                      *
*   Description:   Linux device driver for Taos ambient light and         *
*   proximity sensors.                                     *
*   Author:         John Koshi                                             *
*   History:   09/16/2009 - Initial creation                          *
*           10/09/2009 - Triton version         *
*           12/21/2009 - Probe/remove mode                *
*           02/07/2010 - Add proximity          *
*                                                                                       *
********************************************************************************
*    Proprietary to Taos Inc., 1001 Klein Road #300, Plano, TX 75074        *
*******************************************************************************/
// includes
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <asm/delay.h>
#include <linux/i2c/taos_common.h>
#include <linux/delay.h>
#include <linux/irq.h> 
#include <linux/interrupt.h> 
#include <linux/slab.h>
#include <asm/gpio.h> 
#include <linux/poll.h> 
#include <linux/wakelock.h>
#include <linux/input.h>
#include <linux/workqueue.h>


#ifdef CONFIG_ZTE_DEVICE_INFO_SHOW
#include <linux/zte_device_info.h>
#endif

#define LOG_TAG "SENSOR_PS_ALS"
#define DEBUG_ON //DEBUG SWITCH

#define SENSOR_LOG_FILE__ strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/')+1) : __FILE__

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


// device name/id/address/counts
#define TAOS_DEVICE_NAME                "taos"
#define TAOS_DEVICE_ID                  "tritonFN"
#define TAOS_ID_NAME_SIZE               10
#define TAOS_TRITON_CHIPIDVAL           0x00
#define TAOS_TRITON_MAXREGS             32
#define TAOS_DEVICE_ADDR1               0x29
#define TAOS_DEVICE_ADDR2               0x39
#define TAOS_DEVICE_ADDR3               0x49
#define TAOS_MAX_NUM_DEVICES            3
#define TAOS_MAX_DEVICE_REGS            32
#define I2C_MAX_ADAPTERS                12

// TRITON register offsets
#define TAOS_TRITON_CNTRL               0x00
#define TAOS_TRITON_ALS_TIME            0X01
#define TAOS_TRITON_PRX_TIME            0x02
#define TAOS_TRITON_WAIT_TIME           0x03
#define TAOS_TRITON_ALS_MINTHRESHLO     0X04
#define TAOS_TRITON_ALS_MINTHRESHHI     0X05
#define TAOS_TRITON_ALS_MAXTHRESHLO     0X06
#define TAOS_TRITON_ALS_MAXTHRESHHI     0X07
#define TAOS_TRITON_PRX_MINTHRESHLO     0X08
#define TAOS_TRITON_PRX_MINTHRESHHI     0X09
#define TAOS_TRITON_PRX_MAXTHRESHLO     0X0A
#define TAOS_TRITON_PRX_MAXTHRESHHI     0X0B
#define TAOS_TRITON_INTERRUPT           0x0C
#define TAOS_TRITON_PRX_CFG             0x0D
#define TAOS_TRITON_PRX_COUNT           0x0E
#define TAOS_TRITON_GAIN                0x0F
#define TAOS_TRITON_REVID               0x11
#define TAOS_TRITON_CHIPID              0x12
#define TAOS_TRITON_STATUS              0x13
#define TAOS_TRITON_ALS_CHAN0LO         0x14
#define TAOS_TRITON_ALS_CHAN0HI         0x15
#define TAOS_TRITON_ALS_CHAN1LO         0x16
#define TAOS_TRITON_ALS_CHAN1HI         0x17
#define TAOS_TRITON_PRX_LO              0x18
#define TAOS_TRITON_PRX_HI              0x19
#define TAOS_TRITON_TEST_STATUS         0x1F

// Triton cmd reg masks
//0x by clli2
#define TAOS_TRITON_CMD_REG             0X80
#define TAOS_TRITON_CMD_AUTO            0x20 
#define TAOS_TRITON_CMD_BYTE_RW         0x00 
#define TAOS_TRITON_CMD_WORD_BLK_RW     0x20 
#define TAOS_TRITON_CMD_SPL_FN          0x60 
#define TAOS_TRITON_CMD_PROX_INTCLR     0X05 
#define TAOS_TRITON_CMD_ALS_INTCLR      0X06 
#define TAOS_TRITON_CMD_PROXALS_INTCLR  0X07 
#define TAOS_TRITON_CMD_TST_REG         0X08 
#define TAOS_TRITON_CMD_USER_REG        0X09

// Triton cntrl reg masks
#define TAOS_TRITON_CNTL_PROX_INT_ENBL  0X20
#define TAOS_TRITON_CNTL_ALS_INT_ENBL   0X10
#define TAOS_TRITON_CNTL_WAIT_TMR_ENBL  0X08
#define TAOS_TRITON_CNTL_PROX_DET_ENBL  0X04
#define TAOS_TRITON_CNTL_ADC_ENBL       0x02
#define TAOS_TRITON_CNTL_PWRON          0x01

// Triton status reg masks
#define TAOS_TRITON_STATUS_ADCVALID     0x01
#define TAOS_TRITON_STATUS_PRXVALID     0x02
#define TAOS_TRITON_STATUS_ADCINTR      0x10
#define TAOS_TRITON_STATUS_PRXINTR      0x20

// lux constants
#define TAOS_MAX_LUX                    10000
#define TAOS_SCALE_MILLILUX             3
#define TAOS_FILTER_DEPTH               3
#define CHIP_ID                         0x3d

#define TAOS_INPUT_NAME                 "lightsensor"
#define	POLL_DELAY	                    msecs_to_jiffies(5)
#define	TAOS_ALS_ADC_TIME_WHEN_PROX_ON	0XF5//0XEB
#define TAOS_ALS_GAIN_DIVIDE            1000
#define TAOS_ALS_GAIN_1X                0
#define TAOS_ALS_GAIN_8X                1
#define TAOS_ALS_GAIN_16X               2
#define TAOS_ALS_GAIN_120X              3


// ZTEMT ADD by zhubing 2012-2-20 V8000/X501
// added the work mode marco
//#define WORK_UES_POLL_MODE
// ZTEMT ADD by zhubing 2012-2-20 V8000/X501 END

//#define IRQ_TRIGER_LEVEL_LOW

// forward declarations
static int taos_probe(struct i2c_client *clientp, const struct i2c_device_id *idp);
static int taos_remove(struct i2c_client *client);
static int taos_open(struct inode *inode, struct file *file);
static int taos_release(struct inode *inode, struct file *file);
static long taos_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t taos_read(struct file *file, char *buf, size_t count, loff_t *ppos);
static ssize_t taos_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static loff_t taos_llseek(struct file *file, loff_t offset, int orig);
static int taos_get_lux(void);
static int taos_lux_filter(int raw_lux);
static int taos_device_name(unsigned char *bufp, char **device_name);
static int taos_prox_poll(struct taos_prox_info *prxp);
static void taos_prox_poll_timer_func(unsigned long param);
static void taos_prox_poll_timer_start(void);
//iVIZM
static int taos_prox_threshold_set(void);
static int taos_als_get_data(void);
static int taos_interrupts_clear(void);
static int taos_resume(struct i2c_client *client);
static int taos_suspend(struct i2c_client *client,pm_message_t mesg);
//CLLI@


static int taos_sensors_als_poll_on(void);
static int taos_sensors_als_poll_off(void);
static void taos_als_poll_work_func(struct work_struct *work);
static int taos_als_gain_set(unsigned als_gain);
static void taos_update_sat_als(void);
static int taos_prox_on(void);
static int taos_prox_off(void);
static int taos_prox_calibrate(void);
static long taos_config_get(struct taos_cfg *arg);


//struct workqueue_struct *taos_wq;
//struct work_struct *taos_work;
//#define DEBUG_IN_KERNTL 

DECLARE_WAIT_QUEUE_HEAD(waitqueue_read);//iVIZM

static unsigned int ReadEnable = 0;//iVIZM
struct ReadData { //iVIZM
    unsigned int data;
    unsigned int interrupt;
};
struct ReadData readdata[2];//iVIZM

// first device number
static dev_t taos_dev_number;

// workqueue struct
//static struct workqueue_struct *taos_wq; //iVIZM

// class structure for this device
struct class *taos_class;

// module device table
static struct i2c_device_id taos_idtable[] = {
    {TAOS_DEVICE_ID, 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, taos_idtable);

// board and address info   iVIZM
struct i2c_board_info taos_board_info[] = {
    {I2C_BOARD_INFO(TAOS_DEVICE_ID, TAOS_DEVICE_ADDR2),},
};

unsigned short const taos_addr_list[2] = {TAOS_DEVICE_ADDR2, I2C_CLIENT_END};//iVIZM

// client and device
struct i2c_client *my_clientp;
struct i2c_client *bad_clientp[TAOS_MAX_NUM_DEVICES];
static int num_bad = 0;
static int device_found = 0;
//iVIZM
static char pro_buf[4]; //iVIZM
static int mcount = 0; //iVIZM
static bool pro_ft = false; //by clli2
static bool flag_prox_debug = false;
static bool flag_als_debug  = false;
static bool flag_irq        = false;
static bool flag_just_open_light = false;
static unsigned int taos_als_poll_delay = 1000;
static unsigned int prox_debug_delay_time = 10;
static bool flag_lock_wakelock = false;
u16 status = 0;
static int ALS_ON;
static int als_poll_time_mul  = 1;
static unsigned char reg_addr = 0;
static bool wakeup_from_sleep = false;

// ZTEMT ADD by zhubing
// modify for input filter the same data
static int last_proximity_data = -1;
static int last_als_data       = -1;
// ZTEMT ADD by zhubing END

// driver definition
static struct i2c_driver taos_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = TAOS_DEVICE_NAME,
    },
    .id_table = taos_idtable,
    .probe = taos_probe,
#ifdef CONFIG_PM_SLEEP //by clli2
    .resume = taos_resume,
    .suspend = taos_suspend,
#endif
    .remove = __devexit_p(taos_remove),
};

// per-device data
struct taos_data {
    struct i2c_client *client;
    struct cdev cdev;
    unsigned int addr;
    struct input_dev *input_dev;//iVIZM
    //struct work_struct work;//iVIZM
    struct delayed_work work;//iVIZM
	struct work_struct irq_work;
	struct workqueue_struct *irq_work_queue;
    struct wake_lock taos_wake_lock;//iVIZM
    struct device *class_dev;
	struct delayed_work als_poll_work;
    char taos_id;
    char taos_name[TAOS_ID_NAME_SIZE];
    struct semaphore update_lock;
    char valid;
    //int working;
    unsigned long last_updated;
} *taos_datap;

// file operations
static struct file_operations taos_fops = {
    .owner = THIS_MODULE,
    .open = taos_open,
    .release = taos_release,
    .read = taos_read,
    .write = taos_write,
    .unlocked_ioctl = taos_ioctl,
    .llseek = taos_llseek,
};

// device configuration by clli2
struct taos_cfg *taos_cfgp;
static u32 calibrate_target_param = 300000;
static u16 als_time_param = 200;
static u16 scale_factor_param_prox = 6;    
static u16 scale_factor_param_als = 20;
static u16 gain_trim_param = 512;          //NULL
static u8 filter_history_param = 3;        //NULL
static u8 filter_count_param = 1;          //NULL
/* gain_param  00--1X, 01--8X, 10--16X, 11--120X
 */
static u8 gain_param = 0;                  //same as prox-gain_param 1:0 8X
static u16 prox_calibrate_hi_param = 230;
static u16 prox_calibrate_lo_param = 150;
static u16 prox_threshold_hi_param = 800;
static u16 prox_threshold_lo_param = 750;
static u16 als_threshold_hi_param  = 3000; 
static u16 als_threshold_lo_param  = 10;   
static u8  prox_int_time_param     = 0xCD; // time of the ALS ADC TIME, TIME = (255 - prox_int_time_param) * 2.72ms
static u8  prox_adc_time_param     = 0xFF; // time of the PRO ADC TIME, TIME = (255 - prox_int_time_param) * 2.72ms
static u8  prox_wait_time_param    = 0xFF; // time of the    Wait TIME, TIME = (255 - prox_int_time_param) * 2.72ms
/*7~4->pls,3~0->als*/
static u8  prox_intr_filter_param  = 0x33; // Int filter, Bit7--Bit4:PROX  Bit3--Bit0:ALS
static u8  prox_config_param       = 0x00; // wait long time disable
/*pulse/62.5Khz  less  32 recommand*/
static u8  prox_pulse_cnt_param    = 0x03; //PROX LED pluse count to send for each measure 0x00--0xff:0--255
/* 7:6 11->100ma        00->12.5ma
   5:4 01->ch0          10->ch1    11->both
   1:0(als gain ctrol)  1X 8X 16X 128X        */
static u8  prox_gain_param = 0x20;   //50ma     8X
// prox info
struct taos_prox_info prox_cal_info[20];
struct taos_prox_info prox_cur_info;
struct taos_prox_info *prox_cur_infop = &prox_cur_info;
static u8 prox_history_hi = 0;
static u8 prox_history_lo = 0;
static struct timer_list prox_poll_timer;
static int PROX_ON = 0;
static int device_released = 0;
static u16 sat_als = 0;
static u16 sat_prox = 0;



// device reg init values
u8 taos_triton_reg_init[16] = {0x00,0xFF,0XFF,0XFF,0X00,0X00,0XFF,0XFF,0X00,0X00,0XFF,0XFF,0X00,0X00,0X00,0X00};

// lux time scale
struct time_scale_factor  {
    u16 numerator;
    u16 denominator;
    u16 saturation;
};
struct time_scale_factor TritonTime = {1, 0, 0};
struct time_scale_factor *lux_timep = &TritonTime;

// gain table
u8 taos_triton_gain_table[] = {1, 8, 16, 120};

// lux data
struct lux_data {
    u16 ratio;
    u16 clear;
    u16 ir;
};
struct lux_data TritonFN_lux_data[] = {
    { 9830,  8320,  15360 },
    { 12452, 10554, 22797 },
    { 14746, 6234,  11430 },
    { 17695, 3968,  6400  },
    { 0,     0,     0     }
};
struct lux_data *lux_tablep = TritonFN_lux_data;
static int lux_history[TAOS_FILTER_DEPTH] = {-ENODATA, -ENODATA, -ENODATA};//iVIZM

static int taos_get_data(void);
#ifdef CONFIG_FEATURE_ZTEMT_SENSORS_LOG_ON
static ssize_t attr_set_prox_led_pluse_cnt(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    int ret;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    prox_pulse_cnt_param = val;

    if (NULL!=taos_cfgp)
    {
        taos_cfgp->prox_pulse_cnt = prox_pulse_cnt_param;
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0) 
        {
            SENSOR_LOG_ERROR("failed to write the prox_pulse_cnt reg\n");
        }
    }
    else
    {
        SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_prox_led_pluse_cnt(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "prox_led_pluse_cnt is %d\n", taos_cfgp->prox_pulse_cnt);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_als_adc_time(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    int ret;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    prox_int_time_param = 255 - val;

    if (NULL!=taos_cfgp)
    {
        taos_cfgp->prox_int_time = prox_int_time_param;
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), taos_cfgp->prox_int_time))) < 0) 
        {
            SENSOR_LOG_ERROR("failed to write the als_adc_time reg\n");
        }
    }
    else
    {
        SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_als_adc_time(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "als_adc_time is 2.72 * %d ms\n", 255 - taos_cfgp->prox_int_time);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_prox_adc_time(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    int ret;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    prox_adc_time_param = 255 - val;

    if (NULL!=taos_cfgp)
    {
        taos_cfgp->prox_adc_time = prox_adc_time_param;
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x02), taos_cfgp->prox_adc_time))) < 0) 
        {
            SENSOR_LOG_ERROR("failed to write the prox_adc_time reg\n");
        }
    }
    else
    {
        SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_prox_adc_time(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "prox_adc_time is 2.72 * %d ms\n", 255 - taos_cfgp->prox_adc_time);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_wait_time(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    int ret;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    prox_wait_time_param = 255 - val;

    if (NULL!=taos_cfgp)
    {
        taos_cfgp->prox_wait_time = prox_wait_time_param;
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x03), taos_cfgp->prox_wait_time))) < 0) 
        {   
            SENSOR_LOG_ERROR("failed to write the wait_time reg\n");
        }

    }
    else
    {
        SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_wait_time(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "wait_time is 2.72 * %d ms\n", 255 - taos_cfgp->prox_wait_time);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_prox_led_strength_level(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    int ret;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    if (val>4 || val<=0)
    {        
        SENSOR_LOG_ERROR("input error, please input a number 1~~4");
    }
    else
    {
        val = 4 - val;
        prox_gain_param = (prox_gain_param & 0x3F) | (val<<6);

        if (NULL!=taos_cfgp)
        {
            taos_cfgp->prox_gain = prox_gain_param;
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) 
            {
                SENSOR_LOG_ERROR("failed to write the prox_led_strength reg\n");
            }
        }
        else
        {
            SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
        }
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_prox_led_strength_level(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    char *p_led_strength[4] = {"100", "50", "25", "12.5"};
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "prox_led_strength is %s mA\n", p_led_strength[(taos_cfgp->prox_gain) >> 6]);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}


static ssize_t attr_set_als_gain(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    int ret;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    if (val>4 || val<=0)
    {        
        SENSOR_LOG_ERROR("input error, please input a number 1~~4");
    }
    else
    {
        val = val-1;
        prox_gain_param = (prox_gain_param & 0xFC) | val;
        gain_param      = prox_gain_param & 0x03;

        if (NULL!=taos_cfgp)
        {
            taos_cfgp->gain      = gain_param;
            taos_cfgp->prox_gain = prox_gain_param;
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) 
            {
                SENSOR_LOG_ERROR("failed to write the prox_led_strength reg\n");
            }
        }
        else
        {
            SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
        }
    }


    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_als_gain(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    u8 als_gain[4] = {1, 8, 16, 120};
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "als gain is x%d\n", als_gain[taos_cfgp->prox_gain & 0x03]);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_prox_debug_delay(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    prox_debug_delay_time =  val;

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_prox_debug_delay(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "prox_debug_delay_time is %d\n", prox_debug_delay_time);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}


static ssize_t attr_set_prox_debug(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    if (val)
    {
        flag_prox_debug = true;
    }
    else
    {       
        flag_prox_debug = false;
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_prox_debug(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "flag_prox_debug is %s\n", flag_prox_debug? "true" : "false");
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_als_debug(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    if (val)
    {
        flag_als_debug = true;
    }
    else
    {       
        flag_als_debug = false;
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_als_debug(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "flag_prox_debug is %s\n", flag_als_debug? "true" : "false");
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_irq(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    if (val)
    {
        enable_irq(taos_cfgp->als_ps_int);
        flag_irq = true;
    }
    else
    {       
        disable_irq(taos_cfgp->als_ps_int);
        flag_irq = false;
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_irq(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "flag_prox_debug is %s\n", flag_irq? "true" : "false");
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}


static ssize_t attr_set_prox_calibrate(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    int ret;
    int prox_sum = 0, prox_mean = 0, prox_max = 0;
    u8 reg_cntrl = 0;
    u8 reg_val = 0;
    int i = 0;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    if (val>10)
    {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), taos_cfgp->prox_int_time))) < 0)
        {
            SENSOR_LOG_ERROR("failed write prox_int_time reg\n");
            goto prox_calibrate_error;
        }
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x02), taos_cfgp->prox_adc_time))) < 0)
        {
            SENSOR_LOG_ERROR("failed write prox_adc_time reg\n");
            goto prox_calibrate_error;
        }
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x03), taos_cfgp->prox_wait_time))) < 0)
        {
            SENSOR_LOG_ERROR("failed write prox_wait_time reg\n");
            goto prox_calibrate_error;
        }

        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0D), taos_cfgp->prox_config))) < 0)
        {
            SENSOR_LOG_ERROR("failed write prox_config reg\n");
            goto prox_calibrate_error;
        }

        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0)
        {
            SENSOR_LOG_ERROR("failed write prox_pulse_cnt reg\n");
            goto prox_calibrate_error;
        }
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0)
        {
            SENSOR_LOG_ERROR("failed write prox_gain reg\n");
            goto prox_calibrate_error;
        }

        reg_cntrl = reg_val | (TAOS_TRITON_CNTL_PROX_DET_ENBL | TAOS_TRITON_CNTL_PWRON | TAOS_TRITON_CNTL_ADC_ENBL);
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0)
        {
           SENSOR_LOG_ERROR("failed write cntrl reg\n");
           goto prox_calibrate_error;
        }

        prox_sum = 0;
        prox_max = 0;   
        mdelay(30);
        for (i = 0; i < val; i++) 
        {
            if ((ret = taos_prox_poll(&prox_cal_info[i])) < 0)
            {
                printk(KERN_ERR "TAOS: call to prox_poll failed in ioctl prox_calibrate\n");
                return (ret);
            }
            prox_sum += prox_cal_info[i].prox_data;
            if (prox_cal_info[i].prox_data > prox_max)
                prox_max = prox_cal_info[i].prox_data;
            SENSOR_LOG_ERROR("prox get time %d data is %d",i,prox_cal_info[i].prox_data);
            mdelay(30);
        }

        prox_mean = prox_sum/val;
        taos_cfgp->prox_threshold_hi = ((((prox_max - prox_mean) * prox_calibrate_hi_param) + 50)/100) + prox_mean;
        taos_cfgp->prox_threshold_lo = ((((prox_max - prox_mean) * prox_calibrate_lo_param) + 50)/100) + prox_mean;


		if( prox_mean >800 || taos_cfgp->prox_threshold_hi > 1000 || taos_cfgp->prox_threshold_lo > 900)
        {
		    taos_cfgp->prox_threshold_hi = prox_threshold_hi_param;
            taos_cfgp->prox_threshold_lo = prox_threshold_lo_param;	
		    prox_pulse_cnt_param = 0x03;
		    taos_cfgp->prox_pulse_cnt = prox_pulse_cnt_param;
		}
			
        if(taos_cfgp->prox_threshold_hi < 20)
        {
				taos_cfgp->prox_threshold_hi = 33;
            	taos_cfgp->prox_threshold_lo = 22;	
		}

        SENSOR_LOG_ERROR("TAOS:------------ taos_cfgp->prox_threshold_hi = %d\n",taos_cfgp->prox_threshold_hi );
        SENSOR_LOG_ERROR("TAOS:------------ taos_cfgp->prox_threshold_lo = %d\n",taos_cfgp->prox_threshold_lo );
     
        for (i = 0; i < sizeof(taos_triton_reg_init); i++)
        {
            if(i !=11)
            {
                if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|(TAOS_TRITON_CNTRL +i)), taos_triton_reg_init[i]))) < 0)
                {
                    SENSOR_LOG_ERROR("failed write triton_init reg\n");
                    goto prox_calibrate_error;
                }
             }
         }

        input_report_abs(taos_datap->input_dev,ABS_X,taos_cfgp->prox_threshold_hi);
        input_report_abs(taos_datap->input_dev,ABS_Y,taos_cfgp->prox_threshold_lo);
    	input_sync(taos_datap->input_dev);
    }
    else
    {
        SENSOR_LOG_ERROR("your input error, please input a number that bigger than 10\n");
    }
   
prox_calibrate_error:
    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_set_prox_calibrate_hi_param(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }
    
    if (val>0)
    {
        prox_calibrate_hi_param = val;
    }
    else
    {
        SENSOR_LOG_ERROR("you input error, please input a number that bigger than 0\n");
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}


static ssize_t attr_get_prox_calibrate_hi_param(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "prox_calibrate_hi_param is %d\n",prox_calibrate_hi_param);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_prox_calibrate_lo_param(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }
    
    if (val>0)
    {
        prox_calibrate_lo_param = val;
    }
    else
    {
        SENSOR_LOG_ERROR("you input error, please input a number that bigger than 0\n");
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}


static ssize_t attr_get_prox_calibrate_lo_param(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "prox_calibrate_lo_param is %d\n",prox_calibrate_lo_param);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_als_scale_factor_param_prox(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }
    
    if (val>0)
    {
        scale_factor_param_prox = val;
        if (NULL!=taos_cfgp)
        {
            taos_cfgp->scale_factor_prox = scale_factor_param_prox;
        }
        else
        {
            SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
        }
    }
    else
    {
        SENSOR_LOG_ERROR("you input error, please input a number that bigger than 0\n");
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}


static ssize_t attr_get_als_scale_factor_param_prox(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        sprintf(buf, "als_scale_factor_param_prox is %d\n",taos_cfgp->scale_factor_prox);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_set_als_scale_factor_param_als(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }
    
    if (val>0)
    {
        scale_factor_param_als = val;
        if (NULL!=taos_cfgp)
        {
            taos_cfgp->scale_factor_als = scale_factor_param_als;
        }
        else
        {
            SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
        }
    }
    else
    {
        SENSOR_LOG_ERROR("you input error, please input a number that bigger than 0\n");
    }


    SENSOR_LOG_ERROR("exit\n");
	return size;
}


static ssize_t attr_get_als_scale_factor_param_als(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        sprintf(buf, "als_scale_factor_param_als is %d\n",taos_cfgp->scale_factor_als);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}


static ssize_t attr_get_prox_threshold_high(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "prox_threshold_hi is %d\n", taos_cfgp->prox_threshold_hi);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static ssize_t attr_get_prox_threshold_low(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {
        return sprintf(buf, "prox_threshold_lo is %d\n", taos_cfgp->prox_threshold_lo);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

#endif

static ssize_t attr_set_reg_addr(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }

    reg_addr = val;

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_reg_addr(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
   
    SENSOR_LOG_ERROR("enter\n");
    SENSOR_LOG_ERROR("reg_addr = 0x%02X\n",reg_addr);
	return strlen(buf);
    SENSOR_LOG_ERROR("exit\n");

}

static ssize_t attr_set_reg_data(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    int ret;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }
    
    if (100==reg_addr)
    {
        SENSOR_LOG_ERROR("reg addr error!\n");
    }
    else
    {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|reg_addr), val))) < 0)
        {
            SENSOR_LOG_ERROR("failed write reg\n");
        }   
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}


static ssize_t attr_get_reg_data(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    unsigned char i;
    if (100 == reg_addr)
    {
        for (i=0x00; i<=0x0F; i++)
        {
            i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | i));
            SENSOR_LOG_ERROR("reg[0x%02X] = 0x%02X",i,i2c_smbus_read_byte(taos_datap->client));
        }
        for (i=0x11; i<=0x19; i++)
        {
            i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | i));
            SENSOR_LOG_ERROR("reg[0x%02X] = 0x%02X",i,i2c_smbus_read_byte(taos_datap->client));            
        }

        i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | 0x1F));
        SENSOR_LOG_ERROR("reg[0x1F] = 0x%02X",i2c_smbus_read_byte(taos_datap->client));  
    }   
    else
    {
        i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | reg_addr));
        SENSOR_LOG_ERROR("reg[0x%02X] = 0x%02X",reg_addr,i2c_smbus_read_byte(taos_datap->client)); 
    }

	return strlen(buf);
}


static ssize_t attr_get_threshold_hi(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {

	 return sprintf(buf, "%d\n", taos_cfgp->prox_threshold_hi);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}
static ssize_t attr_set_threshold_hi(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }


    if (NULL!=taos_cfgp)
    {
 	    taos_cfgp->prox_threshold_hi = val;   
        
    }
    else
    {
        SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_threshold_lo(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {

	 return sprintf(buf, "%d\n", taos_cfgp->prox_threshold_lo);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}
static ssize_t attr_set_threshold_lo(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    SENSOR_LOG_ERROR("enter\n");
    if (strict_strtoul(buf, 10, &val))
    {
        return -EINVAL;
    }


    if (NULL!=taos_cfgp)
    {
 	    taos_cfgp->prox_threshold_lo = val;   
        
    }
    else
    {
        SENSOR_LOG_ERROR("taos_cfgp is NULL\n");
    }

    SENSOR_LOG_ERROR("exit\n");
	return size;
}

static ssize_t attr_get_prox_value(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
    if (NULL!=taos_cfgp)
    {

	 return sprintf(buf, "%d\n", last_proximity_data%100000);
    }
    else
    {       
        sprintf(buf, "taos_cfgp is NULL\n");
    }
	return strlen(buf);
}

static struct device_attribute attributes[] = {
#ifdef CONFIG_FEATURE_ZTEMT_SENSORS_LOG_ON
	__ATTR(taos_prox_led_pluse_cnt,             0644,   attr_get_prox_led_pluse_cnt,                attr_set_prox_led_pluse_cnt),
	__ATTR(taos_als_adc_time,                   0644,   attr_get_als_adc_time,                      attr_set_als_adc_time),
	__ATTR(taos_prox_adc_time,                  0644,   attr_get_prox_adc_time,                     attr_set_prox_adc_time),
	__ATTR(taos_wait_time,                      0644,   attr_get_wait_time,                         attr_set_wait_time),
	__ATTR(taos_prox_led_strength_level,        0644,   attr_get_prox_led_strength_level,           attr_set_prox_led_strength_level),
	__ATTR(taos_als_gain,                       0644,   attr_get_als_gain,                          attr_set_als_gain),
	__ATTR(taos_prox_debug,                     0644,   attr_get_prox_debug,                        attr_set_prox_debug),
	__ATTR(taos_als_debug,                      0644,   attr_get_als_debug,                         attr_set_als_debug),
	__ATTR(taos_prox_debug_delay,               0644,   attr_get_prox_debug_delay,                  attr_set_prox_debug_delay),   
	__ATTR(taos_irq,                            0644,   attr_get_irq,                               attr_set_irq),       
	__ATTR(taos_prox_calibrate,                 0644,   NULL,                                       attr_set_prox_calibrate),
	__ATTR(taos_prox_calibrate_hi_param,        0644,   attr_get_prox_calibrate_hi_param,           attr_set_prox_calibrate_hi_param),
	__ATTR(taos_prox_calibrate_lo_param,        0644,   attr_get_prox_calibrate_lo_param,           attr_set_prox_calibrate_lo_param),
	__ATTR(taos_als_scale_factor_param_als,     0644,   attr_get_als_scale_factor_param_als,        attr_set_als_scale_factor_param_als),
	__ATTR(taos_als_scale_factor_param_prox,    0644,   attr_get_als_scale_factor_param_prox,       attr_set_als_scale_factor_param_prox),
	__ATTR(taos_prox_threshold_high,            0644,   attr_get_prox_threshold_high,               NULL),
	__ATTR(taos_prox_threshold_low,             0644,   attr_get_prox_threshold_low,                NULL),
#endif
    __ATTR(taos_reg_addr,                       0644,   attr_get_reg_addr,                          attr_set_reg_addr),
    __ATTR(taos_reg_data,                       0644,   attr_get_reg_data,                          attr_set_reg_data),
	__ATTR(th_hi,                               0644,   attr_get_threshold_hi,                      attr_set_threshold_hi),
	__ATTR(th_low,                              0644,   attr_get_threshold_lo,                      attr_set_threshold_lo),
	__ATTR(prox_value,                          0644,   attr_get_prox_value,                        NULL),
};

static int create_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for ( ; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;
}


static void wake_lock_ops(bool enable)
{
    if (enable)
    {
        if (false == flag_lock_wakelock)
        {
            flag_lock_wakelock = true;
            wake_lock(&taos_datap->taos_wake_lock);
        }
    }
    else
    {
        if (true == flag_lock_wakelock)
        {  
            flag_lock_wakelock = false;
            wake_unlock(&taos_datap->taos_wake_lock);
        }
    }
}

static void taos_irq_work_func(struct work_struct * work) //iVIZM
{
    int retry_times = 0;
    int ret;
    SENSOR_LOG_INFO("enter\n");
    if (wakeup_from_sleep)
    {  
        SENSOR_LOG_INFO(" wakeup_from_sleep = true\n");
        mdelay(50);
        wakeup_from_sleep = false;
    }

    for (retry_times=0; retry_times<=50; retry_times++)
    {
        ret = taos_get_data();
        if (ret >= 0)
        {
            break;
        }
        mdelay(20);
    }
    taos_interrupts_clear();
    msleep(100);
    wake_lock_ops(false);
    enable_irq(taos_cfgp->als_ps_int);
    flag_irq = true;
    SENSOR_LOG_INFO(" retry_times = %d\n",retry_times);
}

static irqreturn_t taos_irq_handler(int irq, void *dev_id) //iVIZM
{
    disable_irq_nosync(taos_cfgp->als_ps_int);
    flag_irq = false;
    wake_lock_ops(true);
    SENSOR_LOG_INFO("enter\n");
    queue_work(taos_datap->irq_work_queue, &taos_datap->irq_work);
    SENSOR_LOG_INFO("exit\n");

    return IRQ_HANDLED;
}

static int taos_get_data(void)//iVIZM
{
    int ret = 0;

    status = i2c_smbus_read_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_STATUS));

    if((status & 0x20) == 0x20) 
    {
        ret = taos_prox_threshold_set();
        if(ret >= 0)
        {
            ReadEnable = 1;
        }
    } 
	else 
    {
        if((status & 0x10) == 0x10) 
        {   

        } 
    	else
    	{
            SENSOR_LOG_ERROR("TAOS:================= status = %d\n",status);
    	}
    }
    return ret;
}


static int taos_interrupts_clear(void)//iVIZM
{
    int ret = 0;
    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_CMD_SPL_FN|0x07)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte(2) failed in taos_work_func()\n");
        return (ret);
    }
    return ret;
}

static int taos_als_get_data(void)//iVIZM
{
    int ret = 0;
    u8 reg_val;
    int lux_val = 0;
    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_data\n");
        return (ret);
    }
    reg_val = i2c_smbus_read_byte(taos_datap->client);
    if ((reg_val & (TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PWRON)) != (TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PWRON))
        return -ENODATA;
    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_STATUS)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_data\n");
        return (ret);
    }
    reg_val = i2c_smbus_read_byte(taos_datap->client);
    if ((reg_val & TAOS_TRITON_STATUS_ADCVALID) != TAOS_TRITON_STATUS_ADCVALID)
        return -ENODATA;

    if ((lux_val = taos_get_lux()) < 0)
    {
        printk(KERN_ERR "TAOS: call to taos_get_lux() returned error %d in ioctl als_data\n", lux_val);
    }

    if (lux_val<TAOS_ALS_GAIN_DIVIDE && gain_param!=TAOS_ALS_GAIN_8X)
    {
        taos_als_gain_set(TAOS_ALS_GAIN_8X);
    }
    else
    {
        if (lux_val>TAOS_ALS_GAIN_DIVIDE && gain_param!=TAOS_ALS_GAIN_1X)
        {
            taos_als_gain_set(TAOS_ALS_GAIN_1X);
        }
    }
    
    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_ALS_TIME)))) < 0)
    {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_data\n");
        return (ret);
    }

    reg_val = i2c_smbus_read_byte(taos_datap->client);

    if (flag_als_debug)
    {        
        SENSOR_LOG_ERROR(KERN_INFO "reg_val = %d lux_val = %d\n",reg_val,lux_val);
    }

    if (reg_val != prox_int_time_param)
    {
        lux_val = (lux_val * (101 - (0XFF - reg_val)))/20;
    }

    lux_val = taos_lux_filter(lux_val);

    if (lux_val>10000)
    {
        lux_val = 10000;
    }

    if (-1 == last_als_data)
    {
        last_als_data = lux_val;
    }
    else
    {
        if (flag_just_open_light)
        {
            flag_just_open_light = false;
            if (lux_val == last_als_data)
            {
                lux_val += 100000;
            }
        }
        last_als_data = lux_val;
    }

    if (flag_als_debug)
    {        
        SENSOR_LOG_ERROR(KERN_INFO "lux_val = %d",lux_val);
    }

	input_report_abs(taos_datap->input_dev,ABS_MISC,lux_val);
	input_sync(taos_datap->input_dev);
    return ret;
}

static int taos_prox_threshold_set(void)//iVIZM
{
    int i,ret = 0;
    u8 chdata[6];
    u16 proxdata = 0;
    u16 cleardata = 0;
	int data = 0;

    for (i = 0; i < 6; i++) {
        chdata[i] = (i2c_smbus_read_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CMD_WORD_BLK_RW| (TAOS_TRITON_ALS_CHAN0LO + i))));
    }
    cleardata = chdata[0] + chdata[1]*256;
    proxdata = chdata[4] + chdata[5]*256;
#ifdef DEBUG_IN_KERNTL

	input_report_abs(taos_datap->input_dev,ABS_DISTANCE,proxdata);
    //emmit make error
    PROX_ON = 0;
	data = 0;
	ret = 0;
	pro_buf[0] = 0x0;
    SENSOR_LOG_INFO( "----------%s: %d: proxdata = %d",__func__,__LINE__,proxdata);
    return 0;
#else 
	if (pro_ft || flag_prox_debug){ //by clli2 for debug
        pro_buf[0] = 0xff;
        pro_buf[1] = 0xff;
        pro_buf[2] = 0xff;
        pro_buf[3] = 0xff;

		pro_ft = false;
        
        for( mcount=0; mcount<4; mcount++ )
        {
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x08) + mcount, pro_buf[mcount]))) < 0)
            {
                 printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in taos prox threshold set\n");
                 return (ret);
            }
        }      
        if (pro_ft)
        {
            SENSOR_LOG_INFO( "----------%s: %d: init the prox threshold",__func__,__LINE__);
        }
        if (flag_prox_debug)
        {
            mdelay(prox_debug_delay_time);
            SENSOR_LOG_INFO( "----------%s: %d: proxdata = %d",__func__,__LINE__,proxdata);
        }
        
	} else if (proxdata < taos_cfgp->prox_threshold_lo ) { //10cm
        pro_buf[0] = 0x0;
        pro_buf[1] = 0x0;
        pro_buf[2] = taos_cfgp->prox_threshold_hi & 0x0ff;
        pro_buf[3] = taos_cfgp->prox_threshold_hi >> 8;
		data = proxdata ;//10;
		// ZTEMT ADD by zhubing 2012-7-25 N8010
        // modify for input filter the same data

        if (-1 == last_proximity_data)
        {
            last_proximity_data = data;
        }
        else
        {
            if (data == last_proximity_data)
            {
                data += 100000;
            }
            last_proximity_data = data;
        }

        // ZTEMT ADD by zhubing 2012-7-25 N8010 END
		SENSOR_LOG_INFO( "----------%s: %d: Far!!! data = %d\n",__func__,__LINE__,data);
        input_report_abs(taos_datap->input_dev,ABS_DISTANCE,data);
    } else if (proxdata > taos_cfgp->prox_threshold_hi ){ //3cm
        if (cleardata > ((sat_als*80)/100)){
			printk(KERN_ERR "TAOS: %u <= %u*0.8 int data\n",proxdata,sat_als);
            return -ENODATA;
        }
        pro_buf[0] = taos_cfgp->prox_threshold_lo & 0x0ff;
        pro_buf[1] = taos_cfgp->prox_threshold_lo >> 8;
        pro_buf[2] = 0xff;
        pro_buf[3] = 0xff;
	    data = proxdata ;//3;
        if (-1 == last_proximity_data)
        {
            last_proximity_data = data;
        }
        else
        {
            if (data == last_proximity_data)
            {
                data += 100000;
            }
            last_proximity_data = data;
        }
		SENSOR_LOG_INFO( "----------%s: %d: Near!!! data = %d\n",__func__,__LINE__,data);
	    input_report_abs(taos_datap->input_dev,ABS_DISTANCE,data);
    }
	else{
	    SENSOR_LOG_ERROR("TAOS:================= proxdata = %d,not in range\n",proxdata);	
    }


	input_sync(taos_datap->input_dev);
    for( mcount=0; mcount<4; mcount++ ) { 
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x08) + mcount, pro_buf[mcount]))) < 0) {
             printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in taos prox threshold set\n");
             return (ret);
        }
    }

    return ret;
#endif
}

// driver init
static int __init taos_init(void) {
    int ret = 0, i = 0, k = 0;
    struct i2c_adapter *my_adap;
	
    gpio_tlmm_config(GPIO_CFG(ALS_PS_GPIO, 0,GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    if ((ret = (alloc_chrdev_region(&taos_dev_number, 0, TAOS_MAX_NUM_DEVICES, TAOS_DEVICE_NAME))) < 0) {
        printk(KERN_ERR "TAOS: alloc_chrdev_region() failed in taos_init()\n");
        return (ret);
    }
    taos_class = class_create(THIS_MODULE, "taos");
    taos_datap = kmalloc(sizeof(struct taos_data), GFP_KERNEL);
    if (!taos_datap) {
        printk(KERN_ERR "TAOS: kmalloc for struct taos_data failed in taos_init()\n");
        return -ENOMEM;
    }
    memset(taos_datap, 0, sizeof(struct taos_data));
    cdev_init(&taos_datap->cdev, &taos_fops);
    taos_datap->cdev.owner = THIS_MODULE;
    if ((ret = (cdev_add(&taos_datap->cdev, taos_dev_number, 1))) < 0) {
        printk(KERN_ERR "TAOS: cdev_add() failed in taos_init()\n");
        //return (ret);
    }
    wake_lock_init(&taos_datap->taos_wake_lock, WAKE_LOCK_SUSPEND, "taos-wake-lock");
    taos_datap->class_dev = device_create(taos_class, NULL, MKDEV(MAJOR(taos_dev_number), 0), &taos_driver ,"taos");
    if (IS_ERR(taos_datap->class_dev)) 
    {
		ret = PTR_ERR(taos_datap->class_dev);
		goto find_device_failed;
	}


    if ((ret = (i2c_add_driver(&taos_driver))) < 0) {
        printk(KERN_ERR "TAOS: i2c_add_driver() failed in taos_init()\n");
	    goto driver_register_failed;
        return (ret);
    }
    for (i = 2; i <= I2C_MAX_ADAPTERS; i++) {
        my_adap = i2c_get_adapter(i);
        if (my_adap == NULL) 
        {
            SENSOR_LOG_INFO( "----------%s: %d: my_adap(%d) = NULL\n",__func__,__LINE__,i);
            goto find_device_failed;
            //break;
        }
        my_clientp = i2c_new_probed_device(my_adap, &taos_board_info[0], taos_addr_list,NULL);
        if ((my_clientp) && (!device_found)) {
            bad_clientp[num_bad] = my_clientp;
            num_bad++;
        }
        if (device_found) 
            break;
        if (num_bad) {
            for (k = 0; k < num_bad; k++)
                i2c_unregister_device(bad_clientp[k]);
            num_bad = 0;
        }
        if (device_found) 
            break;
    }
    return (ret);
/*by clli2 for phone failed to start if  IC been removed*/
find_device_failed:
driver_register_failed:
	device_destroy(taos_class, MKDEV(MAJOR(taos_dev_number), 0));
	wake_lock_destroy(&taos_datap->taos_wake_lock);
	cdev_del(&taos_datap->cdev);
    class_destroy(taos_class);
    kfree(taos_datap);
    return (ret);
}

// driver exit
static void __exit taos_exit(void) {
    if (my_clientp)
        i2c_unregister_device(my_clientp);
    i2c_del_driver(&taos_driver);
    unregister_chrdev_region(taos_dev_number, TAOS_MAX_NUM_DEVICES);
    device_destroy(taos_class, MKDEV(MAJOR(taos_dev_number), 0));
    cdev_del(&taos_datap->cdev);
    class_destroy(taos_class);
    disable_irq(taos_cfgp->als_ps_int);//iVIZM
    flag_irq = false;
    kfree(taos_datap);
}

// client probe
static int taos_probe(struct i2c_client *clientp, const struct i2c_device_id *idp) {
    int ret = 0;
    int i = 0;
    int chip_id = -1;
    unsigned char buf[TAOS_MAX_DEVICE_REGS];
    char *device_name;

    chip_id = i2c_smbus_read_byte_data(clientp, (TAOS_TRITON_CMD_REG | (TAOS_TRITON_CNTRL + 0x12))); //iVIZM
    /*TSL27711=0x00 TSL27713=0x09 TMD27711=0x20 TMD27713=0x29 	2011.09.07*/
	SENSOR_LOG_ERROR(" TAOS chip_id = %x TMD27713=30,TMD27723=39\n",chip_id);	
	//printk(" TAOS ID reg = %d\n",(TAOS_TRITON_CMD_REG | (TAOS_TRITON_CNTRL + 0x12)));	
    if (device_found)
        return -ENODEV;
    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
        printk(KERN_ERR "TAOS: taos_probe() - i2c smbus byte data functions unsupported\n");
        return -EOPNOTSUPP;
    }

    #ifdef CONFIG_ZTE_DEVICE_INFO_SHOW
    zte_device_info_sys_init("p-sensor", &zte_device_info_get);
    #endif

    taos_datap->client = clientp;
    i2c_set_clientdata(clientp, taos_datap);
    INIT_WORK(&(taos_datap->irq_work),taos_irq_work_func);
	sema_init(&taos_datap->update_lock,1);
   taos_datap->input_dev = input_allocate_device();//iVIZM
   if (taos_datap->input_dev == NULL) {
       return -ENOMEM;
   }
   
   taos_datap->input_dev->name = TAOS_INPUT_NAME;
   taos_datap->input_dev->id.bustype = BUS_I2C;
   _set_bit(EV_ABS,taos_datap->input_dev->evbit);
   input_set_abs_params(taos_datap->input_dev, ABS_DISTANCE, 0, 65535, 0, 0);
   input_set_abs_params(taos_datap->input_dev, ABS_MISC, 0, 65535, 0, 0);
   input_set_abs_params(taos_datap->input_dev, ABS_VOLUME, 0, 65535, 0, 0);
   input_set_abs_params(taos_datap->input_dev, ABS_X, 0, 65535, 0, 0);
   input_set_abs_params(taos_datap->input_dev, ABS_Y, 0, 65535, 0, 0);
   ret = input_register_device(taos_datap->input_dev);
    for (i = 0; i < TAOS_MAX_DEVICE_REGS; i++) {
        if ((ret = (i2c_smbus_write_byte(clientp, (TAOS_TRITON_CMD_REG | (TAOS_TRITON_CNTRL + i))))) < 0) {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte() to control reg failed in taos_probe()\n");
            return(ret);
        }
        buf[i] = i2c_smbus_read_byte(clientp);
    }
    if ((ret = taos_device_name(buf, &device_name)) == 0) {
        printk(KERN_ERR "TAOS: chip id that was read found mismatched by taos_device_name(), in taos_probe()\n");
        return -ENODEV;
    }
    if (strcmp(device_name, TAOS_DEVICE_ID)) {
        printk(KERN_ERR "TAOS: chip id that was read does not match expected id in taos_probe()\n");
        return -ENODEV;
    }
    else {
        SENSOR_LOG_ERROR( "TAOS: chip id of %s that was read matches expected id in taos_probe()\n", device_name);
        device_found = 1;
    }
    if ((ret = (i2c_smbus_write_byte(clientp, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL)))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte() to control reg failed in taos_probe()\n");
        return(ret);
    }
    strlcpy(clientp->name, TAOS_DEVICE_ID, I2C_NAME_SIZE);
    strlcpy(taos_datap->taos_name, TAOS_DEVICE_ID, TAOS_ID_NAME_SIZE);
    taos_datap->valid = 0;
    if (!(taos_cfgp = kmalloc(sizeof(struct taos_cfg), GFP_KERNEL))) {
        printk(KERN_ERR "TAOS: kmalloc for struct taos_cfg failed in taos_probe()\n");
        return -ENOMEM;
    }
    taos_cfgp->calibrate_target = calibrate_target_param;
    taos_cfgp->als_time = als_time_param;
    taos_cfgp->scale_factor_als = scale_factor_param_als;
	taos_cfgp->scale_factor_prox = scale_factor_param_prox;
    taos_cfgp->gain_trim = gain_trim_param;
    taos_cfgp->filter_history = filter_history_param;
    taos_cfgp->filter_count = filter_count_param;
    taos_cfgp->gain = gain_param;
    taos_cfgp->als_threshold_hi = als_threshold_hi_param;//iVIZM
    taos_cfgp->als_threshold_lo = als_threshold_lo_param;//iVIZM
    taos_cfgp->prox_threshold_hi = prox_threshold_hi_param;
    taos_cfgp->prox_threshold_lo = prox_threshold_lo_param;
    taos_cfgp->prox_int_time = prox_int_time_param;
    taos_cfgp->prox_adc_time = prox_adc_time_param;
    taos_cfgp->prox_wait_time = prox_wait_time_param;
    taos_cfgp->prox_intr_filter = prox_intr_filter_param;
    taos_cfgp->prox_config = prox_config_param;
    taos_cfgp->prox_pulse_cnt = prox_pulse_cnt_param;
    taos_cfgp->prox_gain = prox_gain_param;
    sat_als = (256 - taos_cfgp->prox_int_time) << 10;
    sat_prox = (256 - taos_cfgp->prox_adc_time) << 10;

    /*dmobile ::power down for init ,Rambo liu*/
    SENSOR_LOG_ERROR("TAOS:Rambo::light sensor will pwr down \n");
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x00), 0x00))) < 0) {
        printk(KERN_ERR "TAOS:Rambo, i2c_smbus_write_byte_data failed in power down\n");
        return (ret);
    }

    taos_datap->irq_work_queue = create_singlethread_workqueue("taos_work_queue");
    if (!taos_datap->irq_work_queue)
    {
        ret = -ENOMEM;
        SENSOR_LOG_INFO( "---------%s: %d: cannot create work taos_work_queue, err = %d",__func__,__LINE__,ret);
        return ret;
    }

    ret = gpio_request(ALS_PS_GPIO,"ALS_PS_INT");
    if (ret < 0) {
        printk("TAOS:failed to request GPIO:%d,ERRNO:%d\n",(int)ALS_PS_GPIO,ret);
       return(ret);
    }
    taos_cfgp->als_ps_int = gpio_to_irq(ALS_PS_GPIO);	
    ret = request_irq(taos_cfgp->als_ps_int,taos_irq_handler, IRQ_TYPE_EDGE_FALLING, "taos_irq",taos_datap);//iVIZM
    if (ret != 0) {
        gpio_free(ALS_PS_GPIO);
        return(ret);
    }
    
    ret = create_sysfs_interfaces(taos_datap->class_dev);
    if (ret<0)
    {
        SENSOR_LOG_ERROR("failed to create sysfs\n");
        return(ret);
    }

    disable_irq(taos_cfgp->als_ps_int);
    flag_irq = false;
    INIT_DELAYED_WORK(&taos_datap->als_poll_work, taos_als_poll_work_func);
    return (ret);
}
#ifdef CONFIG_PM_SLEEP
//don't move these pm blew to ioctl
//resume  
static int taos_resume(struct i2c_client *client) {
	int ret = 0;
    SENSOR_LOG_INFO("enter\n");
	if(1 == PROX_ON)
    {
        SENSOR_LOG_INFO( "----------%s: %d: disable irq wakeup\n",__func__,__LINE__);
		ret = disable_irq_wake(taos_cfgp->als_ps_int);
	}
    if(ret < 0)
		printk(KERN_ERR "TAOS: disable_irq_wake failed\n");
    SENSOR_LOG_INFO("eixt\n");
    return ret ;
}

//suspend  
static int taos_suspend(struct i2c_client *client, pm_message_t mesg) {
	int ret = 0;
    SENSOR_LOG_INFO("enter\n");
	if(1 == PROX_ON)
    {
        SENSOR_LOG_INFO( "----------%s: %d: enable irq wakeup\n",__func__,__LINE__);
       	ret = enable_irq_wake(taos_cfgp->als_ps_int);
    }
	if(ret < 0)
    {
		printk(KERN_ERR "TAOS: enable_irq_wake failed\n");
    }

    wakeup_from_sleep = true;

    SENSOR_LOG_INFO("eixt\n");
    return ret ;
}

#endif
// client remove
static int __devexit taos_remove(struct i2c_client *client) {
    int ret = 0;

    return (ret);
}

// open
static int taos_open(struct inode *inode, struct file *file) {
    struct taos_data *taos_datap;
    int ret = 0;
    device_released = 0;

    taos_datap = container_of(inode->i_cdev, struct taos_data, cdev);
    if (strcmp(taos_datap->taos_name, TAOS_DEVICE_ID) != 0) {
        //printk(KERN_ERR "TAOS: device name incorrect during taos_open(), get %s\n", taos_datap->taos_name);
        printk(KERN_ERR "TAOS: device name incorrect during taos_open()\n");
        ret = -ENODEV;
    }
    memset(readdata, 0, sizeof(struct ReadData)*2);//iVIZM
    return (ret);
}

// release
static int taos_release(struct inode *inode, struct file *file) {
    struct taos_data *taos_datap;
    int ret = 0;
    device_released = 1;
    PROX_ON = 0;
    prox_history_hi = 0;
    prox_history_lo = 0;
    taos_datap = container_of(inode->i_cdev, struct taos_data, cdev);
    if (strcmp(taos_datap->taos_name, TAOS_DEVICE_ID) != 0) {
        printk(KERN_ERR "TAOS: device name incorrect during taos_release(), get %s\n", taos_datap->taos_name);
        ret = -ENODEV;
    }
    return (ret);
}

// read
static ssize_t taos_read(struct file *file, char *buf, size_t count, loff_t *ppos) {
    unsigned long flags;
    int realmax;
    int err;
    if((!ReadEnable) && (file->f_flags & O_NONBLOCK))
        return -EAGAIN;
    local_save_flags(flags);
    local_irq_disable();

    realmax = 0;
    if (down_interruptible(&taos_datap->update_lock))
        return -ERESTARTSYS;
    if (ReadEnable > 0) {
        if (sizeof(struct ReadData)*2 < count)
            realmax = sizeof(struct ReadData)*2;
        else
            realmax = count;
        err = copy_to_user(buf, readdata, realmax);
        if (err) 
            return -EAGAIN;
        ReadEnable = 0;
    }
    up(&taos_datap->update_lock);
    memset(readdata, 0, sizeof(struct ReadData)*2);
    local_irq_restore(flags);
    return realmax;
}

// write
static ssize_t taos_write(struct file *file, const char *buf, size_t count, loff_t *ppos) {
    struct taos_data *taos_datap;
    u8 i = 0, xfrd = 0, reg = 0;
    u8 my_buf[TAOS_MAX_DEVICE_REGS];
    int ret = 0;
    if ((*ppos < 0) || (*ppos >= TAOS_MAX_DEVICE_REGS) || ((*ppos + count) > TAOS_MAX_DEVICE_REGS)) {
        printk(KERN_ERR "TAOS: reg limit check failed in taos_write()\n");
        return -EINVAL;
    }
    reg = (u8)*ppos;
    if ((ret =  copy_from_user(my_buf, buf, count))) {
        printk(KERN_ERR "TAOS: copy_to_user failed in taos_write()\n");
        return -ENODATA;
    }
    taos_datap = container_of(file->f_dentry->d_inode->i_cdev, struct taos_data, cdev);
    while (xfrd < count) {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | reg), my_buf[i++]))) < 0) {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in taos_write()\n");
            return (ret);
        }
        reg++;
        xfrd++;
     }
     return ((int)xfrd);
}

// llseek
static loff_t taos_llseek(struct file *file, loff_t offset, int orig) {
    int ret = 0;
    loff_t new_pos = 0;
    if ((offset >= TAOS_MAX_DEVICE_REGS) || (orig < 0) || (orig > 1)) {
        printk(KERN_ERR "TAOS: offset param limit or origin limit check failed in taos_llseek()\n");
        return -EINVAL;
    }
    switch (orig) {
    case 0:
        new_pos = offset;
        break;
    case 1:
        new_pos = file->f_pos + offset;
        break;
    default:
        return -EINVAL;
        break;
    }
    if ((new_pos < 0) || (new_pos >= TAOS_MAX_DEVICE_REGS) || (ret < 0)) {
        printk(KERN_ERR "TAOS: new offset limit or origin limit check failed in taos_llseek()\n");
        return -EINVAL;
    }
    file->f_pos = new_pos;
    return new_pos;
}

// ioctls
static long taos_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    long ret = 0;
    switch (cmd) {
        case TAOS_IOCTL_ALS_ON:
        {
            ret = taos_sensors_als_poll_on();
            break;
        }

        case TAOS_IOCTL_ALS_OFF:
        {
            ret = taos_sensors_als_poll_off();
            break;
        }

        case TAOS_IOCTL_CONFIG_GET:
        {
            ret = taos_config_get((struct taos_cfg *)arg);
            break;
        }

        case TAOS_IOCTL_PROX_ON:
        {
            ret = taos_prox_on();
            break;
        }
        
        case TAOS_IOCTL_PROX_OFF:
        {
            ret = taos_prox_off();
            break;
        }

        case TAOS_IOCTL_PROX_CALIBRATE:
        {
            ret = taos_prox_calibrate();
            break;
        }

        case TAOS_IOCTL_ALS_SET_DELAY:
        {
            taos_als_delay_set((int *)arg);
            break;
        }

        default:
        {
            ret = -EINVAL;
            break;
        }
    }
    return (ret);
}

// read/calculate lux value
static int taos_get_lux(void) {
    int raw_clear = 0, raw_ir = 0, raw_lux = 0;
    u32 lux = 0;
    u32 ratio = 0;
    u8 dev_gain = 0;
    u16 Tint = 0;
    struct lux_data *p;
    int ret = 0;
    u8 chdata[4];
    int tmp = 0, i = 0;

    for (i = 0; i < 4; i++) {
        if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | (TAOS_TRITON_ALS_CHAN0LO + i))))) < 0) {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte() to chan0/1/lo/hi reg failed in taos_get_lux()\n");
            return (ret);
        }
        chdata[i] = i2c_smbus_read_byte(taos_datap->client);
    }

    tmp = (taos_cfgp->als_time + 25)/50;            //if atime =100  tmp = (atime+25)/50=2.5   tine = 2.7*(256-atime)=  412.5
    TritonTime.numerator = 1;
    TritonTime.denominator = tmp;

    tmp = 300 * taos_cfgp->als_time;               //tmp = 300*atime  400
    if(tmp > 65535)
        tmp = 65535;
    TritonTime.saturation = tmp;
    raw_clear = chdata[1];
    raw_clear <<= 8;
    raw_clear |= chdata[0];
    raw_ir    = chdata[3];
    raw_ir    <<= 8;
    raw_ir    |= chdata[2];

    raw_clear *= (taos_cfgp->scale_factor_als );
    raw_ir *= (taos_cfgp->scale_factor_prox );

    if(raw_ir > raw_clear) {
        raw_lux = raw_ir;
        raw_ir = raw_clear;
        raw_clear = raw_lux;
    }
    dev_gain = taos_triton_gain_table[taos_cfgp->gain & 0x3];
    if(raw_clear >= lux_timep->saturation)
        return(TAOS_MAX_LUX);
    if(raw_ir >= lux_timep->saturation)
        return(TAOS_MAX_LUX);
    if(raw_clear == 0)
        return(0);
    if(dev_gain == 0 || dev_gain > 127) {
        printk(KERN_ERR "TAOS: dev_gain = 0 or > 127 in taos_get_lux()\n");
        return -1;
    }
    if(lux_timep->denominator == 0) {
        printk(KERN_ERR "TAOS: lux_timep->denominator = 0 in taos_get_lux()\n");
        return -1;
    }
    ratio = (raw_ir<<15)/raw_clear;
    for (p = lux_tablep; p->ratio && p->ratio < ratio; p++);
	#ifdef WORK_UES_POLL_MODE
    if(!p->ratio) {//iVIZM
        if(lux_history[0] < 0)
            return 0;
        else
            return lux_history[0];
    }
	#endif
    Tint = taos_cfgp->als_time;
    raw_clear = ((raw_clear*400 + (dev_gain>>1))/dev_gain + (Tint>>1))/Tint;
    raw_ir = ((raw_ir*400 +(dev_gain>>1))/dev_gain + (Tint>>1))/Tint;
    lux = ((raw_clear*(p->clear)) - (raw_ir*(p->ir)));
    lux = (lux + 32000)/64000;
    if(lux > TAOS_MAX_LUX) {
        lux = TAOS_MAX_LUX;
    }
    return(lux);
}

static int taos_lux_filter(int lux)
{
    static u8 middle[] = {1,0,2,0,0,2,0,1};
    int index;

    lux_history[2] = lux_history[1];
    lux_history[1] = lux_history[0];
    lux_history[0] = lux;

    if(lux_history[2] < 0) { //iVIZM
        if(lux_history[1] > 0)
            return lux_history[1];       
        else 
            return lux_history[0];
    }
    index = 0;
    if( lux_history[0] > lux_history[1] ) 
        index += 4;
    if( lux_history[1] > lux_history[2] ) 
        index += 2;
    if( lux_history[0] > lux_history[2] )
        index++;
    return(lux_history[middle[index]]);
}

// verify device
static int taos_device_name(unsigned char *bufp, char **device_name) {
    if( (bufp[0x12]&0x00)!=0x00)        //TSL27711=0x00 TSL27713=0x09 TMD27711=0x20 TMD27713=0x29    2011.07.21
        return(0);
    if(bufp[0x10]|bufp[0x1a]|bufp[0x1b]|bufp[0x1c]|bufp[0x1d]|bufp[0x1e])
        return(0);
    if(bufp[0x13]&0x0c)
        return(0);
    *device_name="tritonFN";
    return(1);
}

// proximity poll
static int taos_prox_poll(struct taos_prox_info *prxp)
{
    int i = 0, ret = 0; //wait_count = 0;
    u8 chdata[6];

    for (i = 0; i < 6; i++) {
        chdata[i] = (i2c_smbus_read_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CMD_AUTO | (TAOS_TRITON_ALS_CHAN0LO + i))));
    }
    prxp->prox_clear = chdata[1];
    prxp->prox_clear <<= 8;
    prxp->prox_clear |= chdata[0];
    if (prxp->prox_clear > ((sat_als*80)/100))
    {
		printk(KERN_ERR "TAOS: %u <= %u*0.8 poll data\n",prxp->prox_clear,sat_als);
        return -ENODATA;
    }
    prxp->prox_data = chdata[5];
    prxp->prox_data <<= 8;
    prxp->prox_data |= chdata[4];

    return (ret);
}

// prox poll timer function
static void taos_prox_poll_timer_func(unsigned long param) {
    int ret = 0;

    if (!device_released) {
        if ((ret = taos_prox_poll(prox_cur_infop)) < 0) {
            printk(KERN_ERR "TAOS: call to prox_poll failed in taos_prox_poll_timer_func()\n");
            return;
        }
        taos_prox_poll_timer_start();
    }
    return;
}

// start prox poll timer
static void taos_prox_poll_timer_start(void) {
    init_timer(&prox_poll_timer);
    prox_poll_timer.expires = jiffies + (HZ/10);
    prox_poll_timer.function = taos_prox_poll_timer_func;
    add_timer(&prox_poll_timer);
    return;
}

static long taos_config_get(struct taos_cfg *arg)
{
    int ret = 0;
    ret = copy_to_user((struct taos_cfg *)arg, taos_cfgp, sizeof(struct taos_cfg));
    if (ret)
    {
        SENSOR_LOG_INFO("copy_to_user failed\n");
    }
    return ret;
}

static void taos_update_sat_als(void)
{
    u8 reg_val = 0;
    int ret = 0;

    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_ALS_TIME)))) < 0) 
    {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_calibrate\n");
        return;
    }
    
    reg_val = i2c_smbus_read_byte(taos_datap->client);

    sat_als = (256 - reg_val) << 10;
}

static int taos_als_gain_set(unsigned als_gain)
{
    int ret;
    prox_gain_param = (prox_gain_param & 0xFC) | als_gain;
    gain_param      = prox_gain_param & 0x03;

    if (NULL!=taos_cfgp)
    {
        taos_cfgp->gain      = gain_param;
        taos_cfgp->prox_gain = prox_gain_param;
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) 
        {
            SENSOR_LOG_ERROR("failed to write the prox_led_strength reg\n");
            return -EINVAL;
        }
    }
    else
    {
        SENSOR_LOG_ERROR("taos_cfgp is NULL\n");        
        return -EINVAL;
    }

    return ret;
}

static void taos_als_poll_work_func(struct work_struct *work)
{
    taos_als_get_data();
	schedule_delayed_work(&taos_datap->als_poll_work, msecs_to_jiffies(als_poll_time_mul*taos_als_poll_delay));
}

static int taos_sensors_als_poll_on(void) 
{
    int  ret = 0, i = 0;
    u8   reg_val = 0, reg_cntrl = 0;

    SENSOR_LOG_INFO("######## TAOS IOCTL ALS ON #########\n");

    for (i = 0; i < TAOS_FILTER_DEPTH; i++)
    {
        lux_history[i] = -ENODATA;
    }

    if (PROX_ON)
    {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_ALS_TIME), TAOS_ALS_ADC_TIME_WHEN_PROX_ON))) < 0) 
        {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
        }
    }
    else
    {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_ALS_TIME), taos_cfgp->prox_int_time))) < 0) 
        {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
            return (ret);
        }
    }

    reg_val = i2c_smbus_read_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_GAIN));

    //SENSOR_LOG_INFO("reg[0x0F] = 0x%02X\n",reg_val);

    reg_val = reg_val & 0xFC;
    reg_val = reg_val | (taos_cfgp->gain & 0x03);//*16
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_GAIN), reg_val))) < 0)
    {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
        return (ret);
    }

    reg_cntrl = i2c_smbus_read_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL));
    SENSOR_LOG_INFO("reg[0x00] = 0x%02X\n",reg_cntrl);

    reg_cntrl |= (TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_PWRON);
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0) 
    {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
        return (ret);
    }

	schedule_delayed_work(&taos_datap->als_poll_work, msecs_to_jiffies(als_poll_time_mul*taos_als_poll_delay));

    flag_just_open_light = true;

    ALS_ON = 1;

    taos_update_sat_als();

	return ret;
}	

static int taos_sensors_als_poll_off(void)
{
    int  ret = 0, i = 0;
    u8  reg_val = 0;

    SENSOR_LOG_INFO("######## TAOS IOCTL ALS OFF #########\n");

    for (i = 0; i < TAOS_FILTER_DEPTH; i++)
    {   
        lux_history[i] = -ENODATA;
    }

    reg_val = i2c_smbus_read_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL));

    //SENSOR_LOG_INFO("reg[0x00] = 0x%02X\n",reg_val);

    if ((reg_val & TAOS_TRITON_CNTL_PROX_DET_ENBL) == 0x00 && (0 == PROX_ON)) 
    {        
        SENSOR_LOG_INFO("TAOS_TRITON_CNTL_PROX_DET_ENBL = 0\n");
        reg_val = 0x00;
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_val))) < 0) 
        {
           printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_off\n");
           return (ret);
        }

    }

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|TAOS_TRITON_ALS_TIME), 0XFF))) < 0) 
    {
       printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
       return (ret);
    }

    ALS_ON = 0;
    cancel_delayed_work_sync(&taos_datap->als_poll_work);

    taos_update_sat_als();

    return (ret);
} 


static void taos_als_delay_set(int *arg)
{
    int ret = 0;
    int delay_want = 0;

    ret = copy_from_user(&delay_want, (int*)arg,  sizeof(int));
    if (ret)
    {
        SENSOR_LOG_INFO("copy_from_user failed\n");
        return;
    }

    taos_als_poll_delay = delay_want/1000000;

    SENSOR_LOG_INFO("delay set to be %d\n",taos_als_poll_delay);
}



static int taos_prox_on(void)
{
    int  ret = 0;
    u8 reg_cntrl = 0;

    PROX_ON = 1;
    als_poll_time_mul = 2;

    SENSOR_LOG_INFO("######## TAOS IOCTL PROX ON  ######## \n");

    if (ALS_ON)
    {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), TAOS_ALS_ADC_TIME_WHEN_PROX_ON))) < 0) 
        {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
        }
    }
    else
    {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), 0XFF))) < 0) 
        {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
        }
    }

    taos_update_sat_als();

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x02), taos_cfgp->prox_adc_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x03), taos_cfgp->prox_wait_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0C), taos_cfgp->prox_intr_filter))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0D), taos_cfgp->prox_config))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    reg_cntrl = TAOS_TRITON_CNTL_PROX_DET_ENBL | TAOS_TRITON_CNTL_PWRON    | TAOS_TRITON_CNTL_PROX_INT_ENBL | 
		                                         TAOS_TRITON_CNTL_ADC_ENBL | TAOS_TRITON_CNTL_WAIT_TMR_ENBL  ;
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
	pro_ft = true;
    taos_prox_threshold_set();   
    enable_irq(taos_cfgp->als_ps_int);
    flag_irq = true;
    return (ret);
}


static int taos_prox_off(void)
{    
    int ret = 0;
    SENSOR_LOG_INFO("########  TAOS IOCTL PROX OFF  ########\n");
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), 0x00))) < 0) 
    {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_off\n");
        return (ret);
    }

    PROX_ON = 0;
    als_poll_time_mul = 1;

	if (ALS_ON == 1) 
    {
        taos_sensors_als_poll_on();
	}

    cancel_delayed_work_sync(&taos_datap->work);
    disable_irq(taos_cfgp->als_ps_int);
    flag_irq = false;
    return (ret);
}

static int taos_prox_calibrate(void)
{
    int prox_sum = 0, prox_mean = 0, prox_max = 0,prox_min=1023;
    int ret = 0;
    u8  reg_val = 0, reg_cntrl = 0, i = 0;

    if (ALS_ON)
    {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), TAOS_ALS_ADC_TIME_WHEN_PROX_ON))) < 0) 
        {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
        }
    }
    else
    {
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x01), 0XFF))) < 0) 
        {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
        }
    }

    taos_update_sat_als();

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x02), taos_cfgp->prox_adc_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x03), taos_cfgp->prox_wait_time))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0D), taos_cfgp->prox_config))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0F), taos_cfgp->prox_gain))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    if ((ret = (i2c_smbus_write_byte(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_GAIN)))) < 0) 
    {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte failed in ioctl als_on\n");
        return (ret);
    }
    reg_val = i2c_smbus_read_byte(taos_datap->client);

    reg_cntrl = reg_val | (TAOS_TRITON_CNTL_PROX_DET_ENBL | TAOS_TRITON_CNTL_PWRON | TAOS_TRITON_CNTL_ADC_ENBL);
    if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG | TAOS_TRITON_CNTRL), reg_cntrl))) < 0) {
        printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        return (ret);
    }

    prox_sum = 0;
    prox_max = 0;
    prox_min = 1023;
    mdelay(20);
    for (i = 0; i < 20; i++) 
    {
        if ((ret = taos_prox_poll(&prox_cal_info[i])) < 0)
        {
            printk(KERN_ERR "TAOS: call to prox_poll failed in ioctl prox_calibrate\n");
            return (ret);
        }

        prox_sum += prox_cal_info[i].prox_data;

        if (prox_cal_info[i].prox_data > prox_max)
        {
            prox_max = prox_cal_info[i].prox_data;
        }

        if (prox_cal_info[i].prox_data < prox_min)
        {
            prox_min = prox_cal_info[i].prox_data;
        }        
        
        mdelay(30);
    }

    prox_mean = prox_sum/20;

    if(prox_mean< 70 ||prox_min < 10)
    {
        prox_pulse_cnt_param = 0x09;
        taos_cfgp->prox_pulse_cnt = prox_pulse_cnt_param;
        if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|0x0E), taos_cfgp->prox_pulse_cnt))) < 0) 
        {
            printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
        }
        prox_sum = 0;
        prox_max = 0;
        mdelay(20);
        for (i = 0; i < 20; i++)
        {
            if ((ret = taos_prox_poll(&prox_cal_info[i])) < 0) 
            {
                printk(KERN_ERR "TAOS: call to prox_poll failed in ioctl prox_calibrate\n");
                return (ret);
            }
            prox_sum += prox_cal_info[i].prox_data;
            if (prox_cal_info[i].prox_data > prox_max)
            {
                prox_max = prox_cal_info[i].prox_data;
            }
        
            mdelay(30);
        }
        prox_mean = prox_sum/20;
        prox_calibrate_hi_param=200;
        prox_calibrate_lo_param=130;
        taos_cfgp->prox_threshold_hi = ((((prox_max - prox_mean) * prox_calibrate_hi_param) + 50)/100) + prox_mean;
        taos_cfgp->prox_threshold_lo = ((((prox_max - prox_mean) * prox_calibrate_lo_param) + 50)/100) + prox_mean;
	}
	else
    {
        taos_cfgp->prox_threshold_hi = ((((prox_max - prox_mean) * prox_calibrate_hi_param) + 50)/100) + prox_mean;
        taos_cfgp->prox_threshold_lo = ((((prox_max - prox_mean) * prox_calibrate_lo_param) + 50)/100) + prox_mean;
	}
   
    SENSOR_LOG_ERROR("TAOS:------------ taos_cfgp->prox_threshold_hi = %d\n",taos_cfgp->prox_threshold_hi );
    SENSOR_LOG_ERROR("TAOS:------------ taos_cfgp->prox_threshold_lo = %d\n",taos_cfgp->prox_threshold_lo );
	
    if( prox_mean >800 || taos_cfgp->prox_threshold_hi > 1000 || taos_cfgp->prox_threshold_lo > 900)
    {
        taos_cfgp->prox_threshold_hi = prox_threshold_hi_param;
    	taos_cfgp->prox_threshold_lo = prox_threshold_lo_param;	
	}
	
    if(taos_cfgp->prox_threshold_hi < 20)
    {
		taos_cfgp->prox_threshold_hi = 33;
    	taos_cfgp->prox_threshold_lo = 22;	
	}


    for (i = 0; i < sizeof(taos_triton_reg_init); i++)
    {
        if(i !=11)
        {
            if ((ret = (i2c_smbus_write_byte_data(taos_datap->client, (TAOS_TRITON_CMD_REG|(TAOS_TRITON_CNTRL +i)), taos_triton_reg_init[i]))) < 0) 
            {
                printk(KERN_ERR "TAOS: i2c_smbus_write_byte_data failed in ioctl als_on\n");
                return (ret);
            }
         }
     }
    return (ret);
}

MODULE_AUTHOR("John Koshi - Surya Software");
MODULE_DESCRIPTION("TAOS ambient light and proximity sensor driver");
MODULE_LICENSE("GPL");

module_init(taos_init);
module_exit(taos_exit);

