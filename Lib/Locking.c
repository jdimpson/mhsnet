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
**	Locking primitives for Unices without them.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	ERRNO

#include	"global.h"



#if	AUTO_LOCKING == 0
#ifdef	F_SETLKW

int
_Lock(file, fd, type)
	char *		file;
	int		fd;
	Lock_t		type;
{
	struct flock	l;

#	ifdef	lint
	file = file;
#	endif	/* lint */

	l.l_whence = 0;
	l.l_start = 0;
	l.l_len = 0;
	l.l_type = (type == for_reading) ? F_RDLCK : F_WRLCK;
	return fcntl(fd, F_SETLKW, &l);
}



void
_UnLock(fd)
	int		fd;
{
	struct flock	l;

	l.l_whence = 0;
	l.l_start = 0;
	l.l_len = 0;
	l.l_type = F_UNLCK;
	(void)fcntl(fd, F_SETLKW, &l);
}

#else	/* F_SETLKW */

#if	LOCKF == 1

lockf_all(fd, op)
{
	register long	l;
	register int	r;

	l = lseek(fd, 0L, 1);	/* Remember where we were */
	(void)lseek(fd, 0L, 0);	/* Start of file */
	r = lockf(fd, op, 0);	/* Lock whole file */
	(void)lseek(fd, l, 0);	/* Back where we were */

	return r;
}

#else	/* LOCKF == 1 */

/*
**	If a lock file is older than LOCK_TIMEOUT it is probably bogus.
*/

#define	LOCK_TIMEOUT	(20*60)
#undef	LOCK_SLEEP
#define	LOCK_SLEEP	(2*60)

#if	V8 == 1

/*
**	Absolute locking: 1 access/time.
*/

int
_Lock(file, fd, type)
	char *		file;
	register int	fd;
	Lock_t		type;
{
#	ifndef	FIOABLOCK
	register int	fib_1;
	register int	fib_2;
#	endif	/* FIOABLOCK */

#	ifdef	FIOABLOCK

	return ioctl(fd, FIOABLOCK, 0);

#	else	/* FIOABLOCK */

	fib_1 = 1;
	fib_2 = 1;

	for ( ;; )
	{
		register int	f;

		if ( ioctl(fd, FIOALOCK, 0) != SYSERROR )
			return 0;

		if ( errno != EPERM )
		{
			if ( type == for_reading )
				return 0;
			else
				return SYSERROR;
		}

		(void)sleep(fib_1);

		f = fib_1;
		fib_1 = fib_2;
		if ( fib_2 < LOCK_SLEEP )
			fib_2 += f;
	}
#	endif	/* FIOABLOCK */
}

#else	/* V8 == 1 */

#include	"debug.h"



static char *	LockName;

/*
**	Absolute locking: 1 access/time.
*/
/*ARGSUSED*/
int
_Lock(file, fd, type)
	char *		file;
	int		fd;
	Lock_t		type;
{
	register int	fib_1;
	register int	fib_2;
	register int	total;
	register int	f;
	struct stat	statb;

	if ( LockName != NULLSTR )
	{
		Fatal3("attempt to lock \"%s\" while \"%s\" exists", file, LockName);
		return SYSERROR;
	}

	LockName = concat(file, ".l", NULLSTR);

	fib_1 = 1;
	fib_2 = 1;
	total = 0;

	do
	{
#		if	FCNTL && defined(O_EXCL)
#		if	KILL_0
		int	mypid;
#		endif	/* KILL_0 */

		if ( (f = open(LockName, O_WRITE | O_CREAT | O_TRUNC | O_EXCL, 0664)) != SYSERROR )
		{
#			if	KILL_0
			mypid = getpid();
			(void) write(f, &mypid, sizeof mypid);
#			endif	/* KILL_0 */
			(void) close(f);
			return 0;
		}
#		else	/* FCNTL && defined(O_EXCL) */
		if ( link(file, LockName) != SYSERROR )
			return 0;
#		endif	/* FCNTL && defined(O_EXCL) */

		if ( errno == EACCES )
		{
			free(LockName);
			LockName = NULLSTR;

			if ( type == for_reading )
				return 0;
			else
				return SYSERROR;
		}

		if ( errno != EEXIST )
		{
			(void)SysWarn(CouldNot, CreateStr, LockName);
			free(LockName);
			LockName = NULLSTR;

			return SYSERROR;
		}

#		if	FCNTL && KILL_0 && defined(O_EXCL)
		if
		(
			(f = open(LockName, O_READ, 0)) != SYSERROR
			&&
			read(f, &mypid, sizeof mypid) == sizeof mypid
			&&
			close(f) != SYSERROR
			&&
			kill(mypid, 0) == SYSERROR
			&&
			errno == ESRCH
		)
		{
			Warn("dead proc has lockfile %s", LockName);

			if ( unlink(LockName) == SYSERROR )
				(void)SysWarn(CouldNot, UnlinkStr, LockName);
		}
		else
#		endif	/* FCNTL && KILL_0 && defined(O_EXCL) */
		if ( stat(LockName, &statb) != SYSERROR )
		{
			long		cur_time;

			(void)time(&cur_time);

			if ( cur_time - statb.st_ctime > LOCK_TIMEOUT )
			{
				Warn("timeout on lockfile %s", LockName);

				if ( unlink(LockName) == SYSERROR )
					(void)SysWarn(CouldNot, UnlinkStr, LockName);
			}
		}

		(void)sleep(fib_1);

		total += fib_1;
		f = fib_1;
		fib_1 = fib_2;
		fib_2 += f;
	}
		while ( total < LOCK_SLEEP );

	return SYSERROR;
}



void
_UnLock()
{
	if ( LockName != NULLSTR )
	{
		if ( unlink(LockName) == SYSERROR )
			(void)SysWarn(CouldNot, UnlinkStr, LockName);
		free(LockName);
		LockName = NULLSTR;
	}
}

#endif	/* V8 == 1 */
#endif	/* LOCKF == 1 */
#endif	/* F_SETLKW */
#else	/* AUTO_LOCKING == 0 */

void null_lock(){};

#endif	/* AUTO_LOCKING == 0 */
