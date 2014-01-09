/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
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

#include "msm_sensor.h"
#define SENSOR_NAME "imx132"
#define PLATFORM_DRIVER_NAME "msm_camera_imx132"
#define imx132_obj imx132_##obj

 /* added by jing.hl
 *Index Tag: ZTEMT_CAMERA_20121126_64xGain
 *TIMX132 global gain table row size  
 */
#define IMX132_GAIN_ROWS	449
#define IMX132_GAIN_COLS	3
#define IMX132_GAIN_0DB	448

#define IMX132_GAIN_COL_AGC	    0
#define IMX132_DIGITAL_DGAIN	1
#define IMX132_ANALOG_DGAIN	2

DEFINE_MUTEX(imx132_mut);
static struct msm_sensor_ctrl_t imx132_s_ctrl;


/* added by jing.hl
 *Index Tag: ZTEMT_CAMERA_20121119_64xGain
 *This is 128-step gain table 
 */
 uint16_t IMX132_GLOBAL_GAIN_TABLE[IMX132_GAIN_ROWS][IMX132_GAIN_COLS] = {
	 /* gain_value*256,  digital gain,  analog gain */
	 {0x8000, 0x0FD9, 0xe0},	   /* index 000, 42db*/   
	 {0x7e9f, 0x0FD9, 0xe0},	   /* index 001,*/ 
	 {0x7d42, 0x0FD9, 0xdf},	   /* index 002,*/ 
	 {0x7be8, 0x0FD9, 0xdf},	   /* index 003,*/ 
	 {0x7a93, 0x0FD9, 0xdf},	   /* index 004,*/ 
	 {0x7941, 0x0FD9, 0xde},	   /* index 005,*/ 
	 {0x77f2, 0x0FD9, 0xde},	   /* index 006,*/ 
	 {0x76a8, 0x0FD9, 0xdd},	   /* index 007,*/ 
	 {0x7560, 0x0FD9, 0xdd},	   /* index 008,*/ 
	 {0x741d, 0x0FD9, 0xdd},	   /* index 009,*/ 
	 {0x72dd, 0x0FD9, 0xdc},	   /* index 010,*/ 
	 {0x71a0, 0x0FD9, 0xdc},	   /* index 011,*/ 
	 {0x7066, 0x0FD9, 0xdc},	   /* index 012,*/ 
	 {0x6f30, 0x0FD9, 0xdb},	   /* index 013,*/ 
	 {0x6dfe, 0x0FD9, 0xdb},	   /* index 014,*/ 
	 {0x6ccf, 0x0FD9, 0xda},	   /* index 015,*/ 
	 {0x6ba2, 0x0FD9, 0xda},	   /* index 016,*/ 
	 {0x6a7a, 0x0FD9, 0xda},	   /* index 017,*/ 
	 {0x6954, 0x0FD9, 0xd9},	   /* index 018,*/ 
	 {0x6832, 0x0FD9, 0xd9},	   /* index 019,*/ 
	 {0x6712, 0x0FD9, 0xd8},	   /* index 020,*/ 
	 {0x65f6, 0x0FD9, 0xd8},	   /* index 021,*/ 
	 {0x64dd, 0x0FD9, 0xd7},	   /* index 022,*/ 
	 {0x63c7, 0x0FD9, 0xd7},	   /* index 023,*/ 
	 {0x62b4, 0x0FD9, 0xd7},	   /* index 024,*/ 
	 {0x61a3, 0x0FD9, 0xd6},	   /* index 025,*/ 
	 {0x6096, 0x0FD9, 0xd6},	   /* index 026,*/ 
	 {0x5f8c, 0x0FD9, 0xd5},	   /* index 027,*/ 
	 {0x5e84, 0x0FD9, 0xd5},	   /* index 028,*/ 
	 {0x5d80, 0x0FD9, 0xd4},	   /* index 029,*/ 
	 {0x5c7e, 0x0FD9, 0xd4},	   /* index 030,*/ 
	 {0x5b7f, 0x0FD9, 0xd3},	   /* index 031,*/ 
	 {0x5a82, 0x0FD9, 0xd3},	   /* index 032,*/ 
	 {0x5989, 0x0FD9, 0xd2},	   /* index 033,*/ 
	 {0x5892, 0x0FD9, 0xd2},	   /* index 034,*/ 
	 {0x579e, 0x0FD9, 0xd1},	   /* index 035,*/ 
	 {0x56ac, 0x0FD9, 0xd1},	   /* index 036,*/ 
	 {0x55bd, 0x0FD9, 0xd0},	   /* index 037,*/ 
	 {0x54d1, 0x0FD9, 0xd0},	   /* index 038,*/ 
	 {0x53e7, 0x0FD9, 0xcf},	   /* index 039,*/ 
	 {0x52ff, 0x0FD9, 0xcf},	   /* index 040,*/ 
	 {0x521b, 0x0FD9, 0xce},	   /* index 041,*/ 
	 {0x5138, 0x0FD9, 0xce},	   /* index 042,*/ 
	 {0x5058, 0x0FD9, 0xcd},	   /* index 043,*/ 
	 {0x4f7b, 0x0FD9, 0xcc},	   /* index 044,*/ 
	 {0x4e9f, 0x0FD9, 0xcc},	   /* index 045,*/ 
	 {0x4dc7, 0x0FD9, 0xcb},	   /* index 046,*/ 
	 {0x4cf0, 0x0FD9, 0xcb},	   /* index 047,*/ 
	 {0x4c1c, 0x0FD9, 0xca},	   /* index 048,*/ 
	 {0x4b4a, 0x0FD9, 0xca},	   /* index 049,*/ 
	 {0x4a7a, 0x0FD9, 0xc9},	   /* index 050,*/ 
	 {0x49ad, 0x0FD9, 0xc8},	   /* index 051,*/ 
	 {0x48e2, 0x0FD9, 0xc8},	   /* index 052,*/ 
	 {0x4819, 0x0FD9, 0xc7},	   /* index 053,*/ 
	 {0x4752, 0x0FD9, 0xc7},	   /* index 054,*/ 
	 {0x468d, 0x0FD9, 0xc6},	   /* index 055,*/ 
	 {0x45cb, 0x0FD9, 0xc5},	   /* index 056,*/ 
	 {0x450a, 0x0FD9, 0xc5},	   /* index 057,*/ 
	 {0x444c, 0x0FD9, 0xc4},	   /* index 058,*/ 
	 {0x4390, 0x0FD9, 0xc3},	   /* index 059,*/ 
	 {0x42d5, 0x0FD9, 0xc3},	   /* index 060,*/ 
	 {0x421d, 0x0FD9, 0xc2},	   /* index 061,*/ 
	 {0x4167, 0x0FD9, 0xc1},	   /* index 062,*/ 
	 {0x40b2, 0x0FD9, 0xc1},	   /* index 063,*/ 
	 {0x4000, 0x07F1, 0xe0},	   /* index 064,*/ 
	 {0x3f50, 0x07F1, 0xe0},	   /* index 065,*/ 
	 {0x3ea1, 0x07F1, 0xdf},	   /* index 066,*/ 
	 {0x3df4, 0x07F1, 0xdf},	   /* index 067,*/ 
	 {0x3d49, 0x07F1, 0xdf},	   /* index 068,*/ 
	 {0x3ca0, 0x07F1, 0xde},	   /* index 069,*/ 
	 {0x3bf9, 0x07F1, 0xde},	   /* index 070,*/ 
	 {0x3b54, 0x07F1, 0xdd},	   /* index 071,*/ 
	 {0x3ab0, 0x07F1, 0xdd},	   /* index 072,*/ 
	 {0x3a0e, 0x07F1, 0xdd},	   /* index 073,*/ 
	 {0x396e, 0x07F1, 0xdc},	   /* index 074,*/ 
	 {0x38d0, 0x07F1, 0xdc},	   /* index 075,*/ 
	 {0x3833, 0x07F1, 0xdc},	   /* index 076,*/ 
	 {0x3798, 0x07F1, 0xdb},	   /* index 077,*/ 
	 {0x36ff, 0x07F1, 0xdb},	   /* index 078,*/ 
	 {0x3667, 0x07F1, 0xda},	   /* index 079,*/ 
	 {0x35d1, 0x07F1, 0xda},	   /* index 080,*/ 
	 {0x353d, 0x07F1, 0xda},	   /* index 081,*/ 
	 {0x34aa, 0x07F1, 0xd9},	   /* index 082,*/ 
	 {0x3419, 0x07F1, 0xd9},	   /* index 083,*/ 
	 {0x3389, 0x07F1, 0xd8},	   /* index 084,*/ 
	 {0x32fb, 0x07F1, 0xd8},	   /* index 085,*/ 
	 {0x326e, 0x07F1, 0xd7},	   /* index 086,*/ 
	 {0x31e3, 0x07F1, 0xd7},	   /* index 087,*/ 
	 {0x315a, 0x07F1, 0xd7},	   /* index 088,*/ 
	 {0x30d2, 0x07F1, 0xd6},	   /* index 089,*/ 
	 {0x304b, 0x07F1, 0xd6},	   /* index 090,*/ 
	 {0x2fc6, 0x07F1, 0xd5},	   /* index 091,*/ 
	 {0x2f42, 0x07F1, 0xd5},	   /* index 092,*/ 
	 {0x2ec0, 0x07F1, 0xd4},	   /* index 093,*/ 
	 {0x2e3f, 0x07F1, 0xd4},	   /* index 094,*/ 
	 {0x2dbf, 0x07F1, 0xd3},	   /* index 095,*/ 
	 {0x2d41, 0x07F1, 0xd3},	   /* index 096,*/ 
	 {0x2cc4, 0x07F1, 0xd2},	   /* index 097,*/ 
	 {0x2c49, 0x07F1, 0xd2},	   /* index 098,*/ 
	 {0x2bcf, 0x07F1, 0xd1},	   /* index 099,*/ 
	 {0x2b56, 0x07F1, 0xd1},	   /* index 100,*/ 
	 {0x2adf, 0x07F1, 0xd0},	   /* index 101,*/ 
	 {0x2a68, 0x07F1, 0xd0},	   /* index 102,*/ 
	 {0x29f3, 0x07F1, 0xcf},	   /* index 103,*/ 
	 {0x2980, 0x07F1, 0xcf},	   /* index 104,*/ 
	 {0x290d, 0x07F1, 0xce},	   /* index 105,*/ 
	 {0x289c, 0x07F1, 0xce},	   /* index 106,*/ 
	 {0x282c, 0x07F1, 0xcd},	   /* index 107,*/ 
	 {0x27bd, 0x07F1, 0xcc},	   /* index 108,*/ 
	 {0x2750, 0x07F1, 0xcc},	   /* index 109,*/ 
	 {0x26e3, 0x07F1, 0xcb},	   /* index 110,*/ 
	 {0x2678, 0x07F1, 0xcb},	   /* index 111,*/ 
	 {0x260e, 0x07F1, 0xca},	   /* index 112,*/ 
	 {0x25a5, 0x07F1, 0xca},	   /* index 113,*/ 
	 {0x253d, 0x07F1, 0xc9},	   /* index 114,*/ 
	 {0x24d7, 0x07F1, 0xc8},	   /* index 115,*/ 
	 {0x2471, 0x07F1, 0xc8},	   /* index 116,*/ 
	 {0x240c, 0x07F1, 0xc7},	   /* index 117,*/ 
	 {0x23a9, 0x07F1, 0xc7},	   /* index 118,*/ 
	 {0x2347, 0x07F1, 0xc6},	   /* index 119,*/ 
	 {0x22e5, 0x07F1, 0xc5},	   /* index 120,*/ 
	 {0x2285, 0x07F1, 0xc5},	   /* index 121,*/ 
	 {0x2226, 0x07F1, 0xc4},	   /* index 122,*/ 
	 {0x21c8, 0x07F1, 0xc3},	   /* index 123,*/ 
	 {0x216b, 0x07F1, 0xc3},	   /* index 124,*/ 
	 {0x210f, 0x07F1, 0xc2},	   /* index 125,*/ 
	 {0x20b3, 0x07F1, 0xc1},	   /* index 126,*/ 
	 {0x2059, 0x07F1, 0xc1},	   /* index 127,*/	 
	 {0x2000, 0x03FB, 0xe0},	   /* index 128, gain = 30.061800 dB */ 
	 {0x1fa8, 0x03FB, 0xe0},	   /* index 129, gain = 30.061800 dB */ 
	 {0x1f50, 0x03FB, 0xdf},	   /* index 130, gain = 29.794521 dB */ 
	 {0x1efa, 0x03FB, 0xdf},	   /* index 131, gain = 29.794521 dB */ 
	 {0x1ea5, 0x03FB, 0xdf},	   /* index 132, gain = 29.794521 dB */ 
	 {0x1e50, 0x03FB, 0xde},	   /* index 133, gain = 29.535221 dB */ 
	 {0x1dfd, 0x03FB, 0xde},	   /* index 134, gain = 29.535221 dB */ 
	 {0x1daa, 0x03FB, 0xdd},	   /* index 135, gain = 29.283438 dB */ 
	 {0x1d58, 0x03FB, 0xdd},	   /* index 136, gain = 29.283438 dB */ 
	 {0x1d07, 0x03FB, 0xdd},	   /* index 137, gain = 29.283438 dB */ 
	 {0x1cb7, 0x03FB, 0xdc},	   /* index 138, gain = 29.038749 dB */ 
	 {0x1c68, 0x03FB, 0xdc},	   /* index 139, gain = 29.038749 dB */ 
	 {0x1c1a, 0x03FB, 0xdc},	   /* index 140, gain = 29.038749 dB */ 
	 {0x1bcc, 0x03FB, 0xdb},	   /* index 141, gain = 28.800765 dB */ 
	 {0x1b7f, 0x03FB, 0xdb},	   /* index 142, gain = 28.800765 dB */ 
	 {0x1b34, 0x03FB, 0xda},	   /* index 143, gain = 28.569127 dB */ 
	 {0x1ae9, 0x03FB, 0xda},	   /* index 144, gain = 28.569127 dB */ 
	 {0x1a9e, 0x03FB, 0xda},	   /* index 145, gain = 28.569127 dB */ 
	 {0x1a55, 0x03FB, 0xd9},	   /* index 146, gain = 28.343507 dB */ 
	 {0x1a0c, 0x03FB, 0xd9},	   /* index 147, gain = 28.343507 dB */ 
	 {0x19c5, 0x03FB, 0xd8},	   /* index 148, gain = 28.123599 dB */ 
	 {0x197e, 0x03FB, 0xd8},	   /* index 149, gain = 28.123599 dB */ 
	 {0x1937, 0x03FB, 0xd7},	   /* index 150, gain = 27.909122 dB */ 
	 {0x18f2, 0x03FB, 0xd7},	   /* index 151, gain = 27.909122 dB */ 
	 {0x18ad, 0x03FB, 0xd7},	   /* index 152, gain = 27.909122 dB */ 
	 {0x1869, 0x03FB, 0xd6},	   /* index 153, gain = 27.699813 dB */ 
	 {0x1826, 0x03FB, 0xd6},	   /* index 154, gain = 27.699813 dB */ 
	 {0x17e3, 0x03FB, 0xd5},	   /* index 155, gain = 27.495430 dB */ 
	 {0x17a1, 0x03FB, 0xd5},	   /* index 156, gain = 27.495430 dB */ 
	 {0x1760, 0x03FB, 0xd4},	   /* index 157, gain = 27.295746 dB */ 
	 {0x171f, 0x03FB, 0xd4},	   /* index 158, gain = 27.295746 dB */ 
	 {0x16e0, 0x03FB, 0xd3},	   /* index 159, gain = 27.100549 dB */ 
	 {0x16a1, 0x03FB, 0xd3},	   /* index 160, gain = 27.100549 dB */ 
	 {0x1662, 0x03FB, 0xd2},	   /* index 161, gain = 26.909643 dB */ 
	 {0x1624, 0x03FB, 0xd2},	   /* index 162, gain = 26.909643 dB */ 
	 {0x15e7, 0x03FB, 0xd1},	   /* index 163, gain = 26.722842 dB */ 
	 {0x15ab, 0x03FB, 0xd1},	   /* index 164, gain = 26.722842 dB */ 
	 {0x156f, 0x03FB, 0xd0},	   /* index 165, gain = 26.539975 dB */ 
	 {0x1534, 0x03FB, 0xd0},	   /* index 166, gain = 26.539975 dB */ 
	 {0x14fa, 0x03FB, 0xcf},	   /* index 167, gain = 26.360878 dB */ 
	 {0x14c0, 0x03FB, 0xcf},	   /* index 168, gain = 26.360878 dB */ 
	 {0x1487, 0x03FB, 0xce},	   /* index 169, gain = 26.185399 dB */ 
	 {0x144e, 0x03FB, 0xce},	   /* index 170, gain = 26.185399 dB */ 
	 {0x1416, 0x03FB, 0xcd},	   /* index 171, gain = 26.013396 dB */ 
	 {0x13df, 0x03FB, 0xcc},	   /* index 172, gain = 25.844732 dB */ 
	 {0x13a8, 0x03FB, 0xcc},	   /* index 173, gain = 25.844732 dB */ 
	 {0x1372, 0x03FB, 0xcb},	   /* index 174, gain = 25.679282 dB */ 
	 {0x133c, 0x03FB, 0xcb},	   /* index 175, gain = 25.679282 dB */ 
	 {0x1307, 0x03FB, 0xca},	   /* index 176, gain = 25.516924 dB */ 
	 {0x12d3, 0x03FB, 0xca},	   /* index 177, gain = 25.516924 dB */ 
	 {0x129f, 0x03FB, 0xc9},	   /* index 178, gain = 25.357546 dB */ 
	 {0x126b, 0x03FB, 0xc8},	   /* index 179, gain = 25.201039 dB */ 
	 {0x1238, 0x03FB, 0xc8},	   /* index 180, gain = 25.201039 dB */ 
	 {0x1206, 0x03FB, 0xc7},	   /* index 181, gain = 25.047302 dB */ 
	 {0x11d5, 0x03FB, 0xc7},	   /* index 182, gain = 25.047302 dB */ 
	 {0x11a3, 0x03FB, 0xc6},	   /* index 183, gain = 24.896239 dB */ 
	 {0x1173, 0x03FB, 0xc5},	   /* index 184, gain = 24.747759 dB */ 
	 {0x1143, 0x03FB, 0xc5},	   /* index 185, gain = 24.747759 dB */ 
	 {0x1113, 0x03FB, 0xc4},	   /* index 186, gain = 24.601774 dB */ 
	 {0x10e4, 0x03FB, 0xc3},	   /* index 187, gain = 24.458203 dB */ 
	 {0x10b5, 0x03FB, 0xc3},	   /* index 188, gain = 24.458203 dB */ 
	 {0x1087, 0x03FB, 0xc2},	   /* index 189, gain = 24.316966 dB */ 
	 {0x105a, 0x03FB, 0xc1},	   /* index 190, gain = 24.177988 dB */ 
	 {0x102d, 0x03FB, 0xc1},	   /* index 191, gain = 24.177988 dB */  
	 {0x1000, 0x01ff, 0xe0},	   /* index 192, gain = 24.061800 dB */ 
	 {0x0fd4, 0x01ff, 0xe0},	   /* index 193, gain = 24.061800 dB */ 
	 {0x0fa8, 0x01ff, 0xdf},	   /* index 194, gain = 23.794521 dB */ 
	 {0x0f7d, 0x01ff, 0xdf},	   /* index 195, gain = 23.794521 dB */ 
	 {0x0f52, 0x01ff, 0xdf},	   /* index 196, gain = 23.794521 dB */ 
	 {0x0f28, 0x01ff, 0xde},	   /* index 197, gain = 23.535221 dB */ 
	 {0x0efe, 0x01ff, 0xde},	   /* index 198, gain = 23.535221 dB */ 
	 {0x0ed5, 0x01ff, 0xdd},	   /* index 199, gain = 23.283438 dB */ 
	 {0x0eac, 0x01ff, 0xdd},	   /* index 200, gain = 23.283438 dB */ 
	 {0x0e84, 0x01ff, 0xdd},	   /* index 201, gain = 23.283438 dB */ 
	 {0x0e5c, 0x01ff, 0xdc},	   /* index 202, gain = 23.038749 dB */ 
	 {0x0e34, 0x01ff, 0xdc},	   /* index 203, gain = 23.038749 dB */ 
	 {0x0e0d, 0x01ff, 0xdc},	   /* index 204, gain = 23.038749 dB */ 
	 {0x0de6, 0x01ff, 0xdb},	   /* index 205, gain = 22.800765 dB */ 
	 {0x0dc0, 0x01ff, 0xdb},	   /* index 206, gain = 22.800765 dB */ 
	 {0x0d9a, 0x01ff, 0xda},	   /* index 207, gain = 22.569127 dB */ 
	 {0x0d74, 0x01ff, 0xda},	   /* index 208, gain = 22.569127 dB */ 
	 {0x0d4f, 0x01ff, 0xda},	   /* index 209, gain = 22.569127 dB */ 
	 {0x0d2b, 0x01ff, 0xd9},	   /* index 210, gain = 22.343507 dB */ 
	 {0x0d06, 0x01ff, 0xd9},	   /* index 211, gain = 22.343507 dB */ 
	 {0x0ce2, 0x01ff, 0xd8},	   /* index 212, gain = 22.123599 dB */ 
	 {0x0cbf, 0x01ff, 0xd8},	   /* index 213, gain = 22.123599 dB */ 
	 {0x0c9c, 0x01ff, 0xd7},	   /* index 214, gain = 21.909122 dB */ 
	 {0x0c79, 0x01ff, 0xd7},	   /* index 215, gain = 21.909122 dB */ 
	 {0x0c56, 0x01ff, 0xd7},	   /* index 216, gain = 21.909122 dB */ 
	 {0x0c34, 0x01ff, 0xd6},	   /* index 217, gain = 21.699813 dB */ 
	 {0x0c13, 0x01ff, 0xd6},	   /* index 218, gain = 21.699813 dB */ 
	 {0x0bf1, 0x01ff, 0xd5},	   /* index 219, gain = 21.495430 dB */ 
	 {0x0bd1, 0x01ff, 0xd5},	   /* index 220, gain = 21.495430 dB */ 
	 {0x0bb0, 0x01ff, 0xd4},	   /* index 221, gain = 21.295746 dB */ 
	 {0x0b90, 0x01ff, 0xd4},	   /* index 222, gain = 21.295746 dB */ 
	 {0x0b70, 0x01ff, 0xd3},	   /* index 223, gain = 21.100549 dB */ 
	 {0x0b50, 0x01ff, 0xd3},	   /* index 224, gain = 21.100549 dB */ 
	 {0x0b31, 0x01ff, 0xd2},	   /* index 225, gain = 20.909643 dB */ 
	 {0x0b12, 0x01ff, 0xd2},	   /* index 226, gain = 20.909643 dB */ 
	 {0x0af4, 0x01ff, 0xd1},	   /* index 227, gain = 20.722842 dB */ 
	 {0x0ad6, 0x01ff, 0xd1},	   /* index 228, gain = 20.722842 dB */ 
	 {0x0ab8, 0x01ff, 0xd0},	   /* index 229, gain = 20.539975 dB */ 
	 {0x0a9a, 0x01ff, 0xd0},	   /* index 230, gain = 20.539975 dB */ 
	 {0x0a7d, 0x01ff, 0xcf},	   /* index 231, gain = 20.360878 dB */ 
	 {0x0a60, 0x01ff, 0xcf},	   /* index 232, gain = 20.360878 dB */ 
	 {0x0a43, 0x01ff, 0xce},	   /* index 233, gain = 20.185399 dB */ 
	 {0x0a27, 0x01ff, 0xce},	   /* index 234, gain = 20.185399 dB */ 
	 {0x0a0b, 0x01ff, 0xcd},	   /* index 235, gain = 20.013396 dB */ 
	 {0x09ef, 0x01ff, 0xcc},	   /* index 236, gain = 19.844732 dB */ 
	 {0x09d4, 0x01ff, 0xcc},	   /* index 237, gain = 19.844732 dB */ 
	 {0x09b9, 0x01ff, 0xcb},	   /* index 238, gain = 19.679282 dB */ 
	 {0x099e, 0x01ff, 0xcb},	   /* index 239, gain = 19.679282 dB */ 
	 {0x0983, 0x01ff, 0xca},	   /* index 240, gain = 19.516924 dB */ 
	 {0x0969, 0x01ff, 0xca},	   /* index 241, gain = 19.516924 dB */ 
	 {0x094f, 0x01ff, 0xc9},	   /* index 242, gain = 19.357546 dB */ 
	 {0x0936, 0x01ff, 0xc8},	   /* index 243, gain = 19.201039 dB */ 
	 {0x091c, 0x01ff, 0xc8},	   /* index 244, gain = 19.201039 dB */ 
	 {0x0903, 0x01ff, 0xc7},	   /* index 245, gain = 19.047302 dB */ 
	 {0x08ea, 0x01ff, 0xc7},	   /* index 246, gain = 19.047302 dB */ 
	 {0x08d2, 0x01ff, 0xc6},	   /* index 247, gain = 18.896239 dB */ 
	 {0x08b9, 0x01ff, 0xc5},	   /* index 248, gain = 18.747759 dB */ 
	 {0x08a1, 0x01ff, 0xc5},	   /* index 249, gain = 18.747759 dB */ 
	 {0x088a, 0x01ff, 0xc4},	   /* index 250, gain = 18.601774 dB */ 
	 {0x0872, 0x01ff, 0xc3},	   /* index 251, gain = 18.458203 dB */ 
	 {0x085b, 0x01ff, 0xc3},	   /* index 252, gain = 18.458203 dB */ 
	 {0x0844, 0x01ff, 0xc2},	   /* index 253, gain = 18.316966 dB */ 
	 {0x082d, 0x01ff, 0xc1},	   /* index 254, gain = 18.177988 dB */ 
	 {0x0816, 0x01ff, 0xc1},	   /* index 255, gain = 18.177988 dB */ 
	 {0x0800, 0x0100, 0xe0},	   /* index 256, gain = 18.061800 dB */ 
	 {0x07ea, 0x0100, 0xe0},	   /* index 257, gain = 18.061800 dB */ 
	 {0x07d4, 0x0100, 0xdf},	   /* index 258, gain = 17.794521 dB */ 
	 {0x07bf, 0x0100, 0xdf},	   /* index 259, gain = 17.794521 dB */ 
	 {0x07a9, 0x0100, 0xdf},	   /* index 260, gain = 17.794521 dB */ 
	 {0x0794, 0x0100, 0xde},	   /* index 261, gain = 17.535221 dB */ 
	 {0x077f, 0x0100, 0xde},	   /* index 262, gain = 17.535221 dB */ 
	 {0x076a, 0x0100, 0xdd},	   /* index 263, gain = 17.283438 dB */ 
	 {0x0756, 0x0100, 0xdd},	   /* index 264, gain = 17.283438 dB */ 
	 {0x0742, 0x0100, 0xdd},	   /* index 265, gain = 17.283438 dB */ 
	 {0x072e, 0x0100, 0xdc},	   /* index 266, gain = 17.038749 dB */ 
	 {0x071a, 0x0100, 0xdc},	   /* index 267, gain = 17.038749 dB */ 
	 {0x0706, 0x0100, 0xdc},	   /* index 268, gain = 17.038749 dB */ 
	 {0x06f3, 0x0100, 0xdb},	   /* index 269, gain = 16.800765 dB */ 
	 {0x06e0, 0x0100, 0xdb},	   /* index 270, gain = 16.800765 dB */ 
	 {0x06cd, 0x0100, 0xda},	   /* index 271, gain = 16.569127 dB */ 
	 {0x06ba, 0x0100, 0xda},	   /* index 272, gain = 16.569127 dB */ 
	 {0x06a8, 0x0100, 0xda},	   /* index 273, gain = 16.569127 dB */ 
	 {0x0695, 0x0100, 0xd9},	   /* index 274, gain = 16.343507 dB */ 
	 {0x0683, 0x0100, 0xd9},	   /* index 275, gain = 16.343507 dB */ 
	 {0x0671, 0x0100, 0xd8},	   /* index 276, gain = 16.123599 dB */ 
	 {0x065f, 0x0100, 0xd8},	   /* index 277, gain = 16.123599 dB */ 
	 {0x064e, 0x0100, 0xd7},	   /* index 278, gain = 15.909122 dB */ 
	 {0x063c, 0x0100, 0xd7},	   /* index 279, gain = 15.909122 dB */ 
	 {0x062b, 0x0100, 0xd7},	   /* index 280, gain = 15.909122 dB */ 
	 {0x061a, 0x0100, 0xd6},	   /* index 281, gain = 15.699813 dB */ 
	 {0x0609, 0x0100, 0xd6},	   /* index 282, gain = 15.699813 dB */ 
	 {0x05f9, 0x0100, 0xd5},	   /* index 283, gain = 15.495430 dB */ 
	 {0x05e8, 0x0100, 0xd5},	   /* index 284, gain = 15.495430 dB */ 
	 {0x05d8, 0x0100, 0xd4},	   /* index 285, gain = 15.295746 dB */ 
	 {0x05c8, 0x0100, 0xd4},	   /* index 286, gain = 15.295746 dB */ 
	 {0x05b8, 0x0100, 0xd3},	   /* index 287, gain = 15.100549 dB */ 
	 {0x05a8, 0x0100, 0xd3},	   /* index 288, gain = 15.100549 dB */ 
	 {0x0599, 0x0100, 0xd2},	   /* index 289, gain = 14.909643 dB */ 
	 {0x0589, 0x0100, 0xd2},	   /* index 290, gain = 14.909643 dB */ 
	 {0x057a, 0x0100, 0xd1},	   /* index 291, gain = 14.722842 dB */ 
	 {0x056b, 0x0100, 0xd1},	   /* index 292, gain = 14.722842 dB */ 
	 {0x055c, 0x0100, 0xd0},	   /* index 293, gain = 14.539975 dB */ 
	 {0x054d, 0x0100, 0xd0},	   /* index 294, gain = 14.539975 dB */ 
	 {0x053e, 0x0100, 0xcf},	   /* index 295, gain = 14.360878 dB */ 
	 {0x0530, 0x0100, 0xcf},	   /* index 296, gain = 14.360878 dB */ 
	 {0x0522, 0x0100, 0xce},	   /* index 297, gain = 14.185399 dB */ 
	 {0x0514, 0x0100, 0xce},	   /* index 298, gain = 14.185399 dB */ 
	 {0x0506, 0x0100, 0xcd},	   /* index 299, gain = 14.013396 dB */ 
	 {0x04f8, 0x0100, 0xcc},	   /* index 300, gain = 13.844732 dB */ 
	 {0x04ea, 0x0100, 0xcc},	   /* index 301, gain = 13.844732 dB */ 
	 {0x04dc, 0x0100, 0xcb},	   /* index 302, gain = 13.679282 dB */ 
	 {0x04cf, 0x0100, 0xcb},	   /* index 303, gain = 13.679282 dB */ 
	 {0x04c2, 0x0100, 0xca},	   /* index 304, gain = 13.516924 dB */ 
	 {0x04b5, 0x0100, 0xca},	   /* index 305, gain = 13.516924 dB */ 
	 {0x04a8, 0x0100, 0xc9},	   /* index 306, gain = 13.357546 dB */ 
	 {0x049b, 0x0100, 0xc8},	   /* index 307, gain = 13.201039 dB */ 
	 {0x048e, 0x0100, 0xc8},	   /* index 308, gain = 13.201039 dB */ 
	 {0x0482, 0x0100, 0xc7},	   /* index 309, gain = 13.047302 dB */ 
	 {0x0475, 0x0100, 0xc7},	   /* index 310, gain = 13.047302 dB */ 
	 {0x0469, 0x0100, 0xc6},	   /* index 311, gain = 12.896239 dB */ 
	 {0x045d, 0x0100, 0xc5},	   /* index 312, gain = 12.747759 dB */ 
	 {0x0451, 0x0100, 0xc5},	   /* index 313, gain = 12.747759 dB */ 
	 {0x0445, 0x0100, 0xc4},	   /* index 314, gain = 12.601774 dB */ 
	 {0x0439, 0x0100, 0xc3},	   /* index 315, gain = 12.458203 dB */ 
	 {0x042d, 0x0100, 0xc3},	   /* index 316, gain = 12.458203 dB */ 
	 {0x0422, 0x0100, 0xc2},	   /* index 317, gain = 12.316966 dB */ 
	 {0x0416, 0x0100, 0xc1},	   /* index 318, gain = 12.177988 dB */ 
	 {0x040b, 0x0100, 0xc1},	   /* index 319, gain = 12.177988 dB */ 
	 {0x0400, 0x0100, 0xc0},	   /* index 320, gain = 12.041200 dB */
	 {0x03f5, 0x0100, 0xbf},	   /* index 321, gain = 11.906532 dB */ 
	 {0x03ea, 0x0100, 0xbf},	   /* index 322, gain = 11.906532 dB */ 
	 {0x03df, 0x0100, 0xbe},	   /* index 323, gain = 11.773921 dB */ 
	 {0x03d5, 0x0100, 0xbd},	   /* index 324, gain = 11.643303 dB */ 
	 {0x03ca, 0x0100, 0xbc},	   /* index 325, gain = 11.514621 dB */ 
	 {0x03c0, 0x0100, 0xbc},	   /* index 326, gain = 11.514621 dB */ 
	 {0x03b5, 0x0100, 0xbb},	   /* index 327, gain = 11.387817 dB */ 
	 {0x03ab, 0x0100, 0xba},	   /* index 328, gain = 11.262839 dB */ 
	 {0x03a1, 0x0100, 0xb9},	   /* index 329, gain = 11.139632 dB */ 
	 {0x0397, 0x0100, 0xb9},	   /* index 330, gain = 11.139632 dB */ 
	 {0x038d, 0x0100, 0xb8},	   /* index 331, gain = 11.018149 dB */ 
	 {0x0383, 0x0100, 0xb7},	   /* index 332, gain = 10.898342 dB */ 
	 {0x037a, 0x0100, 0xb6},	   /* index 333, gain = 10.780165 dB */ 
	 {0x0370, 0x0100, 0xb6},	   /* index 334, gain = 10.780165 dB */ 
	 {0x0366, 0x0100, 0xb5},	   /* index 335, gain = 10.663574 dB */ 
	 {0x035d, 0x0100, 0xb4},	   /* index 336, gain = 10.548527 dB */ 
	 {0x0354, 0x0100, 0xb3},	   /* index 337, gain = 10.434985 dB */ 
	 {0x034b, 0x0100, 0xb2},	   /* index 338, gain = 10.322907 dB */ 
	 {0x0342, 0x0100, 0xb1},	   /* index 339, gain = 10.212257 dB */ 
	 {0x0339, 0x0100, 0xb1},	   /* index 340, gain = 10.212257 dB */ 
	 {0x0330, 0x0100, 0xb0},	   /* index 341, gain = 10.103000 dB */ 
	 {0x0327, 0x0100, 0xaf},	   /* index 342, gain =  9.995099 dB */ 
	 {0x031e, 0x0100, 0xae},	   /* index 343, gain =  9.888522 dB */ 
	 {0x0316, 0x0100, 0xad},	   /* index 344, gain =  9.783237 dB */ 
	 {0x030d, 0x0100, 0xac},	   /* index 345, gain =  9.679214 dB */ 
	 {0x0305, 0x0100, 0xab},	   /* index 346, gain =  9.576421 dB */ 
	 {0x02fc, 0x0100, 0xaa},	   /* index 347, gain =  9.474830 dB */ 
	 {0x02f4, 0x0100, 0xa9},	   /* index 348, gain =  9.374414 dB */ 
	 {0x02ec, 0x0100, 0xa8},	   /* index 349, gain =  9.275146 dB */ 
	 {0x02e4, 0x0100, 0xa7},	   /* index 350, gain =  9.176999 dB */ 
	 {0x02dc, 0x0100, 0xa6},	   /* index 351, gain =  9.079949 dB */ 
	 {0x02d4, 0x0100, 0xa5},	   /* index 352, gain =  8.983971 dB */ 
	 {0x02cc, 0x0100, 0xa5},	   /* index 353, gain =  8.983971 dB */ 
	 {0x02c5, 0x0100, 0xa4},	   /* index 354, gain =  8.889043 dB */ 
	 {0x02bd, 0x0100, 0xa3},	   /* index 355, gain =  8.795140 dB */ 
	 {0x02b5, 0x0100, 0xa1},	   /* index 356, gain =  8.610327 dB */ 
	 {0x02ae, 0x0100, 0xa0},	   /* index 357, gain =  8.519375 dB */ 
	 {0x02a7, 0x0100, 0x9f},	   /* index 358, gain =  8.429365 dB */ 
	 {0x029f, 0x0100, 0x9e},	   /* index 359, gain =  8.340278 dB */ 
	 {0x0298, 0x0100, 0x9d},	   /* index 360, gain =  8.252095 dB */ 
	 {0x0291, 0x0100, 0x9c},	   /* index 361, gain =  8.164799 dB */ 
	 {0x028a, 0x0100, 0x9b},	   /* index 362, gain =  8.078372 dB */ 
	 {0x0283, 0x0100, 0x9a},	   /* index 363, gain =  7.992796 dB */ 
	 {0x027c, 0x0100, 0x99},	   /* index 364, gain =  7.908055 dB */ 
	 {0x0275, 0x0100, 0x98},	   /* index 365, gain =  7.824133 dB */ 
	 {0x026e, 0x0100, 0x97},	   /* index 366, gain =  7.741013 dB */ 
	 {0x0268, 0x0100, 0x96},	   /* index 367, gain =  7.658682 dB */ 
	 {0x0261, 0x0100, 0x94},	   /* index 368, gain =  7.496324 dB */ 
	 {0x025a, 0x0100, 0x93},	   /* index 369, gain =  7.416269 dB */ 
	 {0x0254, 0x0100, 0x92},	   /* index 370, gain =  7.336946 dB */ 
	 {0x024d, 0x0100, 0x91},	   /* index 371, gain =  7.258340 dB */ 
	 {0x0247, 0x0100, 0x90},	   /* index 372, gain =  7.180439 dB */ 
	 {0x0241, 0x0100, 0x8e},	   /* index 373, gain =  7.026702 dB */ 
	 {0x023b, 0x0100, 0x8d},	   /* index 374, gain =  6.950842 dB */ 
	 {0x0234, 0x0100, 0x8c},	   /* index 375, gain =  6.875640 dB */ 
	 {0x022e, 0x0100, 0x8b},	   /* index 376, gain =  6.801082 dB */ 
	 {0x0228, 0x0100, 0x89},	   /* index 377, gain =  6.653860 dB */ 
	 {0x0222, 0x0100, 0x88},	   /* index 378, gain =  6.581174 dB */ 
	 {0x021c, 0x0100, 0x87},	   /* index 379, gain =  6.509092 dB */ 
	 {0x0217, 0x0100, 0x85},	   /* index 380, gain =  6.366697 dB */ 
	 {0x0211, 0x0100, 0x84},	   /* index 381, gain =  6.296366 dB */ 
	 {0x020b, 0x0100, 0x83},	   /* index 382, gain =  6.226599 dB */ 
	 {0x0206, 0x0100, 0x81},	   /* index 383, gain =  6.088725 dB */ 
	 {0x0200, 0x0100, 0x80},	   /* index 384, gain =  6.020600 dB */ 
	 {0x01fa, 0x0100, 0x7f},	   /* index 385, gain =  5.953005 dB */ 
	 {0x01f5, 0x0100, 0x7d},	   /* index 386, gain =  5.819373 dB */ 
	 {0x01f0, 0x0100, 0x7c},	   /* index 387, gain =  5.753321 dB */ 
	 {0x01ea, 0x0100, 0x7a},	   /* index 388, gain =  5.622703 dB */ 
	 {0x01e5, 0x0100, 0x79},	   /* index 389, gain =  5.558124 dB */ 
	 {0x01e0, 0x0100, 0x77},	   /* index 390, gain =  5.430388 dB */ 
	 {0x01db, 0x0100, 0x76},	   /* index 391, gain =  5.367218 dB */ 
	 {0x01d6, 0x0100, 0x74},	   /* index 392, gain =  5.242239 dB */ 
	 {0x01d0, 0x0100, 0x73},	   /* index 393, gain =  5.180417 dB */ 
	 {0x01cb, 0x0100, 0x71},	   /* index 394, gain =  5.058079 dB */ 
	 {0x01c6, 0x0100, 0x70},	   /* index 395, gain =  4.997549 dB */ 
	 {0x01c2, 0x0100, 0x6e},	   /* index 396, gain =  4.877742 dB */ 
	 {0x01bd, 0x0100, 0x6d},	   /* index 397, gain =  4.818453 dB */ 
	 {0x01b8, 0x0100, 0x6b},	   /* index 398, gain =  4.701074 dB */ 
	 {0x01b3, 0x0100, 0x69},	   /* index 399, gain =  4.585260 dB */ 
	 {0x01af, 0x0100, 0x68},	   /* index 400, gain =  4.527928 dB */ 
	 {0x01aa, 0x0100, 0x66},	   /* index 401, gain =  4.414385 dB */ 
	 {0x01a5, 0x0100, 0x64},	   /* index 402, gain =  4.302307 dB */ 
	 {0x01a1, 0x0100, 0x63},	   /* index 403, gain =  4.246806 dB */ 
	 {0x019c, 0x0100, 0x61},	   /* index 404, gain =  4.136857 dB */ 
	 {0x0198, 0x0100, 0x5f},	   /* index 405, gain =  4.028282 dB */ 
	 {0x0193, 0x0100, 0x5e},	   /* index 406, gain =  3.974499 dB */ 
	 {0x018f, 0x0100, 0x5c},	   /* index 407, gain =  3.867922 dB */ 
	 {0x018b, 0x0100, 0x5a},	   /* index 408, gain =  3.762638 dB */ 
	 {0x0187, 0x0100, 0x58},	   /* index 409, gain =  3.658614 dB */ 
	 {0x0182, 0x0100, 0x56},	   /* index 410, gain =  3.555821 dB */ 
	 {0x017e, 0x0100, 0x55},	   /* index 411, gain =  3.504877 dB */ 
	 {0x017a, 0x0100, 0x53},	   /* index 412, gain =  3.403877 dB */ 
	 {0x0176, 0x0100, 0x51},	   /* index 413, gain =  3.304038 dB */ 
	 {0x0172, 0x0100, 0x4f},	   /* index 414, gain =  3.205334 dB */ 
	 {0x016e, 0x0100, 0x4d},	   /* index 415, gain =  3.107739 dB */ 
	 {0x016a, 0x0100, 0x4b},	   /* index 416, gain =  3.011228 dB */ 
	 {0x0166, 0x0100, 0x49},	   /* index 417, gain =  2.915778 dB */ 
	 {0x0162, 0x0100, 0x47},	   /* index 418, gain =  2.821365 dB */ 
	 {0x015e, 0x0100, 0x45},	   /* index 419, gain =  2.727967 dB */ 
	 {0x015b, 0x0100, 0x43},	   /* index 420, gain =  2.635563 dB */ 
	 {0x0157, 0x0100, 0x41},	   /* index 421, gain =  2.544132 dB */ 
	 {0x0153, 0x0100, 0x3f},	   /* index 422, gain =  2.453653 dB */ 
	 {0x0150, 0x0100, 0x3d},	   /* index 423, gain =  2.364107 dB */ 
	 {0x014c, 0x0100, 0x3b},	   /* index 424, gain =  2.275475 dB */ 
	 {0x0148, 0x0100, 0x38},	   /* index 425, gain =  2.144199 dB */ 
	 {0x0145, 0x0100, 0x36},	   /* index 426, gain =  2.057772 dB */ 
	 {0x0141, 0x0100, 0x34},	   /* index 427, gain =  1.972196 dB */ 
	 {0x013e, 0x0100, 0x32},	   /* index 428, gain =  1.887455 dB */ 
	 {0x013a, 0x0100, 0x30},	   /* index 429, gain =  1.803533 dB */ 
	 {0x0137, 0x0100, 0x2d},	   /* index 430, gain =  1.679150 dB */ 
	 {0x0134, 0x0100, 0x2b},	   /* index 431, gain =  1.597207 dB */ 
	 {0x0130, 0x0100, 0x29},	   /* index 432, gain =  1.516030 dB */ 
	 {0x012d, 0x0100, 0x26},	   /* index 433, gain =  1.395669 dB */ 
	 {0x012a, 0x0100, 0x24},	   /* index 434, gain =  1.316346 dB */ 
	 {0x0127, 0x0100, 0x22},	   /* index 435, gain =  1.237740 dB */ 
	 {0x0124, 0x0100, 0x1f},	   /* index 436, gain =  1.121149 dB */ 
	 {0x0120, 0x0100, 0x1d},	   /* index 437, gain =  1.044282 dB */ 
	 {0x011d, 0x0100, 0x1a},	   /* index 438, gain =  0.930243 dB */ 
	 {0x011a, 0x0100, 0x18},	   /* index 439, gain =  0.855040 dB */ 
	 {0x0117, 0x0100, 0x15},	   /* index 440, gain =  0.743442 dB */ 
	 {0x0114, 0x0100, 0x13},	   /* index 441, gain =  0.669832 dB */ 
	 {0x0111, 0x0100, 0x10},	   /* index 442, gain =  0.560574 dB */ 
	 {0x010e, 0x0100, 0x0d},	   /* index 443, gain =  0.452674 dB */ 
	 {0x010b, 0x0100, 0x0b},	   /* index 444, gain =  0.381478 dB */ 
	 {0x0108, 0x0100, 0x08},	   /* index 445, gain =  0.275766 dB */ 
	 {0x0106, 0x0100, 0x05},	   /* index 446, gain =  0.171325 dB */ 
	 {0x0103, 0x0100, 0x03},	   /* index 447, gain =  0.102389 dB */ 
	 {0x0100, 0x0100, 0x00},	   /* index 448, gain =  0.000000 dB */ 
 };


static struct msm_camera_i2c_reg_conf imx132_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf imx132_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf imx132_groupon_settings[] = {
	{0x0104, 0x01},
};

static struct msm_camera_i2c_reg_conf imx132_groupoff_settings[] = {
	{0x0104, 0x00},
};

static struct msm_camera_i2c_reg_conf imx132_prev_settings[] = {
	/* 30fps 1/2 * 1/2 */
	/* PLL setting */
	{0x0305, 0x02}, /* pre_pll_clk_div[7:0] */
	{0x0307, 0x43}, /* pll_multiplier[7:0] */
	{0x30A4, 0x02},
	{0x303C, 0x4B},
	/* mode setting */
	{0x0340, 0x09}, /* frame_length_lines[15:8] */
	{0x0341, 0x50}, /* frame_length_lines[7:0] */
	{0x0342, 0x08}, /* line_length_pck[15:8] */
	{0x0343, 0xC8}, /* line_length_pck[7:0] */
	{0x0344, 0x00}, /* x_addr_start[15:8] */
	{0x0345, 0x1C}, /* x_addr_start[7:0] */
	{0x0346, 0x00}, /* y_addr_start[15:8] */
	{0x0347, 0x3C}, /* y_addr_start[7:0] */
	{0x0348, 0x07}, /* x_addr_end[15:8] */
	{0x0349, 0x9B}, /* x_addr_end[7:0] */
	{0x034A, 0x04}, /* y_addr_end[15:8] */
	{0x034B, 0x73}, /* y_addr_end[7:0] */
	{0x034C, 0x07}, /* x_output_size[15:8] */
	{0x034D, 0x80}, /* x_output_size[7:0] */
	{0x034E, 0x04}, /* y_output_size[15:8] */
	{0x034F, 0x38}, /* y_output_size[7:0] */
	{0x0381, 0x01}, /* x_even_inc[3:0] */
	{0x0383, 0x01}, /* x_odd_inc[3:0] */
	{0x0385, 0x01}, /* y_even_inc[7:0] */
	{0x0387, 0x01}, /* y_odd_inc[7:0] */
	{0x303D, 0x10},
	{0x303E, 0x4A},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x00},
	{0x304C, 0x2F},
	{0x304D, 0x02},
	{0x3064, 0x92},
	{0x306A, 0x10},
	{0x309B, 0x00},
	{0x309E, 0x41},
	{0x30A0, 0x10},
	{0x30A1, 0x0B},
	{0x30B2, 0x00},
	{0x30D5, 0x00},
	{0x30D6, 0x00},
	{0x30D7, 0x00},
	{0x30D8, 0x00},
	{0x30D9, 0x00},
	{0x30DA, 0x00},
	{0x30DB, 0x00},
	{0x30DC, 0x00},
	{0x30DD, 0x00},
	{0x30DE, 0x00},
	{0x3102, 0x0c},
	{0x3103, 0x33},
	{0x3104, 0x30},
	{0x3105, 0x00},
	{0x3106, 0xCA},
	{0x3107, 0x00},
	{0x3108, 0x06},
	{0x3109, 0x04},
	{0x310A, 0x04},
	{0x315C, 0x3D},
	{0x315D, 0x3C},
	{0x316E, 0x3E},
	{0x316F, 0x3D},
	{0x3301, 0x00},
	{0x3304, 0x07},
	{0x3305, 0x06},
	{0x3306, 0x19},
	{0x3307, 0x03},
	{0x3308, 0x0F},
	{0x3309, 0x07},
	{0x330A, 0x0C},
	{0x330B, 0x06},
	{0x330C, 0x0B},
	{0x330D, 0x07},
	{0x330E, 0x03},
	{0x3318, 0x60},
	{0x3322, 0x09},
	{0x3342, 0x00},
	{0x3348, 0xE0},
};

static struct msm_camera_i2c_reg_conf imx132_recommend_settings[] = {
	/* global setting */
	{0x3087, 0x53},
	{0x308B, 0x5A},
	{0x3094, 0x11},
	{0x309D, 0xA4},
	{0x30AA, 0x01},
	{0x30C6, 0x00},
	{0x30C7, 0x00},
	{0x3118, 0x2F},
	{0x312A, 0x00},
	{0x312B, 0x0B},
	{0x312C, 0x0B},
	{0x312D, 0x13},
	/* black level setting */
	{0x3032, 0x40},	
};


static struct v4l2_subdev_info imx132_subdev_info[] = {
	{
	.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
	.colorspace = V4L2_COLORSPACE_JPEG,
	.fmt    = 1,
	.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array imx132_init_conf[] = {
	{&imx132_recommend_settings[0],
	ARRAY_SIZE(imx132_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array imx132_confs[] = {
	{&imx132_prev_settings[0],
	ARRAY_SIZE(imx132_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&imx132_prev_settings[0],
	ARRAY_SIZE(imx132_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t imx132_dimensions[] = {
	{
	/* full size */
		.x_output = 0x780, /* 1920 */
		.y_output = 0x438, /* 1080 */
		.line_length_pclk = 0x8C8, /* 2248 */
		.frame_length_lines = 0x950, /* 2384 */
		.vt_pixel_clk = 160800000,
		.op_pixel_clk = 160800000,
		.binning_factor = 1,
	},
	{
	/* full size */
		.x_output = 0x780, /* 1920 */
		.y_output = 0x438, /* 1080 */
		.line_length_pclk = 0x8C8, /* 2248 */
		.frame_length_lines = 0x950, /* 2384 */
		.vt_pixel_clk = 160800000,
		.op_pixel_clk = 160800000,
		.binning_factor = 1,
	},
};

static struct msm_sensor_output_reg_addr_t imx132_reg_addr = {
	.x_output = 0x034C,
	.y_output = 0x034E,
	.line_length_pclk = 0x0342,
	.frame_length_lines = 0x0340,
};

static struct msm_sensor_id_info_t imx132_id_info = {
	.sensor_id_reg_addr = 0x0000,
	.sensor_id = 0x0132,
};

static struct msm_sensor_exp_gain_info_t imx132_exp_gain_info = {
	.coarse_int_time_addr = 0x0202,
	.global_gain_addr = 0x0204,
	.vert_offset = 5,
};

static enum msm_camera_vreg_name_t imx132_veg_seq[] = {
	CAM_VANA,
	CAM_VDIG,
	CAM_VIO,	
};

static const struct i2c_device_id imx132_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&imx132_s_ctrl},
	{ }
};

static struct i2c_driver imx132_i2c_driver = {
	.id_table = imx132_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx132_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&imx132_i2c_driver);
}

static struct v4l2_subdev_core_ops imx132_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops imx132_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops imx132_subdev_ops = {
	.core = &imx132_subdev_core_ops,
	.video  = &imx132_subdev_video_ops,
};

/* added by jing.hl
 *Index Tag: ZTEMT_CAMERA_20121126_64xGain
 * Fuction: 
 */

static int32_t ztemt_sensor_write_exp_gain1(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line, int32_t luma_avg, uint16_t fgain)		
{
	uint32_t fl_lines;
	uint8_t offset;
	uint32_t gain_index;
	uint16_t analog_gain = 0xffff;
	uint16_t digital_gain = 0xffff;

	fl_lines = s_ctrl->curr_frame_length_lines;
	fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;
	offset = s_ctrl->sensor_exp_gain_info->vert_offset;
	if (line > (fl_lines - offset))
		fl_lines = line + offset;

	s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, s_ctrl->sensor_output_reg_addr->frame_length_lines, fl_lines, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,	s_ctrl->sensor_exp_gain_info->coarse_int_time_addr, line, MSM_CAMERA_I2C_WORD_DATA);

	for (gain_index = IMX132_GAIN_0DB; gain_index>0; gain_index--)
	{
		if (IMX132_GLOBAL_GAIN_TABLE[gain_index][IMX132_GAIN_COL_AGC] >= gain)
		{
			break;
		}
	}
    
	analog_gain = IMX132_GLOBAL_GAIN_TABLE[gain_index][IMX132_ANALOG_DGAIN];
	digital_gain = IMX132_GLOBAL_GAIN_TABLE[gain_index][IMX132_DIGITAL_DGAIN];
	
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0204, analog_gain, MSM_CAMERA_I2C_WORD_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x020E, digital_gain, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0210, digital_gain, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0212, digital_gain, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0214, digital_gain, MSM_CAMERA_I2C_WORD_DATA);
	
	//printk("<ZTEMT_JHL> gain index =%d, gain is %d, {0x%x,0x%x,0x%x}\n",gain_index,gain,IMX132_GLOBAL_GAIN_TABLE[gain_index][IMX132_GAIN_COL_AGC],digital_gain,analog_gain);

	s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	return 0;
}



static struct msm_sensor_fn_t imx132_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = ztemt_sensor_write_exp_gain1,  //Index Tag: ZTEMT_CAMERA_20121126_64xGain
	.sensor_write_snapshot_exp_gain = ztemt_sensor_write_exp_gain1,	 //Index Tag: ZTEMT_CAMERA_20121126_64xGain
	.sensor_setting = msm_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_adjust_frame_lines = msm_sensor_adjust_frame_lines1,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
};

static struct msm_sensor_reg_t imx132_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = imx132_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(imx132_start_settings),
	.stop_stream_conf = imx132_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(imx132_stop_settings),
	.group_hold_on_conf = imx132_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(imx132_groupon_settings),
	.group_hold_off_conf = imx132_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(imx132_groupoff_settings),
	.init_settings = &imx132_init_conf[0],
	.init_size = ARRAY_SIZE(imx132_init_conf),
	.mode_settings = &imx132_confs[0],
	.output_settings = &imx132_dimensions[0],
	.num_conf = ARRAY_SIZE(imx132_confs),
};

static struct msm_sensor_ctrl_t imx132_s_ctrl = {
	.msm_sensor_reg = &imx132_regs,
	.sensor_i2c_client = &imx132_sensor_i2c_client,
	.sensor_i2c_addr = 0x6C,
	.vreg_seq = imx132_veg_seq,
	.num_vreg_seq = ARRAY_SIZE(imx132_veg_seq),
	.sensor_output_reg_addr = &imx132_reg_addr,
	.sensor_id_info = &imx132_id_info,
	.sensor_exp_gain_info = &imx132_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.msm_sensor_mutex = &imx132_mut,
	.sensor_i2c_driver = &imx132_i2c_driver,
	.sensor_v4l2_subdev_info = imx132_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx132_subdev_info),
	.sensor_v4l2_subdev_ops = &imx132_subdev_ops,
	.func_tbl = &imx132_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("SONY 2.4MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
