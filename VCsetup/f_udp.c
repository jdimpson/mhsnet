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


#define	STDIO

#include	"global.h"
#include	"debug.h"

#include	"call.h"
#include	"setup.h"

#if	UDP_IP == 1
#ifndef	_TYPES_
#include	<sys/types.h>
#endif

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
#include	<netdb.h>
#else	/* BSD4 >= 2 || BSD_IP == 1 */
#include	<socket.h>
#include	<in.h>
#include	<netdb.h>
#endif	/* BSD4 >= 2 || BSD_IP == 1 */
#endif	/* UDP_IP == 1 */



/*
**	Establish UDP/IP connection to remote.
*/

char *
f_udp(dev_op, va)
	DevOp		dev_op;
	VarArgs *	va;
{
	char *		target;
	char *		server;
	int		port;
	char *		Source;		/* Set this source address for bind(2) */
	bool		ZeroSource;	/* Don't set source address for bind(2) */
	char		namebuf[1024];

	switch ( dev_op )
	{
	case t_device:
		return ip_device(va);

	case t_open:
		break;

	default:
		return ip_close();
	}

	if ( NARGS(va) < 2 || NARGS(va) > 4 )
	{
		Error(english("need UDP target, server[, port[, source]]"));
		return DEVFAIL;
	}

	if ( (target = ARG(va, 0)) == NULLSTR )
	{
		Error(english("UDP target?"));
		return DEVFAIL;
	}

	if ( (server = ARG(va, 1)) == NULLSTR )
	{
		Error(english("UDP server?"));
		return DEVFAIL;
	}

#	if	UDP_IP == 1

	if ( NARGS(va) < 3 || ARG(va, 2) == NULLSTR )
		port = 0;
	else
		port = atol(ARG(va, 2));

	ZeroSource = false;

	if ( NARGS(va) < 4 || ARG(va, 3) == NULLSTR || strcmp(ARG(va, 3), "0") )
	{
		if ( gethostname(namebuf, sizeof namebuf) == SYSERROR )
		{
			Syserror(CouldNot, "gethostname", EmptyString);
			return DEVFAIL;
		}

		if ( namebuf[0] == '\0' )
		{
			static char	ghe[] = english("gethostname() returned NULL");
	
#			if	SYSTEM >= 5
			if ( uname((struct utsname *)namebuf) == SYSERROR || namebuf[0] == '\0' )
			{
#			endif	/* SYSTEM >= 5 */
				Reason = ghe;
				return DEVFAIL;
#			if	SYSTEM >= 5
			}

			Warn(ghe);

			(void)strcpy(namebuf, ((struct utsname *)namebuf)->nodename);
#			endif	/* SYSTEM >= 5 */
		}

		Source = newstr(namebuf);	/* UDP protocol needs bind(3) */
	}
	else
		Source = newstr(ARG(va, 3));

	Trace6(
		1, "f_udp(%s, %s, %d, %s, %s)",
		target, server, port,
		ZeroSource?"zerosource":"false",
		Source==NULLSTR?NullStr:Source
	);

	if ( (Fd = ConnectSocket(target, server, "udp", port, ZeroSource, Source)) == SYSERROR )
		return Reason;

#	ifdef	SO_RCVBUF
	{
		int	size	= 4 * 8 * 1024;

		(void)setsockopt(Fd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
	}
#	endif	/* SO_RCVBUF */

	FreeStr(&Fn);
	Fn = newstr("UDP/IP");

	VCtype = DATAGRAM;

	return DEVOK;

#	else	/* UDP_IP == 1 */

	Error(english("UDP/IP connection unavailable"));
	return DEVFAIL;

#	endif	/* UDP_IP == 1 */
}
