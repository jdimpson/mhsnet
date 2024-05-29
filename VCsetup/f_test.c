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


#define	FILE_CONTROL
#define	STDIO

#include	"global.h"
#include	"debug.h"

#include	"call.h"
#include	"setup.h"



/*
**	Test device.
*/

char *
f_test(dev_op, va)
	DevOp		dev_op;
	VarArgs *	va;
{
	register int	i;
	char *		a[4];
	static char	name[]	= "test";

	if ( dev_op == t_close )
	{
		MesgN(name, "close");

		if ( close(Fd) == SYSERROR )
			Reason = ErrnoStr(errno);
		else
			Reason = DEVOK;

		Fd = SYSERROR;
		FreeStr(&Fn);

		return Reason;
	}

	for ( i = 0 ; i < NARGS(va) ; i++ )
		a[i] = ARG(va, i);

	if ( i == 0 )
		a[i++] = DevNull;

	for ( ; i < 4 ; i++ )
		a[i] = EmptyString;

	if ( dev_op != t_open )
	{
		MesgN(name, "device %s %s %s %s", a[0], a[1], a[2], a[3]);
		return DEVOK;
	}

	MesgN(name, "open %s %s %s %s", a[0], a[1], a[2], a[3]);

	if ( access(a[0], 6) == SYSERROR || (Fd = open(a[0], O_RDWR)) == SYSERROR )
		return ErrnoStr(errno);

	FreeStr(&Fn);
	Fn = newstr(a[0]);

	VCtype = BYTESTREAM;

	return DEVOK;
}
