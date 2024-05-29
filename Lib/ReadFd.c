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


#define	STAT_CALL

#include	"global.h"
#include	"debug.h"


int		RdFileMode;
unsigned	RdFileSize;
Time_t		RdFileTime;



/*
**	Read in a file, and pass back a pointer to data.
**
**	Remove any trailing '\n's,
**	and guarantee at least 1 byte exists beyond '\0'.
**
**	Size of data is exported in "RdFileSize".
**	Modify time of data is exported in "RdFileTime".
*/

char *
ReadFd(fd)
	register int	fd;
{
	register char *	data;
	struct stat	statb;

	Trace2(2, "ReadFd(%d)", fd);

	statb.st_mtime = 0;
	RdFileMode = 0;
	RdFileSize = 0;
	errno = 0;

	if
	(
		fstat(fd, &statb) == SYSERROR
		||
		(RdFileSize = statb.st_size) == 0
	)
	{
		RdFileMode = statb.st_mode;
		RdFileTime = statb.st_mtime;
		return NULLSTR;
	}

	RdFileMode = statb.st_mode;
	RdFileTime = statb.st_mtime;

	data = Malloc((int)RdFileSize+2);	/* +2 so that callers may add 1 byte on end */

	if ( read(fd, data, (int)RdFileSize) != RdFileSize )
	{
		free(data);
		return NULLSTR;
	}

	if ( RdFileSize > 0 && data[RdFileSize-1] == '\n' )
		RdFileSize--;
	data[RdFileSize] = '\0';

	Trace3(3, "ReadFd(%d) %d bytes", fd, RdFileSize);

	return data;
}
