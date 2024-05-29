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
**	General purpose `file' device.
*/

char *
f_file(dev_op, va)
	DevOp		dev_op;
	VarArgs *	va;
{
	register char *	dev	= ARG(va, 0);

	if ( dev_op == t_close )
	{
		if ( close(Fd) == SYSERROR )
			Reason = ErrnoStr(errno);
		else
			Reason = DEVOK;

		Fd = SYSERROR;
		FreeStr(&Fn);

		return Reason;
	}

	if ( dev_op != t_open )
		return DEVFAIL;

	if ( access(dev, 6) == SYSERROR || (Fd = open(dev, O_RDWR)) == SYSERROR )
		return ErrnoStr(errno);

	FreeStr(&Fn);
	Fn = newstr(dev);

	VCtype = BYTESTREAM;

	return DEVOK;
}
