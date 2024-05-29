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
#define	SELECT

#include	"global.h"
#include	"debug.h"



/*
**	Check that a pipe can be read without blocking.
*/

#if	PIPE_FSTAT_SIZE == 1

bool
CanReadPipe(fd, name)
	int		fd;
	char *		name;
{
	struct stat	statb;

	Trace3(3, "CanReadPipe(%d, %s)", fd, name);

	while ( fstat(fd, &statb) == SYSERROR )
		Syserror(CouldNot, StatStr, name);

	if ( statb.st_size == 0 )
		return false;

	Trace3(1, "CanReadPipe %s size=%lu.", name, (PUlong)statb.st_size);

	return true;

}

#else	/* PIPE_FSTAT_SIZE == 1 */

#if	BSD4 >= 2 || linux == 1

#include	<sys/time.h>

static struct timeval nowait = { 0, 0 };

bool
CanReadPipe(fd, name)
	int		fd;
	char *		name;
{
	fd_set		read_fds;

	Trace3(3, "CanReadPipe(%d, %s)", fd, name);

	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);

	if
	(
		select(fd+1, &read_fds, (fd_set *)0, (fd_set *)0, &nowait) <= 0
		||
		!FD_ISSET(fd, &read_fds)
	)
		return false;

	Trace2(1, "CanReadPipe %s", name);

	return true;
}

#endif	/* BSD4 >= 2 || linux == 1 */

#if	V8 == 1

bool
CanReadPipe(fd, name)
	int		fd;
	char *		name;
{
	fd_set		read_fds;

	Trace3(3, "CanReadPipe(%d, %s)", fd, name);

	(void)strclr((char *)&read_fds, sizeof(fd_set));
	FD_SET(fd, read_fds);

	if
	(
		select(fd+1, &read_fds, (fd_set *)0, 0) <= 0
		||
		!FD_ISSET(fd, read_fds)
	)
		return false;

	Trace2(1, "CanReadPipe %s", name);

	return true;
}

#endif	/* V8 == 1 */

#endif	/* PIPE_FSTAT_SIZE == 1 */
