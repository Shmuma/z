/*
** ZABBIX
** Copyright (C) 2008 Max Lapan <max.lapan@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**/
#ifndef __HISTORY_FS__
#define __HISTORY_FS__

#include "common.h"


void		HFSadd_history (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, double value, int clock);
void		HFSadd_history_uint (const char* hfs_base_dir, zbx_uint64_t itemid, unsigned int delay, zbx_uint64_t value, int clock);

zbx_uint64_t	HFS_get_count (const char* hfs_base_dir, zbx_uint64_t itemid, int from);
zbx_uint64_t	HFS_get_count_u64_eq (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_ne (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_gt (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_lt (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_ge (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value);
zbx_uint64_t	HFS_get_count_u64_le (const char* hfs_base_dir, zbx_uint64_t itemid, int from, zbx_uint64_t value);

zbx_uint64_t	HFS_get_count_float_eq (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_ne (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_gt (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_lt (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_ge (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value);
zbx_uint64_t	HFS_get_count_float_le (const char* hfs_base_dir, zbx_uint64_t itemid, int from, double value);

zbx_uint64_t	HFS_get_sum_sec_u64 (const char* hfs_base_dir, zbx_uint64_t itemid, int from);
double		HFS_get_sum_sec_float (const char* hfs_base_dir, zbx_uint64_t itemid, int from);
zbx_uint64_t	HFS_get_sum_vals_u64 (const char* hfs_base_dir, zbx_uint64_t itemid, int count);
double		HFS_get_sum_vals_float (const char* hfs_base_dir, zbx_uint64_t itemid, int count);

#endif
