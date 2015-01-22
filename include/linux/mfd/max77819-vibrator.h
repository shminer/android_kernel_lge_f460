/*
 * Vibrator driver for Maxim MAX77819
 *
 * Copyright (C) 2013 Maxim Integrated Product
 * Gyungoh Yoo <jack.yoo@maximintegrated.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __MAX77819_VIBRATOR_H__
#define __MAX77819_VIBRATOR_H__

struct max77819_vib_platform_data {
	int vib_current;	/* 1000uA to 140000uA. See the datasheet */
};

#endif /* __MAX77819_VIBRATOR_H__ */
