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


#include	"global.h"
#include	"shell.h"
#include	"setup.h"


/*
**	Match VC types by invoked name.
*/

extern char *	a_tty _FA_((VCtype *));
extern char *	a_ttyx _FA_((VCtype *));
extern void	a_ttyparams _FA_((VCtype *));
#if	UDP_IP == 1
extern char *	a_udp _FA_((VCtype *));
#endif	/* UDP_IP == 1 */
#if	TCP_IP == 1
extern char *	a_tcp _FA_((VCtype *));
#endif	/* TCP_IP == 1 */
#if	X25 == 1
extern char *	a_x25 _FA_((VCtype *));
#endif	/* X25 == 1 */
#if	XTI == 1
extern char *	a_xti _FA_((VCtype *));
#endif	/* XTI == 1 */

/*
**	Declare acceptable VC types.
*/

static char	Params[]	= "-fl";		/* No fork, lock already set */
static char	Starts[]	= SHELLSTARTS;
static char	XStarts[]	= XSHELLSTARTS;
#if	SUN3 == 1
static char	NParams[]	= "-flB";		/* No fork, lock already set, batchmode */
static char	XParams[]	= "-cflB";		/* Cooked, no fork, lock already set, batchmode */
static char	NStarts[]	= "daemon starts ...\n";
static char	KStarts[]	= "daemon starts ... -C\n";
static char	CStarts[]	= "daemon 2 starts ...\n";
static char	PStarts[]	= "PNdaemon starts ...\n";
static char	NNdaemon[]	= "NNdaemon";
#endif	/* SUN3 == 1 */

VCtype		Shells[] =
{
/*	name          func    args          reply  mesg     daemon      params   descrip               vcp		VCtype		VCname */

	{"ttyshell",  a_tty,  "ttyparams",  true,  Starts,  NULLSTR,    Params,  "tty",                a_ttyparams,	BYTESTREAM},
	{"ttyxshell", a_ttyx, "ttyxparams", true,  XStarts, NULLSTR,    Params,  "cooked tty",         a_ttyparams,	BYTESTREAM},
#	if	TCP_IP == 1
	{"tcpshell",  a_tcp,  "tcpparams",  true,  Starts,  NULLSTR,    Params,  "TCP/IP",             (vFuncp)0,	BYTESTREAM},
#	endif	/* TCP_IP == 1 */
#	if	UDP_IP == 1
	{"udpshell",  a_udp,  "udpparams",  true,  Starts,  NULLSTR,    Params,  "UDP/IP",             (vFuncp)0,	DATAGRAM},
#	endif	/* UDP_IP == 1 */
#	if	X25 == 1
	{"x25shell",  a_x25,  "x25params",  true,  Starts,  NULLSTR,    Params,  "X-25",               (vFuncp)0,	BYTESTREAM},
#	endif	/* X25 == 1 */
#	if	XTI == 1
	{"xtishell",  a_xti,  "xtiparams",  true,  Starts,  NULLSTR,    Params,  "XTI",               (vFuncp)0,	BYTESTREAM},
#	endif	/* XTI == 1 */
#	if	SUN3 == 1
	{"NNshell",   a_tty,  "NNparams",   false, NStarts, NNdaemon,   NParams, "SUNIII tty",         a_ttyparams,	BYTESTREAM},
	{"XNshell",   a_ttyx, "XNparams",   false, KStarts, NNdaemon,   XParams, "cooked SUNIII tty",  a_ttyparams,	BYTESTREAM},
	{"CNshell",   a_tty,  "CNparams",   false, CStarts, "CNdaemon", NParams, "checked SUNIII tty", a_ttyparams,	BYTESTREAM},
	{"PNshell",   a_tty,  "PNparams",   false, PStarts, "PNdaemon", NParams, "PPSN SUNIII tty",    a_ttyparams,	BYTESTREAM},
#	if	UDP_IP == 1
	{"ENshell",   a_udp,  "ENparams",   false, NULLSTR, "ENdaemon", Params,  "SUNIII UPD/IP",      (vFuncp)0,	DATAGRAM},
#	endif	/* UDP_IP == 1 */
#	if	TCP_IP == 1
	{"TNshell",   a_tcp,  "NNparams",   false, NStarts, NNdaemon,   Params,  "SUNIII TCP/IP",      (vFuncp)0,	BYTESTREAM},
#	endif	/* TCP_IP == 1 */
#	endif	/* SUN3 == 1 */
	{NULLSTR}	/* End of array */
};
