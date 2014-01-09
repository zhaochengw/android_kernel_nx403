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
#include "mipi_zte_z5mini_lcd.h"
#include <linux/slab.h>
#ifdef CONFIG_ZTEMT_LCD_Z5MINI2_SIZE_4P7
#include "mipi_zte_sharp_720p_r69431.h"
#include "mipi_zte_tianma_720p_otm1282a.h"
#include <linux/nubia_lcd_ic_type.h>
#endif

static struct mipi_dsi_panel_platform_data *mipi_z5mini_pdata;
#define MAX_BL_LEVEL 81
#define TM_GET_PID(id) (((id) & 0xff00)>>8)

static int lcd_gpio_reset;
static int lcd_mpp2;

unsigned int v5_intensity_value;

static int mipi_z5mini_lcd_on(struct platform_device *pdev);
static int mipi_z5mini_lcd_off(struct platform_device *pdev);
static void mipi_z5mini_set_backlight(struct msm_fb_data_type *mfd);

extern void mipi_dsi_host_init(struct mipi_panel_info *pinfo);
extern int mipi_dsi_on_modify(struct platform_device *pdev);
extern int mipi_nt35590_lcd_short_off(void);

struct platform_device *pdev_panel = NULL;

 struct dsi_buf z5mini_tx_buf;
 struct dsi_buf z5mini_rx_buf;

struct ilitek_state_type z5mini_lcd_state = 
{
    .disp_initialized = FALSE,
    .display_on = TRUE,
}; 


struct msm_fb_panel_data z5mini_panel_data = {
	.on		= mipi_z5mini_lcd_on,
	.off		= mipi_z5mini_lcd_off,
	.set_backlight  = mipi_z5mini_set_backlight,
};

#ifdef CONFIG_ZTEMT_LCD_Z5MINI2
/*shutdown fun point,mayu add ,2013.12.2*/
void (*zte_lcd_power_shutdown)(void);
//defined sys.c
extern void (*zte_lcd_poweroff)(void);
#endif

void lcd_pmgpio_reset (void)
{
	gpio_set_value_cansleep(lcd_gpio_reset, 1);
	msleep(10);
	gpio_set_value_cansleep(lcd_gpio_reset, 0);
	msleep(10);
	gpio_set_value_cansleep(lcd_gpio_reset, 1);
	msleep(20);
}

static void mipi_z5mini_set_backlight(struct msm_fb_data_type *mfd)
{

#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P7)
	mipi_nt35590_set_backlight(mfd);
#endif
#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P5)
	mipi_r63415_set_backlight(mfd);
#endif	
#if defined( CONFIG_ZTEMT_LCD_Z5MINI2_SIZE_4P7)
	mipi_r69431_set_backlight(mfd);
#endif	
}

static int mipi_z5mini_lcd_on(struct platform_device *pdev)
{
	int ret = 0;
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P7)
	ret=mipi_nt35590_lcd_on(pdev);
#endif
#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P5)
	ret=mipi_r63415_lcd_on(pdev);
#endif
#if defined( CONFIG_ZTEMT_LCD_Z5MINI2_SIZE_4P7)
	if (0 == mini2_get_lcd_type()) {
		ret=mipi_r69431_lcd_on(pdev);
	} else {
		ret=mipi_tianma_lcd_on(pdev);
	}
#endif
	return ret;
}

static int mipi_z5mini_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);
	pr_err("%s\n", __func__);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;
#if defined( CONFIG_ZTEMT_LCD_Z5MINI2_SIZE_4P7)
	if (0 == mini2_get_lcd_type()) {
		mipi_r69431_lcd_short_off();
	} else {
		mipi_tianma_lcd_short_off();
	}
#else
	mipi_nt35590_lcd_short_off();
#endif
	return 0;
}

int zte_sharp_lcd_off(void)
{
	int ret;

#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P7)
	ret=mipi_nt35590_lcd_off( );
#endif
#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P5)
	ret=mipi_r63415_lcd_off( );
#endif
#if defined( CONFIG_ZTEMT_LCD_Z5MINI2_SIZE_4P7)
	if (0 == mini2_get_lcd_type()) {
		ret=mipi_r69431_lcd_off( );
	} else {
		ret=mipi_tianma_lcd_off( );
	}
#endif
	return ret;
}

#ifdef CONFIG_ZTEMT_LCD_Z5MINI2
/*lcd shutdown when restart and poweroff,mayu add ,2013.12.2*/
void zte_lcd_shutdown(void)
{
#if defined( CONFIG_ZTEMT_LCD_Z5MINI2_SIZE_4P7)
	if (0 == mini2_get_lcd_type()) {
		mipi_r69431_lcd_off( );
	} else {
		mipi_tianma_lcd_off( );
	}
#endif

  if(zte_lcd_power_shutdown)
     zte_lcd_power_shutdown();
}
#endif


static int __devinit mipi_z5mini_lcd_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	mfd = platform_get_drvdata(pdev);	

	if (pdev->id == 0) {
		mipi_z5mini_pdata = pdev->dev.platform_data;
		lcd_gpio_reset = mipi_z5mini_pdata->gpio[0];
		lcd_mpp2 = mipi_z5mini_pdata->gpio[1];
#ifdef CONFIG_ZTEMT_LCD_Z5MINI2
    if(mipi_z5mini_pdata->shutdown)
       zte_lcd_power_shutdown = mipi_z5mini_pdata->shutdown;
//zte_lcd_poweroff defined in sys.c
    zte_lcd_poweroff = zte_lcd_shutdown;
#endif
		return 0;
	}

	msm_fb_add_device(pdev);
	
	return 0;
}


static struct platform_driver this_driver = {
	.probe  = mipi_z5mini_lcd_probe,
	.driver = {
		.name   = "mipi_zte_sharp_panel",
	},
};

static int __init mipi_z5mini_lcd_init(void)
{
       int ret;
	struct platform_device *pdev = NULL;
	u32 channel = MIPI_DSI_PRIM;
	u32 panel = MIPI_DSI_PANEL_720P_PT;


	mipi_dsi_buf_alloc(&z5mini_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&z5mini_rx_buf, DSI_BUF_SIZE);
	
	ret = platform_driver_register(&this_driver);
	if (ret) {
		pr_err("mipi_z5mini_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P7)
	mipi_video_sharp_nt35590_panel_info(&z5mini_panel_data.panel_info);
#endif
#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P5)
	mipi_video_sharp_r63415_panel_info(&z5mini_panel_data.panel_info);
#endif
#if defined( CONFIG_ZTEMT_LCD_Z5MINI2_SIZE_4P7)
	mipi_video_sharp_r69431_panel_info(&z5mini_panel_data.panel_info);
#endif
	pdev = platform_device_alloc("mipi_zte_sharp_panel", (panel << 8)|channel);
	pdev_panel=pdev;
	if (!pdev)
		return -ENOMEM;
	ret = platform_device_add_data(pdev, &z5mini_panel_data,
		sizeof(z5mini_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
	
}
module_init(mipi_z5mini_lcd_init);
 
