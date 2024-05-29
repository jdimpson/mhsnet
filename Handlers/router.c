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


static char	sccsid[]	= "%W% %E%";

/*
**	Route messages from node-node daemons or senders.
**
**	Messages are passed to a local handler if destination is reached,
**	otherwise they are spooled for further transmission.
*/

#define	WAIT_CALL
#define	FILE_CONTROL
#define	STAT_CALL
#define	RECOVER
#define	LOCKING
#define	SIGNALS
#define	STDIO
#define	TIME
#define	ERRNO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"expand.h"
#include	"handlers.h"
#include	"header.h"
#include	"licence.h"
#include	"link.h"
#include	"msgsdb.h"
#include	"params.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"stats.h"
#include	"sub_proto.h"
#include	"sysexits.h"

#include	<ndir.h>


#define	CHILD_TIMEOUT	HOUR	/* Max runtime for a child process */
#define	MAX_ROUTE_LEN	256	/* Max length of a route string passed to a handler */
#define	SCAN_RATE	60	/* Max wait time between directory scans */
/*
**	Arguments.
*/

int	ChildTimeout	= CHILD_TIMEOUT;	/* Max runtime for a child process */
int	DropQSize;				/* Maximum queue size before DROP */
int	BatchTime;				/* Limited duration */
char *	HndlrName;				/* Handler for sub-router */
int	LowDropQSize;				/* Low queue size before UNDROP */
#if	SUN3 == 1
int	MaxLoops	= 2;			/* Maximum times round loop before action */
#else
int	MaxLoops	= 3;			/* Maximum times round loop before action */
#endif
int	MaxRetSize	= MAXRETSIZE;		/* Truncate returned messages > */
char *	Name		= sccsid;		/* Program invoked name */
bool	NoFork;					/* Don't fork off parent */
bool	OnePass;				/* Terminate after one scan */
int	Parallel;				/* Parallel router number */
int	Parent;					/* Parent's pid for SubRouter */
bool	Reportflag	= true;			/* Report on various things */
bool	ReturnBadAddress;			/* Return all bad addresses */
char *	SaveRoute;				/* Route filename for sub-routers */
int	ScanRate	= SCAN_RATE;		/* Directory scan rate */
bool	Stats		= true;			/* True if statistics logging required */
bool	SubRouter;				/* Sub-router -- delivery only */
int	Traceflag;				/* Global tracing control */

AFuncv	getDropQ _FA_((PassVal, Pointer));	/* Control broadcast drop */
AFuncv	getNotbool _FA_((PassVal, Pointer));	/* Invert boolean */
AFuncv	getMR _FA_((PassVal, Pointer));		/* Check MaxRetSize */
AFuncv	getRoute _FA_((PassVal, Pointer));	/* Alternate routing tables */
AFuncv	getStr _FA_((PassVal, Pointer));	/* Save copy of string */

/*
**	Must save strings because using "NameProg()"
*/

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, &ReturnBadAddress, 0),
	Arg_bool(f, &NoFork, 0),
	Arg_bool(o, &OnePass, 0),
	Arg_bool(r, &Reportflag, getNotbool),
	Arg_bool(s, &SubRouter, 0),
	Arg_bool(x, &Stats, getNotbool),
	Arg_int(A, &ChildTimeout, 0, english("CHILD_TIMEOUT"), OPTARG),
	Arg_int(B, &BatchTime, 0, english("duration"), OPTARG),
	Arg_string(D, &HndlrName, getStr, english("handler-dir"), OPTARG),
	Arg_int(L, &MaxLoops, 0, english("max-loop-count"), OPTARG),
	Arg_int(M, &MaxRetSize, getMR, english("MAXRETSIZE"), OPTARG),
	Arg_int(N, &Parallel, getInt1, english("parallel"), OPTARG),
	Arg_int(P, &Parent, 0, english("parent pid"), OPTARG),
	Arg_string(Q, 0, getDropQ, english("drop queue size[:low]"), OPTARG),
	Arg_string(R, 0, getRoute, english("routefile"), OPTARG),
	Arg_int(S, &ScanRate, getInt1, english("scan-rate"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_end
};

/*
**	Parameters set from commands file.
*/

char *	BounceDesc;		/* Error string for message to be returned */
CmdHead	Cleanup;		/* Commands to be executed after message routed */
char *	CmdsFile;		/* Commands file name */
int	CmdFlags;		/* Combined flags from commands file */
CmdHead	DataCmds;		/* Commands describing message data */
char *	DelEnvStr;		/* Delete environment field from header */
int	DmnPid;			/* Pid of daemon "owning" message */
Ulong	ElapsedTime;		/* Journey time */
char *	Filter;			/* Filter used on message */
char *	LinkDir;		/* Directory for spooling message */
char *	LinkName;		/* Message arrived/leaves on this link */
Link	LinkData;		/* Link routing details */
Ulong	MesgLength;		/* Length of data in current message */
Ulong	MesgStart;		/* Start of data in current message */
char *	NewEnv;			/* New environment field for header */
Ulong	Timeout;		/* Timeout time */
char *	UseAddr;		/* Advised destination address for rerouted message */
char *	UseLink;		/* Advised link for rerouted message */
char *	UseSrce;		/* Advised source address for rerouted message */

#define	BounceMesg	(CmdFlags & BOUNCE_MESG)	/* Message should be returned to sender */
#define	Delivered	(CmdFlags & DELIVERED)		/* Message has been delivered */
#define	LinkDown	(CmdFlags & LINK_DOWN)		/* Link has timed-out */
#define	LinkUp		(CmdFlags & LINK_UP)		/* Link came back up / started */
#define	MesgDup		(CmdFlags & MESG_DUP)		/* Message may be a duplicate */
#define	NakOnTimeout	(CmdFlags & NAK_ON_TIMEOUT)	/* Message must be NAKed on timeout */
#define	OutFiltered	(CmdFlags & OUT_FILTERED)	/* Message has been filtered */
#define	Reroute		(CmdFlags & RE_ROUTE)		/* Message is being re-routed */
#define	Retry		(CmdFlags & (RE_ROUTE|RETRY))	/* Message is being re-submitted or re-routed */

/*
**	Parameters set from Message.
*/

bool	Broadcast;		/* This message has a broadcast address that matches Home */
Addr *	Destination;		/* Destination address */
Handler *HndlrP;		/* Handler structure */
char *	MesgHdrFile;		/* File containing message header */
bool	NoCall;			/* Don't auto-call on this message */
bool	NoOpt;			/* True if no message ordering required */
bool	Returning;		/* True if this is a returning message */
int	RouteType;		/* Fast / cheap */
bool	StateMessage;		/* This message should have high priority in queue */
Addr *	Source_Addr;		/* Source address */

/*
**	Message list built from queue directory search.
*/

typedef enum
{
	new, processed, locked, oldlocked
}
	Mstate;

typedef struct Mlist	Mlist;
struct Mlist
{
	Mlist *	next;
	char	name[UNIQUE_NAME_LENGTH+1];
	Mstate	state;
};

Mlist *	MfreeHead;		/* Head of free list */
Mlist *	MlistHead;		/* Head of current list */

#define	RESCAN_QUEUE_TIME	20

/*
**	Structure for sub-router variables.
**
**	Can place pointer in ExBuf.ex_sig
**	- which is only used by Execute() for pipes.
*/

typedef struct
{
	char *	sr_handler;	/* Handler name */
	char *	sr_cmdfile;	/* Template for sub-router command file */
	Time_t	sr_action;	/* Last action */
}
	ExSubr;

#define	EXSUBR(P)	((ExSubr *)((P)->ex_sig))

/*
**	Miscellaneous.
*/

int	ABORT		= 0;		/* Terminate with prejudice */
char	AddrErr[]	= english("Address error -- please examine addresses before re-routing");
jmp_buf	AlrmJmp;			/* Used to break out of wait(3) */
char *	BadFile;			/* Template for bad commandsfile */
char	BadFilterMsg[]	= english("unexpected return status 0x%x from filter \"%s\", error \"%s\"");
char *	BadHandler;			/* Name of misunderstood message handler */
char *	CmdTemp;			/* Template for making commandsfile */
Time_t	CmdsTime;			/* Modify time of commands file */
bool	DropState;			/* Set true by DROPFILE */
int	ERROR		= EX_OK;	/* Terminate with error */
ExBuf	ExArgs;				/* Used to pass args to Execute() */
CmdHead	FDataCmds;			/* Commands describing altered data files from filter */
char *	FilterMesg;			/* Message returned by a filter */
char *	FilterTemp;			/* File template for output from a filter */
bool	Finish;				/* Terminate flag */
char *	FinishReason	= english("FINISHED");
Time_t	FinishTime;			/* Terminate after this and empty Q */
char *	FMesgHdrFile;			/* File containing message header from filter */
Ulong	FMesgLength;			/* Length of data in filter output message */
char *	HdrDestStr;			/* free() if non-null */
char *	HdrSourceStr;			/* free() if non-null */
ExBuf *	HndArray;			/* Active sub-router stati */
int	HndArrCount;			/* Active sub-router count */
Ulong	InMesgCount;			/* Total messages routed */
CmdHead	FUnlinkCmds;			/* Commands describing altered data files from filter */
char	GetCmdErrF[]	 = english("Multiple %s in commands file \"%s\"");
Addr *	Home_Addr;			/* Address of this node */
bool	JmpTerm;			/* True if SIGTERM catcher should longjmp */
int	CmdsFd		= SYSERROR;	/* Fd of locked commandsfile, if Parallel */
char	LockName[UNIQUE_NAME_LENGTH+1]	= LOCKFILE;
bool	LockSet;			/* Router lock in place */
char	LogName[UNIQUE_NAME_LENGTH+1]	= LOGFILE;
char *	LoopHandler;			/* Name of looping message handler */
char *	MesgTemp;			/* Template for making data copies */
Time_t	MesgTime;			/* Used by spool() for UniqueName() */
char *	NetControl;			/* Name of program to communicate with `netinit' */
char *	NewStater;			/* Name of program to make new links */
Ulong	OutMesgCount;			/* Total messages produced */
char *	PendingFile;			/* Template for suspended commandsfile */
Ulong	Qmax;				/* Maximum size of routing queue */
char *	RerouteFile;			/* Template for re-routed commandsfile */
char *	Router;				/* Full path name of this program */
char *	RoutingDir;			/* Directory for router to operate from */
char	RoutExcept[]	= english("Subject: network routing exception needs attention.\n\n");
char *	SavHdrDest;			/* free() if non-null */
bool	SourceIsHome;			/* True if HdrSource == home */
Ulong	StatsDelay;			/* Last link delay for incoming messages for statistics */
Ulong	StatsLength;			/* Length of message for statistics */
char *	StatsLink;			/* Name of link for statistics */
char	SubrMesg[]	= english("Sub-router for handler \"%s\" [%d] %s.");
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char	TermDmnPid[]	= english("transport daemon for link %s [%d] terminated");
char *	TraceFile;			/* Template for message trace files */
char *	TraceCmdF;			/* Template for message trace command files */
char *	TraceMatch;			/* String to be matched against header */
char *	VCcall;				/* Name of default call program */
jmp_buf	WakeJmp;			/* Used to break out of sleep(3) */

#ifdef	SIG_SETMASK
sigset_t NullSigset;			/* Used to clear all blocked signals */
#endif	/* SIG_SETMASK */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"ABORT",		(char **)&ABORT,		PINT},
	{"ERROR",		(char **)&ERROR,		PINT},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

/*
**	Argument flags.
*/

char	Flag__[]	= "-&";
char	Flag_A[]	= "-A";
char	Flag_C[]	= "-C";
char	Flag_D[]	= "-D";
char	Flag_E[]	= "-E";
char	Flag_H[]	= "-H";
char	Flag_I[]	= "-I";
char	Flag_L[]	= "-L";
char	Flag_N[]	= "-N";
char	Flag_P[]	= "-P";
char	Flag_R[]	= "-R";
char	Flag_S[]	= "-S";
char	Flag_b[]	= "-b";
char	Flag_d[]	= "-d";
char	Flag_f[]	= "-f";
char	Flag_i[]	= "-i";
char	Flag_o[]	= "-o";
char	Flag_r[]	= "-r";
char	Flag_s[]	= "-s";
char	Flag_u[]	= "-u";
char	Flag_x[]	= "-x";

/*
**	Directions for filters
*/

#define	IN	true
#define	OUT	false

/*
**	Misc. definitions
*/

#define	NEXT_ARG	NEXTARG(&ExArgs.ex_cmd)

void	addresserr _FA_((Addr *, Addr *));
bool	check_link _FA_((char *, int));
void	child_alarm _FA_((VarArgs *));
char *	convert_route _FA_((void));
bool	deliver _FA_((void));
int	dofilter _FA_((Link *, bool, CmdHead *, Ulong));
void	filter_setup _FA_((VarArgs *));
void	finish _FA_((int));
bool	getfiltercmd _FA_((CmdT, CmdV));
void	handlebad _FA_((char *));
void	handler_setup _FA_((VarArgs *));
void	ignore_sigterm _FA_((VarArgs *));
bool	invoke_spooler _FA_((Link *, CmdHead *));
void	leave_cmds _FA_((int));
void	listbuild _FA_((DIR *));
void	looping _FA_((char *, char *));
void	mailex _FA_((FILE *));
void	mailpr _FA_((FILE *));
void	mailspace _FA_((FILE *));
char *	new_link _FA_((char *, int));
void	Return _FA_((int, char *));
bool	route _FA_((void));
void	router _FA_((void));
void	show_timeout _FA_((void));
void	spool _FA_((Addr *, Addr *, Link *));
void	start_daemon _FA_((Link *, char *, char *));
void	sub_check _FA_((void));
void	sub_child _FA_((int, int));
int	sub_cmp _FA_((const void *, const void *));
ExBuf *	sub_collect _FA_((int, int));
bool	sub_router _FA_((void));
void	sub_start _FA_((ExBuf *));
void	sub_term _FA_((void));
bool	sub_wait _FA_((void));
void	sync_close _FA_((int, char *));
void	unlink_files _FA_((CmdHead *));
void	unlink_remove _FA_((char *));
void	un_lock _FA_((char *));
bool	UpPrint _FA_((FILE *, int));
void	UpStats _FA_((char *, Ulong));
void	write_cmds _FA_((CmdHead *, char *));
bool	write_lock _FA_((Mlist *));

extern SigFunc	alrmcatch, termcatch, wakecatch;

DODEBUG(extern int malloc_debug(int));
DODEBUG(extern int malloc_verify(void));



/*
**	Argument processing and setup.
*/

int
main(argc, argv, envp)
	int	argc;
	char *	argv[];
	char *	envp[];
{
	char	n[ULONG_SIZE+3];

	SetNameProg(argc, argv, envp);

	Recover(ert_exit);

	DODEBUG((void)malloc_debug(MALLOC_DEBUG));

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

	if ( Parallel > 1 )
	{
		int	l;

#		ifndef	F_SETLKW
		Error(english("Parallel routing only on systems with fcntl(F_SETLK)");
#		else	/* F_SETLKW */
		(void)sprintf(n, "%d", Parallel);
		l = UNIQUE_NAME_LENGTH - (strlen(n) + 1);
		(void)sprintf(LockName, "%.*s%s.%s", l - (int)strlen(LockName), Name, n, LOCKFILE);
		(void)sprintf(LogName, "%.*s%s.%s", l - (int)strlen(LogName), Name, n, LOGFILE);
#		endif	/* F_SETLKW */
	}

	if ( Parallel )
		(void)sprintf(n, "%d//", Parallel);
	else
		n[0] = '\0';

	if ( Traceflag )
		Stats = Reportflag = true;

	if ( SubRouter )
	{
		if ( HndlrName == NULLSTR )
			Error(english("No directory."));

		RoutingDir = concat(SPOOLDIR, ROUTINGDIR, HndlrName, Slash, NULLSTR);
		Name = concat(n, HndlrName, Slash, Name, NULLSTR);
	}
	else
	{
		Router = concat(SPOOLDIR, LIBDIR, Name, NULLSTR);
		RoutingDir = concat(SPOOLDIR, ROUTINGDIR, NULLSTR);
		Name = concat(n, Name, NULLSTR);
	}

	Pid = Detach(NoFork, 0, true, true);

	StartTime = Time;
	if ( BatchTime )
		FinishTime = Time + BatchTime;

	if ( geteuid() != NetUid )
		Error(english("No permission."));

#	if	SIG_SETMASK
	sigemptyset(&NullSigset);
	(void)sigprocmask(SIG_SETMASK, &NullSigset, (sigset_t *)0);
#	endif	/* SIG_SETMASK */

	(void)signal(SIGTERM, termcatch);

	while ( chdir(RoutingDir) == SYSERROR )
		if ( !CheckDirs(RoutingDir) )
			Syserror(CouldNot, "chdir", RoutingDir);

	if ( !SetLock(LockName, Pid, false) )
	{
		Warn(english("Router (%d) already running on %s."), LockPid, LockNode);
		exit(EX_OK);
	}

	LockSet = true;

	StderrLog(LogName);
	(void)fclose(stdin);
	(void)fclose(stdout);

	if ( Stats )
		Report("Vn=\"%s\" [%d] STARTED", Version, Pid);

	if ( !ReadRoute(SUMCHECK) )
		finish(EX_OSFILE);

	Home_Addr = StringToAddr(newstr(HomeName), NO_STA_MAP);
	ExHomeAddress = StripTypes(HomeName);

	(void)sprintf(Template, "%*s", (int)(sizeof Template - 1), EmptyString);

	BadFile = concat(SPOOLDIR, BADDIR, Template, NULLSTR);
	BadHandler = concat(SPOOLDIR, LIBDIR, BADHANDLER, NULLSTR);
	CmdTemp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	FilterTemp = concat(TMPDIR, Template, NULLSTR);
	LoopHandler = concat(SPOOLDIR, LIBDIR, LOOPHANDLER, NULLSTR);
	MesgTemp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	NetControl = concat(SPOOLDIR, LIBDIR, "netcontrol", NULLSTR);
	NewStater = concat(SPOOLDIR, LIBDIR, NEWSTATEHANDLER, NULLSTR);
	PendingFile = concat(SPOOLDIR, PENDINGDIR, Template, NULLSTR);
	RerouteFile = concat(SPOOLDIR, REROUTEDIR, Template, NULLSTR);
	TraceCmdF = concat(SPOOLDIR, TRACEDIR, Template, NULLSTR);
	TraceFile = CmdTemp;
	VCcall = concat(SPOOLDIR, LIBDIR, VCCALL, NULLSTR);

	NoLinkFunc = unlink_remove;
	TakeUnkChild = sub_child;

	if ( SubRouter )
	{
		if ( Parent == 0 )
			Error(english("No parent pid."));
	}
	else
		sub_check();

	Recover(ert_return);

	router();

	return 0;
}



/*
**	Unknown region encountered in destination address:
**	spool foreign message in _pending for re-routing,
**	else Return().
*/

/*ARGSUSED*/
void
addresserr(source, dest)
	Addr *		source;
	Addr *		dest;
{
	register CmdV	cmdv;
	register char *	cp;
	int		fd;
	CmdHead		cmds;

	Report4(
		english("message [%s] with unknown address\n\tto:\t%s\n\troute:\t%s"),
		CmdsFile,
		TypedAddress(dest),
		convert_route()
	);

	cp = concat
	     (
		english("Address \""),
		UnTypAddress(dest),
		english("\" unknown at \""),
		UnTypName(Home_Addr),
		"\"",
		NULLSTR
	     );

	if ( SourceIsHome || ReturnBadAddress )
	{
		Return(0, cp);
		return;
	}

	/*
	**	Need to make a new header, and spool commands in _pending.
	*/

	if ( FreeHdrSource )
		free(HdrSource);
	HdrSource = TypedAddress(source);
	FreeHdrSource = false;

	if ( FreeHdrDest )
		free(HdrDest);
	HdrDest = TypedAddress(dest);
	FreeHdrDest = false;

	/*
	**	Make links to data files.
	*/

	InitCmds(&cmds);

	LinkCmds(&DataCmds, &cmds, (Ulong)0, DataLength, MesgTemp, MesgTime);

	/*
	**	Make a new file for header.
	*/

	fd = create(UniqueName(MesgTemp, CHOOSE_QUALITY, OPTNAME, MesgLength, MesgTime));

	while ( WriteHeader(fd, (long)0, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, MesgTemp);

	sync_close(fd, MesgTemp);

	/*
	**	Make a command file for this message in a temporary directory.
	*/

	(void)AddCmd(&cmds, file_cmd, (cmdv.cv_name = MesgTemp, cmdv));
	(void)AddCmd(&cmds, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&cmds, end_cmd, (cmdv.cv_number = HdrLength, cmdv));
	(void)AddCmd(&cmds, unlink_cmd, (cmdv.cv_name = MesgTemp, cmdv));
	(void)AddCmd(&cmds, flags_cmd, (cmdv.cv_number = RE_ROUTE|NAK_ON_TIMEOUT, cmdv));
	(void)AddCmd(&cmds, address_cmd, (cmdv.cv_name = HdrDest, cmdv));

	if ( (cmdv.cv_number = HdrTtd) == 0 || HdrTtd > WEEK )
		cmdv.cv_number = WEEK;
	(void)AddCmd(&cmds, timeout_cmd, cmdv);

	(void)AddCmd(&cmds, bounce_cmd, (cmdv.cv_name = cp, cmdv));

	free(cp);

	(void)UniqueName(CmdTemp, CHOOSE_QUALITY, OPTNAME, MesgLength, MesgTime);

	write_cmds(&cmds, CmdTemp);

#	if	PRIORITY_ROUTING == 1
	move(CmdTemp, UniqueName(PendingFile, HdrQuality, NoOpt, MesgLength, MesgTime));
#	else	/* PRIORITY_ROUTING == 1 */
	move(CmdTemp, UniqueName(PendingFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, MesgTime));
#	endif	/* PRIORITY_ROUTING == 1 */

	UpStats("PENDING", DataLength+HdrLength);

	looping(NULLSTR, PendingFile);

	FreeCmds(&cmds);
}



/*
**	Catch SIGALRM.
*/

void
alrmcatch(sig)
	int	sig;
{
	DODEBUG(char dateb[DATETIMESTRLEN]);

	(void)signal(sig, SIG_IGN);
	Trace2(1, "%s: SIGALRM", DateTimeStr((Time_t)time((time_t *)0), dateb));
	(void)longjmp(AlrmJmp, sig);
}



/*
**	Set up LinkData for link `name'.
*/

bool
check_link(name, flag)
	char *		name;
	int		flag;
{
	register int	count;
	register char *	errs;

	Trace3(1, "check_link(%s, %s)", name, (flag==LINK_UP)?"up":(flag==LINK_DOWN)?"down":"any");

	for ( count = 0 ; ; )
	{
		if
		(
			FindLink(name, &LinkData)
			&&
			(
				flag == 0
				||
				(LinkData.ln_flags & FL_NOCHANGE)
				||
				(flag == LINK_UP && (LinkData.ln_flags & (FL_DOWN|FL_DEAD)) == 0)
				||
				(flag == LINK_DOWN && (LinkData.ln_flags & (FL_DOWN|FL_DEAD)) != 0)
			)
			&&
			(
				count
				||
				Filter == NULLSTR
				||
				OutFiltered
				||
				(LinkData.ln_filter != NULLSTR && strcmp(Filter, LinkData.ln_filter) == STREQUAL)
			)
		)
			return true;

		if ( count++ )
		{
			handlebad
			(
				concat
				(
					"Unknown link \"",
					name,
					"\" in commandsfile \"",
					CmdsFile,
					"\"",
					NULLSTR
				)
			);

			return false;
		}

		if ( (errs = new_link(name, flag)) != NULLSTR )
		{
			handlebad(errs);
			return false;
		}

		flag = 0;
	}
}



/*
**	Set alarm timeout for children.
*/

void
child_alarm(vap)
	VarArgs *	vap;
{
#	if	POSIX == 1
	setsid();
#	endif	/* POSIX == 1 */
	(void)signal(SIGALRM, SIG_DFL);
	(void)alarm(ChildTimeout);
}



/*
**	Cleanup after commandsfile processed.
*/

void
cleanup(strings)
	bool	strings;
{
	unlink_files(&Cleanup);

	Trace2(3, "cleanup(%s)", strings?"strings":EmptyString);

	if ( CmdsFile != NULLSTR )
	{
		Trace2(2, "unlink(%s)", CmdsFile);
		(void)unlink(CmdsFile);
		un_lock(CmdsFile);
		CmdsFile = NULLSTR;
	}

	FreeCmds(&Cleanup);
	FreeCmds(&DataCmds);

	if ( !strings )
		return;

	FreeHeader();
	HdrSource = NULLSTR;

	FreeStr(&BounceDesc);
	FreeStr(&DelEnvStr);
	FreeStr(&Filter);
	FreeStr(&HdrDestStr);
	FreeStr(&HdrSourceStr);
	FreeStr(&LinkDir);
	FreeStr(&LinkName);
	FreeStr(&NewEnv);
	FreeStr(&SavHdrDest);
	FreeStr(&UseAddr);
	FreeStr(&UseLink);
	FreeStr(&UseSrce);
}



#if	DEBUG >= 1
/*
**	Convert chars in string.
*/

char *
convert_route()
{
	register char *	cp;
	register char *	op;
	register char	c;
	register int	len;

	static char *	keep_cp;
	static int	keep_len;

	if ( (cp = HdrRoute) == NULLSTR )
		return EmptyString;

	if ( (len = strlen(cp)) == 0 )
		len = 64;			/* Something reasonable */
	else
	{
		for ( op = cp ; (c = *op++) != '\0' ; )
			if ( c == ROUTE_RS )
				len += 2;	/* Allow for 2 tabs */

		len = (len+7) & ~7;		/* Round up */
	}

	if ( len > keep_len )
	{
		FreeStr(&keep_cp);
		keep_cp = Malloc(len);
		keep_len = len;
	}

	op = keep_cp;

	if ( *cp++ != ROUTE_RS )
		--cp;

	do
		if ( (c = *cp++) == ROUTE_US )
			c = ' ';
		else
		if ( c == ROUTE_RS )
		{
			*op++ = '\n';
			*op++ = '\t';
			c = '\t';
		}
	while
		( (*op++ = c) != '\0' );

	return keep_cp;
}
#endif	/* DEBUG >= 1 */



/*
**	Pass message to local handler for delivery.
*/

bool
deliver()
{
	char *	handler;
	char *	errs;
	FILE *	fd;
	int	routelen;
	char	Dbuf[NUMERICARGLEN];
	char	Fbuf[NUMERICARGLEN];
	char	Mbuf[NUMERICARGLEN];
	char	Wbuf[NUMERICARGLEN];

	if ( StateMessage && SourceIsHome )
		return false;	/* Don't bother */

	Trace2(1, "deliver() for %s", HdrHandler);

	/*
	**	Check handler exists.
	*/

	HndlrP = GetHandler(HdrHandler);

	handler = concat(SPOOLDIR, HANDLERSDIR, HdrHandler, NULLSTR);

	if ( strchr(HdrHandler, '/') != NULLSTR || access(handler, 1) == SYSERROR )
	{
		int	status;

		/*
		**	Requested handler doesn't exist here.
		**
		**	Return message to source if unknown.
		*/

		if ( HndlrP != (Handler *)0 )
			status = EX_OSFILE;	/* Known handler == error */
		else
			status = 0;

		Return(status, concat("Message handler \"", HdrHandler, "\" unavailable.", NULLSTR));
		free(handler);
		return false;
	}

	/*
	**	Setup HndlrP and check for sub-router.
	*/

	if
	(
		HndlrP != (Handler *)0
		&&
		!SubRouter
		&&
		HndlrP->router
	)
	{
		free(handler);
		return sub_router();
	}

	NameProg("%s DELIVER %s", Name, (HndlrP!=(Handler *)0)?HndlrP->descrip:HdrHandler);

	/*
	**	Execute handler.
	*/

	FIRSTARG(&ExArgs.ex_cmd) = handler;
	if ( Broadcast )
		NEXT_ARG = Flag_b;
	NEXT_ARG = NumericArg(Dbuf, 'D', DataLength);
	if ( HdrFlags )
		NEXT_ARG = NumericArg(Fbuf, 'F', HdrFlags);
	NEXT_ARG = Flag_H;
	NEXT_ARG = TypedAddress(Home_Addr);
	NEXT_ARG = Flag_I;
	NEXT_ARG = HdrID;
	if ( LinkName != NULLSTR )
	{
		NEXT_ARG = Flag_L;
		NEXT_ARG = LinkName;
	}
	NEXT_ARG = NumericArg(Mbuf, 'M', HdrTt);
	if ( HdrPart[0] != '\0' )
	{
		NEXT_ARG = Flag_P;
		NEXT_ARG = HdrPart;
	}
	NEXT_ARG = Flag_R;
	NEXT_ARG = HdrRoute;
	if ( (int)strlen(HdrRoute) > MAX_ROUTE_LEN )	/* Some arbitrary cut-off */
	{
		routelen = MAX_ROUTE_LEN;
		while ( HdrRoute[routelen] != ROUTE_RS && routelen > 0 )
			routelen--;
		HdrRoute[routelen] = '\0';
	}
	else
		routelen = 0;
	NEXT_ARG = Flag_S;
	NEXT_ARG = HdrSource;
	if ( HdrEnv[0] != '\0' )
	{
		NEXT_ARG = Flag_E;
		NEXT_ARG = HdrEnv;
	}
	if ( Parent )
		NEXT_ARG = NumericArg(Wbuf, 'W', Parent);

	fd = (FILE *)Execute(&ExArgs, handler_setup, ex_pipe, SYSERROR);

	(void)WriteCmds(&DataCmds, fileno(fd), PipeStr);

	if ( routelen )
		HdrRoute[routelen] = ROUTE_RS;

	if ( (errs = ExClose(&ExArgs, fd)) != NULLSTR )
	{
		/*
		**	Handler failed.
		**
		**	Return message to source.
		*/

		if ( Finish )
			leave_cmds(MESG_DUP);
		else
			Return(ExStatus, errs);

		free(handler);
		return false;
	}

	free(handler);
	return true;
}



/*
**	Run filter on Message.
*/

int
dofilter(linkp, in, cmdh, tt)
	Link *		linkp;
	bool		in;
	CmdHead *	cmdh;
	Ulong		tt;
{
	register char *	cp;
	FILE *		fd;
	int		ofd;
	char		numbuf1[NUMERICARGLEN];
	char		numbuf2[NUMERICARGLEN];

	if ( linkp->ln_filter[0] == '/' )
		cp = newstr(linkp->ln_filter);
	else
		cp = concat(SPOOLDIR, linkp->ln_filter, NULLSTR);

	Trace4(1, "Message %s \"%s\" passed to filter \"%s\"", in?"from":"to", linkp->ln_rname, cp);

	/*
	**	Create file for new commands from filter.
	*/

	ofd = create(UniqueName(FilterTemp, CHOOSE_QUALITY, OPTNAME, MesgLength, MesgTime));

	/*
	**	Run filter.
	*/

	FIRSTARG(&ExArgs.ex_cmd) = cp;
	NEXT_ARG = in?Flag_i:Flag_o;
	if ( Reroute )
		NEXT_ARG = Flag_r;
	if ( UseLink != NULLSTR )
		NEXT_ARG = Flag_u;
	NEXT_ARG = Flag_H;
	NEXT_ARG = HomeName;
	NEXT_ARG = Flag_L;
	NEXT_ARG = linkp->ln_rname;
	NEXT_ARG = Flag_N;
	NEXT_ARG = linkp->ln_name;
	NEXT_ARG = NumericArg(numbuf1, 'M', MesgTime);
	if ( tt != 0 )
		NEXT_ARG = NumericArg(numbuf2, 'X', tt);

	fd = (FILE *)Execute(&ExArgs, filter_setup, ex_pipo, ofd);

	(void)close(ofd);

	(void)WriteCmds(cmdh, fileno(fd), PipeStr);

	if ( (FilterMesg = ExClose(&ExArgs, fd)) == NULLSTR )
		FilterMesg = newstr(NULLSTR);

	Trace3(2, "Filter returns status %d: %s", ExStatus, FilterMesg);

	/*
	**	Check new commands from filter.
	*/

	MesgStart = 0;
	FMesgLength = 0;
	FMesgHdrFile = NULLSTR;

	if ( ReadCmds(FilterTemp, getfiltercmd) )
	{
		register CmdV	cmdv;

		if ( FMesgHdrFile == NULLSTR || FMesgLength == 0 )
		{
			ExStatus = EX_SOFTWARE;
			goto bad;
		}

		FreeFileCmds(cmdh);		/* Preserve non-file commands */
		MoveCmds(&FDataCmds, cmdh);

		if ( in )
		{
			CmdFlags |= IN_FILTERED;
			MesgLength = FMesgLength;
			MesgHdrFile = FMesgHdrFile;
			MoveCmds(&FUnlinkCmds, &Cleanup);
		}
		else
		{
			(void)AddCmd(cmdh, flags_cmd, (cmdv.cv_number = OUT_FILTERED, cmdv));
			(void)AddCmd(cmdh, filter_cmd, (cmdv.cv_name = linkp->ln_filter, cmdv));
			(void)AddCmd(cmdh, linkdir_cmd, (cmdv.cv_name = linkp->ln_name, cmdv));
			MoveCmds(&FUnlinkCmds, cmdh);
		}
	}
	else
	{
bad:		FreeCmds(&FDataCmds);
		FreeCmds(&FUnlinkCmds);
	}

	(void)unlink(FilterTemp);
	free(cp);

	return ExStatus;
}



/*
**	Setup directory for filter.
*/

void
filter_setup(vap)
	VarArgs *	vap;
{
	while ( chdir(SPOOLDIR) == SYSERROR )
		Syserror(CouldNot, "chdir", SPOOLDIR);

	child_alarm(vap);
}



/*
**	Clean up on error termination.
*/

void
finish(error)
	int		error;
{
	register int 	i;

	(void)signal(SIGTERM, SIG_IGN);
	Finish = true;

	Recover(ert_exit);

	if ( !ExInChild && error != EX_USAGE )
	{
		if ( !SubRouter )
			sub_term();

		CloseMsgsDB();
		(void)CloseStats();

		if ( Stats )
		{
			(void)fprintf(stderr, english("%10lu messages in\n"), (PUlong)InMesgCount);
			(void)fprintf(stderr, english("%10lu messages out\n"), (PUlong)OutMesgCount);
			(void)fprintf(stderr, english("%10lu = max. routing queue length\n"), (PUlong)Qmax);
			CpuStats(stderr, (Time_t)time((time_t *)0) - StartTime);
			Report("Vn=\"%s\" [%d] %s\n", Version, Pid, error?english("ERROR"):FinishReason);
		}

		for ( i = 0 ; i < 9 ; i++ )
			(void)UnLock(i);

		if ( LockSet )
			(void)unlink(LockName);

		if ( error )
		{
			if ( LockSet )
				(void)MailNCC(mailpr, NCC_ADMIN);

			if
			(
				DmnPid != 0
				&&
				kill(DmnPid, SIGERROR) != SYSERROR
			)
			{
				Report3(
					TermDmnPid,
					(LinkName==NULLSTR)?Unknown:LinkName,
					DmnPid
				);
			}

			if ( SubRouter )
				(void)kill(Parent, SIGTERM);
		}
	}

	if ( ABORT )
		abort();

	if ( error == EX_OK )
		error = ERROR;

	(void)exit(error);
}



/*
**	Process a command from commands file.
*/

bool
getcommand(type, val)
	CmdT	type;
	CmdV	val;
{
	char *	cp1;

	switch ( type )
	{
	case address_cmd:
		break;

	case bounce_cmd:
		FreeStr(&BounceDesc);
		BounceDesc = newstr(val.cv_name);
		Trace2(2, "BounceDesc \"%s\"", BounceDesc);
		break;

	case delenv_cmd:
		if ( DelEnvStr != NULLSTR )
			free(DelEnvStr);
		DelEnvStr = newstr(val.cv_name);
		Trace2(2, "DelEnvStr \"%s\"", DelEnvStr);
		break;

	case end_cmd:
		(void)AddCmd(&DataCmds, type, val);
		MesgLength += val.cv_number - MesgStart;
		break;

	case env_cmd:
		if ( NewEnv != NULLSTR )
		{
			cp1 = concat(NewEnv, val.cv_name, NULLSTR);
			free(NewEnv);
			NewEnv = cp1;
		}
		else
			NewEnv = newstr(val.cv_name);
		Trace2(2, "NewEnv \"%s\"", NewEnv);
		break;

	case file_cmd:
		MesgHdrFile = AddCmd(&DataCmds, type, val);	/* Last one will contain header */
		break;

	case filter_cmd:
		if ( Filter != NULLSTR )
		{
			Warn(GetCmdErrF, "filters", CmdsFile);
			return false;
		}
		Filter = newstr(val.cv_name);
		break;

	case flags_cmd:
		CmdFlags |= val.cv_number;
		break;

	case hdrlength_cmd:
		break;

	case linkdir_cmd:
		if ( LinkDir != NULLSTR )
		{
			Warn(GetCmdErrF, "linkdirs", CmdsFile);
			return false;
		}
		LinkDir = newstr(val.cv_name);
		break;

	case linkname_cmd:
		if ( LinkName != NULLSTR )
		{
			Warn(GetCmdErrF, "linknames", CmdsFile);
			return false;
		}
		LinkName = newstr(val.cv_name);
		break;

	case pid_cmd:
		DmnPid = val.cv_number;
		break;

	case start_cmd:
		(void)AddCmd(&DataCmds, type, val);
		MesgStart = val.cv_number;
		break;

	case timeout_cmd:
		if ( val.cv_number < Timeout )
			Timeout = val.cv_number;
		break;

	case traveltime_cmd:
		ElapsedTime += val.cv_number;
		break;

	case unlink_cmd:
		(void)AddCmd(&Cleanup, type, val);
		break;

	case useaddr_cmd:
		FreeStr(&UseAddr);
		UseAddr = newstr(val.cv_name);
		Trace2(2, "UseAddr \"%s\"", UseAddr);
		break;

	case uselink_cmd:
		FreeStr(&UseLink);
		UseLink = newstr(val.cv_name);
		Trace2(2, "UseLink \"%s\"", UseLink);
		break;

	case usesrce_cmd:
		FreeStr(&UseSrce);
		UseSrce = newstr(val.cv_name);
		Trace2(2, "UseSrce \"%s\"", UseSrce);
		break;

	default:
		{
			char	num[ULONG_SIZE+2];

			if ( (unsigned)type > (unsigned)last_cmd )
			{
				(void)sprintf(num, "[%#x]", (Uchar)type);
				cp1 = num;
			}
			else
				cp1 = CmdfDescs[(int)type];
			Warn("Unexpected command \"%s\" in \"%s\" ignored", cp1, CmdsFile);
			return true;
		}
	}

	return true;
}



/*
**	Process a command from filter output commands file.
*/

bool
getfiltercmd(type, val)
	CmdT	type;
	CmdV	val;
{
	char *	cp1;

	switch ( type )
	{
	case delenv_cmd:
		if ( DelEnvStr != NULLSTR )
			free(DelEnvStr);
		DelEnvStr = newstr(val.cv_name);
		Trace2(2, "DelEnvStr \"%s\"", DelEnvStr);
		break;

	case env_cmd:
		if ( NewEnv != NULLSTR )
		{
			cp1 = concat(NewEnv, val.cv_name, NULLSTR);
			free(NewEnv);
			NewEnv = cp1;
		}
		else
			NewEnv = newstr(val.cv_name);
		break;

	case end_cmd:
		FMesgLength += val.cv_number - MesgStart;
		(void)AddCmd(&FDataCmds, type, val);
		break;

	case file_cmd:
		FMesgHdrFile = AddCmd(&FDataCmds, type, val);	/* Last one will contain header */
		break;

	case start_cmd:
		MesgStart = val.cv_number;
		(void)AddCmd(&FDataCmds, type, val);
		break;

	case unlink_cmd:
		(void)AddCmd(&FUnlinkCmds, type, val);
		break;

	default:
		(void)AddCmd(&FDataCmds, type, val);
	}

	return true;
}



/*
**	Set broadcast drop queue limits.
*/

/*ARGSUSED*/
AFuncv
getDropQ(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	cp;

	if ( (DropQSize = atoi(val.p)) < 12 )
		return (AFuncv)english("Drop queue high minimum is 12");

	if ( (cp = strchr(val.p, ':')) != NULLSTR )
		if ( (LowDropQSize = atoi(cp+1)) >= DropQSize )
			return (AFuncv)english("Drop queue low must be < high");

	return ACCEPTARG;
}



/*
**	Get MAXRETSIZE.
*/

/*ARGSUSED*/
AFuncv
getMR(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.l < 1000 )
		return (AFuncv)english("MAXRETSIZE minimum is 1000");

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
**	Get a routefile name argument.
*/

/*ARGSUSED*/
AFuncv
getRoute(val, arg)
	PassVal		val;
	Pointer		arg;
{
	SaveRoute = newstr(val.p);
	SetRoute(SaveRoute);
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



/*
**	Pass commandsfile describing bad message to "badhandler".
*/

void
handlebad(arg)
	char *		arg;	/* free()'d */
{
	register CmdV	cmdv;
	register int	fd;
	char *		errs;
	CmdHead		cmds;
	struct stat	statb;

	Recover(ert_return);

	Trace2(1, "handlebad(%s)", arg);

	if ( CmdsFile == NULLSTR )
	{
		Report(english("no commandsfile for error \"%s\""), arg);
		free(arg);
		return;		/* Already dealt with? */
	}

	if ( stat(CmdsFile, &statb) == SYSERROR || statb.st_size == 0 )
	{
		Report3(english("Zero length commandsfile [%s] ignored\n\treason was \"%s\""), CmdsFile, arg);
		(void)unlink(CmdsFile);
		un_lock(CmdsFile);
		CmdsFile = NULLSTR;
		free(arg);
		return;
	}

	FreeCmds(&Cleanup);	/* Leave evidence */

	/*
	**	Must make copy of commandsfile.
	*/

	fd = create(UniqueName(CmdTemp, CHOOSE_QUALITY, OPTNAME, MesgLength, Time));

	(void)lseek(CmdsFd, 0L, 0);	/* Back to beginning */

	if ( !CopyFdToFd(CmdsFd, CmdsFile, fd, CmdTemp, MAX_ULONG) )
	{
		(void)close(fd);	/* Copy impossible. */
		move(CmdsFile, CmdTemp);
		un_lock(CmdsFile);
		CmdsFile = NULLSTR;

		while ( (fd = open(CmdTemp, O_RDWR)) == SYSERROR )
			if ( !SysWarn(CouldNot, OpenStr, CmdTemp) )
				break;
	}

	if ( fd != SYSERROR )
	{
		/*
		**	Add reason for later analysis.
		*/

		InitCmds(&cmds);

		(void)AddCmd(&cmds, pid_cmd, (cmdv.cv_number = Pid, cmdv));	/* Just in case previous command garbled */
		(void)AddCmd(&cmds, bounce_cmd, (cmdv.cv_name = arg, cmdv));

		if ( CmdsTime > 0 && Time > CmdsTime )
		{
			cmdv.cv_number = Time - CmdsTime;
			(void)AddCmd(&cmds, traveltime_cmd, cmdv);
		}

		if ( !WriteCmds(&cmds, fd, CmdTemp) )
		{
			cleanup(false);	/* Get rid of bad Message! */
			finish(EX_OSERR);
		}

		FreeCmds(&cmds);

		sync_close(fd, CmdTemp);
	}

	/*
	**	Move commands to BADDIR.
	*/

#	if	PRIORITY_ROUTING == 1
	move(CmdTemp, UniqueName(BadFile, CHOOSE_QUALITY, OPTNAME, MesgLength, Time));
#	else	/* PRIORITY_ROUTING == 1 */
	move(CmdTemp, UniqueName(BadFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time));
#	endif	/* PRIORITY_ROUTING == 1 */

	Report4(english("Message passed to %s in \"%s\"\n\treason \"%s\""), BadHandler, BadFile, arg);

	FIRSTARG(&ExArgs.ex_cmd) = BadHandler;

	NEXT_ARG = Flag_C;
	NEXT_ARG = BadFile;
	NEXT_ARG = Flag_E;
	NEXT_ARG = arg;
	NEXT_ARG = Flag_H;
	NEXT_ARG = HomeName;
	NEXT_ARG = Flag_I;
	NEXT_ARG = Name;
	if ( LinkName != NULLSTR )
	{
		NEXT_ARG = Flag_L;
		NEXT_ARG = LinkName;
	}
	if ( HdrSource != NULLSTR )
	{
		NEXT_ARG = Flag_S;
		NEXT_ARG = HdrSource;
	}

	if ( (errs = Execute(&ExArgs, ignore_sigterm, ex_exec, SYSERROR)) != NULLSTR )
	{
		/*
		**	A total disaster!
		*/

		Error(StringFmt, errs);

		cleanup(false);	/* Get rid of bad Message! */
		finish(EX_OSERR);
	}

	free(arg);
}



/*
**	Setup nice for handler children.
*/

void
handler_setup(vap)
	VarArgs *	vap;
{
	int		nicer;
	int		timeout;

#	ifndef	QNX
	if ( HndlrP != (Handler *)0 )
		if ( (nicer = HndlrP->nice) > 0 )
			(void)nice(nicer);
#	endif	/* QNX */

	nicer = ChildTimeout;

	if ( HndlrP != (Handler *)0 )
		if ( (timeout = HndlrP->timeout) > 0 )
			ChildTimeout = timeout;

	filter_setup(vap);	/* -> child_alarm -> alarm(ChildTimeout) */

	ChildTimeout = nicer;
}



/*
**	Ignore system termination for some children.
*/

void
ignore_sigterm(vap)
	VarArgs *	vap;
{
	(void)signal(SIGTERM, SIG_IGN);
	child_alarm(vap);
}



/*
**	Invoke special spooler for link.
*/

bool
invoke_spooler(linkp, cmdsp)
	register Link *	linkp;
	CmdHead *	cmdsp;
{
	register char *	cp;
	char *		errs;
	FILE *		fd;

	if ( linkp->ln_spooler[0] == '/' )
		cp = newstr(linkp->ln_spooler);
	else
		cp = concat(SPOOLDIR, linkp->ln_spooler, NULLSTR);

	Trace3(1, "Message to \"%s\" passed to spooler \"%s\"", linkp->ln_rname, cp);

	FIRSTARG(&ExArgs.ex_cmd) = cp;
	NEXT_ARG = Flag_H;
	NEXT_ARG = HomeName;
	NEXT_ARG = Flag_L;
	NEXT_ARG = linkp->ln_rname;
	NEXT_ARG = Flag_N;
	NEXT_ARG = linkp->ln_name;

	fd = (FILE *)Execute(&ExArgs, filter_setup, ex_pipe, SYSERROR);

	(void)WriteCmds(cmdsp, fileno(fd), PipeStr);

	if ( (errs = ExClose(&ExArgs, fd)) != NULLSTR )
	{
		/*
		**	Spooler failed.
		**
		**	Return message to source.
		*/

		Return(ExStatus, concat("Gateway handler failed :-\n", errs, NULLSTR));

		free(errs);
		free(cp);
		return false;
	}

	Trace4(
		1,
		"message passed to \"%s\" for \"%s\" via \"%s\"",
		cp,
		HdrDest,
		linkp->ln_rname
	);

	free(cp);

	return true;
}



/*
**	Return true if arg is a directory.
*/

bool
IsDir(file)
	char *	file;
{
	struct stat	statb;

	Trace2(2, "IsDir(%s)", file);

	if ( stat(file, &statb) == SYSERROR )
		return false;

	if ( (statb.st_mode & S_IFMT) != S_IFDIR )
		return false;

	Trace2(1, "IsDir(%s) => true", file);

	return true;
}



/*
**	Leave CmdsFile for later processing.
*/

void
leave_cmds(flags)
	int		flags;
{
	register CmdV	cmdv;

	Report4(
		english("leave_cmds(%s%s) in [%s]"),
		(flags&RE_ROUTE)?"RE_ROUTE":EmptyString,
		(flags&MESG_DUP)?"|MESG_DUP":EmptyString,
		CmdsFile
	);

	FreeCmds(&Cleanup);

	if ( CmdsFile == NULLSTR )
		return;

	if ( flags )
	{
		(void)AddCmd(&Cleanup, flags_cmd, (cmdv.cv_number = flags, cmdv));
		(void)WriteCmds(&Cleanup, CmdsFd, CmdsFile);
		sync_close(CmdsFd, CmdsFile);
		CmdsFd = SYSERROR;
	}
	else
		un_lock(CmdsFile);

	CmdsFile = NULLSTR;
}



/*
**	Build a list of message names from queue directory.
*/

void
listbuild(dirp)
	register DIR *		dirp;	/* dirp -> "." */
{
	register Mlist *	mp;
	register Mlist **	nmp;
	register struct direct *dp;
	int			count;
	static char		smsg[] = english("%s broadcast state messages");
	static char		smsgon[] = english("Processing");
	static char		smsgoff[] = english("Dropping");
#	ifdef	DEBUG
	static char		tmsg[] = english("Trace level %d");
#	endif	/* DEBUG */

	Trace1(2, "listbuild");

	for ( nmp = &MlistHead ; (mp = *nmp) != (Mlist *)0 ; )
		if ( mp->state == locked )
		{
			mp->state = oldlocked;
			nmp = &mp->next;
		}
		else
		{
			*nmp = mp->next;
			mp->next = MfreeHead;
			MfreeHead = mp;
		}

	count = 0;

	while ( (dp = readdir(dirp)) != NULL )
	{
		Trace2(9, "readdir => %s", dp->d_name);

		if
		(
			dp->d_name[0] < STATE_QUALITY
			||
			dp->d_name[0] > LOWEST_QUALITY
			||
			strlen(dp->d_name) != UNIQUE_NAME_LENGTH
		)
		{
			if ( strcmp(dp->d_name, STOPFILE) == STREQUAL )
			{
				Finish = true;
				Report("STOP file read => Finish");
				(void)unlink(dp->d_name);
			}
			else
			if ( strcmp(dp->d_name, DROPFILE) == STREQUAL )
			{
				LowDropQSize = 0;
				DropState = true;
				Report(smsg, smsgoff);
				(void)unlink(dp->d_name);
			}
			else
			if ( strcmp(dp->d_name, NODROPFILE) == STREQUAL )
			{
				DropState = false;
				Report(smsg, smsgon);
				(void)unlink(dp->d_name);
			}
			else
			if
			(
				strcmp(dp->d_name, TRACEONFILE) == STREQUAL
				||
				strcmp(dp->d_name, "TRACEON") == STREQUAL
			)
			{
				FreeStr(&TraceMatch);
				if ( (TraceMatch = ReadFile(dp->d_name)) != NULLSTR )
					Report2("Trace matches \"%s\"", TraceMatch);
				else
				{
					Traceflag++;
					Report2(tmsg, Traceflag);
				}
				(void)unlink(dp->d_name);
			}
			else
			if
			(
				strcmp(dp->d_name, TRACEOFFFILE) == STREQUAL
				||
				strcmp(dp->d_name, "TRACEOFF") == STREQUAL
			)
			{
				Traceflag = 0;
				FreeStr(&TraceMatch);
				Report2(tmsg, Traceflag);
				(void)unlink(dp->d_name);
			}

			continue;
		}

		Trace2(3, "listbuild %s", dp->d_name);

		count++;

		if ( (mp = MfreeHead) == (Mlist *)0 )
			mp = Talloc(Mlist);
		else
			MfreeHead = mp->next;

		mp->state = new;
		(void)strcpy(mp->name, dp->d_name);

		*nmp = mp;
		nmp = &mp->next;
	}

	*nmp = (Mlist *)0;

	if ( DropState )
	{
		if ( count <= LowDropQSize )
		{
			DropState = false;
			Report(smsg, smsgon);
		}
	}
	else
	if ( DropQSize > 0 && count > DropQSize )
	{
		DropState = true;
		Report(smsg, smsgoff);
	}


	Trace2(1, "listbuild => %d messages", count);

	if ( count > Qmax )
		Qmax = count;
}



/*
**	Compare two message names.
*/

int
listcmp(mp1, mp2)
	char *	mp1;
	char *	mp2;
{
	int	val;

	val = strcmp(((Mlist *)mp1)->name, ((Mlist *)mp2)->name);

	if ( val == 0 )
	{
		if ( ((Mlist *)mp1)->state == oldlocked )
			((Mlist *)mp2)->state = locked;
		else
			((Mlist *)mp1)->state = locked;
	}

	return val;
}



/*
**	Notify authorities of looping/bad-address message.
*/

void
looping(link, file)
	char *	link;
	char *	file;
{
	char *	errs;

#	if	DEBUG >= 1
	if ( link != NULLSTR )
		Report(
			"looping message [%s]\n\tlink:\t%s\n\tto:\t%s\n\troute:\t%s",
			CmdsFile,
			link,
			HdrDest,
			convert_route()
		);
#	endif	/* DEBUG >= 1 */

	FIRSTARG(&ExArgs.ex_cmd) = LoopHandler;

	NEXT_ARG = Flag_A;
	NEXT_ARG = HdrDest;
	NEXT_ARG = Flag_C;
	NEXT_ARG = file;
	NEXT_ARG = Flag_H;
	NEXT_ARG = HomeName;
	NEXT_ARG = Flag_I;
	NEXT_ARG = Name;

	if ( link != NULLSTR )
	{
		NEXT_ARG = Flag_L;
		NEXT_ARG = link;
	}

	if ( (errs = Execute(&ExArgs, ignore_sigterm, ex_exec, SYSERROR)) != NULLSTR )
	{
		/*
		**	A disaster!
		*/

		handlebad(newstr(errs));	/* Save errs for Error */
		Error(StringFmt, errs);
		cleanup(false);			/* Get rid of bad message! */
		finish(EX_OSERR);
	}
}



/*
**	Called from "MailNCC()" to produce body of mail for daemon errors.
*/

void
mailex(fd)
	FILE *	fd;
{
	(void)fprintf(fd, "%s", RoutExcept);

	(void)fprintf
	(
		fd,
		english("\
The transport daemon for link \"%s\"\n\
is causing problems and has been terminated (pid %d),\n\
please \"tail %s%s\"\n\
which might yield information to help fix the problem.\n\n"),
		(LinkName==NULLSTR)?Unknown:LinkName,
		DmnPid,
		RoutingDir,
		LogName
	);
}



/*
**	Called from "MailNCC()" to produce body of mail for termination errors.
*/

void
mailpr(fd)
	FILE *	fd;
{
	(void)fprintf(fd, "%s", RoutExcept);

	(void)fprintf
	(
		fd,
		english("\
A serious error has occurred that prevents further messages being routed,\n\
please \"tail %s%s\"\n\
and fix the problem as soon as possible.\n\n"),
		RoutingDir,
		LogName
	);
}



/*
**	Called from "MailNCC()" to produce body of mail for LOW SPACE notifications.
*/

void
mailspace(fd)
	FILE *	fd;
{
	(void)fprintf(fd, english("Subject: network LOW SPACE notification.\n\n"));

	(void)fprintf
	(
		fd,
		english("\
The transport daemon for link \"%s\"\n\
has suspended message reception on channel %lu because there is\n\
not enough space left in \"%s\".\n\n\
Message reception will resume when more space becomes available.\n\n\
[Check the output from \"netparams -f\"\n\
and the value of the MINSPOOLFSFREE parameter.]\n\n"),
		(LinkName==NULLSTR)?Unknown:LinkName,
		(PUlong)ElapsedTime,
		SPOOLDIR
	);
}



/*
**	Make a new link.
*/

char *
new_link(link, flag)
	char *	link;
	int	flag;
{
	char *	errs;

	if ( Stats )
		Report4(
			english("%slink \"%s\" %s"),
			flag?EmptyString:english("new "),
			link,
			(flag==LINK_DOWN)?english("down"):english("up")
		);

	FIRSTARG(&ExArgs.ex_cmd) = NewStater;

	if ( flag == LINK_DOWN )
		NEXT_ARG = Flag_d;
	else
		NEXT_ARG = Flag_i;	/* A new link is input only */

	NEXT_ARG = Flag_L;
	NEXT_ARG = link;

	if ( Filter != NULLSTR )
	{
		NEXT_ARG = Flag_R;
		NEXT_ARG = Filter;
	}

	if ( (errs = Execute(&ExArgs, child_alarm, ex_exec, SYSERROR)) != NULLSTR )
	{
		Report(StringFmt, errs);
		return errs;
	}

	(void)CheckRoute(SUMCHECK);

	return NULLSTR;
}



/*
**	Return the message to its source with an explanation of error.
*/

void
Return(status, error)
	int			status;
	char *			error;	/* free()'d */
{
	register HdrFld *	fp;
	register int		i;
	char *			errors;
	HdrFlds			save_header;
	Link			rlink;

	if ( Reportflag && strcmp(HdrHandler, "peter") != STREQUAL )	/* A well known failure */
		Report6(
			english("Message [%s] from \"%s\" to \"%s\"\n\treturned to sender (status 0%o):-\n%s"),
			CmdsFile,
			HdrSource,
			HdrDest,
			status,
			error
		);

	errors = StripErrString(error);

	if ( (int)strlen(errors) < 4 )
	{
		free(errors);

		if ( (int)strlen(error) < 4 )
		{
			free(error);
			error = newstr("Handler failed, reason unknown");
		}

		errors = newstr(error);
	}

	/*
	**	Don't return internal errors.
	*/

#	if	RETURN_TIMEDOUT
	if ( (i = (status & 0xff00)) != 0 && i != (SIGALRM<<8) )
#	else	/* RETURN_TIMEDOUT */
	if ( (i = (status & 0xff00)) != 0 )
#	endif	/* RETURN_TIMEDOUT */
		goto bad;

	switch ( status )
	{
	case EX_USAGE:
	case EX_SOFTWARE:
	case EX_OSFILE:
	case EX_OSERR:
	case EX_FDERR:
	case EX_FDERR+1:
	case EX_FDERR+2:
bad:		handlebad(concat("Operating error: ", error, NULLSTR));
		free(errors);
		free(error);
		return;
	}

	/*
	**	Check no recursion, or returning unwanted.
	*/

	if ( Returning || (HdrFlags & HDR_NORET) )
	{
		if ( Returning )
			handlebad(concat("Unable to return message: ", error, NULLSTR));

		free(errors);
		free(error);
		return;
	}

	/*
	**	Save old header.
	*/

	save_header = HdrFields;

	for ( fp = HdrFields.hdr_field, i = NHDRFIELDS ; --i >= 0 ; fp++ )
		fp->hf_free = false;

	/*
	**	Set new header.
	*/

	HdrEnv = MakeEnv
		 (
			ENV_SOURCE, HdrSource,
			ENV_DEST, HdrDest,
			ENV_ERR, errors,
			ENV_ROUTE, HdrRoute,
			NULLSTR
		 );
	FreeHdrEnv = true;

	HdrDest = TypedAddress(Source_Addr);

	HdrFlags &= (HDR_NO_AUTOCALL|HDR_NOOPT);
	HdrFlags |= (HDR_RETURNED|HDR_REV_CHARGE);
	HdrRoute = EmptyString;
	HdrSource = TypedAddress(Home_Addr);
	HdrTtd = HdrTt = 0;

	(void)UpdateHeader((Ulong)0, HomeName);

	/*
	**	Spool it.
	*/

	Returning = true;

	if ( SourceIsHome )
		(void)deliver();
	else
	{
		char *	source	= newstr(UnTypName(Source_Addr));
		Addr *	ap	= StringToAddr(source, STA_MAP);
		What	result	= FindAddr(ap, &rlink, RouteType);

		if ( result == wh_home )
			(void)deliver();
		else
		if ( result == wh_link )
			spool(Home_Addr, ap, &rlink);
		else
			handlebad
			(
				concat
				(
					english("Unknown source \""),
					UnTypName(Source_Addr),
					english("\" for returning message: "),
					errors,
					NULLSTR
				)
			);

		FreeAddr(&ap);
		free(source);
	}

	Returning = false;

	/*
	**	Restore header.
	*/

	FreeHeader();
	free(errors);
	free(error);

	HdrFields = save_header;
}



/*
**	Process message.
*/

bool
route()
{
	register char *	cp;
	register int	fd;
	HdrReason	reason;
	bool		athome;
	bool		expired;
	bool		nodeliver = false;
	int		save_level;
	DODEBUG(char	dateb[DATETIMESTRLEN]);
	extern bool	NeedRemap;

	Trace1(1, "route()");
	DODEBUG(if(Reroute)Trace1(2, "Reroute"));
	DODEBUG(if(OutFiltered)Trace1(2, "OutFiltered"));

	NameProg("%s ROUTING %s", Name, CmdsFile);

	(void)CheckRoute(SUMCHECK);

	Trace3(1, "%s [%s]", CmdsFile, DateTimeStr(MesgTime, dateb));

	if ( LinkName != NULLSTR && !SubRouter )
	{
		Trace3(1, "Message %s link \"%s\"", Reroute?"rerouted from":"received via", LinkName);

		if ( !Retry )
		{
			if ( !check_link(LinkName, LINK_UP) )
			{
				if ( Finish )
					leave_cmds(0);

				return false;
			}
		}
		else
		if ( !FindLink(LinkName, &LinkData) )
		{
			LinkData.ln_rname = LinkName;
			LinkData.ln_name = WORKDIR;
		}

		if ( Filter != NULLSTR )
			LinkData.ln_filter = Filter;

		if ( LinkDir != NULLSTR )
			LinkData.ln_name = LinkDir;

		/*
		**	If a filter has been specified for link, pass it the message.
		*/

		if ( LinkData.ln_filter != NULLSTR )
		{
			switch ( dofilter(&LinkData, IN, &DataCmds, ElapsedTime) )
			{
			case EX_OK:
				break;

			case EX_DROPMESG:
				free(FilterMesg);
				return true;

			case EX_RETMESG:
				FreeStr(&BounceDesc);
				BounceDesc = FilterMesg;
				FilterMesg = NULLSTR;
				CmdFlags |= BOUNCE_MESG;
			case EX_EXMESG:
				nodeliver = true;
				Trace1(2, "NoDeliver");
				break;

			default:
				if ( Finish )
				{
					leave_cmds(0);
					return true;
				}
				handlebad(FilterMesg);
				return true;
			}

			if ( Finish )
			{
				leave_cmds(0);
				return true;
			}

			FreeStr(&FilterMesg);
		}
	}
	else
		Trace1(1, "Local message received");

	while ( (fd = open(MesgHdrFile, O_RDWR)) == SYSERROR )
		if ( !SysWarn(CouldNot, OpenStr, MesgHdrFile) )
		{
			handlebad
			(
				concat
				(
					english("Could not read headerfile \""),
					MesgHdrFile,
					"\"",
					NULLSTR
				)
			);

			return false;
		}

	reason = ReadHeader(fd);

	(void)close(fd);

	if ( reason != hr_ok )
	{
		/*
		**	Bad message header.
		**
		**	Almost no recovery possible, except to pass
		**	it to the badhandler for post-mortem.
		*/

		handlebad
		(
			concat
			(
				english("Header "),
				HeaderReason(reason),
				english(" error"),
				NULLSTR
			)
		);

		return false;
	}

	if ( HdrID[0] == '\0' && !BounceMesg )
	{
		if ( (cp = GetEnv(ENV_ID)) == NULLSTR )
		{
			/*
			**	Bad message header ID.
			*/

			handlebad(newstr(english("Header ID error")));
			return false;
		}

		HdrID = cp;
		FreeHdrID = true;
	}

	save_level = Traceflag;

	if ( TraceMatch != NULLSTR )
	{
		if
		(
			StringMatch(HdrID, TraceMatch) != NULLSTR
			||
			StringMatch(HdrSource, TraceMatch) != NULLSTR
			||
			StringMatch(HdrDest, TraceMatch) != NULLSTR
			||
			StringMatch(HdrHandler, TraceMatch) != NULLSTR
		)
			Traceflag += 3;
	}

	Trace2(1, "Message ID   %s", HdrID);
	Trace2(1, "From         %s", HdrSource);
	Trace2(1, "To           %s", HdrDest);
	Trace2(1, "For          %s", HdrHandler);

	Trace2(2, "MesgLength  %lu", (PUlong)MesgLength);
	Trace2(2, "Flags       %lx", (PUlong)HdrFlags);

	Trace2(3, "Route      %s", convert_route());

	DataLength = MesgLength - HdrLength;

	if ( !UpdateHeader(ElapsedTime, HomeName) || Timeout < Time )
	{
		Trace1(2, "Expired");
		expired = true;
	}
	else
		expired = false;

	if ( HdrFlags & HDR_RETURNED )
	{
		Trace1(2, "Returning");
		Returning = true;
	}
	else
		Returning = false;

	if ( HdrFlags & HDR_NO_AUTOCALL )
	{
		Trace1(2, "NoCall");
		NoCall = true;
	}
	else
		NoCall = false;

	if ( HdrFlags & HDR_NOOPT )
	{
		Trace1(2, "NoOpt");
		NoOpt = true;
	}
	else
		NoOpt = false;

	if ( HdrFlags & HDR_RQA )
	{
		Trace1(2, "HDR_RQA");
		CmdFlags |= NAK_ON_TIMEOUT;
	}

	if ( HdrSubpt == STATE_PROTO && strcmp(HdrHandler, STATEHANDLER) == STREQUAL )
	{
		Trace1(2, "StateMessage");
		StateMessage = true;
	}
	else
		StateMessage = false;

	RouteType = (HdrQuality <= FAST_QUALITY) ? FASTEST : CHEAPEST;

	if ( MesgDup )
	{
		char *	oldenv = HdrEnv;

		Trace1(2, "MesgDup");
		HdrEnv = concat(oldenv, cp = MakeEnv(ENV_DUP, HomeName, NULLSTR), NULLSTR);
		free(cp);
		if ( FreeHdrEnv )
			free(oldenv);
		else
			FreeHdrEnv = true;
	}

	if ( DelEnvStr != NULLSTR )
	{
		(void)DelEnv(DelEnvStr, NULLSTR);
		FreeStr(&DelEnvStr);
	}

	if ( NewEnv != NULLSTR )
	{
		cp = concat(HdrEnv, NewEnv, NULLSTR);
		if ( FreeHdrEnv )
			free(HdrEnv);
		HdrEnv = cp;
		FreeHdrEnv = true;
		FreeStr(&NewEnv);
	}

	/*
	**	Write out statistics record.
	*/

	if ( !SubRouter )
	{
		if ( (StatsLink = LinkName) == NULLSTR )
			StatsLink = EmptyString;

		StatsLength = MesgLength;
		StatsDelay = ElapsedTime;

		WrStats(ST_INMESG, UpPrint);
	}

	InMesgCount++;

#	if	DEBUG >= 1
	/*
	**	Trace.
	*/

	while ( Traceflag )
	{
		register CmdV	cmdv;
		Ulong		start;
		Ulong		length;
		CmdHead		cmds;

		if ( (length = DataLength) > 8192 )
		{
			length = 8192;
			start = DataLength - 8192;
		}
		else
			start = 0;

		fd = create(UniqueName(TraceFile, CHOOSE_QUALITY, OPTNAME, MesgLength, Time));
		(void)CollectData(&DataCmds, start, DataLength, fd, TraceFile);

		if ( WriteHeader(fd, (long)length, 0) == SYSERROR )
		{
			(void)SysWarn(CouldNot, WriteStr, TraceFile);
			(void)close(fd);
			(void)unlink(TraceFile);
			break;
		}

		length += HdrLength;
		sync_close(fd, TraceFile);

		InitCmds(&cmds);
		(void)AddCmd(&cmds, file_cmd, (cmdv.cv_name = TraceFile, cmdv));
		(void)AddCmd(&cmds, start_cmd, (cmdv.cv_number = 0, cmdv));
		(void)AddCmd(&cmds, end_cmd, (cmdv.cv_number = length, cmdv));
		(void)AddCmd(&cmds, unlink_cmd, (cmdv.cv_name = TraceFile, cmdv));
		(void)AddCmd(&cmds, timeout_cmd, (cmdv.cv_number = TRACETIMEOUT, cmdv));

#		if	PRIORITY_ROUTING == 1
		write_cmds(&cmds, UniqueName(TraceCmdF, HdrQuality, NoOpt, MesgLength, Time));
#		else	/* PRIORITY_ROUTING == 1 */
		write_cmds(&cmds, UniqueName(TraceCmdF, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time));
#		endif	/* PRIORITY_ROUTING == 1 */
		FreeCmds(&cmds);

		break;
		/* NOTREACHED */
	}
#	endif	/* DEBUG >= 1 */

	/*
	**	Extract canonical addresses.
	*/

	if ( RecoverV(ert_here) )
	{
		handlebad(newstr(AddrErr));
		Traceflag = save_level;
		return true;
	}

	if ( UseAddr != NULLSTR )
	{
		if ( FreeHdrDest )
			free(HdrDest);
		HdrDest = UseAddr;
		UseAddr = NULLSTR;
		FreeHdrDest = true;
	}

	if ( !SubRouter )
		SavHdrDest = newstr(HdrDest);

	for ( athome = SubRouter ;; )
	{
		FreeAddr(&Destination);
		Destination = StringToAddr(HdrDestStr = newstr(HdrDest), STA_MAP);

		NeedRemap = false;
		if ( HdrDest[0] == '\0' || AddrAtHome(&Destination, true, false) )
			athome = true;

		if ( !NeedRemap || Destination == (Addr *)0 )
			break;

		cp = TypedAddress(Destination);
		if ( cp == NULLSTR || cp[0] == '\0' )
			break;

		if ( FreeHdrDest )
			free(HdrDest);
		HdrDest = newstr(cp);
		FreeHdrDest = true;
		FreeStr(&HdrDestStr);
	}

	Trace2(1, "Destination \"%s\"", (Destination==(Addr *)0)?NullStr:TypedAddress(Destination));
	Broadcast = AddrIsBC(Destination);

	if ( UseSrce != NULLSTR )
	{
		if ( FreeHdrSource )
			free(HdrSource);
		HdrSource = UseSrce;
		UseSrce = NULLSTR;
		FreeHdrSource = true;
	}
	FreeAddr(&Source_Addr);
	Source_Addr = StringToAddr(HdrSourceStr = newstr(HdrSource), NO_STA_MAP);
	SourceIsHome = AddrAtHome(&Source_Addr, false, false);

	Recover(ert_return);

	/*
	**	Bounce?
	*/

	if ( BounceMesg )
	{
		Traceflag = save_level;

		if ( BounceDesc != NULLSTR )
		{
			Return(0, BounceDesc);
			BounceDesc = NULLSTR;
			return true;
		}

		handlebad(newstr(english("Message returned: reason unknown")));
		return true;
	}

	if ( NakOnTimeout && expired )
	{
		Traceflag = save_level;

		Return
		(
			0,
			concat
			(
				english("Message timed-out before delivery "),
				athome ? english("at \"") : english("to \""),
				athome ? ExHomeAddress : UnTypAddress(Destination),
				"\".",
				NULLSTR
			)
		);
		return true;
	}

	/*
	**	Check broadcast messages are new.
	*/

	if ( !Retry && UseLink == NULLSTR && Broadcast && !SourceIsHome && !SubRouter )
	{
		if ( expired )
		{
			Traceflag = save_level;
			return true;
		}

		if ( HdrTtd == 0 )
			HdrTtd = DFLT_BC_TIMEOUT;

		if
		(
			(DropState && StateMessage)
			||
			(
				LinkCount > 1	/* Save single-link sites DB overhead (?) */
				&&
				LookupMsgsDB(HdrID, UnTypName(Source_Addr), HdrTtd)
			)
		)
		{
			if ( Reportflag )
				Report5(
					english("broadcast %s dropped\n\tID:\t%s\n\tto:\t%s\n\troute:\t%s"),
					(DropState&&StateMessage)?english("state message"):english("duplicate"),
					HdrID,
					HdrDest,
					convert_route()
				);
			Traceflag = save_level;
			return true;
		}
	}

	/*
	**	If this node is in destination, pass message to handler.
	*/

	if ( (cp = GetEnv(ENV_NOTREGIONS)) != NULLSTR )
	{
		if ( StringAtHome(cp) )
		{
			Trace1(2, "NoDeliver");
			nodeliver = true;
		}
		free(cp);
	}

	if
	(
		!Delivered
		&&
		athome
		&&
		!nodeliver
		&&
		deliver()
		&&
		(StateMessage || Parallel)
		&&
		CheckRoute(SUMCHECK)
		&&
		LinkName != NULLSTR
		&&
		!FindLink(LinkName, &LinkData)
	)
	{
		LinkData.ln_rname = LinkName;
		LinkData.ln_name = WORKDIR;
	}

	if ( SubRouter )
	{
		Traceflag = save_level;
		return true;
	}

	Trace2(1, "New destination \"%s\"", (Destination==(Addr *)0)?NullStr:TypedAddress(Destination));

	/*
	**	If the message has not expired,
	**	and there are more addresses in destination,
	**	spool message for each outbound link.
	*/

	if ( expired || Destination == (Addr *)0 )
	{
		Traceflag = save_level;
		return true;
	}

	if ( Finish )
	{
		leave_cmds(RE_ROUTE|DELIVERED);
		Traceflag = save_level;
		return true;
	}

	MesgTime = Time - HdrTt;
	Time = time((time_t *)0);		/* Update Time */

	if ( UseLink == NULLSTR )
	{
		if ( RecoverV(ert_here) )
		{
			handlebad(newstr(AddrErr));
			Traceflag = save_level;
			return true;
		}

		(void)DoRoute
		      (
			Source_Addr,
			&Destination,
			RouteType,
			LinkName!=NULLSTR?&LinkData:(Link *)0,
			spool,
			addresserr
		      );

		Recover(ert_return);
		Traceflag = save_level;
		return true;
	}

	/*
	**	Use advised link.
	*/

	if ( !check_link(UseLink, 0) )
	{
		if ( Finish )
			leave_cmds(RE_ROUTE|MESG_DUP|DELIVERED);

		Traceflag = save_level;
		return false;
	}

	spool(Source_Addr, Destination, &LinkData);

	Traceflag = save_level;
	return true;
}



/*
**	Scan routing directory for messages, and route them.
*/

void
router()
{
	register int		count;
	register Mlist *	mlistp;
	register DIR *		dirp;
	Time_t			listtime;
	DODEBUG(char		dateb[DATETIMESTRLEN]);

	while ( (dirp = opendir(".")) == NULL )
		Syserror(CouldNot, ReadStr, RoutingDir);

	InitCmds(&Cleanup);
	InitCmds(&DataCmds);
	InitCmds(&FDataCmds);
	InitCmds(&FUnlinkCmds);

	for ( count = 1 ;; count = 0 )
	{
		NameProg("%s RESCAN", Name);

#		if	BAD_REWINDDIR
		closedir(dirp);
		while ( (dirp = opendir(".")) == NULL )
			Syserror(CouldNot, ReadStr, RoutingDir);
#		else	/* BAD_REWINDDIR */
		rewinddir(dirp);
#		endif	/* BAD_REWINDDIR */
		listbuild(dirp);
		listsort((char **)&MlistHead, listcmp);
		listtime = Time + RESCAN_QUEUE_TIME;

		if ( LinkCount )
			GetLink(&LinkData, 0);

		if ( CheckLicence(HomeName, LicenceNumber, (int)LinkCount, LinkData.ln_rname) == LICENCE_OK )
		for ( mlistp = MlistHead ; mlistp != (Mlist *)0 ; mlistp = mlistp->next )
		{
			DODEBUG(if(!malloc_verify())Fatal("malloc_verify() error"));

			if ( Finish )
				finish(EX_OK);	/* TERMINATE */

			if ( !write_lock(mlistp) )
				continue;

			count++;

			CmdsFile = mlistp->name;

			Trace3(1, "%s: found \"%s\"", DateTimeStr((Time_t)time((time_t *)0), dateb), CmdsFile);

			CmdFlags = 0;
			CmdsTime = 0;
			DmnPid = 0;
			ElapsedTime = 0;
			MesgHdrFile = NULLSTR;
			MesgStart = 0;
			MesgLength = 0;
			MesgTime = 0;
			Timeout = MAXTIMEOUT;

			if ( !ReadCmdsFd(CmdsFd, getcommand) || MesgHdrFile == NULLSTR || MesgLength == 0 )
			{
				if ( (CmdFlags & (LINK_DOWN|LINK_UP)) && LinkName != NULLSTR )
					(void)check_link(LinkName, LinkUp?LINK_UP:LINK_DOWN);
				else
				if ( (CmdFlags & LOW_SPACE) && LinkName != NULLSTR )
				{
					char *	cp;

					if ( (cp = MailNCC(mailspace, NCC_ADMIN)) != NULLSTR )
					{
						Error(StringFmt, cp);
						free(cp);
					}
				}
				else
					handlebad(newstr(english("bad commands file")));
				cleanup(true);
				continue;
			}

			Time = time((time_t *)0);
			ElapsedTime += Time - RdFileTime;
			Timeout += RdFileTime;
			MesgTime = CmdsTime = RdFileTime;

			DODEBUG(if(Timeout<Time)show_timeout());

			if
			(
				(Timeout >= Time || NakOnTimeout)
				&&
				!route()
				&&
				DmnPid != 0
				&&
				kill(DmnPid, SIGERROR) != SYSERROR
			)
			{
				char *	cp;

				if ( (cp = MailNCC(mailex, NCC_ADMIN)) != NULLSTR )
				{
					Error(StringFmt, cp);
					free(cp);
				}

				Report3(
					TermDmnPid,
					(LinkName==NULLSTR)?Unknown:LinkName,
					DmnPid
				);
			}

			NameProg("%s SCANNING", Name);

			cleanup(true);
			(void)FlushStats();
			DmnPid = 0;

			if ( Time > listtime && count > 10 )
				break;
		}

		if ( count == 0 )
		{
#			ifdef	SIG_SETMASK
			sigset_t	newsigset;
#			endif	/* SIG_SETMASK */

			if ( OnePass || Finish || (Time >= FinishTime && FinishTime) )
				finish(EX_OK);

			CloseMsgsDB();
			(void)CloseStats();

#			if	SIG_SETMASK
			sigemptyset(&newsigset);
			sigaddset(&newsigset, SIGALRM);
			sigaddset(&newsigset, SIGTERM);
			sigaddset(&newsigset, SIGWAKEUP);
			(void)sigprocmask(SIG_BLOCK, &newsigset, (sigset_t *)0);
#			endif	/* SIG_SETMASK */

#			if	WAIT_RESTART == 1
			if ( !setjmp(WakeJmp) )
			{
				JmpTerm = true;
#			endif	/* WAIT_RESTART == 1 */
				(void)signal(SIGWAKEUP, wakecatch);
#				ifdef	SIG_SETMASK
				(void)sigprocmask(SIG_SETMASK, &NullSigset, (sigset_t *)0);
#				endif	/* SIG_SETMASK */
				NameProg("%s WAIT", Name);
				Trace3(1, "%s: sleep %d", DateTimeStr((Time_t)time((time_t *)0), dateb), ScanRate);
				(void)sleep((unsigned)ScanRate);
				(void)signal(SIGWAKEUP, SIG_IGN);
				(void)sub_wait();
#			if	WAIT_RESTART == 1
			}

			JmpTerm = false;
#			endif	/* WAIT_RESTART == 1 */

			Time = time((time_t *)0);

			Trace2(1, "%s: wakeup", DateTimeStr(Time, dateb));

			(void)ReadRoute(SUMCHECK);
		}
		else
		{
			(void)CheckRoute(SUMCHECK);
			if ( BatchTime )
				FinishTime = Time + BatchTime;
		}

		if ( CheckHandlers() )
			sub_check();
	}
}



#if	DEBUG >= 1
/*
**	Show time-out.
*/

void
show_timeout()
{
	char	buf[TIMESTRLEN];

	Trace(1, "Message timed-out by %s", TimeString(Time-Timeout, buf));
}
#endif	/* DEBUG >= 1 */



/*
**	Called once for each link with appropriate address set in 'dest'.
**	'Link' contains the link details.
**	'DataCmds' contain details of the message.
*/

void
spool(source, dest, linkp)
	Addr *		source;
	Addr *		dest;
	register Link *	linkp;
{
	register int	fd;
	register CmdV	cmdv;
	char *		commands;
	char *		savehdrenv;
	char *		dmndir;
	char *		dmnfile;
	Ulong		datalength;
	Ulong		datastart;
	int		flags;
	CmdHead		cmds;

	Recover(ert_return);

	/*
	**	If this node is inderdicted, ignore it.
	*/

	if ( StateMessage && Broadcast && (linkp->ln_flags & (FL_DEAD|FL_FOREIGN)) )
		return;

	Trace2(1, "Message passed to link \"%s\"", linkp->ln_rname);

	dmndir = concat(SPOOLDIR, linkp->ln_name, Slash, NULLSTR);
	dmnfile = concat(dmndir, LINKSPOOLNAME, Slash, Template, NULLSTR);

	if ( FreeHdrSource )
		free(HdrSource);
	HdrSource = TypedAddress(source);
	FreeHdrSource = false;

	if ( FreeHdrDest )
		free(HdrDest);
	HdrDest = TypedAddress(dest);
	FreeHdrDest = false;

	/*
	**	If returned message is too large, truncate it.
	*/

	if ( DataLength > MaxRetSize && Returning )
	{
		savehdrenv = HdrEnv;
		commands = MakeEnv(ENV_TRUNC, HomeName, NULLSTR);
		HdrEnv = concat(savehdrenv, commands, NULLSTR);
		free(commands);
		datalength = MaxRetSize;
		datastart = DataLength - MaxRetSize;	/* Leave any sub-header intact at end */
	}
	else
	{
		savehdrenv = NULLSTR;
		datalength = DataLength;
		datastart = (Ulong)0;
	}

	/*
	**	Make links to data files.
	*/

	InitCmds(&cmds);

	LinkCmds(&DataCmds, &cmds, datastart, datastart+datalength, dmnfile, MesgTime);

	/*
	**	Make a new file for header.
	*/

	Trace4(1, "Message flags = \"%lx\"%s%s", (Ulong)HdrFlags, Returning?" RETURNING":NullStr, (HdrFlags&HDR_RETURNED)?" RETURNED":NullStr);

	fd = create(UniqueName(dmnfile, CHOOSE_QUALITY, OPTNAME, MesgLength, MesgTime));

	Trace2(1, "Write header => %s", dmnfile);

	while ( WriteHeader(fd, (long)0, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, dmnfile);

	sync_close(fd, dmnfile);

	datalength += HdrLength;	/* Now length of whole message */

	if ( savehdrenv != NULLSTR )
	{
		free(HdrEnv);
		HdrEnv = savehdrenv;
	}

	/*
	**	Make a command file for this message in a temporary directory.
	*/

	flags = RE_ROUTE|DELIVERED;
	if ( NakOnTimeout )
		flags |= NAK_ON_TIMEOUT;

	(void)AddCmd(&cmds, file_cmd, (cmdv.cv_name = dmnfile, cmdv));
	(void)AddCmd(&cmds, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&cmds, end_cmd, (cmdv.cv_number = HdrLength, cmdv));
	(void)AddCmd(&cmds, unlink_cmd, (cmdv.cv_name = dmnfile, cmdv));
	(void)AddCmd(&cmds, flags_cmd, (cmdv.cv_number = flags, cmdv));
	(void)AddCmd(&cmds, linkname_cmd, (cmdv.cv_name = linkp->ln_rname, cmdv));
	(void)AddCmd(&cmds, address_cmd, (cmdv.cv_name = HdrDest, cmdv));

	if ( HdrTtd > 0 )
		(void)AddCmd(&cmds, timeout_cmd, (cmdv.cv_number = HdrTtd, cmdv));

	(void)UniqueName(CmdTemp, CHOOSE_QUALITY, OPTNAME, MesgLength, MesgTime);

	/*
	**	If (through routing changes) this message is about
	**	to return to a node that it has already visited more than once,
	**	spool it in a holding directory.
	*/

	fd = InRoute(linkp->ln_rname, MaxLoops);
	flags = InRoute(HomeName, MaxLoops);

	if
	(
		UseLink == NULLSTR
		&&
		(
			fd > MaxLoops
			||
			(fd + flags) > (2*MaxLoops)
			||
			(fd > 1 && !EXPLTYPE(dest) && !AddressMatch(HdrDest, linkp->ln_rname))
		)
	)
	{
		char *	cp;

		if ( !NakOnTimeout )
			(void)AddCmd(&cmds, flags_cmd, (cmdv.cv_number = NAK_ON_TIMEOUT, cmdv));

		if ( HdrTtd == 0 || HdrTtd > WEEK )
			(void)AddCmd(&cmds, timeout_cmd, (cmdv.cv_number = WEEK, cmdv));

		(void)AddCmd
		      (
			&cmds,
			bounce_cmd,
			(cmdv.cv_name = concat
					(
						english("Message for \""),
						UnTypAddress(dest),
						english("\" in routing loop to \""),
						cp = StripTypes(linkp->ln_rname),
						english("\" detected at \""),
						UnTypName(Home_Addr),
						"\"",
						NULLSTR
					),
					cmdv)
		      );

		free(cmdv.cv_name);
		free(cp);

		write_cmds(&cmds, CmdTemp);

		if ( (fd+flags) < (2*MaxLoops) && ReRoutable(HdrDest) )
		{
#			if	PRIORITY_ROUTING == 1
			move(CmdTemp, UniqueName(RerouteFile, HdrQuality, NoOpt, MesgLength, MesgTime));
#			else	/* PRIORITY_ROUTING == 1 */
			move(CmdTemp, UniqueName(RerouteFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, MesgTime));
#			endif	/* PRIORITY_ROUTING == 1 */

			UpStats("REROUTED", datalength);
		}
		else
		{
#			if	PRIORITY_ROUTING == 1
			move(CmdTemp, UniqueName(PendingFile, HdrQuality, NoOpt, MesgLength, MesgTime));
#			else	/* PRIORITY_ROUTING == 1 */
			move(CmdTemp, UniqueName(PendingFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, MesgTime));
#			endif	/* PRIORITY_ROUTING == 1 */

			UpStats("LOOPING", datalength);

			looping(linkp->ln_rname, PendingFile);
		}

		goto out;
	}

	/*
	**	If a filter has been specified for link, pass it the message.
	*/

	if ( linkp->ln_filter != NULLSTR )
	{
		switch ( dofilter(linkp, OUT, &cmds, (Ulong)0) )
		{
		case EX_EXMESG:
		case EX_OK:
			break;

		case EX_DROPMESG:
			unlink_files(&cmds);
			free(FilterMesg);
			goto out;

		case EX_RETMESG:
			unlink_files(&cmds);
			Return(0, FilterMesg);
			goto out;

		default:
			unlink_files(&cmds);
			if ( Finish )
			{
				leave_cmds(MESG_DUP|RE_ROUTE);
				goto out;
			}
			flags = ExStatus;	/* Save from Execute in `handlebad' */
			handlebad(concat(english("filter error: "), FilterMesg, NULLSTR));
			Error(BadFilterMsg, flags, linkp->ln_filter, FilterMesg);
			cleanup(false);	/* Get rid of bad message! */
			finish(EX_SOFTWARE);
		}

		free(FilterMesg);
	}

	if ( linkp->ln_spooler != NULLSTR )
	{
		/*
		**	This node requires a non-standard spooler to be invoked.
		*/

		if ( invoke_spooler(linkp, &cmds) )
			UpStats(linkp->ln_rname, datalength);

		unlink_files(&cmds);

		goto out;
	}

	/*
	**	Move commands into daemon's de-spooling directory.
	*/

	write_cmds(&cmds, CmdTemp);

	commands = UniqueName
		   (
			concat(dmndir, LINKCMDSNAME, Slash, Template, NULLSTR),
			HdrQuality, NoOpt,
			StateMessage?(Ulong)0:MesgLength, MesgTime
		   );

	move(CmdTemp, commands);

	start_daemon(linkp, dmndir, commands);

	UpStats(linkp->ln_rname, datalength);

	Trace3(
		1,
		"message spooled in \"%s\" for \"%s\"",
		commands,
		HdrDest
	);

	free(commands);

out:
	FreeCmds(&cmds);
	free(dmnfile);
	free(dmndir);
}



/*
**	Wakeup daemon, or, if link is "call-on-demand", start a caller.
**
**	Caller should establish lock before returning below.
*/

void
start_daemon(linkp, dmndir, commands)
	register Link *	linkp;
	char *		dmndir;
	char *		commands;
{
	register char *	cp;
	register int	i;
	char *		callfile;
	char *		caller;
	char *		errs;

	Trace4(
		2, "start_daemon(%s, %s, %s)",
		(Finish||NoCall||!(linkp->ln_flags&FL_CALL))?"false":"true",
		dmndir, commands
	);

	cp = strrchr(commands, '/') + 1;
	i = *cp;
	*cp = '\0';

	if
	(
		Finish
		||
		DaemonActive(commands, true)	/* Will wakeup any sleeping daemon */
		||
		NoCall
		||
		!(linkp->ln_flags & FL_CALL)
	)
	{
		*cp = i;
		return;
	}

	*cp = i;

	if ( Reportflag )
		Report2(english("Starting call for link \"%s\""), linkp->ln_rname);

	callfile = concat(dmndir, CALLFILE, NULLSTR);

	if ( (i = access(callfile, 04)) == SYSERROR && linkp->ln_caller == NULLSTR )
	{
		caller = StripTypes(linkp->ln_rname);

		FIRSTARG(&ExArgs.ex_cmd) = NetControl;
		NEXT_ARG = "start";
		NEXT_ARG = caller;
	}
	else
	{
		if ( (caller = linkp->ln_caller) != NULLSTR )
		{
			if ( caller[0] == '/' )
				caller = newstr(caller);
			else
				caller = concat(SPOOLDIR, caller, NULLSTR);
		}
		else
			caller = newstr(VCcall);

		FIRSTARG(&ExArgs.ex_cmd) = caller;
		NEXT_ARG = Flag__;	/* -& */
		NEXT_ARG = Flag_H;
		NEXT_ARG = TypedName(Home_Addr);
		NEXT_ARG = Flag_L;
		NEXT_ARG = linkp->ln_rname;
		NEXT_ARG = Flag_N;
		NEXT_ARG = linkp->ln_name;
		if ( i != SYSERROR )
		{
			NEXT_ARG = Flag_S;
			NEXT_ARG = callfile;
		}
	}

	if ( (errs = Execute(&ExArgs, child_alarm, ex_exec, SYSERROR)) != NULLSTR )
	{
		Warn(StringFmt, errs);
		free(errs);
	}

	free(caller);

	free(callfile);
}



/*
**	Start sub-routers for each existing handler directory if first time,
**	otherwise merge new handler info.
**
**	(`purge' should clean these directories and remove?)
*/

void
sub_check()
{
	register int		i;
	register ExBuf *	ep;
	register ExBuf *	op;
	register ExSubr *	er;
	ExBuf *			oldarray;
	size_t			oldcount;
	ExSubr			subrdata;

	Trace1(1, "sub_check()");

	if ( SubRouter )
	{
		if
		(
			(HndlrP = GetHandler(HndlrName)) == (Handler *)0
			||
			!HndlrP->router
		)
		{
			Finish = true;
			Report(english("sub-router disabled in handlers file, terminating"));
		}

		return;
	}

	oldarray = HndArray;
	oldcount = HndArrCount;

	(void)GetHandler(EmptyString);	/* Initialise handler array */

	if ( (HndArrCount = HandlCount) == 0 )
		HndArray = (ExBuf *)0;
	else
		HndArray = (ExBuf *)Malloc0(sizeof(ExBuf)*HndArrCount);

	for ( HndlrP = Handlers, i = HandlCount ; --i >= 0 ; HndlrP++ )
	{
		ep = &HndArray[HndlrP->index];

		subrdata.sr_handler = HndlrP->handler;
		ep->ex_sig = (SigFunc *)&subrdata;

		if
		(
			oldcount
			&&
			(op = (ExBuf *)lfind(
					(void *)ep,
					(void *)oldarray,
					&oldcount,
					sizeof(ExBuf),
					sub_cmp
					)
			) != (ExBuf *)0
		)
		{
			*ep = *op;
			op->ex_sig = (SigFunc *)0;
		}
		else
		{
			er = Talloc(ExSubr);
			er->sr_handler = newstr(HndlrP->handler);
			er->sr_cmdfile = concat(HndlrP->handler, Slash, Template, NULLSTR);
			er->sr_action = Time + HndlrP->duration;
			ep->ex_sig = (SigFunc *)er;

			if ( oldarray == (ExBuf *)0 && IsDir(EXSUBR(ep)->sr_handler) )
				sub_start(ep);
		}
	}

	if ( (ep = oldarray) != (ExBuf *)0 )
	{
		for ( i = oldcount ; --i >= 0 ; ep++ )
		{
			if ( (er = EXSUBR(ep)) == (ExSubr *)0 )
				continue;
			free(er->sr_cmdfile);
			free(er->sr_handler);
			free((char *)er);
		}

		free((char *)oldarray);

		Report(english("read new handlers file"));
	}
}



/*
**	Take child from sub_wait(), or unknown child from ExClose().
*/

void
sub_child(pid, status)
	int	pid;
	int	status;
{
	ExBuf *	ep;

	if ( (ep = sub_collect(pid, status)) == (ExBuf *)0 )
		return;

	if
	(
		(HndlrP = GetHandler(EXSUBR(ep)->sr_handler)) != (Handler *)0
		&&
		HndlrP->router
		&&
		HndlrP->duration == 0
		&&
		!Finish
		&&
		!CheckHandlers()
	)
	{
		Warn(english("Unexpected return from sub-router for \"%s\""), EXSUBR(ep)->sr_handler);
		finish(EX_OSERR);
	}

	if ( Reportflag )
		Report(SubrMesg, EXSUBR(ep)->sr_handler, pid, english("finished"));
}



int
sub_cmp(hp1, hp2)
	const void *	hp1;
	const void *	hp2;
{
	ExSubr *e1 = EXSUBR((ExBuf *)hp1);
	ExSubr *e2 = EXSUBR((ExBuf *)hp2);
	char *	h1;
	char *	h2;

	if ( e2 == (ExSubr *)0 )
		return 1;	/* Already matched and removed */

	h1 = e1->sr_handler;
	h2 = e2->sr_handler;

	Trace3(4, "sub_cmp(%s, %s)", h1, h2);

	return strcmp(h1, h2);
}



/*
**	Collect sub-router detritus.
*/

ExBuf *
sub_collect(pid, status)
	register int		pid;
	int			status;
{
	register int		i;
	register ExBuf *	ep;
	char *			errs;

	Trace3(1, "sub_collect(%d, %#x)", pid, status);

	for ( ep = HndArray, i = HndArrCount ; --i >= 0 ; ep++ )
		if ( ep->ex_pid == pid )
		{
			ep->ex_pid = 0;

			if ( status )
			{
				ExStatus = ((status >> 8) & 0xff) | ((status & 0xff) << 8);
				errs = GetErrFile(&ep->ex_cmd, status, ep->ex_efd);
				Warn(StringFmt, errs);
				free(errs);
			}
			else
			if ( Traceflag )
				(void)close(ep->ex_efd);

			ep->ex_efd = SYSERROR;
			return ep;
		}

	return (ExBuf *)0;	/* Some other process */
}



/*
**	Pass message to sub_router for asynchronous delivery.
*/

bool
sub_router()
{
	register ExBuf *ep;
	register CmdV	cmdv;
	register int	fd;
	char *		sav_dest;
	CmdHead		cmds;

	Trace2(1, "sub_router() for %s", HndlrP->handler);

	/*
	**	Make links to data files.
	*/

	InitCmds(&cmds);

	LinkCmds(&DataCmds, &cmds, (Ulong)0, DataLength, MesgTemp, MesgTime);

	/*
	**	Make a new file for header.
	*/

	fd = create(UniqueName(MesgTemp, CHOOSE_QUALITY, OPTNAME, MesgLength, MesgTime));

	sav_dest = HdrDest;
	HdrDest = SavHdrDest;

	while ( WriteHeader(fd, (long)0, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, MesgTemp);

	HdrDest = sav_dest;

	sync_close(fd, MesgTemp);

	/*
	**	Make a command file for this message in a temporary directory.
	*/

	(void)AddCmd(&cmds, file_cmd, (cmdv.cv_name = MesgTemp, cmdv));
	(void)AddCmd(&cmds, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&cmds, end_cmd, (cmdv.cv_number = HdrLength, cmdv));
	(void)AddCmd(&cmds, unlink_cmd, (cmdv.cv_name = MesgTemp, cmdv));
	if ( LinkName != NULLSTR )
		(void)AddCmd(&cmds, linkname_cmd, (cmdv.cv_name = LinkName, cmdv));

	(void)UniqueName(CmdTemp, CHOOSE_QUALITY, OPTNAME, MesgLength, MesgTime);

	write_cmds(&cmds, CmdTemp);

	FreeCmds(&cmds);

	/*
	**	Move command file into sub-router directory and wake it.
	*/

	ep = &HndArray[HndlrP->index];

#	if	PRIORITY_ROUTING == 1
	move(CmdTemp, UniqueName(EXSUBR(ep)->sr_cmdfile, HdrQuality, NoOpt, MesgLength, MesgTime));
#	else	/* PRIORITY_ROUTING == 1 */
	move(CmdTemp, UniqueName(EXSUBR(ep)->sr_cmdfile, STATE_QUALITY, NOOPTNAME, (Ulong)0, MesgTime));
#	endif	/* PRIORITY_ROUTING == 1 */

	if ( ep->ex_pid != 0 && HndlrP->duration > 0 && EXSUBR(ep)->sr_action <= Time )
		(void)sub_wait();

	if ( ep->ex_pid != 0 )
	{
		fd = kill(ep->ex_pid, SIGWAKEUP);
		Trace4(2, "sub_router() kill(%d, %d) => %d", ep->ex_pid, SIGWAKEUP, fd);
		if ( fd == SYSERROR )
		{
			Warn("Sub-router for %s disappeared!", EXSUBR(ep)->sr_handler);
			sub_collect(ep->ex_pid, 0);
		}
		else
			EXSUBR(ep)->sr_action = Time + HndlrP->duration;
	}

	if ( ep->ex_pid == 0 )
		sub_start(ep);

	return true;
}



/*
**	Start a sub-router for a handler.
*/

void
sub_start(ep)
	register ExBuf *ep;
{
	register Ulong	ul;
	char *		lockfile;
	bool		val;
	static char	Abuf[NUMERICARGLEN];	/* `static' so that strings are saved for GetErrFile() */
	static char	Bbuf[NUMERICARGLEN];	/* NB: these values will be same every call */
	static char	Nbuf[NUMERICARGLEN];
	static char	Pbuf[NUMERICARGLEN];
	static char	Sbuf[NUMERICARGLEN];
	DODEBUG(static char	Tbuf[NUMERICARGLEN]);

	Trace2(1, "sub_start(%s)", EXSUBR(ep)->sr_handler);

	if ( HndlrP->proto_type == STATE_PROTO )
		lockfile = LOCKFILE;	/* Only one router delivers state messages */
	else
		lockfile = LockName;

	lockfile = concat(EXSUBR(ep)->sr_handler, Slash, lockfile, NULLSTR);
	val = SetLock(lockfile, 0, true);
	free(lockfile);

	if ( !val )
		return;				/* Try later */

	FIRSTARG(&ep->ex_cmd) = Router;		/* Self */
	NEXTARG(&ep->ex_cmd) = Flag_f;		/* No fork */

	if ( OnePass || !HndlrP->router )
		NEXTARG(&ep->ex_cmd) = Flag_o;	/* One pass */

	if ( !Reportflag )
		NEXTARG(&ep->ex_cmd) = Flag_r;	/* No reports */

	NEXTARG(&ep->ex_cmd) = Flag_s;		/* Sub_router */

	if ( !Stats )
		NEXTARG(&ep->ex_cmd) = Flag_x;	/* No stats */

	if ( (ul = ChildTimeout) != CHILD_TIMEOUT )
		NEXTARG(&ep->ex_cmd) = NumericArg(Abuf, 'A', ul);;

	if ( (ul = BatchTime) || (ul = HndlrP->duration) )
		NEXTARG(&ep->ex_cmd) = NumericArg(Bbuf, 'B', ul);

	NEXTARG(&ep->ex_cmd) = Flag_D;		/* Use directory with same name as handler */
	NEXTARG(&ep->ex_cmd) = EXSUBR(ep)->sr_handler;

	if ( Parallel && HndlrP->proto_type != STATE_PROTO )
		NEXTARG(&ep->ex_cmd) = NumericArg(Nbuf, 'N', (Ulong)Parallel);

	NEXTARG(&ep->ex_cmd) = NumericArg(Pbuf, 'P', (Ulong)Pid);

	if ( SaveRoute != NULLSTR )
	{
		NEXTARG(&ep->ex_cmd) = Flag_R;	/* Alternate routefile */
		NEXTARG(&ep->ex_cmd) = SaveRoute;
	}

	if ( (ul = ScanRate) != SCAN_RATE )
		NEXTARG(&ep->ex_cmd) = NumericArg(Sbuf, 'S', ul);

	DODEBUG(if((ul=Traceflag)!=0)NEXTARG(&ep->ex_cmd) = NumericArg(Tbuf, 'T', ul));

	(void)Execute(ep, NULLVFUNCP, ex_nowait, SYSERROR);

	if ( Reportflag )
		Report(SubrMesg, EXSUBR(ep)->sr_handler, ep->ex_pid, english("started"));

	if ( Traceflag == 0 )
	{
		(void)close(ep->ex_efd);
		ep->ex_efd = SYSERROR;
	}

	EXSUBR(ep)->sr_action = Time + HndlrP->duration;
}



/*
**	Terminate all sub-routers.
*/

void
sub_term()
{
	register int		i;
	register ExBuf *	ep;
	register int		pid;
	register int		procs;
	int			status;
	bool			first;

	Trace1(1, "sub_term()");

	if ( HndArray == (ExBuf *)0 )
		return;

	first = true;

	if ( setjmp(AlrmJmp) )
	{
		if ( !first )
		{
			Warn(english("Some sub-routers wouldn't die:"));

			if ( !Stats )
				return;

			for ( ep = HndArray, i = HndArrCount ; --i >= 0 ; ep++ )
			{
				if ( ep->ex_pid == 0 )
					continue;

				Report
				(
					english("\t\"%s\" [%d]"),
					EXSUBR(ep)->sr_handler,
					ep->ex_pid
				);
			}

			return;
		}

		first = false;
	}

	for ( procs = 0, ep = HndArray, i = HndArrCount ; --i >= 0 ; ep++ )
	{
		if ( ep->ex_pid == 0 )
			continue;

		Trace2(2, "sub_term: kill %d", ep->ex_pid);

		if ( kill(ep->ex_pid, SIGTERM) != SYSERROR || first )
			procs++;
	}

	(void)signal(SIGALRM, alrmcatch);
#	ifdef	SIG_SETMASK
	(void)sigprocmask(SIG_SETMASK, &NullSigset, (sigset_t *)0);
#	endif	/* SIG_SETMASK */

	while ( procs > 0 )
	{
		(void)alarm((unsigned)10);

		pid = wait(&status);

		(void)alarm((unsigned)0);

		if ( pid == SYSERROR )
			break;

		if ( sub_collect(pid, status) != (ExBuf *)0 )
			procs--;
	}

	(void)signal(SIGALRM, SIG_IGN);
}



/*
**	Check and restart any sub-routers.
*/

bool
sub_wait()
{
	register int		i;
	register ExBuf *	ep;
	register int		pid;
	int			status;
	DODEBUG(char		dateb[DATETIMESTRLEN]);

	Trace1(2, "sub_wait()");

	if ( SubRouter )
	{
		/** "getppid" is because of bug in some OS's treatment of kill! **/

		if ( getppid() != Parent || kill(Parent, SIG0) == SYSERROR )
			finish(EX_OK);

		return false;
	}

	if ( (ep = HndArray) == (ExBuf *)0 )
		return false;

	for ( i = HndArrCount ; --i >= 0 ; ep++ )
		if ( ep->ex_pid != 0 )
			break;

	if ( i < 0 )
		return false;

	if ( setjmp(AlrmJmp) )
		return false;

	Trace2(1, "%s: sub_waiting", DateTimeStr((Time_t)time((time_t *)0), dateb));

	(void)signal(SIGALRM, alrmcatch);
#	ifdef	SIG_SETMASK
	(void)sigprocmask(SIG_SETMASK, &NullSigset, (sigset_t *)0);
#	endif	/* SIG_SETMASK */
	(void)alarm((unsigned)MINSLEEP+1);

	pid = wait(&status);

	(void)alarm((unsigned)0);
	(void)signal(SIGALRM, SIG_IGN);

	if ( pid != SYSERROR )
		sub_child(pid, status);

	return false;
}



/*
**	Sync and close data file.
*/

void
sync_close(fd, file)
	int	fd;
	char *	file;
{
#	if	FSYNC_2
	while ( fsync(fd) == SYSERROR )
		Syserror(CouldNot, "fsync", file);
#	endif	/* FSYNC_2 */

	(void)close(fd);
}



/*
**	Catch system termination and wind up.
*/

void
termcatch(sig)
	int	sig;
{
	DODEBUG(char dateb[DATETIMESTRLEN]);

	(void)signal(sig, SIG_IGN);
	Trace3(1, "%s: SIGTERM (%d)", DateTimeStr((Time_t)time((time_t *)0), dateb), sig);
	FinishReason = english("TERMINATED");
	Finish = true;
#	if	WAIT_RESTART == 1
	if ( JmpTerm )
	{
		(void)alarm((unsigned)0);
		(void)longjmp(WakeJmp, sig);
	}
#	endif	/* WAIT_RESTART == 1 */
}



/*
**	Un-write lock file, locked by write_lock().
*/

void
un_lock(file)
	char *	file;
{
	if ( CmdsFd == SYSERROR )
		return;

	Trace2(2, "un_lock(%s)", file);

	(void)close(CmdsFd);	/* Will undo lock */
	CmdsFd = SYSERROR;
}



/*
**	Do unlink commands.
*/

void
unlink_files(cmds)
	CmdHead *	cmds;
{
	register Cmd *	cmdp;

	Trace2(3, "unlink_files(%#lx)", (PUlong)cmds);

	for ( cmdp = cmds->ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
		{
			(void)unlink(cmdp->cd_value);
			Trace2(2, "unlink(%s)", cmdp->cd_value);
		}
}



/*
**	Remove an unlink command if file name matches.
*/

void
unlink_remove(file)
	char *		file;
{
	register Cmd *	cmdp;

	Trace2(3, "unlink_remove(%s)", file);

	for ( cmdp = Cleanup.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
		{
			if ( strcmp(cmdp->cd_value, file) != STREQUAL )
				continue;
			cmdp->cd_type = file_cmd;	/* Render it innocuous */
			(void)SysWarn(CouldNot, english("link"), file);
			return;
		}
}



/*
**	Called from "WriteStats()" to put out statistics fields.
*/

bool
UpPrint(fd, count)
	FILE *		fd;
	int		count;
{
	register char *	cp;
	static char *	string	= "%s";
	static char *	number	= "%lu";

	switch ( (StMesg_t)count )
	{
	case sm_delay:	(void)fprintf(fd, number, (PUlong)StatsDelay);	break;
	case sm_dest:	(void)fprintf(fd, string, HdrDest);		break;
	case sm_flags:	(void)fprintf(fd, number, (PUlong)HdrFlags);	break;
	case sm_handler:(void)fprintf(fd, string, HdrHandler);		break;
	case sm_id:	(void)fprintf(fd, string, HdrID);
			if ( HdrPart[0] != '\0' )
			{
				if ( (cp = strchr(HdrPart, ST_RE_SEP)) != NULLSTR )
					(void)fprintf(fd, "/%.*s;%s", (int)(cp-HdrPart), HdrPart, cp+1);
				else
					(void)fprintf(fd, "/%s", HdrPart);
			}
			break;
	case sm_link:	(void)fprintf(fd, string, StatsLink);		break;
	case sm_size:	(void)fprintf(fd, number, (PUlong)StatsLength);	break;
	case sm_source:	(void)fprintf(fd, string, HdrSource);		break;
	case sm_time:	(void)fprintf(fd, number, (PUlong)Time);	break;
	case sm_tt:	(void)fprintf(fd, number, (PUlong)HdrTt);	break;
	}

	if ( count < (int)sm_last )
		return true;

	return false;
}



/*
**	Write out a statistics record for outbound message.
*/

void
UpStats(link, length)
	char *	link;
	Ulong	length;
{
	StatsLink = link;
	StatsLength = length;
	StatsDelay = 0;

	WrStats(ST_OUTMESG, UpPrint);
	OutMesgCount++;
}



/*
**	Catch SIGWAKEUP.
*/

void
wakecatch(sig)
	int	sig;
{
	DODEBUG(char dateb[DATETIMESTRLEN]);

	(void)signal(sig, SIG_IGN);
	Trace3(1, "%s: SIGWAKEUP (%d)", DateTimeStr((Time_t)time((time_t *)0), dateb), sig);
#	if	WAIT_RESTART == 1
	(void)alarm((unsigned)0);
	(void)longjmp(WakeJmp, sig);
#	endif	/* WAIT_RESTART == 1 */
}



/*
**	Create, write, close, a commands file.
*/

void
write_cmds(cmdh, file)
	CmdHead *	cmdh;
	char *		file;
{
	register int	fd;

	fd = create(file);

	if ( !WriteCmds(cmdh, fd, file) )
		finish(EX_OSERR);

	sync_close(fd, file);
}



/*
**	Write lock file, remember fd.
*/

bool
write_lock(file)
	Mlist *		file;
{
	register int	fd;
#	ifdef	F_SETLKW
	struct flock	l;
#	endif	/* F_SETLKW */

	if ( file->state != new )
		return false;

	file->state = processed;

	while ( (fd = open(file->name, O_RDWR)) == SYSERROR )
	{
		if ( errno == ENOENT )
			return false;	/* Someone took it */

		if ( !SysWarn(CouldNot, OpenStr, file->name) )
			return true;	/* This will be passed to badhandler */
	}

#	ifdef	F_SETLKW
	if ( Parallel )
	{
		l.l_whence = 0;
		l.l_start = 0;
		l.l_len = 0;
		l.l_type = F_WRLCK;

		if ( fcntl(fd, F_SETLK, &l) == SYSERROR )
		{
			Trace3(2, "write_lock(%s) ALREADY LOCKED: %s", file->name, ErrnoStr(errno));
			(void)close(fd);
			file->state = locked;
			return false;
		}
	}

	Trace2(2, "write_lock(%s)", file->name);
#	endif	/* F_SETLKW */

	CmdsFd = fd;
	return true;
}
