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

struct net_stat_s {
	zbx_uint64_t ibytes;
	zbx_uint64_t ipackets;
	zbx_uint64_t ierr;
	zbx_uint64_t idrop;
	zbx_uint64_t obytes;
	zbx_uint64_t opackets;
	zbx_uint64_t oerr;
	zbx_uint64_t odrop;
	zbx_uint64_t colls;
};

static int get_net_stat(const char *if_name, struct net_stat_s *result)
{
	int ret = SYSINFO_RET_FAIL;
	char line[MAX_STRING_LEN];

	char name[MAX_STRING_LEN];
	zbx_uint64_t tmp = 0;
	struct net_stat_s res;
	
	FILE *f;
	char	*p;

	assert(result);

	memset (result, 0, sizeof (struct net_stat_s));

	if(NULL != (f = fopen("/proc/net/dev","r") ))
	{
		while(fgets(line,MAX_STRING_LEN,f) != NULL)
		{

			p = strstr(line,":");
			if(p) p[0]='\t';
			
			if(sscanf(line,"%s\t" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" 
					ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t \
					" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" 
					ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\t" ZBX_FS_UI64 "\n",
				name, 
				&(res.ibytes), 		/* bytes */
				&(res.ipackets),	/* packets */
				&(res.ierr), 		/* errs */
				&(res.idrop),		/* drop */
			        &(tmp), 		/* fifo */
				&(tmp),			/* frame */
				&(tmp), 		/* compressed */
				&(tmp),			/* multicast */
				&(res.obytes), 		/* bytes */
				&(res.opackets),	/* packets*/
				&(res.oerr),		/* errs */
				&(res.odrop),		/* drop */
			        &(tmp), 		/* fifo */
				&(res.colls),		/* icolls */
			        &(tmp), 		/* carrier */
			        &(tmp)	 		/* compressed */
				) == 17)
			{
				int len;

				/* if_name is regexp which checked against interface name */
				if (zbx_regexp_match (name, if_name, &len)) {
					ret = SYSINFO_RET_OK;

					/* sum obtained data to result */
					result->ibytes += res.ibytes;
					result->ipackets += res.ipackets;
					result->ierr += res.ierr;
					result->idrop += res.idrop;
					result->obytes += res.obytes;
					result->opackets += res.opackets;
					result->oerr += res.oerr;
					result->odrop += res.odrop;
					result->colls += res.colls;
				}
			}
		}
		zbx_fclose(f);
	}
	
	return ret;
}

int	NET_IF_IN(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	struct net_stat_s	ns;
	
	char	if_name[MAX_STRING_LEN];
	char	mode[MAX_STRING_LEN];
	
	int ret = SYSINFO_RET_FAIL;
        
	assert(result);

        init_result(result);

        if(num_param(param) > 2)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, if_name, sizeof(if_name)) != 0)
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
		zbx_snprintf(mode, sizeof(mode), "bytes");
	}

	ret = get_net_stat(if_name, &ns);
	

	if(ret == SYSINFO_RET_OK)
	{
		if(strncmp(mode, "bytes", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.ibytes);
		} 
		else if(strncmp(mode, "packets", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.ipackets);
		}
		else if(strncmp(mode, "errors", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.ierr);
		}
		else if(strncmp(mode, "dropped", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.idrop);
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
	struct net_stat_s	ns;
	
	char	if_name[MAX_STRING_LEN];
	char	mode[MAX_STRING_LEN];
	
	int ret = SYSINFO_RET_FAIL;
        
	assert(result);

        init_result(result);

        if(num_param(param) > 2)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, if_name, sizeof(if_name)) != 0)
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
		zbx_snprintf(mode, sizeof(mode), "bytes");
	}

	ret = get_net_stat(if_name, &ns);
	

	if(ret == SYSINFO_RET_OK)
	{
		if(strncmp(mode, "bytes", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.obytes);
		} 
		else if(strncmp(mode, "packets", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.opackets);
		}
		else if(strncmp(mode, "errors", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.oerr);
		}
		else if(strncmp(mode, "dropped", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.odrop);
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
	struct net_stat_s	ns;
	
	char	if_name[MAX_STRING_LEN];
	char	mode[MAX_STRING_LEN];
	
	int ret = SYSINFO_RET_FAIL;
        
	assert(result);

        init_result(result);

        if(num_param(param) > 2)
        {
                return SYSINFO_RET_FAIL;
        }

        if(get_param(param, 1, if_name, sizeof(if_name)) != 0)
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
		zbx_snprintf(mode, sizeof(mode), "bytes");
	}

	ret = get_net_stat(if_name, &ns);
	

	if(ret == SYSINFO_RET_OK)
	{
		if(strncmp(mode, "bytes", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.ibytes + ns.obytes);
		} 
		else if(strncmp(mode, "packets", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.ipackets + ns.opackets);
		}
		else if(strncmp(mode, "errors", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.ierr + ns.oerr);
		}
		else if(strncmp(mode, "dropped", MAX_STRING_LEN) == 0)
		{
			SET_UI64_RESULT(result, ns.idrop + ns.odrop);
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
	struct net_stat_s	ns;
	
	char	if_name[MAX_STRING_LEN];
	
	int ret = SYSINFO_RET_FAIL;
        
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
	

	ret = get_net_stat(if_name, &ns);
	

	if(ret == SYSINFO_RET_OK)
	{
		SET_UI64_RESULT(result, ns.colls);
	}
	
	return ret;
}

