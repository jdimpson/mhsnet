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
#include	"spool.h"



/*
**	Encode a number into a string using ValidChars ==> VC_SHIFT bits/char.
*/

int
EncodeNum(name, number, length)
	char *		name;
	register Ulong	number;
	register int	length;
{
	register char *	p;

	if ( length == 0 )
		return length;

	if ( length < 0 )
	{
		register Ulong	mask = MAX_ULONG;

		for ( p = name ; number & mask ; p++, mask <<= VC_SHIFT );
		*p = '\0';
		if ( (length = p - name) == 0 )
			return length;
	}
	else
		p = name + length;

	for ( ;; )
	{
		*--p = ValidChars[number & VC_MASK];

		if ( p <= name )
			break;

		number >>= VC_SHIFT;
	}

	return length;
}
