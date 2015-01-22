/* Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
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
#include <linux/module.h>
#include <linux/export.h>
#include "msm_led_flash.h"
#include <linux/debugfs.h>
#include <linux/qpnp/qpnp-adc.h>  /*                                                                 */

#define FLASH_NAME "qcom,led-flash"

#define CONFIG_MSMB_CAMERA_DEBUG
#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

#define LED_NUMS 2

#define REG_ENABLE			0x01
#define REG_IVFM			0x02
#define REG_TORCH_BR		0x05
#define REG_FLASH_BR		0x05
#define REG_FLASH_TOUT		0x04
#define REG_FLAG			0x08
#define REG_STROBE_SRC		0x06
#define REG_LED1_FLASH_BR 	0x06
#define REG_LED1_TORCH_BR 	0x07

#define MASK_ENABLE			0x03
#define MASK_TORCH_BR		0x70
#define MASK_FLASH_BR		0x0F
#define MASK_FLASH_TOUT		0x07
#define MASK_FLAG			0xFF
#define MASK_STROBE_SRC		0x80

enum led_mode {  // inductor current limit - 1.0A
	MODE_SHDN = 0x0,
	MODE_TORCH = 0x2,
	MODE_FLASH = 0x3,
};

/*  TOTAL FLASH Brightness Max
 *	min 93350uA, step 93750uA, max 1499600uA
 */
#define LM3646_TOTAL_FLASH_BRT_MIN 93350
#define LM3646_TOTAL_FLASH_BRT_STEP 93750
#define LM3646_TOTAL_FLASH_BRT_MAX 1499600
#define LM3646_TOTAL_FLASH_BRT_uA_TO_REG(a)	\
	((a) < LM3646_TOTAL_FLASH_BRT_MIN ? 0 :	\
	 ((((a) - LM3646_TOTAL_FLASH_BRT_MIN) / LM3646_TOTAL_FLASH_BRT_STEP)))

#define LM3646_TOTAL_FLASH_BRT_REG_TO_uA(reg)	\
	((reg) == 0 ? LM3646_TOTAL_FLASH_BRT_MIN : \
	 (((reg) * LM3646_TOTAL_FLASH_BRT_STEP) + LM3646_TOTAL_FLASH_BRT_MIN))

/*  TOTAL TORCH Brightness Max
 *	min 23040uA, step 23430uA, max 187100uA
 */
#define LM3646_TOTAL_TORCH_BRT_MIN 23040
#define LM3646_TOTAL_TORCH_BRT_STEP 23430
#define LM3646_TOTAL_TORCH_BRT_MAX 187100
#define LM3646_TOTAL_TORCH_BRT_uA_TO_REG(a)	\
	((a) < LM3646_TOTAL_TORCH_BRT_MIN ? 0 :	\
	 ((((a) - LM3646_TOTAL_TORCH_BRT_MIN) / LM3646_TOTAL_TORCH_BRT_STEP)))

#define LM3646_TOTAL_TORCH_BRT_REG_TO_uA(reg)	\
	((reg) == 0 ? LM3646_TOTAL_TORCH_BRT_MIN :	\
	 (((reg) * LM3646_TOTAL_TORCH_BRT_STEP) + LM3646_TOTAL_TORCH_BRT_MIN))

/*  LED1 FLASH Brightness
 *	min 23040uA, step 11718uA, max 1499600uA
 */
#define LM3646_LED1_FLASH_BRT_MIN 23040
#define LM3646_LED1_FLASH_BRT_STEP 11718
#define LM3646_LED1_FLASH_BRT_MAX 1499600
#define LM3646_LED1_FLASH_BRT_uA_TO_REG(a)	\
	((a) <= LM3646_LED1_FLASH_BRT_MIN ? 0 :	\
	 ((((a) - LM3646_LED1_FLASH_BRT_MIN) / LM3646_LED1_FLASH_BRT_STEP))+1)

#define LM3646_LED1_FLASH_BRT_REG_TO_uA(reg)	\
	((reg) == 0 ? 0 :	\
	((((reg) - 1) * LM3646_LED1_FLASH_BRT_STEP) + LM3646_LED1_FLASH_BRT_MIN))


/*  LED1 TORCH Brightness
 *	min 2530uA, step 1460uA, max 187100uA
 */
#define LM3646_LED1_TORCH_BRT_MIN 2530
#define LM3646_LED1_TORCH_BRT_STEP 1460
#define LM3646_LED1_TORCH_BRT_MAX 187100
#define LM3646_LED1_TORCH_BRT_uA_TO_REG(a)	\
	((a) <= LM3646_LED1_TORCH_BRT_MIN ? 0 :	\
	 ((((a) - LM3646_LED1_TORCH_BRT_MIN) / LM3646_LED1_TORCH_BRT_STEP))+1)

#define LM3646_LED1_TORCH_BRT_REG_TO_uA(reg)	\
	((reg) == 0 ? 0 :	\
	((((reg) - 1) * LM3646_LED1_TORCH_BRT_STEP) + LM3646_LED1_TORCH_BRT_MIN))

/*  FLASH TIMEOUT DURATION
 *	min 50ms, step 50ms, max 400ms
 */
#define LM3646_FLASH_TOUT_MIN 50
#define LM3646_FLASH_TOUT_STEP 50
#define LM3646_FLASH_TOUT_MAX 400
#define LM3646_FLASH_TOUT_ms_TO_REG(a)	\
	((a) <= LM3646_FLASH_TOUT_MIN ? 0 :	\
	 (((a) - LM3646_FLASH_TOUT_MIN) / LM3646_FLASH_TOUT_STEP))

/* struct lm3646_platform_data
 *
 * @flash_timeout: flash timeout
 * @led1_flash_brt: led1 flash mode brightness, uA
 * @led1_torch_brt: led1 torch mode brightness, uA
 */
struct lm3646_platform_data {
	uint32_t flash_timeout;
	uint32_t led1_flash_brt;
	uint32_t led1_torch_brt;
};

static struct msm_led_flash_ctrl_t fctrl;
static struct i2c_driver lm3646_i2c_driver;

static struct msm_camera_i2c_reg_array lm3646_init_array[] = {
	{REG_ENABLE, MODE_SHDN},
	{REG_FLASH_TOUT, 0x47}, //                                                                
	{REG_IVFM, 0xA4},
};

static struct msm_camera_i2c_reg_array lm3646_off_array[] = {
	{REG_ENABLE, MODE_SHDN},
};

static struct msm_camera_i2c_reg_array lm3646_release_array[] = {
	{REG_ENABLE, MODE_SHDN},
};

static struct msm_camera_i2c_reg_array lm3646_low_array[] = {
	{REG_TORCH_BR, 0},
	{REG_LED1_TORCH_BR, 0},
	{REG_ENABLE, MODE_TORCH},
};

static struct msm_camera_i2c_reg_array lm3646_high_array[] = {
	{REG_FLASH_BR, 0},
	{REG_LED1_FLASH_BR, 0},
	{REG_ENABLE, MODE_FLASH},
};

static uint32_t lm3646_flash_current[LED_NUMS];

static void __exit msm_flash_lm3646_i2c_remove(void)
{
	i2c_del_driver(&lm3646_i2c_driver);
	return;
}

static const struct of_device_id lm3646_trigger_dt_match[] = {
	{.compatible = "qcom,led-flash", .data = &fctrl},
	{}
};

MODULE_DEVICE_TABLE(of, lm3646_trigger_dt_match);

static const struct i2c_device_id flash_i2c_id[] = {
	{"qcom,led-flash", (kernel_ulong_t)&fctrl},
	{ }
};

static const struct i2c_device_id lm3646_i2c_id[] = {
	{FLASH_NAME, (kernel_ulong_t)&fctrl},
	{ }
};

/*                                                                             */
static int lm3646_get_batt_temp(struct msm_led_flash_ctrl_t *fctrl)
{
	struct qpnp_vadc_chip *vadc_dev;
	struct qpnp_vadc_result results;
	int rc, temp;

	vadc_dev = qpnp_get_vadc(&fctrl->pdev->dev, "lm3646");
	rc = qpnp_vadc_read(vadc_dev, LR_MUX6_AMUX_THM3, &results);
	if (!rc)
		temp = (int)results.physical;
	else
		temp = 200;

	CDBG("%s : current thermal %d\n", __func__, temp);

	return temp;
}

static int msm_flash_lm3646_led_init(struct msm_led_flash_ctrl_t *fctrl)
{
	int rc, temp;

	temp = lm3646_get_batt_temp(fctrl);
	if (temp <= -30) {
		lm3646_init_array[0].reg_data = MODE_SHDN;
		lm3646_init_array[2].reg_data = 0xA4;  // UVLO, IVFM enable
		lm3646_off_array[0].reg_data = MODE_SHDN;
		lm3646_low_array[2].reg_data = MODE_TORCH;
		lm3646_high_array[2].reg_data = MODE_FLASH;
		CDBG("%s : Inductor Current Limit 1.0A \n", __func__);
	} else {
		lm3646_init_array[0].reg_data = MODE_SHDN | 0x80;
		lm3646_init_array[2].reg_data = 0x24;  // UVLO disalbe, IVFM enable
		lm3646_off_array[0].reg_data = MODE_SHDN | 0x80;
		lm3646_low_array[2].reg_data = MODE_TORCH | 0x80;
		lm3646_high_array[2].reg_data = MODE_FLASH | 0x80;
		CDBG("%s : Inductor Current Limit 2.2A \n", __func__);
	}

	rc = msm_flash_led_init(fctrl);

	return rc;
}
/*                                                                              */

static int msm_flash_lm3646_low_config(struct msm_led_flash_ctrl_t *fctrl, void *data)
{
	struct msm_camera_led_cfg_t *cfg = (struct msm_camera_led_cfg_t *)data;
	uint32_t total_uA = (cfg->flash_current[0] + cfg->flash_current[1]) * 1000;
	uint32_t led1_uA = cfg->flash_current[0] * 1000;
	uint8_t total_reg, led1_reg;

	CDBG("%s : input led1[%d], led2[%d]\n", __func__, led1_uA, cfg->flash_current[1] * 1000);

	if (total_uA > LM3646_TOTAL_TORCH_BRT_MAX)
		total_uA = LM3646_TOTAL_TORCH_BRT_MAX;

	total_reg = LM3646_TOTAL_TORCH_BRT_uA_TO_REG(total_uA);
	lm3646_low_array[0].reg_data = total_reg << 4;

	if (led1_uA > LM3646_LED1_TORCH_BRT_MAX)
		led1_uA = LM3646_LED1_TORCH_BRT_MAX;

	led1_reg = LM3646_LED1_TORCH_BRT_uA_TO_REG(led1_uA);
	lm3646_low_array[1].reg_data = led1_reg;

	CDBG("%s : calculated total_reg[%X] led1_reg[%X]\n", __func__, \
		total_reg, led1_reg);
	CDBG("%s : calculated total_uA[%d] led1_uA[%d], led2_uA[%d]\n", __func__, \
		LM3646_TOTAL_TORCH_BRT_REG_TO_uA(total_reg), \
		LM3646_LED1_TORCH_BRT_REG_TO_uA(led1_reg), \
		LM3646_TOTAL_TORCH_BRT_REG_TO_uA(total_reg) - \
		LM3646_LED1_TORCH_BRT_REG_TO_uA(led1_reg));

	return 0;
}

static int msm_flash_lm3646_high_config(struct msm_led_flash_ctrl_t *fctrl, void *data)
{
	struct msm_camera_led_cfg_t *cfg = (struct msm_camera_led_cfg_t *)data;
	uint32_t total_uA = (cfg->flash_current[0] + cfg->flash_current[1]) * 1000;
	uint32_t led1_uA = cfg->flash_current[0] * 1000;
	uint8_t total_reg, led1_reg;

	CDBG("%s : input led1[%d], led2[%d]\n", __func__, led1_uA, cfg->flash_current[1] * 1000);

	if (total_uA > LM3646_TOTAL_FLASH_BRT_MAX)
		total_uA = LM3646_TOTAL_FLASH_BRT_MAX;

	total_reg = LM3646_TOTAL_FLASH_BRT_uA_TO_REG(total_uA);
	lm3646_high_array[0].reg_data = total_reg;

	if (led1_uA > LM3646_LED1_FLASH_BRT_MAX)
		led1_uA = LM3646_LED1_FLASH_BRT_MAX;

	led1_reg = LM3646_LED1_FLASH_BRT_uA_TO_REG(led1_uA);
	lm3646_high_array[1].reg_data = led1_reg;

	CDBG("%s : calculated total_reg[%X] led1_reg[%X]\n", __func__, \
		total_reg, led1_reg);
	CDBG("%s : calculated total_uA[%d] led1_uA[%d], led2_uA[%d]\n", __func__, \
		LM3646_TOTAL_FLASH_BRT_REG_TO_uA(total_reg), \
		LM3646_LED1_FLASH_BRT_REG_TO_uA(led1_reg), \
		LM3646_TOTAL_FLASH_BRT_REG_TO_uA(total_reg) - \
		LM3646_LED1_FLASH_BRT_REG_TO_uA(led1_reg));

	return 0;
}

#ifdef CONFIG_DEBUG_FS
static int msm_flash_lm3646_debug_enable(void *data, u64 val)
{
	if (val == 0)
		msm_flash_led_off(&fctrl);
	else if (val == 1)
		msm_flash_led_low(&fctrl);
	else
		msm_flash_led_high(&fctrl);

	return 0;
}

static int msm_flash_lm3646_debug_flash_brt(void *data, u64 val)
{
	lm3646_high_array[0].reg_data = (uint8_t)val;
	return 0;
}

static int msm_flash_lm3646_debug_torch_brt(void *data, u64 val)
{
	lm3646_low_array[0].reg_data = ((uint8_t)val) << 4;
	return 0;
}

static int msm_flash_lm3646_debug_flash_led1_brt(void *data, u64 val)
{
	lm3646_high_array[1].reg_data = (uint8_t)val;
	return 0;
}

static int msm_flash_lm3646_debug_torch_led1_brt(void *data, u64 val)
{
	lm3646_low_array[1].reg_data = (uint8_t)val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(dbg_enable_fops, 	NULL, msm_flash_lm3646_debug_enable, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(flash_brt_fops, 	NULL, msm_flash_lm3646_debug_flash_brt, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(torch_brt_fops, 	NULL, msm_flash_lm3646_debug_torch_brt, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(flash_led1_brt_fops, 	NULL, msm_flash_lm3646_debug_flash_led1_brt, "%llu\n");
DEFINE_SIMPLE_ATTRIBUTE(torch_led1_brt_fops, 	NULL, msm_flash_lm3646_debug_torch_led1_brt, "%llu\n");
#endif

static int msm_flash_lm3646_debug(void)
{
#ifdef CONFIG_DEBUG_FS
	struct dentry *dir_dentry, *dentry;

	dir_dentry = debugfs_create_dir("lm3646", NULL);
	if (!dir_dentry) {
		pr_err("Failed to create the debugfs lm3646 dir");
		return 0;
	}
	dentry = debugfs_create_file("enable", S_IRUGO, dir_dentry, (void *)&fctrl, &dbg_enable_fops);
	if (!dentry) {
		pr_err("Failed to create the debugfs enable file");
		return 0;
	}
	dentry = debugfs_create_file("flash_brt", S_IRUGO, dir_dentry, (void *)&fctrl, &flash_brt_fops);
	if (!dentry) {
		pr_err("Failed to create the debugfs flash_brt file");
		return 0;
	}
	dentry = debugfs_create_file("torch_brt", S_IRUGO, dir_dentry, (void *)&fctrl, &torch_brt_fops);
	if (!dentry) {
		pr_err("Failed to create the debugfs flash_brt file");
		return 0;
	}
	dentry = debugfs_create_file("flash_led1_brt", S_IRUGO, dir_dentry, (void *)&fctrl, &flash_led1_brt_fops);
	if (!dentry) {
		pr_err("Failed to create the debugfs flash_brt file");
		return 0;
	}
	dentry = debugfs_create_file("torch_led1_brt", S_IRUGO, dir_dentry, (void *)&fctrl, &torch_led1_brt_fops);
	if (!dentry) {
		pr_err("Failed to create the debugfs flash_brt file");
		return 0;
	}
#endif
	return 0;
}

static int msm_flash_lm3646_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	if (!id) {
		pr_err("msm_flash_lm3646_i2c_probe: id is NULL");
		id = lm3646_i2c_id;
	}
	pr_err("msm_flash_lm3646_i2c_probe\n");
	return msm_flash_i2c_probe(client, id);
}

static struct i2c_driver lm3646_i2c_driver = {
	.id_table = lm3646_i2c_id,
	.probe  = msm_flash_lm3646_i2c_probe,
	.remove = __exit_p(msm_flash_lm3646_i2c_remove),
	.driver = {
		.name = FLASH_NAME,
		.owner = THIS_MODULE,
		.of_match_table = lm3646_trigger_dt_match,
	},
};

static int msm_flash_lm3646_platform_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	match = of_match_device(lm3646_trigger_dt_match, &pdev->dev);
	if (!match)
		return -EFAULT;
#ifdef CONFIG_MSMB_CAMERA_DEBUG
	msm_flash_lm3646_debug();
#endif
	return msm_flash_probe(pdev, match->data);
}

static struct platform_driver lm3646_platform_driver = {
	.probe = msm_flash_lm3646_platform_probe,
	.driver = {
		.name = "qcom,led-flash",
		.owner = THIS_MODULE,
		.of_match_table = lm3646_trigger_dt_match,
	},
};

static int __init msm_flash_lm3646_init_module(void)
{
	int32_t rc = 0;
	rc = platform_driver_register(&lm3646_platform_driver);
	if (!rc)
		return rc;
	pr_debug("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&lm3646_i2c_driver);
}

static void __exit msm_flash_lm3646_exit_module(void)
{
	if (fctrl.pdev)
		platform_driver_unregister(&lm3646_platform_driver);
	else
		i2c_del_driver(&lm3646_i2c_driver);
}

static struct msm_camera_i2c_client lm3646_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static struct msm_camera_i2c_reg_setting lm3646_init_setting = {
	.reg_setting = lm3646_init_array,
	.size = ARRAY_SIZE(lm3646_init_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_camera_i2c_reg_setting lm3646_off_setting = {
	.reg_setting = lm3646_off_array,
	.size = ARRAY_SIZE(lm3646_off_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_camera_i2c_reg_setting lm3646_release_setting = {
	.reg_setting = lm3646_release_array,
	.size = ARRAY_SIZE(lm3646_release_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_camera_i2c_reg_setting lm3646_low_setting = {
	.reg_setting = lm3646_low_array,
	.size = ARRAY_SIZE(lm3646_low_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_camera_i2c_reg_setting lm3646_high_setting = {
	.reg_setting = lm3646_high_array,
	.size = ARRAY_SIZE(lm3646_high_array),
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.delay = 0,
};

static struct msm_led_flash_reg_t lm3646_regs = {
	.init_setting = &lm3646_init_setting,
	.off_setting = &lm3646_off_setting,
	.low_setting = &lm3646_low_setting,
	.high_setting = &lm3646_high_setting,
	.release_setting = &lm3646_release_setting,
};

static struct msm_flash_fn_t lm3646_func_tbl = {
	.flash_get_subdev_id = msm_led_i2c_trigger_get_subdev_id,
	.flash_led_config = msm_led_i2c_trigger_config,
	.flash_led_init = msm_flash_lm3646_led_init, /*                                                                              */
	.flash_led_release = msm_flash_led_release,
	.flash_led_off = msm_flash_led_off,
	.flash_led_low = msm_flash_led_low,
	.flash_led_high = msm_flash_led_high,
	.flash_led_low_config = msm_flash_lm3646_low_config,
	.flash_led_high_config = msm_flash_lm3646_high_config,
};

static struct msm_led_flash_ctrl_t fctrl = {
	.flash_i2c_client = &lm3646_i2c_client,
	.reg_setting = &lm3646_regs,
	.func_tbl = &lm3646_func_tbl,
	.data = lm3646_flash_current,
};

/*subsys_initcall(msm_flash_i2c_add_driver);*/
module_init(msm_flash_lm3646_init_module);
module_exit(msm_flash_lm3646_exit_module);
MODULE_DESCRIPTION("lm3646 FLASH");
MODULE_LICENSE("GPL v2");

