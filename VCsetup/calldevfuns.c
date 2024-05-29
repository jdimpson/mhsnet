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
#include	"call.h"



extern char *	f_fd _FA_((DevOp, VarArgs *));
extern char *	f_file _FA_((DevOp, VarArgs *));
extern char *	f_test _FA_((DevOp, VarArgs *));
extern char *	f_tty _FA_((DevOp, VarArgs *));

#if	UDIAL == 1
extern char *	f_udial _FA_((DevOp, VarArgs *));
#endif	/* UDIAL == 1 */
#if	UDP_IP == 1
extern char *	f_udp _FA_((DevOp, VarArgs *));
#endif	/* UDP_IP == 1 */
#if	TCP_IP == 1
extern char *	f_tcp _FA_((DevOp, VarArgs *));
#endif	/* TCP_IP == 1 */
#if	X25 == 1
extern char *	f_x25 _FA_((DevOp, VarArgs *));
#endif	/* X25 == 1 */
#if	XTI == 1
extern char *	f_xti _FA_((DevOp, VarArgs *));
#endif	/* XTI == 1 */

/*
**	Available virtual circuit types.
*/

Device	DevFuns[] =
{
	{"dial",	f_tty},
	{"fd",		f_fd},
	{"file",	f_file},
#	if	TCP_IP == 1
	{"tcp",		f_tcp},
#	endif	/* TCP_IP == 1 */
	{"test",	f_test},
	{"tty",		f_tty},
#	if	UDIAL == 1
	{"udial",	f_udial},
#	endif	/* UDIAL == 1 */
#	if	UDP_IP == 1
	{"udp",		f_udp},
#	endif	/* UDP_IP == 1 */
#	if	X25 == 1
	{"x25",		f_x25},
#	endif	/* X25 == 1 */
#	if	XTI == 1
	{"xti",		f_xti},
#	endif	/* XTI == 1 */
};

int	NDevs	= ((sizeof DevFuns)/sizeof(Device));
