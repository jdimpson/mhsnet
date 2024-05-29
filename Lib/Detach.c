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
#define	TERMIOCTL
#define	SIGNALS
#define	STAT_CALL

#include	"global.h"
#include	"debug.h"
#include	"Passwd.h"
#include	"sysexits.h"

#if	BSD4 >= 2
#include	<sys/time.h>
#include	<sys/resource.h>
#endif	/* BSD4 >= 2 */


void	ignore _FA_((int));



/*
**	Detach a daemon from the world.
*/

int
Detach(nofork, niceval, setid, setpg)
	bool		nofork;
	int		niceval;
	bool		setid;
	bool		setpg;
{
	register int	fd;
	int		pid = getpid();

	Trace5(
		1,
		"Detach(%sfork, %d, %sid, %ssetpg)",
		nofork?"no":EmptyString,
		niceval,
		setid?"set":"keep",
		setpg?EmptyString:"no "
	);

	ignore(SIGHUP);
	ignore(SIGINT);
	ignore(SIGQUIT);
	ignore(SIGALRM);	/* To play safe */
	ignore(SIGWAKEUP);	/* To preempt sleeps */
	ignore(SIGERROR);

#	ifdef	SIGTTOU
	ignore(SIGTTOU);
#	endif
#	ifdef	SIGTTIN
	ignore(SIGTTIN);
#	endif
#	ifdef	SIGTSTP
	ignore(SIGTSTP);
#	endif

#	if	KILL_0 != 1 && SIG0 != SIGINT
	ignore(SIG0);		/* To allow interrogation for existence */
#	endif

	if ( getppid() != 1 )
	{
		if ( !nofork )
		switch ( fork() )
		{
		default:
			_exit(EX_OK);	/* Parent returns */

		case SYSERROR:
			exit(EX_OSERR);	/* Bad news */

		case 0:
			pid = getpid();
		}

#		ifdef	TIOCNOTTY
		if ( setpg )
		{
			(void)signal(SIGALRM, ignore);
			(void)alarm(20);	/* Some kernels are ... */
			if ( (fd = open("/dev/tty", O_RDWR)) != SYSERROR )
			{
				(void)ioctl(fd, TIOCNOTTY, (void *)0);
				(void)close(fd);
			}
			ignore(SIGALRM);
			(void)alarm(0);
		}
#		endif

#		if	SYSTEM > 1
		if ( !nofork )
		switch ( fork() )
		{
		default:
			_exit(EX_OK);	/* Parent returns */

		case SYSERROR:
			exit(EX_OSERR);	/* Bad news */

		case 0:
			pid = getpid();
		}
#		endif	/* SYSTEM > 1 */

#		if	POSIX == 1
		if ( setpg && !nofork )
			setsid();
#		endif	/* POSIX == 1 */
	}

#	if	PGRP == 1
#	if	BSD4 >= 2
#	if	BSDI >= 2 || __NetBSD__ == 1 || __APPLE__ == 1
	if ( setpg && pid != getpgrp() )
#	else	/* BSDI >= 2 */
	if ( setpg && pid != getpgrp(0) )
#	endif	/* BSDI >= 2 */
		(void)setpgrp(0, pid);
#	else	/* BSD4 >= 2 */
	if ( setpg && pid != getpgrp() )
#	if	BSD4PGRP == 1
		setpgrp(0, pid);
#	else	/* BSD4PGRP == 1 */
		setpgrp();
#	endif	/* BSD4PGRP == 1 */
#	endif	/* BSD4 >= 2 */
#	endif	/* PGRP == 1 */

	for ( fd = 3 ; close(fd) != SYSERROR || fd < 9 ; fd++ );

	(void)umask(027);

	SetUlimit();

	if ( niceval != 0 )
	{
#		if	BSD4 >= 2
		(void)setpriority(PRIO_PROCESS, 0, niceval);
#		else	/* BSD4 >= 2 */
#		if	SYSTEM > 0
#		ifndef	QNX
		(void)nice(niceval-nice(0));
#		endif	/* QNX */
#		else	/* SYSTEM > 0 */
		if ( geteuid() == 0 )
		{
			(void)nice(-40);
			(void)nice(20+niceval);
		}
		else
		if ( niceval > 0 )
			(void)nice(niceval);
#		endif	/* SYSTEM > 0 */
#		endif	/* BSD4 >= 2 */
	}

	if ( setid )
	{
		GetNetUid();
		SetUser(NetUid, NetGid);
	}

	Trace6(
		1,
		"Detach ==> pid=%d, uid/gid=%d/%d, euid/egid=%d/%d",
		pid,
		getuid(), getgid(),
		geteuid(), getegid()
	);

	return pid;
}



void
ignore(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
}
