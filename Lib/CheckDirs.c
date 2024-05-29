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
#include	"exec.h"
#include	"Passwd.h"



int	SpoolDirLen;



/*
**	Create all directories in ``name'' if errno == ENOENT.
*/

bool
CheckDirs(name)
	register char *	name;
{
	int	en = errno;

	Trace3(2, "CheckDirs(%s) errno=%s", name, ErrnoStr(en));

	if ( (errno = en) != ENOENT )
		return false;

	if ( name[0] == '/' )
	{
		if ( SpoolDirLen == 0 )
			SpoolDirLen = strlen(SPOOLDIR);

		if ( strncmp(name, SPOOLDIR, SpoolDirLen) != STREQUAL )
			return false;
	}

	GetNetUid();

	return MkDirs(name, NetUid, NetGid);
}
