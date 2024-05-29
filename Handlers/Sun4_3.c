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


static char	sccsid[]	= "@(#)Sun4_3.c	1.10 05/12/16";
char *	Name	= sccsid;	/* Program invoked name */

/*
**	Pass Sun4 message to Sun3.
**
**	MUST BE SETUID --> ROOT if SUN3UID != SUN4UID.
*/

#define	FILE_CONTROL
#define	RECOVER
#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"ftheader.h"
#include	"handlers.h"
#include	"header.h"
#include	"params.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"statefile.h"
#include	"sub_proto.h"
#include	"sysexits.h"

#ifdef	SUN3SPOOLDIR

#include	"sun3.h"

/*
**	Parameters set from arguments.
*/

char *	HomeAddress;		/* Typed address of this node */
char *	LinkAddress;		/* Message destined for this link */
char *	LinkName;		/* Local name for link */
char *	NewDest;		/* Override HdrDest if set */
int	Traceflag;		/* Global tracing control */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_string(D, &NewDest, 0, english("new-dest"), OPTARG),
	Arg_string(H, &HomeAddress, 0, english("home"), 0),
	Arg_string(L, &LinkAddress, 0, english("link address"), 0),
	Arg_string(N, &LinkName, 0, english("link name"), 0),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_end
};

/*
**	Sun3 magic.
*/

bool	ChUid;			/* True if NETUSERNAME != SUN3USERNAME */
Passwd	SUN3Passwd;

#define	SUN3UID		SUN3Passwd.P_uid
#define	SUN3GID		SUN3Passwd.P_gid

char	Sun3ERsa[]	= { Sun3ERs, '\0' };
char	Sun3EUsa[]	= { Sun3EUs, '\0' };
char	Sun3RQa[]	= { Sun3RS, Sun3RT, '\0' };
char	Sun3RProg[]	= "receiver";
char *	Sun3MesgFile;
char *	Sun3WorkFile;

/*
**	List struct for holding address parts.
*/

typedef struct List
{
	struct List *	next;
	char *		addr;
}
	List;

List *	B_list;		/* List of addresses with explicit routes */
List *	M_list;		/* List of multicast addresses */

/*
**	Miscellaneous
*/

CmdHead	Commands;		/* Describing FTP part of message when spooling */
CmdHead	Cleanup;		/* Files to be unlinked */
bool	CRCIn;			/* Data CRC present */
CmdHead	DataCmds;		/* Describing FTP part of message */
ExBuf	ExArgs;			/* Used to pass args to Execute() */
char *	ExplAddress;		/* Pre-fix for message addresses */
long	FTPhdrEnd;		/* Seek address for FTP header in MesgHdr */
char *	FtpMesgHdr	= english("No FTP header file!");
char *	HRp;			/* Pointer into converted route */
char *	LRp;			/* Pointer to last homenode in converted route */
char *	LastRtNode;		/* Inbound link for message */
Ulong	LastRtTime;		/* Inbound travel time for message */
int	LenExplAddr;		/* Length of explicit prefix for addresses */
char *	MesgHdr		= english("No message header file!");
Ulong	MesgLength;		/* Length of whole message */
bool	NoOpt;			/* Unoptimised message transmission */
CmdHead	PartCmds;		/* Describing all parts received so far */
bool	Returned;		/* True if invoked for returned message */
int	RetVal		= EX_OK;
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	WorkName;		/* Template for spooling files in WORKDIR */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"SUN3LIBDIR",		&SUN3LIBDIR,			PDIR},
	{"SUN3SPOOLDIR",	&SUN3SPOOLDIR,			PDIR},
	{"SUN3STATEP",		&SUN3STATEP,			PSTRING},
	{"SUN3USERNAME",	&SUN3USERNAME,			PSTRING},
	{"SUN3WORKDIR",		&SUN3WORKDIR,			PDIR},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};


void	b_list _FA_((char *));
void	cleanup _FA_((void));
bool	convroute _FA_((vchar *, Ulong, char *));
void	exec_addr _FA_((int, bool));
bool	getcommand _FA_((CmdT, CmdV));
void	makelists _FA_((char *)),
char *	make_env _FA_((char *, char *, char *));
void	m_list _FA_((char *));
void	passftp _FA_((void));
void	passon _FA_((int, bool));
void	passother _FA_((bool));
void	setids _FA_((void));

#endif	/* SUN3SPOOLDIR */



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
#	ifdef	SUN3SPOOLDIR
	register char *	ep;
	register int	fd;
	HdrReason	hdr_reason;
	static char	hdr_error[]	= english("Message protocol header \"%s\" error");

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(SUN3PARAMS, Params, sizeof Params);

	if ( SUN3SPOOLDIR == NULLSTR || access(SUN3SPOOLDIR, 0) == SYSERROR )
		exit(EX_UNAVAILABLE);

	if ( strcmp(NETUSERNAME, SUN3USERNAME) != STREQUAL )
	{
		if ( !GetUid(&SUN3Passwd, SUN3USERNAME) )
			Error(english("Who is \"%s\"?"), SUN3USERNAME);
		ChUid = true;
	}

	InitCmds(&Cleanup);
	InitCmds(&Commands);
	InitCmds(&DataCmds);
	InitCmds(&PartCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, english("read commands from"), "stdin");

	while ( (fd = open(MesgHdr, O_READ)) == SYSERROR )
		Syserror(CouldNot, OpenStr, MesgHdr);

	if ( (hdr_reason = ReadHeader(fd)) != hr_ok )
		Error(hdr_error, HeaderReason(hdr_reason));

	(void)close(fd);

	DataLength = MesgLength - HdrLength;

	NoOpt = (bool)((HdrFlags & HDR_NOOPT) != 0);
	Returned = (bool)((HdrFlags & HDR_RETURNED) != 0);

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	WorkName = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	Sun3WorkFile = concat(SUN3SPOOLDIR, SUN3WORKDIR, Template, NULLSTR);
	Sun3MesgFile = concat(SUN3SPOOLDIR, SUN3WORKDIR, Template, NULLSTR);

	if ( (ep = GetEnv(ENV_FLAGS)) == NULLSTR || strcmp(ep, NOTSUN3) != STREQUAL )
	switch ( HdrSubpt )
	{
	case STATE_PROTO:
		passother(true);
		break;

	case FTP:
		passftp();
		break;

	default:
		passother(false);
		break;
	}

	if ( RetVal == EX_OK )
		cleanup();

	if ( Sun3WorkFile != NULLSTR )
		(void)unlink(Sun3WorkFile);

	exit(RetVal);

#	else	/* SUN3SPOOLDIR */
	exit(EX_OK);
#	endif	/* SUN3SPOOLDIR */
	return 0;
}

#ifdef	SUN3SPOOLDIR



/*
**	Add explicit list element.
*/

void
b_list(address)
	char *	address;
{
	register List *	lp;

	lp = Talloc(List);
	lp->next = B_list;
	B_list = lp;
	lp->addr = address;
}



/*
**	Do unlink commands.
*/

void
cleanup()
{
	register Cmd *	cmdp;

	for ( cmdp = PartCmds.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
			(void)unlink(cmdp->cd_value);

	for ( cmdp = Cleanup.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
			(void)unlink(cmdp->cd_value);
}



/*
**	Convert route.
*/
/*ARGSUSED*/
bool
convroute(from, tt, to)
	char *		from;
	Ulong		tt;
	char *		to;
{
	register char *	cp;
	bool		home;
	char		numb[12];

	if ( AddressMatch(to, HomeAddress) )
	{
		LRp = HRp;	/* Remember last HomeAddress */
		LastRtTime = tt;
		home = true;
	}
	else
		home = false;

	if ( (cp = strchr(to, DOMAIN_SEP)) != NULLSTR )
		*cp = '\0';
	if ( (cp = strrchr(to, TYPE_SEP)) != NULLSTR )
		to = ++cp;

	if ( !home )
	{
		FreeStr(&LastRtNode);
		LastRtNode = newstr(to);
	}

	(void)sprintf(numb, "%lu", (PUlong)tt);
	*HRp++ = Sun3RS;
	HRp = strcpyend(HRp, to);
	*HRp++ = Sun3RT;
	HRp = strcpyend(HRp, numb);

	return true;
}



/*
**	Write a header for an address, and invoke Sun3 spooler.
*/

void
exec_addr(fd, local)
	int		fd;
	bool		local;
{
	char *		errs;
	char		nbuf[NUMERICARGLEN];
	static int	MinHdrLength;

	FIRSTARG(&ExArgs.ex_cmd) = concat(SUN3SPOOLDIR, SUN3LIBDIR, Sun3RProg, NULLSTR);

	if ( access(ARG(&ExArgs.ex_cmd, 0), 1) == SYSERROR )
	{
		RetVal = EX_UNAVAILABLE;
		return;
	}

	Recover(ert_return);

	/*
	**	Write new message header in message file.
	**	(Works 'cos Sun3 router will make a new header file before returning below.)
	*/

	while ( WriteHeader(fd, (long)DataLength, MinHdrLength) == SYSERROR )
		Syserror(CouldNot, WriteStr, Sun3WorkFile);

	if ( HdrLength > MinHdrLength )
		MinHdrLength = HdrLength;

	make_link(Sun3WorkFile, UniqueName(Sun3MesgFile, '2', NoOpt, MesgLength, Time));

	/*
	**	Pass to Sun3 router.
	*/

	if ( local )
		NEXTARG(&ExArgs.ex_cmd) = "-EL";	/* Report errors back to router */
	else
		NEXTARG(&ExArgs.ex_cmd) = "-E";		/* Just ignore route */
	if ( LastRtNode != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = concat("-l", LastRtNode, NULLSTR);
	if ( LastRtTime != 0 )
		NEXTARG(&ExArgs.ex_cmd) = NumericArg(nbuf, 't', LastRtTime);
	NEXTARG(&ExArgs.ex_cmd) = Sun3MesgFile;

	if ( (errs = Execute(&ExArgs, setids, ex_exec, SYSERROR)) != NULLSTR )
	{
		if ( (RetVal = ExStatus) != 1 )
			RetVal = EX_UNAVAILABLE;
		else
			RetVal = EX_OSERR;
		Error(StringFmt, errs);
		free(errs);
		if ( !local )
			(void)unlink(Sun3MesgFile);
	}

	Recover(ert_finish);
}



/*
**	Called from the errors routines to cleanup.
*/

void
finish(error)
	int	error;
{
	if ( !ExInChild )
	{
		if ( Sun3WorkFile != NULLSTR )
			(void)unlink(Sun3WorkFile);
		if ( Sun3MesgFile != NULLSTR )
			(void)unlink(Sun3MesgFile);
	}

	(void)exit(error);
}



/*
**	Process a command from commands file.
**
**	Last two files contain FTP header, and message header.
*/

bool
getcommand(type, val)
	CmdT		type;
	CmdV		val;
{
	static long	filestart;
	static long	fileend;

	switch ( type )
	{
	case end_cmd:
		fileend = val.cv_number;
		MesgLength += fileend - filestart;
		break;

	case file_cmd:
		FtpMesgHdr = MesgHdr;
		FTPhdrEnd = fileend;
		MesgHdr = AddCmd(&DataCmds, type, val);
		return true;

	case start_cmd:
		filestart = val.cv_number;
		break;

	case unlink_cmd:
		(void)AddCmd(&Cleanup, type, val);
		return true;
	}

	(void)AddCmd(&DataCmds, type, val);

	return true;
}



/*
**	Add multicast list element.
*/

void
m_list(address)
	char *	address;
{
	register List *	lp;

	lp = Talloc(List);
	lp->next = M_list;
	M_list = lp;
	lp->addr = address;
}



/*
**	Special Sun3 MakeEnv
*/

char *
make_env(env, name, val)
	char *	env;
	char *	name;
	char *	val;
{
	QuoteChars(val, Sun3RQa);
	return concat(env, Sun3ERsa, name, Sun3EUsa, val, NULLSTR);
}



/*
**	Split destination into lists depending on contents.
*/

void
makelists(ap)
	register char *	ap;
{
	register char *	np;

	switch ( *ap++ )
	{
	case ATYP_MULTICAST:
		do
		{
			if ( (np = strchr(ap, ATYP_MULTICAST)) != NULLSTR )
				*np++ = '\0';
			makelists(ap);
		}
		while
			( (ap = np) != NULLSTR );
		break;

	case ATYP_EXPLICIT:
		if
		(
			(np = strrchr(ap, ATYP_EXPLICIT)) != NULLSTR
			&&
			ExplAddress != NULLSTR && LenExplAddr == (np-ap)
			&&
			strnccmp(ExplAddress, ap, LenExplAddr) == STREQUAL
		)
			m_list(++np);
		else
		if
		(
			(np = strchr(ap, ATYP_EXPLICIT)) != NULLSTR
			&&
			ExplAddress != NULLSTR && LenExplAddr == (np-ap)
			&&
			strnccmp(ExplAddress, ap, LenExplAddr) == STREQUAL
		)
			b_list(np);
		else
			b_list(--ap);
		break;

	default:
		m_list(--ap);
		break;
	}
}



/*
**	Check FTP message and pass on if all correct.
*/

void
passftp()
{
	register FthUlist *	up;
	register char *		cp;
	register int		fd;
	FthReason		fth_reason;
	static char		fth_error[]	= english("File transfer protocol header \"%s\" error");

	while ( (fd = open(FtpMesgHdr, O_READ)) == SYSERROR )
		Syserror(CouldNot, OpenStr, MesgHdr);

	if ( (fth_reason = ReadFtHeader(fd, FTPhdrEnd)) != fth_ok )
		Error(fth_error, FTHREASON(fth_reason));

	(void)close(fd);

	FtDataLength = DataLength - FthLength;	/* In case multi-file message */

	if ( FthType[0] & FTH_DATACRC )
		CRCIn = true;

	if
	(
		!Returned
		&&
		CRCIn
		&&
		!CheckData(&DataCmds, (Ulong)0, FtDataLength, &DataCrc)
	)
	{
		Recover(ert_return);
		Error(fth_error, FTHREASON(fth_baddatacrc));
		RetVal = EX_DATAERR;
		Recover(ert_finish);
		return;
	}

	if
	(
		!Returned
		&&
		HdrPart[0] != '\0'
		&&
		!AllParts(
				&DataCmds,
				(Ulong)0,
				FtDataLength,
				MesgLength,
				&PartCmds,
				WorkName,
				StringCRC32(LinkAddress)
			 )
	)
		return;

	if ( GetFthTo(false) == 0 )
	{
		Error(english("addressing error - no users in list \"%s\""), UQFthTo);
		return;	/* No users! */
	}

	/*
	**	Collect data into new message.
	*/

	fd = create(UniqueName(Sun3WorkFile, '2', NoOpt, MesgLength, Time));

	if ( ChUid )
		(void)chown(Sun3WorkFile, SUN3UID, SUN3GID);

	if ( Returned || HdrPart[0] == '\0' )
	{
		if ( !CollectData(&DataCmds, (Ulong)0, FtDataLength, fd, Sun3WorkFile) )
			finish(EX_OSERR);
	}
	else
	{
		if ( !CollectData(&PartCmds, (Ulong)0, PartsLength, fd, Sun3WorkFile) )
			finish(EX_OSERR);

		if ( CRCIn )
		{
			/*
			**	Re-create CRC for whole message
			*/

			DataCrc = 0;
			(void)CheckData(&PartCmds, (Ulong)0, PartsLength, &DataCrc);
		}

		FtDataLength = PartsLength;
	}

	/*
	**	Convert addresses. (Remove any trailing Au domain.)
	*/

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
	{
		if ( up->u_name == NULLSTR )
			continue;

		if ( (cp = up->u_dest) != NULLSTR )
			up->u_dest = StripDomain(cp, OzAu, Au, true);
	}

	/*
	**	Write new FTP header.
	**	Magic numbers from Sun3/Include/ftheader.h
	*/

	SetFthTo();
	
	FthType[0] = (1<<2)|0100;
	if ( HdrFlags & HDR_RQA )
		FthType[0] |= (1<<0);
	if ( CRCIn )
		FthType[0] |= (1<<1);

	(void)WriteFtHeader(fd, (long)FtDataLength);

	DataLength = FtDataLength + FthLength;

	passon(fd, true);
}



/*
**	Pass message to Sun3 via ``receiver''.
*/

void
passon(fd, local)
	int		fd;
	bool		local;
{
	register char *	cp;
	register char *	ep;
	register List *	lp;
	register char *	np;

	/*
	**	Make environment. (HDR_FLAGS -> ENV_ strings.)
	**	Magic strings from Sun3/Include/header.h
	*/

	cp = make_env(EmptyString, ENV3_ID, HdrID);	/* Saved in case message crosses back into Sun4 */

	if ( (ep = GetEnv(ENV_FLAGS)) != NULLSTR )
	{
		if ( strcmp(ep, STATE_ENQ_ALL) == STREQUAL )
		{
			cp = concat(cp, Sun3ERsa, ENV3_ENQALL, NULLSTR);
			HdrFlags &= ~HDR_ENQ;
		}
		else
			cp = make_env(cp, ENV3_HANDLER_FLAGS, ep);
	}

	if ( HdrFlags & HDR_ACK )
		cp = concat(cp, Sun3ERsa, ENV3_ACK, NULLSTR);
	if ( HdrFlags & HDR_ENQ )
		cp = concat(cp, Sun3ERsa, ENV3_ENQ, NULLSTR);
	if ( HdrFlags & HDR_NO_AUTOCALL )
		cp = concat(cp, Sun3ERsa, ENV3_NO_AUTOCALL, NULLSTR);
	if
	(
		NoOpt
		&&
		strcmp(HdrHandler, MAILHANDLER) != STREQUAL
		&&
		strcmp(HdrHandler, STATEHANDLER) != STREQUAL
	)
		cp = concat(cp, Sun3ERsa, ENV3_NOOPT, NULLSTR);
	if ( HdrFlags & HDR_NORET )
		cp = concat(cp, Sun3ERsa, ENV3_NORET, NULLSTR);
	if ( Returned )
		cp = concat(cp, Sun3ERsa, ENV3_RETURNED, NULLSTR);
	if ( HdrFlags & HDR_REV_CHARGE )
		cp = concat(cp, Sun3ERsa, ENV3_REV_CHARGE, NULLSTR);
	if ( HdrSubpt == STATE_PROTO && DataLength > 0 )
		cp = concat(cp, Sun3ERsa, ENV3_STATE, NULLSTR);

	if ( (ep = GetEnv(ENV_DEST)) != NULLSTR )
		cp = make_env(cp, ENV3_DESTINATION, ep);
	if ( (ep = GetEnv(ENV_DUP)) != NULLSTR )
		cp = make_env(cp, ENV3_DUP, ep);
	if ( (ep = GetEnv(ENV_ERR)) != NULLSTR )
		cp = make_env(cp, ENV3_ERR1, ep);
	if ( (ep = GetEnv(ENV_ROUTE)) != NULLSTR )
	{
		HRp = np = newstr(ep);
		if ( (ep = GetEnv(ENV_SOURCE)) == NULLSTR )
			ep = HdrDest;
		(void)ExRoute(ep, np, convroute);
		FreeStr(&LastRtNode);				/* This one irrelevant! */
		LRp = NULLSTR;
		cp = make_env(cp, ENV3_ROUTE, np);
	}
	if ( (ep = GetEnv(ENV_TRUNC)) != NULLSTR )
		cp = make_env(cp, ENV3_TRUNC, ep);
	if ( NewDest != NULLSTR )
	{
		cp = concat(cp, Sun3ERsa, NOTSUN4, NULLSTR);	/* Don't pass this back */
		cp = make_env(cp, ENV3_DESTINATION, HdrDest);	/* Save old destination */
	}

	HdrEnv = cp;

	/*
	**	Convert HdrRoute.
	*/

	HRp = cp = newstr(HdrRoute);				/* Used by convroute() */
	*cp = '\0';						/* Might be no route */
	(void)ExRoute(HdrSource, HdrRoute, convroute);
	if ( LRp != NULLSTR )
		*LRp = '\0';					/* Don't include home */
	HdrRoute = cp;

	/*
	**	Other header fields.
	*/

	HdrType = HDR_TYPE1;
	HdrID = NULLSTR;
	HdrPart = NULLSTR;
	HdrQuality = '\0';
	HdrFlags = 0;

	/*
	**	Convert addresses. (Remove any trailing Au domain.)
	*/

	HdrSource = StripDomain(HdrSource, OzAu, Au, false);

	if ( strcmp(LinkAddress, HomeAddress) != STREQUAL )
	{
		ExplAddress = StripDomain(LinkAddress, OzAu, Au, false);
		LenExplAddr = strlen(ExplAddress);
	}

	if ( NewDest != NULLSTR )
		HdrDest = NewDest;
	if ( (HdrDest = StripDomain(HdrDest, OzAu, Au, false))[0] == '\0' )
		return;	/* HdrDest == Au */

	/*
	**	Convert Sun4 destination into a set of Sun3 destinations.
	*/

	cp = Malloc(strlen(HdrDest)+LenExplAddr+3);

	makelists(HdrDest);

	/*
	**	Pass copy of message for each sub-address.
	*/

	HdrDest = ep = cp;

	if ( ExplAddress != NULLSTR )
	{
		*cp++ = '!';
		ep = cp = strcpyend(cp, ExplAddress);
		*cp++ = '!';
	}

	while ( (lp = M_list) != (List *)0 )
	{
		if ( lp->next != (List *)0 )
			*cp++ = ',';
		else
		if
		(
			lp == M_list
			&&
			ExplAddress != NULLSTR
			&&
			strccmp(lp->addr, ExplAddress) == STREQUAL
		)
		{
			cp = HdrDest;
			HdrDest = lp->addr;
			exec_addr(fd, local);
			HdrDest = cp;
			break;
		}

		do
		{
			cp = strcpyend(cp, lp->addr);
			*cp++ = ',';
		}
		while
			( (lp = lp->next) != (List *)0 );

		*--cp = '\0';

		exec_addr(fd, local);

		break;
	}

	if ( (lp = B_list) != (List *)0 )
	{
		do
		{
			(void)strcpy(ep, lp->addr);
			exec_addr(fd, local);
		}
		while
			( (lp = lp->next) != (List *)0 );
	}

	(void)close(fd);
}



/*
**	Check state (or other) message and pass on if all correct.
*/

void
passother(state)
	bool		state;
{
	register int	fd;

	if
	(
		!Returned
		&&
		HdrPart[0] != '\0'
		&&
		!AllParts(
				&DataCmds,
				(Ulong)0,
				DataLength,
				MesgLength,
				&PartCmds,
				WorkName,
				StringCRC32(LinkAddress)
			 )
	)
		return;

	/*
	**	Collect data into new message.
	*/

	fd = create(UniqueName(Sun3WorkFile, '2', NoOpt, MesgLength, Time));

	if ( ChUid )
		(void)chown(Sun3WorkFile, SUN3UID, SUN3GID);

	if ( Returned || HdrPart[0] == '\0' )
	{
		if ( !CollectData(&DataCmds, (Ulong)0, DataLength, fd, Sun3WorkFile) )
			finish(EX_OSERR);
	}
	else
	{
		if ( !CollectData(&PartCmds, (Ulong)0, PartsLength, fd, Sun3WorkFile) )
			finish(EX_OSERR);

		DataLength = PartsLength;
	}

	passon(fd, state?false:true);
}



void
setids()
{
	if ( ChUid )
		SetUser(SUN3UID, SUN3GID);
}

#endif	/* SUN3SPOOLDIR */
