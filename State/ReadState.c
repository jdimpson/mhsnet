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
#include	"link.h"
#include	"route.h"
#include	"spool.h"
#include	"statefile.h"


static char *	LockFile;
static bool	LockSet;
static bool	LockState;
static char *	StateFile;
static char *	TmpFile;
static char	Template[UNIQUE_NAME_LENGTH+1];

extern Time_t	Time;
extern int	Pid;



/*
**	Check if statefile exists for named region.
*/

char *
FindState(statename)
	char *		statename;
{
	register int	fd;
	register char *	pathname;
	register char *	realname;
	register char *	prepath;
	struct stat	statb;
	DODEBUG(static char	tfmt[] = "FindState(%s) ==> %s");

	Trace2(1, "FindState(%s)", statename);

	if ( (pathname = DomainToPath(statename)) == NULLSTR )
		return NULLSTR;

	prepath = concat(SPOOLDIR, STATEDIR, MSGSDIR, NULLSTR);
	realname = concat(prepath, pathname, NULLSTR);

	free(pathname);

	if
	(
		stat(realname, &statb) != SYSERROR
		&&
		(statb.st_mode & S_IFMT) == S_IFREG
		&&
		(fd = open(realname, O_READ)) != SYSERROR
	)
	{
		Trace3(1, tfmt, statename, realname);

		(void)close(fd);
		free(prepath);
		return realname;
	}

	free(realname);
	statename = StripTypes(statename);

	if ( (pathname = DomainToPath(statename)) == NULLSTR )
	{
		free(statename);
		free(prepath);
		return NULLSTR;
	}

	realname = concat(prepath, pathname, NULLSTR);

	free(pathname);
	free(statename);

	if
	(
		stat(realname, &statb) != SYSERROR
		&&
		(statb.st_mode & S_IFMT) == S_IFREG
		&&
		(fd = open(realname, O_READ)) != SYSERROR
	)
	{
		Trace3(1, tfmt, statename, realname);

		(void)close(fd);
		free(prepath);
		return realname;
	}

	free(realname);
	free(prepath);

	return NULLSTR;
}



/*
**	Move new statefile into place, and unlock state file actions.
*/

void
NewState(fd, commit)
	FILE *	fd;
	bool	commit;
{
	Trace4(1, "NewState(%#lx, %s) <== %s", (PUlong)fd, commit?"commit":"nocommit", StateFile);

	if ( fd != NULL )
	{
		(void)fclose(fd);

		if ( commit )
		{
			(void)chmod(TmpFile, 0644);
			move(TmpFile, StateFile);
		}
		else
			(void)unlink(TmpFile);
	}

	if ( LockSet )
	{
		(void)unlink(LockFile);
		LockSet = false;
	}
}



/*
**	Open state file and read it.
**	If 'update' is true, lock state file actions
**		and create new file for writing.
*/

FILE *
ReadState(update, ignCRC)
	Lock_t		update;
	bool		ignCRC;
{
	register FILE *	fd;
	register char *	cp;
	register int	count = 0;

	if ( StateFile == NULLSTR )
	{
		StateFile = concat(SPOOLDIR, STATEDIR, STATEFILE, NULLSTR);
		LockState = true;
	}

	Trace4(1, "ReadState(%s, %s) <== %s", update?"update":"noupdate", ignCRC?"ignCRC":"doCRC", StateFile);

	if ( update && LockState )
	{
		if ( LockFile == NULLSTR )
			LockFile = concat(SPOOLDIR, STATEDIR, LOCKFILE, NULLSTR);

		while ( !SetLock(LockFile, Pid, false) )
		{
			if ( ++count > 3 )
			{
				Syserror(CouldNot, CreateStr, LockFile);
				return NULL;
			}

			(void)sleep(LOCK_SLEEP);
		}

		LockSet = true;
	}

	while ( (fd = fopen(StateFile, "r")) == NULL )
	{
		if ( LockSet )
		{
			(void)close(create(StateFile));
			continue;
		}

		if ( ++count > 3 )
		{
			Syserror(CouldNot, OpenStr, StateFile);
			return NULL;
		}

		(void)sleep(LOCK_SLEEP);
	}

#	if	SUN3 == 1
	(void)ungetc(count = getc(fd), fd);	/* Backward compatible with SUN III */

	if ( (count & TOKEN_C) == 0 )
		R3state(fd, false, NULLSTR, (Time_t)0, ignCRC);
	else
#	endif	/* SUN3 == 1 */
		Rstate(fd, false, NULLSTR, (Time_t)0, ignCRC);

	(void)fclose(fd);

	if ( AnteState || !update )
		return NULL;

	if ( TmpFile == NULLSTR )
	{
		(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);

		if ( (cp = strrchr(StateFile, '/')) != NULLSTR )
		{
			*cp = '\0';
			TmpFile = concat(StateFile, Slash, Template, NULLSTR);
			*cp = '/';
		}
		else
			TmpFile = newstr(Template);
	}

	(void)UniqueName(TmpFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time);

	while ( (fd = fopen(TmpFile, "w")) == NULL )
		if ( !CheckDirs(TmpFile) )
			Syserror(CouldNot, CreateStr, TmpFile);

	return fd;
}



/*
**	Set alternative name for state file.
*/

Time_t
SetState(newfile)
	char *		newfile;
{
	struct stat	statb;

	StateFile = newfile;

	Trace2(1, "SetState(%s)", StateFile);

	if ( stat(StateFile, &statb) == SYSERROR )
		Syserror(CouldNot, StatStr, StateFile);

	return (Time_t)statb.st_mtime;
}
