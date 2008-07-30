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
#include "stats.h"

int	SYSTEM_CPU_NUM(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef HAVE_FUNCTION_SYSCTL_HW_NCPU
	size_t len;
        int mib[2], ncpu;

	assert(result);

	init_result(result);

        mib[0]=CTL_HW;
        mib[1]=HW_NCPU;

        len = sizeof(ncpu);
        if ((sysctl(mib, 2, &ncpu, &len, 0, 0)) != 0)
	{
		return SYSINFO_RET_FAIL;
	}

	SET_UI64_RESULT(result, ncpu);
	return SYSINFO_RET_OK;
#else
	return SYSINFO_RET_FAIL;
#endif
}


static long percentages(int cnt, int *out, register long *new,
                        register long *old, long *diffs)  {

	register int i;
	register long change;
	register long total_change;
	register long *dp;
	long half_total;

	/* initialization */
	total_change = 0;
	dp = diffs;

	/* calculate changes for each state and the overall change */
	for (i = 0; i < cnt; i++) {
		if ((change = *new - *old) < 0) {
			/* this only happens when the counter wraps */
			change = (int)
				((unsigned long)*new-(unsigned long)*old);
		}
		total_change += (*dp++ = change);
		*old++ = *new++;
	}
	/* avoid divide by zero potential */
	if (total_change == 0) { total_change = 1; }

	/* calculate percentages based on overall change, rounding up */
	half_total = total_change / 2l;

	/* Do not divide by 0. Causes Floating point exception */
	if(total_change) {
		for (i = 0; i < cnt; i++) {
			*out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
		}
	}

	/* return the total in case the caller wants to use it */
	return(total_change);
}


#define timertod(tvp) \
    ((double)(tvp)->tv_sec + (double)(tvp)->tv_usec/(1000*1000))


int	SYSTEM_CPU_UTIL(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	char cpuname[MAX_STRING_LEN];
	char type[MAX_STRING_LEN];
	char mode[MAX_STRING_LEN];

	int cpu_num = 0;
	
        assert(result);

        init_result(result);
	
        if(num_param(param) > 3)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, cpuname, sizeof(cpuname)) != 0)
        {
                cpuname[0] = '\0';
        }

	if(cpuname[0] == '\0')
	{
		/* default parameter */
		zbx_snprintf(cpuname, sizeof(cpuname), "all");
	}
	
	if(get_param(param, 2, type, sizeof(type)) != 0)
        {
                type[0] = '\0';
        }
        if(type[0] == '\0')
	{
		/* default parameter */
		zbx_snprintf(type, sizeof(type), "user");
	}
	
	if(get_param(param, 3, mode, sizeof(mode)) != 0)
        {
                mode[0] = '\0';
        }
	
        if(mode[0] == '\0')
	{
		/* default parameter */
		zbx_snprintf(mode, sizeof(mode), "avg1");
	}

	if ( !CPU_COLLECTOR_STARTED(collector) )
	{
		/* trying to live without collector */
		long cp_time[CPUSTATES];
		long cp_diff[CPUSTATES];
		static long cp_old[CPUSTATES];
		static int cpu_states[CPUSTATES];
		static struct timeval this_time, last_time;
		struct timeval time_diff;
		size_t len = sizeof(cp_time);
		int i;

		gettimeofday(&this_time, NULL);
		timersub(&this_time, &last_time, &time_diff);
		if (timertod(&time_diff) < 0.5) {
			goto output;
		}
		last_time = this_time;

		/* puts kern.cp_time array into cp_time */
		if (sysctlbyname("kern.cp_time", &cp_time, &len, NULL, 0) == -1) {
			warn("kern.cp_time");
			return 0.0;
		}
		/* Use percentages function lifted from top(1) to figure percentages */
		percentages(CPUSTATES, cpu_states, cp_time, cp_old, cp_diff);

	output:
		if( 0 == strcmp(type,"idle")) {
			SET_DBL_RESULT (result, (double)(cpu_states[CP_IDLE]) / 10.0);
			return SYSINFO_RET_OK;
		}
		else if( 0 == strcmp(type,"nice")) {
			SET_DBL_RESULT (result, (double)(cpu_states[CP_NICE]) / 10.0);
			return SYSINFO_RET_OK;
		}
		else if( 0 == strcmp(type,"user")) {
			SET_DBL_RESULT (result, (double)(cpu_states[CP_USER]) / 10.0);
			return SYSINFO_RET_OK;
		}
		else if( 0 == strcmp(type,"system")) {
			SET_DBL_RESULT (result, (double)(cpu_states[CP_SYS]) / 10.0);
			return SYSINFO_RET_OK;
		}
		else
			return SYSINFO_RET_FAIL;
	}

	if(strcmp(cpuname,"all") == 0)
	{
		cpu_num = 0;
	}
	else
	{
		cpu_num = atoi(cpuname)+1;
		if ((cpu_num < 1) || (cpu_num > collector->cpus.count))
			return SYSINFO_RET_FAIL;
	}

	if( 0 == strcmp(type,"idle"))
	{
		if( 0 == strcmp(mode,"avg1"))		SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].idle1)
		else if( 0 == strcmp(mode,"avg5"))	SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].idle5)
		else if( 0 == strcmp(mode,"avg15"))	SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].idle15)
		else return SYSINFO_RET_FAIL;

	}
	else if( 0 == strcmp(type,"nice"))
	{
		if( 0 == strcmp(mode,"avg1")) 		SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].nice1)
		else if( 0 == strcmp(mode,"avg5")) 	SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].nice5)
		else if( 0 == strcmp(mode,"avg15"))	SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].nice15)
		else return SYSINFO_RET_FAIL;

	}
	else if( 0 == strcmp(type,"user"))
	{
		if( 0 == strcmp(mode,"avg1")) 		SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].user1)
		else if( 0 == strcmp(mode,"avg5")) 	SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].user5)
		else if( 0 == strcmp(mode,"avg15"))	SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].user15)
		else return SYSINFO_RET_FAIL;
	}
	else if( 0 == strcmp(type,"system"))
	{
		if( 0 == strcmp(mode,"avg1")) 		SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].system1)
		else if( 0 == strcmp(mode,"avg5")) 	SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].system5)
		else if( 0 == strcmp(mode,"avg15"))	SET_DBL_RESULT(result, collector->cpus.cpu[cpu_num].system15)
		else return SYSINFO_RET_FAIL;
	}
	else
	{
		return SYSINFO_RET_FAIL;
	}

	return SYSINFO_RET_OK;
}

/* AIX CPU info */
#ifdef HAVE_KNLIST_H
static int getloadavg_kmem(double loadavg[], int nelem)
{
	struct nlist nl;
	int kmem, i;
	long avenrun[3];

	nl.n_name = "avenrun";
	nl.n_value = 0;

	if(knlist(&nl, 1, sizeof(nl)))
	{
		return FAIL;
	}
	if((kmem = open("/dev/kmem", 0, 0)) <= 0)
	{
		return FAIL;
	}

	if(pread(kmem, avenrun, sizeof(avenrun), nl.n_value) <
				sizeof(avenrun))
	{
		return FAIL;
	}

	for(i=0;i<nelem;i++)
	{
		loadavg[i] = (double) avenrun[i] / 65535;
	}
	return SUCCEED;
}
#endif

int	SYSTEM_CPU_LOAD1(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef HAVE_GETLOADAVG
	double	load[3];

	assert(result);

        init_result(result);
		
	if(getloadavg(load, 3))
	{
		SET_DBL_RESULT(result, load[0]);
		return SYSINFO_RET_OK;
	}
	else
	{
		return SYSINFO_RET_FAIL;	
	}
#else
#ifdef	HAVE_SYS_PSTAT_H
	struct	pst_dynamic dyn;

	assert(result);

        init_result(result);

	if (pstat_getdynamic(&dyn, sizeof(dyn), 1, 0) == -1)
	{
		return SYSINFO_RET_FAIL;
	}
	else
	{
		SET_DBL_RESULT(result, dyn.psd_avg_1_min);
		return SYSINFO_RET_OK;
	}
#else
#ifdef HAVE_PROC_LOADAVG
	return	getPROC("/proc/loadavg",1,1, flags, result);
#else
#ifdef HAVE_KSTAT_H
	static kstat_ctl_t *kc = NULL;
	kstat_t *ks;
	kstat_named_t *kn;

	assert(result);

        init_result(result);
	
	if (!kc && !(kc = kstat_open()))
	{
		return SYSINFO_RET_FAIL;
	}
	if (!(ks = kstat_lookup(kc, "unix", 0, "system_misc")) ||
		kstat_read(kc, ks, 0) == -1 ||
		!(kn = kstat_data_lookup(ks,"avenrun_1min")))
	{
		return SYSINFO_RET_FAIL;
	}
	SET_DBL_RESULT(result, ((double)kn->value.ul)/256.0);
	return SYSINFO_RET_OK;
#else
#ifdef HAVE_KNLIST_H
	double loadavg[3];

	assert(result);

        init_result(result);
		
	if(getloadavg_kmem(loadavg,3) == FAIL)
	{
		return SYSINFO_RET_FAIL;
	}

	SET_DBL_RESULT(result, loadavg[0]);
	return SYSINFO_RET_OK;
#else
	assert(result);

        init_result(result);
	return	SYSINFO_RET_FAIL;
#endif
#endif
#endif
#endif
#endif
}

int	SYSTEM_CPU_LOAD5(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef HAVE_GETLOADAVG
	double	load[3];

	assert(result);

        init_result(result);
		
	if(getloadavg(load, 3))
	{
		SET_DBL_RESULT(result, load[1]);
		return SYSINFO_RET_OK;
	}
	else
	{
		return SYSINFO_RET_FAIL;	
	}
#else
#ifdef	HAVE_SYS_PSTAT_H
	struct	pst_dynamic dyn;

	assert(result);

        init_result(result);
	
	if (pstat_getdynamic(&dyn, sizeof(dyn), 1, 0) == -1)
	{
		return SYSINFO_RET_FAIL;
	}
	else
	{
		SET_DBL_RESULT(result, dyn.psd_avg_5_min);
		return SYSINFO_RET_OK;
	}
#else
#ifdef	HAVE_PROC_LOADAVG
	return	getPROC("/proc/loadavg",1,2, flags, result);
#else
#ifdef HAVE_KSTAT_H
	static kstat_ctl_t *kc = NULL;
	kstat_t *ks;
	kstat_named_t *kn;

	assert(result);

        init_result(result);
		
	if (!kc && !(kc = kstat_open()))
	{
		return SYSINFO_RET_FAIL;
	}
	if (!(ks = kstat_lookup(kc, "unix", 0, "system_misc")) ||
		kstat_read(kc, ks, 0) == -1 ||
		!(kn = kstat_data_lookup(ks,"avenrun_5min")))
	{
		return SYSINFO_RET_FAIL;
	}
	SET_DBL_RESULT(result, ((double)kn->value.ul)/256.0);
	return SYSINFO_RET_OK;
#else
#ifdef HAVE_KNLIST_H
	double loadavg[3];

	assert(result);

        init_result(result);

	if(getloadavg_kmem(loadavg,3) == FAIL)
	{
		return STSINFO_RET_FAIL;
	}

	SET_DBL_RESULT(result, loadavg[1]);
	return SYSINFO_RET_OK;
#else
	assert(result);

        init_result(result);
	return	SYSINFO_RET_FAIL;
#endif
#endif
#endif
#endif
#endif
}

int	SYSTEM_CPU_LOAD15(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#ifdef HAVE_GETLOADAVG
	double	load[3];

	assert(result);

        init_result(result);

	if(getloadavg(load, 3))
	{
		SET_DBL_RESULT(result, load[2]);	
		return SYSINFO_RET_OK;
	}
	else
	{
		return SYSINFO_RET_FAIL;	
	}
#else
#ifdef	HAVE_SYS_PSTAT_H
	struct	pst_dynamic dyn;

	assert(result);

        init_result(result);
		
	if (pstat_getdynamic(&dyn, sizeof(dyn), 1, 0) == -1)
	{
		return SYSINFO_RET_FAIL;
	}
	else
	{
		SET_DBL_RESULT(result, dyn.psd_avg_15_min);
		return SYSINFO_RET_OK;
	}
#else
#ifdef	HAVE_PROC_LOADAVG
	return	getPROC("/proc/loadavg",1,3, flags, result);
#else
#ifdef HAVE_KSTAT_H
	static kstat_ctl_t *kc = NULL;
	kstat_t *ks;
	kstat_named_t *kn;

	assert(result);

        init_result(result);
		
	if (!kc && !(kc = kstat_open()))
	{
		return SYSINFO_RET_FAIL;
	}
	if (!(ks = kstat_lookup(kc, "unix", 0, "system_misc")) ||
		kstat_read(kc, ks, 0) == -1 ||
		!(kn = kstat_data_lookup(ks,"avenrun_15min")))
	{
		return SYSINFO_RET_FAIL;
	}
	SET_DBL_RESULT(result, ((double)kn->value.ul)/256.0);
	return SYSINFO_RET_OK;
#else
#ifdef HAVE_KNLIST_H
	double loadavg[3];

	assert(result);

        init_result(result);
	
	if(getloadavg_kmem(loadavg,3) == FAIL)
	{
		return STSINFO_RET_FAIL;
	}

	SET_DBL_RESULT(result, loadavg[2]);
	return SYSINFO_RET_OK;
#else
	assert(result);

        init_result(result);
	return	SYSINFO_RET_FAIL;
#endif
#endif
#endif
#endif
#endif
}

int	SYSTEM_CPU_LOAD(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{

#define CPU_FNCLIST struct cpu_fnclist_s
CPU_FNCLIST
{
	char *mode;
	int (*function)();
};

	CPU_FNCLIST fl[] = 
	{
		{"avg1" ,	SYSTEM_CPU_LOAD1},
		{"avg5" ,	SYSTEM_CPU_LOAD5},
		{"avg15",	SYSTEM_CPU_LOAD15},
		{0,		0}
	};

	char cpuname[MAX_STRING_LEN];
	char mode[MAX_STRING_LEN];
	int i;
	
        assert(result);

        init_result(result);
	
        if(num_param(param) > 2)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, cpuname, sizeof(cpuname)) != 0)
        {
                return SYSINFO_RET_FAIL;
        }
	if(cpuname[0] == '\0')
	{
		/* default parameter */
		zbx_snprintf(cpuname, sizeof(cpuname), "all");
	}
	if(strncmp(cpuname, "all", sizeof(cpuname)))
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
		zbx_snprintf(mode, sizeof(mode), "avg1");
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

int     SYSTEM_CPU_SWITCHES(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
        assert(result);

        init_result(result);
	
	return SYSINFO_RET_FAIL;
}

int     SYSTEM_CPU_INTR(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
        assert(result);

        init_result(result);
	
	return SYSINFO_RET_FAIL;
}

