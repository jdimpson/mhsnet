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
**	Report on ErrorFd
*/

/*VARARGS*/
void
#ifdef	ANSI_C
Report(char *fmt, ...)
{
	va_list vp;

	va_start(vp, fmt);

#else	/* ANSI_C */

Report(va_alist)
	va_dcl
{
	char *	fmt;
	va_list vp;

	va_start(vp);
	fmt = va_arg(vp, char *);
#endif	/* ANSI_C */

	MesgV(english("report"), fmt, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);
}
