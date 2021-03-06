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

#ifndef ZABBIX_COMMS_H
#define ZABBIX_COMMS_H

typedef enum {
	ZBX_TCP_ERR_NETWORK = 1,
	ZBX_TCP_ERR_TIMEOUT
} zbx_tcp_errors;


#if defined(SOCKET) || defined(_WINDOWS)
	typedef SOCKET ZBX_SOCKET;
#else /* not SOCKET && not _WINDOWS*/
	typedef int ZBX_SOCKET;
#endif /* SOCKET || _WINDOWS */


typedef struct sockaddr_in ZBX_SOCKADDR;

typedef enum
{
	ZBX_BUF_TYPE_STAT = 0,
	ZBX_BUF_TYPE_DYN
} zbx_buf_type_t;

#define ZBX_STAT_BUF_LEN	2048

typedef struct zbx_sock
{
#if defined(HAVE_IPV6)
	ZBX_SOCKET	sockets[FD_SETSIZE];
	int		num_socks;
#endif /* HAVE_IPV6 */
	ZBX_SOCKET	socket;
	ZBX_SOCKET	socket2;
	char		buf_stat[ZBX_STAT_BUF_LEN];
	char		*buf_dyn;
	zbx_buf_type_t	buf_type;
	unsigned char accepted;
	char		*error;
	int		timeout;
} zbx_sock_t;

char*	zbx_tcp_strerror(void);
int	zbx_tcp_error(void);

struct hostent	*zbx_gethost(const char *hostname);

#if !defined(_WINDOWS)
void zbx_gethost_by_ip(const char *ip, char *host, size_t hostlen);
#endif /* WINDOWS */

void	zbx_tcp_init(zbx_sock_t *s, ZBX_SOCKET o);
int	zbx_tcp_connect(zbx_sock_t *s, const char *ip, unsigned short port, int timeout, const char* from_ip);

#define ZBX_TCP_NEW_PROTOCOL	0x01

#define zbx_tcp_send(s, d)		zbx_tcp_send_ext((s), (d), ZBX_TCP_NEW_PROTOCOL)
#define zbx_tcp_send_raw(s, d)	zbx_tcp_send_ext((s), (d), 0)

int     zbx_tcp_send_ext(zbx_sock_t *s, const char *data, unsigned char flags);

void    zbx_tcp_close(zbx_sock_t *s);

int zbx_tcp_listen(
	zbx_sock_t		*s,
	const char		*listen_ip,
	unsigned short	listen_port
	);

int	zbx_tcp_accept(zbx_sock_t *s);
void	zbx_tcp_unaccept(zbx_sock_t *s);

void    zbx_tcp_free(zbx_sock_t *s);

#define ZBX_TCP_READ_UNTIL_CLOSE 0x01

#define	zbx_tcp_recv(s, data) 	zbx_tcp_recv_ext(s, data, 0)

int	zbx_tcp_recv_ext(zbx_sock_t *s, char **data, unsigned char flags);

int	zbx_tcp_check_security(
	zbx_sock_t *s, 
	const char *ip_list, 
	int allow_if_empty
	);

#endif

#if defined(_WINDOWS)
#	if defined(__INT_MAX__) && __INT_MAX__ == 2147483647
		typedef int ssize_t;
#	else
		typedef long ssize_t;
#	endif /* __INT_MAX__ */

#	define ZBX_TCP_WRITE(s, b, bl)	((ssize_t)send((s), (b), (bl), 0))
#	define ZBX_TCP_READ(s, b, bl)	((ssize_t)recv((s), (b), (bl), 0))

#	define ZBX_TCP_ERROR	SOCKET_ERROR
#	define ZBX_SOCK_ERROR	INVALID_SOCKET

#	define zbx_sock_close(s)		if( ZBX_SOCK_ERROR != (s) ) closesocket(s)
#	define zbx_sock_last_error()	WSAGetLastError()

#	define ZBX_SOCK_ERR_TIMEDOUT	WSAETIMEDOUT
#else
#	define ZBX_TCP_WRITE(s, b, bl)	((ssize_t)write((s), (b), (bl)))
#	define ZBX_TCP_READ(s, b, bl)	((ssize_t)read((s), (b), (bl)))

#	define ZBX_TCP_ERROR	-1
#	define ZBX_SOCK_ERROR	-1

#	define	zbx_sock_close(s)		if( ZBX_SOCK_ERROR != (s) ) close(s)
#	define  zbx_sock_last_error()	errno

#	define ZBX_SOCK_ERR_TIMEDOUT	EINTR

#endif /* _WINDOWS */
