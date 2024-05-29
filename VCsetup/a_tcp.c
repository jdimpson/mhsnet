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
**	Accept `tcp' connection -- routine for `tcpshell'.
*/

#include	"global.h"
#include	"debug.h"
#include	"Passwd.h"
#undef	P_ALL	/* defined in some TCP headers */
#include	"sysexits.h"

#include	"setup.h"
#include	"shell.h"

#if	TCP_IP == 1

#ifndef	_TYPES_
#include	<sys/types.h>
#endif

#if	EXCELAN == 1
#include	<sys/socket.h>
#include	<netinet/in.h>
#else	/* EXCELAN == 1 */
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



char *
a_tcp(tp)
	VCtype *		tp;
{
#	if	EXCELAN == 1
	char *			name;
	extern char *		raddr();
	extern long		Ssockaddr;
#	else	/* EXCELAN == 1 */
	struct sockaddr_in	peer;
	struct hostent *	InetPeer;
	socklen_t		peer_len	= sizeof(peer);
	char *			cp;
	char			buf[128];
#	endif	/* EXCELAN == 1 */

#	if	EXCELAN == 1
	if ( (name = raddr(Ssockaddr)) == NULLSTR )
	{
		Error("Bad socket address 0x%lx", (PUlong)Ssockaddr);
		finish(EX_NOHOST);
	}

	return name;

#	else	/* EXCELAN == 1 */

	if ( getpeername(0, (struct sockaddr *)&peer, &peer_len) == SYSERROR )
	{
		Syserror(CouldNot, "getpeername", "fd 0");
		finish(EX_OSERR);
	}

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
		extern int	h_errno;

		Warn("gethostbyaddr(3) failed with h_errno=%d", h_errno);
		return tp->VCname;
	}

	return newstr(InetPeer->h_name);
#	endif	/* EXCELAN == 1 */
}
#endif	/* TCP_IP == 1 */
