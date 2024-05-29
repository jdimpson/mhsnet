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


#define	STDARGS
#define	STDIO

#include	"global.h"
#include	"debug.h"



/*
**	Error message output routine:
**		precede (non-tty) output with datestamp;
**		assumes first arg is error decription,
**			second arg is printf format string,
**			other args are printf parameters;
**		appends '\n'.
*/

/*VARARGS*/

#ifdef	ANSI_C

void
MesgN(char *err, ...)
{
	va_list	vp;
	char *	fmt;

	va_start(vp, err);
	fmt = va_arg(vp, char *);
	MesgV(err, fmt, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);
}

#else	/* ANSI_C */

void
MesgN(va_alist)
	va_dcl
{
	va_list	vp;
	char *	err;
	char *	fmt;

	va_start(vp);
	err = va_arg(vp, char *);
	fmt = va_arg(vp, char *);
	MesgV(err, fmt, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);
}

#endif	/* ANSI_C */
