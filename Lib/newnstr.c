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
**	Allocate new string with given length and copy arg into it.
*/

#include	"global.h"



char *
#ifdef	ANSI_C
newnstr(const char *s, int n)
#else	/* ANSI_C */
newnstr(s, n)
	char *		s;
	int		n;
#endif	/* ANSI_C */
{
	register char *	cp;

	if ( s == NULLSTR )
	{
		s = EmptyString;
		n = 0;
	}

	cp = strncpy(Malloc(n+1), s, n);

	cp[n] = '\0';

	return cp;
}
