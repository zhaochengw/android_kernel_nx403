
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

#ifndef MIPI_ZTE_SHARP_VIDEO_720P_NT35590_H
#define MIPI_ZTE_SHARP_VIDEO_720P_NT35590_H

#include <linux/pwm.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_zte_z5mini_lcd.h"


void zte_mipi_disp_inc(unsigned int state);
void mipi_nt35590_set_backlight(struct msm_fb_data_type *mfd);
int mipi_nt35590_lcd_on(struct platform_device *pdev);
int mipi_nt35590_lcd_off(void);
uint32 nt35590_read_manufacture_id(struct msm_fb_data_type *mfd);
void mipi_video_sharp_nt35590_panel_info(struct msm_panel_info *pinfo);

#endif  /* MIPI_SHARP_H */
