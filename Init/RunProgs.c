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


#define	SIGNALS
#define	STAT_CALL
#define	WAIT_CALL
#define	STDIO
#define	TIME
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"exec.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#define	INIT_DATA

#include	"Netinit.h"

#include	<setjmp.h>
#include	<time.h>


Time_t		ScanDate;

extern int	JmpSet;
extern jmp_buf	SetJmp;

static char *	CoreErrs;
static char *	CoreName;
static Time_t	NewTime;
static char	TimeStr[DATETIMESTRLEN];

#ifdef	SIG_SETMASK
sigset_t newset;
sigset_t oldset;
#endif	/* SIG_SETMASK */

void		catchalarm _FA_((int));
static void	exhume _FA_((void));
static bool	exhumepp _FA_((Proc *));
static bool	geterrors _FA_((char *, int, int));
static bool	inter _FA_((int, int));
static Time_t	newtime _FA_((void));
void		timerrf _FA_((FILE *));



/*
**	Run programs.
*/

void
RunProgs()
{
	register Proc *	pp;
	register Proc **ppp;
	struct tm *	lt;
	unsigned	wtime;
	int		pid;
	int		status;
	Time_t		rtime;
	Time_t		xtime;
	Time_t		action;
	struct stat	statb;
	DODEBUG(char	tbuf[DATETIMESTRLEN]);

	Trace1(1, "RunProgs()");

#	ifdef	SIG_SETMASK
	sigemptyset(&newset);
#	endif	/* SIG_SETMASK */

	for ( ;; )
	{
		if ( stat(InitFile, &statb) != SYSERROR && statb.st_mtime > ScanDate )
			doscan((FILE *)0);

		rtime = newtime();

		if ( netsignal )
		{
			NetMessage();	/* Forks off child */
			continue;
		}

		rtime -= rtime % 60;	/* ``rtime'' is always on the minute */
		action = rtime + 60;	/* When we should next wake up */

		lt = localtime(&rtime);

		Trace7(
			3,
			"%s localtime: min=%d hour=%d mday=%d mon=%d wday=%d",
			DateTimeStr(rtime, tbuf),
			lt->tm_min,
			lt->tm_hour,
			lt->tm_mday,
			lt->tm_mon,
			lt->tm_wday
		);

		for ( ppp = &Procs ; (pp = *ppp) != (Proc *)0 ; )
		{
			if ( (pid = pp->pid) == NOTRUNNING )
			{
				if ( netsignal || Time >= action )
				{
					ppp = &pp->next;
					continue;	/* Don't start any more procs */
				}

				switch ( pp->status )
				{
				default:
					break;

				case REMOVE:
					*ppp = pp->next;
					if ( pp->crontime != (Cron *)0 )
						free((char *)(pp->crontime));
					free(pp->command);
					free((char *)pp);
					continue;

				case KILL:
				case TERMINATE:
					pp->status = DISABLED;	/* What about CRON jobs? */
					pp->delay = 0;
					break;

				case DISABLED:
					if ( pp->type != CRON )
						break;
					if ( !cron(lt, pp) )
						break;
				case ENABLED:
					if
					(
						(xtime = pp->start + pp->delay) <= Time
						&&
						Run1Prog(pp)
					)
					{
						if ( pp->type == RUNONCE || pp->type == CRON )
							pp->status = DISABLED;
						pp->start = Time;
						pp->restarts++;
						pp->check = 0;
						ProgsActive++;
						(void)sleep(MINWAIT);
						Time += MINWAIT;
					}
					else
					if ( xtime < action )
						action = xtime;
				}

				ppp = &pp->next;
				continue;
			}

			switch ( pp->status )
			{
			case TERMINATE:	if ( kill(pid, SIGTERM) == SYSERROR )
					{
						if ( errno == ESRCH )
							pp->check = 1;
						else
							(void)SysWarn(CouldNot, "stop", pp->procId);
					}
					pp->status = DISABLED;	/* Don't overkill */
					break;
			case KILL:	if ( kill(pid, SIGKILL) == SYSERROR )
					{
						if ( errno == ESRCH )
							pp->check = 1;
						else
							(void)SysWarn(CouldNot, "kill", pp->procId);
					}
					pp->status = DISABLED;	/* Don't overkill */
					break;

			default:	if ( kill(pid, SIG0) == SYSERROR && errno == ESRCH )
					{
						if ( pp->check == 2 )
							(void)exhumepp(pp);
						else
							pp->check = 1;
					}
			}

			ppp = &pp->next;
		}

		if ( netsignal )
			continue;

		if ( action > newtime() )
		{
			if ( (wtime = action - Time) < MINSLEEP )
				wtime = MINSLEEP;
		}
		else
			wtime = MINSLEEP;

		Trace4(3, "rtime=%lu, action=%lu, wtime=%u", (PUlong)rtime, (PUlong)action, wtime);

#		if	WAIT_RESTART == 1
		if ( setjmp(SetJmp) )
		{
			errno = EINTR;
			pid = SYSERROR;
		}
		else
		{
			JmpSet = 1;
#		endif	/* WAIT_RESTART == 1 */

			NameProg(english("%s %d active"), Name, ProgsActive);

			(void)signal(SIGALRM, catchalarm);
			(void)alarm(wtime);

#			ifdef	SIG_SETMASK
			(void)sigprocmask(SIG_SETMASK, &newset, &oldset);
#			endif	/* SIG_SETMASK */

			if ( ProgsActive )
				pid = wait(&status);
			else
			{
				(void)pause();	/* Always returns EINTR */
				pid = SYSERROR;
			}

#		if	WAIT_RESTART == 1
		}
		JmpSet = 0;
#		endif	/* WAIT_RESTART == 1 */

		(void)alarm((unsigned)0);

		Trace3(4, "RunProgs() pid=%d, errno=%d", pid, errno);

		if ( pid == SYSERROR && errno == EINTR )
		{
			for ( pp = Procs ; pp != (Proc *)0 ; pp = pp->next )
				if ( pp->check == 1 )	/* Provisionally lost */
					pp->check = 2;	/* Seriously lost */

			continue;
		}

		(void)newtime();

		if ( pid == SYSERROR )
		{
			exhume();
			continue;
		}

		(void)inter(pid, status);
	}
}



bool
Run1Prog(pp)
	register Proc *	pp;
{
	ExBuf		eb;

	NameProg(english("%s STARTING %s"), Name, pp->command);

	for ( ;; )
	{
		switch ( pp->pid = fork() )
		{
		case SYSERROR:
			if ( errno == ENOMEM )
			{
				SysWarn(CouldNot, ForkStr, pp->command);
				(void)sleep(20);
			}
			else
				Syserror(CouldNot, ForkStr, pp->command);
			continue;

		case 0:
			ExInChild = true;
			break;

		default:
			MesgN(english("start"), "%s [%d]", pp->procId, pp->pid);
			Trace2(1, "\"%s\"", pp->command);
			return true;
		}

		break;
	}

#	if	SIG0 != 0
	(void)signal(SIG0, SIG_IGN);
#	endif	/* SIG0 != 0 */

	FIRSTARG(&eb.ex_cmd) = SHELL;
	NEXTARG(&eb.ex_cmd) = "-c";
	NEXTARG(&eb.ex_cmd) = pp->command;

	(void)Execute(&eb, NULLVFUNCP, ex_nofork, SYSERROR);	/* No return */

	return false;
}



/*
**	ALARM calls.
*/

void
catchalarm(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	if ( JmpSet )
		longjmp(SetJmp, 1);
}



/*
**	Write text of mail to NCC_ADMIN.
**
**	(Called from "MailNCC()".)
*/

void
corerrf(fd)
	FILE *	fd;
{
	(void)fprintf(fd, english("Subject: MHSnet process error.\n\n"));

	(void)fprintf
	(
		fd,
		english("Process \"%s\" dumped core, reason:\n%s\nIt will be restarted in %d minutes.\nPlease check \"%s%slog\".\n"),
		CoreName,
		CoreErrs,
		MAXDELAY/60,
		SPOOLDIR, LIBDIR
	);
}



static void
exhume()
{
	register Proc *	pp;

	ProgsActive = 0;

	for ( pp = Procs ; pp != (Proc *)0 ; pp = pp->next )
		if ( pp->pid != NOTRUNNING )
			if ( exhumepp(pp) )
				ProgsActive++;
}



static bool
exhumepp(pp)
	register Proc *	pp;
{
	if ( kill(pp->pid, SIGKILL) != SYSERROR || errno != ESRCH )
	{
		pp->check = 0;
		return true;
	}

	/*
	**	The above kill won't find ZOMBIE procs,
	**	unfortunately, so this may be bullshit:
	*/

	Warn
	(
		english("%s \"%s\" (pid=%d) won't die!"),
		pp->procId,
		pp->command,
		pp->pid
	);

	pp->pid = NOTRUNNING;

	return false;
}



static bool
geterrors(name, pid, status)
	char *	name;
	int	pid;
	int	status;
{
	char *	errs;
	bool	val;
	VarArgs	va;

	FIRSTARG(&va) = NULLSTR;
	ExStatus = ((status >> 8) & 0xff) | ((status & 0xff) << 8);
	errs = GetErrFile(&va, status, SYSERROR);
	MesgN(english("ERROR"), "%s [%d] %s", name, pid, errs);

	if ( status & 0x80 )	/* core */
	{
		char *	err2;

		CoreErrs = errs;
		CoreName = name;

		if ( (err2 = MailNCC(corerrf, NCC_ADMIN)) != NULLSTR )
		{
			Warn(StringFmt, err2);
			free(err2);
		}

		val = false;
	}
	else
		val = true;

	free(errs);

	return val;
}



static bool
inter(pid, status)
	int		pid;
	int		status;
{
	register Proc *	pp;
	Time_t		xtime;

	if ( pid == SYSERROR )
		return false;

	for ( pp = Procs ; pp != (Proc *)0 ; pp = pp->next )
	{
		if ( pp->pid != pid )
			continue;

		ProgsActive--;

		if ( status )
		{
			if ( !geterrors(pp->procId, pid, status) )
				pp->delay = MAXDELAY;
			pp->errors++;
		}
		else
		if ( pp->type == RESTART )
			MesgN(english("exit "), "%s [%d]", pp->procId, pid);

		xtime = Time - pp->start;

		if
		(
			pp->type == RESTART
			&&
			(xtime < CRONDELAY || status)
		)
		{
			if ( (pp->delay += WAITINCR) > MAXDELAY )
				pp->delay = MAXDELAY;
		}
		else
		if ( pp->type == CRON )
			pp->delay = xtime + CRONDELAY - (Time%CRONDELAY);
		else
		if ( xtime > MAXDELAY )
			pp->delay = MINDELAY;

		pp->pid = NOTRUNNING;

		return true;
	}

	if ( status )
		(void)geterrors(english("unknown process"), pid, status);

	return false;
}



static Time_t
newtime()
{
	register Time_t	t;
	register Proc *	pp;

	if ( (t = time((time_t *)0)) < (Time-NVETIMECHANGE) )
	{
		char *	errs;
		char *	admin;

		(void)signal(SIGWAKEUP, SIG_IGN);

		(void)DateTimeStr(Time, TimeStr);
		NewTime = t;
		t = Time - NewTime;

		StartTime -= t;

		for ( pp = Procs ; pp != (Proc *)0 ; pp = pp->next )
			if ( pp->start > t )
				pp->start -= t;

		MesgN(english("ERROR"), english("negative time change from %s"), TimeStr);

		if ( StringMatch(NCC_ADMIN, "root") == NULLSTR )
			admin = concat("root ", NCC_ADMIN, NULLSTR);
		else
			admin = newstr(NCC_ADMIN);

		if ( (errs = MailNCC(timerrf, admin)) != NULLSTR )
		{
			Warn(StringFmt, errs);
			free(errs);
		}

		free(admin);
		(void)sleep(30);	/* Time to deliver mail before netshutdown? */
		Time += 30;		/* For netshutdown() */

		netshutdown();

		Time = time((time_t *)0);
		t = Time - NewTime;

		if ( NveTimeDelay > t )
			(void)sleep((int)(NveTimeDelay-t));	/* Pause, for thought */

		Time = t = time((time_t *)0);

		MesgN(english("RESTARTED"), english("after negative time change"));

		netsignal = 0;
		(void)signal(SIGWAKEUP, NetSig);
	}

	DODEBUG(if(Traceflag>=4)MesgN("newtime()","%lu",(PUlong)t));

	Time = t;
	return t;
}



void
netshutdown()
{
	register Proc *	pp;
	register int	sig;
	register int	pid;
	register int	loops;
	int		status;
	static int	count;
	static int	active;

	NameProg(english("%s SHUTDOWN"), Name);

	for ( count = 1, loops = 3 ; count > 0 && --loops >= 0 ; )
	{
		if ( loops )
			sig = SIGTERM;
		else
			sig = SIGKILL;

		for ( active = 0, count = 0, pp = Procs ; pp != (Proc *)0 ; pp = pp->next )
			if ( (pid = pp->pid) != NOTRUNNING )
			{
				count++;
				active++;

				if ( kill(pid, sig) != SYSERROR )
					continue;

				if ( errno == ESRCH )
				{
					active--;
					continue;
				}

				(void)SysWarn
				(
					english("Could not kill %s \"%s\" (pid=%d)"),
					pp->procId,
					pp->command,
					pid
				);
			}

#		if	WAIT_RESTART == 1
		if ( !setjmp(SetJmp) )
		{
			JmpSet = 1;
#		endif	/* WAIT_RESTART == 1 */

			while ( count > 0 )
			{
				(void)signal(SIGALRM, catchalarm);
				(void)alarm((unsigned)10);

#				ifdef	SIG_SETMASK
				(void)sigprocmask(SIG_SETMASK, &newset, &oldset);
#				endif	/* SIG_SETMASK */

				pid = wait(&status);

				(void)alarm((unsigned)0);

				if ( pid == SYSERROR )
				{
					if ( errno == ECHILD )
					{
						count = 0;
						active = 0;
					}

					break;
				}

				if ( inter(pid, status) )
				{
					count--;
					active--;
				}
			}
#		if	WAIT_RESTART == 1
		}
		JmpSet = 0;
#		endif	/* WAIT_RESTART == 1 */
	}

	if ( active > 0 )
		Warn(english("network processes won't die"));
	else
		exhume();
}



/*
**	Write text of mail to NCC_ADMIN.
**
**	(Called from "MailNCC()".)
*/

void
timerrf(fd)
	FILE *	fd;
{
	char	t1[TIMESTRLEN];
	char	t2[TIMESTRLEN];
	char	d1[DATETIMESTRLEN];

	(void)fprintf(fd, english("Subject: negative time change detected by MHSnet.\n\n"));

	(void)fprintf
	(
		fd,
		english("System time changed backwards by %s\n\tfrom %s\n\t  to %s.\n\n\
The network will resume automatically in %s,\n\
or you may restart sooner by \"kill %d; %s%sstart\".\n"),
		TimeString(Time-NewTime, t1),
		TimeStr,
		DateTimeStr(NewTime, d1),
		TimeString((Time_t)NveTimeDelay, t2),
		Pid,
		SPOOLDIR, LIBDIR
	);
}
