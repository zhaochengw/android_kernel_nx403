#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/ctype.h>
/*added by congshan 20121127 start */
extern unsigned int v5_intensity_value;
#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P7)
void zte_mipi_disp_inc_nt35590(unsigned int state);
#endif
#if defined( CONFIG_ZTEMT_LCD_Z5MINI2_SIZE_4P7)
#include <linux/nubia_lcd_ic_type.h>
void zte_mipi_disp_inc_r69431(unsigned int state);
void zte_mipi_disp_inc_tianma(unsigned int state);
#endif
void zte_mipi_disp_inc(unsigned int state);

static ssize_t intensity_show(struct kobject *kobj, struct kobj_attribute *attr,
   char *buf)
{
	return 0;
}
static ssize_t intensity_store(struct kobject *kobj, struct kobj_attribute *attr,
    const char *buf, size_t size)
{
	ssize_t ret = 0;
	int val;
	static bool is_firsttime = true;
	sscanf(buf, "%d", &val);
	pr_err("sss %s state=%d size=%d\n", __func__, (int)val, (int)size);
	v5_intensity_value = val;
	if (is_firsttime) {
		is_firsttime = false;
		return ret;
	}
#if defined( CONFIG_ZTEMT_LCD_Z5MINI_SIZE_4P7)
	zte_mipi_disp_inc_nt35590(val);
#elif defined( CONFIG_ZTEMT_LCD_Z5MINI2_SIZE_4P7)
	if (SHARP_R69431 == mini2_get_lcd_type()) {
		zte_mipi_disp_inc_r69431(val);
	} else {
		zte_mipi_disp_inc_tianma(val);
	}
#else
	zte_mipi_disp_inc(val);
#endif
	#if 0
	char *after;
	unsigned int state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;
	pr_err("sss %s state=%d size=%d\n", __func__, (int)state, (int)size);
	if (isspace(*after))
		count++;

	if (count == size) {
		ret = count;
		zte_mipi_disp_inc(state);
	}
	#endif
	return ret;


}

static struct kobj_attribute intensity_attribute =
 __ATTR(intensity, 0664, intensity_show, intensity_store);

 static struct attribute *attrs[] = {
  &intensity_attribute.attr,
  NULL, /* need to NULL terminate the list of attributes */
 };
 static struct attribute_group attr_group = {
  .attrs = attrs,
 };
 
 static struct kobject *id_kobj;
 
 static int __init id_init(void)
 {
  int retval;
 
  id_kobj = kobject_create_and_add("lcd_intensity", kernel_kobj);
  if (!id_kobj)
   return -ENOMEM;
 
  /* Create the files associated with this kobject */
  retval = sysfs_create_group(id_kobj, &attr_group);
  if (retval)
   kobject_put(id_kobj);
 
  return retval;
 }
 
 static void __exit id_exit(void)
 {
  kobject_put(id_kobj);
 }
 
 module_init(id_init);
 module_exit(id_exit);
 
 /*added by congshan 20121127 end */

