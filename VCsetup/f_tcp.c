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


#define	RECOVER
#define	SIGNALS
#define	STDIO

#include	"global.h"
#include	"debug.h"

#include	"call.h"
#include	"setup.h"


extern jmp_buf	Alrm_jmp;



/*
**	Establish TCP/IP connection to remote.
*/

char *
f_tcp(dev_op, va)
	DevOp		dev_op;
	VarArgs *	va;
{
	char *		target;
	char *		server;
	int		port;
	char *		Source;		/* Set this source address for bind(2) */
	bool		ZeroSource;	/* Don't set source address for bind(2) */

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
		Error(english("need TCP target, server[, port[, source]]"));
		return DEVFAIL;
	}

	if ( (target = ARG(va, 0)) == NULLSTR )
	{
		Error(english("TCP target?"));
		return DEVFAIL;
	}

	if ( (server = ARG(va, 1)) == NULLSTR )
	{
		Error(english("TCP server?"));
		return DEVFAIL;
	}

	if ( NARGS(va) < 3 || ARG(va, 2) == NULLSTR )
		port = 0;
	else
		port = atol(ARG(va, 2));

	if ( NARGS(va) < 4 || ARG(va, 3) == NULLSTR )
	{
		ZeroSource = false;
		Source = NULLSTR;
	}
	else
	if ( strcmp(ARG(va, 3), "0") )
	{
		ZeroSource = true;
		Source = NULLSTR;
	}
	else
	{
		ZeroSource = false;
		Source = newstr(ARG(va, 3));
	}

	Trace6(
		1, "f_tcp(%s, %s, %d, %s, %s)",
		target, server, port,
		ZeroSource?"zerosource":"false",
		Source==NULLSTR?NullStr:Source
	);

#	if	TCP_IP == 1

	if ( (Fd = ConnectSocket(target, server, "tcp", port, ZeroSource, Source)) == SYSERROR )
		return Reason;

	FreeStr(&Fn);
	Fn = newstr("TCP/IP");

	VCtype = BYTESTREAM;

	return DEVOK;

#	else	/* TCP_IP == 1 */

	Error(english("TCP/IP connection unavailable"));
	return DEVFAIL;

#	endif	/* TCP_IP == 1 */
}



/*
**	`close' for `IP' type circuit.
*/

char *
ip_close()
{
	if ( close(Fd) == SYSERROR )
		Reason = ErrnoStr(errno);
	else
		Reason = DEVOK;

	Fd = SYSERROR;
	FreeStr(&Fn);

	return Reason;
}



/*
**	`device' operations on `IP' type circuit.
*/

char *
ip_device(va)
	VarArgs *	va;
{
	register char *	cp;

	if ( NARGS(va) == 0 || (cp = ARG(va, 0)) == NULLSTR || cp[0] == '\0' )
	{
		Error(english("device control?"));
		return DEVFAIL;
	}

	if ( strcmp(cp, "flush") == STREQUAL )
	{
		char	c;

		if ( setjmp(Alrm_jmp) )
			return DEVOK;

		(void)signal(SIGALRM, alrmcatch);
		(void)alarm(MINSLEEP);

		while ( read(Fd, &c, 1) == 1 );

		(void)alarm((unsigned)0);
		return DEVOK;
	}

	Warn(english("unimplemented command \"%s\""), cp);
	return DEVFAIL;
}
