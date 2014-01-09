/* Copyright (c) 2011-2012, ZTEMT. All rights reserved.
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
#ifndef _ZTEMT_POWER_DEBUG_H
#define _ZTEMT_POWER_DEBUG_H
#define DRV_NAME "zte_power_debug"

struct battery_info {
    int percent_of_battery;
    int battery_current;
    int ibat_ua;
    int vbat_uv;
    int dc_chg_in;
    int usb_chg_in;
};
 
struct power_debug_info {
	struct work_struct work;
	struct hrtimer timer;
   // struct hrtimer longtimer;
    struct battery_info battery_information;
};
//print wake locks
extern void global_print_active_locks(int type);
extern void wakelock_stats_show_debug( void );
//print battery related information
extern int pm8921_bms_get_percent_charge(void);
extern int pm8921_bms_get_battery_current(int *);
extern int pm8921_bms_get_simultaneous_battery_voltage_and_current(int *ibat_ua,int *vbat_uv);
extern int pm8921_is_dc_chg_plugged_in(void);
extern int pm8921_is_usb_chg_plugged_in(void);
//print suspend_states
extern int suspend_stats_debug(void);
    

#endif  //_ZTEMT_POWER_DEBUG_H