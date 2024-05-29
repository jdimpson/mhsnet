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
**	Strip types from "string" to copy of string in "copy".
*/

char *
StripCopyEnd(copy, string)
	char *		copy;
	char *		string;
{
	register int	c;
	register char *	sp;
	register char *	cp;
	register char *	tp;

	Trace3(5, "StripCopyEnd(%#lx, %s)", (PUlong)copy, string);

	sp = string;

	if ( *sp == ATYP_DESTINATION )
		sp++;

	cp = tp = copy;

	do
		if ( (c = *sp++) == TYPE_SEP )
			cp = tp;
		else
		if ( (*cp++ = c) == DOMAIN_SEP )
			tp = cp;
	while
		( c != '\0' );

	Trace3(4, "StripCopyEnd(%s) ==> %s", (copy==string)?EmptyString:string, copy);

	return cp;
}
