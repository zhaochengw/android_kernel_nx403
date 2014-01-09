/******************************************************************************

  Copyright (C), 2001-2012, ZTE. Co., Ltd.

 ******************************************************************************
  File Name     : mipi_zte_sharp_video_720p.c
  Version       : Initial Draft
  Author        : tanyijun
  Created       : 2013/01/06
  Last Modified :
  Description   : This is lcd driver board file
  Function List :
  History       :
  1.Date        :  2013/01/06
    Author      : tanyijun 
    Modification: Created file

*****************************************************************************/


#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_zte_z5mini_lcd.h"
#include "mipi_zte_sharp_video_720p_r63415.h"

#define MAX_BL_LEVEL_R63415   81


//static struct msm_panel_info pinfo;
extern unsigned int v5_intensity_value;

extern void v5_mhl_enable_irq(void);

struct led_brightness_st r63415_led_brightness[]={
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

static struct mipi_dsi_phy_ctrl r63415_dsi_video_mode_phy_db = {
    /* regulator */
	{0x09, 0x08, 0x05, 0x00, 0x20},
	/* timing */
	{0x77, 0x1b, 0x11, 0x00, 0x2f, 0x38, 0x16, 0x1f,
	0x20, 0x03, 0x04, 0xa0},		
    /* phy ctrl */
	{0x5f, 0x00, 0x00, 0x10},
    /* strength */
	{0xff, 0x00, 0x06, 0x00},
	/* pll control */
//	{0x0, 0xcc, 0x31, 0xda, 0x00, 0x40, 0x03, 0x62,
	{0x0, 0x10, 0x30, 0xc0, 0x00, 0x10, 0x0f, 0x61,	
	 0x40, 0x07, 0x03,
	 0x00, 0x1a, 0x00, 0x00, 0x02, 0x00, 0x20, 0x00, 0x02 },
};

/*sharp hd lcd initial code,driver IC R63415,tanyijun add */
static char r63415_B0[] = {0xB0, 0x04};
static char r63415_B3[] = {0xB3, 0x40, 0x00, 0x22, 0x00, 0x00 };//
static char r63415_B4[] = {0xB4, 0x0C, 0x12};//
static char r63415_B6[] = {0xB6, 0x31, 0xB5};//
static char r63415_CC[] = {0xCC, 0x06}; //

static char r63415_CE[] = {0xCE, 0x03,0x0F,0x90,0x00,0x00}; 

static char r63415_C1[] = {0xC1, 0x04, 0x64, 0x10, 0x41, 0x00, 0x00, 0x8E, 0x29, 0xEF, 
	                                       0xBD, 0xF7, 0xDE, 0x7B, 0xEF, 0xBD, 0xF7, 0xDE, 0x7B, 0xC5, 
	                                       0x1C,0x02, 0x86, 0x08, 0x22, 0x22, 0x00, 0x20}; //
static char r63415_C2[] = {0xC2, 0x20, 0xF5, 0x00, 0x14, 0x08, 0x00}; //
static char r63415_C3[] = {0xC3, 0x00, 0x00, 0x00}; //
static char r63415_C4[] = {0xC4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 
	                                       0x00, 0x00, 0x00, 0x08}; //
static char r63415_C6[] = {0xC6, 0xA5, 0x00, 0xA4, 0x05, 0x8A, 0x00, 0x00, 0x00, 0x00, 
	                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x21, 
	                                       0x10, 0xA5, 0x00, 0xA4, 0x05, 0x8A, 0x00, 0x00, 0x00, 0x00, 
	                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0B, 0x21, 0x10,}; //
static char r63415_D0[] = {0xD0, 0x10, 0x4C, 0x18, 0xCC, 0xDA, 0x5A, 0x01, 0x8A, 0x01, 0xBB, 
	                                       0x58, 0x4A}; //
static char r63415_D2[] = {0xD2, 0xB8}; //
static char r63415_D3[] = {0xD3, 0x1A, 0xB3, 0xBB, 0xFF, 0x77, 0x33, 0x33, 0x33, 0x00, 0x01, 
	                                       0x00, 0xA0, 0x38, 0xA0, 0x00, 0xDB, 0xB7, 0x33, 0xA2, 0x72, 0xDB }; //
static char r63415_D5[] = {0xD5, 0x06, 0x00, 0x00, 0x01, 0x35, 0x01, 0x35}; //
static char r63415_D6[] = {0xD6 ,0x01}; //
static char r63415_D7[] = {0xD7, 0x20, 0x80, 0xFC, 0xFF, 0x7F, 0x22, 0xA2, 0xA2, 0x80, 0x0A,
	                                       0xF0, 0x60, 0x7E, 0x00, 0x3C, 0x18, 0x40, 0x05, 0x7E, 0x00, 0x00, 0x00}; //
static char r63415_2A[] = {0x2A, 0x00, 0x00, 0x02, 0xCF}; //
static char r63415_2B[] = {0x2B, 0x00, 0x00, 0x04, 0xFF}; //
static char r63415_3A[] = {0x3a, 0x77};
static char r63415_51[] = {0x51, 0x0f, 0xff};
//static char r63415_53[] = {0x53, 0x04};
static char r63415_53[] = {0x53, 0x24};
static char r63415_55[] = {0x55, 0x01};
static char r63415_35[] = {0x35, 0x00};//
static char r63415_29[] = {0x29, 0x00};
static char r63415_11[] = {0x11, 0x00};
static char r63415_2C[] = {0x2C, 0X00};

static struct dsi_cmd_desc r63415_backlight_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_B0), r63415_B0}, 
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_CE), r63415_CE}, 		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r63415_51), r63415_51},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r63415_55), r63415_55}, 	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r63415_53), r63415_53}, 	
};

static struct dsi_cmd_desc r63415_display_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_B0), r63415_B0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_B3), r63415_B3},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_B4), r63415_B4},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_B6), r63415_B6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_3A), r63415_3A},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_CC), r63415_CC},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_C1), r63415_C1},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_C2), r63415_C2},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_C3), r63415_C3},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_C4), r63415_C4},		
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_C6), r63415_C6},		
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_D0), r63415_D0},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_D2), r63415_D2},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_D3), r63415_D3},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_D5), r63415_D5},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_D6), r63415_D6},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_D7), r63415_D7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_2A), r63415_2A},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_2B), r63415_2B},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_35), r63415_35},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r63415_51), r63415_51},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(r63415_55), r63415_55},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(r63415_53), r63415_53},
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(r63415_29), r63415_29},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(r63415_11), r63415_11},
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(r63415_2C), r63415_2C},	

};

static char r63415_28[2] = {0x28, 0x00};
static char r63415_10[2] = {0x10, 0x00};

static struct dsi_cmd_desc r63415_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50, sizeof(r63415_28), r63415_28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(r63415_10), r63415_10}
};

#if 1
static char sharpca_basic_9b[] = {0xca, 0x01,0x80, 0xc4,0xae,0xaa,0xaa,0xb2,0xb4,0x1d,0x23,
	0x80,0xfe,0x0a, 0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,0x20,0x10,0x3f,0x3f,0x00,
	0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,
}; // VFinal0221
#else
static char sharpca_basic_9b[] = {0xca, 0x01,0x80, 0xc4,0xae,0xaa,0xaa,0xb2,0xb4,0x1d,0x23,
	0x80,0xfe,0x0a, 0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,0x20,0x10,0x3f,0x3f,0x00,
	0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,
}; // VFinal0221
static char sharpca_basic_9b[] = {0xca, 0x01,0x80, 0xc9,0xb4,0xb0,0xb0,0xb0,0xb4,0x1c,0x26,
	0x80,0xfe,0x0a, 0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,0x20,0x10,0x3f,0x3f,0x00,
	0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,
};  ////V1
static char sharpca_basic_9b[] = {0xca, 0x01,0x80, 0x98,0x98,0xbe,0xbe,0xbe,0xbe,0x20,0x20,
	0x80,0xfe,0x0a, 0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,0x20,0x10,0x3f,0x3f,0x00,
	0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,
};
#endif
static struct dsi_cmd_desc sharp_1080p_display_9b[] = {
//	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_B0), r63415_B0},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpca_basic_9b), sharpca_basic_9b},
};

static char sharpca_basic_92[] = {0xca, 0x01,0x80, 0x92,0x92,0x9b,0x9b,0x9b,0x9b,0x20,0x20,
	0x80,0xfe,0x0a, 0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,0x20,0x10,0x3f,0x3f,0x00,
	0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,
};

static struct dsi_cmd_desc sharp_1080p_display_92[] = {
//	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_B0), r63415_B0},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpca_basic_92), sharpca_basic_92},
};
static char sharpca_basic_norm[] = {0xca, 0x00,0x80, 0x80,0x80,0x80,0x80,0x80,0x80,0x08,0x20,
	0x80,0x80,0x0a, 0x4a,0x37,0xa0,0x55,0xf8,0x0c,0x0c,0x20,0x10,0x3f,0x3f,0x00,
	0x00,0x10,0x10,0x3f,0x3f,0x3f,0x3f,
};

static struct dsi_cmd_desc sharp_1080p_display_norm[] = {
//	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r63415_B0), r63415_B0},	
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(sharpca_basic_norm), sharpca_basic_norm},

};
/*added by congshan 20121127 end */
void zte_mipi_disp_inc(unsigned int state)
{
	unsigned int value;
	value =state;
	printk("=========sss %s value=%d1234567==========\n", __func__, value);
	switch (value) {
	case INTENSITY_NORMAL:
		mipi_dsi_cmds_tx(&z5mini_tx_buf,sharp_1080p_display_norm,
			ARRAY_SIZE( sharp_1080p_display_norm));
		break;
	case INTENSITY_01:
		mipi_dsi_cmds_tx(&z5mini_tx_buf,sharp_1080p_display_92,
			ARRAY_SIZE( sharp_1080p_display_92));
		break;
	case INTENSITY_02:
		mipi_dsi_cmds_tx(&z5mini_tx_buf,sharp_1080p_display_9b,
			ARRAY_SIZE( sharp_1080p_display_9b));
		break;
	default:
		//pr_err("sss default\n");
		mipi_dsi_cmds_tx(&z5mini_tx_buf,sharp_1080p_display_92,
			ARRAY_SIZE( sharp_1080p_display_92));
		break;

	}
	
}

 uint32 r63415_read_manufacture_id(struct msm_fb_data_type *mfd)
{

	return 0x20;

}
 void mipi_r63415_set_backlight(struct msm_fb_data_type *mfd)
{
	struct dcs_cmd_req cmdreq;
	static bool once = true;

	printk("=========sss %s ==========\n", __func__);

	if (mfd->bl_level < 0)
		mfd->bl_level = 0;
	if (mfd->bl_level > MAX_BL_LEVEL_R63415)
		mfd->bl_level = MAX_BL_LEVEL_R63415;
		
	if (0 == mfd->bl_level) {
		r63415_51[1] = 0x00;
		r63415_51[2] = 0x00;
	} else {
		r63415_51[1] = r63415_led_brightness[mfd->bl_level].brightness1;
		r63415_51[2] = r63415_led_brightness[mfd->bl_level].brightness2;
	}

	cmdreq.cmds = r63415_backlight_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(r63415_backlight_cmds);

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


int mipi_r63415_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	static bool is_firsttime = true;
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	printk("=========sss %s 11111==========\n", __func__);


	if (is_firsttime)
	{
		lcd_pmgpio_reset();
		is_firsttime=false;
	}

	//if (is_firsttime)
	//	return 0;
	if (!z5mini_lcd_state.disp_initialized) {

		mipi_dsi_cmds_tx(&z5mini_tx_buf,
					r63415_display_on_cmds,
					ARRAY_SIZE(r63415_display_on_cmds));
	if(0)
		zte_mipi_disp_inc(v5_intensity_value);		
	

		z5mini_lcd_state.display_on = TRUE;
		z5mini_lcd_state.disp_initialized = TRUE;
	}
//	is_firsttime = false;
#if 0
	if (0)
	toshiba_pixel_read(mfd);
#endif	
	pr_err("%s [%s,%d]out\n", __func__, current->comm,current->pid);
	return 0;
}

int mipi_r63415_lcd_off(void)
{
	if (z5mini_lcd_state.display_on) {
		
		mipi_dsi_cmds_tx(&z5mini_tx_buf, r63415_display_off_cmds,
				ARRAY_SIZE( r63415_display_off_cmds));
		
		z5mini_lcd_state.display_on = FALSE;
		z5mini_lcd_state.disp_initialized = FALSE;
	}
	return 0;
}

void mipi_video_sharp_r63415_panel_info(struct msm_panel_info *pinfo)
{
//	if(!pinfo)
//		  return;
		printk("==========%s:line%d===========\n",__func__,__LINE__);

	/* Landscape */
	pinfo->xres = 720;
	pinfo->yres = 1280;
	pinfo->lcdc.xres_pad = 0;
	pinfo->lcdc.yres_pad = 0;

	pinfo->type = MIPI_VIDEO_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;

	pinfo->lcdc.h_back_porch = 80;     ////2
	pinfo->lcdc.h_front_porch = 160;   ///4
	pinfo->lcdc.h_pulse_width = 8;     //2//
	pinfo->lcdc.v_back_porch = 16;  //12/
	pinfo->lcdc.v_front_porch = 16; //4  ////4
	pinfo->lcdc.v_pulse_width = 8;  //2/

	pinfo->lcdc.border_clr = 0;	/* blk */
	pinfo->lcdc.underflow_clr = 0xff;	/* blue */
	pinfo->lcdc.hsync_skew = 0;
	pinfo->bl_max =MAX_BL_LEVEL_R63415;
	pinfo->bl_min = 1;
	pinfo->fb_num = 2;
	
	pinfo->mipi.mode = DSI_VIDEO_MODE;
	pinfo->mipi.pulse_mode_hsa_he = TRUE;
	pinfo->mipi.hfp_power_stop = TRUE;
	pinfo->mipi.hbp_power_stop = TRUE;
	pinfo->mipi.hsa_power_stop = FALSE;
	pinfo->mipi.eof_bllp_power_stop = TRUE;
	pinfo->mipi.bllp_power_stop = TRUE;
	pinfo->mipi.traffic_mode =DSI_NON_BURST_SYNCH_EVENT;
	pinfo->mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo->mipi.vc = 0;
	pinfo->mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo->mipi.data_lane0 = TRUE;
	pinfo->mipi.data_lane1 = TRUE;
	pinfo->mipi.data_lane2 = TRUE;
	pinfo->mipi.data_lane3 = TRUE;

	pinfo->mipi.t_clk_post = 0x04;
	pinfo->mipi.t_clk_pre = 0x1c;
	pinfo->mipi.stream = 0; /* dma_p */
	pinfo->mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;// 0;
	pinfo->mipi.dma_trigger = DSI_CMD_TRIGGER_SW;//0;//
	pinfo->mipi.frame_rate = 60;
	pinfo->mipi.dsi_phy_db = &r63415_dsi_video_mode_phy_db;
	pinfo->mipi.tx_eot_append = TRUE;
	pinfo->mipi.esc_byte_ratio = 4;
	printk("==========%s:line%d===========\n",__func__,__LINE__);

}


