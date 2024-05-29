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
**	Expand a string to printable characters.
*/

#include	"global.h"
#include	"debug.h"


static char *	Buf;
static int	BufLen;
#define	MAXLEN	511



char *
#if	ANSI_C
ExpandString(const char *string, int size)
#else	/* ANSI_C */
ExpandString(string, size)
	char *		string;
	int		size;
#endif	/* ANSI_C */
{
	register int	c;
#	if	ANSI_C
	register const char *	cp;
	register const char *	ep;
#	else	/* ANSI_C */
	register char *	cp;
	register char *	ep;
#	endif	/* ANSI_C */
	register char *	bp;
	register char *	bep;

	if ( (cp = string) == NULLSTR )
		return EmptyString;

	if ( (c = size) < 0 )
		c = strlen(cp);

	if ( c == 0 )
		return EmptyString;

	size = c;
	c *= 4;
	c += 1;

	if ( c > MAXLEN )
		c = MAXLEN;

	if ( BufLen < c )
	{
		c |= 63;
		c += 1;

		BufLen = c;

		if ( Buf != NULLSTR )
			free(Buf);

		Buf = Malloc(BufLen);
	}

	bp = Buf;
	bep = &bp[BufLen-5];

	for ( ep = &cp[size] ; cp < ep ; )
	{
		if ( ((c = *cp++) < ' ' /*&& c != '\n'*/) || c >= '\177' || c == '\\' )
		{
			(void)sprintf(bp, "%c%03o", '\\', c&0xff);
			bp += 4;
		}
		else
			*bp++ = c;

		if ( bp >= bep )
			break;
	}

	*bp = '\0';

	if ( cp < ep )
		Warn("ExpandString size %d truncated to %d", size, cp-string);

	return Buf;
}
