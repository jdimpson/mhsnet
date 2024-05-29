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
**	Write remote line with "cooked" bytes.
*/

#define	ERRNO

#include	"global.h"
#include	"debug.h"

#include	"Channel.h"

#include	"daemon.h"


char	magic_chars[] = "@#$~";
char	rep_char = '`';


ssize_t
#if	ANSI_C
RCwrite(int fd, const void *m, size_t n)
#else	/* ANSI_C */
RCwrite(fd, m, n)
	int			fd;
	char *			m;
	int			n;
#endif	/* ANSI_C */
{
	register int		r;
#	if	ANSI_C
	register const char *	dp;
#	else	/* ANSI_C */
	register char *		dp;
#	endif	/* ANSI_C */
	register char *		cp;
	register int		llen;
	register unsigned	c;
	register ssize_t	(*funcp)_FA_((int, const void *, size_t));
	char			cbuf[sizeof(Packet)*2 + (sizeof(Packet)*2)/60 + 2];

#	if	ANSI_C
	Trace2(2, "RCwrite \"%s\"", ExpandString((const char *)m, n));
#	else	/* ANSI_C */
	Trace2(2, "RCwrite \"%s\"", ExpandString(m, n));
#	endif	/* ANSI_C */

	llen = 0;

	if ( CookVC != (CookT *)0 )
		cp = cbuf + (*CookVC->cook)(m, n, cbuf);
	else
#	if	ANSI_C
	for ( r = n, cp = cbuf, dp = (const char *)m ; r-- > 0 ; )
#	else	/* ANSI_C */
	for ( r = n, cp = cbuf, dp = m ; r-- > 0 ; )
#	endif	/* ANSI_C */
	{
		if ( llen >= 60 && BufferOutput == 0 )
		{
			*cp++ = '\r';
			llen = 0;
		}

		c = (*dp++) & 0xFF;

		if
		(
			r >= 3
			&&
			(dp[0] & 0xFF) == c
			&& 
			(dp[1] & 0xFF) == c
		)
		{
			register int	repcnt = 1;

			while ( (*dp & 0xFF) == c )
			{
				repcnt++;
				dp++;
				if ( --r <= 0 || repcnt == 077 )
					break;
			}

			if ( (repcnt & 0x30) != 0x30 )
				repcnt |= 0x40;

			*cp++ = rep_char;
			*cp++ = repcnt;
		}

		if
		(
			c >= ' '
			&&
			c <= '~'
			&&
			c != rep_char
			&&
			!strchr(magic_chars, c)
		)
		{
			*cp++ = c;
			llen++;
			continue;
		}

		*cp++ = magic_chars[(c>>6)&3];
		c &= 077;

		if ( (c & 0x30) != 0x30 )
			c |= 0x40;

		*cp++ = c;
		llen += 2;
		continue;
	}

	if ( llen > 0 && BufferOutput == 0 )
		*cp++ = '\r';

	dp = cbuf;
	c = cp - cbuf;

	Trace2(3, "RCwrote \"%s\"", ExpandString(dp, c));

	if ( BufferOutput )
		funcp = Rwrite;
	else
		funcp = write;

	while ( (r = (*funcp)(fd, dp, c)) != c )
	{
		if ( r <= 0 )
		{
#			ifdef	APOLLO
			if ( r == SYSERROR && errno == EIO )
				continue;
#			endif	/* APOLLO */
			return r;
		}
		
		dp += r;
		c -= r;
	}

	return n;
}


#if	PROTO_STATS >= 1

/*
**	Trace data.
*/

void
_TraceData(desc, size, data)
	char *		desc;
	int		size;
	char *		data;
{
	register char *	cp;
	register int	ld;
	register int	n;

	ld = 12 - strlen(desc);

	if ( (n = size) > 24 )
	{
		n = 24;
		cp = "...";
	}
	else
		cp = EmptyString;

	Trace(4, "%s %*d [%s]%s", desc, ld, size, ExpandString(data, n), cp);
}
#endif	/* PROTO_STATS >= 1 */
