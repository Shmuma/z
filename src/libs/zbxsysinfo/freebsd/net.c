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
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/if_mib.h>

#include "common.h"

#include "sysinfo.h"

static int get_ifmib_general(char *if_name, struct ifmibdata *ifmd)
{
	int i, maxifno, retval;
	int name[6];
	size_t len;
	
	retval = SYSINFO_RET_FAIL;

	name[0] = CTL_NET;
	name[1] = PF_LINK;
	name[2] = NETLINK_GENERIC;
	name[3] = IFMIB_SYSTEM;
	name[4] = IFMIB_IFCOUNT;

	len = sizeof (maxifno);
	if (sysctl(name, 5, &maxifno, &len, 0, 0) < 0)
		/* err(EX_OSERR, "sysctl(net.link.generic.system.ifcount)"); */
		return retval;

	for (i = 1; i <= maxifno; i++) {
		len = sizeof(struct ifmibdata);
		name[3] = IFMIB_IFDATA;
		name[4] = i;
		name[5] = IFDATA_GENERAL;

		if (sysctl(name, 6, ifmd, &len, 0, 0) < 0) {
			if (errno == ENOENT)
				continue;

			/* err(EX_OSERR, "sysctl(net.link.ifdata.%d.general)", i); */
			return retval;
		}

		if (strncmp(ifmd->ifmd_name, if_name, MIN(strlen(if_name),strlen(ifmd->ifmd_name))) != 0)
			continue;

		retval = SYSINFO_RET_OK;
		break;
	}

	return retval;
}

int	NET_IF_IN(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	struct ifmibdata value;
	char if_name[MAX_STRING_LEN], mode[MAX_STRING_LEN];
	int ret = SYSINFO_RET_FAIL;
        
	assert(result);
        init_result(result);

        if(num_param(param) > 2)
                return SYSINFO_RET_FAIL;

        if(get_param(param, 1, if_name, sizeof(if_name)) != 0)
                return SYSINFO_RET_FAIL;
	
	if(get_param(param, 2, mode, sizeof(mode)) != 0)
                mode[0] = '\0';

        if(mode[0] == '\0')
		/* default parameter */
		zbx_snprintf(mode, sizeof(mode), "bytes");

	ret = get_ifmib_general(if_name, &value);
	

	if(ret == SYSINFO_RET_OK)
	{
		if(strncmp(mode, "bytes", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, value.ifmd_data.ifi_ibytes);
		}
		else if(strncmp(mode, "packets", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, value.ifmd_data.ifi_ipackets);
		}
		else if(strncmp(mode, "errors", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, value.ifmd_data.ifi_ierrors);
		}
		else if(strncmp(mode, "dropped", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, value.ifmd_data.ifi_iqdrops);
		}
		else
		{
			ret = SYSINFO_RET_FAIL;
		}
	}
	
	return ret;
}

int	NET_IF_OUT(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	struct ifmibdata value;
	char if_name[MAX_STRING_LEN], mode[MAX_STRING_LEN];
	int ret = SYSINFO_RET_FAIL;
        
	assert(result);
        init_result(result);

        if(num_param(param) > 2)
                return SYSINFO_RET_FAIL;

        if(get_param(param, 1, if_name, sizeof(if_name)) != 0)
                return SYSINFO_RET_FAIL;
	
	if(get_param(param, 2, mode, sizeof(mode)) != 0)
                mode[0] = '\0';

        if(mode[0] == '\0')
		/* default parameter */
		zbx_snprintf(mode, sizeof(mode), "bytes");

	ret = get_ifmib_general(if_name, &value);
	

	if(ret == SYSINFO_RET_OK)
	{
		if(strncmp(mode, "bytes", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, value.ifmd_data.ifi_obytes);
		}
		else if(strncmp(mode, "packets", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, value.ifmd_data.ifi_opackets);
		}
		else if(strncmp(mode, "errors", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, value.ifmd_data.ifi_oerrors);
		}
		else
		{
			ret = SYSINFO_RET_FAIL;
		}
	}
	
	return ret;
}

int	NET_IF_TOTAL(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	struct ifmibdata value;
	char if_name[MAX_STRING_LEN], mode[MAX_STRING_LEN];
	int ret = SYSINFO_RET_FAIL;
        
	assert(result);
        init_result(result);

        if(num_param(param) > 2)
                return SYSINFO_RET_FAIL;

        if(get_param(param, 1, if_name, sizeof(if_name)) != 0)
                return SYSINFO_RET_FAIL;
	
	if(get_param(param, 2, mode, sizeof(mode)) != 0)
                mode[0] = '\0';

        if(mode[0] == '\0')
		/* default parameter */
		zbx_snprintf(mode, sizeof(mode), "bytes");

	ret = get_ifmib_general(if_name, &value);
	

	if(ret == SYSINFO_RET_OK)
	{
		if(strncmp(mode, "bytes", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, (value.ifmd_data.ifi_ibytes + value.ifmd_data.ifi_obytes));
		}
		else if(strncmp(mode, "packets", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, (value.ifmd_data.ifi_ipackets + value.ifmd_data.ifi_opackets));
		}
		else if(strncmp(mode, "errors", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, (value.ifmd_data.ifi_ierrors + value.ifmd_data.ifi_oerrors));
		}
		else
		{
			ret = SYSINFO_RET_FAIL;
		}
	}
	
	return ret;
}

int     NET_TCP_LISTEN(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
        assert(result);

        init_result(result);
	
	return SYSINFO_RET_FAIL;
}

int     NET_IF_COLLISIONS(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	struct ifmibdata value;
	char    if_name[MAX_STRING_LEN];
	int     ret = SYSINFO_RET_FAIL;

	assert(result);

	init_result(result);

	if(num_param(param) > 1)
	{
		return SYSINFO_RET_FAIL;
	}

	if(get_param(param, 1, if_name, MAX_STRING_LEN) != 0)
	{
		return SYSINFO_RET_FAIL;
	}

	ret = get_ifmib_general(if_name, &value);
	
	if(ret == SYSINFO_RET_OK)
	{
		SET_UI64_RESULT(result, value.ifmd_data.ifi_collisions);
		ret = SYSINFO_RET_OK;
	}
	
	return ret;
}

