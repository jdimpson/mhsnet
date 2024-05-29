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
#include	"debug.h"

#define	ASIZE	040
#define	AMASK	(ASIZE-1)



/*
**	Find "match" in "string".
**	Return pointer to start of match, or NULLSTR.
**
**	`QuickSearch' algorithm.
**	[D.M.Sunday, CACM Aug '90, Vol.3 No.8 pp.132-142]
*/

char *
StringMatch(string, match)
	register char *	string;
	register char *	match;
{
	register int	i;
	register int	j;
	register int *	a;
	register char *	m;
	int		b[ASIZE];

	if ( string == NULLSTR || match == NULLSTR )
		return NULLSTR;

	Trace3(5, "StringMatch(%.22s, %.22s)", string, match);

	for ( m = match ; *m++ != '\0' ; );
	i = m - match;

	/** `i' is length of `match' +1 **/

	a = b;
	j = ASIZE;
	*a++ = i-1;	/* '\0' only skips length of `match' */
	while ( --j > 0 )
		*a++ = i;

	for ( a = b, m = match ; --i > 0 ; )
		a[(*m++)&AMASK] = i;

	j = m - match;

	/** `j' is length of `match' **/

	for ( ;; )
	{
		for ( i = 0 ; i < j ; i++ )
			if ( match[i] != string[i] )
				break;

		if ( i == j )
		{
			Trace3(4, "StringMatch(%.22s, %.22s) MATCH", string, match);
			return string;
		}

		/** No match - ensure skipped over string doesn't terminate **/

		for ( ; i < j ; i++ )
			if ( string[i] == '\0' )
				return NULLSTR;

		/** No match - bump `string' **/

		string += a[string[j]&AMASK];
	}
}
