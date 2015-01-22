/*
 * Maxim MAX77819 WLED Backlight Class Driver
 *
 * Copyright (C) 2013 Maxim Integrated Product
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define log_level 2

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>

/* for Regmap */
#include <linux/regmap.h>

/* for Device Tree */
#include <linux/io.h>
#include <linux/of.h>

#include <linux/hrtimer.h>
#include <linux/pwm.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/mfd/max77819.h>
#include <linux/mfd/max77819-wled.h>
#ifdef ODIN_PWM
#include <linux/odin_pwm.h>
#endif

#define DRIVER_NAME    MAX77819_WLED_NAME
#define DRIVER_VERSION MAX77819_DRIVER_VERSION".0"

/* WLED MAX Brightness */
#define WLED_MAX_BRIGHTNESS                0xFF

/* WLED Interrupt register */
#define WLED_INT                           0x9B
#define WLED_INT_WLEDOVP                   BIT(7)
#define WLED_INT_WLEDOL                    BIT(4)
/* WLED Interrupt Mask register */
#define WLED_INT_MASK                      0x9C
/* WLED Currnet register */
#define IWLED                              0x99
/* WLED Control register */
#define WLEDBSTCNTL1                       0x98
#define WLEDBSTCNTL1_WLED1EN               BIT(7)
#define WLEDBSTCNTL1_WLED2EN               BIT(6)
#define WLEDBSTCNTL1_WLEDPWM1EN            BIT(5)
#define WLEDBSTCNTL1_WLEDPWM2EN            BIT(4)
#define WLEDBSTCNTL1_WLEDFOSC              BITS(3, 2)
#define WLEDBSTCNTL1_WLEDOVP               BIT(1)

#define WLEDBSTCNTL1_WLEDEN                (WLEDBSTCNTL1_WLED1EN|\
			WLEDBSTCNTL1_WLED2EN)
#define WLEDBSTCNTL1_WLEDPWMEN             (WLEDBSTCNTL1_WLEDPWM1EN|\
			WLEDBSTCNTL1_WLEDPWM2EN)

struct max77819_wled_current_map_data {
	int dpwm;
	int iwled;
};

struct max77819_wled_current_map {
	const char                                  *name;
	bool                                         exponential;
	int                                          data_sz;
	const struct max77819_wled_current_map_data *data;
};

struct max77819_wled {
	struct mutex                            lock;
	struct max77819_dev                    *chip;
	struct max77819_io                     *io;
	struct device                          *dev;
	struct kobject                         *kobj;
	const struct attribute_group           *attr_grp;
	struct max77819_wled_platform_data     *pdata;
	spinlock_t                              irq_lock;
	struct pwm_device                      *pwm_dev;
	struct backlight_device                *bl_dev;

	struct hrtimer                          ramp_timer;
	int                                     current_code;
	int                                     target_code;
	int                                     brightness;

	/* run-time configs from platform data */
	struct max77819_wled_platform_data      cfg;
};

#ifdef CONFIG_MACH_LGE
struct backlight_device *max77819_bl_device;
extern void (*lge_blk_set_level)(int bl_level);
#endif

#define __lock(_me)    mutex_lock(&(_me)->lock)
#define __unlock(_me)  mutex_unlock(&(_me)->lock)

static const char *max77819_wled_pwm_input_use_desc[] = {
	[MAX77819_WLED_PWM_INPUT_DISABLE] = "N/A",
	[MAX77819_WLED_PWM_INPUT_CABC]    = "CABC dimming",
	[MAX77819_WLED_PWM_INPUT_EXTRA]   = "IWLED extra",
};

#define __wled_pwm_input_use_desc(_n) \
	(max77819_wled_pwm_input_use_desc[_n])

#define WLED_CURRENT_MAP_DATA_NAME         "8exp"
#define WLED_CURRENT_MAP_DATA_EXPONENTIAL  true
#define WLED_CURRENT_MAP_DATA_FILE         \
	"max77819_bl-current_map-8exp"

static const struct max77819_wled_current_map_data
max77819_wled_current_map_data[] = {
#include WLED_CURRENT_MAP_DATA_FILE
};

static const struct max77819_wled_current_map max77819_wled_current_maps[] = {
#define WLED_CURRENT_MAP_DEFINE(_id, _name, _exponential, _data) \
	/*[_id] =*/ {\
		.name = _name,\
		.exponential = _exponential,\
		.data_sz = ARRAY_SIZE(_data),\
		.data = _data,\
	}

	WLED_CURRENT_MAP_DEFINE(0, WLED_CURRENT_MAP_DATA_NAME,
			WLED_CURRENT_MAP_DATA_EXPONENTIAL,
			max77819_wled_current_map_data),
};

#define WLED_NUM_OF_CURRENT_MAPS     (ARRAY_SIZE(max77819_wled_current_maps))
#define __wled_current_map(_id)      (&max77819_wled_current_maps[0])
#define __wled_current_map_name(_id) (__wled_current_map(_id)->name)

#define __wled_using_current_map_id(_me) \
	((_me)->cfg.current_map_id)
#define __wled_using_current_map(_me) \
	(__wled_current_map(__wled_using_current_map_id(_me)))
#define __wled_using_current_map_name(_me) \
	(__wled_using_current_map(_me)->name)
#define __wled_using_current_map_data_sz(_me) \
	(__wled_using_current_map(_me)->data_sz)

#define __wled_using_current_map_data(_me, _code) \
	(&__wled_using_current_map(_me)->data[_code])

#define __wled_current_code_dpwm(_me, _code) \
	(__wled_using_current_map_data(_me, _code)->dpwm)
#define __wled_current_code_iwled(_me, _code) \
	(__wled_using_current_map_data(_me, _code)->iwled)

/* map brightness -> current code */
#define __wled_current_map_code(_me, _brightness) \
	(__wled_using_current_map_data_sz(_me) > 0 ? \
	 (_brightness) * (__wled_using_current_map_data_sz(_me) - 1)\
	 / WLED_MAX_BRIGHTNESS : (_brightness))

static __always_inline
int max77819_wled_write_code_dpwm(struct max77819_wled *me, int code)
{
	int dpwm, rc = 0;
	int duty_ns = 0;

	if (unlikely(!me->pwm_dev)) {
		pr_info("%s: out\n", __func__);
		goto out;
	}

	dpwm = __wled_current_code_dpwm(me, code);

	if (unlikely(me->current_code >= 0 && __wled_current_code_dpwm(me,
					me->current_code) == dpwm)) {
		goto out;
	}

	if (unlikely(dpwm >= 10000)) {
		rc = max77819_masked_write(me->io, WLEDBSTCNTL1,
				WLEDBSTCNTL1_WLEDPWMEN,
				(u8)FFS(WLEDBSTCNTL1_WLEDPWM2EN), 0x00);
		if (unlikely(IS_ERR_VALUE(rc))) {
			log_err("WLEDBSTCNTL1 write error [%d]\n", rc);
			goto out;
		}
		pwm_disable(me->pwm_dev);
		goto out;
	}

#ifdef ODIN_PWM
	duty_ns = (int) me->cfg.pwm_period_nsec * (10000 - dpwm) / 10000;
#else
	duty_ns = (int) me->cfg.pwm_period_nsec * dpwm / 10000;
#endif
	rc = pwm_config(me->pwm_dev, duty_ns, me->cfg.pwm_period_nsec);

	if (unlikely(IS_ERR_VALUE(rc))) {
		log_err("failed to config PWM device [%d]\n", rc);
		goto out;
	}

	if (unlikely(me->current_code >= 0 && __wled_current_code_dpwm(me,
					me->current_code) <= 10000)) {
		log_vdbg("3. __wled_current_code_dpwm = %d\n", dpwm);
		rc = max77819_masked_write(me->io, WLEDBSTCNTL1,
				WLEDBSTCNTL1_WLEDPWMEN,
				(u8)FFS(WLEDBSTCNTL1_WLEDPWM2EN), 0x03);
		if (unlikely(IS_ERR_VALUE(rc))) {
			log_err("WLEDBSTCNTL1 write error [%d]\n", rc);
			goto out;
		}

		rc = pwm_enable(me->pwm_dev);
		if (unlikely(IS_ERR_VALUE(rc))) {
			log_err("failed to config PWM device [%d]\n", rc);
			goto out;
		}
	}

out:
	return rc;
}

	static __always_inline
int max77819_wled_write_code_regval(struct max77819_wled *me, int code)
{
	int iwled, rc = 0;

	iwled = __wled_current_code_iwled(me, code);

	if (unlikely(me->current_code >= 0 && __wled_current_code_iwled(me,
					me->current_code) == iwled)) {
		goto out;
	}

	rc = max77819_write(me->io, IWLED, (u8)iwled);
	if (unlikely(IS_ERR_VALUE(rc))) {
		log_err("IWLED write error [%d]\n", rc);
		goto out;
	}

	rc = max77819_masked_write(me->io, WLEDBSTCNTL1, WLEDBSTCNTL1_WLEDEN,
			(u8)FFS(WLEDBSTCNTL1_WLED2EN), (iwled ? 0x03 : 0x00));
	if (unlikely(IS_ERR_VALUE(rc))) {
		log_err("WLEDBSTCNTL1 write error [%d]\n", rc);
		goto out;
	}

out:
	return rc;
}

static __always_inline
int max77819_wled_write_code(struct max77819_wled *me, int code)
{
	int rc;

	rc = max77819_wled_write_code_dpwm(me, code);
	if (unlikely(IS_ERR_VALUE(rc)))
		goto out;

	rc = max77819_wled_write_code_regval(me, code);
	if (unlikely(IS_ERR_VALUE(rc)))
		goto out;

	me->current_code = code;

out:
	return rc;
}

static int max77819_wled_start_ramp_timer(struct max77819_wled *me,
		u32 ramp_time)
{
	unsigned long nsecs;
	int rc = 0;

	if (unlikely(me->current_code <= me->target_code || ramp_time == 0)) {
		rc = max77819_wled_write_code(me, me->target_code);
		goto out;
	}

	nsecs = (unsigned long)abs(me->target_code - me->current_code);
	nsecs = DIV_ROUND_UP((unsigned long)ramp_time * 1000UL, nsecs);
	log_vdbg("ramp expiry = +%lunsec\n", nsecs);

	rc = max77819_wled_write_code(me, me->current_code);
	if (unlikely(IS_ERR_VALUE(rc)))
		goto out;

	rc = hrtimer_start(&me->ramp_timer, ktime_set(0, nsecs),
			HRTIMER_MODE_REL);
	if (unlikely(IS_ERR_VALUE(rc))) {
		log_err("failed to start hrtimer [%d]\n", rc);
		goto out;
	}

out:
	return rc;
}

	static
enum hrtimer_restart max77819_wled_ramp_up_handler(struct hrtimer *hrtimer)
{
	struct max77819_wled *me =
		container_of(hrtimer, struct max77819_wled, ramp_timer);
	int rc;

	__lock(me);

	rc = max77819_wled_write_code(me, me->current_code + 1);
	if (unlikely(IS_ERR_VALUE(rc))) {
		rc = (int)HRTIMER_NORESTART;
		goto out;
	}

	if (unlikely(me->current_code >= me->target_code)) {
		log_dbg("reached destination brightness code %d\n",
				me->target_code);
		rc = (int)HRTIMER_NORESTART;
		goto out;
	}

	rc = (int)HRTIMER_RESTART;

out:
	__unlock(me);
	return (enum hrtimer_restart)rc;
}

	static
enum hrtimer_restart max77819_wled_ramp_down_handler(struct hrtimer *hrtimer)
{
	struct max77819_wled *me =
		container_of(hrtimer, struct max77819_wled, ramp_timer);
	int rc;

	__lock(me);

	rc = max77819_wled_write_code(me, me->current_code - 1);
	if (unlikely(IS_ERR_VALUE(rc))) {
		rc = (int)HRTIMER_NORESTART;
		goto out;
	}

	if (unlikely(me->current_code <= me->target_code)) {
		log_dbg("reached destination brightness code %d\n",
				me->target_code);
		rc = (int)HRTIMER_NORESTART;
		goto out;
	}

	rc = (int)HRTIMER_RESTART;

out:
	__unlock(me);
	return (enum hrtimer_restart)rc;
}

static __always_inline
int max77819_wled_start_ramp_up(struct max77819_wled *me)
{
	me->ramp_timer.function = max77819_wled_ramp_up_handler;
	return max77819_wled_start_ramp_timer(me, me->cfg.ramp_up_time_msec);
}

static __always_inline
int max77819_wled_start_ramp_down(struct max77819_wled *me)
{
	me->ramp_timer.function = max77819_wled_ramp_down_handler;
	return max77819_wled_start_ramp_timer(me, me->cfg.ramp_dn_time_msec);
}

static int max77819_wled_reset_cfg(struct max77819_wled *me)
{
#ifdef ODIN_PWM
	struct pwm_chip *chip;
	struct odin_chip *odin_chip;
#endif

	hrtimer_cancel(&me->ramp_timer);

	log_dbg("reset_cfg - current_map_id    %u\n", me->cfg.current_map_id);
	log_dbg("reset_cfg - ramp_up_time_msec %u\n",
			me->cfg.ramp_up_time_msec);
	log_dbg("reset_cfg - ramp_dn_time_msec %u\n",
			me->cfg.ramp_dn_time_msec);
	log_dbg("reset_cfg - pwm_input_use     %u\n", me->cfg.pwm_input_use);
	log_dbg("reset_cfg - pwm_id            %u\n", me->cfg.pwm_id);
	log_dbg("reset_cfg - pwm_period_nsec   %u\n", me->cfg.pwm_period_nsec);

	/* free old PWM device */

	if (likely(me->pwm_dev)) {
		pwm_free(me->pwm_dev);
		me->pwm_dev = NULL;
	}

	/* disable WLEDPWM */
	max77819_masked_write(me->io, WLEDBSTCNTL1, WLEDBSTCNTL1_WLEDPWMEN,
			(u8)FFS(WLEDBSTCNTL1_WLEDPWM2EN), 0x00);

	/* action on pwm_input_use */
	switch (me->cfg.pwm_input_use) {
	case MAX77819_WLED_PWM_INPUT_DISABLE:
		break;

	case MAX77819_WLED_PWM_INPUT_CABC:
		max77819_masked_write(me->io, WLEDBSTCNTL1,
				WLEDBSTCNTL1_WLEDPWMEN,
				(u8)FFS(WLEDBSTCNTL1_WLEDPWM2EN),
				0x03);
		break;

	case MAX77819_WLED_PWM_INPUT_EXTRA:
		if (likely(me->cfg.pwm_period_nsec > 0)) {
#ifdef ODIN_PWM
			chip = odin_get_pwm_chip();
			odin_chip = to_odin_chip(chip);
			chip->base = 0;

			me->pwm_dev = chip->pwms;
#else
			me->pwm_dev = pwm_request((int)me->cfg.pwm_id,
					DRIVER_NAME);
#endif
			if (unlikely(!me->pwm_dev)) {
				log_err("failed to request PWM(%u)\n",
						me->cfg.pwm_id);
			}
		}
		break;

	default:
		log_err("unknown PWM input use - %u\n",
				me->cfg.pwm_input_use);
		break;
	}

	/* convert brightness level (0..WLED_MAX_BRIGHTNESS) to
	 *         a code (index in mapping table)
	 */
	me->target_code = __wled_current_map_code(me, me->brightness);
	log_dbg("resetting - brightness level %d (code %d)\n", me->brightness,
			me->target_code);

	/* invalidate current code */
	me->current_code = -1;

	return max77819_wled_write_code(me, me->target_code);
}

static ssize_t max77819_wled_cfg_current_map_id_show(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	int rc;

	__lock(me);

	rc = (int)snprintf(buf, PAGE_SIZE, "%u (%s)\n",
			__wled_using_current_map_id(me),
			__wled_using_current_map_name(me));

	__unlock(me);
	return (ssize_t)rc;
}

static ssize_t max77819_wled_cfg_current_map_id_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	u32 current_map_id;

	__lock(me);

	current_map_id = (u32)simple_strtoul(buf, NULL, 10);

	if (unlikely(current_map_id >= WLED_NUM_OF_CURRENT_MAPS)) {
		log_err("invalid current map id - %u\n", current_map_id);
		goto out;
	}

	if (unlikely(__wled_using_current_map_id(me) == current_map_id))
		goto out;

	__wled_using_current_map_id(me) = current_map_id;
	max77819_wled_reset_cfg(me);

out:
	__unlock(me);
	return (ssize_t)count;
}

static DEVICE_ATTR(current_map_id, S_IWUSR | S_IRUGO,
		max77819_wled_cfg_current_map_id_show,
		max77819_wled_cfg_current_map_id_store);

static ssize_t max77819_wled_cfg_ramp_up_time_show(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	int rc;

	__lock(me);

	rc = (int)snprintf(buf, PAGE_SIZE, "%u msec\n",
			me->cfg.ramp_up_time_msec);

	__unlock(me);
	return (ssize_t)rc;
}

static ssize_t max77819_wled_cfg_ramp_up_time_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	u32 ramp_up_time_msec;

	__lock(me);

	ramp_up_time_msec = (u32)simple_strtoul(buf, NULL, 10);

	if (unlikely(me->cfg.ramp_up_time_msec == ramp_up_time_msec))
		goto out;

	me->cfg.ramp_up_time_msec = ramp_up_time_msec;
	max77819_wled_reset_cfg(me);

out:
	__unlock(me);
	return (ssize_t)count;
}

static DEVICE_ATTR(ramp_up_time, S_IWUSR | S_IRUGO,
		max77819_wled_cfg_ramp_up_time_show,
		max77819_wled_cfg_ramp_up_time_store);

static ssize_t max77819_wled_cfg_ramp_down_time_show(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	int rc;

	__lock(me);

	rc = (int)snprintf(buf, PAGE_SIZE, "%u msec\n",
			me->cfg.ramp_dn_time_msec);

	__unlock(me);
	return (ssize_t)rc;
}

static ssize_t max77819_wled_cfg_ramp_down_time_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	u32 ramp_dn_time_msec;

	__lock(me);

	ramp_dn_time_msec = (u32)simple_strtoul(buf, NULL, 10);

	if (unlikely(me->cfg.ramp_dn_time_msec == ramp_dn_time_msec))
		goto out;

	me->cfg.ramp_dn_time_msec = ramp_dn_time_msec;
	max77819_wled_reset_cfg(me);

out:
	__unlock(me);
	return (ssize_t)count;
}

static DEVICE_ATTR(ramp_down_time, S_IWUSR | S_IRUGO,
		max77819_wled_cfg_ramp_down_time_show,
		max77819_wled_cfg_ramp_down_time_store);

static ssize_t max77819_wled_cfg_pwm_input_use_show(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	int rc;

	__lock(me);

	rc = (int)snprintf(buf, PAGE_SIZE, "%u (%s)\n", me->cfg.pwm_input_use,
			__wled_pwm_input_use_desc(me->cfg.pwm_input_use));

	__unlock(me);
	return (ssize_t)rc;
}

static ssize_t max77819_wled_cfg_pwm_input_use_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	u32 pwm_input_use;

	__lock(me);

	pwm_input_use = (u32)simple_strtoul(buf, NULL, 10);

	if (unlikely(me->cfg.pwm_input_use == pwm_input_use))
		goto out;

	me->cfg.pwm_input_use = pwm_input_use;
	max77819_wled_reset_cfg(me);

out:
	__unlock(me);
	return (ssize_t)count;
}

static DEVICE_ATTR(pwm_input_use, S_IWUSR | S_IRUGO,
		max77819_wled_cfg_pwm_input_use_show,
		max77819_wled_cfg_pwm_input_use_store);

static ssize_t max77819_wled_cfg_pwm_id_show(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	int rc;

	__lock(me);

	rc = (int)snprintf(buf, PAGE_SIZE, "%u\n", me->cfg.pwm_id);

	__unlock(me);
	return (ssize_t)rc;
}

static ssize_t max77819_wled_cfg_pwm_id_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	u32 pwm_id;

	__lock(me);

	pwm_id = (u32)simple_strtoul(buf, NULL, 10);

	if (unlikely(me->cfg.pwm_id == pwm_id))
		goto out;

	me->cfg.pwm_id = pwm_id;
	max77819_wled_reset_cfg(me);

out:
	__unlock(me);
	return (ssize_t)count;
}

static DEVICE_ATTR(pwm_id, S_IWUSR | S_IRUGO,
		max77819_wled_cfg_pwm_id_show,
		max77819_wled_cfg_pwm_id_store);

static ssize_t max77819_wled_cfg_pwm_period_show(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	int rc;

	__lock(me);

	rc = (int)snprintf(buf, PAGE_SIZE, "%u nsec\n",
			me->cfg.pwm_period_nsec);

	__unlock(me);
	return (ssize_t)rc;
}

static ssize_t max77819_wled_cfg_pwm_period_store(struct device *dev,
		struct device_attribute *devattr, const char *buf, size_t count)
{
	struct max77819_wled *me = dev_get_drvdata(dev);
	u32 pwm_period_nsec;

	__lock(me);

	pwm_period_nsec = (u32)simple_strtoul(buf, NULL, 10);

	if (unlikely(me->cfg.pwm_period_nsec == pwm_period_nsec))
		goto out;

	me->cfg.pwm_period_nsec = pwm_period_nsec;
	max77819_wled_reset_cfg(me);

out:
	__unlock(me);
	return (ssize_t)count;
}

static DEVICE_ATTR(pwm_period, S_IWUSR | S_IRUGO,
		max77819_wled_cfg_pwm_period_show,
		max77819_wled_cfg_pwm_period_store);

static struct attribute *max77819_wled_attr[] = {
	&dev_attr_current_map_id.attr,
	&dev_attr_ramp_up_time.attr,
	&dev_attr_ramp_down_time.attr,
	&dev_attr_pwm_input_use.attr,
	&dev_attr_pwm_id.attr,
	&dev_attr_pwm_period.attr,
	NULL,
};

static const struct attribute_group max77819_wled_attr_group = {
	.attrs = max77819_wled_attr,
};

static void *max77819_wled_get_platdata(struct max77819_wled *me)
{
#ifdef CONFIG_OF
	struct device *dev = me->dev;
	struct device_node *np = dev->of_node;
	struct max77819_wled_platform_data *pdata;

	pdata = devm_kzalloc(dev, sizeof(*me), GFP_KERNEL);
	if (unlikely(!pdata)) {
		log_err("out of memory (%uB requested)\n", sizeof(*me));
		pdata = ERR_PTR(-ENOMEM);
		goto out;
	}

	pdata->ramp_up_time_msec = 0;
	of_property_read_u32(np, "ramp_up_time_msec",
			&pdata->ramp_up_time_msec);
	log_dbg("property:RAMP UP          %umsec\n", pdata->ramp_up_time_msec);

	pdata->ramp_dn_time_msec = 0;
	of_property_read_u32(np, "ramp_dn_time_msec",
			&pdata->ramp_dn_time_msec);
	log_dbg("property:RAMP DOWN        %umsec\n", pdata->ramp_dn_time_msec);

	pdata->pwm_input_use = MAX77819_WLED_PWM_INPUT_DISABLE;
	of_property_read_u32(np, "pwm_input_use", &pdata->pwm_input_use);
	log_dbg("property:WLED PWM INPUT   %s\n",
			__wled_pwm_input_use_desc(pdata->pwm_input_use));

	pdata->pwm_id = 0;
	of_property_read_u32(np, "pwm_id", &pdata->pwm_id);
	log_dbg("property:PWM ID           %u\n", pdata->pwm_id);

	pdata->pwm_period_nsec = 0;
	of_property_read_u32(np, "pwm_period_nsec", &pdata->pwm_period_nsec);
	log_dbg("property:PWM PERIOD       %unsec\n", pdata->pwm_period_nsec);

out:
	return pdata;
#else /* CONFIG_OF */
	return dev_get_platdata(me->dev) ?
		dev_get_platdata(me->dev) : ERR_PTR(-EINVAL);
#endif /* CONFIG_OF */
}

static __always_inline void max77819_wled_destroy(struct max77819_wled *me)
{
	struct device *dev = me->dev;

	hrtimer_cancel(&me->ramp_timer);

	if (likely(me->attr_grp))
		sysfs_remove_group(me->kobj, me->attr_grp);

	if (likely(me->bl_dev))
		backlight_device_unregister(me->bl_dev);

	if (likely(me->pwm_dev))
		pwm_free(me->pwm_dev);

#ifdef CONFIG_OF
	if (likely(me->pdata))
		devm_kfree(dev, me->pdata);
#endif /* CONFIG_OF */

	mutex_destroy(&me->lock);
	/* spin_lock_destroy(&me->irq_lock); */

	devm_kfree(dev, me);
}

static int max77819_bl_update_status(struct backlight_device *bl_dev)
{
	struct max77819_wled *me = bl_get_data(bl_dev);
	int brightness_new, rc = 0;

	__lock(me);

	if (unlikely(bl_dev->props.power    != FB_BLANK_UNBLANK ||
				bl_dev->props.fb_blank != FB_BLANK_UNBLANK ||
				bl_dev->props.state & BL_CORE_FBBLANK ||
				bl_dev->props.state & BL_CORE_SUSPENDED)) {
		log_vdbg("state is suspended or power down\n");
		brightness_new = 0;
	} else {
		brightness_new
			= max(0, min(WLED_MAX_BRIGHTNESS,
						bl_dev->props.brightness));
	}

	/* cancel ramp timer */
	hrtimer_cancel(&me->ramp_timer);

	/* convert brightness level (0..WLED_MAX_BRIGHTNESS) to
	 *         a code (index in mapping table)
	 */
	me->target_code = __wled_current_map_code(me, brightness_new);
	pr_info("%s: updating - brightness level %d (code %d)\n", __func__,
			brightness_new, me->target_code);
	/* log_vdbg("updating - brightness level %d (code %d)\n",
			brightness_new, me->target_code);
	*/

	if (unlikely(me->target_code == me->current_code))
		goto out;

	rc = me->target_code > me->current_code ?
		max77819_wled_start_ramp_up(me) :
		max77819_wled_start_ramp_down(me);

out:
	__unlock(me);
	return rc;
}

static int max77819_bl_get_brightness(struct backlight_device *bl_dev)
{
	struct max77819_wled *me = bl_get_data(bl_dev);
	int rc;

	__lock(me);

	rc = me->brightness;

	__unlock(me);
	return rc;
}

static const struct backlight_ops max77819_bl_ops = {
	.update_status  = max77819_bl_update_status,
	.get_brightness = max77819_bl_get_brightness,
};

#ifdef CONFIG_MACH_LGE
void max77819_lcd_backlight_set_level(int bl_level)
{
	struct max77819_wled *me;
	int ret = 0;

	if (max77819_bl_device == NULL) {
		pr_err("%s : max77819_bl is not registered\n", __func__);
		return;
	}

	me = bl_get_data(max77819_bl_device);

	/* Set the minimum brightness limit */
	if ((bl_level != 0) && (bl_level <= 150))
		bl_level = 150;

	me->bl_dev->props.brightness = bl_level;
	pr_info("%s : level is %d\n", __func__, me->brightness);

	ret = max77819_bl_update_status(me->bl_dev);
	if (ret)
		pr_err("%s DEBUG error set backlight", __func__);
}
#endif


#ifdef CONFIG_OF
static struct of_device_id max77819_bl_of_ids[] = {
	{ .compatible = "maxim,"MAX77819_WLED_NAME },
	{ },
};
MODULE_DEVICE_TABLE(of, max77819_bl_of_ids);
#endif /* CONFIG_OF */

static int max77819_bl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct max77819_dev *chip = dev_get_drvdata(dev->parent);
	struct max77819_wled *me;
	struct backlight_properties bl_props;
	int rc;

	pr_info("%s: started\n", __func__);

	me = devm_kzalloc(dev, sizeof(*me), GFP_KERNEL);
	if (unlikely(!me)) {
		log_err("out of memory (%uB requested)\n", sizeof(*me));
		return -ENOMEM;
	}

	dev_set_drvdata(dev, me);

	spin_lock_init(&me->irq_lock);
	mutex_init(&me->lock);
	me->io   = max77819_get_io(chip);
	me->dev  = dev;
	me->kobj = &dev->kobj;

	hrtimer_init(&me->ramp_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);

	me->pdata = max77819_wled_get_platdata(me);
	if (unlikely(IS_ERR(me->pdata))) {
		rc = PTR_ERR(me->pdata);
		me->pdata = NULL;
		log_err("failed to get platform data [%d]\n", rc);
		goto abort;
	}

	memcpy(&me->cfg, me->pdata, sizeof(me->cfg));

	/* brightness is initially MAX */
	me->brightness = WLED_MAX_BRIGHTNESS;

	max77819_wled_reset_cfg(me);

	memset(&bl_props, 0x00, sizeof(bl_props));
	bl_props.type           = BACKLIGHT_RAW;
	bl_props.max_brightness = WLED_MAX_BRIGHTNESS;
	bl_props.brightness = WLED_MAX_BRIGHTNESS;

	me->bl_dev = backlight_device_register(DRIVER_NAME, dev, me,
			&max77819_bl_ops, &bl_props);
#ifdef CONFIG_MACH_LGE
	lge_blk_set_level = max77819_lcd_backlight_set_level;
	max77819_bl_device = me->bl_dev;
#endif
	if (unlikely(IS_ERR(me->bl_dev))) {
		rc = PTR_ERR(me->bl_dev);
		me->bl_dev = NULL;
		log_err("failed to register backlight device [%d]\n", rc);
		goto abort;
	}

	/* Create max77819-wled sysfs attributes */
	me->attr_grp = &max77819_wled_attr_group;
	rc = sysfs_create_group(me->kobj, me->attr_grp);
	if (unlikely(IS_ERR_VALUE(rc))) {
		log_err("failed to create attribute group [%d]\n", rc);
		me->attr_grp = NULL;
		goto abort;
	}

	log_info("driver "DRIVER_VERSION" installed\n");
	pr_info("%s: finished\n", __func__);

	return 0;

abort:
	dev_set_drvdata(dev, NULL);
	max77819_wled_destroy(me);
	return rc;
}

/* temporary backlight off during power-off. */
static void max77819_bl_shutdown(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct max77819_wled *me = dev_get_drvdata(dev);
	int rc;

	pr_info("[lightmin][%s]\n", __func__);
	/* current value is set by 0x00 */
	rc = max77819_write(me->io, IWLED, 0x00);
	if (unlikely(IS_ERR_VALUE(rc))) {
		log_err("IWLED write error [%d]\n", rc);
	}
	/* WLED2EN and WLED1EN is set 0, Current source 1/2 disabled. */
	rc = max77819_masked_write(me->io, WLEDBSTCNTL1, WLEDBSTCNTL1_WLEDEN,
			(u8)FFS(WLEDBSTCNTL1_WLED2EN), 0x00);
	if (unlikely(IS_ERR_VALUE(rc)))
		log_err("WLEDBSTCNTL1 write error [%d]\n", rc);
}

static int max77819_bl_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct max77819_wled *me = dev_get_drvdata(dev);

	dev_set_drvdata(dev, NULL);
	max77819_wled_destroy(me);

	return 0;
}

#ifdef CONFIG_PM
static int max77819_bl_suspend(struct device *dev)
{
	return 0;
}

static int max77819_bl_resume(struct device *dev)
{
	return 0;
}
#endif /* CONFIG_PM_SLEEP */

static SIMPLE_DEV_PM_OPS(max77819_bl_pm, max77819_bl_suspend,
		max77819_bl_resume);

static struct platform_driver max77819_bl_driver = {
	.probe              = max77819_bl_probe,
	.remove             = max77819_bl_remove,
	.shutdown			= max77819_bl_shutdown,
	.driver = {
		.name       = DRIVER_NAME,
		.owner      = THIS_MODULE,
		.pm         = &max77819_bl_pm,
#ifdef CONFIG_OF
		.of_match_table = max77819_bl_of_ids,
#endif /* CONFIG_OF */
	},
};

static __init int max77819_bl_init(void)
{
	return platform_driver_register(&max77819_bl_driver);
}
module_init(max77819_bl_init);

static __exit void max77819_bl_exit(void)
{
	platform_driver_unregister(&max77819_bl_driver);
}
module_exit(max77819_bl_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MAX77819 WLED Driver");
MODULE_VERSION(DRIVER_VERSION);
