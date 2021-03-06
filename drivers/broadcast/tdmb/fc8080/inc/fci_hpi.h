/*****************************************************************************
	Copyright(c) 2013 FCI Inc. All Rights Reserved

	File name : fci_hpi.h

	Description : tuner interface header file

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA


	History :
	----------------------------------------------------------------------
*******************************************************************************/

#ifndef __FCI_HPI_H__
#define __FCI_HPI_H__

#include "fci_types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern fci_s32 fci_hpi_init(HANDLE handle, fci_s32 speed, fci_s32 slaveaddr);
extern fci_s32 fci_hpi_read(
		HANDLE handle, fci_u8 chip, fci_u8 addr, fci_u8 alen,
		fci_u8 *data, fci_u8 len);
extern fci_s32 fci_hpi_write(
		HANDLE handle, fci_u8 chip, fci_u8 addr, fci_u8 alen,
		fci_u8 *data, fci_u8 len);
extern fci_s32 fci_hpi_deinit(HANDLE handle);

#ifdef __cplusplus
}
#endif

#endif /* __FCI_HPI_H__ */

