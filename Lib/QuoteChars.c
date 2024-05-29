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


#include	"global.h"



/*
**	Disable/enable any restricted characters within strings.
*/

void
QuoteChars(cp, restricted)
	register char *	cp;
	register char *	restricted;
{
	while ( (cp = strpbrk(cp, restricted)) != NULLSTR )
		*cp++ |= 0200;
}



void
UnQuoteChars(cp, restricted)
	register char *	cp;
	register char *	restricted;
{
	register int	c;

	while ( (c = *cp++) != 0 )
		if ( c & 0200 )
		{
			c &= 0177;

			if ( strchr(restricted, (char)c) != NULLSTR )
				cp[-1] = c;
		}
}
