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


static int	VM_MEMORY_CACHED(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef HAVE_SYS_SYSCTL_H
	size_t len = 0;
	unsigned int val;
	zbx_uint64_t res;

	len = sizeof (val);
	if (sysctlbyname ("vm.stats.vm.v_cache_count", &val, &len, NULL, 0) == -1 || !len)
		return SYSINFO_RET_FAIL;
	else {
		res = getpagesize ();
		res *= val;
		SET_UI64_RESULT (result, res);
		return SYSINFO_RET_OK;
	}
#elif defined(HAVE_PROC)
        FILE    *f = NULL;
        char    *t;
        char    c[MAX_STRING_LEN];
	zbx_uint64_t    res = 0;

        assert(result);

        init_result(result);

        if(NULL == (f = fopen("/proc/meminfo","r")))
        {
                return  SYSINFO_RET_FAIL;
        }

        while(NULL != fgets(c,MAX_STRING_LEN,f))
        {
                if(strncmp(c,"Cached:",7) == 0)
                {
                        t=(char *)strtok(c," ");
                        t=(char *)strtok(NULL," ");
                        sscanf(t, ZBX_FS_UI64, &res );
                        t=(char *)strtok(NULL," ");

                        if(strcasecmp(t,"kb"))          res <<= 10;
                        else if(strcasecmp(t, "mb"))    res <<= 20;
                        else if(strcasecmp(t, "gb"))    res <<= 30;
                        else if(strcasecmp(t, "tb"))    res <<= 40;

                        break;
                }
        }

        zbx_fclose(f);

        SET_UI64_RESULT(result, res);
        return SYSINFO_RET_OK;
#else
	assert(result);

        init_result(result);
		
	return SYSINFO_RET_FAIL;
#endif
}

static int	VM_MEMORY_BUFFERS(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	assert(result);

        init_result(result);
		
#ifdef HAVE_SYSINFO_BUFFERRAM
	struct sysinfo info;

	if( 0 == sysinfo(&info))
	{
#ifdef HAVE_SYSINFO_MEM_UNIT
		SET_UI64_RESULT(result, (zbx_uint64_t)info.bufferram * (zbx_uint64_t)info.mem_unit);
#else
		SET_UI64_RESULT(result, info.bufferram);
#endif
		return SYSINFO_RET_OK;
	}
	else
	{
		return SYSINFO_RET_FAIL;
	}
#elif defined(HAVE_SYS_SYSCTL_H)
	size_t len = 0;
	unsigned int val;
	zbx_uint64_t res = 0;

	len = sizeof (val);
	if (sysctlbyname ("vfs.bufspace", &val, &len, NULL, 0) == -1 || !len)
		return SYSINFO_RET_FAIL;
	else {
		res = val;
		SET_UI64_RESULT (result, res);
		return SYSINFO_RET_OK;
	}
#else
	return	SYSINFO_RET_FAIL;
#endif
}

static int	VM_MEMORY_SHARED(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	assert(result);

        init_result(result);
		
#if defined(HAVE_SYS_VMMETER_VMTOTAL)
	int mib[2],len;
	struct vmtotal v;
	zbx_uint64_t res;

	len=sizeof(struct vmtotal);
	mib[0]=CTL_VM;
	mib[1]=VM_METER;

	sysctl(mib,2,&v,&len,NULL,0);
	res = getpagesize ();
	res *= v.t_armshr;

	SET_UI64_RESULT(result, res);
	return SYSINFO_RET_OK;
#elif defined(HAVE_SYSINFO_SHAREDRAM)
	struct sysinfo info;

	if( 0 == sysinfo(&info))
	{
#ifdef HAVE_SYSINFO_MEM_UNIT
		SET_UI64_RESULT(result, (zbx_uint64_t)info.sharedram * (zbx_uint64_t)info.mem_unit);
#else
		SET_UI64_RESULT(result, info.sharedram);
#endif
		return SYSINFO_RET_OK;
	}
	else
	{
		return SYSINFO_RET_FAIL;
	}
#else
	return	SYSINFO_RET_FAIL;
#endif
}

static int	VM_MEMORY_TOTAL(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	assert(result);

        init_result(result);
		
#if defined(HAVE_SYS_VMMETER_VMTOTAL)
	int mib[2];
	size_t len;
	zbx_uint64_t total = 0;

	len = sizeof(total);
	mib[0]=CTL_HW;
	mib[1]=HW_PHYSMEM;
	sysctl(mib, 2, &total, &len, NULL, 0);

	SET_UI64_RESULT(result, total);
	return SYSINFO_RET_OK;
#elif defined(HAVE_SYS_PSTAT_H)
	struct	pst_static pst;
	long	page;

	if(pstat_getstatic(&pst, sizeof(pst), (size_t)1, 0) == -1)
	{
		return SYSINFO_RET_FAIL;
	}
	else
	{
		/* Get page size */	
		page = pst.page_size;
		/* Total physical memory in bytes */	
		SET_UI64_RESULT(result, (zbx_uint64_t)page*(zbx_uint64_t)pst.physical_memory);
		return SYSINFO_RET_OK;
	}
#elif defined(HAVE_SYSINFO_TOTALRAM)
	struct sysinfo info;

	if( 0 == sysinfo(&info))
	{
#ifdef HAVE_SYSINFO_MEM_UNIT
		SET_UI64_RESULT(result, (zbx_uint64_t)info.totalram * (zbx_uint64_t)info.mem_unit);
#else
		SET_UI64_RESULT(result, info.totalram);
#endif
		return SYSINFO_RET_OK;
	}
	else
	{
		return SYSINFO_RET_FAIL;
	}
#elif defined(HAVE_SYS_SYSCTL_H)
	static int mib[] = { CTL_HW, HW_PHYSMEM };
	size_t len;
	unsigned int memory;
	zbx_uint64_t res;
	int ret;
	
	len=sizeof(memory);

	if(0==sysctl(mib,2,&memory,&len,NULL,0))
	{
		res = memory;
		SET_UI64_RESULT(result, res);
		ret=SYSINFO_RET_OK;
	}
	else
	{
		ret=SYSINFO_RET_FAIL;
	}
	return ret;
#else
	return	SYSINFO_RET_FAIL;
#endif
}

static int	VM_MEMORY_FREE(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	assert(result);

        init_result(result);
		
#if defined(HAVE_SYS_VMMETER_VMTOTAL)
	int mib[2],len;
	zbx_uint64_t res = getpagesize ();
	struct vmtotal v;

	len=sizeof(struct vmtotal);
	mib[0]=CTL_VM;
	mib[1]=VM_METER;

	sysctl(mib,2,&v,&len,NULL,0);

	res *= v.t_free;
	SET_UI64_RESULT(result, res);
	return SYSINFO_RET_OK;
#elif defined(HAVE_SYS_PSTAT_H)
	struct	pst_static pst;
	struct	pst_dynamic dyn;
	long	page;

	if(pstat_getstatic(&pst, sizeof(pst), (size_t)1, 0) == -1)
	{
		return SYSINFO_RET_FAIL;
	}
	else
	{
		/* Get page size */	
		page = pst.page_size;
/*		return pst.physical_memory;*/

		if (pstat_getdynamic(&dyn, sizeof(dyn), 1, 0) == -1)
		{
			return SYSINFO_RET_FAIL;
		}
		else
		{
/*		cout<<"total virtual memory allocated is " << dyn.psd_vm << "
		pages, " << dyn.psd_vm * page << " bytes" << endl;
		cout<<"active virtual memory is " << dyn.psd_avm <<" pages, " <<
		dyn.psd_avm * page << " bytes" << endl;
		cout<<"total real memory is " << dyn.psd_rm << " pages, " <<
		dyn.psd_rm * page << " bytes" << endl;
		cout<<"active real memory is " << dyn.psd_arm << " pages, " <<
		dyn.psd_arm * page << " bytes" << endl;
		cout<<"free memory is " << dyn.psd_free << " pages, " <<
*/
		/* Free memory in bytes */

			SET_UI64_RESULT(result, (zbx_uint64_t)dyn.psd_free * (zbx_uint64_t)page);
			return SYSINFO_RET_OK;
		}
	}
#elif defined(HAVE_SYSINFO_FREERAM)
	struct sysinfo info;

	if( 0 == sysinfo(&info))
	{
#ifdef HAVE_SYSINFO_MEM_UNIT
		SET_UI64_RESULT(result, (zbx_uint64_t)info.freeram * (zbx_uint64_t)info.mem_unit);
#else	
		SET_UI64_RESULT(result, info.freeram);
#endif
		return SYSINFO_RET_OK;
	}
	else
	{
		return SYSINFO_RET_FAIL;
	}
/* OS/X */
#elif defined(HAVE_MACH_HOST_INFO_H)
	vm_statistics_data_t page_info;
	vm_size_t pagesize;
	mach_msg_type_number_t count;
	kern_return_t kret;
	int ret;
	
	pagesize = 0;
	kret = host_page_size (mach_host_self(), &pagesize);

	count = HOST_VM_INFO_COUNT;
	kret = host_statistics (mach_host_self(), HOST_VM_INFO,
	(host_info_t)&page_info, &count);
	if (kret == KERN_SUCCESS)
	{
		double pw, pa, pi, pf, pu;

		pw = (double)page_info.wire_count*pagesize;
		pa = (double)page_info.active_count*pagesize;
		pi = (double)page_info.inactive_count*pagesize;
		pf = (double)page_info.free_count*pagesize;

		pu = pw+pa+pi;

		SET_UI64_RESULT(result, pf);
		ret = SYSINFO_RET_OK;
	}
	else
	{
		ret = SYSINFO_RET_FAIL;
	}
	return ret;
#else
	return	SYSINFO_RET_FAIL;
#endif
}

static int      VM_MEMORY_PFREE(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	AGENT_RESULT	result_tmp;
	zbx_uint64_t	tot_val = 0;
	zbx_uint64_t	free_val = 0;

	assert(result);

	init_result(result);
	init_result(&result_tmp);

	if(VM_MEMORY_TOTAL(cmd, param, flags, &result_tmp) != SYSINFO_RET_OK ||
		!(result_tmp.type & AR_UINT64))
			return  SYSINFO_RET_FAIL;
	tot_val = result_tmp.ui64;

	/* Check fot division by zero */
	if(tot_val == 0)
	{
		free_result(&result_tmp);
		return  SYSINFO_RET_FAIL;
	}

	if(VM_MEMORY_FREE(cmd, param, flags, &result_tmp) != SYSINFO_RET_OK ||
		!(result_tmp.type & AR_UINT64))
			return  SYSINFO_RET_FAIL;
	free_val = result_tmp.ui64;

	free_result(&result_tmp);

	SET_DBL_RESULT(result, (100.0 * (double)free_val) / (double)tot_val);

	return SYSINFO_RET_OK;
}

int     VM_MEMORY_SIZE(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#define MEM_FNCLIST struct mem_fnclist_s
MEM_FNCLIST
{
	char *mode;
	int (*function)();
};

	MEM_FNCLIST fl[] = 
	{
		{"free",	VM_MEMORY_FREE},
		{"pfree",       VM_MEMORY_PFREE},
		{"shared",	VM_MEMORY_SHARED},
		{"total",	VM_MEMORY_TOTAL},
		{"buffers",	VM_MEMORY_BUFFERS},
		{"cached",	VM_MEMORY_CACHED},
		{0,	0}
	};
        char    mode[MAX_STRING_LEN];
	int i;

        assert(result);

        init_result(result);

        if(num_param(param) > 1)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, mode, sizeof(mode)) != 0)
        {
                mode[0] = '\0';
        }

        if(mode[0] == '\0')
	{
		/* default parameter */
		zbx_snprintf(mode, sizeof(mode), "total");
	}
	
	for(i=0; fl[i].mode!=0; i++)
	{
		if(strncmp(mode, fl[i].mode, MAX_STRING_LEN)==0)
		{
			return (fl[i].function)(cmd, param, flags, result);
		}
	}
	
	return SYSINFO_RET_FAIL;
}

