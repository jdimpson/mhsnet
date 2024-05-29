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


static char	sccsid[]	= "@(#)main.c	1.33 05/12/16";

/*
**	Startup for node-node daemon.
*/

#define	FILE_CONTROL
#define	LOCKING
#define	RECOVER
#define	SIGNALS
#define	STDIO
#define	TIME
#define	ERRNO

#include	"global.h"
#include	"Args.h"
#include	"debug.h"
#include	"handlers.h"
#include	"params.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"stats.h"
#include	"sysexits.h"

#include	"Debug.h"
#include	"Stream.h"
#include	"driver.h"
#undef	Extern
#define	Extern
#define	ExternU
#include	"daemon.h"


#ifndef	DMNPROG
#define	DMNPROG		"endprog"		/* Name of program to be run on termination */
#endif	/* DMNPROG */

/*
**	Arguments (others in "daemon.h").
*/

char *	ConnectArg	= EmptyString;		/* Optional string to be appended to CONNECTLOG */
int	Idle_Timeout;				/* (Ignored) */
bool	IgnoreOldPid;				/* Ignore lock file pid */
bool	InOnly;					/* Start inbound messsages only */
bool	LockSet;				/* Lock file already set by invoker*/
char *	Name		= sccsid;		/* Program invoked name */
int	Nbufs		= 3;			/* Window size */
bool	NoFork;					/* Don't fork! (used by NNshell) */
bool	NoLog;					/* Don't create "log" */
bool	NotSun3;				/* Not SUNIII connection */
bool	OldDaemon;				/* (Ignored) */
bool	OutOnly;				/* Start outbound messsages only */
int	Ptraceflag;				/* Global packet tracing control */
int	PktTraceflag;				/* Global packet tracing control */
char	Quality		= LOWEST_QUALITY;	/* Maximum quality selected to transmit */
int	SleepTime;				/* Delay between outbound queue scans */
long	Speed		= 960/4;		/* Minimum effective bytes per second for link */
bool	Stats		= true;			/* True if statistics logging required */
int	Traceflag;				/* Global tracing control */
bool	TurnAround;				/* Half-duplex mode */
int	UpdateRate	= UPDATE_TIME;		/* Status file update frequency */
bool	UseCrc		= true;			/* Use CRC error detection */

AFuncv	getBatch _FA_((PassVal, Pointer));	/* Select batch mode */
AFuncv	getCCook _FA_((PassVal, Pointer));	/* Select cooking */
AFuncv	getNotbool _FA_((PassVal, Pointer));	/* Invert boolean */
AFuncv	getQuality _FA_((PassVal, Pointer));	/* Message priority */
AFuncv	getTraceFile _FA_((PassVal, Pointer));	/* File for Trace/Error output */

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(3, &NotSun3, 0),
	Arg_bool(c, &Cook, 0),
	Arg_bool(d, &DieOnDown, 0),
	Arg_bool(f, &NoFork, 0),
	Arg_bool(g, &NoLog, 0),
	Arg_bool(i, &InOnly, 0),
	Arg_bool(l, &LockSet, 0),
	Arg_bool(n, &NoAdjust, 0),
	Arg_bool(o, &OutOnly, 0),
	Arg_bool(p, &IgnoreOldPid, 0),
	Arg_bool(r, &UseCrc, getNotbool),
	Arg_bool(s, &Stats, getNotbool),
	Arg_bool(t, &HalfDuplex, 0),
	Arg_bool(u, &NoUpDown, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_bool(z, &OldDaemon, 0),
	Arg_long(B, 0, getBatch, english("batch run-on time"), OPTARG|OPTVAL),
	Arg_string(C, 0, getCCook, english("cook protocol"), OPTARG),
	Arg_int(D, &PktZ, 0, english("output data packet data size"), OPTARG),
	Arg_string(E, &ErrorFd, getTraceFile, english("error file"), OPTARG),
	Arg_string(F, &TraceFd, getTraceFile, english("trace file"), OPTARG),
	Arg_int(G, &Nbufs, getInt1, english("packet buffers"), OPTARG),
	Arg_string(H, &HomeNode, 0, english("home address"), 0),
	Arg_int(I, &IntraPktDelay, 0, english("intra-packet delay"), OPTARG),
	Arg_int(J, &Fstream, 0, english("first channel"), OPTARG),
	Arg_string(K, &ConnectArg, 0, english("acct string"), OPTARG),
	Arg_string(L, &LinkNode, 0, english("link address"), 0),
	Arg_string(N, &LinkDir, 0, english("link directory"), 0),
	Arg_int(O, &BufferOutput, 0, english("output size"), OPTARG),
	Arg_int(P, &Ptraceflag, getInt1, english("packet trace level"), OPTARG|OPTVAL),
	Arg_char(Q, &Quality, getQuality, english("priority"), OPTARG),
	Arg_long(R, &MaxRunTime, 0, english("max run time"), OPTARG),
	Arg_long(S, &MinSpeed, 0, english("minimum bytes/sec."), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("trace level"), OPTARG|OPTVAL),
	Arg_int(U, &UpdateRate, getInt1, english("status update rate"), OPTARG),
	Arg_int(W, &HoldAcks, 0, english("ACK window"), OPTARG),
	Arg_long(X, &Speed, 0, english("nominal bytes/sec."), OPTARG),
	Arg_int(Y, &Idle_Timeout, 0, english("idle timeout"), OPTARG),
	Arg_int(Z, &SleepTime, 0, english("queue scan rate"), OPTARG),
	Arg_igndups,				/* Allow repeat specifications */
	Arg_end
};

/*
**	Filenames that must be set up
*/

char *	BadHandler;				/* Name of bad message handler process */
char	ChdirStr[]	= "chdir";
char *	CmdsName	= LINKCMDSNAME;
char *	LockFile;
char *	Statusfile	= "status";		/* File name for state info. */
char	Template[STREAMIDZ+1];			/* Last component of node-node command file name */

/*
**	Miscellaneous
*/

bool	Pausing;
bool	Starting	= true;
char *	FinishReason	= english("FINISHED");

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"NICEDAEMON",	(char **)&NICEDAEMON,	PINT},
	{"TRACEFLAG",	(char **)&Traceflag,	PINT},
};

void	mesg _FA_((char *));
void	stats _FA_((char *));

extern void	Pprintstats _FA_((FILE *));

extern SigFunc	terminate;

DODEBUG(extern void InitPtrace(void));
extern int	ReadState(void);



/*
**	Argument processing and setup.
*/

int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	char *	cp;

	InitParams();
#	if	DEBUG >= 1
	InitPtrace();
#	endif	/* DEBUG */

	DoArgs(argc, argv, Usage);

	GetParams("daemon", Params, sizeof Params);

	Pid = Detach(NoFork, NICEDAEMON, true, false);	/* Sets "NetUid" */

	if ( geteuid() != NetUid )
		Error(english("No permission."));

	while ( chdir(SPOOLDIR) == SYSERROR )
		Syserror(CouldNot, ChdirStr, SPOOLDIR);

	while ( chdir(LinkDir) == SYSERROR )
	{
		cp = concat(LinkDir, Slash, NULLSTR);
		if ( !CheckDirs(cp) )
			Syserror(CouldNot, ChdirStr, LinkDir);
		free(cp);
	}

	if ( MaxRunTime > 0 )
		MaxRunTime += Time;

	if ( Cook && BufferOutput > 1 )		/* One byte smaller for \r */
		BufferOutput--;

	if ( Fstream >= NSTREAMS )
		Fstream = NSTREAMS-1;

	PktTraceflag = Ptraceflag;		/* Compatible with VCdaemon */

	LockFile = concat(CmdsName, Slash, LOCKFILE, NULLSTR);

	if ( !LockSet )
	{
		if
		(
			!SetLock(LockFile, Pid, false)
			&&
			(
				!IgnoreOldPid
				||
				!ReSetLock(LockFile, Pid, LockPid)
			)
		)
		{
			Warn(english("Daemon (%d) for %s already running on %s."), LockPid, LinkNode, LockNode);
			(void)sleep(10);	/* Loop slowly */
			(void)_exit(EX_OK);
		}

		LockSet = true;
	}

	if ( !NoLog )
		StderrLog(LOGFILE);

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);

	RouterDir = concat(SPOOLDIR, ROUTINGDIR, NULLSTR);
	CmdsRouteFile = concat(RouterDir, Template, NULLSTR);
	CmdsTempFile = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	BadHandler = concat(SPOOLDIR, LIBDIR, BADHANDLER, NULLSTR);
	WorkDir = concat(SPOOLDIR, LinkDir, Slash, LINKMSGSINNAME, Slash, NULLSTR);

	/*
	**	Call ReadState() to initialise the statefile.
	*/

	(void)ReadState();

	NNstate.procid = Pid;
	LastTime = Time;
	NNstate.starttime = Time;
	NNstate.lasttime = Time;
	NNstate.activetime = 0;

	(void)signal(SIGTERM, terminate);
	(void)signal(SIGHUP, terminate);
	(void)signal(SIGPIPE, terminate);
	(void)signal(SIGERROR, finish);
	(void)signal(SIGWAKEUP, SIG_IGN);

	mesg("STARTED");
	if ( Verbose )
	{
		MesgN("version", Version);
		Stats = true;
	}
	if ( Stats )
		EchoArgs(argc, argv);

	if ( Cook || CookVC != (CookT *)0 )
		WriteO = RCwrite;
	else
	if ( BufferOutput )
		WriteO = Rwrite;
	else
		WriteO = write;

	Starting = false;

	driver();

	finish(EX_OK);
	return 0;
}



void
finish(error)
	int		error;
{
	static bool	finishing;

	ALARM_OFF();

	if ( finishing )
	{
		if ( !error )
			return;

		Fatal1("recursing in finish");
	}

	finishing = true;

	if ( !Starting )
	{
		extern void	Pflush();

		Pflush();

		Update(up_finish);
		stats(FinishReason);

		DODEBUG(if(error&&Traceflag)(void)kill(Pid, SIGIOT));	/* Dump a core */
	}

	if ( LockSet )
		(void)unlink(LockFile);

	if ( Starting )
		(void)sleep(10);	/* Depress bad invokation loops */

	Shell(DMNPROG, StripTypes(LinkNode), FinishReason, 1);

	(void)_exit(error);
}



/*
**	Set up batch mode.
*/

/*ARGSUSED*/
AFuncv
getBatch(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( (BatchTime = val.l) == 0 )
		BatchTime = BATCH_TIMEOUT;

	return ACCEPTARG;
}



/*
**	Set up cooking pointer.
*/

/*ARGSUSED*/
AFuncv
getCCook(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	cp;
	register int	i;

	if ( val.p[1] != '\0' || (i = val.p[0] - '0') < 0 || i >= NCookers )
	{
		if ( (cp = strchr(val.p, ',')) != NULLSTR )
			*cp = '\0';

		for ( i = 0 ; i < NCookers ; i++ )
			if ( strccmp(val.p, Cookers[i].type) == STREQUAL )
				break;

		if ( i >= NCookers )
			return ARGERROR;

		if ( cp != NULLSTR )
		{
			*cp++ = ',';

			if ( Cookers[i].setup == (cFuncp)0 )
				return (AFuncv)english("unexpected parameterisation");

			if ( (cp = (*Cookers[i].setup)(&Cookers[i], cp)) != NULLSTR )
				return (AFuncv)cp;
		}
		else
		if ( Cookers[i].setdefault != NULLVFUNCP )
			(*Cookers[i].setdefault)();
	}

	CookVC = &Cookers[i];
	return ACCEPTARG;
}



/*
**	Invert boolean.
*/

/*ARGSUSED*/
AFuncv
getNotbool(val, arg)
	PassVal		val;
	Pointer		arg;
{
	*(bool *)arg = false;
	return ACCEPTARG;
}



/*
**	Select message priorities for transmission.
*/

/*ARGSUSED*/
AFuncv
getQuality(val, arg)
	PassVal		val;
	Pointer		arg;
{
	char *		errs;

	if ( val.c < HIGHEST_QUALITY || val.c > LOWEST_QUALITY )
	{
		errs = Malloc(512);
		(void)sprintf(errs, english("priority: %c (high) to %c (low)"), HIGHEST_QUALITY, LOWEST_QUALITY);
		return (AFuncv)errs;
	}

	return ACCEPTARG;
}



/*
**	Setup file for error/trace messages.
*/

/*ARGSUSED*/
AFuncv
getTraceFile(val, arg)
	register PassVal	val;
	register Pointer	arg;
{
	register FILE *		fd	= *(FILE **)arg;

	if
	(
		strncmp(val.p, SPOOLDIR, strlen(SPOOLDIR)) != STREQUAL
		&&
		strncmp(val.p, TMPDIR, strlen(TMPDIR)) != STREQUAL
	)
		return (AFuncv)english("illegal pathname");

	(void)fflush(fd);

	while ( (fd = fopen(val.p, "a")) == NULL )
		if ( !SysWarn(CouldNot, OpenStr, val.p) )
			return ARGERROR;

	*(FILE **)arg = fd;

#	if	FCNTL == 1 && O_APPEND != 0
	(void)fcntl
	(
		fileno(fd),
		F_SETFL,
		fcntl(fileno(fd), F_GETFL, 0) | O_APPEND
	);
#	endif

	return ACCEPTARG;
}



/*
**	Print out statistics
*/

void
stats(string)
	char *			string;
{
	FILE *			fd;
	register struct tm *	tmp;
	char *			connectfile;
#	if	VCDAEMON_STATS
	Time_t			elapsed;
#	endif	/* VCDAEMON_STATS */

	mesg(string);

	if ( NNstate.starttime == 0 )
		return;

	elapsed = time((time_t *)0);

	if ( elapsed < NNstate.starttime )
		elapsed = 1;
	else
	if ( (elapsed -= NNstate.starttime) == 0 )
		elapsed = 1;

	/*
	**	Connect accounting.
	*/

	connectfile = concat(SPOOLDIR, STATSDIR, CONNECTLOG, NULLSTR);

	if
	(
		access(connectfile, 0) != SYSERROR
		&&
		(fd = fopen(connectfile, "a")) != NULL
	)
	{
		tmp = localtime(&NNstate.starttime);

		if ( tmp->tm_year >= 100 )
			tmp->tm_year -= 100;	/* Take care of anno domini 2000 */

		(void)fprintf
		(
			fd,
			"%02d/%02d/%02d %02d:%02d:%02d %d %s %s %lu %lu %lu %lu %s\n",
			tmp->tm_year, tmp->tm_mon+1, tmp->tm_mday,
			tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
			tmp->tm_wday,
			"i&o", LinkNode,
			(PUlong)elapsed, (PUlong)NNstate.allmessages, (PUlong)NNstate.allbytes,
			(PUlong)NNstate.starttime, ConnectArg
		);

		(void)fclose(fd);
	}

	if ( !Stats )
		return;

#	if	VCDAEMON_STATS
	(void)fprintf
	(
		 stderr
		,"%lu bytes in %lu messages in %lu seconds,\naverage active transfer rate: %lu bytes/second,\noverall transfer rate: %lu bytes/second.\n"
		,(PUlong)NNstate.allbytes
		,(PUlong)NNstate.allmessages
		,(PUlong)elapsed
		,(PUlong)((NNstate.activetime>0)?NNstate.allbytes/NNstate.activetime:0)
		,(PUlong)(NNstate.allbytes/elapsed)
	);
#	endif	/* VCDAEMON_STATS */
#	if	PROTO_STATS
	Pprintstats(stderr);
#	endif	/* PROTO_STATS */
#	if	VCDAEMON_STATS
	if ( BlockCount )
		(void)fprintf(stderr, "%9lu writes BLOCKED\n", (PUlong)BlockCount);
	CpuStats(stderr, elapsed);
#	endif	/* VCDAEMON_STATS */
	putc('\n', stderr);
	(void)fflush(stderr);
}



/*
**	Start/Stop messages in logfile.
*/

void
mesg(s)
	char *	s;
{
	MesgN(LinkNode, "[%d] %s", Pid, s);
}



/*
**	Catch system termination signal.
*/

void
terminate(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);

	if ( !Pausing )
	{
		if ( sig == SIGHUP )
			FinishReason = "HANGUP";
		else
			FinishReason = "TERMINATED";
		Finish = true;
		return;
	}

	Update(up_finish);
	(void)_exit(EX_OK);
}



/*
**	Close all files except 'stderr' for child process.
**
**	(Called from routines in "children.c".)
*/

void
closeall(vap)
	VarArgs *	vap;
{
	register int	i;

	if ( Alarm )
	{
		ALARM_OFF();
		Warn("Alarm set at call to closeall()");
	}

	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGTERM, SIG_IGN);

	if ( (i = fileno(stderr)) != 2 )
	{
		if ( i > 2 )
			(void)close(2);

		while ( i != 2 )
			while ( (i = dup(i)) == SYSERROR );
			{
				Syserror("Can't dup log file descriptor");
				if ( BatchMode )
					finish(SYSERROR);
			}
	}

	(void)close(0);

	while ( open(DevNull, O_RDWR) == SYSERROR )
	{
		Syserror(DevNull); /* (you wouldn't believe how silly some systems can be...) */
		if ( BatchMode )
			finish(SYSERROR);
	}

	(void)close(1);
	(void)dup(0);

	for ( i = 3 ; close(i) != SYSERROR || i < 9 ; i++ );
		/* There can be up to 9 files open */
}



/*
**	Compare two strings in reverse order to "strcmp()".
*/

int
strrcmp(s1, s2)
	register char *	s1;
	register char *	s2;
{
	register int	l1 = strlen(s1);
	register int	l2 = strlen(s2);

	s1 += l1;
	s2 += l2;

	while ( --l1 >= 0 && --l2 >= 0 )
	{
		if ( *--s1 == *--s2 )
			continue;

		return (*s1&0xff) - (*s2&0xff);
	}

	return l1 - l2;
}
