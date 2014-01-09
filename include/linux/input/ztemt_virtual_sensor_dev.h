#ifndef ZTEMT_VIRTUAL_SENSOR_DEV_H
#define ZTEMT_VIRTUAL_SENSOR_DEV_H

#define ZTEMT_VIRTUAL_SENSOR_DEV_NAME "virtual_sensor_dev"

//power for i2c and sensor start
#define ZTE_SENSORS_VTG_MAX_UV			3000000
#define ZTE_SENSORS_VTG_MIN_UV			3000000
#define ZTE_SENSORS_CURR_24HZ_UA		17500
#define ZTE_SENSORS_SLEEP_CURR_UA		10
#define ZTE_SENSORS_I2C_VTG_MAX_UV	    1800000
#define ZTE_SENSORS_I2C_VTG_MIN_UV		1800000
#define ZTE_SENSORS_I2C_CURR_UA			9630
#define ZTE_SENSORS_I2C_SLEEP_CURR_UA	10
struct zte_sensors_power_regulator {
	const char *name;
	__u32	max_uV;
	__u32	min_uV;
	__u32	hpm_load_uA;
	__u32	lpm_load_uA;
};

struct ztemt_virtual_sensor_dev_platform_data {
    __u8 num_regulators;
    struct zte_sensors_power_regulator *regulator_info;
};

#endif
