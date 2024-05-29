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
#define	LOCKING
#define	SIGNALS

#include	"global.h"
#include	"debug.h"
#include	"lockfile.h"
#include	"Passwd.h"

#include	<setjmp.h>


#define	LOCK_ALRM	(2*60)

static jmp_buf	BrkJmp;
static LockFile	LockData;
char *		LockNode;
int		LockPid;
Time_t		LockTime;
static bool	write_lock _FA_((int, char *, int, int));

#	if	DEBUG
static char	Rfalse[]	= "SetLock return false";
static char	Rtrue[]		= "SetLock return true";
#	endif	/* DEBUG */



void
lockalarmcatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	longjmp(BrkJmp, 1);
}



#if	AUTO_LOCKING != 1
static bool
lockfile(file, fd)
	char *	file;
	int	fd;
{
	int	count;

	for ( count = 3 ; Lock(file, fd, for_writing) == SYSERROR ; --count )
		if ( count <= 0 || !SysWarn(CouldNot, LockStr, file) )
			return false;

	return true;
}
#endif	/* AUTO_LOCKING != 1 */



/*
**	Reset a lock file with passed pid provided old pids match.
*/

bool
ReSetLock(file, pid, opid)
	char *		file;
	int		pid;
	int		opid;
{
	register int	fd;
	unsigned	prev_alarm;
	SigFunc *	prev_al_sig;

	Trace4(1, "ReSetLock(\"%s\", %d, %d)", file, pid, opid);

	if ( pid == 0 )
		Fatal1("ReSetLock() pid==0");

	if ( setjmp(BrkJmp) )
		Error(CouldNot, LockStr, file);

	prev_al_sig = (SigFunc *)signal(SIGALRM, lockalarmcatch);
	prev_alarm = alarm((unsigned)LOCK_ALRM);

	while ( (fd = open(file, O_RDWR)) == SYSERROR )
		Syserror(CouldNot, OpenStr, file);

#	if	AUTO_LOCKING != 1
	if ( !lockfile(file, fd) )
	{
		Trace1(1, Rfalse);
		return false;
	}
#	endif	/* AUTO_LOCKING != 1 */

	(void)alarm(prev_alarm);
	(void)signal(SIGALRM, prev_al_sig);

	if
	(
		read(fd, (char *)&LockData, sizeof(LockData)) != sizeof(LockData)
		||
		opid != atoi(LockData.lck_pid)
		||
		strcmp(NodeName(), LockData.lck_node) != STREQUAL
	)
	{
#		if	AUTO_LOCKING != 1
		(void)UnLock(fd);
#		endif	/* AUTO_LOCKING != 1 */

		(void)close(fd);

		Trace1(1, Rfalse);
		return false;
	}

	return write_lock(fd, file, pid, opid);
}



/*
**	Set/test a lock file with passed pid.
**
**	If pid == 0, just test.
*/

bool
SetLock(file, pid, wakeup)
	char *		file;
	int		pid;
	bool		wakeup;
{
	register int	opid;
	register int	count;

	struct stat	statb;

	static int	fd;
	static bool	looping;
	static unsigned	prev_alarm;
	static SigFunc *prev_al_sig;

	Trace4(1, "SetLock(\"%s\", %d, %s)", file, pid, wakeup?"wakeup":"nowake");

	GetNetUid();

	LockNode = NULLSTR;
	LockPid = 0;
	LockTime = 0;
	looping = false;
	fd = SYSERROR;

	if ( setjmp(BrkJmp) )
	{
		char *	cp;

		if ( fd != SYSERROR )
		{
#			if	AUTO_LOCKING != 1
			if ( pid )
				(void)UnLock(fd);
#			endif	/* AUTO_LOCKING != 1 */
			(void)close(fd);
			fd = SYSERROR;
		}

		if ( pid == 0 )
		{
			(void)alarm(prev_alarm);
			(void)signal(SIGALRM, prev_al_sig);
			Trace1(1, Rfalse);
			return false;
		}

#		if	TRUNCDIRNAME == 1
		if ( (cp = strrchr(file, '/')) == NULLSTR )
			opid = strlen(file);
		else
			opid = strlen(cp);

		if ( looping || opid > 11 )
#		else	/* TRUNCDIRNAME == 1 */
		if ( looping )
#		endif	/* TRUNCDIRNAME == 1 */
			Error(CouldNot, LockStr, file);
		looping = true;

		if ( stat(file, &statb) != SYSERROR )
		{
			cp = concat(file, "BAD", NULLSTR);
			move(file, cp);
			free(cp);
		}

		(void)alarm((unsigned)LOCK_ALRM);
	}
	else
	{
		prev_al_sig = (SigFunc *)signal(SIGALRM, lockalarmcatch);
		prev_alarm = alarm((unsigned)LOCK_ALRM);
	}

	if ( stat(file, &statb) == SYSERROR )
	{
		if ( pid == 0 )	/* Could set lock, but don't want to */
		{
			(void)alarm(prev_alarm);
			(void)signal(SIGALRM, prev_al_sig);
			Trace1(1, Rtrue);
			return true;
		}

		if ( !MakeLock(file) )
		{
			Trace1(1, Rfalse);
			return false;
		}
	}

	LockTime = statb.st_mtime;

	if ( statb.st_uid != NetUid )
	{
		(void)alarm((unsigned)0);
		(void)sleep(MINSLEEP+1);	/* Avoid race with another MakeLock */
		(void)alarm((unsigned)LOCK_ALRM);
	}

	for ( count = 3 ; (fd = open(file, (pid==0)?O_READ:O_RDWR)) == SYSERROR ; --count )
		if ( count <= 0 || !SysWarn(CouldNot, OpenStr, file) )
		{
			(void)alarm(prev_alarm);
			(void)signal(SIGALRM, prev_al_sig);
			Trace1(1, Rfalse);
			return false;
		}

#	if	AUTO_LOCKING != 1
	if ( pid )
		if ( !lockfile(file, fd) )
		{
			Trace1(1, Rfalse);
			return false;
		}
#	endif	/* AUTO_LOCKING != 1 */

	(void)alarm(prev_alarm);
	(void)signal(SIGALRM, prev_al_sig);

	opid = 0;

	if
	(
		read(fd, (char *)&LockData, sizeof(LockData)) == sizeof(LockData)
		&&
		(opid = atoi(LockData.lck_pid)) > 0
		&&
		(
			strcmp(NodeName(), LockData.lck_node) != STREQUAL
			||
			(opid != pid && kill(opid, wakeup?SIGWAKEUP:SIG0) != SYSERROR)
		)
	)
	{
#		if	AUTO_LOCKING != 1
		if ( pid )
			(void)UnLock(fd);
#		endif	/* AUTO_LOCKING != 1 */

		(void)close(fd);

		Trace3(2, "SetLock: lock already set with pid %d, node %s", opid, LockData.lck_node);

		LockPid = opid;
		LockNode = LockData.lck_node;

		Trace1(1, Rfalse);
		return false;
	}

	return write_lock(fd, file, pid, opid);
}

static bool
write_lock(fd, file, pid, opid)
	int		fd;
	char *		file;
	int		pid;
	int		opid;
{
	if ( pid != 0 && opid != pid )
	{
		int	count;

		(void)strclr((char *)&LockData, sizeof(LockData));
		(void)sprintf(LockData.lck_pid, "%d", pid);
		(void)strcpy(LockData.lck_node, NodeName());

		for ( count = 3 ;; --count )
		{
			(void)lseek(fd, (long)0, 0);

			if ( write(fd, (char *)&LockData, sizeof(LockData)) == sizeof(LockData) )
				break;

			if ( count <= 0 || !SysWarn(CouldNot, WriteStr, file) )
			{
#				if	AUTO_LOCKING != 1
				(void)UnLock(fd);
#				endif	/* AUTO_LOCKING != 1 */
				(void)close(fd);

				Trace1(1, Rfalse);
				return false;
			}
		}
	}

	LockPid = pid;
	LockNode = LockData.lck_node;

#	if	AUTO_LOCKING != 1
	if ( pid )
		(void)UnLock(fd);
#	endif	/* AUTO_LOCKING != 1 */

	(void)close(fd);

	Trace1(1, Rtrue);
	return true;
}
