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
**	Copy s2 into s1 (up to `n')
**	and return address of null (or `n'th char) in s1.
*/

char *
strncpyend(s1, s2, n)
	char *		s1;
	char *		s2;
	register int	n;
{
	register char *	cp1 = s1;
	register char *	cp2 = s2;

	do
		if ( --n < 0 )
			return cp1;
	while
		( (*cp1++ = *cp2++) != '\0' );

	return cp1 - 1;
}
