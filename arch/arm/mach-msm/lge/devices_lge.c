#include <linux/kernel.h>
#include <linux/string.h>

#include <mach/board_lge.h>

#include <linux/platform_device.h>
#include <asm/setup.h>
#include <asm/system_info.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_fdt.h>

#ifdef CONFIG_LGE_PM
#include <linux/qpnp/qpnp-adc.h>
#include <mach/board_lge.h>
#include <linux/power_supply.h>
#include <linux/power/lge_battery_id.h>
#endif

#ifdef CONFIG_USB_G_LGE_ANDROID
#include <linux/platform_data/lge_android_usb.h>
#endif

#define PROP_VAL_MAX_SIZE 50

#ifdef CONFIG_LGE_PM
struct chg_cable_info_table {
	int threshhold;
	acc_cable_type type;
	unsigned ta_ma;
	unsigned usb_ma;
};

#define ADC_NO_INIT_CABLE   0
#define C_NO_INIT_TA_MA     0
#define C_NO_INIT_USB_MA    0
#define ADC_CABLE_NONE      1900000
#define C_NONE_TA_MA        700
#define C_NONE_USB_MA       500

#define MAX_CABLE_NUM		15
static bool cable_type_defined;
static struct chg_cable_info_table lge_acc_cable_type_data[MAX_CABLE_NUM];
#endif

static int cn_arr_len = 3;
struct cn_prop {
	char *name;
	enum cn_prop_type type;
	uint32_t cell_u32;
	uint64_t cell_u64;
	char str[PROP_VAL_MAX_SIZE];
	uint8_t is_valid;
};

static struct cn_prop cn_array[] = {
	{
		.name = "lge,log_buffer_phy_addr",
		.type = CELL_U32,
	},
	{
		.name = "lge,sbl_delta_time",
		.type = CELL_U32,
	},
	{
		.name = "lge,lk_delta_time",
		.type = CELL_U64,
	},
};

int __init lge_init_dt_scan_chosen(unsigned long node, const char *uname,
							int depth, void *data)
{
	unsigned long len;
	int i;
	enum cn_prop_type type;
	char *p;
	uint32_t *u32;
	void *temp;

	if (depth != 1 || (strcmp(uname, "chosen") != 0
				&& strcmp(uname, "chosen@0") != 0))
		return 0;
	for (i = 0; i < cn_arr_len; i++) {
		type = cn_array[i].type;
		temp = of_get_flat_dt_prop(node, cn_array[i].name, &len);
		if (temp == NULL)
			continue;
		if (type == CELL_U32) {
			u32 = of_get_flat_dt_prop(node, cn_array[i].name, &len);
			if (u32 != NULL)
				cn_array[i].cell_u32 = of_read_ulong(u32, 1);
		} else if (type == CELL_U64) {
			u32 = of_get_flat_dt_prop(node, cn_array[i].name, &len);
			if (u32 != NULL)
				cn_array[i].cell_u64 = of_read_number(u32, 2);
		} else {
			p = of_get_flat_dt_prop(node, cn_array[i].name, &len);
			if (p != NULL)
				strlcpy(cn_array[i].str, p, len);
		}
		cn_array[i].is_valid = 1;
	}

	return 0;
}

void get_dt_cn_prop_u32(const char *name, uint32_t *u32)
{
	int i;
	for (i = 0; i < cn_arr_len; i++) {
		if (cn_array[i].is_valid &&
			!strcmp(name, cn_array[i].name)) {
			*u32 = cn_array[i].cell_u32;
			return;
		}
	}
	pr_err("The %s node have not property value\n", name);
}

void get_dt_cn_prop_u64(const char *name, uint64_t *u64)
{
	int i;
	for (i = 0; i < cn_arr_len; i++) {
		if (cn_array[i].is_valid &&
			!strcmp(name, cn_array[i].name)) {
			*u64 = cn_array[i].cell_u64;
			return;
		}
	}
	pr_err("The %s node have not property value\n", name);
}

void get_dt_cn_prop_str(const char *name, char *value)
{
	int i;
	for (i = 0; i < cn_arr_len; i++) {
		if (cn_array[i].is_valid &&
			!strcmp(name, cn_array[i].name)) {
			strlcpy(value, cn_array[i].str, strlen(cn_array[i].str));
			return;
		}
	}
	pr_err("The %s node have not property value\n", name);
}

#ifdef CONFIG_LGE_ECO_MODE
static struct platform_device lge_kernel_device = {
	.name = "lge_kernel_driver",
	.id = -1,
};

void __init lge_add_lge_kernel_devices(void)
{
	platform_device_register(&lge_kernel_device);
}
#endif

#ifdef CONFIG_LGE_QFPROM_INTERFACE
static struct platform_device qfprom_device = {
	.name = "lge-qfprom",
	.id = -1,
};

void __init lge_add_qfprom_devices(void)
{
	platform_device_register(&qfprom_device);
}
#endif

#ifdef CONFIG_LGE_ENABLE_MMC_STRENGTH_CONTROL
static struct platform_device lge_mmc0_strength_device = {
    .name = "lge_mmc0_strength_driver",
    .id = -1,
};

void __init lge_add_mmc0_strength_devices(void)
{
    platform_device_register(&lge_mmc0_strength_device);
}

static struct platform_device lge_mmc1_strength_device = {
    .name = "lge_mmc1_strength_driver",
    .id = -1,
};

void __init lge_add_mmc1_strength_devices(void)
{
    platform_device_register(&lge_mmc1_strength_device);
}
#endif

#ifdef CONFIG_LGE_DIAG_ENABLE_SYSFS
static struct platform_device lg_diag_cmd_device = {
	.name = "lg_diag_cmd",
	.id = -1,
	.dev    = {
		.platform_data = 0, /* &lg_diag_cmd_pdata */
	},
};

void __init lge_add_diag_devices(void)
{
	platform_device_register(&lg_diag_cmd_device);
}
#endif

#ifdef CONFIG_LGE_PM
void get_cable_data_from_dt(void *of_node)
{
	int i;
	u32 cable_value[3];
	struct device_node *node_temp = (struct device_node *)of_node;
	const char *propname[MAX_CABLE_NUM] = {
		"lge,no-init-cable",
		"lge,cable-mhl-1k",
		"lge,cable-u-28p7k",
		"lge,cable-28p7k",
		"lge,cable-56k",
		"lge,cable-100k",
		"lge,cable-130k",
		"lge,cable-180k",
		"lge,cable-200k",
		"lge,cable-220k",
		"lge,cable-270k",
		"lge,cable-330k",
		"lge,cable-620k",
		"lge,cable-910k",
		"lge,cable-none"
	};
	if (cable_type_defined) {
		pr_info("Cable type is already defined\n");
		return;
	}

	for (i = 0; i < MAX_CABLE_NUM; i++) {
		of_property_read_u32_array(node_temp, propname[i],
				cable_value, 3);
		lge_acc_cable_type_data[i].threshhold = cable_value[0];
		lge_acc_cable_type_data[i].type = i;
		lge_acc_cable_type_data[i].ta_ma = cable_value[1];
		lge_acc_cable_type_data[i].usb_ma = cable_value[2];
	}
	cable_type_defined = 1;
}

int lge_pm_get_cable_info(struct qpnp_vadc_chip *vadc,
		struct chg_cable_info *cable_info)
{
	char *type_str[] = {
		"NOT INIT", "MHL 1K", "U_28P7K", "28P7K", "56K",
		"100K", "130K", "180K", "200K", "220K",
		"270K", "330K", "620K", "910K", "OPEN"
	};

	struct qpnp_vadc_result result;
	struct chg_cable_info *info = cable_info;
	struct chg_cable_info_table *table;
	int table_size = ARRAY_SIZE(lge_acc_cable_type_data);
	int acc_read_value = 0;
	int i, rc;
	int count = 1;

	if (!info) {
		pr_err("%s : invalid info parameters\n", __func__);
		return -EINVAL;
	}

	if (!vadc) {
		pr_err("%s : invalid vadc parameters\n", __func__);
		return -EINVAL;
	}

	if (!cable_type_defined) {
		pr_err("%s : cable type is not defined yet.\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < count; i++) {
		rc = qpnp_vadc_read(vadc, LR_MUX10_USB_ID_LV, &result);
		if (rc < 0) {
			if (rc == -ETIMEDOUT) {
				/* reason: adc read timeout,
				 * assume it is open cable
				 */
				info->cable_type = CABLE_NONE;
				info->ta_ma = C_NONE_TA_MA;
				info->usb_ma = C_NONE_USB_MA;
			}
			pr_err("%s : adc read error - %d\n", __func__, rc);
			return rc;
		}

		acc_read_value = (int)result.physical;
		pr_info("%s : adc_read-%d\n", __func__, (int)result.physical);
		/* mdelay(10); */
	}

	info->cable_type = NO_INIT_CABLE;
	info->ta_ma = C_NO_INIT_TA_MA;
	info->usb_ma = C_NO_INIT_USB_MA;

	/* assume: adc value must be existed in ascending order */
	for (i = 0; i < table_size; i++) {
		table = &lge_acc_cable_type_data[i];

		if (acc_read_value <= table->threshhold) {
			info->cable_type = table->type;
			info->ta_ma = table->ta_ma;
			info->usb_ma = table->usb_ma;
			break;
		}
	}

	pr_err("\n\n[PM]Cable detected: %d(%s)(%d, %d)\n\n",
			acc_read_value, type_str[info->cable_type],
			info->ta_ma, info->usb_ma);

	return 0;
}

/* Belows are for using in interrupt context */
static struct chg_cable_info lge_cable_info;

acc_cable_type lge_pm_get_cable_type(void)
{
	return lge_cable_info.cable_type;
}

unsigned lge_pm_get_ta_current(void)
{
	return lge_cable_info.ta_ma;
}

unsigned lge_pm_get_usb_current(void)
{
	return lge_cable_info.usb_ma;
}

/* This must be invoked in process context */
void lge_pm_read_cable_info(struct qpnp_vadc_chip *vadc)
{
	lge_cable_info.cable_type = NO_INIT_CABLE;
	lge_cable_info.ta_ma = C_NO_INIT_TA_MA;
	lge_cable_info.usb_ma = C_NO_INIT_USB_MA;

	lge_pm_get_cable_info(vadc, &lge_cable_info);
}

struct pseudo_batt_info_type pseudo_batt_info = {
	.mode = 0,
};

void pseudo_batt_set(struct pseudo_batt_info_type *info)
{
	struct power_supply *batt_psy;

	pr_err("pseudo_batt_set\n");

	batt_psy = power_supply_get_by_name("battery");

	if (!batt_psy) {
		pr_err("called before init\n");
		return;
	}

	pseudo_batt_info.mode = info->mode;
	pseudo_batt_info.id = info->id;
	pseudo_batt_info.therm = info->therm;
	pseudo_batt_info.temp = info->temp;
	pseudo_batt_info.volt = info->volt;
	pseudo_batt_info.capacity = info->capacity;
	pseudo_batt_info.charging = info->charging;

	power_supply_changed(batt_psy);
}

#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
/* get panel maker ID from cmdline */
static unsigned lge_batt_id;

static int __init board_batt_id(char *batt_id)
{
	/* CAUTION : These strings are come from LK */
	if (!strcmp(batt_id, "DS2704_L"))
		lge_batt_id = BATT_ID_DS2704_L;
	else if (!strcmp(batt_id, "DS2704_C"))
		lge_batt_id = BATT_ID_DS2704_C;
	else if (!strcmp(batt_id, "DS2704_N"))
		lge_batt_id = BATT_ID_DS2704_N;
	else if (!strcmp(batt_id, "ISL6296_L"))
		lge_batt_id = BATT_ID_ISL6296_L;
	else if (!strcmp(batt_id, "ISL6296_C"))
		lge_batt_id = BATT_ID_ISL6296_C;
	else if (!strcmp(batt_id, "ISL6296_N"))
		lge_batt_id = BATT_ID_ISL6296_N;
	else
		lge_batt_id = BATT_ID_UNKNOWN;

	return 1;
}
__setup("lge.battid=", board_batt_id);

unsigned lge_get_batt_id(void)
{
	return lge_batt_id;
}
#endif

#endif

#if defined(CONFIG_LGE_KSWITCH)
static int kswitch_status;
#endif

/* setting whether uart console is enalbed or disabled */
#ifdef CONFIG_EARJACK_DEBUGGER
static unsigned int uart_console_mode;  /* Not initialized */
#else
static unsigned int uart_console_mode = 1;  /* Alway Off */
#endif

unsigned int lge_get_uart_mode(void)
{
#ifdef CONFIG_LGE_KSWITCH
	if ((kswitch_status & LGE_KSWITCH_UART_DISABLE) >> 3)
		uart_console_mode = 0;
#endif
	return uart_console_mode;
}

void lge_set_uart_mode(unsigned int um)
{
	uart_console_mode = um;
}

static int __init lge_uart_mode(char *uart_mode)
{
	if (!strncmp("enable", uart_mode, 6)) {
		pr_info("UART CONSOLE : enable\n");
		lge_set_uart_mode((UART_MODE_ALWAYS_ON_BMSK | UART_MODE_EN_BMSK)
				& ~UART_MODE_ALWAYS_OFF_BMSK);
	} else if (!strncmp("detected", uart_mode, 8)) {
		pr_info("UART CONSOLE : detected\n");
		lge_set_uart_mode(UART_MODE_EN_BMSK &
				~UART_MODE_ALWAYS_OFF_BMSK);
	} else {
		pr_info("UART CONSOLE : disable\n");
	}

	return 1;
}
__setup("lge.uart=", lge_uart_mode);

/* for supporting two LCD types with one image */
static bool cont_splash_enabled;

static int __init cont_splash_enabled_setup(char *enabled)
{
	/* INFO : argument string is from LK */
	if (!strncmp("true", enabled, 6))
		cont_splash_enabled = true;
	pr_info("cont splash enabled : %s\n", enabled);
	return 1;
}
__setup("cont_splash_enabled=", cont_splash_enabled_setup);

bool lge_get_cont_splash_enabled(void)
{
	return cont_splash_enabled;
}

#ifdef CONFIG_LGE_SUPPORT_LCD_MAKER_ID
/* get panel maker ID from cmdline */
static lcd_maker_id lge_panel_maker;

/* CAUTION : These strings are come from LK */
char *panel_maker[] = {"0", "1", "2"};

static int __init board_panel_maker(char *maker_id)
{
	int i;

	for (i = 0; i < LCD_MAKER_MAX; i++) {
		if (!strncmp(maker_id, panel_maker[i], 1)) {
			lge_panel_maker = (lcd_maker_id) i;
			break;
		}
	}

	pr_debug("MAKER : %s\n", panel_maker[lge_panel_maker]);
	return 1;
}
__setup("lcd_maker_id=", board_panel_maker);

lcd_maker_id lge_get_panel_maker(void)
{
	return lge_panel_maker;
}
#endif

/*
	for download complete using LAF image
	return value : 1 --> right after laf complete & reset
*/

int android_dlcomplete;

int __init lge_android_dlcomplete(char *s)
{
	if (strncmp(s, "1", 1) == 0)   /* if same string */
		android_dlcomplete = 1;
	else	/* not same string */
		android_dlcomplete = 0;
	pr_info("androidboot.dlcomplete = %d\n", android_dlcomplete);

	return 1;
}
__setup("androidboot.dlcomplete=", lge_android_dlcomplete);

int lge_get_android_dlcomplete(void)
{
	return android_dlcomplete;
}
/* get boot mode information from cmdline.
 * If any boot mode is not specified,
 * boot mode is normal type.
 */
static enum lge_boot_mode_type lge_boot_mode = LGE_BOOT_MODE_NORMAL;
int __init lge_boot_mode_init(char *s)
{
	if (!strcmp(s, "charger"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGER;
	else if (!strcmp(s, "chargerlogo"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGERLOGO;
	else if (!strcmp(s, "qem_56k"))
		lge_boot_mode = LGE_BOOT_MODE_QEM_56K;
	else if (!strcmp(s, "qem_130k"))
		lge_boot_mode = LGE_BOOT_MODE_QEM_130K;
	else if (!strcmp(s, "qem_910k"))
		lge_boot_mode = LGE_BOOT_MODE_QEM_910K;
	else if (!strcmp(s, "pif_56k"))
		lge_boot_mode = LGE_BOOT_MODE_PIF_56K;
	else if (!strcmp(s, "pif_130k"))
		lge_boot_mode = LGE_BOOT_MODE_PIF_130K;
	else if (!strcmp(s, "pif_910k"))
		lge_boot_mode = LGE_BOOT_MODE_PIF_910K;
	/*                            */
	else if (!strcmp(s, "miniOS"))
		lge_boot_mode = LGE_BOOT_MODE_MINIOS;
	pr_info("ANDROID BOOT MODE : %d %s\n", lge_boot_mode, s);
	/*                            */

	return 1;
}
__setup("androidboot.mode=", lge_boot_mode_init);

enum lge_boot_mode_type lge_get_boot_mode(void)
{
	return lge_boot_mode;
}

int lge_get_factory_boot(void)
{
	int res;

	/*   if boot mode is factory,
	 *   cable must be factory cable.
	 */
	switch (lge_boot_mode) {
	case LGE_BOOT_MODE_QEM_56K:
	case LGE_BOOT_MODE_QEM_130K:
	case LGE_BOOT_MODE_QEM_910K:
	case LGE_BOOT_MODE_PIF_56K:
	case LGE_BOOT_MODE_PIF_130K:
	case LGE_BOOT_MODE_PIF_910K:
	case LGE_BOOT_MODE_MINIOS:
		res = 1;
		break;
	default:
		res = 0;
		break;
	}
	return res;
}

/* for board revision */
static hw_rev_type lge_bd_rev = HW_REV_B;

/* CAUTION: These strings are come from LK. */
char *rev_str[] = {"evb1", "evb2", "rev_a", "rev_b", "rev_c", "rev_d",
	"rev_e", "rev_f", "rev_g", "rev_h", "rev_10", "rev_11", "rev_12",
	"revserved"};

static int __init board_revno_setup(char *rev_info)
{
	int i;

	for (i = 0; i < HW_REV_MAX; i++) {
		if (!strncmp(rev_info, rev_str[i], 6)) {
			lge_bd_rev = (hw_rev_type) i;
			/* it is defined externally in <asm/system_info.h> */
			system_rev = lge_bd_rev;
			break;
		}
	}

	pr_info("BOARD : LGE %s\n", rev_str[lge_bd_rev]);
	return 1;
}
__setup("lge.rev=", board_revno_setup);

hw_rev_type lge_get_board_revno(void)
{
	return lge_bd_rev;
}

int on_hidden_reset;

static int __init lge_check_hidden_reset(char *reset_mode)
{
	on_hidden_reset = 0;

	if (!strncmp(reset_mode, "on", 2))
		on_hidden_reset = 1;

	return 1;
}

__setup("lge.hreset=", lge_check_hidden_reset);


static int lge_frst_status;

int get_lge_frst_status(void){
	return lge_frst_status;
}

static int __init lge_check_frst(char *frst_status)
{
	lge_frst_status = 0;

	if (!strncmp(frst_status, "3", 1))
		lge_frst_status = 3;

	pr_info("lge_frst_status=%d\n", lge_frst_status);

	return 1;
}

__setup("lge.frst=", lge_check_frst);

static enum lge_laf_mode_type lge_laf_mode = LGE_LAF_MODE_NORMAL;

int __init lge_laf_mode_init(char *s)
{
	if (strcmp(s, ""))
		lge_laf_mode = LGE_LAF_MODE_LAF;

	return 1;
}
__setup("androidboot.laf=", lge_laf_mode_init);

enum lge_laf_mode_type lge_get_laf_mode(void)
{
	return lge_laf_mode;
}

#if defined(CONFIG_LGE_KSWITCH)
static int atoi(const char *name)
{
	int val = 0;

	for (;; name++) {
		switch (*name) {
		case '0' ... '9':
			val = 10*val+(*name-'0');
			break;
		default:
			return val;
		}
	}
}

static int __init kswitch_setup(char *value)
{
	kswitch_status = atoi(value);

	if (kswitch_status < 0)
		kswitch_status = 0;

	printk(KERN_INFO "[KSwitch] %d \n", kswitch_status);
	return 1;
}
__setup("kswitch=", kswitch_setup);

int lge_get_kswitch_status(void)
{
	return kswitch_status;
}
#endif

#ifdef CONFIG_LGE_FOTA_SILENT_RESET
static int lge_boot_reason = -1;
static int __init lge_check_bootreason(char *reason)
{
	int ret = 0;

	/* handle corner case of kstrtoint */
	if (!strcmp(reason, "0xffffffff")) {
		lge_boot_reason = 0xffffffff;
		return 1;
	}

	ret = kstrtoint(reason, 16, &lge_boot_reason);
	if (!ret)
		pr_info("LGE REBOOT REASON: %x\n", lge_boot_reason);
	else
		pr_info("LGE REBOOT REASON: Couldn't get bootreason -%d\n",
				ret);
	return 1;
}
__setup("lge.bootreason=", lge_check_bootreason);

int lge_get_bootreason(void)
{
	return lge_boot_reason;
}
#endif

#ifdef CONFIG_USB_G_LGE_ANDROID
static int get_factory_cable(void)
{
	int res = 0;

	/* if boot mode is factory, cable must be factory cable. */
	switch (lge_boot_mode) {
	case LGE_BOOT_MODE_QEM_56K:
	case LGE_BOOT_MODE_PIF_56K:
		res = LGEUSB_FACTORY_56K;
		break;

	case LGE_BOOT_MODE_QEM_130K:
	case LGE_BOOT_MODE_PIF_130K:
		res = LGEUSB_FACTORY_130K;
		break;

	case LGE_BOOT_MODE_QEM_910K:
	case LGE_BOOT_MODE_PIF_910K:
		res = LGEUSB_FACTORY_910K;
		break;

	default:
		res = 0;
		break;
	}

	return res;
}

struct lge_android_usb_platform_data lge_android_usb_pdata = {
	.vendor_id = 0x1004,
	.factory_pid = 0x6000,
	.iSerialNumber = 0,
	.product_name = "LGE Android Phone",
	.manufacturer_name = "LG Electronics Inc.",
	.factory_composition = "acm,diag",
	.get_factory_cable = get_factory_cable,
};

struct platform_device lge_android_usb_device = {
	.name = "lge_android_usb",
	.id = -1,
	.dev = {
		.platform_data = &lge_android_usb_pdata,
	},
};

void __init lge_add_android_usb_devices(void)
{
	platform_device_register(&lge_android_usb_device);
}
#endif

#ifdef CONFIG_LGE_DIAG_USB_ACCESS_LOCK
static struct platform_device lg_diag_cmd_device = {
	.name = "lg_diag_cmd",
	.id = -1,
	.dev    = {
		.platform_data = 0, /* &lg_diag_cmd_pdata */
	},
};

static int __init lge_diag_devices_init(void)
{
	return platform_device_register(&lg_diag_cmd_device);
}
arch_initcall(lge_diag_devices_init);
#endif
