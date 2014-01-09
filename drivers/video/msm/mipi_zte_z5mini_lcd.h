
/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef MIPI_SHARP_H
#define MIPI_SHARP_H

#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>

#include "msm_fb.h"
#include "mipi_dsi.h"
#include <linux/gpio.h>
#include "mipi_zte_sharp_video_720p_r63415.h"
#include "mipi_zte_sharp_video_720p_nt35590.h"

extern struct dsi_buf z5mini_tx_buf;
extern struct dsi_buf z5mini_rx_buf;

struct lcdc_funcs_list_t {
	uint32  channel;
	uint32  id;
	uint32  (*read_id)(struct msm_fb_data_type *mfd);
	void (*config_panel_info)(struct msm_panel_info *pinfo);	
	int (*mipi_disp_on)(struct platform_device *pdev);
	int (*mipi_disp_off)(void);	
	void (*panel_set_backlight)(struct msm_fb_data_type *mfd);

};

struct ilitek_state_type{
	boolean disp_initialized;
	boolean display_on;
};

// int mipi_z5mini_lcd_init(void);
 
extern struct ilitek_state_type z5mini_lcd_state ;

enum {
	INTENSITY_NORMAL=24,
	INTENSITY_01,
	INTENSITY_02
};

int mipi_z5mini_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel);

void lcd_pmgpio_reset (void);
#endif  /* MIPI_SHARP_H */
