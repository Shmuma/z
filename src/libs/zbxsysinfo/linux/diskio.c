/* 
** ZABBIX
** Copyright (C) 2000-2005 SIA Zabbix
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

#include "common.h"
#include "sysinfo.h"

#include "../common/common.h"

struct disk_stat_s {
	zbx_uint64_t rio;
	zbx_uint64_t rsect;
	zbx_uint64_t wio;
	zbx_uint64_t wsect;
};

#if defined(KERNEL_2_4)
#	define INFO_FILE_NAME	"/proc/partitions"
#	define PARSE(line)	if(sscanf(line,"%*d %*d %*d %s " \
					ZBX_FS_UI64 " %*d " ZBX_FS_UI64 " %*d " \
					ZBX_FS_UI64 " %*d " ZBX_FS_UI64 " %*d %*d %*d %*d", \
				name, 			/* name */ \
				&(result->rio), 	/* rio */ \
				&(result->rsect),	/* rsect */ \
				&(result->wio), 	/* rio */ \
				&(result->wsect)	/* wsect */ \
				) != 5) continue
#else
#	define INFO_FILE_NAME	"/proc/diskstats"
#	define PARSE(line)	if(sscanf(line, "%*d %*d %s " \
					ZBX_FS_UI64 " %*d " ZBX_FS_UI64 " %*d " \
					ZBX_FS_UI64 " %*d " ZBX_FS_UI64 " %*d %*d %*d %*d", \
				name, 			/* name */ \
				&(result->rio), 	/* rio */ \
				&(result->rsect),	/* rsect */ \
				&(result->wio), 	/* wio */ \
				&(result->wsect)	/* wsect */ \
				) != 5)  \
					if(sscanf(line,"%*d %*d %s " \
						ZBX_FS_UI64 " " ZBX_FS_UI64 " " \
						ZBX_FS_UI64 " " ZBX_FS_UI64, \
					name, 			/* name */ \
					&(result->rio), 	/* rio */ \
					&(result->rsect),	/* rsect */ \
					&(result->wio), 	/* wio */ \
					&(result->wsect)	/* wsect */ \
					) != 5) continue
#endif

static int get_disk_stat(const char *interface, struct disk_stat_s *result)
{
	int ret = SYSINFO_RET_FAIL;
	char line[MAX_STRING_LEN];

	char name[MAX_STRING_LEN];
	
	FILE *f;

	assert(result);

	if(NULL != (f = fopen(INFO_FILE_NAME,"r") ))
	{
		while(fgets(line,MAX_STRING_LEN,f) != NULL)
		{
			PARSE(line);
		
			if(strncmp(name, interface, MAX_STRING_LEN) == 0)
			{
				ret = SYSINFO_RET_OK;
				break;
			}
		}
		zbx_fclose(f);
	}

	if(ret != SYSINFO_RET_OK)
	{
		memset(result, 0, sizeof(struct disk_stat_s));
	}
	
	return ret;
}

int	VFS_DEV_WRITE(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	struct disk_stat_s ds;
	char devname[MAX_STRING_LEN];
	char mode[MAX_STRING_LEN];
	int ret = SYSINFO_RET_FAIL;
	
        assert(result);

        init_result(result);
	
        if(num_param(param) > 2)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, devname, sizeof(devname)) != 0)
        {
                return SYSINFO_RET_FAIL;
        }
	
	if(get_param(param, 2, mode, sizeof(mode)) != 0)
        {
                mode[0] = '\0';
        }
	
        if(mode[0] == '\0')
	{
		/* default parameter */
		zbx_snprintf(mode, sizeof(mode), "sectors");
	}
	
	ret = get_disk_stat(devname, &ds);

	if(ret == SYSINFO_RET_OK)
	{
		if(strncmp(mode, "sectors", MAX_STRING_LEN)==0)
		{
			SET_UI64_RESULT(result, ds.wsect);
			ret = SYSINFO_RET_OK;
		}
		else if(strncmp(mode, "operations", MAX_STRING_LEN)==0)
		{
			SET_UI64_RESULT(result, ds.wio);
			ret = SYSINFO_RET_OK;
		}
		else
		{
			ret = SYSINFO_RET_FAIL;
		}
	}
	
	return ret;
}

int	VFS_DEV_READ(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	struct disk_stat_s ds;
	char devname[MAX_STRING_LEN];
	char mode[MAX_STRING_LEN];
	int ret = SYSINFO_RET_FAIL;
	
        assert(result);

        init_result(result);
	
        if(num_param(param) > 2)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, devname, sizeof(devname)) != 0)
        {
                return SYSINFO_RET_FAIL;
        }
	
	if(get_param(param, 2, mode, sizeof(mode)) != 0)
        {
                mode[0] = '\0';
        }
	
        if(mode[0] == '\0')
	{
		/* default parameter */
		zbx_snprintf(mode, sizeof(mode), "sectors");
	}
	
	ret = get_disk_stat(devname, &ds);

	if(ret == SYSINFO_RET_OK)
	{
		if(strncmp(mode, "sectors", MAX_STRING_LEN)==0)
		{
			SET_UI64_RESULT(result, ds.rsect);
			ret = SYSINFO_RET_OK;
		}
		else if(strncmp(mode, "operations", MAX_STRING_LEN)==0)
		{
			SET_UI64_RESULT(result, ds.rio);
			ret = SYSINFO_RET_OK;
		}
		else
		{
			ret = SYSINFO_RET_FAIL;
		}
	}
	
	return ret;
}

static int	DISK_IO(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef	HAVE_PROC
	return	getPROC("/proc/stat",2,2, flags, result);
#else
	return	SYSINFO_RET_FAIL;
#endif
}

static int	DISK_RIO(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef	HAVE_PROC
	return	getPROC("/proc/stat",3,2, flags, result);
#else
	return	SYSINFO_RET_FAIL;
#endif
}

static int	DISK_WIO(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef	HAVE_PROC
	return	getPROC("/proc/stat",4,2, flags, result);
#else
	return	SYSINFO_RET_FAIL;
#endif
}

static int	DISK_RBLK(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef	HAVE_PROC
	return	getPROC("/proc/stat",5,2, flags, result);
#else
	return	SYSINFO_RET_FAIL;
#endif
}

static int	DISK_WBLK(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef	HAVE_PROC
	return	getPROC("/proc/stat",6,2, flags, result);
#else
	return	SYSINFO_RET_FAIL;
#endif
}

int	OLD_IO(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	char    key[MAX_STRING_LEN];
	int 	ret;

	assert(result);

        init_result(result);

        if(num_param(param) > 1)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, key, MAX_STRING_LEN) != 0)
        {
                return SYSINFO_RET_FAIL;
        }

	if(strcmp(key,"disk_io") == 0)
	{
		ret = DISK_IO(cmd, param, flags, result);
	}
	else if(strcmp(key,"disk_rio") == 0)
	{
		ret = DISK_RIO(cmd, param, flags, result);
	}
	else if(strcmp(key,"disk_wio") == 0)
	{
		ret = DISK_WIO(cmd, param, flags, result);
	}
    	else if(strcmp(key,"disk_rblk") == 0)
	{
		ret = DISK_RBLK(cmd, param, flags, result);
	}
    	else if(strcmp(key,"disk_wblk") == 0)
	{
		ret = DISK_WBLK(cmd, param, flags, result);
	}
	else
	{
		ret = SYSINFO_RET_FAIL;
	}
    
	return ret;
}

#define BIO_STAT "/proc/stat"
#define BIO_VMSTAT "/proc/vmstat"

#define BIO_IN  1
#define BIO_OUT 2

int get_bio_stat(int type, zbx_uint64_t *res)
{
	int ret = SYSINFO_RET_FAIL;
	char line[MAX_STRING_LEN];
	zbx_uint64_t pages_in;
	FILE *f;

	if((f = fopen(BIO_STAT,"r")) != NULL)
	{
		while(fgets(line,MAX_STRING_LEN,f) != NULL)
		{
			if(sscanf(line,"page " ZBX_FS_UI64 " " ZBX_FS_UI64, &pages_in, res) == 2) {
				if (type == BIO_IN)
					*res = pages_in;

				ret = SYSINFO_RET_OK;
				break;
			}	
		}
		zbx_fclose(f);

		if (ret == SYSINFO_RET_OK)
			return ret;
	}

	if ((f = fopen(BIO_VMSTAT,"r")) != NULL)
	{
		while(fgets(line,MAX_STRING_LEN,f) != NULL)
		{
			if (type == BIO_IN &&
			    sscanf(line,"pgpgin " ZBX_FS_UI64, res) == 1)
			{
				ret = SYSINFO_RET_OK;
				break;
			}

			if (type == BIO_OUT &&
			    sscanf(line,"pgpgout " ZBX_FS_UI64, res) == 1)
			{
				ret = SYSINFO_RET_OK;
				break;
			}
		}
		zbx_fclose(f);
	}

	return ret;
}

int	SYSTEM_BIO_IN(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	int ret = SYSINFO_RET_FAIL;
	zbx_uint64_t in;

	assert(result);
	init_result(result);

	if((ret = get_bio_stat(BIO_IN, &in)) == SYSINFO_RET_OK)
		SET_UI64_RESULT(result, in);

	return ret;
}

int	SYSTEM_BIO_OUT(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	int ret = SYSINFO_RET_FAIL;
	zbx_uint64_t out;

	assert(result);
	init_result(result);

	if((ret = get_bio_stat(BIO_OUT, &out)) == SYSINFO_RET_OK)
		SET_UI64_RESULT(result, out);

	return ret;
}
