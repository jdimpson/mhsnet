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

#include	<ctype.h>

/*
**	Convert string in `s' to lower-case in new string returned.
*/

char *
ToLower(s, len)
	register char *	s;
	int		len;
{
	register char *	cp;
	register int	c;
	char *		r;

	r = cp = Malloc(len+1);

	while ( (c = *s++) != '\0' )
		if ( isupper(c) )
			*cp++ = tolower(c);
		else
			*cp++ = c;

	*cp = c;
	return r;
}
