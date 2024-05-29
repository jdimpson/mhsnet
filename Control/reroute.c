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


static char	sccsid[]	= "@(#)reroute.c	1.20 05/12/16";

/*
**	Re-route messages held in PENDINGDIR.
*/

#define	RECOVER
#define	SIGNALS
#define	STDIO

#include	"global.h"
#include	"Args.h"
#include	"address.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"link.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sysexits.h"

#include	<ndir.h>


/*
**	Arguments.
*/

bool	All;			/* Re-route all from source dir. */
bool	Bad;			/* Re-route _bad directory */
bool	Break;			/* Break link between us and OldLink */
bool	Dead;			/* Mark link dead between us and OldLink */
char *	LinkLink;		/* Use link instead of _bad */
char *	Name	= sccsid;	/* Program invoked name */
char *	NewLink;		/* New link suggested for messages */
char *	OldLink;		/* Old link messages were queued on */
char	Quality	= LOWEST_QUALITY;	/* Only move messages with priority <= this */
bool	ReRouteOld;		/* Force all files queued for OldLink to be re-routed */
bool	Silent;			/* Don't burble */
int	Traceflag;		/* Global tracing control */
bool	Verbose;		/* Show each message re-routed */

AFuncv	getNewDest _FA_((PassVal, Pointer));	/* Optional new address for messages */
AFuncv	getQuality _FA_((PassVal, Pointer));	/* Message priority */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(a, &All, 0),
	Arg_bool(b, &Break, 0),
	Arg_bool(d, &Dead, 0),
	Arg_bool(e, &Bad, 0),
	Arg_bool(f, &ReRouteOld, 0),
	Arg_bool(s, &Silent, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_string(L, &LinkLink, 0, english("link"), OPTARG),
	Arg_char(P, &Quality, getQuality, english("priority"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_noflag(&OldLink, 0, english("address>|<old-link"), OPTARG),
	Arg_noflag(&NewLink, 0, english("new-link>|<home"), OPTARG),
	Arg_noflag(0, getNewDest, english("forward-via"), OPTARG),
	Arg_end
};

/*
**	Miscellaneous.
*/

char *	CmdAddress;		/* Address for message in commands */
Ulong	CmdFlags;		/* Flags from commandsfile */
CmdHead	Commands;		/* Commands from current commandsfile */
bool	Finish;			/* Wind up */
char *	HdrFile;		/* File containing header for current message */
Passwd	Invoker;		/* Person invoked by */
Link	LinkData;		/* Details of a link */
char *	LinkName;		/* Link used by current commandsfile */
Addr *	LLink_Addr;		/* Structure for LinkLink */
char *	LockName;		/* Name of current lockfile */
bool	LockSet;		/* Directory locked */
Ulong	MesgLength;		/* Length of message being moved */
char *	NewDest;		/* Optional new address for messages */
Addr *	NLink_Addr;		/* Structure for NewLink */
char *	NLinkAddress;		/* Full address for NewLink */
char *	OldAddress;		/* Full address for OldLink interpreted as an address */
Addr *	OLink_Addr;		/* Structure for OldLink */
char *	OLinkAddress;		/* Full address for OldLink interpreted as a link */
char *	OLinkDirectory;		/* Directory name for old link */
bool	ReDeliver;		/* True if `newlink' is home */
int	Rerouted;		/* Count of messages re-routed */
char *	RoutingDir;		/* Queueing directory for router */
char *	RoutngFile;		/* Queueing file template */
char *	SCmdAddress;		/* Address for message in commands with types stripped */
char	Template[UNIQUE_NAME_LENGTH+1];

char *	bangaddress _FA_((char *, char *));
void	changestate _FA_((void));
char *	check_link _FA_((Addr **, char *));
void	finish _FA_((int));
void	reroute _FA_((char *));
char *	stripexpl _FA_((char *, char *));

extern SigFunc	sigcatch;

#define	Fprintf		(void)fprintf



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	InitParams();

	DoArgs(argc, argv, Usage);

	GetInvoker(&Invoker);
	SetUser(NetUid, NetGid);

	Recover(ert_return);

	if ( geteuid() != NetUid || !(Invoker.P_flags & P_SU) )
	{
		Error(english("No permission."));
		exit(EX_NOPERM);
	}

	if ( !ReadRoute(SUMCHECK) )
	{
		Error(english("No routing tables."));
		exit(EX_OSFILE);
	}

	if ( All )
	{
		if ( Break || Dead || ReRouteOld || OldLink != NULLSTR || NewLink != NULLSTR )
		{
			Error(english("Ambiguous arguments."));
			exit(EX_USAGE);
		}

		Recover(ert_finish);
		NLinkAddress = HomeName;
	}
	else
	{
		if ( OldLink == NULLSTR || NewLink == NULLSTR )
		{
			Error(english("Must supply: \"address\" or \"old-link\", and \"new-link\"."));
			exit(EX_USAGE);
		}

		Recover(ert_finish);

		if ( StringAtHome(NewLink) )
		{
			ReDeliver = true;
			NLinkAddress = HomeName;
		}
		else
			NLinkAddress = check_link(&NLink_Addr, NewLink);

		if ( Break || Dead || ReRouteOld )
		{
			OLinkAddress = check_link(&OLink_Addr, OldLink);
			OLinkDirectory = LinkData.ln_name;
		}
		else
		{
			OLink_Addr = StringToAddr(newstr(OldLink), NO_STA_MAP);
			OldAddress = UnTypAddress(OLink_Addr);
			Trace2(1, "OldAddress=\"%s\"", OldAddress);
			if ( LinkLink != NULLSTR )
			{
				OLinkAddress = check_link(&LLink_Addr, LinkLink);
				OLinkDirectory = LinkData.ln_name;
				ReRouteOld = true;
				OldLink = EmptyString;
			}
		}
	}

	InitCmds(&Commands);

	(void)signal(SIGHUP, sigcatch);
	(void)signal(SIGINT, sigcatch);
	(void)signal(SIGQUIT, sigcatch);
	(void)signal(SIGTERM, sigcatch);

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	RoutingDir = concat(SPOOLDIR, ROUTINGDIR, NULLSTR);
	RoutngFile = concat(RoutingDir, Template, NULLSTR);

	if ( Break || Dead )
	{
		changestate();
		
		if ( Finish )
		{
			exit(EX_OK);
			return 0;
		}

		ReRouteOld = true;
	}

	if ( ReRouteOld )
		reroute(concat(OLinkDirectory, Slash, LINKCMDSNAME, Slash, NULLSTR));

	if ( Bad )
		reroute(BADDIR);
	reroute(PENDINGDIR);
	reroute(REROUTEDIR);

	/*
	**	Activate sleeping router
	*/

	if ( Rerouted && !DaemonActive(RoutingDir, true) )
		Warn(english("routing daemon not running."));

	if ( !Silent )
	{
		if ( Rerouted )
			(void)fprintf
			(
				stdout,
				english("%d message%s re-routed.\n"),
				Rerouted, Rerouted==1?EmptyString:english("s")
			);
		else
			(void)fprintf(stdout, english("No messages re-routed.\n"));
	}

	exit(EX_OK);
}



/*
**	Prefix addr with explicit head.
*/

char *
bangaddress(head, addr)
	char *		head;
	register char *	addr;
{
	register char *	np;
	register char *	cp;
	register char *	xp;
	register char *	yp;

	static char	bang[] = {ATYP_EXPLICIT, '\0'};
	static char	mult[] = {ATYP_MULTICAST, '\0'};

	switch ( *addr++ )
	{
	default:
		--addr;
		break;

	case ATYP_EXPLICIT:	/** Strip all explicit destinations **/
		if ( (np = strrchr(addr, ATYP_EXPLICIT)) != NULLSTR )
			addr = ++np;
		break;

	case ATYP_MULTICAST:
		cp = newstr(EmptyString);
		for ( ;; )
		{
			if ( (np = strchr(addr, ATYP_MULTICAST)) != NULLSTR )
				*np = '\0';
			yp = bangaddress(head, addr);
			xp = concat(cp, mult, yp, NULLSTR);
			free(yp);
			free(cp);
			cp = xp;
			if ( (addr = np) == NULLSTR )
				break;
			*addr++ = ATYP_MULTICAST;
		}
		return cp;
	}

	if ( strccmp(head, addr) == STREQUAL )
		return newstr(head);

	return concat(bang, head, bang, addr, NULLSTR);
}



/*
**	Run state program to alter state information for `OldLink'.
*/

void
changestate()
{
	register FILE *	fd;
	char *		errs;
	ExBuf		ea;

	FIRSTARG(&ea.ex_cmd) = STATE;
	/*
	**	(Accept new commands on stdin, and
	**	 update the state and routing files.)
	*/
	NEXTARG(&ea.ex_cmd) = "-crsC";

	fd = (FILE *)Execute(&ea, NULLVFUNCP, ex_pipe, SYSERROR);

	if ( Break )
	{
		if ( LinkData.ln_filter != NULLSTR )
			Fprintf(fd, english("halfbreak\t%s,%s\n"), HomeName, OLinkAddress);
		else
			Fprintf(fd, english("break\t%s,%s\n"), HomeName, OLinkAddress);
	}
	else
		Fprintf(fd, english("flag\t%s,%s\t+dead\n"), HomeName, OLinkAddress);

	if ( (errs = ExClose(&ea, fd)) != NULLSTR )
	{
		Error(StringFmt, errs);
		free(errs);
	}
}



/*
**	Check for real link.
*/

char *
check_link(linkap, name)
	register Addr **linkap;
	register char *	name;
{
	Trace2(1, "check_link(%s)", name);

	*linkap = StringToAddr(newstr(name), NO_STA_MAP);

	if ( FindLink(TypedName(*linkap), &LinkData) )
		return LinkData.ln_rname;

	if ( Break || Dead || ReRouteOld || FindAddr(*linkap, &LinkData, FASTEST) != wh_link )
	{
		Error("Unknown link \"%s\"", name);
		return NULLSTR;
	}

	if ( NewDest == NULLSTR )
	{
		NewDest = newstr(TypedAddress(*linkap));
		Trace2(1, "NewDest set to %s", NewDest);
	}

	return LinkData.ln_rname;
}



/*
**	Cleanup after error.
*/

void
finish(error)
	int	error;
{
	if ( !ExInChild && LockSet )
		(void)unlink(LockName);

	(void)exit(error);
}



/*
**	Process a command from commands file.
*/

bool
getcommand(type, val)
	CmdT		type;
	CmdV		val;
{
	static Ulong	mesgstart;

	switch ( type )
	{
	case address_cmd:
		CmdAddress = newstr(val.cv_name);
		SCmdAddress = StripTypes(val.cv_name);
		Trace2(2, "CmdAddress=\"%s\"", CmdAddress);
		return true;

	case end_cmd:
		MesgLength += val.cv_number - mesgstart;
		break;

	case file_cmd:
		HdrFile = AddCmd(&Commands, type, val);
		return true;

	case flags_cmd:
		CmdFlags = val.cv_number;
		return true;

	case linkname_cmd:
		LinkName = newstr(val.cv_name);
		break;

	case start_cmd:
		mesgstart = val.cv_number;
		break;

	case useaddr_cmd:
	case uselink_cmd:
		return true;

	default:
		break;	/* gcc -Wall */
	}

	(void)AddCmd(&Commands, type, val);

	return true;
}



/*
**	Optional `forward-via' address argument.
*/

/*ARGSUSED*/
AFuncv
getNewDest(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Addr *		ap;

	ap = StringToAddr(newstr(val.p), NO_STA_MAP);

	if ( FindAddr(ap, (Link *)0, FASTEST) == wh_none )
		return (AFuncv)english("unknown address");

	NewDest = newstr(TypedAddress(ap));
	Trace2(1, "NewDest set to %s", NewDest);
	FreeAddr(&ap);

	return ACCEPTARG;
}



/*
**	Select message priorities for rerouting.
*/

/*ARGSUSED*/
AFuncv
getQuality(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.c < STATE_QUALITY || val.c > LOWEST_QUALITY )
	{
		char *		errs;
		static char	errf[]	= english("max. priority must be in range %c (high) to %c (low)");

		errs = newstr(errf);
		(void)sprintf(errs, errf, STATE_QUALITY, LOWEST_QUALITY);
		return (AFuncv)errs;
	}

	return ACCEPTARG;
}



/*
**	Find command files in "dir" using OldLink, and re-route them via NewLink.
*/

void
reroute(dir)
	char *			dir;
{
	register struct direct *direp;
	register char *		cp1;
	register char *		fp1;
	register DIR *		dirp;
	register CmdV		cmdv;
	Addr *			ap;
	char *			commandtemp;
	CmdV			use_link_val;
	CmdV			use_addr_val;
	int			fd;

	Trace2(1, "reroute(%s)", dir);

	dir = concat(SPOOLDIR, dir, NULLSTR);
	LockName = concat(dir, REROUTELOCKFILE, NULLSTR);

	if ( !SetLock(LockName, Pid, false) )
	{
		Warn(CouldNot, LockStr, LockName);
		free(LockName);
		free(dir);
		return;
	}

	LockSet = true;

	if ( !ReDeliver )
		use_link_val.cv_name = NLinkAddress;

	commandtemp = UniqueName
		      (
			concat(SPOOLDIR, WORKDIR, Template, NULLSTR),
			CHOOSE_QUALITY, NOOPTNAME,
			(Ulong)MAX_MESG_DATA, Time
		      );

	cp1 = concat(dir, Template, NULLSTR);

	fp1 = cp1 + strlen(dir);

	while ( (dirp = opendir(dir)) == NULL )
		Syserror(CouldNot, ReadStr, dir);

	while ( (direp = readdir(dirp)) != NULL && !Finish )
	{
		/*
		**	Find valid command file.
		*/

		if ( direp->d_name[0] < STATE_QUALITY || direp->d_name[0] > Quality )
			continue;

		if ( strlen(direp->d_name) != UNIQUE_NAME_LENGTH )
			continue;

		Trace2(2, "found \"%s\"", direp->d_name);

		(void)strcpy(fp1, direp->d_name);

		FreeCmds(&Commands);
		FreeStr(&CmdAddress);
		FreeStr(&SCmdAddress);
		FreeStr(&LinkName);
		MesgLength = 0;
		HdrFile = 0;
		CmdFlags = 0;

		if ( !ReadCmds(cp1, getcommand) )
		{
			Warn(CouldNot, ReadStr, cp1);
			continue;
		}

		if ( RdFileTime >= Time )
		{
			char	d[DATETIMESTRLEN];

			if ( RdFileTime > (Time + 60) )
				Warn
				(
					english("Message file \"%s\" has future timestamp: %s"),
					cp1,
					DateTimeStr(RdFileTime, d)
				);

			continue;	/* Avoid looping the loop! */
		}

		if ( OldAddress != NULLSTR )
		{
			if
			(
				!ReRouteOld
				&&
				(
					CmdAddress == NULLSTR
					||
					(
						!AddressMatch(OldAddress, SCmdAddress)
						&&
						!AddressMatch(OldLink, CmdAddress)
						&&
						!AddressMatch(OldLink, SCmdAddress)
					)
				)
			)
				continue;
		}
		else
		if
		(
			!All
			&&
			!ReRouteOld
			&&
			(
				LinkName == NULLSTR
				||
				strccmp(LinkName, OLinkAddress) != STREQUAL
			)
		)
			continue;

		Trace3(1, "reroute %s via %s", (CmdAddress==NULLSTR)?NullStr:CmdAddress, NLinkAddress);

		if ( Verbose )
			(void)fprintf
			(
				stdout,
				english("Reroute \"%s\" via \"%s\"\n"),
				(CmdAddress==NULLSTR)?NullStr:StripTypes(CmdAddress),
				StripTypes(NLinkAddress)
			);

		if ( CmdAddress != NULLSTR )
		{
			char *	desc;

			if ( NewDest != NULLSTR )
			{
				use_addr_val.cv_name = bangaddress(NewDest, CmdAddress);
				desc = "forward";
useaddr:			Trace4(1, "%s %s to %s", desc, CmdAddress, use_addr_val.cv_name);
				(void)AddCmd(&Commands, useaddr_cmd, use_addr_val);
				free(use_addr_val.cv_name);
			}
			else
			if ( OLinkAddress != NULLSTR )
			{
				use_addr_val.cv_name = stripexpl(OLinkAddress, CmdAddress);
				desc = "strip";
				goto useaddr;
			}
	
			ap = StringToAddr(CmdAddress, NO_STA_MAP);	/* Clobbers `CmdAddress' */
			if ( !AddrIsBC(ap) && AddrAtHome(&ap, false, false) )
				CmdFlags &= ~RE_ROUTE;
			FreeAddr(&ap);
		}

		CmdFlags &= ~BOUNCE_MESG;
		CmdFlags |= RETRY;
		(void)AddCmd(&Commands, flags_cmd, (cmdv.cv_number = CmdFlags, cmdv));

		if ( !All && !ReDeliver )
			(void)AddCmd(&Commands, uselink_cmd, use_link_val);
		(void)AddCmd(&Commands, traveltime_cmd, (cmdv.cv_number = Time - RdFileTime, cmdv));

		fd = create(commandtemp);
		(void)WriteCmds(&Commands, fd, commandtemp);
		(void)close(fd);

#		if	PRIORITY_ROUTING == 1
		move(commandtemp, UniqueName(RoutngFile, fp1[0], NOOPTNAME, MesgLength, RdFileTime));
#		else	/* PRIORITY_ROUTING == 1 */
		move(commandtemp, UniqueName(RoutngFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, RdFileTime));
#		endif	/* PRIORITY_ROUTING == 1 */

		(void)unlink(cp1);

		Rerouted++;
	}

	closedir(dirp);

	(void)unlink(LockName);
	LockSet = false;

	free(cp1);
	free(commandtemp);
	free(LockName);
	free(dir);
}



/*
**	Catch system termination and wind up.
*/

void
sigcatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);

	Finish = true;
}



/*
**	Strip any explicit `old' from `addr'.
*/

char *
stripexpl(old, addr)
	char *		old;
	register char *	addr;
{
	register char *	np;
	register char *	cp;
	register char *	xp;
	register char *	yp;

	/* static char	bang[] = {ATYP_EXPLICIT, '\0'}; */
	static char	mult[] = {ATYP_MULTICAST, '\0'};

	switch ( *addr++ )
	{
	default:
		return newstr(--addr);

	case ATYP_EXPLICIT:
		for ( ;; )
		{
			if ( (np = strrchr(addr, ATYP_EXPLICIT)) != NULLSTR )
				*np = '\0';
			else
				return newstr(addr);
			if ( strccmp(old, addr) != STREQUAL )
			{
				*np = ATYP_EXPLICIT;
				return newstr(--addr);
			}
			*np++ = ATYP_EXPLICIT;
			addr = np;
		}

	case ATYP_MULTICAST:
		cp = newstr(EmptyString);
		for ( ;; )
		{
			if ( (np = strchr(addr, ATYP_MULTICAST)) != NULLSTR )
				*np = '\0';
			yp = stripexpl(old, addr);
			xp = concat(cp, mult, yp, NULLSTR);
			free(yp);
			free(cp);
			cp = xp;
			if ( (addr = np) == NULLSTR )
				break;
			*addr++ = ATYP_MULTICAST;
		}
		return cp;
	}
}
