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
#include "mipi_toshiba.h"
#include <linux/gpio.h>
#include "mipi_zte_sharp_video_720p_nt35590.h"

#define MAX_BL_LEVEL_NT35590    255    

extern void v5_mhl_enable_irq(void);

extern unsigned int v5_intensity_value;
//static struct msm_panel_info pinfo;
#if 0
static uint16 nt35590_led_brightness[]={0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,
	                                                             0x40,0x44,0x48,0x4c,0x50,0x54,0x58,0x5c,0x60,0x64,0x68,0x6c,0x70,0x74,0x78,0x7c,
									     0x80,0x84,0x88,0x8c,0x90,0x94,0x98,0x9c,0xa0,0xa4,0xa8,0xac,0xb0,0xb4,0xb8,0xbc,
									     0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0xd8,0xdc,0xe0,0xe4,0xe8,0xec,0xf0,0xf4,0xf8,0xfc,};
#endif
static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = { 

/* DSI_BIT_CLK at 549MHz, 3 lane, RGB888 */ 
{0x09, 0x08, 0x05, 0x00, 0x20}, /* regulator */ 
/* timing */ 
{0xab, 0x4f, 0x1c, 0x00, 0x63, 0x6a, 0x21, 0x53, 
0x2b, 0x03, 0x04, 0xa0}, 
{0x5f, 0x00, 0x00, 0x10}, /* phy ctrl */ 
{0xff, 0x00, 0x06, 0x00}, /* strength */ 
/* pll control */ 
{0x0, 0x21, 0x32, 0xda, 0x00, 0x20, 0x07, 0x62, 
0x41, 0x0f, 0x01, 
0x00, 0x14, 0x03, 0x00, 0x02, 0x00, 0x20, 0x00, 0x01},

};

static char nt35590_2608[] = {0x26, 0x08}; 
static char nt35590_2600[] = {0x26, 0x00}; 

static char nt35590_C2[] = {0xC2, 0x03}; ///0x03 video mode,GRAM acceses disable
									  ///0x0B video mode,GRAM acceses enable
static char nt35590_3B[] = {0x3B, 0x40,0x03, 0x07,  0x02, 0x02 };//

static char nt35590_FFEE[] = {0xFF, 0xEE};//
static char nt35590_12[] = {0x12, 0x50}; //
static char nt35590_13[] = {0x13, 0x02}; //
static char nt35590_6A[] = {0x6A, 0x60}; //
static char nt35590_16[] = {0x16, 0x08}; //
static char nt35590_FF05[] = {0xFF, 0x05};//
static char nt35590_72[] = {0x72, 0x21}; //
static char nt35590_73[] = {0x73, 0x00}; //
static char nt35590_74[] = {0x74, 0x22}; //
static char nt35590_75[] = {0x75, 0x01}; //
static char nt35590_76[] = {0x76, 0x1c}; //
static char nt35590_BA[] = {0xBA, 0x02};// 3 LANE  delay 100+ ms
static char nt35590_FF00[] = {0xFF, 0x00};//
static char nt35590_FB[] = {0xFB, 0x01};//
static char nt35590_11[] = {0x11, 0x00};
static char nt35590_51[] = {0x51, 0xff};
//static char nt35590_53[] = {0x53, 0x04};
static char nt35590_53[] = {0x53, 0x24};
static char nt35590_5e[] = {0x5e, 0x06};
static char nt35590_55[] = {0x55, 0x01};
static char nt35590_29[] = {0x29, 0x00};

static char nt35590_FF04[] = {0xFF, 0x04};//
static char nt35590_0A[] = {0x0a, 0x03};//

static struct dsi_cmd_desc nt35590_temperature_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_FFEE), nt35590_FFEE},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 2, sizeof(nt35590_2608), nt35590_2608},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_2600), nt35590_2600},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 12, sizeof(nt35590_FF00), nt35590_FF00}
};
static struct dsi_cmd_desc nt35590_display_on_cmds[] = {
	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_C2), nt35590_C2},		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_3B), nt35590_3B},	
//	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_51), nt35590_51},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_5e), nt35590_5e}, 	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_53), nt35590_53}, 
//	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_55), nt35590_55}, 
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(nt35590_11), nt35590_11},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 120, sizeof(nt35590_BA), nt35590_BA},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_FFEE), nt35590_FFEE},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_12), nt35590_12},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_13), nt35590_13},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_6A), nt35590_6A},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_FF00), nt35590_FF00},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_FFEE), nt35590_FFEE},		
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_FB), nt35590_FB},		
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_16), nt35590_16},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_FF00), nt35590_FF00},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_5e), nt35590_5e}, 			
//	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_55), nt35590_55}, 	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_53), nt35590_53}, 	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FF04), nt35590_FF04}, 		
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_0A), nt35590_0A}, 	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FF00), nt35590_FF00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_FF05), nt35590_FF05},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_FB), nt35590_FB},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_72), nt35590_72},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_73), nt35590_73},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_74), nt35590_74},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_75), nt35590_75},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_76), nt35590_76},	
	{DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(nt35590_FF00), nt35590_FF00},
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(nt35590_29), nt35590_29},
};


static struct dsi_cmd_desc nt35590_backlight_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FF00), nt35590_FF00},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_51), nt35590_51},
 		
};

static char nt35590_28[2] = {0x28, 0x00};
static char nt35590_10[2] = {0x10, 0x00};
static struct dsi_cmd_desc nt35590_display_off_short_cmds[] = {
	{DTYPE_DCS_WRITE1, 1, 0, 0, 12, sizeof(nt35590_FF00), nt35590_FF00},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 50, sizeof(nt35590_28), nt35590_28},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 120, sizeof(nt35590_10), nt35590_10}
};

static struct dsi_cmd_desc nt35590_display_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 12, sizeof(nt35590_FF00), nt35590_FF00},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 50, sizeof(nt35590_28), nt35590_28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 120, sizeof(nt35590_10), nt35590_10}
};

static char nt35590_manufacture_id[] = {0x04,0x00};

static struct dsi_cmd_desc nt35590_manufacture_id_cmd[] = {
    {DTYPE_DCS_READ, 1, 0, 0, 0, sizeof(nt35590_manufacture_id), nt35590_manufacture_id}
};
/*
Reg55
H:B0   reg1A
M:90   reg19
L:80    reg18
*/
static char nt35590_FF03[] = {0xFF, 0x03};//
//static char nt35590_FE08[] = {0xFE, 0x08};//
static char nt35590_18[] = {0x18, 0x00}; //
static char nt35590_19[] = {0x19, 0x04}; //
static char nt35590_1A[] = {0x1A, 0x0A}; //08
static char nt35590_25[] = {0x25, 0x26}; //
//static char nt35590_FE01[] = {0xFE, 0x01};//
static char nt35590_01_cmd2[] = {0x01, 0x02}; //
static char nt35590_02_cmd2[] = {0x02, 0x04}; //
static char nt35590_03_cmd2[] = {0x03, 0x06}; //
static char nt35590_04_cmd2[] = {0x04, 0x08}; //
static char nt35590_05_cmd2[] = {0x05, 0x0a}; //
static char nt35590_06_cmd2[] = {0x06, 0x0c}; //
static char nt35590_07_cmd2[] = {0x07, 0x0e}; //
static char nt35590_08_cmd2[] = {0x08, 0x10}; //
static char nt35590_09_cmd2[] = {0x09, 0x12}; //
static char nt35590_10_cmd2[] = {0x0a, 0x14}; //
static char nt35590_11_cmd2[] = {0x0b, 0x16}; //
static char nt35590_12_cmd2[] = {0x0c, 0x18}; //
static char nt35590_13_cmd2[] = {0x0d, 0x1a}; //
static char nt35590_14_cmd2[] = {0x0e, 0x1c}; //
static char nt35590_55B0[] = {0x55, 0xB1}; //B0// 90//80
static char nt35590_5580[] = {0x55, 0x81}; //B0// 90//80

//static char nt35590_5590[] = {0x55, 0x90}; //B0// 90//80
//static char nt35590_5580[] = {0x55, 0xB0}; //B0// 90//80

static struct dsi_cmd_desc sharp_720p_ec_cmd[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FF03), nt35590_FF03},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FE08), nt35590_FE08},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_18), nt35590_18},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_19), nt35590_19}, 	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_1A), nt35590_1A}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_25), nt35590_25}, 	
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FB), nt35590_FB},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FF00), nt35590_FF00},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FE01), nt35590_FE01},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_55B0), nt35590_55B0}, 	

};
static struct dsi_cmd_desc sharp_720p_ec_normal_cmd[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FF00), nt35590_FF00},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_55), nt35590_55}, 	

};
static struct dsi_cmd_desc sharp_720p_ec_normal_cmd_01[] = {
	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FF03), nt35590_FF03},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FE08), nt35590_FE08},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_01_cmd2), nt35590_01_cmd2}, 
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_02_cmd2), nt35590_02_cmd2},   
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_03_cmd2), nt35590_03_cmd2},  
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_04_cmd2), nt35590_04_cmd2},   
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_05_cmd2), nt35590_05_cmd2},   
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_06_cmd2), nt35590_06_cmd2},   
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_07_cmd2), nt35590_07_cmd2},
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_08_cmd2), nt35590_08_cmd2}, 
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_09_cmd2), nt35590_09_cmd2}, 
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_10_cmd2), nt35590_10_cmd2},   
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_11_cmd2), nt35590_11_cmd2},  
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_12_cmd2), nt35590_12_cmd2},   
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_13_cmd2), nt35590_13_cmd2},   
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_14_cmd2), nt35590_14_cmd2},   
    {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_25), nt35590_25},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_18), nt35590_18},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_19), nt35590_19}, 	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_1A), nt35590_1A}, 
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_25), nt35590_25}, 	
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FB), nt35590_FB},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FF00), nt35590_FF00},
	//{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_FE01), nt35590_FE01},	
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(nt35590_5580), nt35590_5580},	
};

void zte_mipi_disp_inc_nt35590(unsigned int state)
{
	struct dcs_cmd_req cmdreq;
	unsigned int value;
	value =state;

	pr_err("sss %s value=%d\n", __func__, value);
	switch (value) {
	case INTENSITY_NORMAL:
		cmdreq.cmds = sharp_720p_ec_normal_cmd;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_720p_ec_normal_cmd);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	
		break;
	case INTENSITY_01:

		cmdreq.cmds = sharp_720p_ec_normal_cmd_01;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_720p_ec_normal_cmd_01);
		
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		
		mipi_dsi_cmdlist_put(&cmdreq);	

		break;
	case INTENSITY_02:

		cmdreq.cmds = sharp_720p_ec_cmd;
		cmdreq.cmds_cnt = ARRAY_SIZE(sharp_720p_ec_cmd);
		
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

 uint32 nt35590_read_manufacture_id(struct msm_fb_data_type *mfd)
{
    struct dsi_buf *rp, *tp;
    struct dsi_cmd_desc *cmd;
    uint32 *lp;
    uint32 id=0xff;

	tp = &z5mini_tx_buf;
	rp = &z5mini_rx_buf;
	cmd = nt35590_manufacture_id_cmd;
	
	mipi_dsi_buf_init(rp);
	mipi_dsi_buf_init(tp);	
	mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */	
	mipi_dsi_cmds_rx(mfd, tp, rp, cmd, 2);
	
	lp = (uint32 *)rp->data;
	id=(((*lp)&0xff00)>>8);
	pr_info("%s: manufacture_id=0x%x,id=0x%x\n", __func__, (*lp)&0xffff ,id);	
	return id;
}
 void mipi_nt35590_set_backlight(struct msm_fb_data_type *mfd)
{
	struct dcs_cmd_req cmdreq;
	static bool once = true;
	
	if (mfd->bl_level < 0)
		mfd->bl_level = 0;
	if (mfd->bl_level > MAX_BL_LEVEL_NT35590)
		mfd->bl_level = MAX_BL_LEVEL_NT35590;
		
	if (0 == mfd->bl_level) {
		nt35590_51[1] = 0x00;
	} else {
		if (mfd->bl_level <= 10) {
			nt35590_51[1] = 1;
		} else if (mfd->bl_level < 21){
			nt35590_51[1] = 3;
		} else {
			nt35590_51[1] = mfd->bl_level - 9;
		}
	}

	cmdreq.cmds = nt35590_backlight_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(nt35590_backlight_cmds);
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

int  mipi_nt35590_lcd_on(struct platform_device *pdev)
{
	struct dcs_cmd_req cmdreq;
	struct msm_fb_data_type *mfd;
	static bool is_firsttime = true;
	printk("==========%s:line%d===========\n",__func__,__LINE__);

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	if (is_firsttime) {
		zte_mipi_disp_inc_nt35590(v5_intensity_value);
		is_firsttime = false;
		return 0;
	}
	//if (!z5mini_lcd_state.disp_initialized) {
		cmdreq.cmds = nt35590_temperature_display_on_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(nt35590_temperature_display_on_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;
		mipi_dsi_cmdlist_put(&cmdreq);
		lcd_pmgpio_reset();
		cmdreq.cmds = nt35590_display_on_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(nt35590_display_on_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);	
	/*
		mipi_dsi_cmds_tx(&z5mini_tx_buf,
					nt35590_display_on_cmds,
					ARRAY_SIZE(nt35590_display_on_cmds));
	*/
		zte_mipi_disp_inc_nt35590(v5_intensity_value);	

		//z5mini_lcd_state.display_on = TRUE;
		//z5mini_lcd_state.disp_initialized = TRUE;
	//}
	pr_err("%s [%s,%d]out\n", __func__, current->comm,current->pid);

	return 0;
}

int mipi_nt35590_lcd_off(void)
{	
	struct dcs_cmd_req cmdreq;
   // printk("==========%s:line%d===========\n",__func__,__LINE__);

	//if (z5mini_lcd_state.display_on) {
		printk("==========%s:line%d===========\n",__func__,__LINE__);
		cmdreq.cmds = nt35590_display_off_cmds;
		cmdreq.cmds_cnt = ARRAY_SIZE(nt35590_display_off_cmds);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);
		
	//	z5mini_lcd_state.display_on = FALSE;
	//	z5mini_lcd_state.disp_initialized = FALSE;
	//}
	return 0;
}

int mipi_nt35590_lcd_short_off(void)
{	
	struct dcs_cmd_req cmdreq;
	cmdreq.cmds = nt35590_display_off_short_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(nt35590_display_off_short_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
	return 0;
}

void mipi_video_sharp_nt35590_panel_info(struct msm_panel_info *pinfo)
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

	pinfo->lcdc.h_back_porch =96;
	pinfo->lcdc.h_front_porch = 52;//32//12;// 52; //4;
	pinfo->lcdc.h_pulse_width =  8;//8; //4;
	pinfo->lcdc.v_back_porch = 4; //16;// 8; //1; //14;//4; //12;
	pinfo->lcdc.v_front_porch =  14;//4; //4; //14; //4;
	pinfo->lcdc.v_pulse_width = 2; //2; //1; //2;

	pinfo->lcdc.border_clr = 0;	/* blk */
	pinfo->lcdc.underflow_clr = 0xff;	/* blue */
	pinfo->lcdc.hsync_skew = 0;
	pinfo->bl_max =MAX_BL_LEVEL_NT35590;
	pinfo->bl_min = 1;
	pinfo->fb_num = 2;
	
	pinfo->mipi.mode = DSI_VIDEO_MODE;
	pinfo->mipi.pulse_mode_hsa_he = 0;//TRUE;//
	pinfo->mipi.hfp_power_stop =0;// TRUE;
	pinfo->mipi.hbp_power_stop =0;// TRUE;
	pinfo->mipi.hsa_power_stop = 0;//TRUE;//FALSE;
	pinfo->mipi.eof_bllp_power_stop = TRUE;
	pinfo->mipi.bllp_power_stop = TRUE;
	pinfo->mipi.traffic_mode =  DSI_BURST_MODE;// DSI_NON_BURST_SYNCH_EVENT;
	pinfo->mipi.dst_format = DSI_VIDEO_DST_FORMAT_RGB888;
	pinfo->mipi.vc = 0;
	pinfo->mipi.rgb_swap = DSI_RGB_SWAP_RGB;
	pinfo->mipi.data_lane0 = TRUE;
	pinfo->mipi.data_lane1 = TRUE;
	pinfo->mipi.data_lane2 = TRUE;
	pinfo->mipi.data_lane3 = FALSE;	

	pinfo->mipi.t_clk_post = 0x08;
	pinfo->mipi.t_clk_pre = 0x26;////0x1c;
	pinfo->mipi.stream = 0; /* dma_p */
	pinfo->mipi.mdp_trigger = 0;//DSI_CMD_TRIGGER_SW;// 0;
	pinfo->mipi.dma_trigger = DSI_CMD_TRIGGER_SW;//0;//
	pinfo->mipi.frame_rate = 66;
	pinfo->mipi.dsi_phy_db = &dsi_video_mode_phy_db;
	pinfo->mipi.tx_eot_append = TRUE;
	pinfo->mipi.esc_byte_ratio = 4;

}

