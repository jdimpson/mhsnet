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
**	Convert HEXADECIMAL format string to long integer.
*/

long
xtol(s)
	register char *	s;
{
	register long	n;
	register int	c;
	register int	yet;

	for ( n = 0, yet = 0 ; (c = *s++) != 0 ; )
	{
		switch ( c )
		{
		case ' ': case '\t':	/* Ignore leading white space */
			if ( !yet )
				continue;
			return n;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			c -= '0';
			break;
		default:
			switch ( c |= 040 )
			{
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
				c -= 'a';
				c += 10;
				break;
			default:
				return n;
			}
		}
		n = n * 16 + c;
		yet = 1;
	}

	return n;
}
