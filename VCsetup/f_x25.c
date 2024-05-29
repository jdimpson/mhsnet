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

#	if	X25 == 1
#include	"iocmd.h"
#include	"x25.h"
#	endif	/* X25 == 1 */


/*
**	Establish X.25 connection to remote.
*/

char *
f_x25(dev_op, va)
	DevOp		dev_op;
	VarArgs *	va;
{
#	if	X25 == 1
	char *		address;
	int		controller;
	int		line;
	char *		home;
	char *		cp;

	extern int	x25call();
	extern char *	x25error();

	if ( dev_op != t_open )
	{
		Error(english("only open operation defined for X.25"));
		return DEVFAIL;
	}

	if ( NARGS(va) != 4 )
	{
		Error(english("need X.25 address, controller, line, and home"));
		return DEVFAIL;
	}

	if ( (address = ARG(va, 0)) == NULLSTR )
	{
		Error(english("X.25 address?"));
		return DEVFAIL;
	}

	if ( (cp = ARG(va, 1)) == NULLSTR )
	{
		Error(english("X.25 controller?"));
		return DEVFAIL;
	}

	controller = atol(cp);

	if ( (cp = ARG(va, 2)) == NULLSTR )
	{
		Error(english("X.25 line?"));
		return DEVFAIL;
	}

	line = atol(cp);

	if ( (home = ARG(va, 3)) == NULLSTR )
	{
		Error(english("home address?"));
		return DEVFAIL;
	}

	Trace5(1, "f_x25(%s, %d, %d, %s)", address, controller, line, home);

	if
	(
		(Fd = x25call(line, controller, X25_ADAPTER, DATA_MODE, address)) == SYSERROR
		||
		write(Fd, home, strlen(home)+1) == SYSERROR
	)
	{
		if ( (cp = x25error()) == NULLSTR )
			cp = ErrnoStr(errno);

		return cp;
	}

	FreeStr(&Fn);
	Fn = newstr("X.25");

	VCtype = BYTESTREAM;

	return DEVOK;

#	else	/* X25 == 1 */

	return english("Service not available");

#	endif	/* X25 == 1 */
}
