#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <linux/err.h>
#include <linux/qpnp/pwm.h>
#include <linux/qpnp/pin.h>
#include <linux/spmi.h>
#include "qpnp-leds.h"

extern struct qpnp_pin_spec *red_led;
extern struct qpnp_pin_spec *blue_led;
extern struct qpnp_pin_spec *green_led;

/*                                    */
#define RGB_BRIGHTNESS_TUNNING_R	1
#define RGB_BRIGHTNESS_TUNNING_G	1
#define RGB_BRIGHTNESS_TUNNING_B	1
/*                                    */

#if defined(CONFIG_LEDS_WINDOW_COLOR)
/*                                             */
#define WINDOW_COLOR_BRIGHTNESS_TUNNING_BK	49/255
#define WINDOW_COLOR_BRIGHTNESS_TUNNING_WH	1
extern enum WINDOW_COLORS window_color;
static int init_patterns_for_window_color;
/*                                    */
int mix_brightness_tunning = 1;
#endif
#define BRIGHTNESS_TUNNING	49/255


/* For pattern brightness tunning */
int leds_pwm_duty_pcts_brightness_tunning[79] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0,
	1, 1, 0, 0,
	1, 1, 0, 0,
	0, 0, 0, 65
};



void make_onoff_led_pattern(int rgb)
{
	int onoff_pattern[6] = {0, 0, 0, 0, 0, 0};
	struct lut_params rgb_lut_params;
	red_led->cdev.brightness = 0;
	green_led->cdev.brightness = 0;
	blue_led->cdev.brightness = 0;

	printk(KERN_DEBUG "[RGB LED] make_onoff_led_pattern rgb is %06x\n", rgb);
	pwm_disable(red_led->pwm_dev);
	pwm_disable(green_led->pwm_dev);
	pwm_disable(blue_led->pwm_dev);

	rgb_lut_params.idx_len = 2;
	rgb_lut_params.lut_pause_hi = 0;
	rgb_lut_params.lut_pause_lo = 0;
	rgb_lut_params.ramp_step_ms = 0;
	rgb_lut_params.flags = 65;

#if defined(CONFIG_LEDS_WINDOW_COLOR)
	switch (window_color) {
	case WINDOW_COLOR_WH:
		onoff_pattern[0] = ((rgb >> 16) & 0xFF)
			* RGB_BRIGHTNESS_TUNNING_R / mix_brightness_tunning
			* WINDOW_COLOR_BRIGHTNESS_TUNNING_WH;

		onoff_pattern[2] = ((rgb >> 8) & 0xFF)
			* RGB_BRIGHTNESS_TUNNING_G / mix_brightness_tunning
			* WINDOW_COLOR_BRIGHTNESS_TUNNING_WH;

		onoff_pattern[4] = (rgb & 0xFF)
			* RGB_BRIGHTNESS_TUNNING_B / mix_brightness_tunning
			* WINDOW_COLOR_BRIGHTNESS_TUNNING_WH;
		break;

	case WINDOW_COLOR_BK:
	case WINDOW_COLOR_SV:
	case WINDOW_COLOR_TK:
	default:
		onoff_pattern[0] = ((rgb >> 16) & 0xFF)
			* RGB_BRIGHTNESS_TUNNING_R / mix_brightness_tunning
			* WINDOW_COLOR_BRIGHTNESS_TUNNING_BK;
		onoff_pattern[2] = ((rgb >> 8) & 0xFF)
			* RGB_BRIGHTNESS_TUNNING_G / mix_brightness_tunning
			* WINDOW_COLOR_BRIGHTNESS_TUNNING_BK;
		onoff_pattern[4] = (rgb & 0xFF)
			* RGB_BRIGHTNESS_TUNNING_B / mix_brightness_tunning
			* WINDOW_COLOR_BRIGHTNESS_TUNNING_BK;
		break;
	}
#else
	onoff_pattern[0] = ((rgb >> 16) & 0xFF)
		* RGB_BRIGHTNESS_TUNNING_R;
	onoff_pattern[2] = ((rgb >> 8) & 0xFF)
		* RGB_BRIGHTNESS_TUNNING_G;
	onoff_pattern[4] = (rgb & 0xFF)
		* RGB_BRIGHTNESS_TUNNING_B;
#endif

	onoff_pattern[1] = onoff_pattern[0];
	onoff_pattern[3] = onoff_pattern[2];
	onoff_pattern[5] = onoff_pattern[4];

	rgb_lut_params.start_idx = 0;
	pwm_lut_config(red_led->pwm_dev, 200,
		onoff_pattern, rgb_lut_params);

	rgb_lut_params.start_idx = 2;
	pwm_lut_config(green_led->pwm_dev, 200,
		&onoff_pattern[2], rgb_lut_params);

	rgb_lut_params.start_idx = 4;
	pwm_lut_config(blue_led->pwm_dev, 200,
		&onoff_pattern[4], rgb_lut_params);

	pwm_enable(red_led->pwm_dev);
	usleep(100);
	pwm_enable(green_led->pwm_dev);
	usleep(100);
	pwm_enable(blue_led->pwm_dev);

	red_led->cdev.brightness = (rgb >> 16) & 0xFF;
	green_led->cdev.brightness = (rgb >> 8) & 0xFF;
	blue_led->cdev.brightness = rgb & 0xFF;
}

void rgb_luts_set(void)
{
	unsigned rgb_brightness = 0;
	rgb_brightness = (red_led->cdev.brightness << 16)
		+ (green_led->cdev.brightness << 8)
		+ blue_led->cdev.brightness;
	if (rgb_brightness > 0)
		make_onoff_led_pattern(rgb_brightness);
	else
		make_onoff_led_pattern(0);
}

void make_blink_led_pattern(int rgb, int delay_on, int delay_off)
{
	int blink_pattern[6] = {0, 0, 0, 0, 0, 0};
	struct lut_params rgb_lut_params;

	red_led->cdev.brightness = 0;
	green_led->cdev.brightness = 0;
	blue_led->cdev.brightness = 0;

	pwm_disable(red_led->pwm_dev);
	pwm_disable(green_led->pwm_dev);
	pwm_disable(blue_led->pwm_dev);

	rgb_lut_params.idx_len = 2;
	rgb_lut_params.lut_pause_hi = delay_off/2;
	rgb_lut_params.lut_pause_lo = delay_on/2;
	rgb_lut_params.ramp_step_ms = 1;
	rgb_lut_params.flags = 89;

	if (mix_brightness_tunning) {
#if defined(CONFIG_LEDS_WINDOW_COLOR)
		switch (window_color) {
		case WINDOW_COLOR_WH:
			blink_pattern[0] = ((rgb >> 16) & 0xFF)
								* RGB_BRIGHTNESS_TUNNING_R / mix_brightness_tunning
								* WINDOW_COLOR_BRIGHTNESS_TUNNING_WH;
			blink_pattern[2] = ((rgb >> 8) & 0xFF)
								* RGB_BRIGHTNESS_TUNNING_G / mix_brightness_tunning
								* WINDOW_COLOR_BRIGHTNESS_TUNNING_WH;
			blink_pattern[4] = (rgb & 0xFF)
								* RGB_BRIGHTNESS_TUNNING_B / mix_brightness_tunning
								* WINDOW_COLOR_BRIGHTNESS_TUNNING_WH;
			break;
		case WINDOW_COLOR_BK:
		case WINDOW_COLOR_SV:
		case WINDOW_COLOR_TK:
		default:
			blink_pattern[0] = ((rgb >> 16) & 0xFF)
								* RGB_BRIGHTNESS_TUNNING_R / mix_brightness_tunning
								* WINDOW_COLOR_BRIGHTNESS_TUNNING_BK;
			blink_pattern[2] = ((rgb >> 8) & 0xFF)
								* RGB_BRIGHTNESS_TUNNING_G / mix_brightness_tunning
								* WINDOW_COLOR_BRIGHTNESS_TUNNING_BK;
			blink_pattern[4] = (rgb & 0xFF)
								* RGB_BRIGHTNESS_TUNNING_B / mix_brightness_tunning
								* WINDOW_COLOR_BRIGHTNESS_TUNNING_BK;
			break;
		}
#else
		blink_pattern[0] = ((rgb >> 16) & 0xFF)
							* RGB_BRIGHTNESS_TUNNING_R / mix_brightness_tunning
							* BRIGHTNESS_TUNNING;
		blink_pattern[2] = ((rgb >> 8) & 0xFF)
							* RGB_BRIGHTNESS_TUNNING_G / mix_brightness_tunning
							* BRIGHTNESS_TUNNING;
		blink_pattern[4] = (rgb & 0xFF)
							* RGB_BRIGHTNESS_TUNNING_B / mix_brightness_tunning
							* BRIGHTNESS_TUNNING;
#endif
		/* printk(KERN_INFO "[RGB LED] Tunning RGB R = %d, G = %d, B = %d\n",
			onoff_pattern[0], onoff_pattern[2], onoff_pattern[4]); */
	}

	rgb_lut_params.start_idx = 0;
	pwm_lut_config(red_led->pwm_dev, 200, blink_pattern, rgb_lut_params);
	rgb_lut_params.start_idx = 2;
	pwm_lut_config(green_led->pwm_dev, 200, &blink_pattern[2], rgb_lut_params);
	rgb_lut_params.start_idx = 4;
	pwm_lut_config(blue_led->pwm_dev, 200, &blink_pattern[4], rgb_lut_params);

	pwm_enable(red_led->pwm_dev);
	usleep(100);
	pwm_enable(green_led->pwm_dev);
	usleep(100);
	pwm_enable(blue_led->pwm_dev);

	red_led->cdev.brightness = (rgb >> 16) & 0xFF;
	green_led->cdev.brightness = (rgb >> 8) & 0xFF;
	blue_led->cdev.brightness = rgb & 0xFF;

}

#if defined(CONFIG_LEDS_WINDOW_COLOR)
static int qpnp_get_config_pwm_window_color(struct qpnp_pin_spec *led){
	printk("[RGB LED] %s window_color is %d\n", __func__, window_color);
	switch (window_color) {
		case WINDOW_COLOR_WH:
		led->duty_cycles->num_duty_pcts = leds_pwm_duty_cycles_wh.num_duty_pcts;
		led->duty_cycles->duty_pcts0 = leds_pwm_duty_cycles_wh.duty_pcts0;
		led->duty_cycles->duty_pcts1 = leds_pwm_duty_cycles_wh.duty_pcts1;
		led->duty_cycles->duty_pcts2 = leds_pwm_duty_cycles_wh.duty_pcts2;
		led->duty_cycles->duty_pcts3 = leds_pwm_duty_cycles_wh.duty_pcts3;
		led->duty_cycles->duty_pcts4 = leds_pwm_duty_cycles_wh.duty_pcts4;
		led->duty_cycles->duty_pcts5 = leds_pwm_duty_cycles_wh.duty_pcts5;
		led->duty_cycles->duty_pcts6 = leds_pwm_duty_cycles_wh.duty_pcts6;
		led->duty_cycles->duty_pcts7 = leds_pwm_duty_cycles_wh.duty_pcts7;
		led->duty_cycles->duty_pcts8 = leds_pwm_duty_cycles_wh.duty_pcts8;
		led->duty_cycles->duty_pcts12 = leds_pwm_duty_cycles_wh.duty_pcts12;
		led->duty_cycles->duty_pcts13 = leds_pwm_duty_cycles_wh.duty_pcts13;
		led->duty_cycles->duty_pcts14 = leds_pwm_duty_cycles_wh.duty_pcts14;
		led->duty_cycles->duty_pcts17 = leds_pwm_duty_cycles_wh.duty_pcts17;
		led->duty_cycles->duty_pcts18 = leds_pwm_duty_cycles_wh.duty_pcts18;
		led->duty_cycles->duty_pcts19 = leds_pwm_duty_cycles_wh.duty_pcts19;
		led->duty_cycles->duty_pcts20 = leds_pwm_duty_cycles_wh.duty_pcts20;
		led->duty_cycles->duty_pcts29 = leds_pwm_duty_cycles_wh.duty_pcts29;
		led->duty_cycles->duty_pcts30 = leds_pwm_duty_cycles_wh.duty_pcts30;
		led->duty_cycles->duty_pcts31 = leds_pwm_duty_cycles_wh.duty_pcts31;
		led->duty_cycles->duty_pcts32 = leds_pwm_duty_cycles_wh.duty_pcts32;
		led->duty_cycles->duty_pcts37 = leds_pwm_duty_cycles_wh.duty_pcts37;
		led->duty_cycles->duty_pcts39 = leds_pwm_duty_cycles_wh.duty_pcts39;
		led->duty_cycles->duty_pcts40 = leds_pwm_duty_cycles_wh.duty_pcts40;
		led->duty_cycles->duty_pcts41 = leds_pwm_duty_cycles_wh.duty_pcts41;
		led->duty_cycles->duty_pcts42 = leds_pwm_duty_cycles_wh.duty_pcts42;
		led->duty_cycles->duty_pcts43 = leds_pwm_duty_cycles_wh.duty_pcts43;
		led->duty_cycles->duty_pcts44 = leds_pwm_duty_cycles_wh.duty_pcts44;
		led->duty_cycles->duty_pcts45 = leds_pwm_duty_cycles_wh.duty_pcts45;
		led->duty_cycles->duty_pcts46 = leds_pwm_duty_cycles_wh.duty_pcts46;
		led->duty_cycles->duty_pcts47 = leds_pwm_duty_cycles_wh.duty_pcts47;
		led->duty_cycles->duty_pcts101 = leds_pwm_duty_cycles_wh.duty_pcts101;
		led->duty_cycles->duty_pcts102 = leds_pwm_duty_cycles_wh.duty_pcts102;
		break;
		case WINDOW_COLOR_BK:
		case WINDOW_COLOR_SV:
		case WINDOW_COLOR_TK:
		default:
		led->duty_cycles->num_duty_pcts = leds_pwm_duty_cycles_bk.num_duty_pcts;
		led->duty_cycles->duty_pcts0 = leds_pwm_duty_cycles_bk.duty_pcts0;
		led->duty_cycles->duty_pcts1 = leds_pwm_duty_cycles_bk.duty_pcts1;
		led->duty_cycles->duty_pcts2 = leds_pwm_duty_cycles_bk.duty_pcts2;
		led->duty_cycles->duty_pcts3 = leds_pwm_duty_cycles_bk.duty_pcts3;
		led->duty_cycles->duty_pcts4 = leds_pwm_duty_cycles_bk.duty_pcts4;
		led->duty_cycles->duty_pcts5 = leds_pwm_duty_cycles_bk.duty_pcts5;
		led->duty_cycles->duty_pcts6 = leds_pwm_duty_cycles_bk.duty_pcts6;
		led->duty_cycles->duty_pcts7 = leds_pwm_duty_cycles_bk.duty_pcts7;
		led->duty_cycles->duty_pcts8 = leds_pwm_duty_cycles_bk.duty_pcts8;
		led->duty_cycles->duty_pcts12 = leds_pwm_duty_cycles_bk.duty_pcts12;
		led->duty_cycles->duty_pcts13 = leds_pwm_duty_cycles_bk.duty_pcts13;
		led->duty_cycles->duty_pcts14 = leds_pwm_duty_cycles_bk.duty_pcts14;
		led->duty_cycles->duty_pcts17 = leds_pwm_duty_cycles_bk.duty_pcts17;
		led->duty_cycles->duty_pcts18 = leds_pwm_duty_cycles_bk.duty_pcts18;
		led->duty_cycles->duty_pcts19 = leds_pwm_duty_cycles_bk.duty_pcts19;
		led->duty_cycles->duty_pcts20 = leds_pwm_duty_cycles_bk.duty_pcts20;
		led->duty_cycles->duty_pcts29 = leds_pwm_duty_cycles_bk.duty_pcts29;
		led->duty_cycles->duty_pcts30 = leds_pwm_duty_cycles_bk.duty_pcts30;
		led->duty_cycles->duty_pcts31 = leds_pwm_duty_cycles_bk.duty_pcts31;
		led->duty_cycles->duty_pcts32 = leds_pwm_duty_cycles_bk.duty_pcts32;
		led->duty_cycles->duty_pcts37 = leds_pwm_duty_cycles_bk.duty_pcts37;
		led->duty_cycles->duty_pcts39 = leds_pwm_duty_cycles_bk.duty_pcts39;
		led->duty_cycles->duty_pcts40 = leds_pwm_duty_cycles_bk.duty_pcts40;
		led->duty_cycles->duty_pcts41 = leds_pwm_duty_cycles_bk.duty_pcts41;
		led->duty_cycles->duty_pcts42 = leds_pwm_duty_cycles_bk.duty_pcts42;
		led->duty_cycles->duty_pcts43 = leds_pwm_duty_cycles_bk.duty_pcts43;
		led->duty_cycles->duty_pcts44 = leds_pwm_duty_cycles_bk.duty_pcts44;
		led->duty_cycles->duty_pcts45 = leds_pwm_duty_cycles_bk.duty_pcts45;
		led->duty_cycles->duty_pcts46 = leds_pwm_duty_cycles_bk.duty_pcts46;
		led->duty_cycles->duty_pcts47 = leds_pwm_duty_cycles_bk.duty_pcts47;
		led->duty_cycles->duty_pcts101 = leds_pwm_duty_cycles_bk.duty_pcts101;
		led->duty_cycles->duty_pcts102 = leds_pwm_duty_cycles_bk.duty_pcts102;
		break;
	}
	return 0;
}
#endif
void change_led_pattern(int pattern)
{
	int *duty_pcts_red = NULL;
	int *duty_pcts_green = NULL;
	int *duty_pcts_blue = NULL;
	struct lut_params rgb_lut_params;
	int i;

	printk("%s, %d : pattern : %d \n", __func__ , __LINE__, pattern);

#if defined(CONFIG_LEDS_WINDOW_COLOR)
	if (unlikely(!init_patterns_for_window_color && pattern > 1)) {
		qpnp_get_config_pwm_window_color(red_led);
		qpnp_get_config_pwm_window_color(green_led);
		qpnp_get_config_pwm_window_color(blue_led);
		init_patterns_for_window_color = 1;
	}
#endif

	switch (pattern) {
	case 0:
		duty_pcts_red = red_led->duty_cycles->duty_pcts0;
		duty_pcts_green = red_led->duty_cycles->duty_pcts0;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts0;
		break;
	case 1:
		duty_pcts_red = red_led->duty_cycles->duty_pcts1;
		duty_pcts_green = red_led->duty_cycles->duty_pcts1;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts1;
		break;
	case 2:
		duty_pcts_red = red_led->duty_cycles->duty_pcts2;
		duty_pcts_green = red_led->duty_cycles->duty_pcts2;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts2;
		break;
	case 3:
		duty_pcts_red = red_led->duty_cycles->duty_pcts3;
		duty_pcts_green = red_led->duty_cycles->duty_pcts3;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts3;
		break;
	case 4:
		duty_pcts_red = red_led->duty_cycles->duty_pcts4;
		duty_pcts_green = red_led->duty_cycles->duty_pcts4;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts4;
		break;
	case 5:
		duty_pcts_red = red_led->duty_cycles->duty_pcts5;
		duty_pcts_green = red_led->duty_cycles->duty_pcts5;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts5;
		break;
	case 6:
		duty_pcts_red = red_led->duty_cycles->duty_pcts6;
		duty_pcts_green = red_led->duty_cycles->duty_pcts6;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts6;
		break;
	case 7:
		duty_pcts_red = red_led->duty_cycles->duty_pcts7;
		duty_pcts_green = red_led->duty_cycles->duty_pcts7;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts7;
		break;
	case 8:
		duty_pcts_red = red_led->duty_cycles->duty_pcts8;
		duty_pcts_green = red_led->duty_cycles->duty_pcts8;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts8;
		break;
	case 12:
		duty_pcts_red = red_led->duty_cycles->duty_pcts12;
		duty_pcts_green = red_led->duty_cycles->duty_pcts12;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts12;
		break;
	case 13:
		duty_pcts_red = red_led->duty_cycles->duty_pcts13;
		duty_pcts_green = red_led->duty_cycles->duty_pcts13;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts13;
		break;
	case 14:
		duty_pcts_red = red_led->duty_cycles->duty_pcts14;
		duty_pcts_green = red_led->duty_cycles->duty_pcts14;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts14;
		break;
	case 17:
		duty_pcts_red = red_led->duty_cycles->duty_pcts17;
		duty_pcts_green = red_led->duty_cycles->duty_pcts17;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts17;
		break;
	case 18:
		duty_pcts_red = red_led->duty_cycles->duty_pcts18;
		duty_pcts_green = red_led->duty_cycles->duty_pcts18;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts18;
		break;
	case 19:
		duty_pcts_red = red_led->duty_cycles->duty_pcts19;
		duty_pcts_green = red_led->duty_cycles->duty_pcts19;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts19;
		break;
	case 20:
		duty_pcts_red = red_led->duty_cycles->duty_pcts20;
		duty_pcts_green = red_led->duty_cycles->duty_pcts20;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts20;
		break;
	case 29:
		duty_pcts_red = red_led->duty_cycles->duty_pcts29;
		duty_pcts_green = red_led->duty_cycles->duty_pcts29;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts29;
		break;
	case 30:
		duty_pcts_red = red_led->duty_cycles->duty_pcts30;
		duty_pcts_green = red_led->duty_cycles->duty_pcts30;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts30;
		break;
	case 31:
		duty_pcts_red = red_led->duty_cycles->duty_pcts31;
		duty_pcts_green = red_led->duty_cycles->duty_pcts31;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts31;
		break;
	case 32:
		duty_pcts_red = red_led->duty_cycles->duty_pcts32;
		duty_pcts_green = red_led->duty_cycles->duty_pcts32;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts32;
		break;
	case 37:
		duty_pcts_red = red_led->duty_cycles->duty_pcts37;
		duty_pcts_green = red_led->duty_cycles->duty_pcts37;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts37;
		break;
	case 39:
		duty_pcts_red   = red_led->duty_cycles->duty_pcts39;
		duty_pcts_green = red_led->duty_cycles->duty_pcts39;
		duty_pcts_blue  = red_led->duty_cycles->duty_pcts39;
		break;
	case 40:
		duty_pcts_red   = red_led->duty_cycles->duty_pcts40;
		duty_pcts_green = red_led->duty_cycles->duty_pcts40;
		duty_pcts_blue  = red_led->duty_cycles->duty_pcts40;
		break;
	case 41:
		duty_pcts_red   = red_led->duty_cycles->duty_pcts41;
		duty_pcts_green = red_led->duty_cycles->duty_pcts41;
		duty_pcts_blue  = red_led->duty_cycles->duty_pcts41;
		break;
	case 42:
		duty_pcts_red   = red_led->duty_cycles->duty_pcts42;
		duty_pcts_green = red_led->duty_cycles->duty_pcts42;
		duty_pcts_blue  = red_led->duty_cycles->duty_pcts42;
		break;
	case 43:
		duty_pcts_red   = red_led->duty_cycles->duty_pcts43;
		duty_pcts_green = red_led->duty_cycles->duty_pcts43;
		duty_pcts_blue  = red_led->duty_cycles->duty_pcts43;
		break;
	case 44:
		duty_pcts_red   = red_led->duty_cycles->duty_pcts44;
		duty_pcts_green = red_led->duty_cycles->duty_pcts44;
		duty_pcts_blue  = red_led->duty_cycles->duty_pcts44;
		break;
	case 45:
		duty_pcts_red   = red_led->duty_cycles->duty_pcts45;
		duty_pcts_green = red_led->duty_cycles->duty_pcts45;
		duty_pcts_blue  = red_led->duty_cycles->duty_pcts45;
		break;
	case 46:
		duty_pcts_red   = red_led->duty_cycles->duty_pcts46;
		duty_pcts_green = red_led->duty_cycles->duty_pcts46;
		duty_pcts_blue  = red_led->duty_cycles->duty_pcts46;
		break;
	case 47:
		duty_pcts_red   = red_led->duty_cycles->duty_pcts47;
		duty_pcts_green = red_led->duty_cycles->duty_pcts47;
		duty_pcts_blue  = red_led->duty_cycles->duty_pcts47;
	case 101:
		duty_pcts_red = red_led->duty_cycles->duty_pcts101;
		duty_pcts_green = red_led->duty_cycles->duty_pcts101;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts101;
		break;
	case 102:
		duty_pcts_red = red_led->duty_cycles->duty_pcts102;
		duty_pcts_green = red_led->duty_cycles->duty_pcts102;
		duty_pcts_blue = red_led->duty_cycles->duty_pcts102;
		break;
	default:
		return;
	}

/*for pattern tunning*/
	for(i = 0; i < 79; i++) {
		if(i >= 0 && i <= 62) {
#if defined(CONFIG_LEDS_WINDOW_COLOR)
			switch (window_color) {
			case WINDOW_COLOR_WH:
				leds_pwm_duty_pcts_brightness_tunning[i] = duty_pcts_red[i]
															* WINDOW_COLOR_BRIGHTNESS_TUNNING_WH;
				break;
			case WINDOW_COLOR_BK:
			case WINDOW_COLOR_SV:
			case WINDOW_COLOR_TK:
			default:
				leds_pwm_duty_pcts_brightness_tunning[i] = duty_pcts_red[i]
															* WINDOW_COLOR_BRIGHTNESS_TUNNING_BK;
				break;
			}
#else
			leds_pwm_duty_pcts_brightness_tunning[i] = duty_pcts_red[i]
														* BRIGHTNESS_TUNNING;
#endif
		} else {
			leds_pwm_duty_pcts_brightness_tunning[i] = duty_pcts_red[i];
		}
	}

	duty_pcts_red = leds_pwm_duty_pcts_brightness_tunning;
	duty_pcts_green = leds_pwm_duty_pcts_brightness_tunning;
	duty_pcts_blue = leds_pwm_duty_pcts_brightness_tunning;

	pwm_disable(red_led->pwm_dev);
	pwm_disable(green_led->pwm_dev);
	pwm_disable(blue_led->pwm_dev);

	// red lut config
	rgb_lut_params.start_idx = duty_pcts_red[63];
	rgb_lut_params.idx_len = duty_pcts_red[64];
	rgb_lut_params.lut_pause_hi = duty_pcts_red[66];
	rgb_lut_params.ramp_step_ms = duty_pcts_red[78];
	rgb_lut_params.flags = duty_pcts_red[75];

	if (pattern != 0) {
		if (duty_pcts_red[65] != 0) {
			pwm_lut_config(red_led->pwm_dev, 200,
					&duty_pcts_red[duty_pcts_red[63]], rgb_lut_params);
			pwm_enable(red_led->pwm_dev);
		}
	}
	usleep(100);

	// green lut config
	rgb_lut_params.start_idx = duty_pcts_green[67];
	rgb_lut_params.idx_len = duty_pcts_green[68];
	rgb_lut_params.lut_pause_hi = duty_pcts_green[70];
	rgb_lut_params.flags = duty_pcts_green[76];

	if (pattern != 0) {
		if (duty_pcts_green[69] != 0){
			pwm_lut_config(green_led->pwm_dev, 200,
					&duty_pcts_green[duty_pcts_green[67]], rgb_lut_params);
			pwm_enable(green_led->pwm_dev);
		}
	}
	usleep(100);

	// blue lut config
	rgb_lut_params.start_idx = duty_pcts_blue[71];
	rgb_lut_params.idx_len = duty_pcts_blue[72];
	rgb_lut_params.lut_pause_hi = duty_pcts_blue[74];
	rgb_lut_params.flags = duty_pcts_blue[77];

	if (pattern != 0){
		if (duty_pcts_blue[73] != 0){
			pwm_lut_config(blue_led->pwm_dev, 200,
					&duty_pcts_blue[duty_pcts_blue[71]], rgb_lut_params);
			pwm_enable(blue_led->pwm_dev);
		}
	}

}


static int qpnp_pwm_init(void)
{
	struct pwm_period_config period;

	red_led->pwm_dev = pwm_request(red_led->pwm_channel_id, "lpg_3");
	blue_led->pwm_dev = pwm_request(blue_led->pwm_channel_id, "lpg_4");
	green_led->pwm_dev = pwm_request(green_led->pwm_channel_id, "lpg_5");

	if (IS_ERR_OR_NULL(red_led->pwm_dev) ||
			IS_ERR_OR_NULL(blue_led->pwm_dev) ||
			IS_ERR_OR_NULL(green_led->pwm_dev)) {
		printk("%s,%d : Unable to acquire PWM Channel \n",
				__func__, __LINE__);
	}

	red_led->duty_cycles = devm_kzalloc(&red_led->q_chip->spmi->dev,
			sizeof(struct pwm_duty_cycles), GFP_KERNEL);

	if (!red_led->duty_cycles) {
		printk("%s,%d : Unable to allocate Memory \n",
				__func__, __LINE__);
		return -ENOMEM;
	}

	blue_led->duty_cycles = devm_kzalloc(&blue_led->q_chip->spmi->dev,
			sizeof(struct pwm_duty_cycles), GFP_KERNEL);

	if (!blue_led->duty_cycles) {
		printk("%s,%d : Unable to allocate Memory \n",
				__func__, __LINE__);
		return -ENOMEM;
	}

	green_led->duty_cycles = devm_kzalloc(&green_led->q_chip->spmi->dev,
			sizeof(struct pwm_duty_cycles), GFP_KERNEL);

	if (!red_led->duty_cycles) {
		printk("%s,%d : Unable to allocate Memory \n",
				__func__, __LINE__);
		return -ENOMEM;
	}

	red_led->duty_cycles->num_duty_pcts =
		leds_pwm_duty_cycles.num_duty_pcts;
	red_led->duty_cycles->duty_pcts0 = leds_pwm_duty_cycles.duty_pcts0;
	red_led->duty_cycles->duty_pcts1 = leds_pwm_duty_cycles.duty_pcts1;
	red_led->duty_cycles->duty_pcts2 = leds_pwm_duty_cycles.duty_pcts2;
	red_led->duty_cycles->duty_pcts3 = leds_pwm_duty_cycles.duty_pcts3;
	red_led->duty_cycles->duty_pcts4 = leds_pwm_duty_cycles.duty_pcts4;
	red_led->duty_cycles->duty_pcts5 = leds_pwm_duty_cycles.duty_pcts5;
	red_led->duty_cycles->duty_pcts6 = leds_pwm_duty_cycles.duty_pcts6;
	red_led->duty_cycles->duty_pcts7 = leds_pwm_duty_cycles.duty_pcts7;
	red_led->duty_cycles->duty_pcts8 = leds_pwm_duty_cycles.duty_pcts8;
	red_led->duty_cycles->duty_pcts12 = leds_pwm_duty_cycles.duty_pcts12;
	red_led->duty_cycles->duty_pcts13 = leds_pwm_duty_cycles.duty_pcts13;
	red_led->duty_cycles->duty_pcts14 = leds_pwm_duty_cycles.duty_pcts14;
	red_led->duty_cycles->duty_pcts17 = leds_pwm_duty_cycles.duty_pcts17;
	red_led->duty_cycles->duty_pcts18 = leds_pwm_duty_cycles.duty_pcts18;
	red_led->duty_cycles->duty_pcts19 = leds_pwm_duty_cycles.duty_pcts19;
	red_led->duty_cycles->duty_pcts20 = leds_pwm_duty_cycles.duty_pcts20;
	red_led->duty_cycles->duty_pcts29 = leds_pwm_duty_cycles.duty_pcts29;
	red_led->duty_cycles->duty_pcts30 = leds_pwm_duty_cycles.duty_pcts30;
	red_led->duty_cycles->duty_pcts31 = leds_pwm_duty_cycles.duty_pcts31;
	red_led->duty_cycles->duty_pcts32 = leds_pwm_duty_cycles.duty_pcts32;
	red_led->duty_cycles->duty_pcts37 = leds_pwm_duty_cycles.duty_pcts37;
	red_led->duty_cycles->duty_pcts101 = leds_pwm_duty_cycles.duty_pcts101;
	red_led->duty_cycles->duty_pcts102 = leds_pwm_duty_cycles.duty_pcts102;

	pwm_change_mode(red_led->pwm_dev, PM_PWM_MODE_LPG);
	pwm_change_mode(blue_led->pwm_dev, PM_PWM_MODE_LPG);
	pwm_change_mode(green_led->pwm_dev, PM_PWM_MODE_LPG);

	period.pwm_size = PM_PWM_SIZE_9BIT;
	period.clk = PM_PWM_CLK_19P2MHZ;
	period.pre_div = PM_PWM_PDIV_2;
	period.pre_div_exp = 0;
	pwm_config_period(red_led->pwm_dev, &period);
	pwm_config_period(green_led->pwm_dev, &period);
	pwm_config_period(blue_led->pwm_dev, &period);

	return 0;
}

static int qpnp_leds_probe(struct platform_device *pdev)
{
	int ret;

	printk("%s,%d : qpnp_leds_probe !! \n", __func__, __LINE__);

	ret = qpnp_pwm_init();

	if (ret < 0) {
		printk("%s,%d : qpnp_leds_probe failed \n",
				__func__, __LINE__);
		return 0;
	}

	return 0;
};

#ifdef CONFIG_OF
static struct of_device_id of_led_match_table[] = {
	{ .compatible = "qcom,qpnp-leds",},
	{ },
};
#else
#define of_led_match_table NULL
#endif

static struct platform_driver qpnp_leds_driver = {
	.probe		= qpnp_leds_probe,
	.remove		= __exit_p(qpnp_leds_remove),
	.driver		= {
		.name	= "qcom,qpnp-leds",
		.of_match_table = of_led_match_table,
	},
};

static int __init qpnp_leds_init(void)
{
	return platform_driver_register(&qpnp_leds_driver);
}
module_init(qpnp_leds_init);

static void __exit qpnp_leds_remove(void)
{
	platform_driver_unregister(&qpnp_leds_driver);
}
module_exit(qpnp_leds_remove);

MODULE_DESCRIPTION("QPNP LEDs driver");
MODULE_LICENSE("GPL v2");
