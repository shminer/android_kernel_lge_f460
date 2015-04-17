obj-$(CONFIG_MACH_APQ8084_LGPC9) += board-8084-lgpc9.o board-8084-lgpc9-gpiomux.o
obj-$(CONFIG_MACH_APQ8084_T6_LGU) += board-8084-t6-kr.o board-8084-t6-kr-gpiomux.o
obj-$(CONFIG_MACH_APQ8084_T6_KT) += board-8084-t6-kr.o board-8084-t6-kr-gpiomux.o
obj-$(CONFIG_MACH_APQ8084_T6_SKT) += board-8084-t6-kr.o board-8084-t6-kr-gpiomux.o
obj-$(CONFIG_WIFI_CONTROL_FUNC) += wifi_bcm_lge.o

obj-$(CONFIG_MACH_LGE) += devices_lge.o

obj-$(CONFIG_LGE_BOOTLOADER_LOG) += lge_bootloader_log.o
obj-$(CONFIG_LGE_HANDLE_PANIC) += lge_handle_panic.o
obj-$(CONFIG_LGE_BOOTLOADER_TIME_CHECKER) += lge_bootloader_time_checker.o
obj-$(CONFIG_LGE_ECO_MODE) += lge_kernel_driver.o
obj-$(CONFIG_LGE_BLUETOOTH) += board-8084-bt-rfkill.o
obj-$(CONFIG_LGE_QFPROM_INTERFACE) += lge_qfprom_access.o
obj-$(CONFIG_LGE_PM) += lge_charging_scenario.o
obj-$(CONFIG_LGE_PM) += lge_gpio_debug.o
obj-$(CONFIG_LGE_PM) += lge_monitor_thermal.o
obj-$(CONFIG_HAS_EARLYSUSPEND) += lge_suspend_autotest.o
obj-$(CONFIG_TBD) += lge_blocking_monitor.o
obj-$(CONFIG_LGE_MSM_HSIC_TTY) += hsic_tty.o
obj-$(CONFIG_LGE_ENABLE_MMC_STRENGTH_CONTROL) += lge_mmc0_strength_control.o
obj-$(CONFIG_LGE_ENABLE_MMC_STRENGTH_CONTROL) += lge_mmc1_strength_control.o

CFLAGS_devices_lge.o += -Idrivers/staging/android
