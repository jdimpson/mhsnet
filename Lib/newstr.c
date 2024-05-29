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
**	Allocate new string and copy arg into it.
*/

#include	"global.h"

char *
#ifdef	ANSI_C
newstr(const char *p)
#else	/* ANSI_C */
newstr(p)
	char *			p;
#endif	/* ANSI_C */
{
#	if	__hpux
	register char *		s;
#	else	/* __hpux */
	register const char *	s;
#	endif	/* __hpux */

	if ( (s = p) == NULLSTR )
		s = EmptyString;

	return strcpy(Malloc(strlen(s)+1), s);
}
