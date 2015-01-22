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

#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <soc/qcom/socinfo.h>
#include <mach/board_lge.h>

static struct gpiomux_setting ap2mdm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting mdm2ap_errfatal_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting mdm2ap_pblrdy = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};


static struct gpiomux_setting ap2mdm_soft_reset_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting ap2mdm_wakeup = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gpio_epm_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

/*                                                       */
#ifdef CONFIG_LGE_NFC_PN547_C2
static struct gpiomux_setting nfc_pn547_sda_cfg = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting nfc_pn547_scl_cfg = {
	.func = GPIOMUX_FUNC_3,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting nfc_pn547_ven_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting nfc_pn547_irq_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting nfc_pn547_mode_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config msm_nfc_configs[] __initdata = {
	{
		/* I2C SDA */
		.gpio      = 41,
		.settings = {
			[GPIOMUX_ACTIVE]    = &nfc_pn547_sda_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_sda_cfg,
		},
	},
	{
		/* I2C SCL */
		.gpio      = 42,
		.settings = {
			[GPIOMUX_ACTIVE]    = &nfc_pn547_scl_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_scl_cfg,
		},
	},
	{
		/* VEN */
		.gpio      = 26,
		.settings = {
			[GPIOMUX_ACTIVE]    = &nfc_pn547_ven_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_ven_cfg,
		},
	},
	{
		/* IRQ */
		.gpio      = 58,
		.settings = {
			[GPIOMUX_ACTIVE]    = &nfc_pn547_irq_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_irq_cfg,
		},
	},
	{
		/* MODE *//* WAKE */
		.gpio      = 27,
		.settings = {
			[GPIOMUX_ACTIVE]    = &nfc_pn547_mode_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_pn547_mode_cfg,
		},
	},
};
#endif
/*                                                       */

static struct msm_gpiomux_config mdm_configs[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 110,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* MDM2AP_STATUS */
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 111,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
	/* AP2MDM_ERRFATAL */
	{
		.gpio = 112,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* AP2MDM_SOFT_RESET, aka AP2MDM_PON_RESET_N */
	{
		.gpio = 128,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_soft_reset_cfg,
		}
	},
	/* AP2MDM_WAKEUP */
	{
		.gpio = 108,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_wakeup,
		}
	},
	/* MDM2AP_PBL_READY*/
	{
		.gpio = 113,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_pblrdy,
		}
	},
};

static struct gpiomux_setting gpio_i2c_config = {
	.func = GPIOMUX_FUNC_3,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting led_en_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting led_en_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting synaptics_reset_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
	.dir  = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting synaptics_reset_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting synaptics_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting synaptics_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting synaptics_i2c_act_cfg = {
	.func = GPIOMUX_FUNC_3,
	.drv  = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting synaptics_i2c_sus_cfg = {
	.func = GPIOMUX_FUNC_3,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting hall_ic_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting gpio_uart_config = {
	.func = GPIOMUX_FUNC_2,
	.drv  = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting haptic_en_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting haptic_en_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting motor_pwm_act_cfg = {
	.func = GPIOMUX_FUNC_5,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting motor_pwm_sus_cfg = {
	.func = GPIOMUX_FUNC_5,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm_led_configs[] __initdata = {
	{
		.gpio = 126,
		.settings = {
			[GPIOMUX_ACTIVE] = &led_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &led_en_sus_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_synaptics_configs[] __initdata = {
	{
		.gpio = 143,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_int_sus_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_ACTIVE] = &synaptics_reset_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_reset_sus_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_hall_ic_configs_rev_a[] __initdata = {
	{
		.gpio = 78,
		.settings = {
			[GPIOMUX_ACTIVE] = &hall_ic_act_cfg,
		},
	},
	{
		.gpio = 123,
		.settings = {
			[GPIOMUX_ACTIVE] = &hall_ic_act_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_hall_ic_configs_rev_g[] __initdata = {
	{
		.gpio = 78,
		.settings = {
			[GPIOMUX_ACTIVE] = &hall_ic_act_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_vibrator_configs_rev_g[] __initdata = {
	{
		.gpio = 48,
		.settings = {
			[GPIOMUX_ACTIVE] = &haptic_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &haptic_en_sus_cfg,
		},
	},
	{
		.gpio = 97,
		.settings = {
			[GPIOMUX_ACTIVE] = &motor_pwm_act_cfg,
			[GPIOMUX_SUSPENDED] = &motor_pwm_sus_cfg,
		},
	},
};

static struct gpiomux_setting gpio_spi_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gpio_uart_gps_liquid_config = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};

#ifdef CONFIG_LGE_IRRC
static struct gpiomux_setting gpio_uart_irrc_config = {
	.func = GPIOMUX_FUNC_3,
	.drv  = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};
#else
static struct gpiomux_setting gpio_uart_gps_cdp_config = {
	.func = GPIOMUX_FUNC_3,
	.drv  = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

static struct msm_gpiomux_config msm_blsp_configs[] __initdata = {
	{
		.gpio      = 0,		/* BLSP1 QUP1 SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 1,		/* BLSP1 QUP1 SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 3,		/* BLSP1 QUP1 SPI_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 10,		/* BLSP1 QUP3 I2C_SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 11,		/* BLSP1 QUP3 I2C_SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 29,		/* BLSP1 QUP4 I2C_SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 30,		/* BLSP1 QUP4 I2C_SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 41,		/* BLSP1 QUP5 I2C_SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 42,		/* BLSP1 QUP5 ISC_SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 51,		/* BLSP2 UART1 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 52,		/* BLSP2 UART1 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 61,		/* BLSP12 QUP4 I2C_SDA */
		.settings = {
			[GPIOMUX_ACTIVE]    = &synaptics_i2c_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_i2c_sus_cfg,
		},
	},
	{
		.gpio      = 62,		/* BLSP2 QUP4 I2C_SCL */
		.settings = {
			[GPIOMUX_ACTIVE]    = &synaptics_i2c_act_cfg,
			[GPIOMUX_SUSPENDED] = &synaptics_i2c_sus_cfg,
		},
	},
	{
		.gpio      = 116,		/* BLSP1 QUP1 SPI_CS1 */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	}
};

static struct msm_gpiomux_config msm_sbc_blsp_configs[] __initdata = {
	{
		.gpio      = 2,		/* BLSP1 QUP0 I2C_SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 3,		/* BLSP1 QUP0 I2C_SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 4,		/* BLSP1 QUP1 SPI_DATA_MOSI */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 5,		/* BLSP1 QUP1 SPI_DATA_MISO */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 6,		/* BLSP1 QUP1 SPI_CS_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 7,		/* BLSP1 QUP1 SPI_CLK */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_spi_config,
		},
	},
	{
		.gpio      = 10,		/* BLSP1 QUP2 I2C_SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 11,		/* BLSP1 QUP2 I2C_SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 49,		/* BLSP2 QUP5 I2C_SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 50,		/* BLSP2 QUP5 I2C_SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 51,		/* BLSP2 UART1 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 52,		/* BLSP2 UART1 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_config,
		},
	},
	{
		.gpio      = 53,		/* BLSP2 QUP1 I2C_SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 54,		/* BLSP2 QUP1 I2C_SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},
	{
		.gpio      = 61,		/* BLSP2 QUP3 I2C_SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	},

	{
		.gpio      = 62,		/* BLSP2 QUP3 I2C_SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_i2c_config,
		},
	}
};

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
static struct gpiomux_setting gpio_eth_config = {
	.pull = GPIOMUX_PULL_UP,
	.drv = GPIOMUX_DRV_2MA,
	.func = GPIOMUX_FUNC_GPIO,
};

static struct msm_gpiomux_config msm_eth_configs[] __initdata = {
	{
		.gpio = 60,			/* SPI IRQ */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
	{
		.gpio = 117,		/* CS */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_eth_config,
		}
	},
};
#endif

/* QCA1530 uses differnt GPIOs based on platform, hence these settings */
static struct msm_gpiomux_config msm_blsp2_uart5_configs[] __initdata = {
	{
		.gpio      = 112,		/* BLSP2 UART5 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_gps_liquid_config,
		},
	},
	{
		.gpio      = 113,		/* BLSP2 UART5 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_gps_liquid_config,
		},
	},
};

#ifdef CONFIG_LGE_IRRC
static struct msm_gpiomux_config msm_blsp2_uart1_configs[] __initdata = {
	{
		.gpio      = 130,		/* BLSP2 UART1 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_irrc_config,
		},
	},
	{
		.gpio      = 131,		/* BLSP2 UART1 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_irrc_config,
		},
	},
};
#else
static struct msm_gpiomux_config msm_blsp2_uart1_configs[] __initdata = {
	{
		.gpio      = 130,		/* BLSP2 UART1 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_gps_cdp_config,
		},
	},
	{
		.gpio      = 131,		/* BLSP2 UART1 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_uart_gps_cdp_config,
		},
	},
};
#endif

static struct gpiomux_setting general_led_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting general_led_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

/*General led configuration for SBC 8084 platform*/
static struct msm_gpiomux_config apq8084_general_led_configs[] = {
	{
		.gpio = 83,
		.settings = {
			[GPIOMUX_ACTIVE]    = &general_led_cfg,
			[GPIOMUX_SUSPENDED] = &general_led_sus_cfg,
		},
	},
};

static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_active_mux_lpm_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting hdmi_active_mux_en_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting hdmi_active_mux_sel_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config msm_hdmi_nomux_configs[] __initdata = {
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 33,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
};

static struct msm_gpiomux_config msm_hdmi_configs[] __initdata = {
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 33,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_mux_lpm_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 83,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_mux_en_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_mux_sel_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
};

static struct gpiomux_setting hsic_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hsic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting hsic_wakeup_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting hsic_wakeup_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting hsic_ap2mdm_chlrdy_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting hsic_ap2mdm_chlrdy_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config apq8084_hsic_configs[] = {
	{
		.gpio = 134,               /*HSIC_STROBE */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 135,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
	{
		.gpio = 107,              /* wake up */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_wakeup_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_wakeup_sus_cfg,
		},
	},
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_ap2mdm_chlrdy_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_ap2mdm_chlrdy_sus_cfg,
		}
	},
	{
		.gpio = 108,     /* ap2mdm_wakeup is used as resume gpio */
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_wakeup,
		}
	},
};

static struct gpiomux_setting lcd_en_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_HIGH,
};

static struct gpiomux_setting lcd_en_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm_lcd_configs[] __initdata = {
	{
		.gpio = 96,			/* LCD RESET */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_en_sus_cfg,
		},
	},
	{
		.gpio = 86,			/* BKLT ENABLE */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_en_sus_cfg,
		},
	},
#ifdef CONFIG_MFD_TPS65132
	{
		.gpio = 136,		/* DSV NEGATIVE ENABLE*/
		.settings = {
			[GPIOMUX_ACTIVE]	= &lcd_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_en_sus_cfg,
		},
	},
#endif
	{
		.gpio = 137,		/* DSV POSITIVE ENABLE */
		.settings = {
			[GPIOMUX_ACTIVE]    = &lcd_en_act_cfg,
			[GPIOMUX_SUSPENDED] = &lcd_en_sus_cfg,
		},
	},
};

static struct gpiomux_setting wlan_wake_msm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting pcie0_wake_n_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
	.dir = GPIOMUX_IN,
};


static struct msm_gpiomux_config wlan_wake_msm_configs[] __initdata = {
	{
		.gpio = 121,			/* interrupt pin for WLAN Wake MSM , do not use now*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &wlan_wake_msm_cfg,
			[GPIOMUX_SUSPENDED] = &wlan_wake_msm_cfg,
		},
	},
};

static struct msm_gpiomux_config pcie0_wake_n_configs[] __initdata = {
	{
		.gpio = 69,			/* interrupt pin for WLAN Wake MSM , use now*/
		.settings = {
			[GPIOMUX_ACTIVE]    = &pcie0_wake_n_cfg,
			[GPIOMUX_SUSPENDED] = &pcie0_wake_n_cfg,
		},
	},
};


static struct gpiomux_setting sd_card_det_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting sd_card_det_sleep_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config sd_card_det[] __initdata = {
	{
		.gpio = 122,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sd_card_det_active_config,
			[GPIOMUX_SUSPENDED] = &sd_card_det_sleep_config,
		},
	},
};

static struct gpiomux_setting eth_pwr_sleep_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct msm_gpiomux_config eth_pwr[] = {
	{
		.gpio = 39,
		.settings = {
			[GPIOMUX_SUSPENDED] = &eth_pwr_sleep_config,
		},
	},
};

static struct gpiomux_setting gpio_suspend_config[] = {
	{
		.func = GPIOMUX_FUNC_GPIO,  /* IN-NP */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
	{
		.func = GPIOMUX_FUNC_GPIO,  /* O-LOW */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_LOW,
	},
};

#ifdef CONFIG_SND_SOC_CS35L32
static struct gpiomux_setting qua_mi2s_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting qua_mi2s_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config msm8974_qua_mi2s_configs[] __initdata = {
	{
		.gpio = 91,
		.settings = {
			[GPIOMUX_SUSPENDED] = &qua_mi2s_sus_cfg,
			[GPIOMUX_ACTIVE] = &qua_mi2s_act_cfg,
		},
	},
	{
		.gpio = 92,
		.settings = {
			[GPIOMUX_SUSPENDED] = &qua_mi2s_sus_cfg,
			[GPIOMUX_ACTIVE] = &qua_mi2s_act_cfg,
		},
	},
	{
		.gpio = 93,
		.settings = {
			[GPIOMUX_SUSPENDED] = &qua_mi2s_sus_cfg,
			[GPIOMUX_ACTIVE] = &qua_mi2s_act_cfg,
		},
	},
	{
		.gpio = 94,
		.settings = {
			[GPIOMUX_SUSPENDED] = &qua_mi2s_sus_cfg,
			[GPIOMUX_ACTIVE] = &qua_mi2s_act_cfg,
		},
	},
};
#endif

static struct gpiomux_setting cam_settings[] = {
	{
		.func = GPIOMUX_FUNC_1, /*active 1*/ /* 0 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},

	{
		.func = GPIOMUX_FUNC_1, /*suspend*/ /* 1 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},

	{
		.func = GPIOMUX_FUNC_1, /*i2c suspend*/ /* 2 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*active 0*/ /* 3 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
		.dir = GPIOMUX_OUT_HIGH,
	},

	{
		.func = GPIOMUX_FUNC_GPIO, /*suspend 0*/ /* 4 */
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
};

static struct msm_gpiomux_config msm_sensor_configs[] __initdata = {
	{
		.gpio = 15, /* CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 17, /* CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 19, /* CCI_I2C_SDA0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[2],
		},
	},
	{
		.gpio = 20, /* CCI_I2C_SCL0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[2],
		},
	},
	{
		.gpio = 21, /* CCI_I2C_SDA1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[2],
		},
	},
	{
		.gpio = 22, /* CCI_I2C_SCL1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[2],
		},
	},
	{
		.gpio = 23, /* FLASH_LED_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 25, /* CAM1_RESET_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 36,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 37,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 38,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 39,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 57,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
	{
		.gpio = 120,
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &cam_settings[4],
		},
	},
};

static struct msm_gpiomux_config msm_sbc_sensor_configs[] __initdata = {
	{
		.gpio = 15, /* CSI0 (CAM1) CAM_MCLK0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 16, /* CSI1 (CAM 3D L) CAM_MCLK1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 17, /* CSI2 (CAM2) CAM_MCLK2 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 18, /* CSI1 (CAM 3D R) CAM_MCLK3 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &cam_settings[1],
		},
	},
	{
		.gpio = 19, /* (CAM1, CAM2 and CAM 3D L) CCI_I2C_SDA0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 20, /* (CAM1, CAM2 and CAM 3D L) CCI_I2C_SCL0 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 21, /* (CAM 3D R) CCI_I2C_SDA1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 22, /* (CAM 3D L) CCI_I2C_SCL1 */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[0],
		},
	},
	{
		.gpio = 23, /* (CAM1 CCI_TIMER0) FLASH_LED_EN */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 24, /* (CAM1 CCI_TIMER1) FLASH_LED_NOW */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[0],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 25, /* CAM2_RESET_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 35, /* CAM1_STANDBY_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 36, /* CAM1_RST_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 78, /* CAM2_STANDBY  */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
	{
		.gpio = 118, /* (CAM 3D L&R) CAM_3D_STANDBY  */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},

	{
		.gpio = 120, /* (CAM 3D L&R) CAM_3D_RESET */
		.settings = {
			[GPIOMUX_ACTIVE]    = &cam_settings[3],
			[GPIOMUX_SUSPENDED] = &gpio_suspend_config[1],
		},
	},
};

static struct gpiomux_setting gpio_qca1530_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

#if defined(CONFIG_SWITCH_MAX1462X) || defined(CONFIG_INPUT_MAX14688)
static struct gpiomux_setting headset_active_cfg_gpio9 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir  = GPIOMUX_IN,
};

static struct gpiomux_setting headset_active_cfg_gpio8 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir  = GPIOMUX_IN,
};

static struct msm_gpiomux_config headset_configs[]  = {
	{
		.gpio = 8,
		.settings = {
			[GPIOMUX_ACTIVE]    = &headset_active_cfg_gpio8,
		},
	},

	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_ACTIVE]    = &headset_active_cfg_gpio9,
		},
	},
};
#endif

#ifdef CONFIG_SND_SOC_TPA2015D
static struct gpiomux_setting tpa2015d_extamp_cfg_gpio91 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_OUT_LOW,
};

static struct msm_gpiomux_config tpa2015d_extamp_configs[] = {
	{
		.gpio = 91,
		.settings = {
			[GPIOMUX_ACTIVE] = &tpa2015d_extamp_cfg_gpio91,
		},
	},
};
#endif


static struct gpiomux_setting gpio_qca1530_config_mpp7 = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting gpio_pcie_clkreq_config[] = {
	{
		.func = GPIOMUX_FUNC_2,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
	},
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_UP,
	},
};

static struct msm_gpiomux_config msm_pcie_configs[] __initdata = {
	{
		.gpio = 68,    /* PCIE0_CLKREQ_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_pcie_clkreq_config[0],
		},
	},
	{
		.gpio = 141,    /* PCIE1_CLKREQ_N */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_pcie_clkreq_config[1],
		},
	},
};

static struct msm_gpiomux_config msm_qca1530_liquid_configs[] __initdata = {
	{
		.gpio = 128,    /* qca1530 reset */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_qca1530_config,
		},
	},
	{
		.gpio = 66,     /* qca1530 power extra */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_qca1530_config_mpp7,
		},
	},
};

static struct msm_gpiomux_config msm_epm_configs[] __initdata = {
	{
		.gpio      = 92,		/* EPM enable */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_epm_config,
		},
	},
};

#if defined(CONFIG_SLIMPORT_ANX7808)
static struct gpiomux_setting slimport_int_sus_cfg = {
	.func  = GPIOMUX_FUNC_GPIO,
	.drv   = GPIOMUX_DRV_2MA,
	.pull  = GPIOMUX_PULL_UP,
	.dir   = GPIOMUX_IN,
};

static struct msm_gpiomux_config slimport_configs[] __initdata = {
	{
		.gpio = 40,    /* slimport int */
		.settings = {
			[GPIOMUX_SUSPENDED]	= &slimport_int_sus_cfg,
		},
	},
};
#endif

#ifdef CONFIG_LGE_BLUETOOTH
static struct gpiomux_setting bt_gpio_uart_active_config = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA, /* Drive Strength */
	.pull = GPIOMUX_PULL_NONE, /* Should be PULL NONE */
};

static struct gpiomux_setting bt_gpio_uart_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO, /* SUSPEND Configuration */
	.drv = GPIOMUX_DRV_2MA, /* Drive Strength */
	.pull = GPIOMUX_PULL_NONE, /* PULL Configuration */
};

#if 0
static struct gpiomux_setting bt_rfkill_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting bt_rfkill_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

static struct gpiomux_setting bt_host_wakeup_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting bt_host_wakeup_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting bt_wakeup_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting bt_wakeup_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting bt_pcm_active_config = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting bt_pcm_suspend_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct msm_gpiomux_config bt_msm_blsp_configs[] __initdata = {
	{
		.gpio = 43, /* BLSP6 UART TX */
		.settings = {
			[GPIOMUX_ACTIVE] = &bt_gpio_uart_active_config ,
			[GPIOMUX_SUSPENDED] = &bt_gpio_uart_suspend_config ,
		},
	},
	{
		.gpio = 44, /* BLSP6 UART RX */
		.settings = {
			[GPIOMUX_ACTIVE] = &bt_gpio_uart_active_config ,
			[GPIOMUX_SUSPENDED] = &bt_gpio_uart_suspend_config ,
		},
	},
	{
		.gpio = 45, /* BLSP6 UART CTS */
		.settings = {
			[GPIOMUX_ACTIVE] = &bt_gpio_uart_active_config ,
			[GPIOMUX_SUSPENDED] = &bt_gpio_uart_suspend_config ,
		},
	},
	{
		.gpio = 46, /* BLSP6 UART RFR */
		.settings = {
			[GPIOMUX_ACTIVE] = &bt_gpio_uart_active_config ,
			[GPIOMUX_SUSPENDED] = &bt_gpio_uart_suspend_config ,
		},
	},
};

#if 0
static struct msm_gpiomux_config bt_rfkill_configs[] = {
	{
		.gpio = 4, /* BT_RESET_N */
		.settings = {
			[GPIOMUX_ACTIVE]    = &bt_rfkill_active_config,
			[GPIOMUX_SUSPENDED] = &bt_rfkill_suspend_config,
		},
	},
};
#endif

static struct msm_gpiomux_config bt_host_wakeup_configs[] __initdata = {
	{
		.gpio = 67,
		.settings = {
			[GPIOMUX_ACTIVE]    = &bt_host_wakeup_active_config,
			[GPIOMUX_SUSPENDED] = &bt_host_wakeup_suspend_config,
		},
	},
};

static struct msm_gpiomux_config bt_wakeup_configs[] __initdata = {
	{
		.gpio = 133,
		.settings = {
			[GPIOMUX_ACTIVE]    = &bt_wakeup_active_config,
			[GPIOMUX_SUSPENDED] = &bt_wakeup_suspend_config,
		},
	},
};

static struct msm_gpiomux_config bt_pcm_configs[] __initdata = {
	{
		.gpio = 87, /* BT_PCM_CLK */
		.settings = {
			[GPIOMUX_ACTIVE] = &bt_pcm_active_config,
			[GPIOMUX_SUSPENDED] = &bt_pcm_suspend_config,
		},
	},
	{
		.gpio = 88, /* BT_PCM_SYNC */
		.settings = {
			[GPIOMUX_ACTIVE] = &bt_pcm_active_config,
			[GPIOMUX_SUSPENDED] = &bt_pcm_suspend_config,
		},
	},
	{
		.gpio = 89, /* BT_PCM_DIN */
		.settings = {
			[GPIOMUX_ACTIVE] = &bt_pcm_active_config,
			[GPIOMUX_SUSPENDED] = &bt_pcm_suspend_config,
		},
	},
	{
		.gpio = 90, /* BT_PCM_DOUT */
		.settings = {
			[GPIOMUX_ACTIVE] = &bt_pcm_active_config,
			[GPIOMUX_SUSPENDED] = &bt_pcm_suspend_config,
		},
	}
};

static void bluetooth_msm_gpiomux_install(void)
{
	/* UART */
	msm_gpiomux_install(bt_msm_blsp_configs, ARRAY_SIZE(bt_msm_blsp_configs));

	/* RFKILL */
	/* RESET PIN USES PMA8084 GPIO */
#if 0
	msm_gpiomux_install(bt_rfkill_configs, ARRAY_SIZE(bt_rfkill_configs));
#endif
	/* HOST WAKE-UP */
	msm_gpiomux_install(bt_host_wakeup_configs, ARRAY_SIZE(bt_host_wakeup_configs));

	/* BT WAKE-UP */
	msm_gpiomux_install(bt_wakeup_configs, ARRAY_SIZE(bt_wakeup_configs));

	/* PCM I/F */
	msm_gpiomux_install(bt_pcm_configs, ARRAY_SIZE(bt_pcm_configs));
}
#endif /*                      */

void __init lgpc9_rev_a_init_gpiomux(void)
{
	msm_gpiomux_install(msm_hall_ic_configs_rev_a,
			ARRAY_SIZE(msm_hall_ic_configs_rev_a));
};

void __init lgpc9_rev_g_init_gpiomux(void)
{
	msm_gpiomux_install(msm_hall_ic_configs_rev_g,
			ARRAY_SIZE(msm_hall_ic_configs_rev_g));

	msm_gpiomux_install(msm_vibrator_configs_rev_g,
			ARRAY_SIZE(msm_vibrator_configs_rev_g));
};

void __init lgpc9_init_gpiomux(void)
{
	hw_rev_type rev = lge_get_board_revno();

	switch (rev) {
		case HW_REV_EVB1:
		case HW_REV_EVB2:
		case HW_REV_A:
			lgpc9_rev_a_init_gpiomux();
			break;
		case HW_REV_B:
		case HW_REV_C:
		case HW_REV_D:
		case HW_REV_E:
		case HW_REV_F:
		case HW_REV_G:
			lgpc9_rev_g_init_gpiomux();
			break;
		case HW_REV_H:
		case HW_REV_1_0:
		case HW_REV_1_1:
		case HW_REV_1_2:
		case HW_REV_MAX:
			break;
	}
};

void __init apq8084_init_gpiomux(void)
{
	int rc;

	rc = msm_gpiomux_init_dt();
	if (rc) {
		pr_err("%s failed %d\n", __func__, rc);
		return;
	}

	if (of_board_is_sbc()) {
		msm_gpiomux_install(msm_sbc_blsp_configs,
				ARRAY_SIZE(msm_sbc_blsp_configs));
	} else {
		msm_gpiomux_install(msm_blsp_configs,
				ARRAY_SIZE(msm_blsp_configs));
	}

	msm_gpiomux_install(msm_synaptics_configs,
					ARRAY_SIZE(msm_synaptics_configs));
	msm_gpiomux_install(msm_led_configs,
					ARRAY_SIZE(msm_led_configs));

	if (of_board_is_liquid() || of_board_is_sbc()) {
		msm_gpiomux_install(msm_blsp2_uart5_configs,
				ARRAY_SIZE(msm_blsp2_uart5_configs));
		msm_gpiomux_install(msm_qca1530_liquid_configs,
				ARRAY_SIZE(msm_qca1530_liquid_configs));
	} else {
		msm_gpiomux_install(mdm_configs, ARRAY_SIZE(mdm_configs));
		msm_gpiomux_install(msm_blsp2_uart1_configs,
				ARRAY_SIZE(msm_blsp2_uart1_configs));

	}
	msm_gpiomux_install_nowrite(msm_lcd_configs,
			ARRAY_SIZE(msm_lcd_configs));

	if (of_board_is_sbc()) {
		msm_gpiomux_install(msm_hdmi_nomux_configs,
			ARRAY_SIZE(msm_hdmi_nomux_configs));
		msm_gpiomux_install(apq8084_general_led_configs,
			ARRAY_SIZE(apq8084_general_led_configs));
	} else {
		msm_gpiomux_install(apq8084_hsic_configs,
			ARRAY_SIZE(apq8084_hsic_configs));
		msm_gpiomux_install(msm_hdmi_configs,
			ARRAY_SIZE(msm_hdmi_configs));
	}

	msm_gpiomux_install(wlan_wake_msm_configs, ARRAY_SIZE(wlan_wake_msm_configs));
	msm_gpiomux_install(pcie0_wake_n_configs, ARRAY_SIZE(pcie0_wake_n_configs));
	msm_gpiomux_install(sd_card_det, ARRAY_SIZE(sd_card_det));
	if (of_board_is_cdp() || of_board_is_sbc())
		msm_gpiomux_install(eth_pwr, ARRAY_SIZE(eth_pwr));
	if (of_board_is_sbc())
		msm_gpiomux_install(msm_sbc_sensor_configs,
				ARRAY_SIZE(msm_sbc_sensor_configs));
	else
		msm_gpiomux_install(msm_sensor_configs,
				ARRAY_SIZE(msm_sensor_configs));
	msm_gpiomux_install(msm_pcie_configs, ARRAY_SIZE(msm_pcie_configs));
	msm_gpiomux_install(msm_epm_configs, ARRAY_SIZE(msm_epm_configs));
#if defined(CONFIG_SWITCH_MAX1462X) || defined(CONFIG_INPUT_MAX14688)
	msm_gpiomux_install(headset_configs, ARRAY_SIZE(headset_configs));
#endif

#ifdef CONFIG_SND_SOC_CS35L32
	if (lge_get_board_revno() >= HW_REV_G)
		msm_gpiomux_install(msm8974_qua_mi2s_configs,
				ARRAY_SIZE(msm8974_qua_mi2s_configs));
#endif
#ifdef CONFIG_SND_SOC_TPA2015D
	if (lge_get_board_revno() < HW_REV_B)
		msm_gpiomux_install(tpa2015d_extamp_configs,
				ARRAY_SIZE(tpa2015d_extamp_configs));
#endif

#if defined(CONFIG_KS8851) || defined(CONFIG_KS8851_MODULE)
	if (of_board_is_cdp())
		msm_gpiomux_install(msm_eth_configs,
			ARRAY_SIZE(msm_eth_configs));
#endif

#if defined(CONFIG_SLIMPORT_ANX7808)
	msm_gpiomux_install(slimport_configs,
			ARRAY_SIZE(slimport_configs));
#endif

	/*                                                       */
#ifdef CONFIG_LGE_NFC_PN547_C2
	msm_gpiomux_install(msm_nfc_configs, ARRAY_SIZE(msm_nfc_configs));
#endif
	/*                                                       */

#ifdef CONFIG_LGE_BLUETOOTH
	bluetooth_msm_gpiomux_install();
#endif

	lgpc9_init_gpiomux();
}
