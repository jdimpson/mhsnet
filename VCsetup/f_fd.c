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


#define	STAT_CALL
#define	STDIO

#include	"global.h"
#include	"debug.h"

#include	"call.h"
#include	"setup.h"


static char *	fd_close _FA_((void));
static char *	fd_open _FA_((VarArgs *));



/*
**	`fd' device.
*/

char *
f_fd(dev_op, va)
	DevOp		dev_op;
	VarArgs *	va;
{
	Reason = DEVFAIL;

	switch ( dev_op )
	{
	case t_device:
		return tty_device(va);

	case t_open:
		return fd_open(va);

	case t_close:
		return fd_close();
	}

	Fatal2(english("unknown dev_op %d"), dev_op);

	return Reason;
}



static char *
fd_close()
{
	if ( close(Fd) == SYSERROR )
		Reason = ErrnoStr(errno);
	else
		Reason = DEVOK;

	Fd = SYSERROR;
	FreeStr(&Fn);

	return Reason;
}



static char *
fd_open(va)
	VarArgs *	va;
{
	register char *	cp;
	struct stat	statb;

	if ( NARGS(va) == 0 || (cp = ARG(va, 0)) == NULLSTR )
	{
		Fd = SYSERROR;
		Error(english("file descriptor number?"));
		return DEVFAIL;
	}

	if ( (Fd = atoi(cp)) < 0 || Fd > 9 )
	{
		Fd = SYSERROR;
		Error(english("file descriptor number \"%s\" must be >=0 || <=9"), cp);
		return DEVFAIL;
	}

	if ( fstat(Fd, &statb) == SYSERROR )
	{
		Fd = SYSERROR;
		return ErrnoStr(errno);
	}

	(void)SetRaw(Fd, 0, 1, 0, false);

	FreeStr(&Fn);
	Fn = newprintf("fd %s", cp);

	VCtype = BYTESTREAM;

	return DEVOK;
}
