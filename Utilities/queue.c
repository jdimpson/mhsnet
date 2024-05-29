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
**	Manipulate command files describing messages.
**
**	SETUID ==> NETUID.
*/

#define	FILE_CONTROL
#define	SIGNALS
#define	STAT_CALL
#define	STDARGS
#define	STDIO
#define	TIME
#define	ERRNO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"ftheader.h"
#include	"handlers.h"
#include	"header.h"
#include	"link.h"
#include	"lockfile.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sub_proto.h"
#include	"sysexits.h"
#if	SUN3 == 1
#include	"sun3.h"
#endif	/* SUN3 == 1 */

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


#include	<ndir.h>


/*
**	Arguments.
*/

char *	AdvUseLink;			/* For re-routing messages */
bool	All;				/* Print out all messages found */
bool	AllQueues;			/* Search all message queues */
bool	Bad;				/* Search BADDIR */
bool	CheckBad;			/* Process ``_bad'' directory */
bool	Continuous;			/* Keep scanning queues */
bool	Delete;				/* Delete the lot */
char *	DestAddress;			/* Look for messages to a particular destination */
bool	Fast;				/* Don't search messages */
bool	IgnErr;				/* Ignore (header) error */
bool	Init;				/* Look for active netinit */
bool	MessageDetails;			/* Print header details from messages */
char *	Name	= sccsid;		/* Program invoked name */
bool	NoRet;				/* Don't return messages, just remove */
bool	Pending;			/* Look for looping messages */
char	Quality;			/* Priority for routing */
char *	RHandler;			/* Restrict messages to those for this handler */
bool	Reroute;			/* Look for re-routed messages */
bool	Return_count;			/* Exit value == queued message count (max 127) */
bool	Routing;			/* Show messages in router's queue */
bool	ShowCmds;			/* Show commands from commands file */
bool	ShowHeader;			/* Show all header fields */
char *	SourceAddress;			/* Look for messages from a particular source */
bool	Stop;				/* Stop selected messages */
char *	StopMesg;			/* Reason for return of non-local message */
bool	Silent;				/* Quietly */
int	SleepTime;			/* Delay for continuous mode scans */
int	Traceflag;			/* Trace level */
bool	Traces;				/* Show messages in _trace */
char *	UserName;			/* Different user to look for */
bool	Verbose;			/* Print all details from ftp header */
bool	Warnings;			/* Whinge */
bool	Yes;				/* Yes to all questions */

AFuncv	getAdvUseLink _FA_((PassVal, Pointer));	/* Check advisory link */
AFuncv	getDelete _FA_((PassVal, Pointer));	/* Accept if string is "elete" */
AFuncv	getHandler _FA_((PassVal, Pointer));	/* Set different handler */
AFuncv	getLink _FA_((PassVal, Pointer));	/* Link or destination */
AFuncv	getQuality _FA_((PassVal, Pointer));	/* Message priority */
AFuncv	getSleep _FA_((PassVal, Pointer));	/* Set scan rate */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_string(d, 0, getDelete, english("elete"), OPTARG),
	Arg_bool(a, &All, 0),
	Arg_bool(b, &Bad, 0),
	Arg_bool(c, &Return_count, 0),
	Arg_bool(e, &IgnErr, 0),
	Arg_bool(f, &Fast, 0),
	Arg_bool(h, &ShowHeader, 0),
	Arg_bool(i, &Init, 0),
	Arg_bool(k, &ShowCmds, 0),
	Arg_bool(l, &Pending, 0),
	Arg_bool(m, &MessageDetails, 0),
	Arg_bool(n, &NoRet, 0),
	Arg_bool(p, &Pending, 0),
	Arg_bool(q, &Routing, 0),
	Arg_bool(r, &Reroute, 0),
	Arg_bool(s, &Silent, 0),
	Arg_bool(t, &Traces, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_bool(w, &Warnings, 0),
	Arg_bool(x, &AllQueues, 0),
	Arg_bool(y, &Yes, 0),
	Arg_string(A, 0, getHandler, english("handler"), OPTARG),
	Arg_int(C, &SleepTime, getSleep, english("seconds"), OPTARG|OPTVAL),
	Arg_string(D, &DestAddress, 0, english("destination"), OPTARG),
	Arg_string(L, 0, getAdvUseLink, english("use-link"), OPTARG),
	Arg_char(P, &Quality, getQuality, english("priority"), OPTARG),
	Arg_string(R, &StopMesg, 0, english("stop reason"), OPTARG),
	Arg_string(S, &SourceAddress, 0, english("source"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_string(U, &UserName, 0, english("user"), OPTARG),
	Arg_noflag(0, getLink, english("address"), OPTARG|MANY),
	Arg_end
};

/*
**	Name variations.
*/

bool	set_stop _FA_((void)), set_queue _FA_((void)), set_checkbad _FA_((void));

struct Names
{
	char *	n_match;
	bool	(*n_setup)_FA_((void));
}
	Names[]	=
{
	{"stop",	set_stop	},
	{"bad",		set_checkbad	},
	{NULLSTR,	set_queue	}	/* Default */
};

/*
**	Message manipulating commands:
*/

typedef enum
{
	r_yes, r_no, r_remove, r_reroute, r_return, r_stop, r_quit, r_reason, r_address, r_source, r_linkname
}
	Cmdtyp;

typedef struct
{
	char *	c_name;
	Cmdtyp	c_type;
	int	c_minl;		/* Minimum length */
	bool	c_show;		/* Explain? */
	bool	c_stop;		/* Legal for Stop? */
}
	Command;

typedef Command * Cmd_p;

/*
**	Sorted list of commands.
**
**	english( Sort this array ).
*/

Command	Cmds[] =
{
	{english("address"),	r_address,	1,	true,	false},
	{english("delete"),	r_remove,	1,	true,	true},
	{english("linkname"),	r_linkname,	1,	true,	false},
	{english("no"),		r_no,		1,	true,	true},
	{english("quit"),	r_quit,		1,	true,	true},
	{english("reason"),	r_reason,	3,	true,	true},
	{english("remove"),	r_remove,	3,	true,	true},
	{english("reroute"),	r_reroute,	3,	true,	false},
	{english("return"),	r_return,	3,	true,	true},
	{english("source"),	r_source,	2,	true,	false},
	{english("stop"),	r_stop,		2,	true,	true},
	{english("x"),		r_quit,		1,	false,	true},
	{english("yes"),	r_yes,		1,	true,	true}
};

#define	CMDZ	(sizeof(Command))
#define	NCMDS	((sizeof Cmds)/CMDZ)

#define	MAXCOMLEN	8
#define	MINCOMLEN	1

/*
**	List for found links.
*/

typedef struct Lnk	Lnk;
struct Lnk
{
	Lnk *	lnk_next;
	char *	lnk_dir;		/* Link directory */
	char *	lnk_cmds;		/* Commands directory */
	char *	lnk_name;		/* Real name */
};

Lnk *	LinkList;			/* Head of linked list */
Lnk **	Link_Vec;			/* Vector of sorted links */
int	NLinks;				/* No. of links found */

/*
**	Structure for message commandfile names in a queue.
*/

typedef struct MsgQ	MsgQ;
struct MsgQ
{
	MsgQ *	mq_next;
	char	mq_name[UNIQUE_NAME_LENGTH+1];
};

/*
**	Parameters set from current message.
*/

char *	BounceMesg;			/* Error message for this message */
Ulong	CmdFileTime;			/* Modify time of command file for message */
Ulong	CmdFlags;			/* From command file */
char *	CmdUseAddr;			/* New destination address from commands file */
char *	CmdUseLink;			/* Advisory link from commands file */
char *	CmdUseSrce;			/* New source address from commands file */
CmdHead	Commands;			/* Sanitised commands */
Ulong	ElapsedTime;			/* Time waiting */
char *	From;				/* Message sender */
char *	HeaderFile;			/* Current message header */
Ulong	HeaderEnd;			/* Start address in message for HeaderFile contents */
char *	LinkName;			/* Link name from ``linkname_cmd'' */
char *	MesgAddress;			/* Destination address for this message */
Ulong	MesgLength;			/* Total size of message */
char *	OrigBounce;			/* First error message for this message (if > 1) */
bool	OwnMesg;			/* Message belongs to invoker */
char *	SubFile;			/* Sub-protocol file */
Ulong	SubEnd;				/* Start address in message for SubFile contents */
Ulong	Time_to_die;			/* From timeout commands */
char *	To;				/* Message receiver */
CmdHead	Unlink;				/* If message removed */

/*
**	Miscellaneous data.
*/

char	FormatL[]	= english("%-20.20s %9s %15s%-10s %4d message%c in queue\n");
char	FormatH[]	= english("\n      What    From              To                At                     Bytes\n");
char	Format[]	= "%4d: %-7s %-17.17s %-17.17s %-17.17s %10lu\n";
#define	FORMATDESTLEN	17
char	FormatT[]	= english("%4d  Total%67lu.\n");
char	FormatV[]	= english("%4d: %s from %s@%s to %s at %s  %lu bytes\n");

char	RemoveStr[]	= english("remove ? [y/n/? def:n] ");
char	RerouteStr[]	= english("reroute ? [y/n/? def:n] ");
char	StopeStr[]	= english("stop ? [y/n/? def:n] ");

int	ArgCount;			/* Count of link/address arguments */
bool	CheckVal;			/* Value returned by CheckAddr */
char *	CmdFn;				/* Name of current commandsfile */
Addr *	Destinations;			/* Complex address arguments */
char *	DestSAddress;			/* Stripped DestAddress */
char *	DestTAddress;			/* Typed DestAddress */
bool	Finish;				/* Got signal to windup */
bool	Isatty;				/* Fd 0 is a terminal */
char *	KeepErrFile;			/* (Needed by some loaders) */
Link	LinkD;				/* Link descriptor */
char *	LockName;			/* Name of current lockfile */
bool	LockSet;			/* Directory locked */
Passwd	Me;				/* My name */
char *	NewHdrDest;			/* Change of address */
char *	NewHdrSource;			/* Change of address */
char *	NewStopMesg;			/* Reason for return of current message */
char *	NextWord;			/* From stdin */
char *	RealAddress;			/* Real address used to find link */
char *	RealSAddress;			/* Stripped RealAddress */
char *	ReRoutFile;			/* Spool name for rerouted messages */
int	Returned;			/* Returned message count */
int	RoutDirLen;			/* Length of string ROUTINGDIR */
char *	SourceTAddress;			/* Typed SourceAddress */
char *	SourceSAddress;			/* Stripped SourceAddress */
int	Stopped;			/* Stopped message count */
bool	SU;				/* Invoker has P_SU */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
Ulong	Tt;				/* Travel time from route */
char *	UseLink;			/* For re-routing messages */

#define	Fprintf		(void)fprintf
#define	Fflush		(void)fflush
#define	MSGINDENT	6		/* WAS: (6+7+1) */
#define	MIN_SPEED	10		/* Don't show speeds slower than this */

/*
**	Message selection routine return values.
*/

typedef enum
{
	accept, reject, error
}
	State;

/*
**	Routines
*/

void	bad_session _FA_((char *, bool));
bool	check_dest _FA_((char **));
void	check_err _FA_((char *, ...));
bool	check_link _FA_((char **));
void	errf _FA_((Addr *, Addr *));
bool	find_links _FA_((void));
char *	getname _FA_((char *));
bool	get_cmd _FA_((CmdT, CmdV));
void	linkf _FA_((Addr *, Addr *, Link *));
void	list_link _FA_((char *, char *));
void	list_sub_routers _FA_((void));
int	lnkcmp _FA_((const void *, const void *));
int	msgcmp _FA_((const void *, const void *));
int	msg_stats _FA_((Lnk *));
void	print_ftp_files _FA_((void));
void	print_header _FA_((void));
bool	print_route _FA_((char *, Ulong, char *));
int	proc_msgs _FA_((char *, int, char **));
int	read_command _FA_((char *, int, bool));
State	read_ftp _FA_((void));
State	read_header _FA_((int, bool));
Cmdtyp	response _FA_((void));
void	return_message _FA_((char *, int));
void	sort_links _FA_((void));
char **	sort_msgs _FA_((int, MsgQ *));
void	stop_session _FA_((char *));
void	unlink_message _FA_((char *));

extern SigFunc	sigcatch;



int
main(argc, argv)
	int			argc;
	char *			argv[];
{
	register struct Names *	np;
	register int		l1;
	register int		l2;
	register char *		cp;

	InitParams();
	GetNetUid();
	GetInvoker(&Me);

	while ( chdir(SPOOLDIR) == SYSERROR )
		Syserror(CouldNot, "chdir", SPOOLDIR);

	if ( !ReadRoute(NOSUMCHECK) )
	{
		Warn(english("No routefile."));
		exit(EX_OSFILE);
	}

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);

	DoArgs(argc, argv, Usage);

	if ( IgnErr )
		Warnings = true;

	if ( Destinations != (Addr *)0 )
	{
		Passwd	nullpw;
		Addr *	source = StringToAddr(newstr(HomeName), NO_STA_MAP);

		nullpw.P_name = Name;
		nullpw.P_flags = P_ALL;
		nullpw.P_region = NULLSTR;

		RealAddress = newstr(CheckAddr(&Destinations, &nullpw, Error, true));
		RealSAddress = newstr(UnTypAddress(Destinations));

		if ( Quality <= FAST_QUALITY )
			l1 = FASTEST;
		else
			l1 = CHEAPEST;

		(void)DoRoute(source, &Destinations, l1, (Link *)0, linkf, errf);

		FreeAddr(&source);
		FreeAddr(&Destinations);
	}

	for ( l2 = 0, l1 = strlen(Name), np = Names ; ; np++ )
	{
		if ( np->n_match != NULLSTR && (l2 = l1 - strlen(np->n_match)) < 0 )
			continue;

		if ( np->n_match == NULLSTR || strccmp(&Name[l2], np->n_match) == STREQUAL )
		{
			if ( !(*np->n_setup)() )
			{
				Warn(english("No permission."));
				exit(EX_NOPERM);
			}

			break;
		}
	}

	if ( !(Me.P_flags & P_SU) )
	{
		if ( UserName != NULLSTR || CheckBad )
		{
			Warn(english("No permission."));
			exit(EX_NOPERM);
		}

		/* All = false;	(Allow all) */
	}
	else
		SU = true;

	if ( UserName != NULLSTR )
		Me.P_name = UserName;

	if ( DestAddress != NULLSTR )
	{
		Destinations = StringToAddr(cp = newstr(DestAddress), NO_STA_MAP);
		DestSAddress = newstr(UnTypAddress(Destinations));
		DestTAddress = newstr(TypedAddress(Destinations));
		FreeAddr(&Destinations);
		free(cp);
	}
	else
	if ( ArgCount == 1 )
	{
		DestAddress = RealAddress;
		DestSAddress = RealSAddress;
		DestTAddress = RealAddress;
	}

	Trace3(1, "DestAddress %s ==> %s", DestAddress==NULLSTR ? NullStr : DestAddress, DestTAddress==NULLSTR ? NullStr : DestTAddress);

	if ( SourceAddress != NULLSTR )
	{
		Destinations = StringToAddr(cp = newstr(SourceAddress), NO_STA_MAP);
		SourceTAddress = newstr(TypedAddress(Destinations));
		SourceSAddress = newstr(UnTypAddress(Destinations));
		FreeAddr(&Destinations);
		free(cp);
	}

	if ( Stop || CheckBad )
	{
		if ( (SigFunc *)signal(SIGINT, SIG_IGN) != SIG_IGN )
		{
			(void)signal(SIGINT, sigcatch);
#			ifdef	SIGTSTP
			(void)signal(SIGTSTP, sigcatch);
#			endif	/* SIGTSTP */
		}
		(void)signal(SIGTERM, sigcatch);

		Isatty = (bool)isatty(fileno(stdin));

		ReRoutFile = concat(ROUTINGDIR, Template, NULLSTR);
		RoutDirLen = strlen(ROUTINGDIR);

		if ( CheckBad )
		{
			LinkList = (Lnk *)0;
			NLinks = 0;
		}

		if ( (Routing || AllQueues) && DaemonActive(ROUTINGDIR, false) )
			Error(english("routing daemon active."));
	}

	if ( Init || AllQueues )
		list_link(LIBDIR, english("*init*"));

	if ( Routing || AllQueues )
	{
		list_link(ROUTINGDIR, english("*routing*"));
		list_sub_routers();
	}

	if ( Bad || AllQueues )
		list_link(BADDIR, english("*errors*"));

	if ( Pending || AllQueues )
		list_link(PENDINGDIR, english("*looping*"));

	if ( Reroute || AllQueues )
		list_link(REROUTEDIR, english("*re-routing*"));

	if ( Traces )
		list_link(TRACEDIR, english("*traces*"));

	if ( (NLinks == 0 || AllQueues) && !find_links() )
	{
		if ( Warnings )
			Warn(english("no network links found!"));

		if ( NLinks == 0 )
			exit(EX_OSFILE);
	}

	sort_links();

	InitCmds(&Commands);
	InitCmds(&Unlink);

	setbuf(stdout, SObuf);

	for ( ;; )
	{
		for ( l2 = 0, l1 = 0 ; l1 < NLinks && !Finish ; l1++ )
		{
			l2 += msg_stats(Link_Vec[l1]);
			if ( !Continuous )
				Fflush(stdout);
		}

		if ( !Continuous )
			break;

		putc('\f', stdout);
		Fflush(stdout);
		(void)sleep(SleepTime);
		Time = time((time_t *)0);
	}

	if ( Return_count )
		finish((l2>127)?127:l2);

	finish(EX_OK);
	return 0;
}



/*
**	Try both litteral and stripped addresses.
*/

bool
address_match(a1, s1, t1, a2)
	char *	a1;
	char *	s1;
	char *	t1;
	char *	a2;
{
	char *	s2;
	bool	val;

	MatchRegion = true;

	if ( AddressMatch(a1, a2) )
		val = true;
	else
	if ( a1 != t1 && AddressMatch(t1, a2) )
		val = true;
	else
	{
		s2 = StripTypes(a2);

		val = AddressMatch(s1, s2);

		free(s2);
	}

	MatchRegion = false;

	return val;
}



/*
**	Interact for BADDIR message manipulation.
*/

void
bad_session(cmdsfile, remove)
	char *	cmdsfile;
	bool	remove;
{
	char *	cp;

	FreeStr(&NewHdrDest);
	FreeStr(&NewHdrSource);
	FreeStr(&NewStopMesg);
	FreeStr(&UseLink);

	for ( ;; )
	{
		if ( !Silent )
			Fprintf(stdout, "%s", remove?RemoveStr:RerouteStr);

		switch ( response() )
		{
		default:
			Fatal1(english("unrecognised response"));
			break;

		case r_address:
			if ( remove )
			{
				Fprintf(stdout, english("No header!\n"));
				continue;
			}
			FreeStr(&NewHdrDest);
			NewHdrDest = StripTypes(HdrDest);
			do
			{
				cp = NewHdrDest;
				NewHdrDest = getname((cp[0]=='\0')?english("address"):cp);
				Trace2(1, "=> \"%s\"", NewHdrDest==NULLSTR?NullStr:NewHdrDest);
				free(cp);
			}
			while
				( NewHdrDest != NULLSTR && NewHdrDest[0] != '\0' && !check_dest(&NewHdrDest) );
			if ( NewHdrDest != NULLSTR && NewHdrDest[0] == '\0' )
				FreeStr(&NewHdrDest);
			continue;

		case r_source:
			if ( remove )
			{
				Fprintf(stdout, english("No header!\n"));
				continue;
			}
			FreeStr(&NewHdrSource);
			NewHdrSource = StripTypes(HdrSource);
			do
			{
				cp = NewHdrSource;
				NewHdrSource = getname((cp[0]=='\0')?english("source"):cp);
				Trace2(1, "=> \"%s\"", NewHdrSource==NULLSTR?NullStr:NewHdrSource);
				free(cp);
			}
			while
				( NewHdrSource != NULLSTR && NewHdrSource[0] != '\0' && !check_dest(&NewHdrSource) );
			if ( NewHdrSource != NULLSTR && NewHdrSource[0] == '\0' )
				FreeStr(&NewHdrSource);
			continue;

		case r_yes:
			if ( !remove )
				goto reroute;
		case r_remove:
		case r_stop:
			unlink_message(cmdsfile);
			break;

		case r_return:
			if ( NextWord != NULLSTR )
			{
				FreeStr(&NewStopMesg);
				NewStopMesg = NextWord;
				NextWord = NULLSTR;
			}
			return_message(cmdsfile, BOUNCE_MESG);
			break;

		case r_reroute:
reroute:		if ( AdvUseLink != NULLSTR )
				UseLink = newstr(AdvUseLink);
			else
			if ( !StringAtHome(NewHdrDest == NULLSTR ? HdrDest : NewHdrDest) )
			{
				do
					UseLink = getname(english("link"));
				while
					( !check_link(&UseLink) );
			}
			return_message(cmdsfile, RETRY);
			FreeStr(&UseLink);
			break;

		case r_reason:
			FreeStr(&NewStopMesg);
			NewStopMesg = getname(english("reason"));
			continue;

		case r_linkname:
			FreeStr(&LinkName);
			do
				LinkName = getname(english("linkname"));
			while
				( !check_link(&LinkName) );
			continue;

		case r_no:
			break;
		}

		break;
	}
}



/*
**	Check new destination for legality, and convert to canonical form.
*/

bool
check_dest(cpp)
	char **	cpp;
{
	char *	cp;
	Addr *	ap;
	Passwd	nullpw;

	nullpw.P_name = Name;
	nullpw.P_flags = P_ALL;
	nullpw.P_region = NULLSTR;

	Trace2(1, "check_dest(%s)", *cpp==NULLSTR?NullStr:*cpp);
	CheckVal = true;
	ap = StringToAddr(*cpp, STA_MAP);
	cp = newstr(CheckAddr(&ap, &nullpw, check_err, true));
	if ( *cpp != NULLSTR )
		free(*cpp);
	*cpp = cp;
	FreeAddr(&ap);
	return CheckVal;
}



/*
**	Handle error call from CheckAddr().
*/

void
#ifdef	ANSI_C
check_err(char *fmt, ...)
{
#else	/* ANSI_C */
check_err(va_alist)
	va_dcl
{
	char *	fmt;
#endif	/* ANSI_C */
	va_list vp;

#	ifdef	ANSI_C
	va_start(vp, fmt);
#	else	/* ANSI_C */
	va_start(vp);
	fmt = va_arg(vp, char *);
#	endif	/* ANSI_C */

	(void)vfprintf(stdout, fmt, vp);
	va_end(vp);

	putc('\n', stdout);
	CheckVal = false;
}



/*
**	Check name refers to real link.
*/

bool
check_link(link)
	char **	link;
{
	Addr *	ap;

	Trace2(1, "check_link(%s)", *link);

	if ( *link == NULLSTR )
		return true;

	ap = StringToAddr(*link, NO_STA_MAP);

	if ( FindLink(TypedName(ap), &LinkD) )
	{
		FreeAddr(&ap);
		free(*link);
		*link = newstr(LinkD.ln_rname);
		return true;
	}

	FreeAddr(&ap);
	FreeStr(link);

	Fprintf(stdout, "Unknown link.\n");
	return false;
}



/*
**	Compare two commands.
*/

int
#if	ANSI_C
compare(const void *cmdp1, const void *cmdp2)
#else	/* ANSI_C */
compare(cmdp1, cmdp2)
	register char *	cmdp1;
	register char *	cmdp2;
#endif	/* ANSI_C */
{
	register int	len1 = ((Cmd_p)cmdp1)->c_minl;
	register int	len2 = ((Cmd_p)cmdp2)->c_minl;

	if ( len2 > len1 )
		len1 = len2;

	return strncmp(((Cmd_p)cmdp1)->c_name, ((Cmd_p)cmdp2)->c_name, len1);
}



/*
**	Unknown address error.
*/
/*ARGSUSED*/
void
errf(source, dest)
	Addr *	source;
	Addr *	dest;
{
	if ( AddrAtHome(&dest, false, false) )
		Error(english("This IS \"%s\"!"), UnTypAddress(dest));
	else
		Error(english("Link for \"%s\" unknown."), UnTypAddress(dest));
}



/*
**	Search routing table for links.
*/

bool
find_links()
{
	register int	i;
	struct stat	dstat;
	bool		found = false;

	Trace1(1, "find_links()");

	for ( i = LinkCount ; --i >= 0 ; )
	{
		GetLink(&LinkD, (Index)i);

		Trace2(2, "found \"%s\"", LinkD.ln_name);

		if
		(
			stat(LinkD.ln_name, &dstat) != SYSERROR
			&&
			(dstat.st_mode & S_IFMT) == S_IFDIR
		)
		{
			list_link(LinkD.ln_name, StripTypes(LinkD.ln_rname));
			found = true;
		}
	}

	return found;
}



/*
**	Cleanup for error routines
*/

void
finish(err)
	int	err;
{
	if ( LockSet )
		(void)unlink(LockName);

	if ( !Silent )
	{
		if ( Stopped )
			Fprintf
			(
				stdout,
				english("%d message%s %s.\n"),
				Stopped,
				(Stopped==1)?EmptyString:english("s"),
				CheckBad?english("removed"):english("stopped")
			);

		if ( Returned )
			Fprintf
			(
				stdout,
				english("%d message%s %s.\n"),
				Returned,
				(Returned==1)?EmptyString:english("s"),
				Stop?english("returned"):english("rerouted")
			);
	}

	(void)exit(err);
}



/*
**	Function called from "ReadCmds()" to process a command.
*/

bool
get_cmd(type, value)
	CmdT		type;
	CmdV		value;
{
	static Ulong	mesgstart;
	char		dbuf[DATETIMESTRLEN];

	if ( ShowCmds )
	{
		register char *	cp;
		register CmdC	conv;

		if ( CmdFn != NULLSTR )
		{
			Fprintf(stdout, "%s  %s:\n", CmdFn, DateTimeStr(RdFileTime, dbuf));
			CmdFn = NULLSTR;
		}

		if ( (unsigned)(type) > (unsigned)last_cmd )
			conv = error_type;
		else
			conv = CmdfTypes[(int)type];

		switch ( conv )
		{
		case number_type:
			cp = CmdToString(type, value.cv_number);
			break;

		case string_type:
			cp = value.cv_name;
			break;

		default:
			Fprintf(stdout, "\tunrecognised command type [%d]\n", (int)type);
			return true;
		}

		Fprintf(stdout, "\t%-*s", MAX_CMD_NAMES_LENGTH+1, CmdfDescs[(int)type]);
		ExpandFile(stdout, cp, -(8+MAX_CMD_NAMES_LENGTH+1));
	}

	switch ( type )
	{
	case address_cmd:
		FreeStr(&MesgAddress);
		MesgAddress = newstr(value.cv_name);
		return true;

	case bounce_cmd:
		if ( OrigBounce == NULLSTR )
			OrigBounce = BounceMesg;
		else
			FreeStr(&BounceMesg);
		BounceMesg = newstr(value.cv_name);
		return true;

	case deleted_cmd:
		return true;

	case end_cmd:
		MesgLength += value.cv_number - mesgstart;
		HeaderEnd = value.cv_number;
		break;

	case file_cmd:
		FreeStr(&SubFile);
		SubFile = HeaderFile;
		SubEnd = HeaderEnd;
		HeaderFile = newstr(value.cv_name);
		break;

	case flags_cmd:
		CmdFlags |= value.cv_number;
		return true;

	case linkname_cmd:
		FreeStr(&LinkName);
		LinkName = newstr(value.cv_name);
		return true;

	case pid_cmd:
		return true;

	case timeout_cmd:
		if ( Time_to_die == 0 || (value.cv_number < Time_to_die) )
			Time_to_die = value.cv_number;
		return true;

	case traveltime_cmd:
		ElapsedTime += value.cv_number;
		return true;

	case start_cmd:
		mesgstart = value.cv_number;
		break;

	case unlink_cmd:
		(void)AddCmd(&Unlink, type, value);
		break;

	case useaddr_cmd:
		FreeStr(&CmdUseAddr);
		CmdUseAddr = newstr(value.cv_name);
		return true;

	case usesrce_cmd:
		FreeStr(&CmdUseSrce);
		CmdUseSrce = newstr(value.cv_name);
		return true;

	case uselink_cmd:
		FreeStr(&CmdUseLink);
		CmdUseLink = newstr(value.cv_name);
		return true;

	default:
		break;	/* gcc -Wall */
	}

	(void)AddCmd(&Commands, type, value);

	return true;
}



#if	SUN3 == 1
/*
**	Special Sun3 GetEnvBool.
*/

bool
get3envbool(name)
	char *		name;
{
	register char *	ep;
	register char *	fp;
	register char *	sp;
	register int	val;

	if ( (ep = HdrEnv) == NULLSTR || *ep == '\0' )
		return false;

	for ( ;; )
	{
		if ( (sp = strchr(ep, Sun3ERs)) != NULLSTR )
			*sp = '\0';
		
		if ( (fp = strchr(ep, Sun3EUs)) != NULLSTR )
			val = strncmp(name, ep, fp-ep);
		else
			val = strcmp(name, ep);
		
		if ( val == STREQUAL )
		{
			if ( sp != NULLSTR )
				*sp = Sun3ERs;

			return true;
		}

		if ( (ep = sp) == NULLSTR )
			break;

		*ep++ = Sun3ERs;
	}

	return false;
}
#endif	/* SUN3 == 1 */



/*
**	Check advisory link.
*/

/*ARGSUSED*/
AFuncv
getAdvUseLink(val, arg)
	PassVal			val;
	Pointer			arg;
{
	AdvUseLink = newstr(val.p);

	if ( !check_link(&AdvUseLink) )
		return (AFuncv)english("unknown link");

	return ACCEPTARG;
}



/*
**	Accept if argument is "delete".
*/

/*ARGSUSED*/
AFuncv
getDelete(val, arg)
	PassVal	val;
	Pointer	arg;
{
	if ( strcmp(val.p, "elete") == STREQUAL )
	{
		Delete = true;
		Yes = true;
		return ACCEPTARG;
	}

	return REJECTARG;
}



/*
**	Set different handler.
*/

/*ARGSUSED*/
AFuncv
getHandler(val, arg)
	PassVal			val;
	Pointer			arg;
{
	register Handler *	hp;

	if
	(
		(hp = GetHandler(val.p)) == (Handler *)0
		&&
		(hp = GetDHandler(val.p)) == (Handler *)0
	)
		return (AFuncv)english("unknown handler");

	RHandler = hp->handler;
	All = true;

	return ACCEPTARG;
}



/*
**	Find name of explicit link or save address for later resolution.
*/

/*ARGSUSED*/
AFuncv
getLink(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	cp;
	register char *	name;
	register char *	link	= val.p;
	Addr *		ap;
	struct stat	statb;

	ArgCount++;

	if ( (name = strrchr(link, '/')) != NULLSTR )
		name++;
	else
		name = link;

	ap = StringToAddr(cp = newstr(name), NO_STA_MAP);

	if ( FindLink(TypedName(ap), &LinkD) )
	{
		name = StripTypes(LinkD.ln_rname);
		link = LinkD.ln_name;
	}
	else
	if
	(
		stat(link, &statb) == SYSERROR
		||
		(statb.st_mode&S_IFMT) != S_IFDIR
	)
	{
		free(cp);
		ap = StringToAddr(cp = newstr(name), STA_MAP);
		AddAddr(&Destinations, ap);
		return ACCEPTARG;
	}

	FreeAddr(&ap);
	free(cp);

	list_link(link, name);

	return ACCEPTARG;
}



/*
**	Get a name.
*/

char *
getname(type)
	char *	type;
{
	char *	cp;
	char	name[130];

	if ( (cp = NextWord) != NULLSTR )
	{
		NextWord = NULLSTR;
		return cp;
	}

	if ( Yes || Silent )
		return NULLSTR;

	Fprintf(stdout, english("New %s ? "), type);

	if ( read_command(name, sizeof name, false) > 0 )
		return newstr(name);

	return NULLSTR;
}



/*
**	Set routing priority.
*/

/*ARGSUSED*/
AFuncv
getQuality(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.c < HIGHEST_QUALITY || val.c > LOWEST_QUALITY )
	{
		static char	efmt[]	= english("priority: %c (high) to %c (low)");
		char *		errs;

		errs = Malloc(sizeof(efmt));
		(void)sprintf(errs, efmt, HIGHEST_QUALITY, LOWEST_QUALITY);
		return (AFuncv)errs;
	}

	return ACCEPTARG;
}



/*
**	Set continuous operation with optional sleep time.
*/

/*ARGSUSED*/
AFuncv
getSleep(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.l == 0 )
		SleepTime = 10;

	Continuous = true;
	return ACCEPTARG;
}



/*
**	List link from routing function.
*/
/*ARGSUSED*/
void
linkf(source, dest, link)
	Addr *		source;
	Addr *		dest;
	register Link *	link;
{
	list_link(link->ln_name, StripTypes(link->ln_rname));
}



/*
**	Queue a network link directory.
*/

void
list_link(link, name)
	register char *	link;
	char *		name;
{
	register Lnk *	lp;
	register int	i;

	Trace3(1, "list_link(%s, %s)", link, name);

	for ( lp = LinkList ; lp != (Lnk *)0 ; lp = lp->lnk_next )
		if ( strcmp(lp->lnk_name, name) == STREQUAL )
			return;

	lp = Talloc(Lnk);

	lp->lnk_name = name;

	i = strlen(link);

	if ( link[i-1] == '/' )
	{
		lp->lnk_dir = newstr(link);
		lp->lnk_cmds = lp->lnk_dir;
	}
	else
	{
		lp->lnk_dir = newnstr(link, i+2);
		lp->lnk_dir[i] = '/';
		lp->lnk_dir[++i] = '\0';
		lp->lnk_cmds = concat(lp->lnk_dir, LINKCMDSNAME, Slash, NULLSTR);
	}

	lp->lnk_next = LinkList;
	LinkList = lp;

	NLinks++;
}



/*
**	Look for active sub-routers.
*/

void
list_sub_routers()
{
	register int		i;
	register Handler *	hp;
	register char *		dir;

	Trace1(1, "list_sub_routers()");

	(void)GetHandler(EmptyString);	/* Initialise handler array */

	for ( hp = Handlers, i = HandlCount ; --i >= 0 ; hp++ )
		if ( hp->router )
		{
			dir = concat(ROUTINGDIR, hp->handler, Slash, NULLSTR);
			list_link(dir, concat("*", hp->handler, "*", NULLSTR));
			free(dir);
		}
}



/*
**	Alpha compare of link names.
*/

int
#ifdef	ANSI_C
lnkcmp(const void *lpp1, const void *lpp2)
#else	/* ANSI_C */
lnkcmp(lpp1, lpp2)
	char *	lpp1;
	char *	lpp2;
#endif	/* ANSI_C */
{
	return strcmp((*(Lnk **)lpp1)->lnk_name, (*(Lnk **)lpp2)->lnk_name);
}



/*
**	Compare two command names for message priority (reverse order).
*/

int
#ifdef	ANSI_C
msgcmp(const void *msgp1, const void *msgp2)
#else	/* ANSI_C */
msgcmp(msgp1, msgp2)
	char *	msgp1;
	char *	msgp2;
#endif	/* ANSI_C */
{
	return strcmp(*(char **)msgp2, *(char **)msgp1);
}



/*
**	Search a directory for message command files and print details.
*/

int
msg_stats(lp)
	Lnk *			lp;
{
	register MsgQ *		msgp;
	register MsgQ *		msgs;
	register int		msgcount;
	register struct direct *direp;
	register DIR *		dirp;
	bool			force;
	int			d_pid;

	Trace2(1, "msg_stats \"%s\"", lp->lnk_name);

	if ( All || NLinks == 1 )
		force = true;
	else
		force = false;

	d_pid = 0;
	msgcount = 0;
	msgs = (MsgQ *)0;

	/*
	**	Search link commands directory for command files.
	*/

	if ( Stop || CheckBad )
	{
		FreeStr(&LockName);
		LockName = concat(lp->lnk_cmds, REROUTELOCKFILE, NULLSTR);

		if ( !SetLock(LockName, Pid, false) )
		{
			Warn(CouldNot, LockStr, LockName);
			return 0;
		}

		LockSet = true;
	}

	while ( (dirp = opendir(lp->lnk_cmds)) == NULL )
		if ( !Warnings || Stop || !SysWarn(CouldNot, ReadStr, lp->lnk_cmds) )
		{
			if ( LockSet )
			{
				(void)unlink(LockName);
				LockSet = false;
			}
			return 0;
		}

	while ( (direp = readdir(dirp)) != NULL && !Finish )
	{
		/*
		**	Find valid command file.
		*/

		if
		(
			direp->d_name[0] < STATE_QUALITY
			||
			direp->d_name[0] > LOWEST_QUALITY
			||
			strlen(direp->d_name) != UNIQUE_NAME_LENGTH
		)
		{
			if
			(
				!Silent
				&&
				strcmp(direp->d_name, LOCKFILE) == STREQUAL
				&&
				DaemonActive(lp->lnk_cmds, false)
			)
			{
				d_pid = LockPid;
				if ( AllQueues )
					force = true;
			}

			continue;
		}

		Trace2(2, "found \"%s\"", direp->d_name);

		msgcount++;

		if ( !Fast )
		{
			msgp = Talloc(MsgQ);
			msgp->mq_next = msgs;
			msgs = msgp;
			(void)strcpy(msgp->mq_name, direp->d_name);
		}
	}

	closedir(dirp);

	if ( !Finish && !Silent && (msgcount || force) )
	{
		char *	d_state;
		char *	l_state;
		char	pidstr[1+7+1+5+1+1];
		char	linkstr[12];

		pidstr[0] = '\0';
		linkstr[0] = '\0';

		if ( d_pid )
		{
			bool	check_pid;

			if ( strcmp(NodeName(), LockNode) != STREQUAL )
				check_pid = false;
			else
				check_pid = true;

			if ( lp->lnk_dir[0] != WORKFLAG )
			{
				(void)GetLnkState(lp->lnk_dir, &d_state, &l_state, &d_pid, check_pid);
				if ( d_pid == 0 )
				{
					d_pid = LockPid;
					l_state = english("calling");
				}
				(void)sprintf(linkstr, english(" link %.4s"), l_state);
			}
			else
				d_state = msgcount ? english("active") : english("idle");

			if ( SU )
			{
				if ( !check_pid )
					(void)sprintf(pidstr, "(%.7s %d)", LockNode, d_pid&0xffff);
				else
					(void)sprintf(pidstr, "(%d)", d_pid&0xffff);
			}
		}
		else
			d_state = english("inactive");

		Fprintf
		(
			stdout,
			FormatL,
			lp->lnk_name,
			d_state,
			pidstr,
			linkstr,
			msgcount,
			msgcount==1?' ':'s'
		);
	}

	if ( msgcount && !Fast && !Finish )
	{
		char **	msgs_vec;

		msgs_vec = sort_msgs(msgcount, msgs);

		msgcount = proc_msgs(lp->lnk_cmds, msgcount, msgs_vec);

		free((char *)msgs_vec);

		while ( (msgp = msgs) != (MsgQ *)0 )
		{
			msgs = msgp->mq_next;
			free((char *)msgp);
		}
	}

	if ( LockSet )
	{
		(void)unlink(LockName);
		LockSet = false;
	}

	return msgcount;
}



/*
**	Print FTP files list.
*/

void
print_ftp_files()
{
	register FthFD_p	fp;
	FthReason		hr;
	char			date[DATESTRLEN];

	if ( (hr = GetFthFiles()) != fth_ok )
	{
		if ( Warnings )
			Warn(english("FTP Header \"%s\" error"), FTHREASON(hr));
		return;
	}

	for ( fp = FthFiles ; fp != (FthFD_p)0 ; fp = fp->f_next )
		Fprintf
		(
			stdout,
			"      %10lu  %s  %s\n",
			(PUlong)fp->f_length,
			DateString(fp->f_time, date),
			fp->f_name
		);

	FreeFthFiles();
}



/*
**	Print queue header.
*/

void
print_header()
{
	if ( !Verbose )
		Fprintf(stdout, "%s", FormatH);	/* Header line for message queue */
}



/*
**	Print out each node in route, and accumulate travel-time.
*/

bool
print_route(from, time, to)
	char *		from;
	Ulong		time;
	char *		to;
{
	char		timeb[TIMESTRLEN];

	if ( Tt == (Ulong)0 )
	{
		from = StripTypes(from);

		Fprintf
		(
			stdout,
			english("%*sFrom: %s\n%*svia   "),
			MSGINDENT, EmptyString,
			from,
			MSGINDENT, EmptyString
		);

		free(from);
	}
	else
		Fprintf(stdout, ",\n%*s      ", MSGINDENT, EmptyString);

	if ( time == 0 )
		time = 1;
	Tt += time;

	to = StripTypes(to);
	Fprintf(stdout, "%s", to);
	free(to);

	Fprintf(stdout, english(" (delay %s)"), TimeString(time, timeb));

	if ( (time = MesgLength / time) >= MIN_SPEED )
		Fprintf
		(
			stdout,
			english(" (%lu bytes/sec.)"),
			(PUlong)time
		);

	return true;
}



/*
**	Show and process each message.
*/

int
proc_msgs(dir, msgcount, msgs_vec)
	char *			dir;
	register int		msgcount;
	register char **	msgs_vec;
{
	register char *		fp;
	register char *		cp;
	int			maxcount;
	int			selcount;
	Ulong			total;
	bool			timed_out;
	bool			do_header = true;
	State			val;
	char			dbuf[DATETIMESTRLEN];

	fp = concat(dir, Template, NULLSTR);
	cp = &fp[strlen(dir)];

	maxcount = msgcount;
	selcount = 0;
	total = 0;

	while ( --msgcount >= 0 && !Finish )
	{
		(void)strcpy(cp, msgs_vec[msgcount]);

		CmdFlags = 0;
		ElapsedTime = 0;
		HeaderEnd = 0;
		MesgLength = 0;
		SubEnd = 0;
		Time_to_die = 0;

		FreeStr(&BounceMesg);
		FreeStr(&CmdUseAddr);
		FreeStr(&CmdUseLink);
		FreeStr(&CmdUseSrce);
		FreeStr(&HeaderFile);
		FreeStr(&LinkName);
		FreeStr(&MesgAddress);
		FreeStr(&OrigBounce);
		FreeStr(&SubFile);

		FreeCmds(&Commands);	/* Cleaned-up commands for reroute */
		FreeCmds(&Unlink);	/* File names for remove */

		if ( CheckBad || (MessageDetails && Verbose) )
			CmdFn = fp;

		if
		(
			!(timed_out = ReadCmds(fp, get_cmd))
			||
			HeaderFile == NULLSTR
		)
		{
			if ( (!CheckBad || !timed_out) && Warnings )
				Warn
				(
					english("bad commands file \"%s%s\" %s"),
					CheckBad?EmptyString:SPOOLDIR,
					fp,
					DateTimeStr(RdFileTime, dbuf)
				);

			if ( !Finish && CheckBad )
			{
				bad_session(fp, true);
				selcount++;
			}
			else
				maxcount--;

			continue;
		}

		if
		(
			MesgAddress != NULLSTR
			&&
			DestAddress != NULLSTR
			&&
			!address_match(DestAddress, DestSAddress, DestTAddress, MesgAddress)
		)
			continue;

		CmdFileTime = RdFileTime;

		timed_out = (bool)(Time_to_die && Time > (CmdFileTime + Time_to_die));
		total += MesgLength;

		if
		(
			(
				All
				||
				!timed_out
			)
			&&
			(val = read_header(maxcount-msgcount, do_header)) == accept
		)
		{
			char	tbuf[TIMESTRLEN];

			if ( MessageDetails )
			{
				if ( CmdFlags )
					Fprintf
					(
						stdout,
						english("%*sFlags: %s\n"),
						MSGINDENT, EmptyString,
						CmdToString(flags_cmd, CmdFlags)
					);

				if ( CmdUseLink != NULLSTR )
					Fprintf
					(
						stdout,
						english("%*sUse link: %s\n"),
						MSGINDENT, EmptyString,
						CmdUseLink
					);

				if ( CmdUseAddr != NULLSTR )
					Fprintf
					(
						stdout,
						english("%*sUse destination address: %s\n"),
						MSGINDENT, EmptyString,
						CmdUseAddr
					);

				if ( CmdUseSrce != NULLSTR )
					Fprintf
					(
						stdout,
						english("%*sUse source address: %s\n"),
						MSGINDENT, EmptyString,
						CmdUseSrce
					);

				if ( BounceMesg != NULLSTR )
				{
					char *	concise = StripErrString(BounceMesg);

					Fprintf
					(
						stdout,
						english("%*sReason: "),
						MSGINDENT, EmptyString
					);

					ExpandFile(stdout, BounceMesg, -(MSGINDENT+2));
					putc('\n', stdout);

					if ( strcmp(concise, BounceMesg) != STREQUAL )
					{
						Fprintf
						(
							stdout,
							english("%*sConcise reason:\n%*s"),
							MSGINDENT, EmptyString,
							MSGINDENT+2, EmptyString
						);

						ExpandFile(stdout, concise, -(MSGINDENT+2));
						putc('\n', stdout);
					}
					free(concise);
				}

				if ( timed_out )
					Fprintf
					(
						stdout,
						english("%*sTIMED OUT\n"),
						MSGINDENT, EmptyString
					);
				else
				if ( Time_to_die > 0 )
					Fprintf
					(
						stdout,
						english("%*sTime-to-die: %s\n"),
						MSGINDENT, EmptyString,
						TimeString((CmdFileTime+Time_to_die)-Time, tbuf)
					);
			}

			do_header = false;

			if ( Finish )
				;
			else
			if ( Stop )
				stop_session(fp);
			else
			if ( CheckBad )
				bad_session(fp, false);

			selcount++;
			continue;
		}

		if ( val == error && CheckBad )
		{
			bad_session(fp, true);
			selcount++;
		}
		else
		if ( timed_out )
			maxcount--;
	}
		
	free(fp);

	if ( Silent )
		return selcount;

	if ( maxcount > 1 && total > 0 && !do_header )
		Fprintf(stdout, FormatT, maxcount, (PUlong)total);

	if ( maxcount > 0 && !Verbose && !do_header )
		putc('\n', stdout);

	return selcount;
}



/*
**	Read a line from user.
*/

int
read_command(word, size, command)
	char *		word;
	int		size;
	bool		command;
{
	register char *	cp;
	register int	n;
	register int	i;
	bool		chop;
	char		buf[512];
	static char	space[]	= " \t";

	FreeStr(&NextWord);

	if ( !Yes )
	{
		Fflush(stdout);

		n = sizeof(buf)-1;
		cp = buf;
		chop = false;

		for ( ;; )
		{
			if ( (i = read(fileno(stdin), cp, Isatty?n:1)) <= 0 )
				finish(EX_OK);

			if ( cp[i-1] == '\n' )
			{
				cp[--i] = '\0';
				break;
			}

			if ( cp < &buf[sizeof(buf)-1] )
			{
				if ( (n -= i) == 0 )
					n = 1;
				cp += i;
			}
			else
				chop = true;
		}

		if ( chop )
		{
			word[0] = '\0';
			n = 0;
		}
		else
		{
			cp = buf;
			cp += strspn(cp, space);

			if ( (NextWord = strpbrk(cp, space)) != NULLSTR )
			{
				*NextWord++ = '\0';
				NextWord += strspn(NextWord, space);
				NextWord = newstr(NextWord);
			}

			if ( (n = strlen(cp)) == 0 && command )
			{
				(void)strcpy(word, english("no"));
				n = strlen(word);
			}
			else
			if ( n >= size )
			{
				n = size-1;
				(void)strncpy(word, cp, n);
				word[n] = '\0';
			}
			else
				(void)strcpy(word, cp);
		}
	}
	else
	if ( command )
	{
		(void)strcpy(word, Delete?english("delete"):english("yes"));
		n = strlen(word);
	}
	else
	{
		word[0] = '\0';
		n = 0;
	}

	if ( !Silent && (Yes || !Isatty) )
	{
		Fprintf(stdout, "%s\n", word);
		Fflush(stdout);
	}

	return n;
}



/*
**	Read the data part of message for FTP header.
*/

State
read_ftp()
{
	int		fd;
	char *		fn;
	FthReason	hr;
	Ulong		end;

	if ( DataLength == 0 )
	{
		fn = SubFile;
		end = SubEnd;
	}
	else
	{
		fn = HeaderFile;
		end = DataLength;
	}

	Trace6(
		1,
		"read_ftp \"%s\" end %lu, headerend %lu, hdrlength %lu, datalength %lu",
		fn==NULLSTR ? NullStr : fn, (PUlong)end,
		(PUlong)HeaderEnd,
		(PUlong)HdrLength,
		(PUlong)DataLength
	);

	if ( fn == NULLSTR || (fd = open(fn, O_READ)) == SYSERROR )
	{
		if ( Warnings )
			(void)SysWarn(CouldNot, ReadStr, fn==NULLSTR ? NullStr : fn);

		return error;
	}

	if ( (hr = ReadFtHeader(fd, (long)end)) != fth_ok && hr != fth_type )
	{
		if ( Warnings )
			Warn(english("FTP Header \"%s\" error"), FTHREASON(hr));

		(void)close(fd);
		return error;
	}

	(void)close(fd);

	if ( strcmp(FthFrom, Me.P_name) == STREQUAL )
		OwnMesg = true;

	if ( !OwnMesg && (!All || !SU || UserName != NULLSTR) )
		return reject;

	UnQuoteChars(FthTo, FthToRestricted);

	/*
	**	Have valid FTP message to print
	*/

	if ( HdrFlags & (HDR_ACK|HDR_RETURNED) )
	{
		To = FthFrom;
		From = Verbose?((HdrFlags&HDR_ACK)?"ACK":"RETURNED"):FthTo;
	}
	else
	{
		To = FthTo;
		From = FthFrom;
	}

	return accept;
}



/*
**	Read HeaderFile, and print details.
*/

State
read_header(num, do_header)
	int		num;
	bool		do_header;
{
	int		fd;
	int		len;
	HdrReason	hr;
	char *		cp;
	Handler *	handler;
	char *		cp1;
	char *		cp2;
	char *		dest;
	char *		source;
	bool		athome = false;
	State		ftpval;

	Trace2(1, "read_header \"%s\"", HeaderFile);

	if ( (fd = open(HeaderFile, O_READ)) == SYSERROR )
	{
		if ( Warnings )
			(void)SysWarn(CouldNot, ReadStr, HeaderFile);

		return error;
	}
	
	if ( (hr = ReadHeader(fd)) != hr_ok )
	{
		if ( Warnings )
			Warn(english("Message header \"%s\" error."), HeaderReason(hr));

		(void)close(fd);
		return error;
	}

	(void)close(fd);

	if
	(
		RHandler != NULLSTR
		&&
		strcmp(HdrHandler, RHandler) != STREQUAL
	)
		return reject;

#	if	SUN3 == 1
	if ( HdrType == HDR_TYPE1 )
	{
		if ( get3envbool(ENV3_RETURNED) )
			HdrFlags |= HDR_RETURNED;
		if ( get3envbool(ENV3_ACK) )
			HdrFlags |= HDR_ACK;
		if ( get3envbool(ENV3_NORET) )
			HdrFlags |= HDR_NORET;
	}
#	endif	/* SUN3 == 1 */

	/*
	**	Fetch protcocol details if from HomeAddress, to/from specific address, or All.
	*/

	if ( AddressMatch(HomeName, HdrSource) )
		athome = true;
	else
	if ( !All || !SU )
		return reject;

	if ( SourceAddress != NULLSTR && !address_match(SourceAddress, SourceSAddress, SourceTAddress, HdrSource) )
		return reject;

	if ( DestAddress != NULLSTR && !address_match(DestAddress, DestSAddress, DestTAddress, HdrDest) )
		return reject;

	To = HdrHandler;
	From = HdrHandler;
	OwnMesg = false;

	if ( HdrSubpt == FTP )
	{
		if ( !Silent || UserName != NULLSTR )
		{
			if ( (ftpval = read_ftp()) != accept && (ftpval == reject || !IgnErr) )
				return ftpval;

			if ( !athome )
				OwnMesg = false;	/* Override read_ftp() */
		}
		else
			ftpval = reject;
	}
	else
	if ( !All || !SU )
		return reject;

	if ( Silent )
		return accept;

	if ( HdrFlags & HDR_ACK )
		cp = "ACK";
	else
	if ( (handler = GetHandler(HdrHandler)) == (Handler *)0 || (cp = handler->descrip) == NULLSTR )
		cp = "Message";

	if ( do_header )
		print_header();

	if ( !MessageDetails )
	{
		cp1 = StripTypes(HdrSource);
		cp2 = StripTypes(HdrDest);
		source = cp1;
		if ( (len = strlen(cp2)) > FORMATDESTLEN && !Verbose )
			len = FORMATDESTLEN;
		dest = ExpandString(cp2, len);
	}
	else
	{
		cp1 = NULLSTR;
		cp2 = NULLSTR;
		source = HdrSource;
		if ( (len = strlen(HdrDest)) > FORMATDESTLEN && !Verbose )
			len = FORMATDESTLEN;
		dest = ExpandString(HdrDest, len);
	}

	if ( !Verbose )
	{
		if ( !athome || HdrSubpt != FTP || (!OwnMesg && !SU) )
			From = source;

		Fprintf(stdout, Format, num, cp, From, To, dest, (PUlong)MesgLength);
	}
	else
		Fprintf(stdout, FormatV, num, cp, From, source, To, dest, (PUlong)MesgLength);

	FreeStr(&cp2);
	FreeStr(&cp1);

	if ( Verbose && HdrSubpt == FTP && ftpval == accept )
		print_ftp_files();

	/*
	**	Print message header info if requested.
	*/

	if ( ShowHeader )
		PrintHeader(stdout, MSGINDENT, false);

	if ( MessageDetails )
	{
		char	timeb[TIMESTRLEN];
		char	dateb[DATESTRLEN];

		Fprintf
		(
			stdout,
			english("%*sSent: %s\n"),
			MSGINDENT, EmptyString,
			DateString(CmdFileTime-HdrTt-ElapsedTime, dateb)
		);

		Tt = 0;

		if ( ExRoute(HdrSource, HdrRoute, print_route) )
			Fprintf(stdout, ".\n");

		Fprintf
		(
			stdout,
			english("%-*sTravel + queue time: %s.\n"),
			MSGINDENT, EmptyString,
			TimeString(Time-CmdFileTime+Tt+ElapsedTime, timeb)
		);
	}

	return accept;
}



/*
**	Read a response from user.
*/

Cmdtyp
response()
{
	register Cmd_p	cmdp;
	register int	len;
	register int	l;
	Command		cmd;
	char		com[MAXCOMLEN+60];

	cmd.c_name = com;

	for ( ;; )
	{
		if
		(
			(cmd.c_minl = read_command(com, sizeof com, true)) >= MINCOMLEN
			&&
			cmd.c_minl <= MAXCOMLEN
			&&
			(cmdp = (Cmd_p)bsearch((char *)&cmd, (char *)Cmds, NCMDS, CMDZ, compare)) != (Cmd_p)0
			&&
			(!Stop || cmdp->c_stop)
		)
		{
			if ( cmdp->c_type == r_quit )
				finish(EX_OK);

			return cmdp->c_type;
		}

		if ( Yes || !Isatty )
			Error(english("illegal response \"%s\", use manual interaction"), com);

		Fprintf(stdout, english("(Respond: "));

		for ( len = 0, cmdp = Cmds ; cmdp < &Cmds[NCMDS] ; cmdp++ )
			if ( cmdp->c_show && (!Stop || cmdp->c_stop) )
			{
				if ( len )
					Fprintf(stdout, ", ");

				if ( (len += (l = 4+strlen(cmdp->c_name))) > 60 )
				{
					Fprintf(stdout, "\n\t  ");
					len = l;
				}

				Fprintf(stdout, "\"%s\"", cmdp->c_name);
			}

		Fprintf(stdout, ".) ? ");
	}
}



/*
**	Return message via routing daemon.
*/

void
return_message(cmdsfile, flag)
	char *		cmdsfile;
	int		flag;
{
	register CmdV	cmdv;
	register int	fd;
	char *		cp1;
	char *		cp2;

	Trace3(1, "return_message(%s, 0x%x)", cmdsfile, flag);

	if ( !CheckBad && (HdrFlags & HDR_NORET) )
	{
		if ( Warnings )
			Warn("Message not returned:- NO RETURN requested");

		unlink_message(cmdsfile);
		return;
	}

	/*
	**	Add reason for later analysis.
	*/

	if
	(
		(cmdv.cv_name = NewStopMesg) == NULLSTR
		&&
		(cmdv.cv_name = StopMesg) == NULLSTR
		&&
		(cmdv.cv_name = OrigBounce) == NULLSTR
	)
	{
		cp1 = Stop ? english("returned") : english("rerouted");
		cmdv.cv_name = concat(english("Message "), cp1, english(" by operator.\n"), BounceMesg, NULLSTR);
		(void)AddCmd(&Commands, bounce_cmd, cmdv);
		free(cmdv.cv_name);
	}
	else
		(void)AddCmd(&Commands, bounce_cmd, cmdv);

	if ( (cp1 = NewHdrDest) != NULLSTR )
		(void)AddCmd(&Commands, useaddr_cmd, (cmdv.cv_name = cp1, cmdv));
	else
		cp1 = HdrDest;

	if ( (cmdv.cv_name = NewHdrSource) != NULLSTR )
		(void)AddCmd(&Commands, usesrce_cmd, cmdv);

	if ( (cmdv.cv_name = LinkName) != NULLSTR )
		(void)AddCmd(&Commands, linkname_cmd, cmdv);

	if ( flag & RETRY )
	{
		Addr *	ap;

		ap = StringToAddr(cp2 = newstr(cp1), NO_STA_MAP);
		if ( !AddrIsBC(ap) && AddrAtHome(&ap, false, false) )
			CmdFlags &= ~RE_ROUTE;
		FreeAddr(&ap);
		free(cp2);
	}
	else
		flag |= RETRY;

	CmdFlags &= ~BOUNCE_MESG;
	CmdFlags |= flag;

	if ( UseLink != NULLSTR )
	{
		(void)AddCmd(&Commands, uselink_cmd, (cmdv.cv_name = UseLink, cmdv));
		(void)AddCmd(&Commands, flags_cmd, (cmdv.cv_number = CmdFlags, cmdv));
	}
	else
	if ( flag == BOUNCE_MESG )
		(void)AddCmd(&Commands, flags_cmd, (cmdv.cv_number = CmdFlags, cmdv));
	else
		(void)AddCmd(&Commands, flags_cmd, (cmdv.cv_number = CmdFlags&~RE_ROUTE, cmdv));

	if ( Time_to_die )
	{
		if ( Time < (CmdFileTime + Time_to_die) )
			cmdv.cv_number = (CmdFileTime+Time_to_die)-Time;
		else
			cmdv.cv_number = DAY;	/* New lease of life? */

		(void)AddCmd(&Commands, timeout_cmd, cmdv);
	}

	(void)AddCmd(&Commands, traveltime_cmd, (cmdv.cv_number = Time-CmdFileTime+ElapsedTime, cmdv));

	/*
	**	Re-write commands.
	*/

	(void)unlink(cmdsfile);
	fd = create(cmdsfile);	/* Reuse old file */

	(void)WriteCmds(&Commands, fd, cmdsfile);

	(void)close(fd);

	/*
	**	Place message back in circulation.
	*/

#	if	PRIORITY_ROUTING == 1
	move(cmdsfile, UniqueName(ReRoutFile, HdrQuality, OPTNAME, MesgLength, CmdFileTime));
#	else	/* PRIORITY_ROUTING == 1 */
	move(cmdsfile, UniqueName(ReRoutFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, CmdFileTime));
#	endif	/* PRIORITY_ROUTING == 1 */

	(void)DaemonActive(ROUTINGDIR, true);	/* Kick daemon */

	Returned++;
}



/*
**	Set default flags for checking messages in ``_bad'' directory.
*/

bool
set_checkbad()
{
	SetUser(NetUid, NetGid);

	if ( geteuid() != NetUid )
		return false;

	All = true;
	AllQueues = false;
	CheckBad = true;
	Fast = false;
	IgnErr = true;
	Routing = false;
	Traces = false;
	Verbose = true;

	if ( !Yes )
		Silent = false;

	if ( !Silent )
	{
		MessageDetails = true;
		Warnings = true;
	}

	if ( !Pending && !Reroute )
		Bad = true;

	return true;
}



/*
**	Set default flags for ``queue''.
*/

bool
set_queue()
{
	if ( geteuid() != NetUid )
		return false;

	return true;
}



/*
**	Set default flags for ``stop''.
*/

bool
set_stop()
{
	if ( geteuid() != NetUid )
		return false;

	if ( !Yes )
		Silent = false;

	if ( !Silent )
		Warnings = true;

	Fast = false;
	Stop = true;
	Verbose = true;

	return true;
}



/*
**	Catch signals to clean up.
*/

void
sigcatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	Finish = true;
	Continuous = false;
}



/*
**	Sort the links into alphabetic order.
*/

void
sort_links()
{
	register Lnk *	lp;
	register Lnk **	lpp;

	Link_Vec = lpp = (Lnk **)Malloc(sizeof(Lnk *) * NLinks);

	for ( lp = LinkList ; lp != (Lnk *)0 ; lp = lp->lnk_next )
		*lpp++ = lp;

	DODEBUG(if((lpp-Link_Vec)!=NLinks)Fatal1("bad NLinks"));

	Trace2(1, "sort %d links", NLinks);

	if ( NLinks > 1 )
		qsort((char *)Link_Vec, NLinks, sizeof(Lnk *), lnkcmp);
}



/*
**	Sort messages found for this link according to queue order (lexicographically.)
*/

char **
sort_msgs(msgcount, msgs)
	int		msgcount;
	MsgQ *		msgs;
{
	register MsgQ *	msgp;
	register char **msgvp;
	char **		msgs_vec;

	msgs_vec = msgvp = (char **)Malloc(msgcount * sizeof(char *));

	if ( (msgp = msgs) != (MsgQ *)0 )
	do
		*msgvp++ = msgp->mq_name;
	while
		( (msgp = msgp->mq_next) != (MsgQ *)0 );

	if ( msgcount > 1 )
		qsort((char *)msgs_vec, msgcount, sizeof(char *), msgcmp);

	return msgs_vec;
}



/*
**	Interact for stopped message manipulation.
*/

void
stop_session(cmdsfile)
	char *	cmdsfile;
{
	bool	remove;

	Trace2(1, "stop_session(%s)", cmdsfile);

	if ( NoRet || strncmp(cmdsfile, ROUTINGDIR, RoutDirLen) == STREQUAL )
		remove = true;
	else
		remove = false;

	FreeStr(&NewStopMesg);

	for ( ;; )
	{
		if ( !Silent )
			Fprintf(stdout, "%s", remove?RemoveStr:StopeStr);

		switch ( response() )
		{
		default:
			Fatal1(english("unrecognised response"));
			return;

		case r_return:
		case r_stop:
			if ( remove )
				break;
		case r_yes:
		case r_remove:
			if ( NextWord != NULLSTR )
			{
				FreeStr(&NewStopMesg);
				NewStopMesg = NextWord;
				NextWord = NULLSTR;
			}
rem:
			if ( OwnMesg || remove )
				unlink_message(cmdsfile);
			else
				return_message(cmdsfile, BOUNCE_MESG);

			return;

		case r_reason:
			if ( remove )
				break;
			FreeStr(&NewStopMesg);
			NewStopMesg = getname(english("reason"));
			goto rem;

		case r_no:
			return;
		}

		Fprintf(stdout, english("remove only\n"));
	}
}



/*
**	Throw message away.
*/

void
unlink_message(cmdsfile)
	char *		cmdsfile;
{
	register Cmd *	cmdp;

	Trace2(1, "unlink_message(%s)", cmdsfile);

	(void)unlink(cmdsfile);
	Trace2(2, "unlink(%s)", cmdsfile);

	for ( cmdp = Unlink.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
		{
			(void)unlink(cmdp->cd_value);
			Trace2(2, "unlink(%s)", cmdp->cd_value);
		}

	Stopped++;
}
