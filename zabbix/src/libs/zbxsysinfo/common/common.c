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

#include "log.h"
#include "threads.h"

#include "file.h"
#include "http.h"
#include "net.h"
#include "system.h"

#if !defined(_WINDOWS)
#	define VFS_TEST_FILE "/etc/passwd"
#	define VFS_TEST_REGEXP "root"
#else
#	define VFS_TEST_FILE "c:\\windows\\win.ini"
#	define VFS_TEST_REGEXP "fonts"
#endif /* _WINDOWS */

static int	AGENT_PING(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result);
static int	AGENT_VERSION(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result);
static int	ONLY_ACTIVE(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result);

ZBX_METRIC	parameters_common[]=
/*      KEY                     FLAG    FUNCTION        ADD_PARAM       TEST_PARAM */
	{
	{"agent.ping",		0,		AGENT_PING, 		0,	0},
	{"agent.version",	0,		AGENT_VERSION,		0,	0},

	{"system.localtime",	0,		SYSTEM_LOCALTIME,	0,	0},
	{"system.run",		CF_USEUPARAM,	RUN_COMMAND,	 	0,	"echo test"},

	{"web.page.get",	CF_USEUPARAM,	WEB_PAGE_GET,	 	0,	"localhost,,80"},
	{"web.page.perf",	CF_USEUPARAM,	WEB_PAGE_PERF,	 	0,	"localhost,,80"},
	{"web.page.regexp",	CF_USEUPARAM,	WEB_PAGE_REGEXP,	0,	"localhost,,80,OK"},

	{"vfs.file.exists",	CF_USEUPARAM,	VFS_FILE_EXISTS,	0,	VFS_TEST_FILE},
	{"vfs.file.time",       CF_USEUPARAM,   VFS_FILE_TIME,          0,      VFS_TEST_FILE ",modify"},
	{"vfs.file.size",	CF_USEUPARAM,	VFS_FILE_SIZE, 		0,	VFS_TEST_FILE},
	{"vfs.file.regexp",	CF_USEUPARAM,	VFS_FILE_REGEXP,	0,	VFS_TEST_FILE "," VFS_TEST_REGEXP},
	{"vfs.file.regmatch",	CF_USEUPARAM,	VFS_FILE_REGMATCH, 	0,	VFS_TEST_FILE "," VFS_TEST_REGEXP},
	{"vfs.file.cksum",	CF_USEUPARAM,	VFS_FILE_CKSUM,		0,	VFS_TEST_FILE},
	{"vfs.file.md5sum",	CF_USEUPARAM,	VFS_FILE_MD5SUM,	0,	VFS_TEST_FILE},

	{"net.tcp.dns",		CF_USEUPARAM,	CHECK_DNS,		0,	"127.0.0.1,localhost"},
	{"net.tcp.port",	CF_USEUPARAM,	CHECK_PORT,		0,	",80"},

	{"system.hostname",	0,		SYSTEM_HOSTNAME,	0,	0},
	{"system.uname",	0,		SYSTEM_UNAME,		0,	0},

	{"system.users.num",	0,		SYSTEM_UNUM, 		0,	0},
	{"log",			CF_USEUPARAM,	ONLY_ACTIVE, 		0,	"logfile"},
	{"eventlog",		CF_USEUPARAM,	ONLY_ACTIVE, 		0,	"system"},

	{0}
	};

static int	ONLY_ACTIVE(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	assert(result);

        init_result(result);	
	SET_MSG_RESULT(result, strdup("Accessible only as active check!"));
	return SYSINFO_RET_FAIL;
}

int	getPROC(char *file, int lineno, int fieldno, unsigned flags, AGENT_RESULT *result)
{
#ifdef	HAVE_PROC
	FILE	*f;
	char	*t;
	char	c[MAX_STRING_LEN];
	int	i;
	double	value = 0;
        
	assert(result);

        init_result(result);	
		
	if(NULL == (f = fopen(file,"r")))
	{
		return	SYSINFO_RET_FAIL;
	}

	for(i=1; i<=lineno; i++)
	{	
		fgets(c,MAX_STRING_LEN,f);
	}

	t=(char *)strtok(c," ");
	for(i=2; i<=fieldno; i++)
	{
		t=(char *)strtok(NULL," ");
	}

	zbx_fclose(f);

	sscanf(t, "%lf", &value);
	SET_DBL_RESULT(result, value);

	return SYSINFO_RET_OK;
#else
	return SYSINFO_RET_FAIL;
#endif /* HAVE_PROC */
}

static int	AGENT_PING(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
        assert(result);

        init_result(result);	
	
	SET_UI64_RESULT(result, 1);
	return SYSINFO_RET_OK;
}

static int	AGENT_VERSION(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
	static	char	version[]=ZABBIX_VERSION;

	assert(result);

        init_result(result);
		
	SET_STR_RESULT(result, strdup(version));
	
	return	SYSINFO_RET_OK;
}


int	EXECUTE_STR(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{

#if defined(_WINDOWS)

	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	SECURITY_ATTRIBUTES sa;
	HANDLE hOutput;
	char szTempPath[MAX_PATH],szTempFile[MAX_PATH];

#else /* not _WINDOWS */

	FILE	*f;

#endif /* _WINDOWS */

	char	cmd_result[MAX_STRING_LEN];
	char	command[MAX_STRING_LEN];
	int	i,len;

        assert(result);

        init_result(result);
	
	memset(cmd_result, 0, MAX_STRING_LEN);

#if defined(_WINDOWS)

	/* Create temporary file to hold process output */
	GetTempPath( MAX_PATH-1,	szTempPath);
	GetTempFileName( szTempPath, "zbx", 0, szTempFile);

	sa.nLength		= sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor	= NULL;
	sa.bInheritHandle	= TRUE;

	if(INVALID_HANDLE_VALUE == (hOutput = CreateFile(
		szTempFile,
		GENERIC_READ | GENERIC_WRITE,
		0,
		&sa,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY,
		NULL)))
	{
		zabbix_log(LOG_LEVEL_DEBUG, "Unable to create temporary file: '%s' [%s]", szTempFile, strerror_from_system(GetLastError()));
		return SYSINFO_RET_FAIL;
	}

	/* Fill in process startup info structure */
	memset(&si,0,sizeof(STARTUPINFO));
	si.cb		= sizeof(STARTUPINFO);
	si.dwFlags	= STARTF_USESTDHANDLES;
	si.hStdInput	= GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput	= hOutput;
	si.hStdError	= GetStdHandle(STD_ERROR_HANDLE);

	zbx_snprintf(command, sizeof(command), "cmd /C \"%s\"", param);

	/* Create new process */
	if (!CreateProcess(NULL,command,NULL,NULL,TRUE,0,NULL,NULL,&si,&pi))
	{
		zabbix_log(LOG_LEVEL_DEBUG, "Unable to create process: '%s' [%s]", command, strerror_from_system(GetLastError()));

		/* Remove temporary file */
		CloseHandle(hOutput);
		DeleteFile(szTempFile);

		return SYSINFO_RET_FAIL;
	}

	/* Wait for process termination and close all handles */
	WaitForSingleObject(pi.hProcess,INFINITE);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);

	/* Rewind temporary file for reading */
	SetFilePointer(hOutput,0,NULL,FILE_BEGIN);

	/* Read process output */
	ReadFile(hOutput, cmd_result, MAX_STRING_LEN-1, &len, NULL);

	cmd_result[len] = '\0';
	
	/* Remove temporary file */
	CloseHandle(hOutput);
	DeleteFile(szTempFile);

#else /* not _WINDOWS */
	zbx_strlcpy(command, param, sizeof(command));

	if(0 == (f = popen(command,"r")))
	{
		switch (errno)
		{
			case	EINTR:
				return SYSINFO_RET_TIMEOUT;
			default:
				return SYSINFO_RET_FAIL;
		}
	}

	len = fread(cmd_result, 1, sizeof(cmd_result)-1, f);

	if(0 != ferror(f))
	{
		switch (errno)
		{
			case	EINTR:
				pclose(f);
				return SYSINFO_RET_TIMEOUT;
			default:
				pclose(f);
				return SYSINFO_RET_FAIL;
		}
	}

	cmd_result[len] = '\0';

	if(pclose(f) == -1)
	{
		switch (errno)
		{
			case	EINTR:
				return SYSINFO_RET_TIMEOUT;
			default:
				return SYSINFO_RET_FAIL;
		}
	}

#endif /* _WINDOWS */

	zabbix_log(LOG_LEVEL_DEBUG, "Before");

        for(i=(int)strlen(cmd_result); i>0 && (cmd_result[i] == '\n' || cmd_result[i] == '\r' || cmd_result[i] == '\0'); cmd_result[i--] = '\0');

	/* We got EOL only */
	if(cmd_result[0] == '\0')
	{
		return SYSINFO_RET_FAIL;
	}

	zabbix_log(LOG_LEVEL_DEBUG, "Run remote command [%s] Result [%d] [%s]", command, strlen(cmd_result), cmd_result);

	SET_TEXT_RESULT(result, strdup(cmd_result));

	return	SYSINFO_RET_OK;

}

int	EXECUTE_INT(const char *cmd, const char *command, unsigned flags, AGENT_RESULT *result)
{
	int	ret	= SYSINFO_RET_FAIL;
	double	value	= 0;

	ret = EXECUTE_STR(cmd,command,flags,result);

	if(SYSINFO_RET_OK == ret)
	{
		sscanf(result->text, "%lf", &value);

		UNSET_TEXT_RESULT(result);

		SET_DBL_RESULT(result, value);
	}

	return ret;
}

int	RUN_COMMAND(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{
#define MAX_FLAG_LEN 10

	char	command[MAX_STRING_LEN];
	char	flag[MAX_FLAG_LEN];

#if defined (_WINDOWS)
	STARTUPINFO    si;
	PROCESS_INFORMATION  pi;

	char	full_command[MAX_STRING_LEN];
#else /* not _WINDOWS */
	pid_t	pid;
#endif

	
        assert(result);

	init_result(result);
	

	if(CONFIG_ENABLE_REMOTE_COMMANDS != 1)
	{
		SET_MSG_RESULT(result, strdup("ZBX_NOTSUPPORTED"));
		return  SYSINFO_RET_FAIL;
	}
	
        if(num_param(param) > 2)
        {
                return SYSINFO_RET_FAIL;
        }
        
	if(get_param(param, 1, command, sizeof(command)) != 0)
        {
                return SYSINFO_RET_FAIL;
        }

	if(command[0] == '\0')
	{
		return SYSINFO_RET_FAIL;
	}

	zabbix_log(LOG_LEVEL_DEBUG, "Run command '%s'",command);
	
	if(get_param(param, 2, flag, sizeof(flag)) != 0)
        {
                flag[0] = '\0';
        }

	if(flag[0] == '\0')
	{
		zbx_snprintf(flag,sizeof(flag),"wait");
	}

	if(strcmp(flag,"wait") == 0)
	{
		return EXECUTE_STR(cmd,command,flags,result);
	}
	else if(strcmp(flag,"nowait") != 0)
	{
		return SYSINFO_RET_FAIL;
	}
	
#if defined(_WINDOWS)

	zbx_snprintf(full_command, sizeof(full_command), "cmd /C \"%s\"", command);

	GetStartupInfo(&si);

	zabbix_log(LOG_LEVEL_DEBUG, "Execute command '%s'",full_command);

	if(!CreateProcess(
		NULL,	/* No module name (use command line) */
		full_command,/* Name of app to launch */
		NULL,	/* Default process security attributes */
		NULL,	/* Default thread security attributes */
		FALSE,	/* Don't inherit handles from the parent */
		0,	/* Normal priority */
		NULL,	/* Use the same environment as the parent */
		NULL,	/* Launch in the current directory */
		&si,	/* Startup Information */
		&pi))	/* Process information stored upon return */
	{
		return SYSINFO_RET_FAIL;
	}


#else /* not _WINDOWS */

	pid = zbx_fork(); /* run new thread 1 */
	switch(pid)
	{
	case -1:
		zabbix_log(LOG_LEVEL_WARNING, "fork failed for command '%s'",command);
		return SYSINFO_RET_FAIL;
	case 0:
		pid = zbx_fork(); /* run new tread 2 to replace by command */
		switch(pid)
		{
		case -1:
			zabbix_log(LOG_LEVEL_WARNING, "fork2 failed for '%s'",command);
			return SYSINFO_RET_FAIL;
		case 0:
			/* 
			 * DON'T REMOVE SLEEP
			 * sleep needed to return server result as "1"
			 * then we can run "execl"
			 * otherwise command print result into socket with STDOUT id
			 */
			sleep(3); 
			/**/
			
			/* replace thread 2 by the execution of command */
			if(execl("/bin/sh", "sh", "-c", command, (char *)0))
			{
				zabbix_log(LOG_LEVEL_WARNING, "execl failed for command '%s'",command);
			}
			/* In normal case the program will never reach this point */
			exit(0);
		default:
			waitpid(pid, NULL, WNOHANG); /* NO WAIT can be used for thread 2 closing */
			exit(0); /* close thread 1 and transmit thread 2 to system (solve zombie state) */
			break;
		}
	default:
		waitpid(pid, NULL, 0); /* wait thread 1 closing */
		break;
	}

#endif /* _WINDOWS */

	SET_UI64_RESULT(result, 1);
	
	return	SYSINFO_RET_OK;
}
