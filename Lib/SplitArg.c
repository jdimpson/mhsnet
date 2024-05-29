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



/*
**	Split argument into separate "VarArg" entries on white space.
**	White space (and quotes) may be quoted (single or double) or escaped.
*/

int
SplitArg(to, arg)
	VarArgs *	to;
	register char *	arg;
{
	register int	c;
	register int	quote;
	register char *	cp;
	char *		temp;
	int		count;
	bool		hadquote;

	Trace3(2, "SplitArg(%#lx, %s)", (PUlong)to, arg);

	count = NARGS(to);

	if ( arg == NULLSTR )
		return count;

	for ( temp = Malloc(strlen(arg)+2), c = *arg++ ; c != '\0' ; )
	{
		cp = temp;
		quote = '\0';
		hadquote = false;

		while ( c == ' ' || c == '\t' || c == '\n' )
			c = *arg++;

		if ( c == '#' )
		{
			while ( (c = *arg++) && c != '\n' );
			continue;
		}

		for ( ; c != '\0' ; c = *arg++ )
		{
			if ( c == quote )
			{
				quote = '\0';
				continue;
			}

			switch ( c )
			{
			case ' ': case '\t': case '\n':
					if ( quote != '\0' )
						break;
					goto break2;

			case '\'': case '"':
					if ( quote != '\0' )
						break;
					hadquote = true;
					quote = c;
					continue;

			case '\\':	switch ( c = *arg++ )
					{
					case '\0':	arg--;
					default:	*cp++ = '\\';
					case '#':
					case '\\':	*cp++ = c;
							continue;

					case 'r':	*cp++ = '\r';
							continue;
					case 'n':	*cp++ = '\n';
							continue;
					case 't':	*cp++ = '\t';
							continue;
					case 's':	*cp++ = ' ';
							continue;
					case 'b':	*cp++ = '\b';
							continue;

					case '\'': case '"':
							if ( quote != '\0' && c != quote )
								*cp++ = '\\';
					case ' ': case '\t': case '\n':
							break;
					}
			}

			*cp++ = c;
		}

break2:
		if ( cp != temp || hadquote )
		{
			if ( ++count <= MAXVARARGS )
				NEXTARG(to) = newnstr(temp, cp-temp);

			DODEBUG(*cp = '\0'; Trace2(3, "SplitArg ==> %s", temp));
		}
	}

	free(temp);
	return count;
}
