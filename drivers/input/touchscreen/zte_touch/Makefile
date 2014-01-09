######################################################
#	        ZTE Touchscreen                         #
#####################################################
#Note: zte_cap_touch must palce ahead of the others panels
obj-$(CONFIG_ZTE_TOUCHSCREEN)	+= zte_ts_common.o
obj-$(CONFIG_ZTE_TOUCH_CAPACITANCE)	+= zte_cap_touch.o
#Note end

############ Add your CTP drivers at here ###########

obj-$(CONFIG_TOUCHSCREEN_ZTE_ATMEL_MXT336)	+= atmel_mxt336.o atmel_mxt336A.o
obj-$(CONFIG_TOUCHSCREEN_ZTE_MELFAS_MMS144)   += melfas_mms144.o melfas_mms144_isc_update.o
################### End #############################

#Note: zte_res_touch must palce behind of the others panels
obj-$(CONFIG_ZTE_TOUCH_RESISTANCE)	+= zte_res_touch.o
#Note end
