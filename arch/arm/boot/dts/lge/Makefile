ifeq ($(CONFIG_OF),y)

dtb-$(CONFIG_MACH_APQ8084_LGPC9)         += apq8084-lgpc9.dtb
dtb-$(CONFIG_MACH_APQ8084_T6_LGU)        += apq8084-t6-kr.dtb
dtb-$(CONFIG_MACH_APQ8084_T6_LGU)        += apq8084-v1-1-t6-kr.dtb
dtb-$(CONFIG_MACH_APQ8084_T6_KT)         += apq8084-t6-kr.dtb
dtb-$(CONFIG_MACH_APQ8084_T6_KT)         += apq8084-v1-1-t6-kr.dtb
dtb-$(CONFIG_MACH_APQ8084_T6_SKT)        += apq8084-t6-kr.dtb
dtb-$(CONFIG_MACH_APQ8084_T6_SKT)        += apq8084-v1-1-t6-kr.dtb

DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif

targets += dtbs
endif

DTS_TARGET := $(patsubst "%",%,$(call CONFIG_DTS_TARGET,$(1)))

$(obj)/../%.dtb: $(src)/$(DTS_TARGET)/%.dts
	$(call if_changed_dep,dtc)

dtbs: $(addprefix $(obj)/../,$(DTB_LIST))

clean-files := *.dtb
