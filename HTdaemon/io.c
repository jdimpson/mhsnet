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
**	Safe system calls.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"driver.h"

#include	<setjmp.h>


jmp_buf		IOerrorJmp;



/*
**	close(2) - check no error.
*/

void
Close(fd, sync, file)
	int	fd;
	bool	sync;
	char *	file;
{
#	if	FSYNC_2
	if ( sync )
		while ( fsync(fd) == SYSERROR )
			Syserror(CouldNot, "fsync", file);
#	endif	/* FSYNC_2 */

	while ( close(fd) == SYSERROR )
		Syserror(CouldNot, "close", file);

	Trace3(4, "Close file \"%s\" fd %d", file, fd);
}



/*
**	creat(2) - new file mode 0600, and non-existent directories if necessary.
*/

int
Create(file)
	char *		file;
{
	register int	fd;

#	ifdef	O_CREAT
	while ( (fd = open(file, O_CREAT|O_EXCL|O_WRITE, 0600)) == SYSERROR )
#	else	/* O_CREAT */
	while ( (fd = creat(file, 0600)) == SYSERROR )
#	endif	/* O_CREAT */
		if ( !CheckDirs(file) )
			Syserror(CouldNot, CreateStr, file);

	Trace3(4, "Create file \"%s\" ==> fd %d", file, fd);

	return fd;
}



/*
**	open(2).
*/

int
Open(file)
	char *		file;
{
	register int	fd;

	while ( (fd = open(file, O_READ)) == SYSERROR )
		Syserror(CouldNot, OpenStr, file);

	Trace3(4, "Open file \"%s\" ==> fd %d", file, fd);

	return fd;
}



/*
**	open(2) with recovery.
*/

int
OpenJ(chan, file)
	int		chan;
	char *		file;
{
	register int	fd;

	while ( (fd = open(file, O_READ)) == SYSERROR )
		if ( !SysWarn(CouldNot, OpenStr, file) || Finish )
			longjmp(IOerrorJmp, chan+1);

	Trace4(4, "OpenJ chan %d file \"%s\" ==> fd %d", chan, file, fd);

	return fd;
}



/*
**	read(2).
*/

int
Read(fd, addr, size, file)
	int		fd;
	char *		addr;
	int		size;
	char *		file;
{
	register int	n;

	Trace4(4, "Read fd %d size %d file \"%s\"", fd, size, file);

	DODEBUG(if(size==0)Fatal("Read size 0"));

	while ( (n = read(fd, addr, size)) == SYSERROR )
		Syserror(CouldNot, ReadStr, file);

	return n;
}



/*
**	read(2) with recovery.
*/

int
ReadJ(chan, fd, addr, size, file)
	int		chan;
	int		fd;
	char *		addr;
	int		size;
	char *		file;
{
	register int	n;

	Trace5(4, "ReadJ chan %d fd %d size %d file \"%s\"", chan, fd, size, file);

	DODEBUG(if(size==0)Fatal("Read size 0"));

	while ( (n = read(fd, addr, size)) == SYSERROR )
		if ( !SysWarn(CouldNot, ReadStr, file) || Finish )
			longjmp(IOerrorJmp, chan+1);

	return n;
}



/*
**	lseek(2).
*/

Ulong
Seek(fd, addr, mode, file)
	int		fd;
	Ulong		addr;
	int		mode;
	char *		file;
{
	register long	posn;

	Trace5(4, "Seek fd %d addr %lu mode %d file \"%s\"", fd, (PUlong)addr, mode, file);

	while ( (posn = lseek(fd, (long)addr, mode)) == SYSERROR )
		Syserror(CouldNot, SeekStr, file);

	return (Ulong)posn;
}



/*
**	lseek(2) with error recovery.
*/

Ulong
SeekJ(chan, fd, addr, mode, file)
	int		chan;
	int		fd;
	Ulong		addr;
	int		mode;
	char *		file;
{
	register long	posn;

	Trace6(4, "SeekJ chan %d fd %d addr %lu mode %d file \"%s\"", chan, fd, (PUlong)addr, mode, file);

	while ( (posn = lseek(fd, (long)addr, mode)) == SYSERROR )
		if ( !SysWarn(CouldNot, SeekStr, file) || Finish )
			longjmp(IOerrorJmp, chan+1);

	return (Ulong)posn;
}



/*
**	unlink(2) - check no error.
*/

void
Unlink(file)
	char *	file;
{
	while ( unlink(file) == SYSERROR )
		if ( !SysWarn(CouldNot, UnlinkStr, file) || Finish )
			break;

	Trace2(4, "Unlink file \"%s\"", file);
}



/*
**	write(2), taking care of partial writes.
*/

int
Write(fd, addr, size, file)
	int		fd;
	char *		addr;
	int		size;
	char *		file;
{
	register int	n;
	register int	s = size;

	Trace4(4, "Write fd %d size %d file \"%s\"", fd, s, file);

	DODEBUG(if(s==0)Fatal("Write size 0"));

	while ( (n = write(fd, addr, s)) != s )
	{
		if ( n <= 0 )
		{
			if ( n == 0 || errno == EINTR )
				continue;

			Syserror(CouldNot, WriteStr, file);
			continue;
		}

		addr += n;
		s -= n;
	}

	return size;
}
