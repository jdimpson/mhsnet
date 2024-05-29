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
#include	"exec.h"
#include	"Passwd.h"



#ifndef	MKDIR_2
#if	BSD4 >= 2 || SYSTEM == 5 && SYSVREL >= 3
#define	MKDIR_2	1
#else	/* BSD4 >= 2 || SYSTEM == 5 && SYSVREL >= 3 */
#define	MKDIR_2	0
#endif	/* BSD4 >= 2 || SYSTEM == 5 && SYSVREL >= 3 */
#endif	/* MKDIR_2 */

static int	MDUid;
static int	MDGid;
void		SetDirsUid _FA_((VarArgs *));



/*
**	Create all directories in `name' that don't already exist.
*/

bool
MkDirs(name, uid, gid)
	char *		name;
	int		uid;
	int		gid;
{
	register char *	cp = name;
	register int	i;
#	if	MKDIR_2 == 0
	register char *	errs;
	ExBuf		eb;
#	endif	/* MKDIR_2 == 0 */
	struct stat	statb;

	Trace4(2, "MkDirs(%s, %d, %d)", name, uid, gid);

	if ( (i = SpoolDirLen) == 0 )
		SpoolDirLen = i = strlen(SPOOLDIR);

	if ( strncmp(cp, SPOOLDIR, i) == STREQUAL )
		cp += i;
	else
		cp += 1;

#	if	MKDIR_2 == 0
	FIRSTARG(&eb.ex_cmd) = MKDIR;
#	else	/* MKDIR_2 == 0 */
	i = 0;
#	endif	/* MKDIR_2 == 0 */

	while ( (cp = strchr(cp, '/')) != NULLSTR )
	{
		*cp = '\0';

		if ( stat(name, &statb) == SYSERROR && errno == ENOENT )
		{
			Trace2(2, "MkDirs ==> mkdir %s", name);
#			if	MKDIR_2 == 1
			while ( mkdir(name, 0775) == SYSERROR )
			{
				if ( SysWarn(CouldNot, "mkdir", name) )
					continue;
				*cp++ = '/';
				return false;
			}
			(void)chown(name, uid, gid);
			i++;
#			else	/* MKDIR_2 == 1 */
			NEXTARG(&eb.ex_cmd) = newstr(name);
#			endif	/* MKDIR_2 == 1 */
		}

		*cp++ = '/';
	}

#	if	MKDIR_2 == 0

	Trace3(2, "MkDirs ==> %s %d dirs", MKDIR, NARGS(&eb.ex_cmd)-1);

	if ( NARGS(&eb.ex_cmd) == 1 )
		return false;

	cp = KeepErrFile;
	KeepErrFile = NULLSTR;

	MDUid = uid;
	MDGid = gid;

	if ( (errs = Execute(&eb, SetDirsUid, ex_exec, SYSERROR)) != NULLSTR )
	{
		Warn(StringFmt, errs);
		free(errs);
	}

	KeepErrFile = cp;

	for ( i = NARGS(&eb.ex_cmd) ; --i > 0 ; )
	{
#		if	SYSTEM == 5 && SYSVREL <= 2

		name = ARG(&eb.ex_cmd, i);
		(void)chown(name, uid, gid);	/* Fixes SYSV.2 setuid bug */
		free(name);

#		else	/* SYSTEM == 5 && SYSVREL <= 2 */

		free(ARG(&eb.ex_cmd, i));

#		endif	/* SYSTEM == 5 && SYSVREL <= 2 */
	}

	if ( errs != NULLSTR )
		return false;

#	else	/* MKDIR_2 == 0 */

	if ( i == 0 )
		return false;

#	endif	/* MKDIR_2 == 0 */

	return true;
}



/*
**	Set network uid, so that directories have correct owner.
*/

void
SetDirsUid(args)
	VarArgs *	args;
{
	int	om;

	if ( ((om = umask(02)) & 02) != 0 )
		umask(om);

	while ( setgid(MDGid) == SYSERROR && geteuid() == 0 )
		if ( !SysWarn(CouldNot, "setgid", "group") )
			break;

	while ( setuid(MDUid) == SYSERROR && geteuid() == 0 )
		Syserror(CouldNot, "setuid", "user");
}
