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


#include	"global.h"
#include	"debug.h"
#include	"exec.h"


#if	BSD4 >= 2 || SYSTEM == 5 && SYSVREL >= 3
#define	RMDIR_2	1
#else	/* BSD4 >= 2 || SYSTEM == 5 && SYSVREL >= 3 */
#define	RMDIR_2	0
#endif	/* BSD4 >= 2 || SYSTEM == 5 && SYSVREL >= 3 */



/*
**	Remove an empty directory.
*/

bool
RmDir(name)
	char *		name;
{
	register char *	cp;
#	if	RMDIR_2 == 0
	register char *	errs;
	register char *	kef;
	ExBuf		eb;
#	endif	/* RMDIR_2 == 0 */
	bool		retval = true;

	Trace2(1, "RmDir(%s)", name);

	cp = &name[strlen(name)-1];

	if ( *cp == '/' )
		*cp = '\0';
	else
		cp = NULLSTR;

#	if	RMDIR_2 == 1

	while ( rmdir(name) == SYSERROR )
		if ( !SysWarn(CouldNot, "rmdir", name) )
		{
			retval = false;
			break;
		}

#	else	/* RMDIR_2 == 1 */

	FIRSTARG(&eb.ex_cmd) = RMDIR;
	NEXTARG(&eb.ex_cmd) = name;

	kef = KeepErrFile;
	KeepErrFile = NULLSTR;

	if ( (errs = Execute(&eb, NULLVFUNCP, ex_exec, SYSERROR)) != NULLSTR )
	{
		Warn(StringFmt, errs);
		free(errs);
		retval = false;
	}

	KeepErrFile = kef;

#	endif	/* RMDIR_2 == 1 */

	if ( cp != NULLSTR )
		*cp = '/';

	return retval;
}
