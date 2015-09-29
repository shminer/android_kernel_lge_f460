#define MSM_LIMIT			"msm_limiter"
#define LIMITER_ENABLED			1
#define DEFAULT_SUSPEND_DEFER_TIME	10
#define DEFAULT_SUSPEND_FREQUENCY	1497600

#if defined(CONFIG_ARCH_APQ8084)
#define DEFAULT_RESUME_FREQUENCY	2649600
#else
#define DEFAULT_RESUME_FREQUENCY	2265600
#endif
#define DEFAULT_MIN_FREQUENCY		300000

static struct cpu_limit {
	unsigned int limiter_enabled;
	uint32_t suspend_max_freq;
	uint32_t limit_max_freq;
	uint32_t limit_min_freq;
	uint32_t limit_max_ori;
	unsigned int suspended;
	unsigned int suspend_defer_time;
	struct delayed_work suspend_work;
	struct work_struct resume_work;
	struct mutex resume_suspend_mutex;
	struct mutex msm_limiter_mutex[4];
	struct notifier_block notif;
} limit = {
	.limiter_enabled = LIMITER_ENABLED,
	.suspend_max_freq = DEFAULT_SUSPEND_FREQUENCY,
	.limit_max_ori = DEFAULT_RESUME_FREQUENCY,
	.limit_max_freq = DEFAULT_RESUME_FREQUENCY,
	.limit_min_freq = DEFAULT_MIN_FREQUENCY,
	.suspend_defer_time = DEFAULT_SUSPEND_DEFER_TIME,
};
