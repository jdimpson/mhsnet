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
**	Warning on ErrorFd
*/

/*VARARGS*/
#ifdef	ANSI_C
void
Warn(char *fmt, ...)
{
	va_list vp;

	va_start(vp, fmt);
	MesgV(english("warning"), fmt, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);
}
#else	/* ANSI_C */
void
Warn(va_alist)
	va_dcl
{
	va_list vp;
	char *	fmt;

	va_start(vp);
	fmt = va_arg(vp, char *);
	MesgV(english("warning"), fmt, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);
}
#endif	/* ANSI_C */
