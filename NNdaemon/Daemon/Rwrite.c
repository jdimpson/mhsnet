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
**	Buffer output to save system calls, and/or align writes.
*/

#define	ERRNO

#include	"global.h"
#include	"debug.h"

#include	"daemon.h"
#include	"Pconfig.h"


static char	buffer[PBUFSIZE];
int		Wbufcount;	/* Global so driver can clear it */

static int	_write _FA_((int, const char *, int, bool));



ssize_t
#if	ANSI_C
Rwrite(int fd, const void *abuf, size_t size)
{
	register const char *buf = (const char *)abuf;
#else	/* ANSI_C */
Rwrite(fd, buf, size)
	int		fd;
	register char *	buf;
	size_t		size;
{
#endif	/* ANSI_C */
	register int	osize;		/* Writes aligned on this quantity */
	register int	count;
	register int	n;

	if ( (count = size) == 0 )
		return count;

	if ( (osize = BufferOutput) == 0 )
		osize = size;

	while ( !Cook && Wbufcount == 0 && count >= osize )
	{
		if ( (n = _write(fd, buf, osize, false)) <= 0 )
			return n;

		count -= n;
		buf += n;
	}

	while ( count )
	{
		if ( (n = osize - Wbufcount) > count )
			n = count;

		if ( n > 0 )
		{
			bcopy(buf, &buffer[Wbufcount], n);

			count -= n;
			buf += n;

			if ( (Wbufcount += n) < osize )
				continue;
		}

		if ( (n = RWflush(fd)) <= 0 )
			return n;
	}

	return size;
}



int
RWflush(fd)
	int		fd;
{
	register int	n;

	if ( Cook && buffer[Wbufcount-1] != '\r' )
		buffer[Wbufcount++] = '\r';

	if ( (n = _write(fd, buffer, Wbufcount, true)) > 0 )
		Wbufcount = 0;

	return n;
}



static int
_write(fd, buf, size, fix)
	int		fd;
#	if	ANSI_C
	const char *	buf;
#	else	/* ANSI_C */
	char *		buf;
#	endif	/* ANSI_C */
	int		size;
	bool		fix;
{
	register int	count;
	register int	n;

	if ( (count = size) <= 0 )
		return count;

	while ( (n = write(fd, buf, count)) != count )
	{
		if ( n <= 0 )
		{
			if ( n == SYSERROR && errno == EINTR )
				continue;

#			ifdef	APOLLO
			if ( n == SYSERROR && errno == EIO )
				continue;
#			endif	/* APOLLO */

			if ( fix && buf != buffer )
			{
				bcopy(buf, buffer, count);
				Wbufcount = count;
			}

			return n;
		}

		count -= n;
		buf += n;
	}

	return size;
}
