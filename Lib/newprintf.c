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
**	`sprintf' into allocated string.
*/

/*VARARGS*/
#if	ANSI_C
char *
newprintf(char *fmt, ...)
{
	va_list	vp;
	char *	val;

	va_start(vp, fmt);
	val = newvprintf(fmt, vp);
	va_end(vp);

	return val;
}
#else	/* ANSI_C */
char *
newprintf(va_alist)
	va_dcl
{
	va_list	vp;
	char *	val;
	char *	fmt;

	va_start(vp);
	fmt = va_arg(vp, char *);
	val = newvprintf(fmt, vp);
	va_end(vp);

	return val;
}
#endif	/* ANSI_C */



char *
newvprintf(fmt, vp)
	char *		fmt;
	va_list		vp;
{
	register int	i;
#	if	VPRINTF == 0
	char *		a[12];
#	endif	/* VPRINTF == 0 */
	char		buf[1024];

	if ( fmt == NULLSTR )
		i = 0;
	else
	{
#		if	VPRINTF == 0

		for ( i = 0 ; i < 12 ; i++ )
			a[i] = va_arg(vp, char *);

		(void)sprintf(buf, fmt,
				a[0], a[1], a[2], a[3], a[4], a[5],
				a[6], a[7], a[8], a[9], a[10], a[11]
				);

#		else	/* VPRINTF == 0 */

		(void)vsprintf(buf, fmt, vp);

#		endif	/* VPRINTF == 0 */

		i = strlen(buf);	/* Only way to be sure (BSD returns pointer to "buf") */
	}

	return newnstr(buf, i);
}
