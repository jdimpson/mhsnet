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


static char	sccsid[]	= "@(#)net.c	1.28 05/12/16";

/*
**	Send a message to netinit and wait for a reply
*/

#define	NEED_HZ
#define	SIGNALS
#define	STAT_CALL
#define	STDIO
#define	TIME
#define	TIMES
#define	ERRNO

#include	"global.h"
#include	"Args.h"
#include	"debug.h"
#include	"link.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */

#include	"net.h"

#include	<setjmp.h>


/*
**	Variables set from program arguments
*/

char *		CheckLock;	/* Lock file name */
char *		Cmd;		/* Command to send to netinit */
bool		Force;		/* Wait if locked */
char *		Name = sccsid;	/* Program name */
char *		OutFile;	/* Output file name */
char *		RegExp;		/* Regular expression arg for command */
bool		Silent;		/* No comment */
int		Traceflag;	/* Debug trace level */
bool		Validate;	/* Validate lock file time */

AFuncv		getCmd _FA_((PassVal, Pointer));

Args	Usage[] =
{
        Arg_0(0, getName),
	Arg_bool(f, &Force, 0),
	Arg_bool(s, &Silent, 0),
	Arg_bool(v, &Validate, 0),
	Arg_string(F, &OutFile, 0, english("output-file-name"), OPTARG),
	Arg_string(L, &CheckLock, 0, english("lock-dir-name"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("trace-level"), OPTARG|OPTVAL),
	Arg_noflag(0, getCmd, english("command"), OPTARG),
	Arg_noflag(&RegExp, 0, english("regular-expression"), OPTARG),
	Arg_end
};

/*
**	Miscellaneous
*/

FILE *		Fd;
char *		InitArgs;
Passwd		Invoker;
char *		LibDir;
bool		LockSet;
char *		NetLockName;
char		ShutDownCmd[]	= "shutdown";
int		Woken;

bool		canwritetty _FA_((int));
bool		signal_links _FA_((void));
void		TrapSig _FA_((int));

extern char *	ttyname _FA_((int));



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	int	i;
	char *	cp;
	bool	outfisatty;
	bool	netshutdown;
	int	netinit_pid;
	int	retval = EX_OK;

	InitParams();

	DoArgs(argc, argv, Usage);

	GetInvoker(&Invoker);
	SetUser(NetUid, NetGid);

	if ( !(Invoker.P_flags & P_SU) || geteuid() != NetUid )
	{
		Warn(english("No permission."));
		exit(EX_NOPERM);
	}

	if ( CheckLock != NULLSTR )
	{
		cp = concat(SPOOLDIR, CheckLock, Slash, NULLSTR);

		if ( !DaemonActive(cp, false) )
		{
			if ( !Silent )
				Warn(english("daemon not running."));

			exit(EX_UNAVAILABLE);
		}

		if ( !Silent )
		{
			(void)fprintf(stdout, english("%d"), LockPid);

			if ( strcmp(NodeName(), LockNode) != STREQUAL )
				(void)fprintf(stdout, english(" (%s)"), LockNode);

			(void)fprintf(stdout, "\n");
		}

		if ( Validate )
		{
			Time_t		boot_time;
			struct tms	tbuf;

			boot_time = time((time_t *)0) - times(&tbuf)/HZ;

			if ( LockTime < (boot_time - (10*60)) )
			{
				if ( !Silent )
					(void)fprintf(stdout, english("Warning: lock may pre-date system boot.\n"));

				exit(EX_UNAVAILABLE);
			}
		}

		exit(EX_OK);
	}

	if ( Cmd == NULLSTR )
	{
		if ( strccmp(Name, ShutDownCmd) != STREQUAL )
			usagerror(ArgsUsage(Usage));

		Cmd = ShutDownCmd;
		netshutdown = true;
	}
	else
	if ( strcmp(Cmd, ShutDownCmd) == STREQUAL )
		netshutdown = true;
	else
		netshutdown = false;

	LibDir = concat(SPOOLDIR, LIBDIR, NULLSTR);

	if ( !DaemonActive(LibDir, false) )
	{
		if ( !Silent )
			Warn(english("netinit not running."));

		exit(EX_UNAVAILABLE);
	}

	netinit_pid = LockPid;

	if
	(
		strcmp(NodeName(), LockNode) != STREQUAL
		&&
		strcmp(NodeName(), Unknown) != STREQUAL
		&&
		strcmp(Unknown, LockNode) != STREQUAL
	)
	{
		Warn(english("netinit active on %s."), LockNode);
		exit(EX_NOHOST);
	}

	NetLockName = concat(LibDir, "control.lock", NULLSTR);

	for ( i = 0 ; ; i++ )
	{
		if ( SetLock(NetLockName, Pid, false) )
			break;

		if ( !Force || i >= 3 )
		{
			Warn(english("%s (%d) active on %s."), Name, LockPid, LockNode);
			exit(EX_TEMPFAIL);
		}

		if ( Force && netshutdown )
		{
shutout:		(void)kill(netinit_pid, SIGTERM);
			signal_links();
			exit(EX_TEMPFAIL);
		}

		(void)sleep(MINSLEEP+3);
	}

	LockSet = true;

	if ( RegExp == NULLSTR )
		RegExp = EmptyString;

	Trace3(3, "command = \"%s\", regular expression = \"%s\"", Cmd, RegExp);

	InitArgs = concat(LibDir, NETINITARGSFILE, NULLSTR);

	while ( (Fd = fopen(InitArgs, "w")) == (FILE *)0 )
	{
		if ( Force && netshutdown )
			goto shutout;
		Syserror(CouldNot, OpenStr, InitArgs);
	}

	outfisatty = false;

	if ( OutFile == NULLSTR )
	{
		if ( Silent )
			OutFile = DevNull;
		else
		if ( isatty(1) && canwritetty(1) )
		{
			outfisatty = true;
			OutFile = ttyname(1);
		}
		else
			OutFile = concat(LibDir, "netinit.reply", NULLSTR);
	}

	Trace2(1, "OutFile=%s", OutFile);

	(void)fprintf(Fd, "%s\n%d\n%s\n%s\n", OutFile, Pid, Cmd, RegExp);
	if ( fclose(Fd) != 0 && Force && netshutdown )
		goto shutout;

	(void)signal(SIGALRM, TrapSig);
	(void)signal(SIGINT, TrapSig);
	(void)signal(SIGWAKEUP, TrapSig);

	(void)sleep(MINSLEEP);

	if ( !DaemonActive(LibDir, true) )
	{
		if ( !Silent )
			Warn(english("netinit not running."));
		retval = EX_UNAVAILABLE;
	}
	else
	{
		(void)alarm(netshutdown?(unsigned)34:(unsigned)10);

		if ( !Woken )
			pause(); /* wait until netinit finishes */

		(void)alarm((unsigned)0);

		if ( !outfisatty && !Silent )
		{
			if ( (cp = ReadFile(OutFile)) != NULLSTR )
				(void)fprintf(stdout, "%s\n", cp);

			if ( unlink(OutFile) == SYSERROR )
				SysWarn(CouldNot, UnlinkStr, OutFile);
		}
	}

	if ( netshutdown && signal_links() )
	{
		if ( Silent )
			retval = EX_UNAVAILABLE;
		else
			Warn(english("transport daemons active"));
	}

	finish(retval);
	return 0;
}



bool
canwritetty(fd)
	int		fd;
{
	struct stat	statb;

	while ( fstat(fd, &statb) == SYSERROR )
		Syserror(CouldNot, StatStr, "tty");

	Trace3(1, "canwritetty(%d) ==> 0%o", fd, statb.st_mode & 2);

	if ( statb.st_mode & 2 )
		return true;

	return false;
}



void
finish(error)
	int	error;
{
	if ( LockSet )
	{
		(void)unlink(InitArgs);
		(void)unlink(NetLockName);
	}

	exit(error);
}



/*
**	Command/pattern argument.
*/
/*ARGSUSED*/
AFuncv
getCmd(val, arg)
	PassVal	val;
	Pointer	arg;
{
	if ( Cmd != NULLSTR )
		return REJECTARG;

	Cmd = val.p;

	return ACCEPTARG;
}



/*
**	Find any links still active and signal them if possible.
*/

bool
signal_links()
{
	register int	i;
	register char *	lockfile;
	bool		active = false;
	Link		data;

	(void)sleep(MINSLEEP+1);
	(void)ReadRoute(NOSUMCHECK);

	for ( i = LinkCount ; --i >= 0 ; )
	{
		GetLink(&data, (Index)i);

		lockfile = concat(SPOOLDIR, data.ln_name, Slash, LINKCMDSNAME, Slash, LOCKFILE, NULLSTR);

		if ( !SetLock(lockfile, 0, false) )
		{
			if
			(
				strcmp(LockNode, NodeName()) != STREQUAL
				||
				kill(LockPid, SIGTERM) == SYSERROR
			)
				active = true;
		}

		free(lockfile);
	}

	return active;
}



void
TrapSig(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);

	Woken++;

	if ( sig == SIGALRM )
		Warn(english("Communications with ``netinit'' timed-out"));
	else
		Trace1(1, "Woken");
}
