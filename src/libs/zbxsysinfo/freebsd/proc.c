/*
 * ** ZABBIX
 * ** Copyright (C) 2000-2005 SIA Zabbix
 * **
 * ** This program is free software; you can redistribute it and/or modify
 * ** it under the terms of the GNU General Public License as published by
 * ** the Free Software Foundation; either version 2 of the License, or
 * ** (at your option) any later version.
 * **
 * ** This program is distributed in the hope that it will be useful,
 * ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 * ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * ** GNU General Public License for more details.
 * **
 * ** You should have received a copy of the GNU General Public License
 * ** along with this program; if not, write to the Free Software
 * ** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * **/

#include "common.h"

#include "sysinfo.h"

#include <sys/sysctl.h>
#include <sys/user.h>


typedef enum {
	CT_MODE,
	CT_STATE,
} cond_type_t;


typedef enum {
	MT_SUM,
	MT_MAX,
	MT_MIN,
	MT_AVG,
} mode_type_t;


typedef void (*kinfo_callback_t) (struct kinfo_proc* proc);


/* global state */
char    procname[MAX_STRING_LEN];
char    usrname[MAX_STRING_LEN];
char    procstat;
mode_type_t mode;

struct  passwd *usrinfo;

zbx_uint64_t value, value2;


/* Routine walks all processes (without specific order) and call
   passed routine for every kinfo_proc structure. It's up to structure
   to save and update it's state. Returns 1 if success, zero
   otherise. */
static int enumerate_processes (kinfo_callback_t callback)
{
	static const int    sysctl_name[] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	size_t	length;
	int	err, done, i;
	struct kinfo_proc *kproc = NULL;

	err = sysctl( (int *) sysctl_name, (sizeof(sysctl_name) / sizeof(*sysctl_name)) - 1, NULL, &length, NULL, 0);
	if (err)
		return 0;

	kproc = (struct kinfo_proc*)malloc (length);
	if (!kproc)
		return 0;

	err = sysctl( (int *) sysctl_name, (sizeof(sysctl_name) / sizeof(*sysctl_name)) - 1, kproc, &length, NULL, 0);
	if (err)
		return 0;

	for (i = 0; i < length / sizeof (*kproc); i++)
		callback (kproc+i);
	
	free (kproc);
	return 1;
}


/* prepare global conditions state. Return 1 is succeeded, zero otherwise */
static int prepare_conditions (const char *param, cond_type_t type)
{
	char buf[MAX_STRING_LEN];

        if(num_param (param) > 3)
                return 0;
    
        if(get_param (param, 1, procname, MAX_STRING_LEN) != 0)
                return 0;
    
        if(get_param(param, 2, usrname, MAX_STRING_LEN) != 0)
		usrinfo = NULL;
        else
            if(usrname[0] != 0)
            {
                usrinfo = getpwnam(usrname);
                if(usrinfo == NULL)
                    /* incorrect user name */
                    return 0;
            }
    
	if (type == CT_STATE) {
		if(get_param(param, 3, buf, MAX_STRING_LEN) != 0)
			procstat = 0;
		else if(strcmp(buf,"run") == 0)
			procstat = 'R';
		else if(strcmp(buf,"sleep") == 0)
			procstat = 'S';
		else if(strcmp(buf,"zomb") == 0)
			procstat = 'Z';
		else if(strcmp(buf,"all") == 0)
			procstat = 0;
		else
			return 0;
	} else {
		if(get_param(param, 3, buf, MAX_STRING_LEN) != 0)
			mode = MT_SUM;
		else if(strcmp(buf,"avg") == 0)
			mode = MT_AVG;
		else if(strcmp(buf,"max") == 0)
			mode = MT_MAX;
		else if(strcmp(buf,"min") == 0)
			mode = MT_MIN;
		else if(strcmp(buf,"sum") == 0)
			mode = MT_SUM;
		else
			return 0;
	}
	
	return 1;
}


/* checks that given process fits conditions. Return 1 if it is, 0 otherwise */
static int check_conditions (struct kinfo_proc* proc, int check_state)
{
	int proc_ok = 1, stat_ok = 1, usr_ok = 1;

	if (procname[0] != 0) {
		if (strcmp (procname, proc->ki_comm) != 0)
			proc_ok = 0;
	}

	if (check_state && procstat != 0) {
		switch (proc->ki_stat) {
		case SIDL:
		case SSLEEP:
		case SSTOP:
		case SWAIT:
		case SLOCK:
			stat_ok = procstat == 'S';
			break;
		case SRUN:
			stat_ok = procstat == 'R';
			break;
		case SZOMB:
			stat_ok = procstat == 'Z';
			break;
		default:
			stat_ok = 0;
		}
	}

	if (usrinfo) {
		if (usrinfo->pw_uid != proc->ki_uid)
			usr_ok = 0;
	}

	if (proc_ok && stat_ok && usr_ok)
		return 1;
	else
		return 0;
}


/* callback which checks that process fits criteria and increments
   global count if it is */
static void proc_num_cb (struct kinfo_proc* proc)
{
	if (check_conditions (proc, 1))
		value++;
}


static void proc_mem_cb (struct kinfo_proc* proc)
{
	if (!check_conditions (proc, 0))
		return;

	switch (mode) {
	case MT_SUM:
		value += proc->ki_size;
		break;
	case MT_MIN:
		if (!value || proc->ki_size < value)
			value = proc->ki_size;
		break;
	case MT_MAX:
		if (proc->ki_size > value)
			value = proc->ki_size;
		break;
	case MT_AVG:
		value += proc->ki_size;
		value2++;
		break;
	}
}



static void proc_mem_rss_cb (struct kinfo_proc* proc)
{
	if (!check_conditions (proc, 0))
		return;

	switch (mode) {
	case MT_SUM:
		value += proc->ki_rssize;
		break;
	case MT_MIN:
		if (!value || proc->ki_rssize < value)
			value = proc->ki_rssize;
		break;
	case MT_MAX:
		if (proc->ki_rssize > value)
			value = proc->ki_rssize;
		break;
	case MT_AVG:
		value += proc->ki_rssize;
		value2++;
		break;
	}
}


				    
int     PROC_MEMORY(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
        init_result(result);

	if (!prepare_conditions (param, CT_MODE))
		return SYSINFO_RET_FAIL;

	value = value2 = 0;

	if (!enumerate_processes (proc_mem_cb))
		return SYSINFO_RET_FAIL;

	if (mode == MT_AVG) {
		if (!value2) {
			SET_DBL_RESULT (result, 0);
		}
		else {
			SET_DBL_RESULT (result, value / value2);
		}
	}
	else {
		SET_UI64_RESULT(result, value);
	}

	return SYSINFO_RET_OK;
}



int     PROC_MEMORY_RSS(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
        init_result(result);

	if (!prepare_conditions (param, CT_MODE))
		return SYSINFO_RET_FAIL;

	value = value2 = 0;

	if (!enumerate_processes (proc_mem_rss_cb))
		return SYSINFO_RET_FAIL;

	if (mode == MT_AVG) {
		if (!value2) {
			SET_DBL_RESULT (result, 0);
		}
		else {
			SET_DBL_RESULT (result, value*getpagesize() / value2);
		}
	}
	else {
		SET_UI64_RESULT(result, value*getpagesize());
	}

	return SYSINFO_RET_OK;
}




int	    PROC_NUM(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
        assert(result);
        init_result(result);

	if (!prepare_conditions (param, CT_STATE))
		return SYSINFO_RET_FAIL;

	value = 0;
	if (!enumerate_processes (proc_num_cb))
		return SYSINFO_RET_FAIL;
	SET_UI64_RESULT(result, value);
	return SYSINFO_RET_OK;
}
