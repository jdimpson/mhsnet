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
**	Accept `udp' connection -- routine for `udpshell'.
*/

#include	"global.h"
#include	"debug.h"
#include	"Passwd.h"
#undef	P_ALL	/* defined in some TCP headers */
#include	"sysexits.h"

#include	"setup.h"
#include	"shell.h"

#if	UDP_IP == 1

#ifndef	_TYPES_
#include	<sys/types.h>
#endif

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



char *
a_udp(tp)
	VCtype *		tp;
{
	register int		sock;
	struct sockaddr_in	peer;
	struct sockaddr_in	self;
	struct hostent *	InetPeer;
	socklen_t		peer_len	= sizeof(peer);
	socklen_t		self_len	= sizeof(self);
	char			buf[1024];
	char *			cp;

	if ( recvfrom(0, buf, sizeof(buf), 0, (struct sockaddr *)&peer, &peer_len) == SYSERROR )
	{
		Syserror(CouldNot, "recvfrom", "fd 0");
		finish(EX_OSERR);
	}

	if ( (sock = socket(AF_INET, SOCK_DGRAM, 0)) == SYSERROR )
	{
		Syserror(CouldNot, "socket", "AF_INET, SOCK_DGRAM");
		finish(EX_OSERR);
	}

	/*
	**	Fork off parent so inetd can continue on DGRAM socket.
	*/

	for ( ;; )
	{
		switch ( fork() )
		{
		case 0:		break;
		case SYSERROR:	Syserror(CouldNot, ForkStr, EmptyString);	continue;
		default:	exit(EX_OK);
		}
		break;
	}

	Pid = getpid();

	if ( sock != 0 )
	{
		(void)close(0);
		(void)dup(sock);
	}

	if ( sock != 1 )
	{
		(void)close(1);
		(void)dup(sock);
	}

	if ( sock != 0 && sock != 1 )
		(void)close(sock);

	if
	(
		connect(0, (struct sockaddr *)&peer, sizeof(peer)) == SYSERROR
		||
		getsockname(0, (struct sockaddr *)&self, &self_len) == SYSERROR
	)
	{
		Syserror(CouldNot, "connect", "peer");
		finish(EX_NOHOST);
	}

	if
	(
		(InetPeer = (struct hostent *)gethostbyaddr
				(
					(char *)&peer.sin_addr,
					sizeof(peer.sin_addr),
					peer.sin_family
				)) == (struct hostent *)0
	)
	{
		Error(CouldNot, "gethostbyaddr", "peer");
		finish(EX_NOHOST);
	}

	self.sin_family = htons(self.sin_family);

	/** Needed for backward compatibility **/

	if ( write(1, (char *)&self, sizeof(self)) != sizeof(self) )
	{
		Syserror(CouldNot, WriteStr, "fd 1");
		finish(EX_OSERR);
	}

#	ifdef	SO_RCVBUF
	{
		int	size	= 4 * 8 * 1024;

		(void)setsockopt(0, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
	}
#	endif	/* SO_RCVBUF */

	if ( (cp = inet_ntoa(peer.sin_addr)) == (char *)(-1) || cp == NULLSTR )
		(void)sprintf(buf, "%lx", (PUlong)peer.sin_addr.s_addr);
	else
	{	/** take care of versions that return stack value (don't ask) **/
		char *	cp2;

		for ( cp2 = buf ; (*cp2++ = *cp++) ; )
			if ( cp2 >= &buf[(sizeof buf)-1] )
			{
				*cp2 = '\0';
				break;
			}
	}
	tp->VCname = newprintf("%s:%d", buf, ntohs(peer.sin_port));

	return newstr(InetPeer->h_name);
}
#endif	/* UDP_IP == 1 */
