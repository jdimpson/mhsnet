/*
**	Copyright 2012 Piers Lauder
**
**	This file is part of MHSnet.
**
**	MHSnet is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	MHSnet is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with MHSnet.  If not, see <http://www.gnu.org/licenses/>.
*/


/*
**	Make a circuit via an internet socket to remote internet target.
*/

#define	SIGNALS
#define	STDIO

#include	"global.h"
#include	"debug.h"
#include	"call.h"

#if	UDP_IP == 1 || TCP_IP == 1
#ifdef	TWG_IP
#include	<sys/twg_config.h>
#endif

#ifndef	_TYPES_
#include	<sys/types.h>
#endif

#if	EXCELAN == 1
#include	<sys/socket.h>
#include	<sys/utsname.h>
#include	<netinet/in.h>
#include	<netdb.h>
#else	/* EXCELAN == 1 */
#if	SYSTEM >= 5
#include	<sys/utsname.h>
#endif	/* SYSTEM >= 5 */
#if	BSD4 >= 2 || BSD_IP == 1
#ifdef	M_XENIX
#ifndef	scounix
#include	<sys/types.tcp.h>
#endif
#endif	/* M_XENIX */
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<netdb.h>
#else	/* BSD4 >= 2 || BSD_IP == 1 */
#include	<socket.h>
#include	<in.h>
#include	<netdb.h>
#endif	/* BSD4 >= 2 || BSD_IP == 1 */
#endif	/* EXCELAN == 1 */

#include	<setjmp.h>


#define	HOSTNAMESIZE	256

static jmp_buf		cur_env;
void			time_out _FA_((int));
#endif	/* UDP_IP == 1 || TCP_IP == 1 */



/*
**	TCP/UDP socket establishment routines.
*/

#if	EXCELAN != 1

int
ConnectSocket(Target, Server, Proto, Port, ZeroSource, Source)
	char *			Target;
	char *			Server;
	char *			Proto;
	int			Port;
	bool			ZeroSource;
	char *			Source;
{
#	if	UDP_IP == 1 || TCP_IP == 1
	struct hostent *	host;
	struct sockaddr_in	dest;
	struct sockaddr_in	my_sock;
	int			sock_type;
	register int		s;
	static char		unkad[]	= english("unknown internet host/address: \"%s\"");

	if ( Target == NULLSTR )
		Target = NullStr;

	if ( Server == NULLSTR )
		Server = NullStr;

	if ( Proto == NULLSTR )
		Proto = NullStr;

	Trace7(
		1, "ConnectSocket(%s, %s, %s, %d, %d, %s)",
		Target, Server, Proto, Port,
		ZeroSource, Source==NULLSTR?NullStr:Source
	);

#	if	UDP_IP == 1
	if ( strccmp(Proto, "udp") == STREQUAL )
		sock_type = SOCK_DGRAM;
	else
#	endif	/* UDP_IP == 1 */
#	if	TCP_IP == 1
	if ( strccmp(Proto, "tcp") == STREQUAL )
		sock_type = SOCK_STREAM;
	else
#	endif	/* TCP_IP == 1 */
	{
		Reason = newprintf(english("unrecognised socket protocol type: \"%s\""), Proto);
		return SYSERROR;
	}

	if ( Port )
		Port = htons((Ushort)Port);
	else
	{
		struct servent *	serv;

		if ( (serv = (struct servent *)getservbyname(Server, Proto)) == (struct servent *)0 )
		{
			Reason = newprintf(english("unknown service: \"%s/%s\""), Server, Proto);
			return SYSERROR;
		}

		Port = serv->s_port;
	}

	Trace2(2, "ConnectSocket => port %d", (int)ntohs(Port));

	(void)strclr((char *)&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = Port;

	if ( (host = (struct hostent *)gethostbyname(Target)) == (struct hostent *)0 )
	{
		if ( (dest.sin_addr.s_addr = inet_addr(Target)) == -1 )
		{
			Reason = newprintf(unkad, Target);
			return SYSERROR;
		}
	}
	else
		bcopy(host->h_addr, (char *)&dest.sin_addr, host->h_length);

	Trace2(2, "ConnectSocket => dest %s", inet_ntoa(dest.sin_addr));

	if ( (s = socket(AF_INET, sock_type, 0)) == SYSERROR )
	{
		Reason = newprintf(CouldNot, "socket", ErrnoStr(errno));
		return SYSERROR;
	}

	if ( Source != NULLSTR || ZeroSource )
	{
		(void)strclr((char *)&my_sock, sizeof(my_sock));
		my_sock.sin_family = AF_INET;

		if ( ZeroSource )
			my_sock.sin_addr.s_addr = INADDR_ANY;
		else
		{
			if ( (host = (struct hostent *)gethostbyname(Source)) == (struct hostent *)0 )
			{
				if ( (my_sock.sin_addr.s_addr = inet_addr(Source)) == -1 )
				{
					my_sock.sin_addr.s_addr = INADDR_ANY;
					Warn(unkad, Source);
				}
			}
			else
				bcopy(host->h_addr, (char *)&my_sock.sin_addr, host->h_length);
		}

		Trace2(2, "ConnectSocket => bind addr %s", inet_ntoa(my_sock.sin_addr));

		if ( bind(s, (struct sockaddr *)&my_sock, sizeof(my_sock)) == SYSERROR )
		{
			Reason = newprintf(CouldNot, "bind", ErrnoStr(errno));
			return SYSERROR;
		}
	}

#	if	UDP_IP == 1
	if ( sock_type == SOCK_DGRAM )
	{
		socklen_t		my_sock_len = sizeof(my_sock);
		socklen_t		dest_len = sizeof(dest);
		struct sockaddr_in	buf;
		static vFuncp		old_al_sig;

		/*
		**	Send (what ought to be) garbage to get things going
		**	(but isn't for backward compatibility with SUNIII).
		*/

		if ( getsockname(s, (struct sockaddr *)&my_sock, &my_sock_len) == SYSERROR )
		{
			Reason = newprintf(CouldNot, "getsockname", ErrnoStr(errno));
			return SYSERROR;
		}

		my_sock.sin_family = htons(my_sock.sin_family);

		Trace2(2, "ConnectSocket => sending to addr %s", inet_ntoa(dest.sin_addr));

		if ( sendto(s, (char *)&my_sock, sizeof(my_sock), 0, (struct sockaddr *)&dest, sizeof(dest)) == SYSERROR )
		{
			Reason = newprintf(CouldNot, "sendto", ErrnoStr(errno));
			return SYSERROR;
		}

		Trace2(2, "ConnectSocket => sent addr %s", inet_ntoa(my_sock.sin_addr));

		if ( setjmp(cur_env) )
		{
			(void)signal(SIGALRM, old_al_sig);
			Reason = newprintf(english("recvfrom(2) timed out"));
			return SYSERROR;
		}

		Trace2(2, "ConnectSocket => receiving from %s", inet_ntoa(dest.sin_addr));

		old_al_sig = (vFuncp)signal(SIGALRM, time_out);
		(void)alarm(TimeOut);

		buf.sin_addr.s_addr = INADDR_ANY;

		if ( recvfrom(s, (char *)&buf, sizeof(buf), 0, (struct sockaddr *)&dest, &dest_len) == SYSERROR )
		{
			Reason = newprintf(CouldNot, "recvfrom", ErrnoStr(errno));
			(void)alarm((unsigned)0);
			(void)signal(SIGALRM, old_al_sig);
			return SYSERROR;
		}

		(void)alarm((unsigned)0);
		(void)signal(SIGALRM, old_al_sig);

		Trace2(2, "ConnectSocket => received addr %s", inet_ntoa(buf.sin_addr));
	}
	else
#	endif	/* UDP_IP == 1 */
		Trace3(2, "ConnectSocket => socket %d, port %d", s, (int)my_sock.sin_port);

	if ( connect(s, (struct sockaddr *)&dest, sizeof(dest)) == SYSERROR )
	{
		Reason = newprintf(CouldNot, "connect", ErrnoStr(errno));
		return SYSERROR;
	}

	return s;

#	else	/* UDP_IP == 1 || TCP_IP == 1 */

	Reason = english("Service not available");
	return SYSERROR;

#	endif	/* UDP_IP == 1 || TCP_IP == 1 */
}

#else	/* EXCELAN != 1 */

int
ConnectSocket(Target, Server, Proto, Port, ZeroSource, Source)
	char *			Target;
	char *			Server;
	char *			Proto;
	int			Port;
	bool			ZeroSource;
	char *			Source;
{
#	if	UDP_IP == 1 || TCP_IP == 1
	struct utsname		my_name;
	char *			nmptr;
	extern long		rhost();
	struct sockaddr_in	my_sock;
	struct sockaddr_in	dest;
	int			sock_type;
	register int		s;

	if ( Target == NULLSTR )
		Target = NullStr;

	if ( Server == NULLSTR )
		Server = NullStr;

	if ( Proto == NULLSTR )
		Proto = NullStr;

	Trace7(
		1, "ConnectSocket(%s, %s, %s, %d, %d, %s)",
		Target, Server, Proto, Port,
		ZeroSource, Source==NULLSTR?NullStr:Source
	);

#	if	UDP_IP == 1
	if ( strccmp(Proto, "udp") == STREQUAL )
		sock_type = SOCK_DGRAM;
	else
#	endif	/* UDP_IP == 1 */
#	if	TCP_IP == 1
	if ( strccmp(Proto, "tcp") == STREQUAL )
		sock_type = SOCK_STREAM;
	else
#	endif	/* TCP_IP == 1 */
	{
		Reason = newprintf(english("unrecognised socket protocol type: \"%s\""), Proto);
		return SYSERROR;
	}

	if ( Port )
		Port = htons((Ushort)Port);
	else
	{
		struct servent *	serv;

		if ( (serv = getservbyname(Server, Proto)) == (struct servent *)0 )
		{
			Reason = newprintf(english("unknown service: \"%s/%s\""), Server, Proto);
			return SYSERROR;
		}

		Port = serv->s_port;
	}

	Trace2(2, "ConnectSocket => port %d", ntohs(Port));

	(void)strclr((char *)&my_sock, sizeof(my_sock));

	(void)uname(&my_name);
	nmptr = my_name.nodename;

	Trace2(2, "ConnectSocket => hostname %s", nmptr);

	if ( (my_sock.sin_addr.s_addr = rhost(&nmptr)) == SYSERROR )
	{
		Reason = english("could not find my inet address");
		return SYSERROR;
	}

	my_sock.sin_family = AF_INET;
	my_sock.sin_port = 0;

	(void)strclr((char *)&dest, sizeof(dest));

	if ( (dest.sin_addr.s_addr = rhost(&Target)) == SYSERROR )
	{
		Reason = newprintf(english("unknown internet host: \"%s\""), Target);
		return SYSERROR;
	}

	dest.sin_family = AF_INET;
	dest.sin_port = Port;

	if ( (s = socket(sock_type, (struct sockproto *)0, &my_sock, 0)) == SYSERROR )
	{
		Reason = newprintf(CouldNot, "socket", ErrnoStr(errno));
		return SYSERROR;
	}

	Trace3(2, "ConnectSocket => socket %d, port %d", s, (int)my_sock.sin_port);

#	if	UDP_IP == 1
	if ( sock_type == SOCK_DGRAM )
	{
		int			my_sock_len = sizeof(my_sock);
		int			dest_len = sizeof(dest);
		static vFuncp		old_al_sig;

		/*
		**	Send (what ought to be) garbage to get things going
		**	(but isn't for backward compatibility with SUNIII).
		*/

		Trace2(2, "ConnectSocket => sending on port %d", (int)dest.sin_port);

		if ( sendto(s, &my_sock, sizeof(my_sock), 0, &dest, sizeof(dest)) == SYSERROR )
		{
			Reason = newprintf(CouldNot, "sendto", ErrnoStr(errno));
			return SYSERROR;
		}

		if ( setjmp(cur_env) )
		{
			Reason = TIMEOUT;
			(void)signal(SIGALRM, old_al_sig);
			return SYSERROR;
		}

		old_al_sig = (vFuncp)signal(SIGALRM, time_out);
		(void)alarm(TimeOut);

		if ( recvfrom(s, buf, sizeof(buf), 0, &dest, &dest_len) == SYSERROR )
		{
			Reason = newprintf(CouldNot, "recvfrom", ErrnoStr(errno));
			(void)alarm(0);
			(void)signal(SIGALRM, old_al_sig);
			return SYSERROR;
		}

		(void)alarm((unsigned)0);
		(void)signal(SIGALRM, old_al_sig);

		Trace2(2, "ConnectSocket => received from port %d", (int)dest.sin_port);
	}
#	endif	/* UDP_IP == 1 */

	if ( connect(s, &dest) == SYSERROR )
	{
		Reason = newprintf(CouldNot, "connect", ErrnoStr(errno));
		return SYSERROR;
	}

	return s;

#	else	/* UDP_IP == 1 || TCP_IP == 1 */

	Reason = english("Service not available");
	return SYSERROR;

#	endif	/* UDP_IP == 1 || TCP_IP == 1 */
}
#endif	/* EXCELAN != 1 */



#if	UDP_IP == 1
void
time_out(sig)
	int	sig;
{
	longjmp(cur_env, 1);
}
#endif	/* UDP_IP == 1 */
