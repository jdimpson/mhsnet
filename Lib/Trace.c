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


FILE *	TraceFd;

#define	Fflush	(void)fflush
#define	Fprintf	(void)fprintf



/*
**	Trace on TraceFd.
*/

/*VARARGS*/
void
#ifdef	ANSI_C
Trace(int l, char *fmt, ...)
{
#else	/* ANSI_C */
Trace(va_alist)
	va_dcl
{
	register int	l;
	char *		fmt;
#endif	/* ANSI_C */
	va_list		vp;
	static bool	intrace;
#	if	VPRINTF != 1
	register int	i;
	char *		a[10];
#	endif	/* VPRINTF != 1 */

#	ifdef	ANSI_C
	va_start(vp, fmt);
#	else	/* ANSI_C */
	va_start(vp);
	l = va_arg(vp, int);
	fmt = va_arg(vp, char *);
#	endif	/* ANSI_C */

	if ( fmt == NULLSTR )
	{
		if ( intrace )
			goto out;
		va_end(vp);
		return;
	}

	if ( intrace )
	{
		va_end(vp);
		return;
	}
	intrace = true;

	Fflush(stdout);
	Fflush(stderr);

	(void)fseek(TraceFd, (long)0, 2);

	if ( l > 0 )
	{
		putc('\t', TraceFd);
		while ( --l > 0 )
			putc(' ', TraceFd);
	}

#	if	VPRINTF != 1

	for ( i = 0 ; i < 10 ; i++ )
		a[i] = va_arg(vp, char *);

	Fprintf(TraceFd, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9]);

#	else	/* VPRINTF != 1 */

	(void)vfprintf(TraceFd, fmt, vp);

#	endif	/* VPRINTF != 1 */

out:	va_end(vp);

	putc('\n', TraceFd);
	Fflush(TraceFd);

	intrace = false;
}



/*
**	Initialise `TraceFd'.
*/

void
InitTrace()
{
	TraceFd = stderr;
}
