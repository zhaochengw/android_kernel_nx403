/******************************************************************************

  Copyright (C), 2001-2011, ZTE. Co., Ltd.

******************************************************************************/

#include <linux/input/zte_touch_common.h>
/*
*ts_status save the status of CTP,  if CTP detected ts_status = CTP_OFFLINE, otherwise ts_status = CTP_ONLINE
*/
static unsigned int zte_touch_status = CTP_OFFLINE;

unsigned int zte_touch_dbg = CTP_DBG_DISABLE;

unsigned int zte_touch_get_status(void)
{
	TP_INFO("ts_status = %d\n",zte_touch_status);
	return zte_touch_status;
}

void zte_touch_set_status(unsigned int flag)
{
	if (flag == CTP_OFFLINE || flag == CTP_ONLINE){
		zte_touch_status = flag;
		TP_INFO("ts_status = %d\n",zte_touch_status);
	}
	else {
		TP_ERR("flag value is invalid\n");
	}
}

void zte_touch_enable_dbg(unsigned int enable)
{
	TP_INFO("CTP_DBG %s\n",(enable == 1) ? "enable":"disable");
	zte_touch_dbg = enable;
}
