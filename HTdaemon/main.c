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


static char	sccsid[]	= "@(#)main.c	1.32 05/12/16";

/*
**	Virtual circuit daemon startup.
*/

#define	FILE_CONTROL
#define	LOCKING
#define	RECOVER
#define	SIGNALS
#define	STDIO
#define	TIME
#define	ERRNO
#define	WAIT_CALL

#include	"global.h"
#include	"Args.h"
#include	"debug.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"stats.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"driver.h"
#include	"packet.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"params.h"
#include	"pktstats.h"
#include	"status.h"



/*
**	Arguments (others in "driver.h").
*/

char *	ConnectArg	= EmptyString;		/* Optional string to be appended to CONNECTLOG */
char *	Efile;					/* Alternate error file */
char *	Ffile;					/* Alternate trace file */
int	Idle_Timeout	= IDLE_TIMEOUT;		/* IDLE timeout for dead circuit detection */
int	Ignore;					/* Ignore parameters */
bool	IgnoreOldPid;				/* Ignore lock file pid */
bool	LockSet;				/* Lock file already set by invoker*/
char *	Name		= sccsid;		/* Program invoked name */
bool	NoFork;					/* Don't fork off parent */
bool	NoLog;					/* Don't create "log" */
long	OutDataSize	= FILEBUFFERSIZE;	/* Output data packet data size */
char	Quality		= LOWEST_QUALITY;	/* Maximum quality selected to transmit */
bool	Stats		= true;			/* True if statistics logging required */
int	Traceflag;				/* Global tracing control */
int	TraceRflag;				/* Trace reader */
int	TraceWflag;				/* Trace writer */
bool	UseDataCRC	= true;			/* True if CRC on data */
int	WriteAheadSecs	= 4;			/* Seconds of message start `write-ahead' */

AFuncv	getBatch _FA_((PassVal, Pointer));	/* Turn on `batchmode' */
AFuncv	getcCook _FA_((PassVal, Pointer));	/* Default cooking */
AFuncv	getCCook _FA_((PassVal, Pointer));	/* Select cooking */
AFuncv	getNotbool _FA_((PassVal, Pointer));	/* Invert boolean */
AFuncv	getODS _FA_((PassVal, Pointer));	/* Otput data size */
AFuncv	getQuality _FA_((PassVal, Pointer));	/* Message priority */
AFuncv	getStr _FA_((PassVal, Pointer));	/* Save copy of string */

/*
**	Must save strings because using "NameProg()"
*/

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(3, &Ignore, 0),
	Arg_bool(c, 0, getcCook),
	Arg_bool(d, &DieOnDown, 0),
	Arg_bool(f, &NoFork, 0),
	Arg_bool(g, &NoLog, 0),
	Arg_bool(i, &InOnly, 0),
	Arg_bool(l, &LockSet, 0),
	Arg_bool(n, &NoAdjust, 0),
	Arg_bool(o, &OutOnly, 0),
	Arg_bool(p, &IgnoreOldPid, 0),
	Arg_bool(r, &UseDataCRC, getNotbool),
	Arg_bool(s, &Stats, getNotbool),
	Arg_bool(t, &TurnAround, 0),
	Arg_bool(u, &NoUpDown, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_bool(z, &Ignore, 0),
	Arg_int(A, &WriteAheadSecs, 0, english("write-ahead seconds"), OPTARG),
	Arg_long(B, 0, getBatch, english("batch run-on time"), OPTARG|OPTVAL),
	Arg_string(C, 0, getCCook, english("cook protocol"), OPTARG),
	Arg_long(D, &OutDataSize, getODS, english("output data packet data size"), OPTARG),
	Arg_string(E, &Efile, getStr, english("error file"), OPTARG),
	Arg_string(F, &Ffile, getStr, english("trace file"), OPTARG),
	Arg_int(G, &Ignore, getInt1, english("packet buffers"), OPTARG),
	Arg_string(H, &HomeAddress, getStr, english("home address"), 0),
	Arg_int(I, &Ignore, 0, english("intra-packet delay"), OPTARG),
	Arg_int(J, &Ignore, 0, english("first channel"), OPTARG),
	Arg_string(K, &ConnectArg, getStr, english("acct string"), OPTARG),
	Arg_string(L, &LinkAddress, getStr, english("link address"), 0),
	Arg_int(M, &TraceRflag, getInt1, english("reader trace level"), OPTARG|OPTVAL),
	Arg_string(N, &LinkName, getStr, english("link name"), 0),
	Arg_int(O, &Ignore, 0, english("output size"), OPTARG),
	Arg_int(P, &PktTraceflag, getInt1, english("packet trace level"), OPTARG|OPTVAL),
	Arg_char(Q, &Quality, getQuality, english("priority"), OPTARG),
	Arg_long(R, &MaxRunTime, 0, english("max run time"), OPTARG),
	Arg_long(S, &MinSpeed, 0, english("minimum bytes/sec."), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("trace level"), OPTARG|OPTVAL),
	Arg_int(U, &UpdateTime, getInt1, english("status update rate"), OPTARG),
	Arg_int(V, &TraceWflag, getInt1, english("writer trace level"), OPTARG|OPTVAL),
	Arg_int(W, &Ignore, 0, english("ACK window"), OPTARG),
	Arg_long(X, &NomSpeed, 0, english("nominal bytes/sec."), OPTARG),
	Arg_int(Y, &Idle_Timeout, 0, english("idle timeout"), OPTARG),
	Arg_int(Z, &SleepTime, 0, english("queue scan rate"), OPTARG),
	Arg_igndups,				/* Allow repeat specifications */
	Arg_end
};

/*
**	Finish reasons.
*/

ExCode	FinChilderror	= { english("WRITER ERROR"), EX_IOERR };
ExCode	FinHangup	= { english("HANGUP"), EX_HANGUP };
ExCode	FinLowSpace	= { english("LOW SPACE"), EX_IOERR };
ExCode	FinMaxrun	= { english("MAX RUNTIME EXCEEDED"), EX_MAXRT };
ExCode	FinMismatch	= { english("LINK/HOME ADDRESS MISMATCH"), EX_MISMATCH };
ExCode	FinNegtime	= { english("-VE TIME CHANGE"), EX_TEMPFAIL };
ExCode	FinReaderror	= { english("VC READ ERROR"), EX_IOERR };
ExCode	FinRdtimeout	= { english("READ TIMEOUT"), EX_RDTIMO };
ExCode	FinRemSlow	= { english("REMOTE SLOW"), EX_REMSLOW };
ExCode	FinRemsync	= { english("REMOTE SYNC SLOW"), EX_REMSLOW };
ExCode	FinRemterm	= { english("REMOTE TERMINATE"), EX_REMTERM };
ExCode	FinSigpipe	= { english("SIGPIPE"), EX_SIGPIPE };
ExCode	FinSync		= { english("UNEXPECTED SYNC"), EX_IOERR };
ExCode	FinTerminated	= { english("TERMINATED"), EX_OK };
ExCode	FinUnxsignal	= { english("UNEXPECTED SIGNAL"), EX_UNXSIG };
ExCode	FinVCEcho	= { english("VIRTUAL CIRCUIT ECHO"), EX_VCECHO };
ExCode	FinWrtrBlocked	= { english("WRITER BLOCKED"), EX_IOERR };
ExCode	FinWriterror	= { english("VC WRITE ERROR"), EX_IOERR };
ExCode	Finished	= { english("FINISHED"), EX_OK };

/*
**	Miscellaneous
*/

char	ChdirStr[]	= "chdir";
char	CmdsName[]	= LINKCMDSNAME;
char *	LockFile;
char	LockName[]	= LOCKFILE;
char	LogFmt[]	= "[%d] %s";
char	LogName[]	= LOGFILE;
char	MsgsInName[]	= LINKMSGSINNAME;
int	OtherPid	= SYSERROR;
char	PinName[]	= english("in-pipe");
char	PoutName[]	= english("out-pipe");
char	ReaderName[]	= english("HTreader");
char	RepFmt[]	= english("chan %d %s %21s \"%.14s\" size %lu addr %lu");
char	Template[UNIQUE_NAME_LENGTH+1];		/* Last component of a command file name */
int	UpdateTime	= STATUS_UPDATE;	/* Status files update rate */
jmp_buf	VCerrorJmp;				/* For virtual circuit errors */
char	WriterName[]	= english("HTwriter");

#ifdef	SIG_SETMASK
static sigset_t sigallset;
#endif	/* SIG_SETMASK */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"MINSPOOLFSFREE",	(char **)&MINSPOOLFSFREE,	PLONG},
	{"NVETIMECHANGE",	(char **)&NVETIMECHANGE,	PINT},
	{"NICEDAEMON",		(char **)&NICEDAEMON,		PINT},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

void	MsgStats _FA_((FILE *));
void	ProtoStats _FA_((FILE *));
void	setTraceFile _FA_((char **));

extern void	reader _FA_((int, int));
extern void	writer _FA_((int, int));


/*
**	Argument processing and setup.
*/

int
main(argc, argv, envp)
	int	argc;
	char *	argv[];
	char *	envp[];
{
	register char *	cp;
	int		inpfds[2];
	int		outpfds[2];
	extern int	VCWrite();

#	ifdef	SIG_SETMASK
	sigfillset(&sigallset);
#	endif	/* SIG_SETMASK */

	SetNameProg(argc, argv, envp);

	InitParams();

	LockFile = concat(CmdsName, Slash, LockName, NULLSTR);	/* Here in case LockSet and DoArgs() error */

	DoArgs(argc, argv, Usage);

	Name = newstr(Name);	/* Save copy */

#if	XTI == 1
	if ( ConnectArg != NULL && strcmp(ConnectArg, "XTI") == 0 )
	{
		xti_connect = true;
		t_sync(0);
		t_sync(1);
	}
	else
		xti_connect = false;
#endif	/* XTI == 1 */

	SLinkAddress = StripTypes(LinkAddress);

	GetParams("daemon", Params, sizeof Params);
	GetParams(SLinkAddress, Params, sizeof Params);

	if ( InOnly && OutOnly )
		Error(english("-i + -o ==> nothing to do."));
	if ( TurnAround && !InOnly && !OutOnly )
		Error(english("-t needs -i or -o."));

	Pid = Detach(NoFork, NICEDAEMON, true, false);	/* Sets "NetUid" */

	if ( geteuid() != NetUid )
		Error(english("No permission."));

	while ( chdir(SPOOLDIR) == SYSERROR )
		Syserror(CouldNot, ChdirStr, SPOOLDIR);

	while ( chdir(LinkName) == SYSERROR )
	{
		cp = concat(LinkName, Slash, NULLSTR);
		if ( !CheckDirs(cp) )
			Syserror(CouldNot, ChdirStr, LinkName);
		free(cp);
	}

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
			Warn(english("Daemon (%d) for %s already running on %s."), LockPid, LinkAddress, LockNode);
			(void)_exit(EX_OK);
		}

		LockSet = true;
	}

	if ( Efile != NULLSTR )
		setTraceFile(&Efile);

	if ( Ffile != NULLSTR )
		setTraceFile(&Ffile);

	if ( Efile != NULLSTR && Ffile != NULLSTR )
		NoLog = true;

	if ( !NoLog )
		StderrLog(LogName);

	HomeAdLength = strlen(HomeAddress);
	LinkAdLength = strlen(LinkAddress);
	if ( strcmp(HomeAddress, LinkAddress) > 0 )
		Direction = 1;
	Trace2(1, "Direction=%d", Direction);

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);

	RouterDir = concat(SPOOLDIR, ROUTINGDIR, NULLSTR);
	CmdsRouteFile = concat(RouterDir, Template, NULLSTR);
	CmdsTempFile = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	(void)signal(SIGTERM, SigCatch);
	(void)signal(SIGHUP, SigCatch);
	(void)signal(SIGPIPE, SigCatch);
	(void)signal(SIGERROR, SigCatch);

#	if	SIGWRITER != SIGWAKEUP
	(void)signal(SIGWRITER, SIG_IGN);	/* SIGWAKEUP is ignored in Detach() */
#	endif	/* SIGWRITER != SIGWAKEUP */

	MesgN(LinkAddress, LogFmt, Pid, english("STARTED"));
	if ( Verbose )
	{
		MesgN("version", Version);
		Stats = true;
	}
	if ( Stats )
		EchoArgs(argc, argv);

	while ( pipe(inpfds) == SYSERROR )
		Syserror(CouldNot, CreateStr, PinName);

	while ( pipe(outpfds) == SYSERROR )
		Syserror(CouldNot, CreateStr, PoutName);

	Time = StartTime = time((time_t *)0);
	ActiveTime = 1;		/* Prevent divide/0 */

	if ( Idle_Timeout <= 0 )
	{
		if ( BatchTime > 0 || TurnAround )
			Error(english("-Y0 precludes `batch' or `turn-around' modes."));
		Idle_Timeout = 0;
		LinkDownTime = Time;
	}
	else
		LinkDownTime = LINK_DOWN_TIME;

	if ( MaxRunTime > 0 )
		MaxRunTime += Time;

	InitPktData();

	if ( NomSpeed > 0 )
	{
		if ( CookVC != (CookT *)0 )
		{
			NomSpeed *= 256;			/* 8-bit bytes */
			NomSpeed /= 256 + CookVC->cooked;
			if ( Stats )
				Report(english("`cooking' reduces nominal speed to %d"), NomSpeed);
		}

		if ( MinSpeed > (NomSpeed / 2) )
		{
			MinSpeed = NomSpeed / 2;
			if ( Stats )
				Report(english("`minimum bytes/sec.' reduced to %d"), MinSpeed);
		}

		InitDataCount = (WriteAheadSecs*NomSpeed+OutDataSize-1)/OutDataSize;
	}
	else
		InitDataCount = INTLDATAPKTS;

	VCname = EmptyString;	/* In case error before driver */
	WriteFuncp = VCWrite;

	switch ( OtherPid = fork() )
	{
	case SYSERROR:
		Syserror(CouldNot, "fork", WriterName);
		finish(EX_OSERR);
		break;

	default:
		Name = ReaderName;
		if ( TraceRflag )	/* Reader-specific trace level */
			Traceflag = TraceRflag;
		if ( Traceflag && Ffile == NULLSTR )
			StderrLog("readerlog");
		(void)close(inpfds[0]);
		(void)close(outpfds[0]);
		reader(inpfds[1], outpfds[1]);
		break;

	case 0:
		Name = WriterName;
		if ( TraceWflag )	/* Writer-specific trace level */
			Traceflag = TraceWflag;
		LockSet = false;	/* Let parent clean up */
		OtherPid = Pid;
		Pid = getpid();
		(void)ReSetLock(LockFile, Pid, OtherPid);
		FinChilderror.reason = english("ROUTER OR READER ERROR");
		(void)close(inpfds[1]);
		(void)close(outpfds[1]);
		writer(inpfds[0], outpfds[0]);
		break;
	}

	finish(EX_OK);
	return 0;
}



/*
**	Own abort.
*/

void
abort()
{
	(void)signal(SIGIOT, SIG_DFL);
	(void)kill(Pid, SIGIOT);
	exit(-1);	/* Just in case */
}



/*
**	Channel state in printable form.
*/

char *
ChnStateStr(chnp)
	register Chan *	chnp;
{
	static char	num[ULONG_SIZE+3];

	switch ( chnp->ch_state )
	{
	case CH_ACTIVE:		return english("ACTIVE  ");
	case CH_ENDED:		return english("ENDED   ");
	case CH_ENDING:		return english("ENDING  ");
	case CH_IDLE:		return english("IDLE    ");
	case CH_STARTING:	return english("STARTING");
	}

	(void)sprintf(num, "[%6d]", (int)chnp->ch_state);

	return num;
}



/*
**	Decay bytes/second by 25% every 20 seconds
**	(allows (200/ACT_MULT) Mb/sec before overflow).
*/

void
DecayActive()
{
	register Ulong	ul;

	Trace5(
		2,
		"DecayActive bytes=%lu rawbytes=%lu time=%lu elaps=%lu",
		(PUlong)ActiveBytes,
		(PUlong)ActiveRawBytes,
		(PUlong)ActiveTime,
		(PUlong)(Time-DecayTime)
	);

	if ( (Time - DecayTime) > 40 )
		DecayTime = Time;		/* +ve time change? */

	do
	{
		ul = ActiveBytes;
		ul -= (ul>>2);			/* Round up */
		ActiveBytes = ul;

		ul = ActiveRawBytes;
		ul -= (ul>>2);
		ActiveRawBytes = ul;

		ul = ActiveTime;
		ul -= (ul>>2);
		if ( (ActiveTime = ul) == 0 )
			ActiveTime = 1;		/* Prevent divide by 0 */
	}
	while
		( (DecayTime += 20) <= Time );

	DecayCount++;

	Trace4(3, "DecayActive ==> bytes=%lu rawbytes=%lu time=%lu", (PUlong)ActiveBytes, (PUlong)ActiveRawBytes, (PUlong)ActiveTime);
	Trace3(1, "DecayActive ==> speed=%lu rawspeed=%lu", (PUlong)(ActiveBytes/ActiveTime), (PUlong)(ActiveRawBytes/ActiveTime));
}



/*
**	Cleanup on error termination.
*/

void
finish(error)
	int		error;
{
	static int	fd;
	static bool	finishing;

	if ( finishing )
		Fatal(english("re-entered finish()"));

	finishing = true;
	Finish = true;
	Jump = false;

	IgnoreSignals();
	(void)alarm((unsigned)0);

	Recover(ert_exit);

	Trace5(
		1,
		"%s: finish(%d), code %d, reason %s",
		Name,
		error,
		(FinishReason==(ExCode *)0)?0:FinishReason->code,
		(FinishReason==(ExCode *)0)?Unknown:FinishReason->reason
	);

	if ( LockSet )
		while ( (fd = open(LockFile, O_RDWR)) == SYSERROR )
			if ( !SysWarn(CouldNot, OpenStr, LockFile) )
			{
				LockSet = false;
				break;
			}

#	if	AUTO_LOCKING != 1
	if ( LockSet )
		while ( Lock(LockFile, fd, for_writing) == SYSERROR )
			if ( !SysWarn(CouldNot, LockStr, LockFile) )
			{
				(void)close(fd);
				LockSet = false;
				break;
			}
#	endif	/* AUTO_LOCKING != 1 */

	if ( FinishReason == (ExCode *)0 )
	{
		FinishReason = &Finished;
		if ( error != EX_OK )
			Finished.code = error;
	}

	if ( FinishReason != &FinChilderror && OtherPid != SYSERROR )
	{
		if ( error != EX_OK || (FinishReason->code != EX_OK && FinishReason->code != EX_REMTERM) )
			(void)kill(OtherPid, SIGERROR);
		else
		{
			if ( Name == ReaderName )
				(void)sleep(MINSLEEP);	/* Writer may be terminating */
			(void)kill(OtherPid, SIGTERM);
#			if	BSD4 >= 2
			if ( Name == ReaderName )
			{
				(void)sleep(MINSLEEP);	/* Writer may be terminating */
				(void)kill(OtherPid, SIGWAKEUP);
			}
#			endif	/* BSD4 >= 2 */
		}
	}

	if ( Name == ReaderName )
	{
		int	pid;
		int	status;

		if ( FinishReason == &FinTerminated && OtherPid != SYSERROR )
		{
			(void)sleep(MINSLEEP);	/* Writer may be terminating */
			(void)kill(OtherPid, SIGTERM);
		}

		while ( (pid = wait(&status)) != SYSERROR && pid != OtherPid );

		if ( FinishReason == &FinSigpipe )
		{
			Report(english("writer died status 0x%x"), status);
			FinishReason = &FinChilderror;
		}

		if ( FinishReason == &FinChilderror )
		{
			status = ((status >> 8) & 0xff) | ((status & 0xff) << 8);

			if ( status == EX_REMTERM )
				FinishReason = &FinRemterm;
			else
				FinChilderror.code = status;
		}

		CTime = Time = time((time_t *)0);

#		if	PROTO_STATS == 1
		ProtoStats(ErrorFd);
#		endif	/* PROTO_STATS == 1 */

#		if	VCDAEMON_STATS == 1
		MsgStats(ErrorFd);
		if ( Stats )
			CpuStats(ErrorFd, Time-StartTime);
#		endif	/* VCDAEMON_STATS == 1 */

		if ( Stats || FinishReason != &Finished )
			MesgN(LinkAddress, LogFmt, Pid, FinishReason->reason);
	}
	else
	if ( Name == WriterName )
	{
		if ( !setjmp(VCerrorJmp) )
		{
			Jump = true;
			(void)signal(SIGALRM, SigCatch);
			(void)alarm(MINSLEEP+1);

			Trace1(1, "send 1st EOT");
			SendEOT(REOT);
			Trace1(1, "send 2nd EOT");
			SendEOT(REOT);			/* To be safer */

			Jump = false;
			(void)alarm((unsigned)0);
			(void)sleep(MINSLEEP+1);	/* To flush I/O */
		}
		else
		{
			(void)alarm((unsigned)0);
			Trace1(1, "EOT jumped");
		}

		Jump = false;

		CTime = Time = time((time_t *)0);

#		if	PROTO_STATS == 1
		ProtoStats(ErrorFd);
#		endif	/* PROTO_STATS == 1 */

#		if	VCDAEMON_STATS == 1
		MsgStats(ErrorFd);
#		endif	/* VCDAEMON_STATS == 1 */

		if ( FinishReason != &Finished )
			MesgN(LinkAddress, LogFmt, Pid, FinishReason->reason);
	}

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);

	Status.st_pid = 0;
	Write_Status();

	if ( LockSet )
	{
#		if	AUTO_LOCKING != 1
		(void)UnLock(fd);
#		endif	/* AUTO_LOCKING != 1 */
		(void)close(fd);
		(void)unlink(LockFile);
	}

	if ( Name == ReaderName )
		Shell(DMNPROG, SLinkAddress, FinishReason->reason, 0);

	(void)_exit(FinishReason->code);
}



/*
**	Turn on `batchmode'.
*/

/*ARGSUSED*/
AFuncv
getBatch(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( (BatchTime = val.l) == 0 )
		BatchTime = 1;
	DieOnDown = true;
	return ACCEPTARG;
}



/*
**	Set up default cooking pointer.
*/

/*ARGSUSED*/
AFuncv
getcCook(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( CookVC != (CookT *)0 && CookVC != &Cookers[0] )
		return (AFuncv)english("only one cooking method allowed");

	CookVC = &Cookers[0];	/* The default */
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
			*cp++ = ',';
	}
	else
		cp = NULLSTR;

	if ( CookVC != (CookT *)0 )
	{
		if ( CookVC == &Cookers[i] )
			return ACCEPTARG;

		return (AFuncv)english("only one cooking method allowed");
	}

	if ( cp != NULLSTR )
	{
		if ( Cookers[i].setup == (char *(*)())0 )
			return (AFuncv)english("unexpected parameterisation");

		if ( (cp = (*Cookers[i].setup)(&Cookers[i], cp)) != NULLSTR )
			return (AFuncv)cp;
	}
	else
	if ( Cookers[i].setdefault != NULLVFUNCP )
		(*Cookers[i].setdefault)(&Cookers[i]);

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
**	Check value of OutDataSize.
*/

AFuncv
getODS(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( (*(long *)arg = val.l) < MINPKTDATASIZE )
	{
		Warn("Minimum size for packet data is %d", MINPKTDATASIZE);
		*(long *)arg = MINPKTDATASIZE;
	}
	else
	if ( val.l > MAXPKTDATASIZE )
	{
		Warn("Maximum size for packet data is %d", MAXPKTDATASIZE);
		*(long *)arg = MAXPKTDATASIZE;
	}

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
	if ( val.c >= HIGHEST_QUALITY && val.c <= LOWEST_QUALITY )
		return ACCEPTARG;

	return (AFuncv)newprintf(english("priority: %c (high) to %c (low)"), HIGHEST_QUALITY, LOWEST_QUALITY);
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



/*
**	Ignore signals for children.
*/

void
IgnoreSignals()
{
	(void)signal(SIGTERM, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
	(void)signal(SIGALRM, SIG_IGN);
	(void)signal(SIGPIPE, SIG_IGN);
	(void)signal(SIGERROR, SIG_IGN);
}



/*
**	Print daemon stats.
*/

#if	VCDAEMON_STATS == 1
void
MsgStats(fd)
	FILE *			fd;
{
	register struct tm *	tmp;
	char *			connectfile;
	Ulong			elapsedtime;
	char			tbuf[TIMESTRLEN];

	if ( Time < StartTime )
		elapsedtime = 1;
	else
	if ( (elapsedtime = Time - StartTime) == 0 )
		elapsedtime = 1;

	if ( Stats )
	{

		(void)fprintf
		(
			fd, english("%s:\t%lu messages, %lu bytes in %s ==> %lu bytes/sec.\n\
\t\t%lu circuit bytes ==> %lu bytes/sec.\n"),
			Name, (PUlong)DmnStats[DS_MESSAGES], (PUlong)DmnStats[DS_DATA],
			TimeString(elapsedtime, tbuf), (PUlong)(DmnStats[DS_DATA]/elapsedtime),
			(PUlong)DmnStats[DS_VCDATA], (PUlong)(DmnStats[DS_VCDATA]/elapsedtime)
		);

		(void)fprintf
		(
			fd, english("\t\tdata throughput ==> %lu bytes/sec.\n"),
			(PUlong)(ActiveBytes/ActiveTime)
		);
	}

	/*
	**	Connect accounting.
	*/

	connectfile = concat(SPOOLDIR, STATSDIR, CONNECTLOG, NULLSTR);

	fd = fopen(connectfile, "a");

	free(connectfile);

	if ( fd == NULL )
		return;

	tmp = localtime(&StartTime);

	if ( tmp->tm_year >= 100 )
		tmp->tm_year -= 100;	/* Take care of anno domini 2000 */

	(void)fprintf
	(
		fd,
		"%02d/%02d/%02d %02d:%02d:%02d %d %s %s %lu %lu %lu %lu %s\n",
		tmp->tm_year, tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
		tmp->tm_wday,
		(Name==ReaderName)?"in ":"out", LinkAddress,
		(PUlong)elapsedtime, (PUlong)DmnStats[DS_MESSAGES],
		(PUlong)DmnStats[DS_DATA], (PUlong)StartTime,
		ConnectArg
	);

	(void)fclose(fd);
}
#endif	/* VCDAEMON_STATS */



/*
**	Print protocol stats.
*/

#if	PROTO_STATS == 1
void
ProtoStats(fd)
	FILE *		fd;
{
	register Chan *	chnp	= Channels;
	register int	i	= NCHANNELS;
	register Ulong	count	= 0;

	if ( !Stats )
		return;

	if ( Name == ReaderName )
	{
		(void)fprintf
		(
			fd, english("%s:\t%lu bytes skipped, %lu data CRC errors.\n"),
			Name, (PUlong)PktStats[PS_SKIPBYTE], (PUlong)PktStats[PS_BADDCRC]
		);

		for ( ; --i >= 0 ; chnp++ )
			count += chnp->ch_statsin(rch_datanak);

		(void)fprintf(fd, english("\t\t%lu NAKs generated.\n"), (PUlong)count);
	}
	else
	{
		for ( ; --i >= 0 ; chnp++ )
			count += chnp->ch_statsout(wch_datanak);

		(void)fprintf(fd, english("%s:\t%lu NAKs received.\n"), Name, (PUlong)count);
	}
}
#endif	/* PROTO_STATS */



/*
**	Setup file for error/trace messages.
*/

/*ARGSUSED*/
void
setTraceFile(filep)
	char **		filep;
{
	register FILE *	fd;
	register FILE **fdp;

	if ( filep == &Efile )
		fdp = &ErrorFd;
	else
		fdp = &TraceFd;

	(void)fflush(*fdp);

	while ( (fd = fopen(*filep, "a")) == NULL )
		Syserror(CouldNot, OpenStr, *filep);

	*fdp = fd;

#	if	FCNTL == 1 && O_APPEND != 0
	(void)fcntl
	(
		fileno(fd),
		F_SETFL,
		fcntl(fileno(fd), F_GETFL, 0) | O_APPEND
	);
#	endif
}



/*
**	Catch system termination etc. and wind up.
*/

void
SigCatch(sig)
	int	sig;
{
#	ifdef	SIG_SETMASK
	sigset_t sigoldset;
	(void)sigprocmask(SIG_BLOCK, &sigallset, &sigoldset);
#	else	/* SIG_SETMASK */
#	if	BSD4 >= 2
	int	omask = sigsetmask(~0);
#	endif	/* BSD4 >= 2 */
#	endif	/* SIG_SETMASK */

	(void)signal(sig, SIG_IGN);

	Trace4(1, "SigCatch(%d), Finish=%d, Jump=%d", sig, Finish, Jump);

	if ( !Finish )
	{
		register ExCode *	finrsn;

		Finish = true;	/* Maybe Syserror() should check this? */

		switch ( sig )
		{
		case SIGALRM:	finrsn = &FinRdtimeout;		break;
		case SIGERROR:	finrsn = &FinChilderror;	break;
		case SIGHUP:	finrsn = &FinHangup;		break;
		case SIGPIPE:	finrsn = &FinSigpipe;		break;
		case SIGTERM:	finrsn = &FinTerminated;	break;
		default:	finrsn = &FinUnxsignal;
				Report2("SIGNAL %d", sig);
		}

		Report1(finrsn->reason);

		if ( FinishReason == (ExCode *)0 )
			FinishReason = finrsn;
	}

#	ifdef	SIG_SETMASK
	(void)sigprocmask(SIG_SETMASK, &sigoldset, (sigset_t *)0);
#	else	/* SIG_SETMASK */
#	if	BSD4 >= 2
	(void)sigsetmask(omask);
#	endif	/* BSD4 >= 2 */
#	endif	/* SIG_SETMASK */

	if ( Jump )
	{
		Jump = false;
		longjmp(VCerrorJmp, EINTR);
	}
}
