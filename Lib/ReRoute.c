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
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"spool.h"

#include	<ndir.h>



/*
**	Search a directory for command files and move them.
*/

int
ReRoute(dir1, dir2)
	char *			dir1;
	char *			dir2;
{
	register struct direct *direp;
	register char *		cp1;
	register char *		cp2;
	register char *		fp1;
	register DIR *		dirp;
	char *			lock1;
	int			count;
	char			template[UNIQUE_NAME_LENGTH+1];
	struct stat		statb;

	Trace3(1, "ReRoute(%s, %s)", dir1, dir2);

	dir1 = concat(SPOOLDIR, dir1, NULLSTR);

	while ( (dirp = opendir(dir1)) == NULL )
		if
		(
			errno == ENOENT
			||
			!SysWarn(CouldNot, ReadStr, dir1)
		)
		{
			free(dir1);
			return 0;
		}

	lock1 = concat(dir1, REROUTELOCKFILE, NULLSTR);

	(void)sprintf(template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);

	cp1 = concat(dir1, template, NULLSTR);
	cp2 = concat(SPOOLDIR, dir2, template, NULLSTR);

	fp1 = cp1 + strlen(dir1);

	if ( SetLock(lock1, Pid, false) )
	{
		count = 0;

		while ( (direp = readdir(dirp)) != NULL )
		{
			/*
			**	Find valid command file.
			*/

			if ( direp->d_name[0] < STATE_QUALITY || direp->d_name[0] > LOWEST_QUALITY )
				continue;

			if ( strlen(direp->d_name) != UNIQUE_NAME_LENGTH )
				continue;

			Trace2(2, "found \"%s\"", direp->d_name);

			(void)strncpy(fp1, direp->d_name, UNIQUE_NAME_LENGTH);

			while ( stat(cp1, &statb) == SYSERROR )
				Syserror(CouldNot, StatStr, cp1);

			if ( statb.st_mtime < Time )
			{
#				if	PRIORITY_ROUTING == 1
				move(cp1, UniqueName(cp2, fp1[0], NOOPTNAME, (Ulong)1, (Time_t)statb.st_mtime));
#				else	/* PRIORITY_ROUTING == 1 */
				move(cp1, UniqueName(cp2, STATE_QUALITY, NOOPTNAME, (Ulong)0, (Time_t)statb.st_mtime));
#				endif	/* PRIORITY_ROUTING == 1 */
				SMdate(cp2, (Time_t)statb.st_mtime);
				count++;
			}
		}

		(void)unlink(lock1);
	}

	closedir(dirp);

	free(cp2);
	free(cp1);

	free(lock1);
	free(dir1);

	return count;
}
