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


/*
**	Change name of program by overwriting args list.
*/

#define	STDARGS

#include	"global.h"
#include	"debug.h"


static int	Argc;
static char **	Argv;
static char *	Argv0;
static int	ArgvLen;
static char *	LastEnd;



/*VARARGS*/
void
#ifdef	ANSI_C
NameProg(char *fmt, ...)
{
#else	/* ANSI_C */
NameProg(va_alist)
	va_dcl
{
	char *		fmt;
#endif	/* ANSI_C */
	va_list		vp;
	register char *	cp;
	register char *	ep;
	register char *	s;

	if ( Argv0 == NULLSTR )
	{
		Report1("No SetNameProg()");
		return;
	}

	while ( Argc >= 2 )
		Argv[--Argc] = NULLSTR;	/* Zero all pointers but first */

#	ifdef	ANSI_C
	va_start(vp, fmt);
#	else	/* ANSI_C */
	va_start(vp);
	fmt = va_arg(vp, char *);
#	endif	/* ANSI_C */
	s = newvprintf(fmt, vp);
	va_end(vp);

	ep = LastEnd;
	cp = strncpyend(Argv0, s, ArgvLen);
	LastEnd = cp;

	while ( cp < ep )
		*cp++ = ' ';

	Trace2(1, "NameProg(%s)", s);
	free(s);
}

void
SetNameProg(argc, argv, envp)
	int	argc;
	char *	argv[];
	char *	envp[];
{
	Argc = argc;
	Argv = argv;
	Argv0  = argv[0];
#	if	0
	*Argv0++ = '-';			/* Makes `ps' print " (prog)" */
#	endif	/* 0 */

	LastEnd = argv[argc-1];

#	if	0			/* NO => clobbers vital environment variables */
	while ( *envp != NULLSTR )
		LastEnd = *envp++;
#	endif	/* 0 */

	LastEnd += strlen(LastEnd);

	ArgvLen = LastEnd - Argv0;
}
