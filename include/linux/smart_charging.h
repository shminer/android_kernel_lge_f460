/*
 * LGE Smart charging Header file.
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

#ifndef __LGE_SMART_CHARGING_H_
#define __LGE_SMART_CHARGING_H_

#define AMC_CONTROL_IOCTL_MAGIC      'c'
#define AMC_CHG_CONTROL              _IOW(AMC_CONTROL_IOCTL_MAGIC, 0, int)
#define AMC_CONTROL_IOCTL_MAX        1 //max index

#endif
