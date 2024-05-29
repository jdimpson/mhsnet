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

#include	"global.h"
#include	"debug.h"


extern unsigned	RdFileSize;
extern Time_t	RdFileTime;



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
ReadFile(file)
	register char *	file;
{
	register int	fd;

	Trace2(2, "ReadFile(%s)", file);

	RdFileSize = 0;
	errno = 0;

	if ( file == NULLSTR || (fd = open(file, O_READ)) == SYSERROR )
	{
		RdFileTime = 0;
		return NULLSTR;
	}

	if ( (file = ReadFd(fd)) == NULLSTR )
	{
		int	saverrno = errno;

		(void)close(fd);
		errno = saverrno;
	}
	else
		(void)close(fd);

	return file;
}
