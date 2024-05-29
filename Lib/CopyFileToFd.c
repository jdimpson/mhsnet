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


#define	FILE_CONTROL
#define	ERRNO

#include	"global.h"
#include	"debug.h"



/*
**	Copy contents of file onto passed file descriptor.
*/

bool
CopyFileToFd(ifile, ofd, ofile)
	char *	ifile;
	int	ofd;
	char *	ofile;
{
	int	ifd;
	bool	val;

	Trace4(1, "CopyFileToFd(%s, %d, %s)", ifile, ofd, ofile);

	while ( (ifd = open(ifile, O_READ)) == SYSERROR )
		if ( !SysWarn(CouldNot, OpenStr, ifile) )
			return false;

	val = CopyFdToFd(ifd, ifile, ofd, ofile, MAX_ULONG);

	(void)close(ifd);

	return val;
}


/*
**	Copy ``length'' bytes from ``ifd'' to ``ofd''.
*/

bool
CopyFdToFd(ifd, ifile, ofd, ofile, length)
	register int	ifd;
	char *		ifile;
	register int	ofd;
	char *		ofile;
	Ulong		length;
{
	register int	r;
	register int	w;
	register char *	cp;
	long		posn;
	bool		pipe = false;
	char		buf[FILEBUFFERSIZE];

	Trace4(1, "CopyFdToFd(%d, %d, %lu)", ifd, ofd, (PUlong)length);

	while ( (posn = lseek(ofd, (long)0, 1)) == SYSERROR )
	{
		if ( errno == ESPIPE )
		{
			pipe = true;
			posn = 0;
			break;
		}

		if ( !SysWarn(CouldNot, SeekStr, ofile) )
			return false;
	}

	while ( length != 0 )
	{
		if ( (r = FILEBUFFERSIZE) > length )
			r = length;

		while ( (r = read(ifd, buf, r)) == SYSERROR )
			if ( !SysWarn(CouldNot, ReadStr, ifile) )
				return false;

		if ( r == 0 )
			break;

		length -= r;

		cp = buf;

		while ( (w = write(ofd, cp, r)) != r )
		{
			if ( w == SYSERROR )
			{
				if ( !SysWarn(CouldNot, WriteStr, ofile) )
					return false;
				if ( !pipe )
					(void)lseek(ofd, posn, 0);
			}
			else
			{
				cp += w;
				r -= w;
				posn += w;
			}
		}

		posn += r;
	}

	return true;
}
