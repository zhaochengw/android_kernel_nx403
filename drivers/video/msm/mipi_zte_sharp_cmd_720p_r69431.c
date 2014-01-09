/******************************************************************************

  Copyright (C), 2001-2012, ZTE. Co., Ltd.

 ******************************************************************************
  File Name     : mipi_zte_sharp_cmd_720p_r69431.c
  Version       : Initial Draft
  Author        : tangjun
  Created       : 2013/11/1
  Last Modified :
  Description   : This is lcd panel file
  Function List :
  History       :
  1.Date        :  2013/11/1
    Author      : tangjun 
    Modification: Created file

*****************************************************************************/


#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_zte_z5mini_lcd.h"
#include "mipi_toshiba.h"
#include <linux/gpio.h>
#include "mipi_zte_sharp_720p_r69431.h"

#define MAX_BL_LEVEL_R69431    255  
//#define CONFIG_ZTEMT_LCD_R69431_LOW_POWER

extern void v5_mhl_enable_irq(void);
extern unsigned int v5_intensity_value;

#ifdef CONFIG_ZTEMT_LCD_R69431_LOW_POWER
///30fps
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = { 
/* regulator */ 
	{0x09, 0x08, 0x05, 0x00, 0x20},
/* timing */ 
{0x66, 0x26, 0x0b, 0x00, 0x0c, 0x82, 0x1e, 0x87, 
0x0c, 0x03, 0x04, 0xa0}, 
	/* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x00, 0xcb, 0x31, 0xd9, 0x00, 0x20, 0x07, 0x62,
	0x41, 0x0f, 0x01,
	0x00, 0x14, 0x03, 0x0, 0x2, 0x0, 0x20, 0x0, 0x01},
};
#else
///60fps
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = { 
/* regulator */ 
	{0x09, 0x08, 0x05, 0x00, 0x20},
/* timing */ 
{0x66, 0x26, 0x18, 0x00, 0x1a, 0x8f, 0x1e, 0x8c, 
0x1a, 0x03, 0x04, 0xa0}, 
	/* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
	/* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
	{0x00, 0xcb, 0x31, 0xd9, 0x00, 0x20, 0x07, 0x62,
	0x41, 0x0f, 0x01,
	0x00, 0x14, 0x03, 0x0, 0x2, 0x0, 0x20, 0x0, 0x01},
};
#endif
#if 0
//45fps
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = { 
/* regulator */ 
{0x03, 0x0a, 0x04, 0x00, 0x20},
/* timing */ 
{0x74, 0x42, 0xF, 0x00, 0x4D, 0x53, 0x14, 0x45, 
0x17, 0x03, 0x04, 0xa0}, 
{0x5f, 0x00, 0x00, 0x10}, /* phy ctrl */ 
{0xff, 0x00, 0x06, 0x00}, /* strength */ 
/* pll control */ 
{0x0, 0x4a, 0x31, 0xda, 0x00, 0x20, 0x07, 0x62, 
0x41, 0x0f, 0x01, 
0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};
#endif

static char r69431_FF00[] = {0xFF, 0x00};
static char r69431_51[] = {0x51, 0xff};
static char r69431_530C[] = {0x53, 0x04};

//static char r69431_55[] = {0x55, 0x01};

static char r69431_B004[] = {0xB0, 0x04};
static char r69431_D6[] = {0xD6, 0x01};
#ifdef CONFIG_ZTEMT_LCD_R69431_LOW_POWER
static char r69431_DD[] = {0xdd, 0x21}; //low power 30fps
#endif
static char r69431_B30C[] = {0xb3, 0x0c};
//static char r69431_3500[] = {0x35, 0x00};
static char r69431_2A[] = {0x2a, 0x00, 0x00, 0x02, 0xcf};
static char r69431_2B[] = {0x2b, 0x00, 0x00, 0x04, 0xff};
static char r69431_29[] = {0x29, 0x00};
static char r69431_11[] = {0x11, 0x00};


static struct dsi_cmd_desc r69431_display_on_cmds_part1[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r69431_B004), r69431_B004},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r69431_D6), r69431_D6},
#ifdef CONFIG_ZTEMT_LCD_R69431_LOW_POWER
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r69431_DD), r69431_DD},
#endif
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(r69431_11), r69431_11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r69431_B30C), r69431_B30C},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_2A), r69431_2A},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_2B), r69431_2B},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_3500), r69431_3500},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 22, sizeof(r69431_29), r69431_29},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_51), r69431_51},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_530C), r69431_530C},
};


static struct dsi_cmd_desc r69431_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_FF00), r69431_FF00}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_51), r69431_51},
};

static char r69431_5300[] = {0x53, 0x00};
static char r69431_28[] = {0x28, 0x00};
static char r69431_10[] = {0x10, 0x00};
static struct dsi_cmd_desc r69431_display_off_short_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_5300), r69431_5300},
	{DTYPE_DCS_WRITE, 1, 0, 0, 22, sizeof(r69431_28), r69431_28},
	{DTYPE_DCS_WRITE, 1, 0, 0, 122, sizeof(r69431_10), r69431_10},
};

static struct dsi_cmd_desc r69431_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_5300), r69431_5300},
	{DTYPE_DCS_WRITE, 1, 0, 0, 22, sizeof(r69431_28), r69431_28},
	{DTYPE_DCS_WRITE, 1, 0, 0, 122, sizeof(r69431_10), r69431_10},
};

static char r69431_manufacture_id[] = {0x04,0x00};

static struct dsi_cmd_desc r69431_manufacture_id_cmd[] = {
    {DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(r69431_manufacture_id), r69431_manufacture_id}
};

static char sharpca_basic_9b[] = {0xca, 
	0x01,0x80,0x98,0x98,0x9b,0x40,0xbe,0xbe,0x20,0x20,	
	0x80,0xfe,0x0a,0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,
	0x20,0x10,0x3f,0x3f,0x00,0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,};
static struct dsi_cmd_desc sharp_720p_ec_improve_cmd[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(sharpca_basic_9b), sharpca_basic_9b},
};

static char sharpca_basic_92[] = {0xca, 
	0x01,0x80,0x92,0x92,0x9b,0x75,0x9b,0x9b,0x20,0x20,	
	0x80,0xfe,0x0a,0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,
	0x20,0x10,0x3f,0x3f,0x00,0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,};
static struct dsi_cmd_desc sharp_720p_ec_normal_cmd[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(sharpca_basic_92), sharpca_basic_92},

};

static char sharpca_basic_norm[] = {0xca, 
	0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x08,0x20,	
	0x80,0x80,0x0a,0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,
	0x20,0x10,0x3f,0x3f,0x00,0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,};
static struct dsi_cmd_desc sharp_720p_ec_weak_cmd[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 1, sizeof(sharpca_basic_norm), sharpca_basic_norm},
};

void zte_mipi_disp_inc_r69431(unsigned int state)
{
	struct dcs_cmd_req cmdreq;
	unsigned int value;
	value =state;

	pr_err("yls...%s value=%d.\n", __func__, value);
	switch (value) {
	case INTENSITY_NORMAL:
		cmdreq.cmds = sharp_720p_ec_weak_cmd;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_720p_ec_weak_cmd);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	
		break;
	case INTENSITY_01:

		cmdreq.cmds = sharp_720p_ec_normal_cmd;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_720p_ec_normal_cmd);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	

		break;
	case INTENSITY_02:

		cmdreq.cmds = sharp_720p_ec_improve_cmd;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_720p_ec_improve_cmd);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	
		
		break;
	default:

		cmdreq.cmds = sharp_720p_ec_normal_cmd;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_720p_ec_normal_cmd);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	
		
		break;

	}
	
}

 uint32 r69431_read_manufacture_id(struct msm_fb_data_type *mfd)
{
    struct dsi_buf *rp, *tp;
    struct dsi_cmd_desc *cmd;
    uint32 *lp;
    uint32 id=0xff;

	tp = &z5mini_tx_buf;
	rp = &z5mini_rx_buf;
	cmd = r69431_manufacture_id_cmd;
	
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);	
	mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */	
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 2);
	
	lp = (uint32 *)rp->data;
	id=(((*lp)&0xff00)>>8);
	pr_info("%s: manufacture_id=0x%x,id=0x%x\n", __func__, (*lp)&0xffff ,id);	
	return id;
}

 void mipi_r69431_set_backlight(struct msm_fb_data_type *mfd)
{
	struct dcs_cmd_req cmdreq;
	static bool once = true;

	if (mfd->bl_level < 0)
		mfd->bl_level = 0;
	if (mfd->bl_level > MAX_BL_LEVEL_R69431)
		mfd->bl_level = MAX_BL_LEVEL_R69431;
		
	if (0 == mfd->bl_level) {
		r69431_51[1] = 0x00;
	} else {
		if (mfd->bl_level <= 10) {
			r69431_51[1] = 1;
		} else if (mfd->bl_level < 21){
			r69431_51[1] = 3;
		} else {
			r69431_51[1] = mfd->bl_level - 9;
		}
	}

	pr_debug("%s [%s,%d]level=%d\n", __func__, current->comm,current->pid,mfd->bl_level);
	cmdreq.cmds = r69431_backlight_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(r69431_backlight_cmds);

	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);	
	if (mfd->bl_level == 0)
		pr_err("%s [%s,%d]level=%d\n", __func__, current->comm,current->pid,mfd->bl_level);

	if (once){
		v5_mhl_enable_irq();
		once = false;
	}

}


int  mipi_r69431_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct dcs_cmd_req cmdreq;	
	static bool is_firsttime = true;

  pr_debug("lcd:%s disp_initialized=%d\n",__func__,z5mini_lcd_state.disp_initialized);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (is_firsttime) {
		zte_mipi_disp_inc_r69431(v5_intensity_value);
		is_firsttime = false;
		return 0;
	}
	
	if (!z5mini_lcd_state.disp_initialized) {
		mipi_set_tx_power_mode(1);

		cmdreq.cmds = r69431_display_on_cmds_part1;
		cmdreq.cmds_cnt = ARRAY_SIZE(r69431_display_on_cmds_part1);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
		
		zte_mipi_disp_inc_r69431(v5_intensity_value);
		
		z5mini_lcd_state.display_on = TRUE;
		z5mini_lcd_state.disp_initialized = TRUE;
	}
	pr_err("%s [%s,%d]out\n", __func__, current->comm,current->pid);

	return 0;
}

int mipi_r69431_lcd_short_off(void)
{	
	struct dcs_cmd_req cmdreq;

  pr_debug("lcd:%s \n",__func__);	
 
	cmdreq.cmds = r69431_display_off_short_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(r69431_display_off_short_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
	return 0;
}

int mipi_r69431_lcd_off(void)
{	
	struct dcs_cmd_req cmdreq;

  pr_debug("lcd:%s display_on=%d\n",__func__,z5mini_lcd_state.display_on);  

	if (z5mini_lcd_state.display_on) {
		
		cmdreq.cmds = r69431_display_off_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(r69431_display_off_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);
		
		z5mini_lcd_state.display_on = FALSE;
		z5mini_lcd_state.disp_initialized = FALSE;
	}
	return 0;
}


void mipi_video_sharp_r69431_panel_info(struct msm_panel_info *pinfo)
{

	pinfo->xres = 720;
	pinfo->yres = 1280;
	pinfo->lcdc.xres_pad = 0;
	pinfo->lcdc.yres_pad = 0;

	pinfo->type = MIPI_CMD_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;

//	pinfo->lcdc.h_back_porch = 80;
//	pinfo->lcdc.h_front_porch = 120;
//	pinfo->lcdc.h_pulse_width = 20;
	pinfo->lcdc.v_back_porch = 11;
	pinfo->lcdc.v_front_porch = 10;
	pinfo->lcdc.v_pulse_width = 5;

	pinfo->lcdc.border_clr = 0;	/* blk */
	pinfo->lcdc.underflow_clr = 0xff;	/* blue */
	pinfo->lcdc.hsync_skew = 0;
	pinfo->bl_max = MAX_BL_LEVEL_R69431;
	pinfo->bl_min = 1;
	pinfo->fb_num = 2;

	pinfo->clk_rate = 331210000;	//60fps 409665600;479196000  359397000 441612000
	
	pinfo->lcd.vsync_enable = TRUE; //
	pinfo->lcd.hw_vsync_mode = TRUE;//
	
	pinfo->lcd.refx100 = 4800; /* adjust refx100 to prevent tearing */
	
	pinfo->lcd.v_back_porch = 8;
	pinfo->lcd.v_front_porch = 13;
	pinfo->lcd.v_pulse_width = 4;

	pinfo->mipi.mode = DSI_CMD_MODE;
	
	pinfo->mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo->mipi.vc = 0;
	pinfo->mipi.esc_byte_ratio = 4;
	pinfo->mipi.data_lane0 = TRUE;
	pinfo->mipi.data_lane1 = TRUE;
	pinfo->mipi.data_lane2 = TRUE;
	pinfo->mipi.data_lane3 = TRUE;

	pinfo->mipi.t_clk_post = 0x09;
	pinfo->mipi.t_clk_pre = 0x1D;
	pinfo->mipi.stream = 0; /* dma_p */
	pinfo->mipi.mdp_trigger = DSI_CMD_TRIGGER_SW_TE;//DSI_CMD_TRIGGER_NONE;//	
	pinfo->mipi.dma_trigger = DSI_CMD_TRIGGER_SW;//
	pinfo->mipi.frame_rate = 60;
	pinfo->mipi.te_sel =  1; /* TE from vsycn gpio */
	pinfo->mipi.interleave_max = 1;
	pinfo->mipi.insert_dcs_cmd = TRUE;
	pinfo->mipi.wr_mem_continue = 0x3c;
	pinfo->mipi.wr_mem_start = 0x2c;
	pinfo->mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo->mipi.tx_eot_append = TRUE;
}

