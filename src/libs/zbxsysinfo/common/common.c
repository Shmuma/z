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
#include "cfg.h"

#if !defined(_WINDOWS)
#	define VFS_TEST_FILE "/etc/passwd"
#	define VFS_TEST_REGEXP "root"

extern char** environ;
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
	{"web.page.isregexp",	CF_USEUPARAM,	WEB_PAGE_ISREGEXP,	0,	"localhost,,80,200"},

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
		if(NULL == fgets(c,MAX_STRING_LEN,f))
		{
			zbx_fclose(f);
			return	SYSINFO_RET_FAIL;
		}
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


static char** updated_environment ()
{
	char** env = (char**)environ;
	int count = 0, i;
	char** res;
	int len;
	const char* PATH = "PATH=";
	const char* CONF = "ZBX_CONFDIR=";
	const char* PATH_POST = "/zabbix/bin/";
	const char* CONF_POST = "/zabbix/";

	while (env[count++]);
	res = (char**)malloc ((count+1) * sizeof (char*));

	i = 0;
	while (env[i]) {
		if (strncmp (env[i], PATH, strlen (PATH)) == 0) {
			len = strlen (env[i]) + 1 + strlen (ZBX_SYSCONF_DIR) + strlen (PATH_POST) + 1;
			res[i] = (char*)malloc (len);
			snprintf (res[i], len, "%s%s%s:%s", PATH, ZBX_SYSCONF_DIR, PATH_POST, env[i]+strlen (PATH));
		}
		else
			res[i] = env[i];
		i++;
	}

	len = strlen (ZBX_SYSCONF_DIR) + strlen (CONF) + strlen (CONF_POST) + 1;
	res[i] = (char*)malloc (len);
	snprintf (res[i], len, "%s%s%s", CONF, ZBX_SYSCONF_DIR, CONF_POST);
	res[i+1] = NULL;

	return res;
}



int	EXECUTE_STR(const char *cmd, const char *param, unsigned flags, AGENT_RESULT *result)
{

#if defined(_WINDOWS)

	STARTUPINFO si = {0};
	PROCESS_INFORMATION pi = {0};
	SECURITY_ATTRIBUTES sa;
	HANDLE hWrite=NULL, hRead=NULL;

#else /* not _WINDOWS */

	pid_t	p;
	int out[2] = {-1, -1}, err[2] = {-1, -1}, i, out_v, err_v, status;
	char* shell;
	fd_set fds;
	struct timeval tv;
	int sel_ret;

#endif /* _WINDOWS */

	int	ret = SYSINFO_RET_FAIL;

	char	stat_buf[128];
	char	*cmd_result=NULL;
	char	*cmd_error=NULL;
	char	*command=NULL;
	int	len;

	assert(result);

	init_result(result);

	cmd_result = zbx_dsprintf(cmd_result,"");
	memset(stat_buf, 0, sizeof(stat_buf));

#if defined(_WINDOWS)

	/* Set the bInheritHandle flag so pipe handles are inherited */
	sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa.bInheritHandle = TRUE; 
	sa.lpSecurityDescriptor = NULL; 

	/* Create a pipe for the child process's STDOUT */
	if (! CreatePipe(&hRead, &hWrite, &sa, sizeof(cmd_result))) 
	{
		zabbix_log(LOG_LEVEL_DEBUG, "Unable to create pipe [%s]", strerror_from_system(GetLastError()));
		ret = SYSINFO_RET_FAIL;
		goto lbl_exit;
	}

	/* Fill in process startup info structure */
	memset(&si,0,sizeof(STARTUPINFO));
	si.cb		= sizeof(STARTUPINFO);
	si.dwFlags	= STARTF_USESTDHANDLES;
	si.hStdInput	= GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput	= hWrite;
	si.hStdError	= hWrite;

	command = zbx_dsprintf(command, "cmd /C \"%s\"", param);

	/* Create new process */
	if (!CreateProcess(NULL,command,NULL,NULL,TRUE,0,NULL,NULL,&si,&pi))
	{
		zabbix_log(LOG_LEVEL_DEBUG, "Unable to create process: '%s' [%s]", command, strerror_from_system(GetLastError()));

		ret = SYSINFO_RET_FAIL;
		goto lbl_exit;
	}
	CloseHandle(hWrite);	hWrite = NULL;

	/* Read process output */
	while( ReadFile(hRead, stat_buf, sizeof(stat_buf)-1, &len, NULL) && len > 0 )
	{
		cmd_result = zbx_strdcat(cmd_result, stat_buf);
		memset(stat_buf, 0, sizeof(stat_buf));
	}

	/* Don't wait child process exiting. */
	/* WaitForSingleObject( pi.hProcess, INFINITE ); */

	/* Terminate child process */
	/* TerminateProcess(pi.hProcess, 0); */

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	CloseHandle(hRead);	hRead = NULL;
		

#else /* not _WINDOWS */
	shell = "/bin/sh";
	command = zbx_dsprintf(command, "%s", param);

	/* initialize pipes for stdout and stderr */
	pipe (out);
	pipe (err);

	p = fork ();

	if (p < 0) {
		zabbix_log (LOG_LEVEL_WARNING, "Cannot fork child process");
		ret = SYSINFO_RET_FAIL;
		goto lbl_exit;
	}

	/* child process */
	if (p == 0) {
		/* close all file descriptors, except out[1] and err[1] */
		for (i = 0; i < 4096; i++)
			if (i != out[1] && i != err[1])
				if (close (i) < 0)
					break;

		dup2 (out[1], 1);
		dup2 (err[1], 2);

		/* append binary checks dir to process's path */
		/* append special variable CONFDIR/checks for check's config files */
		execle (shell, shell, "-c", command, NULL, updated_environment ());
	}
	else { /* parent process */
		/* close write descriptors */
		close (out[1]);
		close (err[1]);

		FD_ZERO (&fds);
		FD_SET (out[0], &fds);
		FD_SET (err[0], &fds);
		out_v = err_v = 1;
		tv.tv_sec = CONFIG_TIMEOUT;
		tv.tv_usec = 0;

		while (sel_ret = select (out[0] > err[0] ? out[0] + 1 : err[0] + 1, &fds, NULL, NULL, &tv)) {
			if (FD_ISSET (out[0], &fds)) {
			/* read data */
				while ((len = read (out[0], stat_buf, sizeof (stat_buf)-1)) > 0) {
					/* remember */
					stat_buf[len] = 0;
					cmd_result = zbx_strdcat (cmd_result, stat_buf);
				}
				if (len <= 0)
					out_v = 0;
			}
			if (FD_ISSET (err[0], &fds)) {
				/* read data */
				while ((len = read (err[0], stat_buf, sizeof (stat_buf)-1)) > 0) {
					stat_buf[len] = 0;
					for (i = 0; i < len; i++)
					    if (isspace (stat_buf[i]))
						stat_buf[i] = ' ';
					if (!cmd_error || strlen (cmd_error) < 256)
						cmd_error = zbx_strdcat (cmd_error, stat_buf);
				}
				if (len <= 0)
					err_v = 0;
			}

			FD_ZERO (&fds);
			if (out_v)
				FD_SET (out[0], &fds);
			if (err_v)
				FD_SET (err[0], &fds);
			if (!out_v && !err_v)
				break;
		}

		/* timeout occured */
		if (sel_ret == 0) {
			kill (p, SIGTERM);
			ret = SYSINFO_RET_TIMEOUT;
		}
		waitpid (p, &status, 0);
	}

	close (out[0]);
	close (err[0]);

#endif /* _WINDOWS */

	zabbix_log(LOG_LEVEL_DEBUG, "Before");

	zbx_rtrim(cmd_result,"\n\r\0");

	zabbix_log(LOG_LEVEL_DEBUG, "Run remote command [%s] Result [%d] [%.20s] (stderr: [%d] [%.20s])...", 
		   command, strlen(cmd_result), cmd_result, cmd_error ? strlen (cmd_error) : 0, cmd_error ? cmd_error : "");

	SET_TEXT_RESULT(result, strdup(cmd_result));
	if (cmd_error)
		SET_ERR_RESULT(result, strdup(cmd_error));

	ret = SYSINFO_RET_OK;

lbl_exit:

#if defined(_WINDOWS)
	if ( hWrite )	{ CloseHandle(hWrite);	hWrite = NULL; }
	if ( hRead)	{ CloseHandle(hRead);	hRead = NULL; }
#else /* not _WINDOWS */
	if (out[0] >= 0)
		close (out[0]);
	if (out[1] >= 0)
		close (out[1]);
	if (err[0] >= 0)
		close (err[0]);
	if (err[1] >= 0)
		close (err[1]);
#endif /* _WINDOWS */

	zbx_free(command)
	zbx_free(cmd_result);
	zbx_free(cmd_error);

	return ret;
}

int	EXECUTE_INT(const char *cmd, const char *command, unsigned flags, AGENT_RESULT *result)
{
	int	ret	= SYSINFO_RET_FAIL;

	ret = EXECUTE_STR(cmd,command,flags,result);

	if(SYSINFO_RET_OK == ret)
	{
		if( NULL == GET_DBL_RESULT(result) )
		{
			zabbix_log(LOG_LEVEL_WARNING, "Remote command [%s] result is not double", command);
			ret = SYSINFO_RET_FAIL;
		}
		UNSET_RESULT_EXCLUDING(result, AR_DOUBLE);
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
