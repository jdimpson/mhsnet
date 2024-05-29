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
**	Extract a number from a field encoded by EncodeNum.
**
**	NB: result is undefined if field not produced by EncodeNum.
*/

#include	"global.h"
#include	"spool.h"



Ulong
DecodeNum(name, length)
	register char *	name;
	register int	length;
{
	register Ulong	number = 0;

	if ( length > 0 )
	for ( ;; )
	{
		number |= strchr(ValidChars, *name++) - ValidChars;

		if ( --length <= 0 )
			break;

		number <<= VC_SHIFT;
	}

	return number;
}
