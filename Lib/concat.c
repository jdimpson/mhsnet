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
**	Concatenate strings into allocated memory.
**	Variable number of args terminated by a NULLSTR.
*/

#define	STDARGS

#include	"global.h"



/*VARARGS*/
#ifdef	ANSI_C

char *
concat(char *s1, ...)
{
	va_list		vp;
	register char *	ap;
	register char *	cp;
	register int	size	= 0;
	char *		string;

	if ( s1 == NULLSTR )
		return newstr(EmptyString);

	size += strlen(s1);
	va_start(vp, s1);
	while ( (ap = va_arg(vp, char *)) != NULLSTR )
		size += strlen(ap);
	va_end(vp);

	string = cp = Malloc(size+1);

	cp = strcpyend(cp, s1);
	va_start(vp, s1);
	while ( (ap = va_arg(vp, char *)) != NULLSTR )
		cp = strcpyend(cp, ap);
	va_end(vp);

	return string;
}

#else	/* ANSI_C */

char *
concat(va_alist)
	va_dcl
{
	va_list		vp;
	register char *	ap;
	register char *	cp;
	register int	size	= 0;
	char *		string;

	va_start(vp);
	while ( (ap = va_arg(vp, char *)) != NULLSTR )
		size += strlen(ap);
	va_end(vp);

	string = cp = Malloc(size+1);
	cp[0] = '\0';	/* In case "concat(NULLSTR)" */

	va_start(vp);
	while ( (ap = va_arg(vp, char *)) != NULLSTR )
		cp = strcpyend(cp, ap);
	va_end(vp);

	return string;
}

#endif	/* ANSI_C */
