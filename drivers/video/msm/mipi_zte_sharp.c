/******************************************************************************

  Copyright (C), 2001-2012, ZTE. Co., Ltd.

 ******************************************************************************
  File Name     : mipi_zte_sharp.c
  Version       : Initial Draft
  Author        : congshan 
  Created       : 2012/9/22
  Last Modified :
  Description   : This is lcd driver board file
  Function List :
  History       :
  1.Date        :  2012/9/22
    Author      : congshan 
    Modification: Created file

*****************************************************************************/


#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_toshiba.h"
#include <linux/gpio.h>

static struct mipi_dsi_panel_platform_data *mipi_toshiba_pdata;
#define MAX_BL_LEVEL 70
#define TM_GET_PID(id) (((id) & 0xff00)>>8)
static int lcd_gpio_reset;
static int lcd_mpp2;
static struct dsi_buf toshiba_tx_buf;
static struct dsi_buf toshiba_rx_buf;
static int mipi_toshiba_lcd_init(void);
struct ilitek_state_type{
	boolean disp_initialized;
	boolean display_on;
};

struct ilitek_state_type ilitek_state = 
{
    .disp_initialized = FALSE,
	.display_on = TRUE,
};
unsigned int v5_intensity_value;
enum {
	INTENSITY_NORMAL=24,
	INTENSITY_01,
	INTENSITY_02
};
void zte_mipi_disp_inc(unsigned int state);

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};
static char toshibaB0[] = {0xB0, 0x04};
static char toshiba00[] = {0x00, 0x00};
static char toshiba01[] = {0x00, 0x00};
static char toshibaC3[] = {0xc3, 0x01, 0x00, 0x10};
static char toshibaCE[] = {0xce, 0x00, 0x01, 0x88, 0xc1, 0x00, 0x1e, 0x04};
static char toshibad6[] = {0xd6, 0x01};

static char toshibaB01[] = {0xB0, 0x03};
static char toshiba02[] = {0x00, 0x00};
static char toshiba03[] = {0x00, 0x00};
static char toshiba55[] = {0x55, 0x01};
static char toshiba53[] = {0x53, 0x2c};
static char toshiba35[] = {0x35, 0x00};
static char toshiba29[] = {0x29, 0x00};
static char toshiba11[] = {0x11, 0x00};
static char toshiba51[] = {0x51, 0x0f, 0xff};
static struct dsi_cmd_desc toshiba_1080p_display_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(toshibaB0), toshibaB0},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(toshiba00), toshiba00},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(toshiba01), toshiba01},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(toshibaC3), toshibaC3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(toshibaCE), toshibaCE},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(toshibad6), toshibad6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(toshibaB01), toshibaB01},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(toshiba02), toshiba02},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(toshiba03), toshiba03},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(toshiba55), toshiba55},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(toshiba53), toshiba53},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(toshiba35), toshiba35},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(toshiba29), toshiba29},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(toshiba11), toshiba11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(toshiba51), toshiba51},
};

static struct dsi_cmd_desc toshiba_1080p_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(toshiba51), toshiba51},
};

static struct dsi_cmd_desc toshiba_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50, sizeof(display_off), display_off},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static char manufacture_id1[] = {0x04,0x00};
static char backlight_read[] = {0x52,0x00};
static char read_0c[] = {0x0c,0x00};

static struct dsi_cmd_desc toshiba_manufacture_id_cmd[] = {
    {DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(manufacture_id1), manufacture_id1}
};
static struct dsi_cmd_desc toshiba_backlight_read_cmd[] = {
    {DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(backlight_read), backlight_read}
};
static struct dsi_cmd_desc toshiba_read_0c_cmd[] = {
    {DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(read_0c), read_0c}
};

static char sharpB0[] = {0xB0, 0x04};

static char sharp00[] = {0x00, 0x00};
static char sharp01[] = {0x00, 0x00};

static char sharpCE[] = {0xce, 0x00, 0x01, 0x88, 0xc1, 0x00, 0x1e, 0x04};

static char sharpd6[] = {0xd6, 0x01};
//static char sharp51[] = {0x51, 0x0f, 0xff};
static char sharp53[] = {0x53, 0x24};
static char sharp29[] = {0x29, 0x00};
static char sharp11[] = {0x11, 0x00};
/*added by congshan 20121127 start */

static char sharpca_basic_9b[] = {0xca, 0x01,0x80, 0x98,0x98,0xbe,0xbe,0xbe,0xbe,0x20,0x20,
	0x80,0xfe,0x0a, 0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,0x20,0x10,0x3f,0x3f,0x00,
	0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,
};
static struct dsi_cmd_desc sharp_1080p_display_9b[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpca_basic_9b), sharpca_basic_9b},

};

static char sharpca_basic_92[] = {0xca, 0x01,0x80, 0x92,0x92,0x9b,0x9b,0x9b,0x9b,0x20,0x20,
	0x80,0xfe,0x0a, 0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,0x20,0x10,0x3f,0x3f,0x00,
	0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,
};

static struct dsi_cmd_desc sharp_1080p_display_92[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpca_basic_92), sharpca_basic_92},

};
static char sharpca_basic_norm[] = {0xca, 0x00,0x80, 0x80,0x80,0x80,0x80,0x80,0x80,0x08,0x20,
	0x80,0x80,0x0a, 0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,0x20,0x10,0x3f,0x3f,0x00,
	0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,
};

static struct dsi_cmd_desc sharp_1080p_display_norm[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpca_basic_norm), sharpca_basic_norm},

};
/*added by congshan 20121127 end */


static struct dsi_cmd_desc sharp_1080p_display_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpB0), sharpB0},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(sharp00), sharp00},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(sharp01), sharp01},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpd6), sharpd6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpCE), sharpCE},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(sharp51), sharp51},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(toshiba55), toshiba55},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(sharp53), sharp53},
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(sharp29), sharp29},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(sharp11), sharp11},

};
static struct dsi_cmd_desc sharp_read_b0_cmd[] = {
    {DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(sharpB0), sharpB0}
};
static struct dsi_cmd_desc sharp_read_d6_cmd[] = {
    {DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(sharpd6), sharpd6}
};
static struct dsi_cmd_desc sharp_read_53_cmd[] = {
    {DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(sharp53), sharp53}
};

static void lcd_pmgpio_reset (void)
{
	gpio_set_value_cansleep(lcd_gpio_reset, 1);
	msleep(10);
	gpio_set_value_cansleep(lcd_gpio_reset, 0);
	msleep(10);
	gpio_set_value_cansleep(lcd_gpio_reset, 1);
	msleep(10);
}

static uint32 toshiba_manufacture_id(struct msm_fb_data_type *mfd)
{
    struct dsi_buf *rp, *tp;
    struct dsi_cmd_desc *cmd;
	uint32 *lp;
	if (0) {
	tp = &toshiba_tx_buf;
	rp = &toshiba_rx_buf;
	cmd = toshiba_manufacture_id_cmd;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);
	mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 2);
	lp = (uint32 *)rp->data;
	pr_info("%s: manufacture_id=0x%x\n", __func__, (*lp)&0xff );
	return *lp&0xff;
	}
	return 0;
}
static uint32 toshiba_backlight_read(struct msm_fb_data_type *mfd)
{
    struct dsi_buf *rp, *tp;
    struct dsi_cmd_desc *cmd;
	uint32 *lp;
	tp = &toshiba_tx_buf;
	rp = &toshiba_rx_buf;
	cmd = toshiba_backlight_read_cmd;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);
	mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 2);
	lp = (uint32 *)rp->data;
	pr_info("%s: manufacture_id=0x%x\n", __func__, (*lp));
	return *lp&0xff;
}
static uint32 toshiba_pixel_read(struct msm_fb_data_type *mfd)
{
    struct dsi_buf *rp, *tp;
    struct dsi_cmd_desc *cmd;
	uint32 *lp;
	tp = &toshiba_tx_buf;
	rp = &toshiba_rx_buf;
	cmd = toshiba_read_0c_cmd;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);
	mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 2);
	lp = (uint32 *)rp->data;
	pr_info("%s: pixel=0x%x\n", __func__, (*lp));
	cmd = sharp_read_b0_cmd;
		mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);
	mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 2);
	lp = (uint32 *)rp->data;
	pr_info("%s: b0=0x%x\n", __func__, (*lp));

	cmd = sharp_read_d6_cmd;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);
	mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 2);
	lp = (uint32 *)rp->data;
	pr_info("%s: d6=0x%x\n", __func__, (*lp));
	cmd = sharp_read_53_cmd;
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);
	mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 2);
	lp = (uint32 *)rp->data;
	pr_info("%s: 53=0x%x\n", __func__, (*lp));
	return *lp&0xff;
}

static int mipi_toshiba_lcd_on(struct platform_device *pdev)
{
	struct dcs_cmd_req cmdreq;
	struct msm_fb_data_type *mfd;
	static bool is_firsttime = false;
	
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	if (0)
		lcd_pmgpio_reset();
	if (is_firsttime)
		return 0;
	if (!ilitek_state.disp_initialized) {
		if (0x01 == toshiba_manufacture_id(mfd)) {
			pr_err("sharp lcd on\n");
//			mipi_dsi_cmds_tx(&toshiba_tx_buf,
//				sharp_1080p_display_on_cmds,
//				ARRAY_SIZE(sharp_1080p_display_on_cmds));
			
			cmdreq.cmds = sharp_1080p_display_on_cmds;
			cmdreq.cmds_cnt = ARRAY_SIZE(sharp_1080p_display_on_cmds);
			
			cmdreq.flags = CMD_REQ_COMMIT;
			cmdreq.rlen = 0;
			cmdreq.cb = NULL;
			
			mipi_dsi_cmdlist_put(&cmdreq);	
		}else if (0x03 == toshiba_manufacture_id(mfd)){
//			mipi_dsi_cmds_tx(&toshiba_tx_buf,
//				toshiba_1080p_display_on_cmds,
//				ARRAY_SIZE(toshiba_1080p_display_on_cmds));

			cmdreq.cmds = toshiba_1080p_display_on_cmds;
			cmdreq.cmds_cnt = ARRAY_SIZE(toshiba_1080p_display_on_cmds);
			
			cmdreq.flags = CMD_REQ_COMMIT;
			cmdreq.rlen = 0;
			cmdreq.cb = NULL;
			
			mipi_dsi_cmdlist_put(&cmdreq);	
		
		} else {
//				mipi_dsi_cmds_tx(&toshiba_tx_buf,
//					sharp_1080p_display_on_cmds,
//					ARRAY_SIZE(sharp_1080p_display_on_cmds));

			cmdreq.cmds = sharp_1080p_display_on_cmds;
			cmdreq.cmds_cnt = ARRAY_SIZE(sharp_1080p_display_on_cmds);
			
			cmdreq.flags = CMD_REQ_COMMIT;
			cmdreq.rlen = 0;
			cmdreq.cb = NULL;
			
			mipi_dsi_cmdlist_put(&cmdreq);	
		}
		zte_mipi_disp_inc(v5_intensity_value);
		ilitek_state.display_on = TRUE;
		ilitek_state.disp_initialized = TRUE;
	}
	is_firsttime = false;
	if (0)
	toshiba_pixel_read(mfd);
	pr_err("%s [%s,%d]out\n", __func__, current->comm,current->pid);
	return 0;
}

static int mipi_toshiba_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);
	pr_err("%s\n", __func__);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	return 0;
}
struct led_brightness_st{
	uint16 brightness1;
	uint16 brightness2;
};
//static uint16 led_brightness[30]={0x00,0x20,0x28,0x30,0x38,0x40,0x48,0x50,0x58,0x60,0x68,0x70,0x78,
//0x80,0x88,0x90,0x98,0xA0,0xA8,0xB0,0xB8,0xC0,0xC8,0xD0,0xD8,0xE0,0xE8,0xF0,0xF8,0xFF};
static struct led_brightness_st led_brightness[]={

	{ 0x00, 0x00},
	{ 0x00, 0x05},
	{ 0x00, 0x0a},
	{ 0x00, 0x11},
	{ 0x00, 0x19},
	{ 0x00, 0x22},
	{ 0x00, 0x33},
	{ 0x00, 0x44},
	{ 0x00, 0x66},
	{ 0x00, 0x99},
	{ 0x00, 0xcc},
	{ 0x00, 0xff},
	{ 0x01, 0x33},
	{ 0x01, 0x66},
	{ 0x01, 0x99},
	{ 0x01, 0xcc},
	{ 0x01, 0xff},
	{ 0x02, 0x33},
	{ 0x02, 0x66},
	{ 0x02, 0x99},
	{ 0x02, 0xcc},
	{ 0x02, 0xff},
	{ 0x03, 0x33},
	{ 0x03, 0x66},
	{ 0x03, 0x99},
	{ 0x03, 0xcc},
	{ 0x03, 0xff},
	{ 0x04, 0x33},
	{ 0x04, 0x66},
	{ 0x04, 0x99},
	{ 0x04, 0xcc},
	{ 0x04, 0xff},
	{ 0x05, 0x33},
	{ 0x05, 0x66},
	{ 0x05, 0x99},
	{ 0x05, 0xcc},
	{ 0x05, 0xff},
	{ 0x06, 0x33},
	{ 0x06, 0x66},
	{ 0x06, 0x99},
	{ 0x06, 0xcc},
	{ 0x06, 0xff},
	{ 0x07, 0x33},
	{ 0x07, 0x66},
	{ 0x07, 0x99},
	{ 0x07, 0xcc},
	{ 0x07, 0xff},
	{ 0x08, 0x33},
	{ 0x08, 0x66},
	{ 0x08, 0x99},
	{ 0x08, 0xcc},
	{ 0x08, 0xff},
	{ 0x09, 0x33},
	{ 0x09, 0x66},
	{ 0x09, 0x99},
	{ 0x09, 0xcc},
	{ 0x09, 0xff},
	{ 0x0A, 0x33},
	{ 0x0A, 0x66},
	{ 0x0A, 0x99},
	{ 0x0A, 0xcc},
	{ 0x0A, 0xff},
	{ 0x0B, 0x33},
	{ 0x0B, 0x66},
	{ 0x0B, 0x99},
	{ 0x0B, 0xcc},
	{ 0x0B, 0xff},
	{ 0x0C, 0x33},
	{ 0x0C, 0x66},
	{ 0x0C, 0x99},
	{ 0x0C, 0xcc},
	{ 0x0C, 0xff},
	{ 0x0D, 0x33},
	{ 0x0D, 0x66},
	{ 0x0D, 0x99},
	{ 0x0D, 0xcc},
	{ 0x0D, 0xff},
	{ 0x0E, 0x33},
	{ 0x0E, 0x66},
	{ 0x0E, 0x99},
	{ 0x0E, 0xcc},
	{ 0x0E, 0xff},
	{ 0x0F, 0x33},
	{ 0x0F, 0x66},
	{ 0x0F, 0x99},
	{ 0x0F, 0xcc},
	{ 0x0F, 0xff},
	
};
/*added by congshan 20121127 start */

void zte_mipi_disp_inc(unsigned int state)
{
	struct dcs_cmd_req cmdreq;
	unsigned int value;
	value =state;

	//pr_err("sss %s value=%d\n", __func__, value);
	switch (value) {
	case INTENSITY_NORMAL:
//		mipi_dsi_cmds_tx(&toshiba_tx_buf,sharp_1080p_display_norm,
//			ARRAY_SIZE( sharp_1080p_display_norm));

		cmdreq.cmds = sharp_1080p_display_norm;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_1080p_display_norm);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	

		break;
	case INTENSITY_01:
//		mipi_dsi_cmds_tx(&toshiba_tx_buf,sharp_1080p_display_92,
//			ARRAY_SIZE( sharp_1080p_display_92));

		cmdreq.cmds = sharp_1080p_display_92;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_1080p_display_92);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	

		break;
	case INTENSITY_02:
//		mipi_dsi_cmds_tx(&toshiba_tx_buf,sharp_1080p_display_9b,
//			ARRAY_SIZE( sharp_1080p_display_9b));

		cmdreq.cmds = sharp_1080p_display_9b;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_1080p_display_9b);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	
		
		break;
	default:
		//pr_err("sss default\n");
//		mipi_dsi_cmds_tx(&toshiba_tx_buf,sharp_1080p_display_92,
//			ARRAY_SIZE( sharp_1080p_display_92));

		cmdreq.cmds = sharp_1080p_display_92;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_1080p_display_92);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	

		
		break;

	}
	
}
/*added by congshan 20121127 end */
void v5_mhl_enable_irq(void);

static void mipi_toshiba_set_backlight(struct msm_fb_data_type *mfd)
{
	struct dcs_cmd_req cmdreq;
	static bool once = true;
	//static int bk_old;
	if (mfd->bl_level < 0)
		mfd->bl_level = 0;
	if (mfd->bl_level > MAX_BL_LEVEL)
		mfd->bl_level = MAX_BL_LEVEL;
		
	if (0 == mfd->bl_level) {
		toshiba51[1] = 0x00;
		toshiba51[2] = 0x00;
	} else {
		toshiba51[1] = led_brightness[mfd->bl_level].brightness1;
		toshiba51[2] = led_brightness[mfd->bl_level].brightness2;
	}
	//if (bk_old == mfd->bl_level)
	//	return;	
	//pr_err("%s [%s,%d]level=%d in\n", __func__, current->comm,current->pid,mfd->bl_level);
//	mipi_dsi_cmds_tx(&toshiba_tx_buf, toshiba_1080p_backlight_cmds,
//			ARRAY_SIZE(toshiba_1080p_backlight_cmds));
	
	cmdreq.cmds = toshiba_1080p_backlight_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(toshiba_1080p_backlight_cmds);
	
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	
	mipi_dsi_cmdlist_put(&cmdreq);	


	
	//if (0 == mfd->bl_level)
	//	pr_err("%s [%s,%d]level=%d out\n", __func__, current->comm,current->pid,mfd->bl_level);
	//bk_old = mfd->bl_level;

	if (0){
		mipi_dsi_op_mode_config(1);
		toshiba_backlight_read(mfd);
		mipi_dsi_op_mode_config(0);
	}
	if (once){
		v5_mhl_enable_irq();
		once = false;
	}

}
int zte_sharp_lcd_off(void)
{
	struct dcs_cmd_req cmdreq;
	
	if (ilitek_state.display_on) {

		cmdreq.cmds = toshiba_display_off_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(toshiba_display_off_cmds);

		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);	
		
		ilitek_state.display_on = FALSE;
		ilitek_state.disp_initialized = FALSE;
	}
	return 0;
}

static int __devinit mipi_toshiba_lcd_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		mipi_toshiba_pdata = pdev->dev.platform_data;
		lcd_gpio_reset = mipi_toshiba_pdata->gpio[0];
		lcd_mpp2 = mipi_toshiba_pdata->gpio[1];
		return 0;
	}

	msm_fb_add_device(pdev);
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_toshiba_lcd_probe,
	.driver = {
		.name   = "mipi_zte_sharp_panel",
	},
};

static struct msm_fb_panel_data toshiba_panel_data = {
	.on		= mipi_toshiba_lcd_on,
	.off		= mipi_toshiba_lcd_off,
	.set_backlight  = mipi_toshiba_set_backlight,
};

static int ch_used[3];

int mipi_sharp_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;
	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_toshiba_lcd_init();
	if (ret) {
		pr_err("mipi_toshiba_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_zte_sharp_panel", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	toshiba_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &toshiba_panel_data,
		sizeof(toshiba_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int mipi_toshiba_lcd_init(void)
{
	mipi_dsi_buf_alloc(&toshiba_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&toshiba_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
