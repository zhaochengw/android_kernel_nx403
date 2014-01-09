#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/slab.h>
#include <linux/types.h>

#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/i2c.h>
#include <linux/semaphore.h>
#include <linux/device.h>

#include <linux/syscalls.h>
#include <asm/uaccess.h>

#include <linux/gpio.h>
//#include <mach/gpio-herring.h>

#include <linux/sched.h>

#include <linux/i2c/drv2605.h>

#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/timer.h>

#include <linux/workqueue.h>
#include <linux/hrtimer.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/wakelock.h>

/*  Current code version: 158 */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Immersion Corp.");
MODULE_DESCRIPTION("Driver for "DEVICE_NAME);

#define LOG_TAG "VIBRATOR"

#define DEBUG_ON //DEBUG SWITCH

#define SENSOR_LOG_FILE__ strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/')+1) : __FILE__

#ifdef  CONFIG_FEATURE_ZTEMT_SENSORS_LOG_ON
#define LOG_ERROR(fmt, args...) printk(KERN_ERR   "[%s] [%s: %d] "  fmt,\
                                              LOG_TAG,__FUNCTION__, __LINE__, ##args)
    #ifdef  DEBUG_ON
#define LOG_INFO(fmt, args...)  printk(KERN_INFO  "[%s] [%s: %d] "  fmt,\
                                              LOG_TAG,__FUNCTION__, __LINE__, ##args)
                                              
#define LOG_DEBUG(fmt, args...) printk(KERN_DEBUG "[%s] [%s: %d] "  fmt,\
                                              LOG_TAG,__FUNCTION__, __LINE__, ##args)
    #else
#define LOG_INFO(fmt, args...)
#define LOG_DEBUG(fmt, args...)
    #endif

#else
#define LOG_ERROR(fmt, args...)
#define LOG_INFO(fmt, args...)
#define LOG_DEBUG(fmt, args...)
#endif
//add by chengdongsheng for muti_effect for 
#define VIBRATOR_MUTI_FEATURE
/* Address of our device */
#define DEVICE_ADDR 0x5A

/* i2c bus that it sits on */
#define DEVICE_BUS  0

/*
   DRV2605 built-in effect bank/library
 */
#define EFFECT_LIBRARY LIBRARY_F

/*
   GPIO port that enable power to the device
*/
#define GPIO_VIB_EN   GPIO_VIBTONE_EN1 
#define GPIO_VIB_PWM  GPIO_VIBTONE_PWM

/*
    Rated Voltage:
    Calculated using the formula r = v * 255 / 5.6
    where r is what will be written to the register
    and v is the rated voltage of the actuator

    Overdrive Clamp Voltage:
    Calculated using the formula o = oc * 255 / 5.6
    where o is what will be written to the register
    and oc is the overdrive clamp voltage of the actuator
 */
#if (EFFECT_LIBRARY == LIBRARY_A)
#define ERM_RATED_VOLTAGE               0x3E
#define ERM_OVERDRIVE_CLAMP_VOLTAGE     0x90

#elif (EFFECT_LIBRARY == LIBRARY_B)
#define ERM_RATED_VOLTAGE               0x90
#define ERM_OVERDRIVE_CLAMP_VOLTAGE     0x90

#elif (EFFECT_LIBRARY == LIBRARY_C)
#define ERM_RATED_VOLTAGE               0x90
#define ERM_OVERDRIVE_CLAMP_VOLTAGE     0x90

#elif (EFFECT_LIBRARY == LIBRARY_D)
#define ERM_RATED_VOLTAGE               0x90
#define ERM_OVERDRIVE_CLAMP_VOLTAGE     0x90

#elif (EFFECT_LIBRARY == LIBRARY_E)
#define ERM_RATED_VOLTAGE               0x90
#define ERM_OVERDRIVE_CLAMP_VOLTAGE     0x90

#else
#define ERM_RATED_VOLTAGE               0x90
#define ERM_OVERDRIVE_CLAMP_VOLTAGE     0x90
#endif

#define LRA_SEMCO1036                   0
#define LRA_SEMCO0934                   1
#define LRA_SEMCO1030                   2

#define LRA_SELECTION                   LRA_SEMCO1030

#if (LRA_SELECTION == LRA_SEMCO1036)
#define LRA_RATED_VOLTAGE               0x60
#define LRA_OVERDRIVE_CLAMP_VOLTAGE     0x9E
#define LRA_RTP_STRENGTH                0x7F // 100% of rated voltage (closed loop)

#elif (LRA_SELECTION == LRA_SEMCO0934)
#define LRA_RATED_VOLTAGE               0x51
#define LRA_OVERDRIVE_CLAMP_VOLTAGE     0x72
#define LRA_RTP_STRENGTH                0x7F // 100% of rated voltage (closed loop)

#elif (LRA_SELECTION == LRA_SEMCO1030)
#define LRA_RATED_VOLTAGE               0x51
#define LRA_OVERDRIVE_CLAMP_VOLTAGE     0x96
#define LRA_RTP_STRENGTH                0x7F // 100% of rated voltage (closed loop)

#endif

#define SKIP_LRA_AUTOCAL        1
#define GO_BIT_POLL_INTERVAL    15
#define STANDBY_WAKE_DELAY      1

#if EFFECT_LIBRARY == LIBRARY_A
#define REAL_TIME_PLAYBACK_STRENGTH 0x38 // ~44% of overdrive voltage (open loop)
#elif EFFECT_LIBRARY == LIBRARY_F
#define REAL_TIME_PLAYBACK_STRENGTH LRA_RTP_STRENGTH
#else
#define REAL_TIME_PLAYBACK_STRENGTH 0x7F // 100% of overdrive voltage (open loop)
#endif

#define MAX_TIMEOUT 10000 /* 10s */

//add by chengdongsheng for new 2605
/*
#define ZTEMT_LRA_RATED_VOLTAGE              0X51
#define ZTEMT_LRA_OVERDRIVE_CLAMP_VOLTAGE    0X96
#define ZTEMT_LRA_EFFECT_LIBRARY             0X06
#define ZTEMT_LRA_Control3_REG               0X60 

#define ZTEMT_LRA_AUTOCAL_COMPENSATION       0X0A
#define ZTEMT_LRA_AUTOCAL_BACKEMF            0XAF
#define ZTEMT_LRA_FEEDBACK_CONTROL_REG       0XA7
*/
#define ZTEMT_LRA_RATED_VOLTAGE              0X38
#define ZTEMT_LRA_OVERDRIVE_CLAMP_VOLTAGE    0X72
#define ZTEMT_LRA_EFFECT_LIBRARY             0X06
#define ZTEMT_LRA_Control3_REG               0X80 

#define ZTEMT_LRA_AUTOCAL_COMPENSATION       0x09       //reference 0X07
#define ZTEMT_LRA_AUTOCAL_BACKEMF            0x69       //reference 0XE9
#define ZTEMT_LRA_FEEDBACK_CONTROL_REG       0xc5       //reference 0XC6


/*
#define ZTEMT_LRA_AUTOCAL_COMPENSATION       0X06
#define ZTEMT_LRA_AUTOCAL_BACKEMF            0XBB
#define ZTEMT_LRA_FEEDBACK_CONTROL_REG       0XA6
*/

static struct drv260x {
    struct class* class;
    struct device* device;
    dev_t version;
    struct i2c_client* client;
    struct semaphore sem;
    struct cdev cdev;
} *drv260x;

static struct vibrator {
    struct wake_lock wklock;
    struct pwm_device *pwm_dev;
    struct hrtimer timer;
    struct mutex lock;
    struct work_struct work;
    struct work_struct work_play_eff;
    unsigned char sequence[8];
    volatile int should_stop;
} vibdata;

static struct timed_output_dev to_dev =
{
    .name           = "vibrator",
    .get_time       = vibrator_get_time,
    .enable         = vibrator_enable,
};

static char g_effect_bank = EFFECT_LIBRARY;
static int device_id = -1;
//ZTEMT changed by chengdongsheng the ERM was not used 
/*
static const unsigned char ERM_autocal_sequence[] = {
    MODE_REG,                       AUTO_CALIBRATION,
    REAL_TIME_PLAYBACK_REG,         REAL_TIME_PLAYBACK_STRENGTH,
    LIBRARY_SELECTION_REG,          EFFECT_LIBRARY,
    WAVEFORM_SEQUENCER_REG,         WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG2,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG3,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG4,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG5,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG6,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG7,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG8,        WAVEFORM_SEQUENCER_DEFAULT,
    OVERDRIVE_TIME_OFFSET_REG,      0x00,
    SUSTAIN_TIME_OFFSET_POS_REG,    0x00,
    SUSTAIN_TIME_OFFSET_NEG_REG,    0x00,
    BRAKE_TIME_OFFSET_REG,          0x00,
    AUDIO_HAPTICS_CONTROL_REG,      AUDIO_HAPTICS_RECT_20MS | AUDIO_HAPTICS_FILTER_125HZ,
    AUDIO_HAPTICS_MIN_INPUT_REG,    AUDIO_HAPTICS_MIN_INPUT_VOLTAGE,
    AUDIO_HAPTICS_MAX_INPUT_REG,    AUDIO_HAPTICS_MAX_INPUT_VOLTAGE,
    AUDIO_HAPTICS_MIN_OUTPUT_REG,   AUDIO_HAPTICS_MIN_OUTPUT_VOLTAGE,
    AUDIO_HAPTICS_MAX_OUTPUT_REG,   AUDIO_HAPTICS_MAX_OUTPUT_VOLTAGE,
    RATED_VOLTAGE_REG,              LRA_RATED_VOLTAGE,
    OVERDRIVE_CLAMP_VOLTAGE_REG,    LRA_OVERDRIVE_CLAMP_VOLTAGE,
    AUTO_CALI_RESULT_REG,           DEFAULT_LRA_AUTOCAL_COMPENSATION,
    AUTO_CALI_BACK_EMF_RESULT_REG,  DEFAULT_LRA_AUTOCAL_BACKEMF,
    FEEDBACK_CONTROL_REG,           FEEDBACK_CONTROL_MODE_LRA | FB_BRAKE_FACTOR_16X | LOOP_RESPONSE_VERY_FAST | FEEDBACK_CONTROL_BEMF_LRA_GAIN3,
    Control1_REG,                   STARTUP_BOOST_ENABLED | DEFAULT_DRIVE_TIME,
    Control2_REG,                   BIDIRECT_INPUT | AUTO_RES_GAIN_MEDIUM | BLANKING_TIME_VERY_SHORT | IDISS_TIME_SHORT,
    Control3_REG,                   NG_Thresh_2,


    
    AUTOCAL_MEM_INTERFACE_REG,      AUTOCAL_TIME_150MS,
    GO_REG,                         STOP,
};

static const unsigned char LRA_autocal_sequence[] = {
    MODE_REG,                       AUTO_CALIBRATION,
    RATED_VOLTAGE_REG,              LRA_RATED_VOLTAGE,
    OVERDRIVE_CLAMP_VOLTAGE_REG,    LRA_OVERDRIVE_CLAMP_VOLTAGE,
    FEEDBACK_CONTROL_REG,           FEEDBACK_CONTROL_MODE_LRA | FB_BRAKE_FACTOR_4X | LOOP_RESPONSE_FAST,
    Control3_REG,                   NG_Thresh_2,
    GO_REG,                         GO,
};

#if SKIP_LRA_AUTOCAL == 1
static const unsigned char LRA_init_sequence[] = {
    MODE_REG,                       MODE_INTERNAL_TRIGGER,
    REAL_TIME_PLAYBACK_REG,         REAL_TIME_PLAYBACK_STRENGTH,
    LIBRARY_SELECTION_REG,          LIBRARY_F,
    WAVEFORM_SEQUENCER_REG,         WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG2,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG3,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG4,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG5,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG6,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG7,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG8,        WAVEFORM_SEQUENCER_DEFAULT,
    GO_REG,                         STOP,
    OVERDRIVE_TIME_OFFSET_REG,      0x00,
    SUSTAIN_TIME_OFFSET_POS_REG,    0x00,
    SUSTAIN_TIME_OFFSET_NEG_REG,    0x00,
    BRAKE_TIME_OFFSET_REG,          0x00,
    AUDIO_HAPTICS_CONTROL_REG,      AUDIO_HAPTICS_RECT_20MS | AUDIO_HAPTICS_FILTER_125HZ,
    AUDIO_HAPTICS_MIN_INPUT_REG,    AUDIO_HAPTICS_MIN_INPUT_VOLTAGE,
    AUDIO_HAPTICS_MAX_INPUT_REG,    AUDIO_HAPTICS_MAX_INPUT_VOLTAGE,
    AUDIO_HAPTICS_MIN_OUTPUT_REG,   AUDIO_HAPTICS_MIN_OUTPUT_VOLTAGE,
    AUDIO_HAPTICS_MAX_OUTPUT_REG,   AUDIO_HAPTICS_MIN_OUTPUT_VOLTAGE,
    RATED_VOLTAGE_REG,              LRA_RATED_VOLTAGE,
    OVERDRIVE_CLAMP_VOLTAGE_REG,    LRA_OVERDRIVE_CLAMP_VOLTAGE,
    AUTO_CALI_RESULT_REG,           DEFAULT_LRA_AUTOCAL_COMPENSATION,
    AUTO_CALI_BACK_EMF_RESULT_REG,  DEFAULT_LRA_AUTOCAL_BACKEMF,
    FEEDBACK_CONTROL_REG,           FEEDBACK_CONTROL_MODE_LRA | FB_BRAKE_FACTOR_2X | LOOP_RESPONSE_MEDIUM | FEEDBACK_CONTROL_BEMF_LRA_GAIN3,
    Control1_REG,                   STARTUP_BOOST_ENABLED | DEFAULT_DRIVE_TIME,
    Control2_REG,                   BIDIRECT_INPUT | AUTO_RES_GAIN_MEDIUM | BLANKING_TIME_SHORT | IDISS_TIME_SHORT,
    Control3_REG,                   NG_Thresh_2,
    AUTOCAL_MEM_INTERFACE_REG,      AUTOCAL_TIME_500MS,
};
#endif
*/
#ifdef VIBRATOR_MUTI_FEATURE
	static int Rtp_Strength = 0x7f; //the 0x7f was the largest strength
	static int Effect_mode	= 0;
#endif

static const unsigned char SamSung_3010_init_sequence[] = {
    MODE_REG,                       MODE_INTERNAL_TRIGGER,
    REAL_TIME_PLAYBACK_REG,         REAL_TIME_PLAYBACK_STRENGTH,
    LIBRARY_SELECTION_REG,          LIBRARY_F,
    WAVEFORM_SEQUENCER_REG,         WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG2,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG3,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG4,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG5,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG6,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG7,        WAVEFORM_SEQUENCER_DEFAULT,
    WAVEFORM_SEQUENCER_REG8,        WAVEFORM_SEQUENCER_DEFAULT,
    GO_REG,                         STOP,
    OVERDRIVE_TIME_OFFSET_REG,      0x00,
    SUSTAIN_TIME_OFFSET_POS_REG,    0x00,
    SUSTAIN_TIME_OFFSET_NEG_REG,    0x00,
    BRAKE_TIME_OFFSET_REG,          0x00,
    AUDIO_HAPTICS_CONTROL_REG,      AUDIO_HAPTICS_RECT_20MS | AUDIO_HAPTICS_FILTER_125HZ,
    AUDIO_HAPTICS_MIN_INPUT_REG,    AUDIO_HAPTICS_MIN_INPUT_VOLTAGE,
    AUDIO_HAPTICS_MAX_INPUT_REG,    AUDIO_HAPTICS_MAX_INPUT_VOLTAGE,
    AUDIO_HAPTICS_MIN_OUTPUT_REG,   AUDIO_HAPTICS_MIN_OUTPUT_VOLTAGE,
    AUDIO_HAPTICS_MAX_OUTPUT_REG,   AUDIO_HAPTICS_MIN_OUTPUT_VOLTAGE,

    RATED_VOLTAGE_REG,              ZTEMT_LRA_RATED_VOLTAGE,
    OVERDRIVE_CLAMP_VOLTAGE_REG,    ZTEMT_LRA_OVERDRIVE_CLAMP_VOLTAGE,
    AUTO_CALI_RESULT_REG,           ZTEMT_LRA_AUTOCAL_COMPENSATION,
    AUTO_CALI_BACK_EMF_RESULT_REG,  ZTEMT_LRA_AUTOCAL_BACKEMF,
    FEEDBACK_CONTROL_REG,           ZTEMT_LRA_FEEDBACK_CONTROL_REG,   

    //Control1_REG,                   STARTUP_BOOST_ENABLED | DEFAULT_DRIVE_TIME,
	Control1_REG,                   0X13,
	//Control2_REG,                   BIDIRECT_INPUT | AUTO_RES_GAIN_MEDIUM | BLANKING_TIME_SHORT | IDISS_TIME_SHORT,
	Control2_REG,                   0XAA,
	Control3_REG,                   ZTEMT_LRA_Control3_REG,//NG_Thresh_2,
    AUTOCAL_MEM_INTERFACE_REG,      AUTOCAL_TIME_500MS,
};

#ifdef VIBRATOR_MUTI_FEATURE
unsigned char nubia_3010_wave_sequence[] = {
	WAVEFORM_SEQUENCER_REG, 		15,
	WAVEFORM_SEQUENCER_REG2,		150,
	WAVEFORM_SEQUENCER_REG3,		15,
	WAVEFORM_SEQUENCER_REG4,		150,
	WAVEFORM_SEQUENCER_REG5,		15,
	WAVEFORM_SEQUENCER_REG6,		150,
	WAVEFORM_SEQUENCER_REG7,		15,
	WAVEFORM_SEQUENCER_REG8,		150,
};

unsigned char nubia_3010_wave_sequence1[] = {
	WAVEFORM_SEQUENCER_REG, 		1,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};

unsigned char nubia_3010_wave_sequence2[] = {
	WAVEFORM_SEQUENCER_REG, 		2,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};

unsigned char nubia_3010_wave_sequence3[] = {
	WAVEFORM_SEQUENCER_REG, 		49,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};

unsigned char nubia_3010_wave_sequence4[] = {
	WAVEFORM_SEQUENCER_REG, 		34,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};

unsigned char nubia_3010_wave_sequence5[] = {
	WAVEFORM_SEQUENCER_REG, 		24,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};
//ztemt add by chengdongsheng for updata the mode
/*
unsigned char nubia_3010_wave_sequence6[] = {
	WAVEFORM_SEQUENCER_REG, 		29,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};
*/
unsigned char nubia_3010_wave_sequence6[] = {
		WAVEFORM_SEQUENCER_REG, 		6,
		WAVEFORM_SEQUENCER_REG2,		0,
		WAVEFORM_SEQUENCER_REG3,		0,
		WAVEFORM_SEQUENCER_REG4,		0,
		WAVEFORM_SEQUENCER_REG5,		0,
		WAVEFORM_SEQUENCER_REG6,		0,
		WAVEFORM_SEQUENCER_REG7,		0,
		WAVEFORM_SEQUENCER_REG8,		0,
	};
//ztemt end
unsigned char nubia_3010_wave_sequence7[] = {
	WAVEFORM_SEQUENCER_REG, 		74,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};

unsigned char nubia_3010_wave_sequence8[] = {
	WAVEFORM_SEQUENCER_REG, 		47,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};
//ztemt add by chengdongsheng for updata the mode 2013.1.2
unsigned char nubia_3010_wave_sequence9[] = {
	WAVEFORM_SEQUENCER_REG, 		5,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};
unsigned char nubia_3010_wave_sequence10[] = {
	WAVEFORM_SEQUENCER_REG, 		4,
	WAVEFORM_SEQUENCER_REG2,		0,
	WAVEFORM_SEQUENCER_REG3,		0,
	WAVEFORM_SEQUENCER_REG4,		0,
	WAVEFORM_SEQUENCER_REG5,		0,
	WAVEFORM_SEQUENCER_REG6,		0,
	WAVEFORM_SEQUENCER_REG7,		0,
	WAVEFORM_SEQUENCER_REG8,		0,
};
//ztemt end

#endif
static const unsigned char ZTEMT_LRA_autocal_sequence[] = {
    MODE_REG,                       AUTO_CALIBRATION,
    RATED_VOLTAGE_REG,              ZTEMT_LRA_RATED_VOLTAGE,
    OVERDRIVE_CLAMP_VOLTAGE_REG,    ZTEMT_LRA_OVERDRIVE_CLAMP_VOLTAGE,
    AUTO_CALI_RESULT_REG,           ZTEMT_LRA_AUTOCAL_COMPENSATION,
    AUTO_CALI_BACK_EMF_RESULT_REG,  ZTEMT_LRA_AUTOCAL_BACKEMF,
    FEEDBACK_CONTROL_REG,           ZTEMT_LRA_FEEDBACK_CONTROL_REG,   
    WAVEFORM_SEQUENCER_REG,         0x30,
    WAVEFORM_SEQUENCER_REG2,        0x00,
    Control3_REG,                   ZTEMT_LRA_Control3_REG,
    LIBRARY_SELECTION_REG,          ZTEMT_LRA_EFFECT_LIBRARY,
    GO_REG,                         GO,
};

#ifdef CONFIG_PM

    #ifndef CONFIG_FEATURE_ZTEMT_VIBRATOR_HAVE_MISS_EVENT

static int drv260x_resume(struct i2c_client *client)
{
    gpio_set_value(GPIO_VIB_EN, GPIO_LEVEL_HIGH);
    return 0;
}

static int drv260x_suspend(struct i2c_client *client, pm_message_t mesg)
{
    gpio_set_value(GPIO_VIB_EN, GPIO_LEVEL_LOW);
    return 0;
}

    #else

#define drv260x_suspend NULL
#define drv260x_resume  NULL
    #endif

#else

#define drv260x_suspend NULL
#define drv260x_resume  NULL

#endif



static void drv260x_write_reg_val(const unsigned char* data, unsigned int size)
{
    int i = 0;

    if (size % 2 != 0)
        return;

    while (i < size)
    {
        i2c_smbus_write_byte_data(drv260x->client, data[i], data[i+1]);
        i+=2;
    }
}

static void drv260x_set_go_bit(char val)
{
    char go[] =
    {
        GO_REG, val
    };
    drv260x_write_reg_val(go, sizeof(go));
}

static unsigned char drv260x_read_reg(unsigned char reg)
{
    return i2c_smbus_read_byte_data(drv260x->client, reg);
}

static void drv2605_poll_go_bit(void)
{
    while (drv260x_read_reg(GO_REG) == GO)
      schedule_timeout_interruptible(msecs_to_jiffies(GO_BIT_POLL_INTERVAL));
}

static void drv2605_select_library(char lib)
{
    char library[] =
    {
        LIBRARY_SELECTION_REG, lib
    };
    drv260x_write_reg_val(library, sizeof(library));
}

static void drv260x_set_rtp_val(char value)
{
    char rtp_val[] =
    {
        REAL_TIME_PLAYBACK_REG, value
    };
    drv260x_write_reg_val(rtp_val, sizeof(rtp_val));
}

static void drv2605_set_waveform_sequence(unsigned char* seq, unsigned int size)
{
    unsigned char data[WAVEFORM_SEQUENCER_MAX + 1];

    if (size > WAVEFORM_SEQUENCER_MAX)
        return;

    memset(data, 0, sizeof(data));
    memcpy(&data[1], seq, size);
    data[0] = WAVEFORM_SEQUENCER_REG;

    i2c_master_send(drv260x->client, data, sizeof(data));
}

static void drv260x_change_mode(char mode)
{
    unsigned char tmp[] =
    {
        MODE_REG, mode
    };
    drv260x_write_reg_val(tmp, sizeof(tmp));
}

/* --------------------------------------------------------------------------------- */
#define YES 1
#define NO  0

static void setAudioHapticsEnabled(int enable);
static int audio_haptics_enabled = NO;
static int vibrator_is_playing = NO;

static int vibrator_get_time(struct timed_output_dev *dev)
{
    if (hrtimer_active(&vibdata.timer)) {
        ktime_t r = hrtimer_get_remaining(&vibdata.timer);
        return ktime_to_ms(r);
    }

    return 0;
}

static void vibrator_off(void)
{
    if (vibrator_is_playing) {
        vibrator_is_playing = NO;
        if (audio_haptics_enabled)
        {
            if ((drv260x_read_reg(MODE_REG) & DRV260X_MODE_MASK) != MODE_AUDIOHAPTIC)
                setAudioHapticsEnabled(YES);
        } else
        {
            drv260x_change_mode(MODE_STANDBY);
        }
    }

    wake_unlock(&vibdata.wklock);
#ifdef VIBRATOR_MUTI_FEATURE
	Effect_mode = 0;
#endif
	
}

#ifdef ZTEMT_FOR_VIBRATOR_TEST

void static drv260x_auto_cal(void)
{
    unsigned char status = 0;

    drv260x_write_reg_val(ZTEMT_LRA_autocal_sequence, sizeof(ZTEMT_LRA_autocal_sequence));
    LOG_ERROR("------------------------------------------------------------------------\n");
    LOG_ERROR("!!! cal before !!!\n");
    LOG_ERROR("------------------------------------------------------------------------\n");

    LOG_ERROR("MODE_REG                      = 0x%2X\n",drv260x_read_reg(MODE_REG));
    LOG_ERROR("RATED_VOLTAGE_REG             = 0x%2X\n",drv260x_read_reg(RATED_VOLTAGE_REG));
    LOG_ERROR("OVERDRIVE_CLAMP_VOLTAGE_REG   = 0x%2X\n",drv260x_read_reg(OVERDRIVE_CLAMP_VOLTAGE_REG));
    LOG_ERROR("AUTO_CALI_RESULT_REG          = 0x%2X\n",drv260x_read_reg(AUTO_CALI_RESULT_REG));
    LOG_ERROR("AUTO_CALI_BACK_EMF_RESULT_REG = 0x%2X\n",drv260x_read_reg(AUTO_CALI_BACK_EMF_RESULT_REG));
    LOG_ERROR("FEEDBACK_CONTROL_REG          = 0x%2X\n",drv260x_read_reg(FEEDBACK_CONTROL_REG));
    LOG_ERROR("WAVEFORM_SEQUENCER_REG        = 0x%2X\n",drv260x_read_reg(WAVEFORM_SEQUENCER_REG));
    LOG_ERROR("WAVEFORM_SEQUENCER_REG2       = 0x%2X\n",drv260x_read_reg(WAVEFORM_SEQUENCER_REG2));
    LOG_ERROR("Control3_REG                  = 0x%2X\n",drv260x_read_reg(Control3_REG));
    LOG_ERROR("LIBRARY_SELECTION_REG         = 0x%2X\n",drv260x_read_reg(LIBRARY_SELECTION_REG));

    
    drv2605_poll_go_bit();

    LOG_ERROR("------------------------------------------------------------------------\n");
    LOG_ERROR("!!! cal after !!!\n");
    LOG_ERROR("------------------------------------------------------------------------\n");

    LOG_ERROR("MODE_REG                      = 0x%2X\n",drv260x_read_reg(MODE_REG));
	LOG_ERROR("STATUS_REG                      = 0x%2X\n",drv260x_read_reg(STATUS_REG));
    LOG_ERROR("RATED_VOLTAGE_REG             = 0x%2X\n",drv260x_read_reg(RATED_VOLTAGE_REG));
    LOG_ERROR("OVERDRIVE_CLAMP_VOLTAGE_REG   = 0x%2X\n",drv260x_read_reg(OVERDRIVE_CLAMP_VOLTAGE_REG));
    LOG_ERROR("AUTO_CALI_RESULT_REG          = 0x%2X\n",drv260x_read_reg(AUTO_CALI_RESULT_REG));
    LOG_ERROR("AUTO_CALI_BACK_EMF_RESULT_REG = 0x%2X\n",drv260x_read_reg(AUTO_CALI_BACK_EMF_RESULT_REG));
    LOG_ERROR("FEEDBACK_CONTROL_REG          = 0x%2X\n",drv260x_read_reg(FEEDBACK_CONTROL_REG));
    LOG_ERROR("WAVEFORM_SEQUENCER_REG        = 0x%2X\n",drv260x_read_reg(WAVEFORM_SEQUENCER_REG));
    LOG_ERROR("WAVEFORM_SEQUENCER_REG2       = 0x%2X\n",drv260x_read_reg(WAVEFORM_SEQUENCER_REG2));
	LOG_ERROR("Control1_REG                  = 0x%2X\n",drv260x_read_reg(Control1_REG));
	LOG_ERROR("Control2_REG                  = 0x%2X\n",drv260x_read_reg(Control2_REG));
    LOG_ERROR("Control3_REG                  = 0x%2X\n",drv260x_read_reg(Control3_REG));
    LOG_ERROR("LIBRARY_SELECTION_REG         = 0x%2X\n",drv260x_read_reg(LIBRARY_SELECTION_REG));

    status = drv260x_read_reg(STATUS_REG);
    if ((status & DIAG_RESULT_MASK) == AUTO_CAL_FAILED)
    {
        LOG_ERROR("drv260x auto-cal failed    status = %2X\n",status);
    }
    else
    {
        LOG_ERROR("drv260x auto-cal successs  status = %2X\n",status);
    }
    /* Read calibration results */
    LOG_ERROR("calibration result is %02X: %02X: %02X:\n",drv260x_read_reg(AUTO_CALI_RESULT_REG),
                                                          drv260x_read_reg(AUTO_CALI_BACK_EMF_RESULT_REG),
                                                          drv260x_read_reg(FEEDBACK_CONTROL_REG)
                                                          );

}


static void vibrator_enable_play_test_sequence(int value)
{
//enable vibrator in internal_trigger
    mutex_lock(&vibdata.lock);
    hrtimer_cancel(&vibdata.timer);
    cancel_work_sync(&vibdata.work);
    drv260x_write_reg_val(nubia_3010_wave_sequence, sizeof(nubia_3010_wave_sequence));
    if (value) {
        wake_lock(&vibdata.wklock);
        vibrator_is_playing = YES;
        drv260x_change_mode(MODE_INTERNAL_TRIGGER);
        drv260x_set_go_bit(GO);
        if (value > 0) {
            if (value > MAX_TIMEOUT)
               value = MAX_TIMEOUT;
            hrtimer_start(&vibdata.timer, ns_to_ktime((u64)value * NSEC_PER_MSEC), HRTIMER_MODE_REL);
        }
    }
    else
        vibrator_off();

    mutex_unlock(&vibdata.lock);
}

#endif

#ifdef VIBRATOR_MUTI_FEATURE
/**************************************************************************
   the vibrator_refresh_register() was used to refresh the init_register 
   when the LIBRARY_SELECTION_REG was not 0x06
   SamSung_3010_init_sequence was the init_register
****************************************************************************/
   static void vibrator_refresh_register(void)
{
	char lib_select= 0x06;
	lib_select=drv260x_read_reg(LIBRARY_SELECTION_REG);
	if(0x06 != lib_select){		
		LOG_ERROR("THE vibrator reg was changed .lib_select = 0x%2X\n",drv260x_read_reg(LIBRARY_SELECTION_REG));
		drv260x_write_reg_val(SamSung_3010_init_sequence, sizeof(SamSung_3010_init_sequence));
	}
	return;
}

static void vibrator_enable_via_rtp(int value)
{
	//enable vibrator in internal_trigger
    mutex_lock(&vibdata.lock);
    hrtimer_cancel(&vibdata.timer);
    cancel_work_sync(&vibdata.work);
    	if (value) {
        	wake_lock(&vibdata.wklock);
        	/* Only change the mode if not already in RTP mode; RTP input already set at init */
        	if ((drv260x_read_reg(MODE_REG) & DRV260X_MODE_MASK) != MODE_REAL_TIME_PLAYBACK)
        	{
            	//drv260x_set_rtp_val(REAL_TIME_PLAYBACK_STRENGTH);           	
            	drv260x_set_rtp_val((char)Rtp_Strength);
         	   drv260x_change_mode(MODE_REAL_TIME_PLAYBACK);
         	   vibrator_is_playing = YES;
        	}

        	if (value > 0) {
            	if (value > MAX_TIMEOUT)
                	value = MAX_TIMEOUT;
            	hrtimer_start(&vibdata.timer, ns_to_ktime((u64)value * NSEC_PER_MSEC), HRTIMER_MODE_REL);
        	}
    	}
    	else
        	vibrator_off();
	mutex_unlock(&vibdata.lock);
}

static void vibrator_enable_via_InternalTrigger(int value)
{
    mutex_lock(&vibdata.lock);
    hrtimer_cancel(&vibdata.timer);
    cancel_work_sync(&vibdata.work);
	if (value) {
    switch(Effect_mode){	
    	case 1:
        	drv260x_write_reg_val(nubia_3010_wave_sequence1, sizeof(nubia_3010_wave_sequence1));
    		break;
    	case 2:
        	drv260x_write_reg_val(nubia_3010_wave_sequence2, sizeof(nubia_3010_wave_sequence2));
    		break;
    	case 3:
        	drv260x_write_reg_val(nubia_3010_wave_sequence3, sizeof(nubia_3010_wave_sequence4));
    		break;
    	case 4:
        	drv260x_write_reg_val(nubia_3010_wave_sequence4, sizeof(nubia_3010_wave_sequence4));
    		break;
    	case 5:
        	drv260x_write_reg_val(nubia_3010_wave_sequence5, sizeof(nubia_3010_wave_sequence5));
    		break;
    	case 6:
        	drv260x_write_reg_val(nubia_3010_wave_sequence6, sizeof(nubia_3010_wave_sequence6));
    		break;
    	case 7:
        	drv260x_write_reg_val(nubia_3010_wave_sequence7, sizeof(nubia_3010_wave_sequence7));
    		break;
    	case 8:
        	drv260x_write_reg_val(nubia_3010_wave_sequence8, sizeof(nubia_3010_wave_sequence8));
    		break;
	//ztemt add by chengdongsheng for updata the mode 2013.1.2
		case 9:
        	drv260x_write_reg_val(nubia_3010_wave_sequence9, sizeof(nubia_3010_wave_sequence9));
    		break;
		case 10:
        	drv260x_write_reg_val(nubia_3010_wave_sequence10, sizeof(nubia_3010_wave_sequence10));
    		break;
	//ztemt end
    	default:
            LOG_ERROR(KERN_ALERT"drv260x effect mode not found: unknown.\n");
            break;
    }
        wake_lock(&vibdata.wklock);
        vibrator_is_playing = YES;
        drv260x_change_mode(MODE_INTERNAL_TRIGGER);
        drv260x_set_go_bit(GO);
        if (value > 0) {
            if (value > MAX_TIMEOUT)
               value = MAX_TIMEOUT;
            hrtimer_start(&vibdata.timer, ns_to_ktime((u64)value * NSEC_PER_MSEC), HRTIMER_MODE_REL);
        }
    }
    else
        vibrator_off();

    mutex_unlock(&vibdata.lock);
}
static void vibrator_enable(struct timed_output_dev *dev, int value)
{
	if(value){
		vibrator_refresh_register();      //refresh the init_regitster if the lib-select_reg was changed 
	}
	if(Effect_mode){
			vibrator_enable_via_InternalTrigger(value);//changed by chengdongsheng for rename vibrator_enable3(value);
		}
	else
	{
		vibrator_enable_via_rtp(value); //if the mdoe was set to 0(the default),it mean vibrator via rtp mode.
	/*
	mutex_lock(&vibdata.lock);
    hrtimer_cancel(&vibdata.timer);
    cancel_work_sync(&vibdata.work);
    	if (value) {
        	wake_lock(&vibdata.wklock);
        	// Only change the mode if not already in RTP mode; RTP input already set at init 
        	if ((drv260x_read_reg(MODE_REG) & DRV260X_MODE_MASK) != MODE_REAL_TIME_PLAYBACK)
        	{
            drv260x_set_rtp_val(REAL_TIME_PLAYBACK_STRENGTH);
         	   drv260x_change_mode(MODE_REAL_TIME_PLAYBACK);
         	   vibrator_is_playing = YES;
        	}

        	if (value > 0) {
            	if (value > MAX_TIMEOUT)
                	value = MAX_TIMEOUT;
            	hrtimer_start(&vibdata.timer, ns_to_ktime((u64)value * NSEC_PER_MSEC), HRTIMER_MODE_REL);
        	}
			
    	}
    	else
        	vibrator_off();
	mutex_unlock(&vibdata.lock);
	*/
	}  
}
#else
static void vibrator_enable(struct timed_output_dev *dev, int value)
{	// the api was supply by ti(or immerse) so we don't change it.
	mutex_lock(&vibdata.lock);
    hrtimer_cancel(&vibdata.timer);
    cancel_work_sync(&vibdata.work);
    	if (value) {
        	wake_lock(&vibdata.wklock);
        	/* Only change the mode if not already in RTP mode; RTP input already set at init */
        	if ((drv260x_read_reg(MODE_REG) & DRV260X_MODE_MASK) != MODE_REAL_TIME_PLAYBACK)
        	{
            drv260x_set_rtp_val(REAL_TIME_PLAYBACK_STRENGTH);
         	   drv260x_change_mode(MODE_REAL_TIME_PLAYBACK);
         	   vibrator_is_playing = YES;
        	}

        	if (value > 0) {
            	if (value > MAX_TIMEOUT)
                	value = MAX_TIMEOUT;
            	hrtimer_start(&vibdata.timer, ns_to_ktime((u64)value * NSEC_PER_MSEC), HRTIMER_MODE_REL);
        	}
    	}
    	else
        	vibrator_off();
	mutex_unlock(&vibdata.lock);
}

#endif
/*
the extern_lineat_vibrator_enbale() is supply for miss_event finctions
vibrator_enbale_have_mode() will play the sequence mode,mode,mode,mode,mode,mode,mode,mode
vibrator_enable_rtp_mode will vibraot via rpt 

*/
void vibrator_enable_have_mode(int mode, int time)
{   
    unsigned char SamSung_3010_wave_sequence[] = {
    WAVEFORM_SEQUENCER_REG,         mode,
    WAVEFORM_SEQUENCER_REG2,        mode,
    WAVEFORM_SEQUENCER_REG3,        mode,
    WAVEFORM_SEQUENCER_REG4,        mode,
    WAVEFORM_SEQUENCER_REG5,        mode,
    WAVEFORM_SEQUENCER_REG6,        mode,
    WAVEFORM_SEQUENCER_REG7,        mode,
    WAVEFORM_SEQUENCER_REG8,        mode,
    };

    mutex_lock(&vibdata.lock);
    hrtimer_cancel(&vibdata.timer);
    cancel_work_sync(&vibdata.work);

    drv260x_write_reg_val(SamSung_3010_wave_sequence, sizeof(SamSung_3010_wave_sequence));

    if (time) {
        wake_lock(&vibdata.wklock);
        vibrator_is_playing = YES;
        drv260x_change_mode(MODE_INTERNAL_TRIGGER);
        drv260x_set_go_bit(GO);

        if (time > 0) {
            if (time > MAX_TIMEOUT)
                time = MAX_TIMEOUT;
            hrtimer_start(&vibdata.timer, ns_to_ktime((u64)time * NSEC_PER_MSEC), HRTIMER_MODE_REL);
        }
    }
    else
        vibrator_off();

    mutex_unlock(&vibdata.lock);
}

void vibrator_enable_rtp_mode(int value)
{
    struct timed_output_dev dev;
    vibrator_enable(&dev, value);
}

void extern_linear_vibrator_enable(int mode, int time)
{
    if (mode>=1 && mode<=123)
    {
        vibrator_enable_have_mode(mode, time);
    }
    else
    {
        vibrator_enable_rtp_mode(time);
    }
}


#ifdef ZTEMT_FOR_VIBRATOR_TEST
static ssize_t attr_set_vibrator_work_mode(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    LOG_ERROR("enter\n");
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

    vibrator_enable_have_mode(val, 1000);

    LOG_ERROR("exit\n");
    return size;
}
static ssize_t attr_paly_effect_mode(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    LOG_ERROR("enter\n");
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

    vibrator_enable_play_test_sequence(val);

    LOG_ERROR("exit\n");
    return size;
}
static ssize_t attr_get_new_test_sequence(struct device *dev,
		struct device_attribute *attr,	const char *mode, size_t size)
{
	char *regvalue;
	char *b;
	char buff[32];
	unsigned long val=0;
	int i=0;
    LOG_ERROR("enter attr_set_vibrator_work_mode2 [%s]\n",mode);	
	strlcpy(buff, mode, sizeof(buff));
	b = strim(buff);
	LOG_ERROR("[dsc]nubia_3010_wave_sequence:");
	while (b) {
			regvalue = strsep(&b, ",");
			if (regvalue) {
				if (strict_strtoul(regvalue, 10, &val)){
					pr_err("there is a invalid mode setting\n");
					return -EINVAL;
				}
				if(val>256){
					pr_err("the mode is too large\n");
					return -EINVAL;
				}
				nubia_3010_wave_sequence[i+1]= val;				
				LOG_ERROR("[%d]",nubia_3010_wave_sequence[i+1]);
				i+=2;
			}
		}
    LOG_ERROR("\nexit\n");
    return size;
}

static ssize_t attr_auto_cal(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    LOG_ERROR("enter\n");
    drv260x_auto_cal();
    LOG_ERROR("exit\n");
    return size;
}

#endif

#ifdef VIBRATOR_MUTI_FEATURE

static ssize_t attr_set_rtp_strength(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
    LOG_ERROR("enter attr_set_rtp_strength\n");
	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
    Rtp_Strength = val;
	drv260x_set_rtp_val((char)Rtp_Strength);
    LOG_ERROR("exitattr_set_rtp_strength [%x]\n",Rtp_Strength);
    return size;
}

static ssize_t attr_set_vibrator_effecte_mode(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
    unsigned long val;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	Effect_mode=val;
    return size;
}

static ssize_t attr_read_vibrator_effecte_mode(struct device *dev,
		struct device_attribute *attr,char *buf)
{
    int val;
	val=sprintf(buf, "%d\n", Effect_mode);
	return val;
}
#endif

static struct device_attribute attributes[] = {
#ifdef ZTEMT_FOR_VIBRATOR_TEST
	//set one mode(1 to 123) and play it in 1000ms
	__ATTR(vibrator_set_mode_with_1000ms, 0644, NULL, attr_set_vibrator_work_mode),
	//set one sequence(1 to 8 register:1,25,22,22,22,22,25,1) and then we can play the test sequence via vibrator_paly_effect_mode
	__ATTR(vibrator_set_effect_mode, 0644, NULL, attr_get_new_test_sequence),
	 
	 //play the test sequence
	 __ATTR(vibrator_paly_effect_mode, 0644, NULL, attr_paly_effect_mode),
	 //do auto_calibrator
    __ATTR(vibrator_auto_cal, 0644, NULL, attr_auto_cal),
#endif
#ifdef VIBRATOR_MUTI_FEATURE
    __ATTR(vibrator_set_mode,0644, attr_read_vibrator_effecte_mode, attr_set_vibrator_effecte_mode),
    __ATTR(vibrator_set_rtp_strength, 0644, NULL, attr_set_rtp_strength),
#endif
   
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

static enum hrtimer_restart vibrator_timer_func(struct hrtimer *timer)
{
    schedule_work(&vibdata.work);
    return HRTIMER_NORESTART;
}

static void vibrator_work(struct work_struct *work)
{
    vibrator_off();
}

/* ----------------------------------------------------------------------------- */

static void play_effect(struct work_struct *work)
{
    if (audio_haptics_enabled &&
        ((drv260x_read_reg(MODE_REG) & DRV260X_MODE_MASK) == MODE_AUDIOHAPTIC))
        setAudioHapticsEnabled(NO);

    drv260x_change_mode(MODE_INTERNAL_TRIGGER);
    drv2605_set_waveform_sequence(vibdata.sequence, sizeof(vibdata.sequence));
    drv260x_set_go_bit(GO);

    while(drv260x_read_reg(GO_REG) == GO && !vibdata.should_stop)
        schedule_timeout_interruptible(msecs_to_jiffies(GO_BIT_POLL_INTERVAL));

    wake_unlock(&vibdata.wklock);
    if (audio_haptics_enabled)
    {
        setAudioHapticsEnabled(YES);
    } else
    {
        drv260x_change_mode(MODE_STANDBY);
    }
}

static void setAudioHapticsEnabled(int enable)
{
    if (enable)
    {
        char audiohaptic_settings[] =
        {
            Control1_REG, STARTUP_BOOST_ENABLED | AC_COUPLE_ENABLED | AUDIOHAPTIC_DRIVE_TIME,
            Control3_REG, NG_Thresh_2 | INPUT_ANALOG
        };
        // Chip needs to be brought out of standby to change the registers
        drv260x_change_mode(MODE_INTERNAL_TRIGGER);
        schedule_timeout_interruptible(msecs_to_jiffies(STANDBY_WAKE_DELAY));
        drv260x_write_reg_val(audiohaptic_settings, sizeof(audiohaptic_settings));
        drv260x_change_mode(MODE_AUDIOHAPTIC);
    } else
    {
        char default_settings[] =
        {
            Control1_REG, STARTUP_BOOST_ENABLED | DEFAULT_DRIVE_TIME,
            Control3_REG, NG_Thresh_2
        };
        drv260x_change_mode(MODE_STANDBY); // Disable audio-to-haptics
        schedule_timeout_interruptible(msecs_to_jiffies(STANDBY_WAKE_DELAY));
        // Chip needs to be brought out of standby to change the registers
        drv260x_change_mode(MODE_INTERNAL_TRIGGER);
        schedule_timeout_interruptible(msecs_to_jiffies(STANDBY_WAKE_DELAY));
        if (g_effect_bank != LIBRARY_F)
            default_settings[3] |= ERM_OpenLoop_Enabled;
        drv260x_write_reg_val(default_settings, sizeof(default_settings));
        drv260x_change_mode(MODE_STANDBY);
    }
}

static int drv260x_probe(struct i2c_client* client, const struct i2c_device_id* id)
{
    char status;
    unsigned char retry_times;

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        printk(KERN_ALERT"drv260x probe failed");
        return -ENODEV;
    }

    drv260x->client = client;
	//do reset eveyr time we power-on the phone by chengdongsheng
	
    drv260x_change_mode(MODE_RESET);
	mdelay(10);
	
    /* Enable power to the chip */
    gpio_direction_output(GPIO_VIB_EN, GPIO_LEVEL_HIGH);
    gpio_direction_output(GPIO_VIB_PWM, GPIO_LEVEL_LOW);
    /* Wait 30 us */
    udelay(30);

    for (retry_times=0; retry_times<10; retry_times++)
    {
        status = drv260x_read_reg(STATUS_REG);
        if (status >= 0)
        { 
            break;   
        }
    }
    
    if (retry_times >= 10)
    {
        LOG_ERROR("drv2605 communaciation failed!\n");
        goto prob_error;
    }
    else
    {
        LOG_ERROR("drv2605 communaciation retry_times = %d\n",retry_times);
    }
  
//    drv260x_auto_cal();

    
    /* Read device ID */
    device_id = (status & DEV_ID_MASK);
    LOG_ERROR("device_id is 0x%X",device_id);

    switch (device_id)
    {
        case DRV2605_ID_0X60:
        {
            LOG_ERROR("drv260x driver found: drv2605, ID = 0x60\n");
            break;
        }
        case DRV2605_ID_0XA0:
        {
            LOG_ERROR("drv260x driver found: drv2605, ID = 0xA0\n");
            break;
        }    
        case DRV2604:
        {    
            LOG_ERROR("drv260x driver found: drv2604. chip is not correct\n");
            goto prob_error;
        }
        default:
        {
            LOG_ERROR("drv260x driver found: unknown.\n");
            goto prob_error;
        }
    }

    if (timed_output_dev_register(&to_dev) < 0)
    {
        LOG_ERROR("drv260x: fail to create timed output dev\n");
        goto prob_error;
    }
    else
    {
        LOG_ERROR("drv260x: success to create timed output dev\n");
    }

    /* Choose default effect library */
    drv2605_select_library(g_effect_bank);

    /* Put hardware in standby */
    drv260x_change_mode(MODE_STANDBY);

    drv260x_write_reg_val(SamSung_3010_init_sequence, sizeof(SamSung_3010_init_sequence));

    status = create_sysfs_interfaces(&client->dev);
	if (status < 0) 
    {
		dev_err(&client->dev,
		   "device drv2605 sysfs register failed\n");
		return status;
	}

    LOG_ERROR(KERN_ALERT"drv260x probe succeeded");
    LOG_ERROR(KERN_ALERT"drv260x driver version: "DRIVER_VERSION);
    return 0;

prob_error:
    LOG_ERROR("drv260x probe failed\n");
    return -ENODEV;
}

static int drv260x_remove(struct i2c_client* client)
{
    printk(KERN_ALERT"drv260x remove");
    return 0;
}

static struct i2c_device_id drv260x_id_table[] =
{
    { DEVICE_NAME, 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, drv260x_id_table);

static struct i2c_driver drv260x_driver =
{
    .driver = {
        .name = DEVICE_NAME,
    },
    .id_table = drv260x_id_table,
    .probe = drv260x_probe,
    .remove = drv260x_remove,
    .suspend = drv260x_suspend,
    .resume = drv260x_resume,
};

static struct i2c_board_info info =
{
    I2C_BOARD_INFO(DEVICE_NAME, DEVICE_ADDR),
};


static char read_val;

static ssize_t drv260x_read(struct file* filp, char* buff, size_t length, loff_t* offset)
{
    buff[0] = read_val;
    return 1;
}

static ssize_t drv260x_write(struct file* filp, const char* buff, size_t len, loff_t* off)
{
    mutex_lock(&vibdata.lock);
    hrtimer_cancel(&vibdata.timer);

    vibdata.should_stop = YES;
    cancel_work_sync(&vibdata.work_play_eff);
    cancel_work_sync(&vibdata.work);

    if (vibrator_is_playing)
    {
        vibrator_is_playing = NO;
        drv260x_change_mode(MODE_STANDBY);
    }

    switch(buff[0])
    {
        case HAPTIC_CMDID_PLAY_SINGLE_EFFECT:
        case HAPTIC_CMDID_PLAY_EFFECT_SEQUENCE:
        {
            memset(&vibdata.sequence, 0, sizeof(vibdata.sequence));
            if (!copy_from_user(&vibdata.sequence, &buff[1], len - 1))
            {
                vibdata.should_stop = NO;
                wake_lock(&vibdata.wklock);
                schedule_work(&vibdata.work_play_eff);
            }
            break;
        }
        case HAPTIC_CMDID_PLAY_TIMED_EFFECT:
        {
            unsigned int value = 0;

            value = buff[2];
            value <<= 8;
            value |= buff[1];

            if (value)
            {
                wake_lock(&vibdata.wklock);

                if ((drv260x_read_reg(MODE_REG) & DRV260X_MODE_MASK) != MODE_REAL_TIME_PLAYBACK)
                {
                    drv260x_set_rtp_val(REAL_TIME_PLAYBACK_STRENGTH);
                    drv260x_change_mode(MODE_REAL_TIME_PLAYBACK);
                    vibrator_is_playing = YES;
                }

                if (value > 0)
                {
                    if (value > MAX_TIMEOUT)
                        value = MAX_TIMEOUT;
                    hrtimer_start(&vibdata.timer, ns_to_ktime((u64)value * NSEC_PER_MSEC), HRTIMER_MODE_REL);
                }
            }
            break;
        }
        case HAPTIC_CMDID_STOP:
        {
            if (vibrator_is_playing)
            {
                vibrator_is_playing = NO;
                if (audio_haptics_enabled)
                {
                    setAudioHapticsEnabled(YES);
                } else
                {
                    drv260x_change_mode(MODE_STANDBY);
                }
            }
            vibdata.should_stop = YES;
            break;
        }
        case HAPTIC_CMDID_GET_DEV_ID:
        {
            /* Dev ID includes 2 parts, upper word for device id, lower word for chip revision */
            int revision = (drv260x_read_reg(SILICON_REVISION_REG) & SILICON_REVISION_MASK);
            read_val = (device_id >> 1) | revision;
            break;
        }
        case HAPTIC_CMDID_RUN_DIAG:
        {
            char diag_seq[] =
            {
                MODE_REG, MODE_DIAGNOSTICS,
                GO_REG,   GO
            };
            drv260x_write_reg_val(diag_seq, sizeof(diag_seq));
            drv2605_poll_go_bit();
            read_val = (drv260x_read_reg(STATUS_REG) & DIAG_RESULT_MASK) >> 3;
            break;
        }
        case HAPTIC_CMDID_AUDIOHAPTIC_ENABLE:
        {
            if ((drv260x_read_reg(MODE_REG) & DRV260X_MODE_MASK) != MODE_AUDIOHAPTIC)
            {
                setAudioHapticsEnabled(YES);
                audio_haptics_enabled = YES;
            }
            break;
        }
        case HAPTIC_CMDID_AUDIOHAPTIC_DISABLE:
        {
            if (audio_haptics_enabled)
            {
                if ((drv260x_read_reg(MODE_REG) & DRV260X_MODE_MASK) == MODE_AUDIOHAPTIC)
                    setAudioHapticsEnabled(NO);
                audio_haptics_enabled = NO;
            }
            break;
        }
        case HAPTIC_CMDID_AUDIOHAPTIC_GETSTATUS:
        {
            if ((drv260x_read_reg(MODE_REG) & DRV260X_MODE_MASK) == MODE_AUDIOHAPTIC)
            {
                read_val = 1;
            }
            else
            {
                read_val = 0;
            }
            break;
        }
    default:
      break;
    }

    mutex_unlock(&vibdata.lock);

    return len;
}

static struct file_operations fops =
{
    .read = drv260x_read,
    .write = drv260x_write
};

static int drv260x_init(void)
{
    int reval = -ENOMEM;
    struct i2c_adapter* adapter;
    struct i2c_client* client;

    if (gpio_request(GPIO_VIB_EN, "vibrator-en") < 0)
    {
        LOG_ERROR("drv260x: error requesting gpio\n");
        goto fail0;
    }

     if (gpio_request(GPIO_VIB_PWM, "vibrator-pwm") < 0)
    {
        printk(KERN_ALERT"drv260x: error requesting gpio\n");
        LOG_ERROR("drv260x: error requesting gpio\n");
        goto fail0;
    }
     

    drv260x = kmalloc(sizeof *drv260x, GFP_KERNEL);
    if (!drv260x)
    {
        printk(KERN_ALERT"drv260x: cannot allocate memory for drv260x driver\n");
        goto fail0;
    }

    adapter = i2c_get_adapter(DEVICE_BUS);
    if (!adapter)
    {
        printk(KERN_ALERT"drv260x: Cannot get adapter\n");
        goto fail1;
    }

    client = i2c_new_device(adapter, &info);
    if (!client)
    {
        printk(KERN_ALERT"drv260x: Cannot create new device \n");
        goto fail1;
    }

    reval = i2c_add_driver(&drv260x_driver);
    if (reval)
    {
        printk(KERN_ALERT"drv260x driver initialization error \n");
        goto fail2;
    }

    drv260x->version = MKDEV(0,0);
    reval = alloc_chrdev_region(&drv260x->version, 0, 1, DEVICE_NAME);
    if (reval < 0)
    {
        printk(KERN_ALERT"drv260x: error getting major number %d\n", reval);
        goto fail3;
    }

    drv260x->class = class_create(THIS_MODULE, DEVICE_NAME);
    if (!drv260x->class)
    {
        printk(KERN_ALERT"drv260x: error creating class\n");
        goto fail4;
    }

    drv260x->device = device_create(drv260x->class, NULL, drv260x->version, NULL, DEVICE_NAME);
    if (!drv260x->device)
    {
        printk(KERN_ALERT"drv260x: error creating device 2605\n");
        goto fail5;
    }

    cdev_init(&drv260x->cdev, &fops);
    drv260x->cdev.owner = THIS_MODULE;
    drv260x->cdev.ops = &fops;
    reval = cdev_add(&drv260x->cdev, drv260x->version, 1);

    if (reval)
    {
        printk(KERN_ALERT"drv260x: fail to add cdev\n");
        goto fail6;
    }

    hrtimer_init(&vibdata.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    vibdata.timer.function = vibrator_timer_func;
    INIT_WORK(&vibdata.work, vibrator_work);
    INIT_WORK(&vibdata.work_play_eff, play_effect);

    wake_lock_init(&vibdata.wklock, WAKE_LOCK_SUSPEND, "vibrator");
    mutex_init(&vibdata.lock);

    printk(KERN_ALERT"drv260x: initialized\n");
    return 0;

fail6:
    device_destroy(drv260x->class, drv260x->version);
fail5:
    class_destroy(drv260x->class);
fail4:
    gpio_direction_output(GPIO_VIB_EN, GPIO_LEVEL_LOW);
    gpio_free(GPIO_VIB_EN);
fail3:
    i2c_del_driver(&drv260x_driver);
fail2:
    i2c_unregister_device(drv260x->client);
fail1:
    kfree(drv260x);
fail0:
    return reval;
}

static void drv260x_exit(void)
{
    gpio_direction_output(GPIO_VIB_EN, GPIO_LEVEL_LOW);
    gpio_free(GPIO_VIB_EN);
    device_destroy(drv260x->class, drv260x->version);
    class_destroy(drv260x->class);
    unregister_chrdev_region(drv260x->version, 1);
    i2c_unregister_device(drv260x->client);
    kfree(drv260x);
    i2c_del_driver(&drv260x_driver);

    printk(KERN_ALERT"drv260x: exit\n");
}

module_init(drv260x_init);
module_exit(drv260x_exit);
