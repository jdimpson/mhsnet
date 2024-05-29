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


static char	sccsid[]	= "@(#)call.c	1.65 05/12/16";

/*
**	Call a remote host and start a network daemon on it.
**
**	SETUID ==> root.
*/

#define	WAIT_CALL
#define	FILE_CONTROL
#define	RECOVER
#define	SELECT
#define	SIGNALS
#define	STAT_CALL
#define	STDIO
#define	TERMIOCTL
#define	TIMES
#define	ERRNO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"exec.h"
#include	"handlers.h"
#include	"link.h"
#include	"params.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sysexits.h"

#include	"call.h"	/* Includes stdio */
#include	"setup.h"

#include	<ctype.h>
#if	BSD4 >= 2 || linux == 1
#include	<sys/time.h>
#endif	/* BSD4 >= 2 || linux == 1 */


/*
**	Variables set from program arguments.
*/

bool	Background;		/* True if call should run in background */
char *	ConnectArg;		/* Optional string to be appended to CONNECTLOG */
bool	IgnoreOldPid;		/* Ignore lock file pid */
char *	LinkDir;		/* Directory for link */
char *	LinkName;		/* Real name of target host */
char *	Name	= sccsid;	/* Program name */
bool	NoUpDown;		/* Don't signal state changes */
bool	Testing;		/* Don't create link */
int	Traceflag;		/* Global tracing control */
bool	Verbose;		/* Complain about locked circuits (always true) */

AFuncv	DefSymbol _FA_((PassVal, Pointer));	/* Function to define a symbol */
AFuncv	getLink _FA_((PassVal, Pointer));	/* Function to lookup link name in routing tables */
AFuncv	getLogFile _FA_((PassVal, Pointer));	/* Function to open log file */
AFuncv	getStr _FA_((PassVal, Pointer));	/* Save copy of string */

/*
**	Must save strings because using "NameProg()"
*/

Args	Usage[] =
{
	Arg_0(&Name, getName),
	Arg_bool(&, &Background, 0),
	Arg_bool(p, &IgnoreOldPid, 0),
	Arg_bool(t, &Testing, 0),
	Arg_bool(u, &NoUpDown, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_string(D, 0, DefSymbol, english("symbol=value"), OPTARG|MANY),
	Arg_string(H, &HomeName, getStr, english("home-name"), OPTARG),
	Arg_string(K, &ConnectArg, getStr, english("acct string"), OPTARG),
	Arg_string(L, &LinkName, getStr, english("link-name"), OPTARG),
	Arg_string(N, &LinkDir, getStr, english("link-directory"), OPTARG),
	Arg_string(S, &CallScript.name, getStr, english("call-script"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("trace-level"), OPTARG|OPTVAL),
	Arg_string(X, 0, getLogFile, english("log-file-name"), OPTARG),
	Arg_noflag(0, getLink, english("link"), OPTARG|MANY),
	Arg_end
};

/*
**	``Tty'' type device speeds.
*/

#ifndef	B19200
#ifdef	EXTA
#define		B19200		EXTA
#endif	/* EXTA */
#endif	/* !B19200 */

#ifndef	B38400
#ifdef	EXTB
#define		B38400		EXTB
#endif	/* EXTB */
#endif	/* !B38400 */

typedef struct Speed
{
	char *	s_name;
	int	s_val;
}
		Speed;

Speed		Speeds[] =
{
	 {"110",	B110}
	,{"1200",	B1200}
#	ifdef	B19200
	,{"19200",	B19200}
#	endif	/* B19200 */
	,{"2400",	B2400}
	,{"300",	B300}
#	ifdef	B38400
	,{"38400",	B38400}
#	endif	/* B38400 */
	,{"4800",	B4800}
	,{"600",	B600}
	,{"9600",	B9600}
#	ifdef	EXTA
	,{"EXTA",	EXTA}
#	endif	/* EXTA */
#	ifdef	EXTB
	,{"EXTB",	EXTB}
#	endif	/* EXTB */
};

#define		SPDZ		(sizeof(Speed))
#define 	NSPDS		((sizeof Speeds)/SPDZ)

/*
**	Default daemon.
*/

VarArgs	DmnArgs	=
{
	2,
	{
	NULLSTR,	/* ==> default daemon name below */
	"-fp"		/* No fork, new lockfile */
	}
};

/*
**	Miscellaneous.
*/

jmp_buf		Alrm_jmp;
bool		Cookflag;		/* True if not RAW */
char		Errbuf[BUFSIZ];
int		Fd		= SYSERROR;
bool		Finish;			/* Wind up */
char *		Fn;			/* File name for open device == ``Fd'' */
bool		Hup;			/* SIGHUP */
char *		LinkPath;		/* Full pathname of link spool directory */
char *		LockFile;		/* Lock out other calls */
bool		LockSet;		/* For cleanup */
char		NoDevice[]	= english("No device for command \"%s\"");
int		Ondelay;		/* Set to O_NDELAY for line */
int		Monitorflag;		/* Call traces */
bool		NewLink;		/* Force signal state changes */
bool		ParamsRead;		/* Parameter file already specified by call name */
char *		Reason;			/* Result of device operations */
int		Retries;		/* Device operation retry count */
int		RetVal = EX_OK;		/* Program value */
char *		SLinkName;		/* Type striped link name */
bool		Terminated;		/* Wound up */
unsigned int	TimeOut		= 30;
char		TooManyArgs[]	= english("too many args [%d > %d] for \"%s\"");
int		UUCPlocked;		/* Set true by tty routines (maybe.) */
int		UUCPlowerdev	= UUCPLOWERDEV;
int		UUCPmlockdev	= UUCPMLOCKDEV;
int		UUCPstrpid	= UUCPSTRPID;
int		VCtype;			/* VC type (BYTESTREAM or DATAGRAM) */
bool		XonXoff;		/* True if using XON/XOFF flow control */

#if	WAIT_RESTART == 1
bool		JmpSet;			/* SetJmp active */
jmp_buf		SetJmp;			/* To get out of wait() */
#endif	/* WAIT_RESTART == 1 */

#ifdef	SIG_SETMASK
sigset_t newset;
sigset_t oldset;
#endif	/* SIG_SETMASK */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"NICEDAEMON",	(char **)&NICEDAEMON,	PINT},
	{"TRACEFLAG",	(char **)&Traceflag,	PINT},
	{"UUCPLCKDIR",	&UUCPLCKDIR,		PDIR},
	{"UUCPLCKPRE",	&UUCPLCKPRE,		PSTRING},
	{"UUCPLOCKS",	(char **)&UUCPLOCKS,	PINT},
	{"UUCPLOWERDEV",(char **)&UUCPlowerdev,	PINT},
	{"UUCPMLOCKDEV",(char **)&UUCPmlockdev,	PINT},
	{"UUCPSTRPID",	(char **)&UUCPstrpid,	PINT},
	{"VCDAEMON",	&DmnArgs.va_args[0],	PSPOOL},
};

/*
**	Routines
*/

extern SigFunc	catchhup, catchterm;

bool	check_prog _FA_((char *));
int	compare _FA_((const void *, const void *));
void	Daemon _FA_((char *));
void	message _FA_((char *, char *, int));
int	readargs _FA_((char *));
void	set_fd0 _FA_((VarArgs *));
void	set_uid _FA_((void));
void	set_up _FA_((void));
bool	typed _FA_((char *));
void	ulockalarm _FA_((int));

extern void	rmlock _FA_((char *));
extern void	ultouch _FA_((void));



int
main(argc, argv, envp)
	int	argc;
	char *	argv[];
	char *	envp[];
{
	int	status;
	Passwd	me;
	Link	link_data;

	SetNameProg(argc, argv, envp);

	InitParams();

	DoArgs(argc, argv, Usage);

	Name = newstr(Name);	/* Save copy */

	GetParams("daemon", Params, sizeof Params);

	if ( LinkName == NULLSTR || LinkDir == NULLSTR || HomeName == NULLSTR )
		Error(english("need home and/or link"));

	SLinkName = StripTypes(LinkName);
	NameProg("%s %s", Name, SLinkName);

	if ( !NoUpDown && FindLink(LinkName, &link_data) && (link_data.ln_flags & FL_NOCHANGE) )
		NoUpDown = true;

	GetNetUid();
	GetInvoker(&me);

	if ( !(me.P_flags & P_SU) )
		Error(english("No permission."));

	Pid = Detach(Background?false:true, NICEDAEMON, false, true);

	(void)signal(SIGTERM, catchterm);
	(void)signal(SIGHUP, catchhup);

#	ifdef	SIG_SETMASK
	sigemptyset(&newset);
#	endif	/* SIG_SETMASK */

	LinkPath = concat(SPOOLDIR, LinkDir, Slash, NULLSTR);
	LockFile = concat(LinkPath, LINKCMDSNAME, Slash, LOCKFILE, NULLSTR);

	if ( ARG(&DmnArgs, 0) == NULLSTR )
		ARG(&DmnArgs, 0) = concat(SPOOLDIR, LIBDIR, VCDAEMON, NULLSTR);

	if ( IgnoreOldPid )
		(void)unlink(LockFile);

	if ( !SetLock(LockFile, Pid, false) )	/* Will make directories as needed */
	{
		Warn(english("Daemon (%d) for %s already running on %s."), LockPid, LinkName, LockNode);
		(void)sleep(MINSLEEP);	/* Loop slower */
		exit(EX_OK);
	}

	LockSet = true;

	(void)setbuf(stderr, Errbuf);

	if ( chdir(LinkPath) == SYSERROR )
	{
		Syserror(CouldNot, "chdir", LinkPath);
		finish(EX_OSFILE);
	}

	Recover(ert_return);	/* Error() returns */

	Monitorflag = Traceflag;

	InitInterp();

	if ( CallScript.name == NULLSTR )
		CallScript.name = CALLFILE;

	if ( (CallScript.head = MakeCall(CallScript.name)) == (Op *)0 )
		finish(EX_DATAERR);

	switch ( status = interp(CallScript.head, CallScript.name) )
	{
	case ERROR:	Error(english("error exit"));
			status = EX_UNAVAILABLE;	break;
	case UNDEF:	status = EX_SOFTWARE;		break;
	case OK:	status = RetVal;		break;
	}

	finish(status);
	return 0;
}



void
alrmcatch(sig)
	int	sig;
{
	(void)signal(sig, alrmcatch);
	(void)longjmp(Alrm_jmp, 1);
}



void
catchhup(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	Hup = true;
}



void
catchterm(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	Finish = true;
#	if	WAIT_RESTART == 1
	Trace3(1, "catchterm(%d) JmpSet=%d", sig, JmpSet);
	if ( JmpSet )
		longjmp(SetJmp, 1);
#	endif	/* WAIT_RESTART == 1 */
}



/*
**	Check legal program.
*/

bool
check_prog(prog)
	char *		prog;
{
	struct stat	statb;

	while ( stat(prog, &statb) == SYSERROR )
	{
		Reason = newprintf(CouldNot, StatStr, prog);
		Syserror(Reason);
	}

	if ( !(statb.st_mode & 0110) )
		Reason = newprintf(english("No execute permission for \"%s\""), prog);
	else
	if ( statb.st_uid != NetUid || statb.st_gid != NetGid )
		Reason = newprintf(english("No permission to run \"%s\""), prog);
	else
	if ( statb.st_mode & 02 )
		Reason = newprintf(english("\"%s\" is writable by all"), prog);
	else
		return true;

	RetVal = EX_NOPERM;
	Error(Reason);
	return false;
}



#ifdef	NEED_CLEANUP
void
cleanup()
{
	if ( UUCPlocked )
	{
		rmlock(NULLSTR);
		UUCPlocked = 0;
	}
}
#endif	/* NEED_CLEANUP */



int
#ifdef	ANSI_C
compare(const void *p1, const void *p2)
#else	/* ANSI_C */
compare(p1, p2)
	char *	p1;
	char *	p2;
#endif	/* ANSI_C */
{
	return strcmp(((Speed *)p1)->s_name, ((Speed *)p2)->s_name);
}



void
Daemon(args)
	char *		args;
{
	register int	i;
	char *		cp;

	if ( !check_prog(ARG(&DmnArgs, 0)) )
		return;

	(void)Shell(CALLPROG, SLinkName, ARG(&DmnArgs, 0), Fd);

	(void)signal(SIGPIPE, SIG_IGN);

	if ( NoUpDown && !NewLink )
		NEXTARG(&DmnArgs) = "-u";	/* No state changes */

	NEXTARG(&DmnArgs) = concat("-H", HomeName, NULLSTR);
	NEXTARG(&DmnArgs) = concat("-L", LinkName, NULLSTR);
	NEXTARG(&DmnArgs) = concat("-N", LinkDir, NULLSTR);

	if ( (cp = ConnectArg) != NULLSTR || (cp = Fn) != NULLSTR )
	{
		if ( strncmp(cp, DevNull, 5) == STREQUAL )
			cp += 5;
		NEXTARG(&DmnArgs) = concat("-K", cp, NULLSTR);
	}

	i = NARGS(&DmnArgs);

	if ( args != NULLSTR && *args != '\0' )
		i = SplitArg(&DmnArgs, args);

	if ( !ParamsRead )
		i = readargs(PARAMSFILE);

	if ( i > MAXVARARGS )
	{
		Reason = newprintf(TooManyArgs, i, MAXVARARGS, ARG(&DmnArgs, 0));
		Error(Reason);
		return;
	}

	ARG(&DmnArgs, i) = NULLSTR;

	if ( Fd != 0 )
	{
		(void)close(0);
		(void)dup(Fd);
	}

	if ( Fd != 1 )
	{
		(void)close(1);
		(void)dup(Fd);
	}

	for ( i = 3 ; close(i) != SYSERROR || i < 9 ; i++ );

	set_uid();

	cp = ARG(&DmnArgs, 0);
	Trace4(1, "execve(%s, %s, %s)", cp, ARG(&DmnArgs, 0), *StripEnv());
	(void)execve(cp, &ARG(&DmnArgs, 0), StripEnv());
	Trace3(1, "execve(%s, ...) returns %d", cp, errno);
	if ( errno == ENOEXEC )
		ExShell(cp, &DmnArgs);
	Reason = newprintf(CouldNot, "execve", cp);
	Syserror(Reason);
}



/*ARGSUSED*/
AFuncv
DefSymbol(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	v;

	if ( (v = strchr(val.p, '=')) == NULLSTR )
		return (AFuncv)english("illegal symbol definition");

	*v++ = '\0';

	(void)InstallId(val.p, newstr(v));

	return ACCEPTARG;
}



int
do_daemon(cp)
	char *	cp;
{
	if ( cp == NULLSTR || cp[0] == '\0' )
	{
		Error(english("daemon name?"));
		return ERROR;
	}

	Trace2(1, "do_daemon(%s)", cp);

	if ( cp[0] != '/' )
	{
		if ( strchr(cp, '/') == NULLSTR )
			ARG(&DmnArgs, 0) = concat(SPOOLDIR, LIBDIR, cp, NULLSTR);
		else
			ARG(&DmnArgs, 0) = concat(SPOOLDIR, cp, NULLSTR);
	}
	else
		ARG(&DmnArgs, 0) = newstr(cp);

	return OK;
}



int
do_execd(cp)
	char *	cp;
{
	if ( Fd == SYSERROR )
	{
		Error(NoDevice, english("execdaemon"));
		return ERROR;
	}

	if ( UUCPlocked )
	{
		Error(english("Can't \"execdaemon\" with locked device"));
		return ERROR;
	}

	Trace2(1, "do_execd(%s)", (cp==NULLSTR)?EmptyString:cp);

	StderrLog(concat(SPOOLDIR, CALLDIR, LOGFILE, NULLSTR));
	MesgN(english("started"), LinkName);

	Daemon(cp);

	RetVal = EX_OSERR;
	return ERROR;
}



/*
**	"forkcommand" arg ...
*/

int
do_forkc(cp)
	char *		cp;
{
	int		i;
	VarArgs		args;

	Trace2(1, "do_forkc(%s)", cp);

	for ( ;; )
	{
		static int	pid;		/* Protect from a broken longjmp() */
		int		xpid;
		int		status;

		switch ( pid = fork() )
		{
		default:
			LockSet = false;	/* Daemon now has it */

			Trace2(1, "forkcommand %d", pid);

#			ifndef	UUCPLOCKLIB
			if ( UUCPlocked )
				ulockalarm(SIGALRM);
#			endif	/* UUCPLOCKLIB */

#			if	WAIT_RESTART == 1
			if ( setjmp(SetJmp) )
				(void)kill(pid, SIGTERM);
			else
				JmpSet = true;
#			endif	/* WAIT_RESTART == 1 */

#			ifdef	SIG_SETMASK
			(void)sigprocmask(SIG_SETMASK, &newset, &oldset);
#			endif	/* SIG_SETMASK */

			while
			(
				(xpid = wait(&status)) != pid
				&&
				(xpid != SYSERROR || errno == EINTR)
			)
				if ( Finish )
					(void)kill(pid, SIGTERM);

#			if	WAIT_RESTART == 1
			JmpSet = false;
#			endif	/* WAIT_RESTART == 1 */

			if ( xpid == SYSERROR )
				Syserror("wait(2)");

			Trace2(1, "forkcommand returns", pid);

#			ifndef	UUCPLOCKLIB
			if ( UUCPlocked )
				(void)alarm((unsigned)0);
#			endif	/* UUCPLOCKLIB */

			if ( status )
			{
				ExStatus = ((status >> 8) & 0xff) | ((status & 0xff) << 8);
				if ( ExStatus & 0377 )
					RetVal = ExStatus;
				else
					RetVal = EX_SOFTWARE;
				Reason = GetErrFile(&DmnArgs, status, SYSERROR);
				Warn(Reason);
			}

			if ( !SetLock(LockFile, Pid, false) )	/* Re-establish lock */
				return ERROR;

			LockSet = true;

			return OK;

		case SYSERROR:
			Syserror(english("Can't fork"));
			continue;

		case 0:
			Pid = getpid();
		}
		break;
	}

	if ( (i = SplitArg(&args, cp)) > MAXVARARGS )
	{
		Error(TooManyArgs, i, MAXVARARGS, ARG(&args, 0));
		_exit(EX_DATAERR);
	}

	ARG(&args, i) = NULLSTR;

	if ( !check_prog(ARG(&args, 0)) )
		_exit(RetVal);

	if ( Fd == SYSERROR && (Fd = open(DevNull, O_RDWR)) == SYSERROR )
		Fd = 2;

	if ( Fd != 0 )
	{
		(void)close(0);
		(void)dup(Fd);
	}

	if ( Fd != 1 )
	{
		(void)close(1);
		(void)dup(Fd);
	}

	for ( i = 3 ; close(i) != SYSERROR || i < 9 ; i++ );

	set_uid();

	cp = ARG(&args, 0);
	(void)execve(cp, &ARG(&args, 0), StripEnv());
	if ( errno == ENOEXEC )
		ExShell(cp, &args);
	Syserror(CouldNot, "execve", cp);

	_exit(EX_OSERR);
	return ERROR;
}



int
do_forkd(cp)
	char *	cp;
{
	if ( Fd == SYSERROR )
	{
		Error(NoDevice, english("forkdaemon"));
		return ERROR;
	}

	Trace2(1, "do_forkd(%s)", (cp==NULLSTR)?EmptyString:cp);

	if ( !UUCPlocked )
		set_uid();			/* Might as well be daemon from here on */

	for ( ;; )
	{
		static int	pid;		/* Protect from a broken longjmp() */
		int		xpid;
		int		status;

		switch ( pid = fork() )
		{
		default:
			LockSet = false;	/* Daemon now has it */

			Trace2(1, "forkdaemon %d", pid);

#			ifndef	UUCPLOCKLIB
			if ( UUCPlocked )
				ulockalarm(SIGALRM);
#			endif	/* UUCPLOCKLIB */

#			if	WAIT_RESTART == 1
			if ( setjmp(SetJmp) )
				(void)kill(pid, SIGTERM);
			else
				JmpSet = true;
#			endif	/* WAIT_RESTART == 1 */

#			ifdef	SIG_SETMASK
			(void)sigprocmask(SIG_SETMASK, &newset, &oldset);
#			endif	/* SIG_SETMASK */

			while
			(
				(xpid = wait(&status)) != pid
				&&
				(xpid != SYSERROR || errno == EINTR)
			)
				if ( Finish )
					(void)kill(pid, SIGTERM);

#			if	WAIT_RESTART == 1
			JmpSet = false;
#			endif	/* WAIT_RESTART == 1 */

			if ( xpid == SYSERROR )
				Syserror("wait(2)");

			Trace2(1, "forkdaemon returns", pid);

#			ifndef	UUCPLOCKLIB
			if ( UUCPlocked )
				(void)alarm((unsigned)0);
#			endif	/* UUCPLOCKLIB */

			if ( status )
			{
				ExStatus = ((status >> 8) & 0xff) | ((status & 0xff) << 8);
				if ( ExStatus & 0377 )
					RetVal = ExStatus;
				else
					RetVal = EX_SOFTWARE;
				Reason = GetErrFile(&DmnArgs, status, SYSERROR);
				Warn(Reason);
			}

			if ( !SetLock(LockFile, Pid, false) )	/* Re-establish lock */
				return ERROR;

			LockSet = true;

			return OK;

		case SYSERROR:
			Syserror(english("Can't fork"));
			continue;

		case 0:
			Pid = getpid();
		}
		break;
	}

	Daemon(cp);

	_exit(EX_OSERR);
	return ERROR;
}



int
do_mode(cp)
	char *	cp;
{
	if ( cp == NULLSTR )
	{
		Error(english("daemon mode?"));
		return ERROR;
	}

	Trace2(1, "do_mode(%s)", (cp==NULLSTR)?EmptyString:cp);

	if ( SplitArg(&DmnArgs, cp) > MAXVARARGS )
	{
		Error(english("too many mode params"));
		return ERROR;
	}

	return OK;
}



/*
**	Run a shell as invoker.
*/

char *
do_shell(cp)
	register char *	cp;
{
	char *		errs;
	ExBuf		va;
	static char	retval[ULONG_SIZE];
	static char	retfmt[] = "%d";

	Trace2(1, "do_shell(%s)", (cp==NULLSTR)?EmptyString:cp);

	if ( cp == NULLSTR || *cp == '\0' )
	{
		Error(english("No args for shell"));
		(void)sprintf(retval, retfmt, EX_USAGE);
		return retval;
	}

	FIRSTARG(&va.ex_cmd) = ShellStr;
	NEXTARG(&va.ex_cmd) = "-c";
	NEXTARG(&va.ex_cmd) = cp;

	if ( (errs = Execute(&va, set_fd0, ex_exec, Fd)) != NULLSTR )
	{
		if ( (cp = errs)[0] == '\0' )
			cp = english("shell error");
		Warn(StringFmt, cp);
		free(errs);
	}

	(void)sprintf(retval, retfmt, ExStatus);
	return retval;
}



int
do_slowwrite(string)
	char *		string;
{
	register char *	cp;
	register int	r;
	register int	n;
	int		savtrc;
	char		data[2];

#	if	SYSTEM > 0 && linux == 0
	register Ulong	s;
	struct tms	tbuf;

	s = (Ulong)times(&tbuf);		/* Time since boot in HZ */
#	endif	/* SYSTEM > 0 && linux == 0 */

#	if	BSD4 >= 2 || linux == 1
	fd_set		zero;
	struct timeval	timeout;

	FD_ZERO(&zero);
	timeout.tv_sec = 0;
	timeout.tv_usec = 66667;		/* Microsecs to give 15 chars/sec. */
#	endif	/* BSD4 >= 2 || linux == 1 */

	Trace2(1, "do_slowwrite(%s)", string);

	data[1] = '\0';
	n = 0;
	cp = string;

	if ( (savtrc = Monitorflag) > 0 )
		Monitorflag = 0;

	while ( (data[0] = cp[0]) != '\0' )
	{
		if ( (r = do_write(data)) != OK )
			return r;

		n++;
		cp++;

		if ( cp[0] == '\0' )
			break;

#		if	SYSTEM > 0 && linux == 0
		s += 4;				/* ~15 chars/sec. */
		while ( (Ulong)times(&tbuf) < s );	/* Very CPU intensive! */
#		endif	/* SYSTEM > 0 && linux == 0 */

#		if	BSD4 >= 2 || linux == 1
		(void)select(32, &zero, &zero, &zero, &timeout);
#		endif	/* BSD4 >= 2 || linux == 1 */
	}

	if ( (Monitorflag = savtrc) > 0 )
		message(english("slowwrite"), string, n);

	return OK;
}



int
do_speed(cp)
	char *		cp;
{
	Speed *		spdp;
	Speed		speed;

	if ( Fd == SYSERROR )
	{
		Error(NoDevice, english("speed"));
		errno = ENXIO;
		return SYSERROR;
	}

	speed.s_name = cp;

	if
	(
		(spdp = (Speed *)bsearch((char *)&speed, (char *)Speeds, NSPDS, SPDZ, compare))
		==
		(Speed *)0
	)
	{
		Error(english("unrecognised speed \"%s\""), cp);
		errno = ENXIO;
		return SYSERROR;
	}

	return SetRaw(Fd, spdp->s_val, 1, 0, XonXoff);
}



char *
do_stty(cp)
	char *	cp;
{
	ExBuf	va;
	char *	errs;

	if ( Fd == SYSERROR )
	{
		Error(NoDevice, english("stty"));
		return DEVFAIL;
	}

	if ( cp == NULLSTR || *cp == '\0' )
	{
		Error(english("No args for stty"));
		return DEVFAIL;
	}

	Trace2(1, "do_stty(%s)", (cp==NULLSTR)?EmptyString:cp);

	FIRSTARG(&va.ex_cmd) = STTY;

	if ( SplitArg(&va.ex_cmd, cp) > MAXVARARGS )
	{
		Error(english("Too many args for stty"));
		return DEVFAIL;
	}

	if ( (errs = Execute(&va, set_fd0, ex_exec, Fd)) != NULLSTR )
	{
		Warn(StringFmt, errs);	/* "stty" usually fails! */
		free(errs);
		return DEVFAIL;
	}

	return DEVOK;
}



#if	XTI == 1
/*
**	Write to XTI file descriptor.
**	Writing to an XTI circuit requires the addition of a leading status byte
*/

static int
xti_write(fd, data, size)
	int		fd;
	char *		data;
	int		size;
{
	char		* buf;
	register int	n;

	/* This isn't a terribly efficient way of doing this - the malloc in
	** particular generates huge overheads - but the alternative isn't
	** much better. (The alternative is to generate a buffer on the
	** stack, but we can't rely on any particular maximum size.)
	*/

	Trace4(8, "xti_write(%d, %x, %d)", fd, data, size);
	buf = Malloc(size + 1);
	bcopy(data, buf+1, size);
	buf[0] = 0;		/* Assume default status of 0. */

	n = t_snd(fd, buf, size+1, 0);
				/* Send data but don't return until we... */

	free(buf);		/* ... free up the buffer. */
	return n>1 ? n-1 : n;
}
#endif	/* XTI == 1 */



int
do_write(cp)
	register char *	cp;
{
	register int	n;
	register int	r;
	register int	c;

	if ( Fd == SYSERROR )
	{
		Error(NoDevice, english("write"));
		return ERROR;
	}

	if ( cp == NULLSTR || (n = strlen(cp)) == 0 )
	{
		Error(english("attempt to write null string"));
		return ERROR;
	}

	Trace2(5, "do_write(%s)", ExpandString(cp, n));

	c = n;	/* Loop count */

#	if	XTI == 1
	while ( xti_connect ? ((r = xti_write(Fd, cp, n)) != n )
	                    : ((r = write(Fd, cp, n)) != n ) )
#	else	/* XTI == 1 */
	while ( (r = write(Fd, cp, n)) != n )
#	endif	/* XTI == 1 */
	{
		if ( r == 0 )
		{
			if ( --c >= 0 )
				continue;
			r = SYSERROR;
			errno = EIO;
		}

		if ( r < 0 )
		{
			Syserror(CouldNot, WriteStr, Fn);
			continue;
		}

		if ( Monitorflag )
			message(english("write"), cp, r);

		cp += r;
		n -= r;
	}

	if ( Monitorflag )
		message(english("write"), cp, n);

	return OK;
}



void
finish(reason)
	int		reason;
{
	static bool	finishing;

	if ( finishing )
	{
		exit(reason);
		return;
	}

	finishing = true;

	if ( RetVal != EX_OK )
		reason = RetVal;

	if ( reason == ERROR )
		reason = EX_UNAVAILABLE;

	if ( !ExInChild )
	{
		if ( UUCPlocked )
		{
			rmlock(NULLSTR);
			UUCPlocked = 0;
		}

		if ( LockSet )
			(void)unlink(LockFile);

		if ( reason != EX_USAGE )
		{
			StderrLog(concat(SPOOLDIR, CALLDIR, LOGFILE, NULLSTR));

			if ( reason == EX_OK )
				MesgN(english("succeeded"), LinkName);
			else
			{
				char *	cp;
				int	len;

				if ( Reason == NULLSTR )
					Reason = Unknown;
				else
				if ( (len = strlen(Reason)) <= 1 )
					Reason = Unknown;
				else
				{
					cp = &Reason[len];

					while ( (len = *--cp) == '\n' || len == ' ' || len == '\t' )
						;

					*++cp = '\0';

					if ( (cp - Reason) <= 1 )
						Reason = Unknown;
				}

				MesgN(english("FAILED"), "%s: %s", LinkName, Reason);
			}
		}
	}

	exit(reason);
}



/*
**	Find name of explicit link.
*/

/*ARGSUSED*/
AFuncv
getLink(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Addr *		ap;
	char *		cp;
	char *		link;
	Link		link_data;

	if ( val.p[0] == '\0' )
		return ACCEPTARG;	/* Null string arg can be ignored */

	if ( LinkName != NULLSTR )
		return (AFuncv)english("only one <link> can be specified");

	if ( LinkDir != NULLSTR )
		return (AFuncv)english("can't specify <link> as well as flag argument -L");

	if ( !ReadRoute(NOSUMCHECK) )
		return (AFuncv)english("can't read routing tables");

	if ( Testing )
	{
		LinkName = newstr(val.p);
		LinkDir = LinkName;
		return ACCEPTARG;
	}

	for ( ;; )
	{
		ap = StringToAddr(cp = newstr(val.p), false);

		link = TypedName(ap);

		if ( FindLink(link, &link_data) )
			break;

		LinkName = link;

		if ( link[0] != '9' || AddrAtHome(&ap, false, false) || !typed(link) )
			return (AFuncv)english("nonexistent link");

		NewLink = true;

		link_data.ln_rname = newstr(link);
		link_data.ln_name = DomainToPath(link);

		break;
		/* NOTREACHED */
	}

	FreeAddr(&ap);
	free(cp);

	LinkDir = link_data.ln_name;
	LinkName = link_data.ln_rname;

	return ACCEPTARG;
}



/*ARGSUSED*/
AFuncv
getLogFile(val, arg)
	PassVal		val;
	Pointer		arg;
{
	StderrLog(val.p);
	return ACCEPTARG;
}



/*
**	Copy new string to target.
*/

/*ARGSUSED*/
AFuncv
getStr(val, arg)
	PassVal		val;
	Pointer		arg;
{
	*(char **)arg = newstr(val.p);
	return ACCEPTARG;
}



#if	XTI == 1
/*
** xti_read reads data from an XTI pipe and strips the leading status byte.
** Trying to read a buffer of the specified size gives bad headers, so
** we'll read size+1 bytes instead...
*/

static int
xti_read(fd, data, size)
	int		fd;
	char *		data;
	int		size;
{
	int	n;
	static int	t_localflags = 0;	/* Don't use any flags. */
	char *		buf;

	buf = Malloc(size+1);
	n = t_rcv(fd, buf, size+1, &t_localflags);
	/* t_rcv (and read() from an XTI pipe) prepends a status byte to
	** the data read. We'll remove that data by using bcopy() to move
	** the data back by a byte and returning one less than the actual
	** data read.
	*/
	bcopy(buf+1, data, n>1 ? n : size);
	free(buf);

	return n>0 ? n-1 : n;
}
#endif	/* XTI == 1 */



void
getinput(delimchars)
	char *		delimchars;
{
	register int	i;

	Trace4(1, "getinput(%s), TimeOut=%d, Fd=%d", ExpandString(delimchars, -1), TimeOut, Fd);

	if ( setjmp(Alrm_jmp) )
	{
		if ( Finish )
		{
			Terminated = true;
			(void)strcpy(input, TERMINATE);
		}
		else
			(void)strcpy(input, TIMEOUT);

		i = strlen(input);
		goto out2;
	}

	(void)signal(SIGALRM, alrmcatch);
	(void)alarm(TimeOut);

	if ( VCtype == DATAGRAM )
	{
		register char	c;
		register int	j;

#		if	XTI == 1
		if ( (i = xti_connect ? xti_read(Fd, input, MAX_LINE_LEN-1)
				      : read(Fd, input, MAX_LINE_LEN-1)) <= 0
		     || (Finish && !Terminated) )
#		else	/* XTI == 1 */
		if ( (i = read(Fd, input, MAX_LINE_LEN-1)) <= 0 || (Finish && !Terminated) )
#		endif	/* XTI == 1 */
		{
			if ( Finish )
			{
				Terminated = true;
				(void)strcpy(input, TERMINATE);
			}
			else
			{
				if ( Hup )
					Reason = "HANGUP";
				else
				if ( i == 0 )
					Reason = UNDEFINED;
				else
				{
					Reason = ErrnoStr(errno);
					Trace3(1, "read error \"%s\" [%d]", Reason, errno);
				}

				(void)strcpy(input, EOFSTR);
			}

			i = strlen(input);
			goto out;
		}

		for ( j = 0 ; j < i ; j++ )
		{
			if ( (c = input[j] & 0x7f) == 0 )
				input[j] = ' ';
			else
			if ( c == '\r' )
				input[j] = '\n';
			else
				input[j] = c;
		}
	}
	else
	for ( i = 0 ; i < MAX_LINE_LEN ; )
	{
		register int	n;
		char		c;

		if ( (n = read(Fd, &c, 1)) <= 0 || (Finish && !Terminated) )
		{
			if ( Finish )
			{
				Terminated = true;
				(void)strcpy(input, TERMINATE);
			}
			else
			{
				if ( Hup )
					Reason = "HANGUP";
				else
				if ( n == 0 )
				{
					if ( Ondelay )
					{
						if ( (n = alarm((unsigned)0)) > 0 && n < (MINSLEEP*2) )
							longjmp(Alrm_jmp, 1);
						(void)sleep(MINSLEEP);
						if ( (n -= MINSLEEP) >= MINSLEEP )
							(void)alarm((unsigned)n);
						continue;
					}
					Reason = UNDEFINED;
				}
				else
				{
					Reason = ErrnoStr(errno);
					Trace3(1, "read error \"%s\" [%d]", Reason, errno);
				}

				(void)strcpy(input, EOFSTR);
			}

			i = strlen(input);
			goto out;
		}

		Trace2(5, "got <%03o>", c&0xff);

		if ( (c &= 0x7f) == 0 )
			continue;

		if ( c == '\r' )
			c = '\n';

		input[i++] = c;

		if ( delimchars == NULLSTR || strchr(delimchars, c) )
			break;
	}

	input[i] = '\0';
out:
	(void)alarm((unsigned)0);
out2:
	if ( Monitorflag >= 2 )
		message(english("read "), input, i);
}



#ifndef	UUCPLOCKLIB
void
ulockalarm(sig)
	int	sig;
{
	(void)signal(sig, ulockalarm);
	(void)alarm((unsigned)1800);
	ultouch();
}
#endif	/* UUCPLOCKLIB */



/*
**	Visible string on stderr.
*/

void
message(s, buf, n)
	char *	s;
	char *	buf;
	int	n;
{
	if ( buf[n-1] == '\n' )
		n--;

	if ( n <= 0 )
	{
		if ( Monitorflag > 3 )
			MesgN(s, NULLSTR);
		return;
	}

	MesgN(s, "%s", ExpandString(buf, n));
}



/*
**	Extract arguments from passed file,
**	and set them up as arguments to the daemon.
*/

int
readargs(file)
	char *		file;
{
	register char *	ap;
	register int	n;

	Trace2(1, "readargs(%s)", file);

	if ( (ap = ReadFile(file)) == NULLSTR )
		return NARGS(&DmnArgs);

	if ( (n = SplitArg(&DmnArgs, ap)) > MAXVARARGS )
	{
		Error("Too many arguments in \"%s\"", file);
		finish(EX_SOFTWARE);
	}

	free(ap);

	ParamsRead = true;

	return n;
}



void
set_fd0(vap)
	VarArgs *	vap;
{
	(void)close(0);
	(void)dup(1);	/* == Fd */

	set_up();
}



void
set_uid()
{
	int	uid;
	int	gid;

	gid = getgid();

	if ( (uid = getuid()) == 0 )
	{
		gid = NetGid;
		uid = NetUid;
	}

	SetUser(uid, gid);
}



void
set_up()
{
	register int	i;

#	ifndef	QNX
	if ( NICEDAEMON < 0 && geteuid() == 0 )
		(void)nice(-(NICEDAEMON));	/* Back to 0 */
#	endif	/* QNX */

	for ( i = 3 ; close(i) != SYSERROR || i < 9 ; i++ );

	set_uid();
}



/*
**	Check all domains are typed.
*/

bool
typed(link)
	char *	link;
{
	char *	np;
	bool	val = true;

	while ( val )
	{
		if ( (np = strchr(link, DOMAIN_SEP)) != NULLSTR )
			*np = '\0';
		if ( strchr(link, TYPE_SEP) == NULLSTR )
			val = false;
		if ( np == NULLSTR )
			break;
		*np++ = DOMAIN_SEP;
		link = np;
	}

	return val;
}
