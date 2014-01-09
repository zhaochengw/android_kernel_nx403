/******************************************************************************

  Copyright (C), 2001-2012, ZTE. Co., Ltd.

 ******************************************************************************
  File Name     : mipi_zte_sharp_video_720p_r69431.c
  Version       : Initial Draft
  Author        : tangjun
  Created       : 2013/08/16
  Last Modified :
  Description   : This is lcd panel file
  Function List :
  History       :
  1.Date        :  2013/08/16
    Author      : tangjun 
    Modification: Created file

*****************************************************************************/


#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_zte_z5mini_lcd.h"
#include "mipi_toshiba.h"
#include <linux/gpio.h>
#include "mipi_zte_sharp_720p_r69431.h"

//#define TEST_R69431

#define MAX_BL_LEVEL_R69431    255  

extern void v5_mhl_enable_irq(void);

extern unsigned int v5_intensity_value;
//static struct msm_panel_info pinfo;
#if 0
static uint16 r69431_led_brightness[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,
	                                                             0x40,0x44,0x48,0x4c,0x50,0x54,0x58,0x5c,0x60,0x64,0x68,0x6c,0x70,0x74,0x78,0x7c,
									     0x80,0x84,0x88,0x8c,0x90,0x94,0x98,0x9c,0xa0,0xa4,0xa8,0xac,0xb0,0xb4,0xb8,0xbc,
									     0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0xd8,0xdc,0xe0,0xe4,0xe8,0xec,0xf0,0xf4,0xf8,0xfc,};
#endif
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
/* regulator */ 
{0x03, 0x0a, 0x04, 0x00, 0x20},
/* timing */ 
{0x8b, 0x47, 0x14, 0x00, 0x56, 0x56, 0x19, 0x4b, 
0x1f, 0x03, 0x04, 0xa0}, 
{0x5f, 0x00, 0x00, 0x10}, /* phy ctrl */ 
{0xff, 0x00, 0x06, 0x00}, /* strength */ 
/* pll control */ 
{0x0, 0xb8, 0x31, 0xda, 0x00, 0x20, 0x07, 0x62, 
0x41, 0x0f, 0x01, 
0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},
};

//static char r69431_2608[] = {0x26, 0x08}; 
//static char r69431_2600[] = {0x26, 0x00}; 

//static char r69431_C2[] = {0xC2, 0x03}; ///0x03 video mode,GRAM acceses disable
									  ///0x0B video mode,GRAM acceses enable
//static char r69431_3B[] = {0x3B, 0x40,0x03, 0x07,  0x02, 0x02 };//

//static char r69431_FFEE[] = {0xFF, 0xEE};//
//static char r69431_12[] = {0x12, 0x50}; //
//static char r69431_13[] = {0x13, 0x02}; //
//static char r69431_6A[] = {0x6A, 0x60}; //
//static char r69431_16[] = {0x16, 0x08}; //
//static char r69431_FF05[] = {0xFF, 0x05};//
//static char r69431_72[] = {0x72, 0x21}; //
//static char r69431_73[] = {0x73, 0x00}; //
//static char r69431_74[] = {0x74, 0x22}; //
//static char r69431_75[] = {0x75, 0x01}; //
//static char r69431_76[] = {0x76, 0x1c}; //
//static char r69431_BA[] = {0xBA, 0x02};// 3 LANE  delay 100+ ms
static char r69431_FF00[] = {0xFF, 0x00};//
//static char r69431_FB[] = {0xFB, 0x01};//
static char r69431_11[] = {0x11, 0x00};
static char r69431_51[] = {0x51, 0x00};
//static char r69431_53[] = {0x53, 0x04};
//static char r69431_53[] = {0x53, 0x0c};
//static char r69431_5e[] = {0x5e, 0x06};
//static char r69431_55[] = {0x55, 0x01};
//static char r69431_5500[] = {0x55, 0x00};
static char r69431_29[] = {0x29, 0x00};

//static char r69431_FF04[] = {0xFF, 0x04};//
//static char r69431_0A[] = {0x0a, 0x03};//

/*static struct dsi_cmd_desc r69431_temperature_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(r69431_FFEE), r69431_FFEE},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 2, sizeof(r69431_2608), r69431_2608},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(r69431_2600), r69431_2600},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 12, sizeof(r69431_FF00), r69431_FF00}
};*/

static char r69431_B004[] = {0xB0, 0x04};
//static char r69431_B003[] = {0xB0, 0x03};
static char r69431_D6[] = {0xD6, 0x01};
static char r69431_B3[] = {0xB3, 0x1c};
static char r69431_5304[] = {0x53, 0x04};
static struct dsi_cmd_desc r69431_display_on_cmds_part[] = {
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r69431_B004), r69431_B004},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r69431_D6), r69431_D6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(r69431_11), r69431_11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r69431_B3), r69431_B3},
	/*{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(r69431_B003), r69431_B003},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(r69431_11), r69431_11},*/
};

static struct dsi_cmd_desc r69431_display_on_cmds_part2[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 22, sizeof(r69431_29), r69431_29},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_51), r69431_51},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_5304), r69431_5304},
};


#ifdef TEST_R69431
static char r69431_B300[] = {0xb3, 0x00};
static char r69431_0a00[] = {0x0a, 0x00};
static struct dsi_cmd_desc r69431_display_on_cmds_part5[] = {
	{DTYPE_GEN_READ2, 1, 0, 0, 0, sizeof(r69431_B300), r69431_B300},
};
static struct dsi_cmd_desc r69431_display_on_cmds_parta[] = {
	{DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(r69431_0a00), r69431_0a00},
};

static char r69431_5600[] = {0x56, 0x00};
static struct dsi_cmd_desc r69431_display_on_cmds_cabc[] = {
	{DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(r69431_5600), r69431_5600},
};
#endif

static struct dsi_cmd_desc r69431_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_FF00), r69431_FF00},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_51), r69431_51},
 		
};

static char r69431_5300[] = {0x53, 0x00};
static char r69431_28[] = {0x28, 0x00};
static char r69431_10[] = {0x10, 0x00};
//static char r69431_B1[2] = {0xB1, 0x01};
static struct dsi_cmd_desc r69431_display_off_short_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_5300), r69431_5300},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 22, sizeof(r69431_28), r69431_28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 122, sizeof(r69431_10), r69431_10},
};

static struct dsi_cmd_desc r69431_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(r69431_5300), r69431_5300},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 22, sizeof(r69431_28), r69431_28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 122, sizeof(r69431_10), r69431_10},
};

static char r69431_manufacture_id[] = {0x04,0x00};

static struct dsi_cmd_desc r69431_manufacture_id_cmd[] = {
    {DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(r69431_manufacture_id), r69431_manufacture_id}
};
/*
Reg55
H:B0   reg1A
M:90   reg19
L:80    reg18
*/
//static char r69431_FF03[] = {0xFF, 0x03};//
//static char r69431_FE08[] = {0xFE, 0x08};//
/*static char r69431_18[] = {0x18, 0x00}; //
static char r69431_19[] = {0x19, 0x04}; //
static char r69431_1A[] = {0x1A, 0x0A}; //08
static char r69431_25[] = {0x25, 0x26}; //
//static char r69431_FE01[] = {0xFE, 0x01};//
static char r69431_01_cmd2[] = {0x01, 0x02}; //
static char r69431_02_cmd2[] = {0x02, 0x04}; //
static char r69431_03_cmd2[] = {0x03, 0x06}; //
static char r69431_04_cmd2[] = {0x04, 0x08}; //
static char r69431_05_cmd2[] = {0x05, 0x0a}; //
static char r69431_06_cmd2[] = {0x06, 0x0c}; //
static char r69431_07_cmd2[] = {0x07, 0x0e}; //
static char r69431_08_cmd2[] = {0x08, 0x10}; //
static char r69431_09_cmd2[] = {0x09, 0x12}; //
static char r69431_10_cmd2[] = {0x0a, 0x14}; //
static char r69431_11_cmd2[] = {0x0b, 0x16}; //
static char r69431_12_cmd2[] = {0x0c, 0x18}; //
static char r69431_13_cmd2[] = {0x0d, 0x1a}; //
static char r69431_14_cmd2[] = {0x0e, 0x1c}; //
static char r69431_55B0[] = {0x55, 0xB1}; //B0// 90//80
static char r69431_5580[] = {0x55, 0x81}; //B0// 90//80*/

//static char r69431_5590[] = {0x55, 0x90}; //B0// 90//80
//static char r69431_5580[] = {0x55, 0xB0}; //B0// 90//80

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

	pr_err("%s [%s,%d]level=%d\n", __func__, current->comm,current->pid,mfd->bl_level);
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

#ifdef TEST_R69431
uint32 r69431_read_manufacture_register(struct msm_fb_data_type *mfd, struct dsi_cmd_desc *cmd)
{
    struct dsi_buf *rp, *tp;
    //struct dsi_cmd_desc *cmd;
    uint32 *lp;
	
	tp = &z5mini_tx_buf;
	rp = &z5mini_rx_buf;
	
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);	
	mipi_dsi_cmd_bta_sw_trigger();
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 2);
	
	lp = (uint32 *)rp->data;
	pr_info("%s: r69431_read_manufacture_register=0x%x\n", __func__, (*lp)&0xffff);	
	return 0;
}
#endif

int  mipi_r69431_lcd_on(struct platform_device *pdev)
{
	struct dcs_cmd_req cmdreq;
	struct msm_fb_data_type *mfd;
	static bool is_firsttime = true;

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
#ifdef TEST_R69431
		r69431_read_manufacture_register(mfd, r69431_display_on_cmds_part5);
		r69431_read_manufacture_register(mfd, r69431_display_on_cmds_parta);
#endif
		cmdreq.cmds = r69431_display_on_cmds_part;
		cmdreq.cmds_cnt = ARRAY_SIZE(r69431_display_on_cmds_part);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
		
		mipi_set_tx_power_mode(0);
		cmdreq.cmds = r69431_display_on_cmds_part2;
		cmdreq.cmds_cnt = ARRAY_SIZE(r69431_display_on_cmds_part2);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
#ifdef TEST_R69431
		r69431_read_manufacture_register(mfd, r69431_display_on_cmds_part5);
		r69431_read_manufacture_register(mfd, r69431_display_on_cmds_parta);
#endif

		zte_mipi_disp_inc_r69431(v5_intensity_value);

		z5mini_lcd_state.display_on = TRUE;
		z5mini_lcd_state.disp_initialized = TRUE;
	}
	pr_err("%s [%s,%d]out\n", __func__, current->comm,current->pid);

	return 0;
}

int mipi_r69431_lcd_off(void)
{	
	struct dcs_cmd_req cmdreq;

	if (z5mini_lcd_state.display_on) {
printk("yls==========%s:line%d===========.\n",__func__,__LINE__);
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

int mipi_r69431_lcd_short_off(void)
{	
	struct dcs_cmd_req cmdreq;

	if (z5mini_lcd_state.display_on) {
printk("yls==========%s:line%d===========.\n",__func__,__LINE__);
		cmdreq.cmds = r69431_display_off_short_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(r69431_display_off_short_cmds);
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
//	if(!pinfo)
//		  return;

	/* Landscape */
	pinfo->xres = 720;
	pinfo->yres = 1280;
	pinfo->lcdc.xres_pad = 0;
	pinfo->lcdc.yres_pad = 0;

	pinfo->type = MIPI_VIDEO_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;

	pinfo->lcdc.h_back_porch = 80;
	pinfo->lcdc.h_front_porch = 120;
	pinfo->lcdc.h_pulse_width = 20;
	pinfo->lcdc.v_back_porch = 8;
	pinfo->lcdc.v_front_porch = 13;
	pinfo->lcdc.v_pulse_width = 4;

	pinfo->lcdc.border_clr = 0;	/* blk */
	pinfo->lcdc.underflow_clr = 0xff;	/* blue */
	pinfo->lcdc.hsync_skew = 0;
	pinfo->bl_max =MAX_BL_LEVEL_R69431;
	pinfo->bl_min = 1;
	pinfo->fb_num = 2;
	
	pinfo->mipi.mode = DSI_VIDEO_MODE;
	pinfo->mipi.pulse_mode_hsa_he = TRUE;
	pinfo->mipi.hfp_power_stop = TRUE;
	pinfo->mipi.hbp_power_stop = TRUE;
	pinfo->mipi.hsa_power_stop = FALSE;
	pinfo->mipi.eof_bllp_power_stop = TRUE;
	pinfo->mipi.bllp_power_stop = TRUE;
	pinfo->mipi.traffic_mode = DSI_BURST_MODE;
	pinfo->mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo->mipi.vc = 0;
	pinfo->mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo->mipi.data_lane0 = TRUE;
	pinfo->mipi.data_lane1 = TRUE;
	pinfo->mipi.data_lane2 = TRUE;
	pinfo->mipi.data_lane3 = TRUE;

	pinfo->mipi.t_clk_post = 0x08;
	pinfo->mipi.t_clk_pre = 0x21;
	pinfo->mipi.stream = 0; /* dma_p */
	pinfo->mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;// 0;
	pinfo->mipi.dma_trigger = DSI_CMD_TRIGGER_SW;//0;//
	pinfo->mipi.frame_rate = 60;
	pinfo->mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo->mipi.tx_eot_append = TRUE;
	pinfo->mipi.esc_byte_ratio = 4;
}