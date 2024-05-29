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


static char	sccsid[]	= "@(#)request.c	1.22 05/12/16";

/*
**	Request remote state files.
**
**	Send request message to statehandlers.
**
**	This program is also invoked by the statefile/routefile maintainer
**	to send the forward list to local links, and broadcast state changes.
*/

#define	FILE_CONTROL
#define	SIGNALS
#define	STAT_CALL
#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"handlers.h"
#include	"header.h"
#include	"link.h"
#include	"Passwd.h"
#include	"route.h"
#include	"routefile.h"
#include	"spool.h"
#include	"statefile.h"
#include	"sub_proto.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Arguments.
*/

bool	All;			/* Fetch all state info. from address */
bool	AllLinks;		/* Include all links in broadcast */
bool	Broadcast;		/* Broadcast request */
bool	ExpIn;			/* Export file on ``stdin'' */
char *	ExportFile;		/* Name of alternative export file */
bool	ExportState;		/* Export state data */
char *	Name	= sccsid;	/* Program invoked name */
bool	NoEnq;			/* Don't ask for remote reply */
bool	NoTimeout;		/* Don't put a message timeout on the request */
char *	RestrictName;		/* Restrict broadcast to this region */
char *	SourceAddress;		/* Source address for broadcast (must be a link) */
bool	RetOK;			/* Allow bad request to be returned */
int	Traceflag;		/* Global tracing control */

AFuncv	getAddress _FA_((PassVal));

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(a, &All, 0),
	Arg_bool(b, &Broadcast, 0),
	Arg_bool(e, &ExportState, 0),
	Arg_bool(i, &ExpIn, 0),
	Arg_bool(l, &AllLinks, 0),
	Arg_bool(n, &NoEnq, 0),
	Arg_bool(r, &RetOK, 0),
	Arg_bool(t, &NoTimeout, 0),
	Arg_string(R, &RestrictName, 0, english("region"), OPTARG),
	Arg_string(S, &SourceAddress, 0, english("source"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_string(X, &ExportFile, 0, english("export file"), OPTARG),
	Arg_long(Z, &HdrTtd, 0, english("seconds-to-die"), OPTARG),
	Arg_noflag(0, getAddress, english("address"), OPTARG|MANY),
	Arg_end
};

/*
**	Miscellaneous.
*/

Addr *	AddrList;		/* List of addresses */
CmdHead	Commands;		/* Message commands */
char *	CmdsTmp;		/* File for building commands */
char *	Destination;		/* Final address list for message */
char *	HdrFile;		/* File containing header */
Addr *	Home_Addr;		/* Home */
Passwd	Invoker;		/* Person invoked by */
Link	LinkD;			/* Details of link */
Addr *	LinksList;		/* List of links for link state message */
char *	NotLinks;		/* Second state message should not be delivered at these */
char	LinkStr[]	= english("link to");
char *	RoutingDir;		/* Router's home */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	TmpExp;			/* Name of export file link */
char *	TmpFwd;			/* Name of forwarding file link */


bool	build_addresses _FA_((void));
char *	copyin _FA_((void));
bool	find_links _FA_((Addr **));
void	queue_commands _FA_((void));
bool	reqsend _FA_((void));
void	write_header _FA_((void));



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	Addr *	ap;

	InitParams();

	if ( !ReadRoute(SUMCHECK) )
	{
		Warn("No routing tables.");
		exit(EX_OSFILE);
	}

	DoArgs(argc, argv, Usage);

	GetInvoker(&Invoker);
	SetUser(NetUid, NetGid);

	if ( geteuid() != NetUid || !(Invoker.P_flags & P_SU) )
	{
		Warn(english("No permission."));
		exit(EX_NOPERM);
	}

	if ( ExpIn && ExportFile != NULLSTR )
	{
		Warn(english("Can't specify -i with -X"));
		exit(EX_USAGE);
	}

	if ( ExpIn || ExportFile != NULLSTR || NoEnq )
		ExportState = true;

	(void)signal(SIGHUP, finish);
	(void)signal(SIGINT, finish);
	(void)signal(SIGTERM, finish);

	(void)sprintf(Template, "%*s", (int)(sizeof Template - 1), EmptyString);
	RoutingDir = concat(SPOOLDIR, ROUTINGDIR, NULLSTR);

	if ( Broadcast || AddrIsBC(AddrList) )
	{
		All = false;

		if ( HdrTtd == 0 || HdrTtd > (Ulong)WEEK )
			HdrTtd = (Ulong)WEEK;
	}
	else
	if ( !NoTimeout && (HdrTtd == 0 || HdrTtd > (Ulong)WEEK) )
		HdrTtd = (Ulong)WEEK;

	if ( RestrictName != NULLSTR )
	{
		if ( strcmp(RestrictName, WORLD_NAME) == STREQUAL )
			VisibleName = EmptyString;
		else
			VisibleName = RestrictName;
	}

	if ( SourceAddress != NULLSTR )
	{
		ap = StringToAddr(newstr(SourceAddress), NO_STA_MAP);

		if ( !FindLink(TypedName(ap), (Link *)0) )
		{
			Warn(english("Unknown link \"%s\""), SourceAddress);
			exit(EX_NOHOST);
		}

		(void)UpdateHeader((Ulong)0, TypedName(ap));	/* Pre-set HdrRoute */
	}
	else
		ap = StringToAddr(newstr(HomeName), NO_STA_MAP);

	HdrSource = TypedAddress(ap);
	HdrHandler = STATEHANDLER;
	HdrQuality = STATE_QUALITY;
	HdrSubpt = STATE_PROTO;

	if ( (!build_addresses() || !reqsend()) && !AllLinks )
		Warn(english("Nothing to do."));
	else
	if ( !DaemonActive(RoutingDir, true) )
		Warn(english("routing daemon not running."));

	exit(EX_OK);
}



/*
**	Make list of broadcast addresses for regions in visible region.
*/

void
broadcast_visible(home, vis)
	char *	home;
	char *	vis;
{
	register char *	cp;
	static char	bc[] = { ATYP_BROADCAST, DOMAIN_SEP, '\0' };

	Trace3(1, "broadcast_visible(%s, %s)", home, vis);

	if ( (cp = vis) == NULLSTR || *cp == '\0')
	{
		vis = &home[strlen(home)];
		AddAddr(&AddrList, StringToAddr(newstr(bc), NO_STA_MAP));
	}
	else
	{
		AddAddr
		(
			&AddrList,
			StringToAddr(concat(bc, cp, NULLSTR), NO_STA_MAP)
		);

		if ( (vis = StringMatch(home, cp)) == NULLSTR )
			return;
	}

	for
	(
		cp = strchr(home, DOMAIN_SEP) ;
		cp != NULLSTR && ++cp < vis ;
		cp = strchr(cp, DOMAIN_SEP)
	)
		AddAddr
		(
			&AddrList,
			StringToAddr(concat(bc, cp, NULLSTR), NO_STA_MAP)
		);
}



/*
**	Build addresses for request.
*/

bool
build_addresses()
{
	static Passwd	nullpw;

	Trace1(1, "build_addresses()");

	if ( Broadcast )
		broadcast_visible(HomeName, VisibleName);

	if ( (AddrList == (Addr *)0 || AllLinks) && !find_links(&AddrList) )
		return false;

	nullpw.P_flags = P_ALL;

	Destination = CheckAddr(&AddrList, &nullpw, Warn, true);

	Trace2(1, "Destination ==> %s", Destination);

	if ( AddrList == (Addr *)0 )
		Error(english("illegal address"));

	return true;
}



/*
**	Grab export file off ``stdin''.
*/

char *
copyin()
{
	register int	r;
	register int	w;
	register char *	cp;
	register int	ifd	= fileno(stdin);
	register int	ofd;
	long		posn	= 0;
	char		buf[FILEBUFFERSIZE];

	Trace1(1, "copyin()");

	TmpExp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	(void)UniqueName(TmpExp, CHOOSE_QUALITY, OPTNAME, DataLength, Time);
	ofd = create(TmpExp);

	for ( ;; )
	{
		while ( (r = read(ifd, buf, FILEBUFFERSIZE)) == SYSERROR )
			if ( !SysWarn(CouldNot, ReadStr, "stdin") )
				return NULLSTR;

		if ( r == 0 )
			break;

		cp = buf;

		while ( (w = write(ofd, cp, r)) != r )
		{
			if ( w == SYSERROR )
			{
				if ( !SysWarn(CouldNot, WriteStr, TmpExp) )
					return NULLSTR;
				(void)lseek(ofd, posn, 0);
			}
			else
			{
				cp += w;
				r -= w;
				posn += w;
			}
		}

		posn += r;
	}

	(void)close(ofd);

	Trace2(2, "copyin() ==> %s", TmpExp);

	return TmpExp;
}



/*
**	Find active ``real'' links.
*/

bool
find_links(listp)
	Addr **		listp;
{
	register int	i;

	Trace2(1, "find_links(%#lx)", (PUlong)listp);

	for ( i = 0 ; i < LinkCount ; i++ )
	{
		GetLink(&LinkD, (Index)i);

		if ( !(LinkD.ln_flags & (FL_FOREIGN|FL_DEAD)) )
			AddAddr(listp, StringToAddr(newstr(LinkD.ln_rname), NO_STA_MAP));
	}

	if ( *listp == (Addr *)0 )
	{
		Warn(english("Can't find any links."));
		return false;
	}

	return true;
}



/*
**	Called from the errors routines to cleanup.
*/

void
finish(error)
	int		error;
{
	if ( TmpFwd != NULLSTR )
		(void)unlink(TmpFwd);

	if ( TmpExp != NULLSTR )
		(void)unlink(TmpExp);

	if ( HdrFile != NULLSTR )
		(void)unlink(HdrFile);

	if ( CmdsTmp != NULLSTR )
		(void)unlink(CmdsTmp);

	(void)exit(error);
}



/*
**	Process an ``address'' argument.
*/

AFuncv
getAddress(arg)
	PassVal	arg;
{
	AddAddr(&AddrList, StringToAddr(arg.p, NO_STA_MAP));
	return ACCEPTARG;
}



/*
**	Write and queue commands for router.
*/

void
queue_commands()
{
	register int	fd;
	register CmdV	cmdv;
	register char *	cmdsfile;

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = HdrFile, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = HdrLength, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = HdrFile, cmdv));

	/*
	**	Write the commands into a temporary file.
	*/

	CmdsTmp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	fd = create(UniqueName(CmdsTmp, CHOOSE_QUALITY, OPTNAME, DataLength+HdrLength, Time));

	(void)WriteCmds(&Commands, fd, CmdsTmp);

	(void)close(fd);

	/*
	**	Queue commands for processing by router.
	*/

	cmdsfile = concat(RoutingDir, Template, NULLSTR);
	(void)UniqueName(cmdsfile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time);
	move(CmdsTmp, cmdsfile);

	Trace2(1, "queue_commands in \"%s\"", cmdsfile);
	free(cmdsfile);
}



/*
**	Send request.
*/

bool
reqsend()
{
	register CmdV	cmdv;
	register char *	expf;
	register int	count = 0;
	struct stat	statb;

	Trace1(1, "reqsend()");

	DataLength = 0;

	if ( All && !DESTTYPE(AddrList) )
		All = false;

	/*
	**	If an export state file exists, include it in the message.
	*/

	if ( ExpIn )
		expf = copyin();
	else
	if ( (expf = ExportFile) != NULLSTR )
		expf = newstr(expf);
	else
	{
		expf = AddrIsBC(AddrList) ? SITEFILE : All ? STATEFILE : EXPORTFILE;
		expf = concat(SPOOLDIR, STATEDIR, expf, NULLSTR);
	}

	if ( ExportState && access(expf, 4) != SYSERROR )
	{
		while ( stat(expf, &statb) == SYSERROR )
			Syserror(CouldNot, StatStr, expf);

		if ( (DataLength = statb.st_size) == 0 && NoEnq )
		{
			free(expf);
			return false;
		}

		if ( DataLength > 0 && !ExpIn )
		{
			TmpExp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
			(void)UniqueName(TmpExp, CHOOSE_QUALITY, OPTNAME, DataLength, Time);

			while ( link(expf, TmpExp) == SYSERROR )
			{
				if ( ++count > 3 )
					Syserror(CouldNot, LinkStr, expf);

				(void)sleep(LOCK_SLEEP);
			}
		}

		free(expf);
	}
	else
	{
		free(expf);
		if ( NoEnq )
			return false;
	}

	/*
	**	Make header.
	*/

	HdrDest = Destination;
	HdrFlags = HDR_NOOPT|HDR_NO_AUTOCALL;

	if ( NotLinks == NULLSTR )
	{
		if ( All && !NoEnq )
			HdrEnv = MakeEnv(ENV_FLAGS, STATE_ENQ_ALL, NULLSTR);
		else
			HdrEnv = NULLSTR;
	}
	else
		HdrEnv = MakeEnv(ENV_NOTREGIONS, NotLinks, NULLSTR);

	if ( !RetOK )
		HdrFlags |= HDR_NORET;

	if ( !NoEnq )
		HdrFlags |= HDR_ENQ;

	write_header();

	/*
	**	Make commands file describing message.
	*/

	InitCmds(&Commands);

	if ( DataLength > 0 )
	{
		(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = TmpExp, cmdv));
		(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
		(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = DataLength, cmdv));
		(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = TmpExp, cmdv));
	}

	queue_commands();

	return true;
}



/*
**	Write out header file.
*/

void
write_header()
{
	register int	fd;

	HdrFile = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	fd = create(UniqueName(HdrFile, CHOOSE_QUALITY, OPTNAME, DataLength, Time));
	HdrID = HdrFile + strlen(SPOOLDIR) + strlen(WORKDIR);
	HdrType = HDR_TYPE2;

	while ( WriteHeader(fd, (long)0, 0) == SYSERROR )
		Syserror(CouldNot, english("write header in"), HdrFile);

	(void)close(fd);

	Trace3(1, "write_header in \"%s\" ==> %lu", HdrFile, (PUlong)HdrLength);
}
