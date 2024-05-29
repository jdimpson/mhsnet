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
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"Passwd.h"



/*
**	Make a lock file.
*/

bool
MakeLock(file)
	char *	file;
{
	int	fd;
	int	count;

	Trace2(1, "MakeLock(%s)", file);

	GetNetUid();

#	if	AUTO_LOCKING == 1

	for ( count = 3 ; mknod(file, S_IFALK|0644, 0) == SYSERROR ; --count )
	{
		if ( errno == EEXIST )
			return true;
		if ( !CheckDirs(file) )
			if ( count <= 0 || !SysWarn(CouldNot, CreateStr, file) )
				return false;
	}

#	else	/* AUTO_LOCKING == 1 */

	for ( count = 3 ; (fd = creat(file, 0644)) == SYSERROR ; --count )
	{
		if ( errno == EEXIST )
			return true;
		if ( !CheckDirs(file) )
			if ( count <= 0 || !SysWarn(CouldNot, CreateStr, file) )
				return false;
	}

	(void)close(fd);

#	endif	/* AUTO_LOCKING == 1 */

	(void)chown(file, NetUid, NetGid);	/* Race condition here, if someone else finds it first */
	(void)chmod(file, 0644);		/* In case stuffed by umask above */

	return true;
}
