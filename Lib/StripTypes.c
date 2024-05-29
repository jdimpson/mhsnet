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
#include	"address.h"
#include	"debug.h"



/*
**	Strip types (and leading ATYP_DESTINATION) from new copy of string.
*/

char *
StripTypes(s)
	char *		s;
{
	register char	c;
	register char *	sp;
	register char *	np;
	register char *	tp;
	char *		val;

	Trace2(3, "StripTypes(%s)", s);

	if ( (sp = s) == NULLSTR )
		return sp;

	if ( *sp == ATYP_DESTINATION )
		sp++;

	val = tp = np = newstr(sp);

	do
		if ( (c = *sp++) == TYPE_SEP )
			np = tp;
		else
		if ( (*np++ = c) == DOMAIN_SEP )
			tp = np;
	while
		( c != '\0' );

	Trace3(4, "StripTypes(%s) ==> %s", s, val);

	return val;
}
