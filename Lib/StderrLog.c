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

#include	"global.h"
#include	"debug.h"
#include	"Passwd.h"



/*
**	Set stderr to passed file.
*/

void
StderrLog(file)
	char *		file;
{
	register int	fd;

	Trace2(1, "StderrLog(%s)", file);

	/*
	**	Ensure we have fds 0, 1, and 2.
	*/

	do
	{
		if ( (fd = dup(fileno(stderr))) == SYSERROR )
			while ( (fd = open(DevNull, O_RDWR)) == SYSERROR )
				Syserror(CouldNot, OpenStr, DevNull);
	}
	while
		( fd < 2 );

	if ( fd > 2 )
		(void)close(fd);

	/*
	**	Open (create) log file as "stderr".
	*/

	while ( freopen(file, "a", stderr) == NULL )
	{
		(void)unlink(file);
		(void)close(create(file));
	}

	GetNetUid();
	(void)chown(file, NetUid, NetGid);
	(void)chmod(file, 0640);

#	if	FCNTL == 1 && O_APPEND != 0
	/*
	**	Real "append" mode.
	*/

	(void)fcntl
	(
		fileno(stderr),
		F_SETFL,
		fcntl(fileno(stderr), F_GETFL, 0) | O_APPEND
	);
#	endif

	ResetErrTty();
}
