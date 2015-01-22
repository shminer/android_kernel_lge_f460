/*
 * LGE Smart charging.
 *
 * Copyright (c) 2013 LGE Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/fcntl.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/smart_charging.h>
#include <linux/power_supply.h>

#define sc_dbg(x, ...) pr_info("[Smart Charging]" x, ##__VA_ARGS__)

#define SC_SLOW_CHARGING	1
#define SC_NORMAL_CHARGING	2
#define SC_FAST_CHARGING	3
#define SC_ON			10
#define SC_OFF			11

#define CURRENT_CONTROL_MAX	4

struct charging_prop_entry
{
	bool step;
	int icl_ma;
	int chg_current1_ma;
	int chg_current2_ma;
	int step_voltage_mv;
};

#ifdef CONFIG_LGE_PM_CHARGING_BQ24296_CHARGER
static struct charging_prop_entry prop_tbl[CURRENT_CONTROL_MAX] = {
	{0, 0, 0, 0, 0},
	{0, 1800, 1000, 0, 0},
	{0, 1800, 1600, 0, 0},
	{0, 2000, 1800, 0, 0},
};
#endif

#define DEVICE_NAME	"smart_charging"
#define BUFF_SIZE		4
#define DEFAULT_CNT  2

static dev_t dev;

static struct cdev smart_charging_control_cdev;
struct class *smart_charging_control_class;

int check_current_control_type(int type) {
	struct power_supply *psy_ac = NULL;
	static union power_supply_propval sc_onoff = {0, };
	static union power_supply_propval sc_chg_current = {0, };
	static union power_supply_propval sc_force_update = {0, };
	static union power_supply_propval sc_limit_current = {0, };

	if (psy_ac == NULL) {
		psy_ac = power_supply_get_by_name("ac");
		if (psy_ac == NULL) {
			sc_dbg("failed to get ps bms\n");
			return -EINVAL;
		}
	}

	switch(type) {
		case SC_SLOW_CHARGING:
		case SC_NORMAL_CHARGING:
		case SC_FAST_CHARGING:
			sc_chg_current.intval = prop_tbl[type].chg_current1_ma;
			sc_limit_current.intval = prop_tbl[type].icl_ma;
			sc_force_update.intval = 1;
			psy_ac->set_event_property(psy_ac, POWER_SUPPLY_PROP_SMART_CHARGING_CHG_CURRENT, &sc_chg_current);
			psy_ac->set_event_property(psy_ac, POWER_SUPPLY_PROP_SMART_CHARGING_FORCE_UPDATE, &sc_force_update);
			break;
		case SC_ON:
			sc_onoff.intval = 1; //smart_charging_on
			sc_force_update.intval = 1;
			sc_chg_current.intval = prop_tbl[DEFAULT_CNT].chg_current1_ma;
			psy_ac->set_event_property(psy_ac, POWER_SUPPLY_PROP_SMART_CHARGING_ENABLE, &sc_onoff);
			psy_ac->set_event_property(psy_ac, POWER_SUPPLY_PROP_SMART_CHARGING_CHG_CURRENT, &sc_chg_current);
			psy_ac->set_event_property(psy_ac, POWER_SUPPLY_PROP_SMART_CHARGING_FORCE_UPDATE, &sc_force_update);
			break;
		case SC_OFF:
		default:
			sc_onoff.intval = 0; //smart_charging_off
			sc_force_update.intval = 0;
			sc_chg_current.intval = 0;
			sc_limit_current.intval = 0;
			psy_ac->set_event_property(psy_ac, POWER_SUPPLY_PROP_SMART_CHARGING_ENABLE, &sc_onoff);
			psy_ac->set_event_property(psy_ac, POWER_SUPPLY_PROP_SMART_CHARGING_CHG_CURRENT, &sc_chg_current);
			psy_ac->set_event_property(psy_ac, POWER_SUPPLY_PROP_SMART_CHARGING_FORCE_UPDATE, &sc_force_update);
			break;
	}
	sc_dbg("[smart_charging]type = %d aicl = %d current = %d\n", type, prop_tbl[type].icl_ma, prop_tbl[type].chg_current1_ma);
	return 1;
}
static int smart_charging_control_open(struct inode *inode, struct file *filp) {
	return 0;
}
static long smart_charging_control_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
	int ret = 0;
	switch(cmd) {
		case AMC_CHG_CONTROL:
			sc_dbg("[smart_charging]Type: %lu\n", arg);
			ret = check_current_control_type((int)arg);
			break;
		default:
			break;
	}
	return ret;
}

int smart_charging_control_release(struct inode *inode, struct file *filp) {
	return 0;
}

static const struct file_operations smart_charging_control_fops = {
	.open           = smart_charging_control_open,
	.release        = smart_charging_control_release,
	.unlocked_ioctl = smart_charging_control_ioctl,
};
static int smart_charging_control_init(void) {
	int error;
	error = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	if (error < 0) {
		sc_dbg("alloc_chrdev_region() failed\n");
		return error;
	}

	smart_charging_control_class = class_create(THIS_MODULE, DEVICE_NAME);
	cdev_init(&smart_charging_control_cdev, &smart_charging_control_fops);
	error = cdev_add(&smart_charging_control_cdev, dev, 1);
	if (error < 0) {
		sc_dbg("[smart_charging]cdev_add() failed\n");
		return error;
	}
	device_create(smart_charging_control_class, NULL, dev, NULL, DEVICE_NAME);
	sc_dbg("\n%s module initialized.\n", DEVICE_NAME);
	return 0;
}
static void smart_charging_control_exit(void) {
	unregister_chrdev_region(dev, 1);
	cdev_del(&smart_charging_control_cdev);
	device_destroy(smart_charging_control_class, smart_charging_control_cdev.dev);
	class_destroy(smart_charging_control_class);
	sc_dbg("\n%s module removed.\n", DEVICE_NAME);
}

module_init( smart_charging_control_init );
module_exit( smart_charging_control_exit );
MODULE_LICENSE("GPL");
