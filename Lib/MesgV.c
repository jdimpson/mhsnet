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
#define	TIME

#include	"global.h"
#include	"debug.h"


FILE *		ErrorFd	= NULL;	/* Initialised in GetParams.c:InitParams() */

extern char *	Name;

#define	Fprintf	(void)fprintf
#define	Fflush	(void)fflush



/*
**	General error output routine:
**		precede (non-tty) output with datestamp;
**		assumes first arg is error decription,
**			second arg is printf format string,
**			other args are printf parameters.
*/

void
MesgV(err, fmt, vp)
	char *		err;
	char *		fmt;
	va_list		vp;
{
#	if	VPRINTF != 1
	register int	i;
	char *		a[7];
#	endif	/* VPRINTF != 1 */

	Fflush(stdout);

	if ( !IsattyDone )
		(void)ErrorTty((int *)0);

	if ( !ErrIsatty )
	{
		char	buf[DATETIMESTRLEN];

		(void)fseek(ErrorFd, (long)0, 2);
		Fprintf(ErrorFd, "%s ", DateTimeStr((Time_t)time((time_t *)0), buf));
	}

	Fprintf(ErrorFd, "%s: ", Name);

	if ( err == NULLSTR )
		return;

	(void)fputs(err, ErrorFd);

	if ( fmt == NULLSTR )
		return;

	(void)fputs(" -- ", ErrorFd);

#	if	VPRINTF != 1

#	if	DOPRNT_BUG == 1
	if ( strchr(fmt, '%') == NULLSTR )
	{
		(void)fputs(fmt, ErrorFd);	/* To bypass an old '_doprnt' bug */
		return;
	}
#	endif	/* DOPRNT_BUG == 1 */

	for ( i = 0 ; i < 7 ; i++ )
		a[i] = va_arg(vp, char *);

	Fprintf(ErrorFd, fmt, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);

#	else	/* VPRINTF != 1 */

	(void)vfprintf(ErrorFd, fmt, vp);

#	endif	/* VPRINTF != 1 */
}
