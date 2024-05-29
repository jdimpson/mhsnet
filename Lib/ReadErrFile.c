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
#define	STAT_CALL
#define	STDIO
#define	ERRNO

#include	"global.h"
#include	"debug.h"



int		ErrSize;

#define	MAXBYTES	300	/* Maximum bytes read from an error file */

static char *	StrSysErr _FA_((char *));



/*
**	Read a file with error indications.
*/

char *
ReadErrFile(file, start)
	char *		file;
	bool		start;
{
	register int	fd;
	register int	size;
	struct stat	statb;

	ErrSize = 0;
	FreeStr(&ErrString);

	while ( (fd = open(file, O_READ)) == SYSERROR )
		if ( (ErrString = StrSysErr(OpenStr)) != NULLSTR )
			return NULLSTR;

	while ( fstat(fd, &statb) == SYSERROR )
		if ( (ErrString = StrSysErr(StatStr)) != NULLSTR )
			return NULLSTR;

	if ( (size = statb.st_size) > MAXBYTES )
	{
		size = MAXBYTES;

		if ( !start )
			(void)lseek(fd, (long)-size, 2);
	}
	
	if ( (ErrSize = size) > 0 )
	{
		file = Malloc(size);

		if ( (ErrSize = read(fd, file, size)) != size )
		{
			if ( (ErrString = StrSysErr(ReadStr)) == NULLSTR )
				ErrString = newstr(EmptyString);

			if ( ErrSize <= 0 )
			{
				free(file);
				ErrSize = 0;
			}
		}
		else
			ErrString = newstr(EmptyString);
	}
	else
		ErrString = newstr(english(" (zero length)"));

	(void)close(fd);

	return (ErrSize > 0) ? file : NULLSTR;
}



/*
**	System call error generator.
*/

static char *
StrSysErr(s)
	char *	s;
{
	register int	en;

	if ( (en = errno) == EINTR )
		return NULLSTR;

	(void)alarm(0);

	switch ( errno = en )
	{
	case ECHILD:
		return NULLSTR;

	case ENFILE:
	case ENOSPC:
	case EAGAIN:

#	ifdef	EALLOCFAIL
	case EALLOCFAIL:
#	endif	/* EALLOCFAIL */

#	ifdef	ENOBUFS
	case ENOBUFS:
#	endif	/* ENOBUFS */

		(void)sleep(60);
		return NULLSTR;
	}

	return newprintf(english(" (Could not %s file: %s.)\n"), s, ErrnoStr(en));
}
