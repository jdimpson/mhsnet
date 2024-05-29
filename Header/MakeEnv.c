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

#include	"global.h"
#include	"header.h"



/*
**	Concatenate strings into header environment.
**	Variable number of argument pairs terminated by a NULLSTR.
*/

/*VARARGS*/
char *
#ifdef	ANSI_C
MakeEnv(char *s1, ...)
{
#else	/* ANSI_C */
MakeEnv(va_alist)
	va_dcl
{
#endif	/* ANSI_C */
	register char *	np;
	register char *	cp;
	register int	size	= 0;
	va_list		vp;
	char *		string;

#	ifdef	ANSI_C
	va_start(vp, s1);
	np = s1;
	while ( np != NULLSTR )
	{
		size += strlen(np) + 1;
		np = va_arg(vp, char *);
	}
#	else	/* ANSI_C */
	va_start(vp);
	while ( (np = va_arg(vp, char *)) != NULLSTR )
		size += strlen(np) + 1;
#	endif	/* ANSI_C */
	va_end(vp);
	
	string = cp = Malloc(size+1);

#	ifdef	ANSI_C
	va_start(vp, s1);
	np = s1;
	while ( np != NULLSTR )
	{
		register char *	ocp;

		*cp++ = ENV_RS;
		cp = strcpyend(cp, np);

		if ( (np = va_arg(vp, char *)) != NULLSTR )
		{
			*cp++ = ENV_US;
			ocp = cp;
			cp = strcpyend(cp, np);
			QuoteChars(ocp, EnvRestricted);
		}

		np = va_arg(vp, char *);
	}
#	else	/* ANSI_C */
	va_start(vp);
	while ( (np = va_arg(vp, char *)) != NULLSTR )
	{
		register char *	ocp;

		*cp++ = ENV_RS;
		cp = strcpyend(cp, np);

		if ( (np = va_arg(vp, char *)) != NULLSTR )
		{
			*cp++ = ENV_US;
			ocp = cp;
			cp = strcpyend(cp, np);
			QuoteChars(ocp, EnvRestricted);
		}
	}
#	endif	/* ANSI_C */
	va_end(vp);

	return string;
}
