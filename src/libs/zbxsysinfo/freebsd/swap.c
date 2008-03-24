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

#include <sys/types.h>
#include <sys/vmmeter.h>

#ifdef HAVE_KVM_H
    #include <unistd.h>
    #include <kvm.h>
#endif

#include "common.h"

#include "sysinfo.h"

#include "md5.h"


/* Solaris. */
#ifndef HAVE_SYSINFO_FREESWAP
#ifdef HAVE_SYS_SWAP_SWAPTABLE
void get_swapinfo(double *total, double *fr)
{
	register int cnt, i, page_size;
/* Support for >2Gb */
/*	register int t, f;*/
	double	t, f;
	struct swaptable *swt;
	struct swapent *ste;
	static char path[256];

	/* get total number of swap entries */
	cnt = swapctl(SC_GETNSWP, 0);

	/* allocate enough space to hold count + n swapents */
	swt = (struct swaptable *)malloc(sizeof(int) +
		cnt * sizeof(struct swapent));

	if (swt == NULL)
	{
		*total = 0;
		*fr = 0;
		return;
	}
	swt->swt_n = cnt;

/* fill in ste_path pointers: we don't care about the paths, so we
point them all to the same buffer */
	ste = &(swt->swt_ent[0]);
	i = cnt;
	while (--i >= 0)
	{
		ste++->ste_path = path;
	}

	/* grab all swap info */
	swapctl(SC_LIST, swt);

	/* walk thru the structs and sum up the fields */
	t = f = 0;
	ste = &(swt->swt_ent[0]);
	i = cnt;
	while (--i >= 0)
	{
		/* dont count slots being deleted */
		if (!(ste->ste_flags & ST_INDEL) &&
		!(ste->ste_flags & ST_DOINGDEL))
		{
			t += ste->ste_pages;
			f += ste->ste_free;
		}
		ste++;
	}

	page_size=getpagesize();

	/* fill in the results */
	*total = page_size*t;
	*fr = page_size*f;
	free(swt);
}
#endif
#endif

static int	SYSTEM_SWAP_FREE(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef HAVE_KVM_H
	kvm_t *kd;
	int page_size, nfree = 0;
	struct kvm_swap swap[1];

	assert(result);

        init_result(result);

	page_size = getpagesize();

	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open")) == NULL)
    	    return SYSINFO_RET_FAIL;

	if (kvm_getswapinfo(kd, swap, 1, 0) == -1) {
		kvm_close(kd);
		return SYSINFO_RET_FAIL;
	}

	if((kvm_close(kd)) == -1)
		return SYSINFO_RET_FAIL;

	if (swap[0].ksw_total > 0)
	        nfree = ((swap[0].ksw_total - swap[0].ksw_used) * page_size);

	SET_UI64_RESULT(result, (zbx_uint64_t) nfree);
	return SYSINFO_RET_OK;
#endif
#ifdef HAVE_SYSINFO_FREESWAP
	struct sysinfo info;

	assert(result);

        init_result(result);

	if( 0 == sysinfo(&info))
	{
#ifdef HAVE_SYSINFO_MEM_UNIT
		SET_UI64_RESULT(result, (zbx_uint64_t)info.freeswap * (zbx_uint64_t)info.mem_unit);
#else
		SET_UI64_RESULT(result, info.freeswap);
#endif
		return SYSINFO_RET_OK;
	}
	else
	{
		return SYSINFO_RET_FAIL;
	}
/* Solaris */
#else
#ifdef HAVE_SYS_SWAP_SWAPTABLE
	double swaptotal,swapfree;

	assert(result);

        init_result(result);

	get_swapinfo(&swaptotal,&swapfree);

	SET_UI64_RESULT(result, swapfree);
	return SYSINFO_RET_OK;
#else
	assert(result);

        init_result(result);

	return	SYSINFO_RET_FAIL;
#endif
#endif
}

static int	SYSTEM_SWAP_TOTAL(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef HAVE_KVM_H
	kvm_t *kd;
	int page_size, ntotal = 0;
	struct kvm_swap swap[1];

	assert(result);

        init_result(result);

	page_size = getpagesize();

	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open")) == NULL)
    	    return SYSINFO_RET_FAIL;

	if (kvm_getswapinfo(kd, swap, 1, 0) == -1) {
		kvm_close(kd);
		return SYSINFO_RET_FAIL;
	}

	if((kvm_close(kd)) == -1)
		return SYSINFO_RET_FAIL;

	if (swap[0].ksw_total > 0)
	        ntotal = (swap[0].ksw_total * page_size);

	SET_UI64_RESULT(result, (zbx_uint64_t) ntotal);
	return SYSINFO_RET_OK;
#endif
#ifdef HAVE_SYSINFO_TOTALSWAP
	struct sysinfo info;

	assert(result);

        init_result(result);

	if( 0 == sysinfo(&info))
	{
#ifdef HAVE_SYSINFO_MEM_UNIT
		SET_UI64_RESULT(result, (zbx_uint64_t)info.totalswap * (zbx_uint64_t)info.mem_unit);
#else
		SET_UI64_RESULT(result, info.totalswap);
#endif
		return SYSINFO_RET_OK;
	}
	else
	{
		return SYSINFO_RET_FAIL;
	}
/* Solaris */
#else
#ifdef HAVE_SYS_SWAP_SWAPTABLE
	double swaptotal,swapfree;

	assert(result);

        init_result(result);

	get_swapinfo(&swaptotal,&swapfree);
	
	SET_UI64_RESULT(result, swaptotal);
	return SYSINFO_RET_OK;
#else
	assert(result);

        init_result(result);

	return	SYSINFO_RET_FAIL;
#endif
#endif
}

static int	SYSTEM_SWAP_PFREE(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	AGENT_RESULT	result_tmp;
        zbx_uint64_t  tot_val = 0;
        zbx_uint64_t  free_val = 0;

        assert(result);

        init_result(result);
        init_result(&result_tmp);

	if(SYSTEM_SWAP_TOTAL(cmd, param, flags, &result_tmp) != SYSINFO_RET_OK ||
		!(result_tmp.type & AR_UINT64))
	                return  SYSINFO_RET_FAIL;
	tot_val = result_tmp.ui64;

	/* Check fot division by zero */
	if(tot_val == 0)
	{
		free_result(&result_tmp);
                return  SYSINFO_RET_FAIL;
	}

	if(SYSTEM_SWAP_FREE(cmd, param, flags, &result_tmp) != SYSINFO_RET_OK ||
		!(result_tmp.type & AR_UINT64))
                	return  SYSINFO_RET_FAIL;
	free_val = result_tmp.ui64;

	free_result(&result_tmp);

	SET_DBL_RESULT(result, (100.0 * (double)free_val) / (double)tot_val);

        return SYSINFO_RET_OK;
}

static int	SYSTEM_SWAP_PUSED(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	AGENT_RESULT	result_tmp;
        zbx_uint64_t  tot_val = 0;
        zbx_uint64_t  free_val = 0;

        assert(result);

        init_result(result);
        init_result(&result_tmp);

	if(SYSTEM_SWAP_TOTAL(cmd, param, flags, &result_tmp) != SYSINFO_RET_OK ||
		!(result_tmp.type & AR_UINT64))
                	return  SYSINFO_RET_FAIL;
	tot_val = result_tmp.ui64;

	/* Check fot division by zero */
	if(tot_val == 0)
	{
		free_result(&result_tmp);
                return  SYSINFO_RET_FAIL;
	}

	if(SYSTEM_SWAP_FREE(cmd, param, flags, &result_tmp) != SYSINFO_RET_OK ||
		!(result_tmp.type & AR_UINT64))
                	return  SYSINFO_RET_FAIL;
	free_val = result_tmp.ui64;

	free_result(&result_tmp);

	SET_DBL_RESULT(result, 100.0-(100.0 * (double)free_val) / (double)tot_val);

        return SYSINFO_RET_OK;
}

int	SYSTEM_SWAP_SIZE(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{

#define SWP_FNCLIST struct swp_fnclist_s
SWP_FNCLIST
{
	char *mode;
	int (*function)();
};

	SWP_FNCLIST fl[] = 
	{
		{"total",	SYSTEM_SWAP_TOTAL},
		{"free",	SYSTEM_SWAP_FREE},
		{"pfree",	SYSTEM_SWAP_PFREE},
		{"pused",	SYSTEM_SWAP_PUSED},
		{0,		0}
	};

	char swapdev[MAX_STRING_LEN];
	char mode[MAX_STRING_LEN];
	int i;
	
        assert(result);

        init_result(result);
	
        if(num_param(param) > 2)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, swapdev, sizeof(swapdev)) != 0)
        {
                return SYSINFO_RET_FAIL;
        }

        if(swapdev[0] == '\0')
	{
		/* default parameter */
		zbx_snprintf(swapdev, sizeof(swapdev),"all");
	}

	if(strncmp(swapdev, "all", sizeof(swapdev)))
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
		zbx_snprintf(mode, sizeof(mode), "free");
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

int     OLD_SWAP(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
        char    key[MAX_STRING_LEN];
        int     ret;

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

        if(strcmp(key,"free") == 0)
        {
                ret = SYSTEM_SWAP_FREE(cmd, param, flags, result);
        }
        else if(strcmp(key,"total") == 0)
        {
                ret = SYSTEM_SWAP_TOTAL(cmd, param, flags, result);
        }
        else
        {
                ret = SYSINFO_RET_FAIL;
        }

        return ret;
}

#ifdef HAVE_KVM_H
int	get_swap_io(zbx_uint64_t *swapin,
                    zbx_uint64_t *pgswapin,
                    zbx_uint64_t *swapout,
                    zbx_uint64_t *pgswapout)
{
	kvm_t *kd;
	struct vmmeter sum;
	struct nlist nlst[] = { { "_cnt" }, { 0 } };

	if ((kd = kvm_open(NULL, NULL, NULL, O_RDONLY, "kvm_open")) == NULL)
    	    return SYSINFO_RET_FAIL;

	(void) kvm_nlist(kd, nlst);
	if (nlst[0].n_type == 0)
	{
		kvm_close(kd);
		return SYSINFO_RET_FAIL;
	}

	if (kvm_read(kd, nlst[0].n_value, (char *) (&sum), sizeof(sum)) != sizeof(sum))
	{
		kvm_close(kd);
		return SYSINFO_RET_FAIL;
	}
	
	if(kvm_close(kd) == -1)
		return SYSINFO_RET_FAIL;
	
	if(swapin)
		(*swapin)    = (zbx_uint64_t) sum.v_swapin;
	if(pgswapin)
		(*pgswapin)  = (zbx_uint64_t) sum.v_swappgsin;
	if(swapout)
		(*swapout)   = (zbx_uint64_t) sum.v_swapout;
	if(pgswapout)
		(*pgswapout) = (zbx_uint64_t) sum.v_swappgsout;
	
	return SYSINFO_RET_OK;
}
#endif

int	SYSTEM_SWAP_IN(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef HAVE_KVM_H
	int  ret = SYSINFO_RET_FAIL;
	char swapdev[MAX_STRING_LEN];
	char mode[MAX_STRING_LEN];
	zbx_uint64_t value;

	assert(result);

        init_result(result);

	if(num_param(param) > 2)
		return SYSINFO_RET_FAIL;

	if(get_param(param, 1, swapdev, sizeof(swapdev)) != 0)
		return SYSINFO_RET_FAIL;

	if(swapdev[0] == '\0')
		zbx_snprintf(swapdev, sizeof(swapdev), "all");

	if(strncmp(swapdev, "all", sizeof(swapdev)))
		return SYSINFO_RET_FAIL;

	if(get_param(param, 2, mode, sizeof(mode)) != 0)
		mode[0] = '\0';

	if(mode[0] == '\0')
		zbx_snprintf(mode, sizeof(mode), "pages");

	if(strcmp(mode,"count") == 0)
		ret = get_swap_io(&value, NULL, NULL, NULL);
	else if(strcmp(mode,"pages") == 0)
		ret = get_swap_io(NULL, &value, NULL, NULL);
	else
		return SYSINFO_RET_FAIL;

	if(ret != SYSINFO_RET_OK)
		return ret;

	SET_UI64_RESULT(result, value);
	return SYSINFO_RET_OK;
#else
	/* in this moment this function for this platform unsupported */
	return SYSINFO_RET_FAIL;
#endif
}

int	SYSTEM_SWAP_OUT(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef HAVE_KVM_H
	int  ret = SYSINFO_RET_FAIL;
	char swapdev[MAX_STRING_LEN];
	char mode[MAX_STRING_LEN];
	zbx_uint64_t value;

	assert(result);

        init_result(result);

	if(num_param(param) > 2)
		return SYSINFO_RET_FAIL;

	if(get_param(param, 1, swapdev, sizeof(swapdev)) != 0)
		return SYSINFO_RET_FAIL;

	if(swapdev[0] == '\0')
		zbx_snprintf(swapdev, sizeof(swapdev), "all");

	if(strncmp(swapdev, "all", sizeof(swapdev)))
		return SYSINFO_RET_FAIL;

	if(get_param(param, 2, mode, sizeof(mode)) != 0)
		mode[0] = '\0';

	if(mode[0] == '\0')
		zbx_snprintf(mode, sizeof(mode), "pages");

	if(strcmp(mode,"count") == 0)
		ret = get_swap_io(NULL, NULL, &value, NULL);
	else if(strcmp(mode,"pages") == 0)
		ret = get_swap_io(NULL, NULL, NULL, &value);
	else
		return SYSINFO_RET_FAIL;

	if(ret != SYSINFO_RET_OK)
		return ret;

	SET_UI64_RESULT(result, value);
	return SYSINFO_RET_OK;
#else
	/* in this moment this function for this platform unsupported */
	return SYSINFO_RET_FAIL;
#endif
}
