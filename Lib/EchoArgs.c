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
**	Echo the arguments.
*/

void
EchoArgs(argc, argv)
	register int	argc;
	register char *	argv[];
{
	register char *	cp;
	va_list		vp;

	MesgV(NULLSTR, NULLSTR, vp);	/* To get date-stamp and `Name' */

	while ( --argc >= 0 )
		if ( (cp = *argv++) != NULLSTR )
			(void)fprintf(ErrorFd, " \"%s\"", ExpandString(cp, -1));
		else
			(void)fprintf(ErrorFd, " <null>");

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);
}
