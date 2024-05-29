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


static char	sccsid[]	= "@(#)spooler.c	1.23 05/12/16";

/*
**	Pass MHSnet message to foreign network handler.
**
**	MUST BE SETUID --> ROOT if FOREIGNUSERNAME != NETUSERNAME.
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
#include	"link.h"
#include	"params.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"statefile.h"
#include	"sub_proto.h"
#include	"sysexits.h"

/*
**	Parameters set from arguments.
*/

char *	HomeAddress;		/* Typed address of this node */
char *	LinkAddress;		/* Message destined for this link */
char *	LinkName;		/* Local name for link */
char *	Name	= sccsid;	/* Program invoked name */
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
**	Foreign network parameters.
*/

bool	ChUid;			/* True if NETUSERNAME != FOREIGNUSERNAME */
char *	FOREIGNUSERNAME;
Passwd	FgnPasswd;

#define	FOREIGNUID	FgnPasswd.P_uid
#define	FOREIGNGID	FgnPasswd.P_gid

char *	FgnRProg;

/*
**	List struct for holding address parts.
*/

typedef struct List
{
	struct List *	next;
	char *		addr;	/* Converted address */
	char *		raddr;	/* Real address */
}
	List;

List *	A_list;		/* List of addresses */
List *	U_list;		/* List of address+user */

/*
**	Miscellaneous.
*/

CmdHead	Cleanup;		/* Files to be unlinked */
bool	CRCIn;			/* Data CRC present */
CmdHead	DataCmds;		/* Describing FTP part of message */
char *	EnvDup;			/* Message possibly duplicated at address */
char *	EnvErr;			/* Error for returned message */
char *	EnvFlags;		/* Optional message environment flags */
char *	EnvID;			/* Message ID from environment */
char *	EnvTrunc;		/* Message truncated at address */
ExBuf	ExArgs;			/* Used to pass args to Execute() */
char *	ExplAddress;		/* Pre-fix for message addresses */
long	FTPhdrEnd;		/* Seek address for FTP header in MesgHdr */
char *	FtpMesgHdr;		/* File containing FTP header */
char *	MesgHdr		= english("No message header file!");
Ulong	MsgHdrSize;		/* Size of data in file containing header */
Ulong	MesgLength;		/* Length of whole message */
char *	Origin;			/* Original MHSnet node (not always == HdrSource) */
CmdHead	PartCmds;		/* Describing all parts received so far */
bool	Returned;		/* True if invoked for returned message */
int	RetVal		= EX_OK;
char *	SourceAddress;		/* Stripped source address */
char *	SourceNode;		/* Source node name */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	TraceLog;		/* Optional log file (default: stderr) */
char *	WorkName;		/* Template for spooling files in WORKDIR */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"FOREIGNUSERNAME",	&FOREIGNUSERNAME,		PSTRING},
	{"RECEIVER",		&FgnRProg,			PSPOOL},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
	{"TRACELOG",		&TraceLog,			PSTRING},
};


void	b_list _FA_((char *));
void	cleanup _FA_((void));
void	exec_addr _FA_((char *, char *, char *, char *, Ulong, Ulong));
char *	FirstName _FA_((char *));
void	ftp_exec_addr _FA_((char *, char *, char *));
bool	getcommand _FA_((CmdT, CmdV));
bool	get_origin _FA_((char *, Ulong, char *));
void	list _FA_((char *, char *));
void	makelists _FA_((char *));
void	passftp _FA_((int));
void	passftplist _FA_((void));
void	passother _FA_((void));
void	setflags _FA_((void));
void	setids _FA_((VarArgs *));
void	setTraceLog _FA_((char *));
bool	u_list _FA_((char *, char *));



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register int	fd;
	HdrReason	hdr_reason;
	static char	hdr_error[]	= english("Message protocol header \"%s\" error");

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

	if ( FgnRProg == NULLSTR )
		FgnRProg = concat(SPOOLDIR, LIBDIR, Name, ".sh", NULLSTR);

	if ( access(FgnRProg, 1) == SYSERROR )
		exit(EX_UNAVAILABLE);

	if ( FOREIGNUSERNAME == NULLSTR )
		FOREIGNUSERNAME = NETUSERNAME;
	else
	if ( strcmp(NETUSERNAME, FOREIGNUSERNAME) != STREQUAL )
	{
		if ( !GetUid(&FgnPasswd, FOREIGNUSERNAME) )
			Error(english("Who is \"%s\"?"), FOREIGNUSERNAME);
		ChUid = true;
	}

	if ( TraceLog != NULLSTR )
		setTraceLog(TraceLog);

	InitCmds(&Cleanup);
	InitCmds(&DataCmds);
	InitCmds(&PartCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, english("read commands from"), "stdin");

	while ( (fd = open(MesgHdr, O_READ)) == SYSERROR )
		Syserror(CouldNot, OpenStr, MesgHdr);

	Trace2(1, "Read header <= %s", MesgHdr);

	if ( (hdr_reason = ReadHeader(fd)) != hr_ok )
		Error(hdr_error, HeaderReason(hdr_reason));

	DataLength = MesgLength - HdrLength;

	Returned = (bool)((HdrFlags & HDR_RETURNED) != 0);

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	WorkName = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	Trace5(1, "Message %sfrom %s to %s for %s", Returned?"RETURNED ":NullStr, HdrSource, HdrDest, HdrHandler);

	setflags();	/* Flags from environment */

	SourceAddress = StripTypes(HdrSource);
	SourceNode = FirstName(HdrSource);

	(void)ExRoute(NULLSTR, HdrRoute, get_origin);
	Trace2(1, "Home address %s", HomeAddress);

	if ( strcmp(LinkAddress, HomeAddress) != STREQUAL )
	{
		ExplAddress = LinkAddress;
		Trace2(1, "Explicit address %s", ExplAddress);
	}

	if ( NewDest != NULLSTR )
		HdrDest = NewDest;

	switch ( HdrSubpt )
	{
	case FTP:
		passftp(fd);
		break;

	default:
		passother();
		break;
	}

	(void)close(fd);

	if ( RetVal == EX_OK )
		cleanup();

	finish(RetVal);
	return 0;
}



/*
**	List explicit address.
*/

void
b_list(ap)
	char *		ap;
{
	register char *	cp;
	register char *	np;
	register char *	xp;
	char *		rp;

	Trace2(1, "b_list(%s)", ap);

	cp = ap + 1;

	if ( strchr(cp, ATYP_BROADCAST) != NULLSTR )
	{
		list(FirstName(LinkAddress), LinkAddress);
		return;
	}

	rp = np = newstr(cp);

	while ( (xp = strchr(cp, ATYP_EXPLICIT)) != NULLSTR )
	{
		*xp = '\0';
		np = strcpyend(np, FirstName(cp));
		*np++ = '!';
		*xp++ = ATYP_EXPLICIT;
		cp = xp;
	}

	np = strcpyend(np, FirstName(cp));

	list(rp, ap);
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
**	Pass to foreign network handler.
*/

void
exec_addr(address, raddr, user, file_name, data_start, data_end)
	char *		address;
	char *		raddr;
	char *		user;
	char *		file_name;
	Ulong		data_start;
	Ulong		data_end;
{
	FILE *		fd;
	char *		errs;
	char		numbuf[NUMERICARGLEN];

	Trace4(1, "exec_addr(%s, %s, %s)", address, raddr, (user==NULLSTR)?NullStr:user);

	Recover(ert_return);

	FIRSTARG(&ExArgs.ex_cmd) = FgnRProg;
	if ( EnvDup != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = EnvDup;
	if ( EnvErr != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = EnvErr;
	if ( EnvFlags != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = EnvFlags;
	if ( EnvID != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = EnvID;
	if ( LinkName != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = LinkName;
	if ( EnvTrunc != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = EnvTrunc;
	if ( Origin != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = Origin;

	NEXTARG(&ExArgs.ex_cmd) = HdrHandler;
	NEXTARG(&ExArgs.ex_cmd) = HdrID;

	(void)sprintf(numbuf, "%lu", (PUlong)(data_end-data_start));
	NEXTARG(&ExArgs.ex_cmd) = numbuf;

	NEXTARG(&ExArgs.ex_cmd) = SourceNode;
	NEXTARG(&ExArgs.ex_cmd) = SourceAddress;
	NEXTARG(&ExArgs.ex_cmd) = address;
	NEXTARG(&ExArgs.ex_cmd) = raddr;

	if ( FthFrom != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = FthFrom;
	if ( user != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = user;
	if ( file_name != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = file_name;

	fd = (FILE *)Execute(&ExArgs, setids, ex_pipe, SYSERROR);

	if ( Returned || HdrPart[0] == '\0' )
		(void)CollectData(&DataCmds, data_start, data_end, fileno(fd), PipeStr);
	else
		(void)CollectData(&PartCmds, data_start, data_end, fileno(fd), PipeStr);

	if ( (errs = ExClose(&ExArgs, fd)) != NULLSTR )
	{
		if ( (RetVal = ExStatus) == 1 )
			RetVal = EX_UNAVAILABLE;
		else
		if ( (ExStatus & 0377) == 0 )
			RetVal = EX_SOFTWARE;
		Error(StringFmt, errs);
		free(errs);
	}

	Recover(ert_finish);
}



/*
**	Called on termination and from the errors routines to cleanup.
*/

void
finish(error)
	int	error;
{
	if ( !ExInChild && RetVal != EX_OK )
		error = RetVal;

	(void)exit(error);
}



/*
**	Return name of first domain in string.
*/

char *
FirstName(acp)
	char *		acp;
{
	register char *	cp;
	register char *	cp1;
	register char *	cp2;

	if ( (cp = acp) == NULLSTR )
		return newstr(EmptyString);

	if ( *cp == ATYP_DESTINATION )
		cp++;

	if ( (cp2 = strchr(cp, DOMAIN_SEP)) != NULLSTR )
		*cp2 = '\0';

	if ( (cp1 = strchr(cp, TYPE_SEP)) != NULLSTR )
		cp1++;
	else
		cp1 = cp;

	cp = newstr(cp1);

	if ( cp2 != NULLSTR )
		*cp2 = DOMAIN_SEP;

	return cp;
}



/*
**	Pass to foreign network handler.
*/

void
ftp_exec_addr(address, raddr, user)
	char *		address;
	char *		raddr;
	char *		user;
{
	register FthFD_p fp;
	register Ulong	start;
	register Ulong	end;

	if ( Returned )
	{
		char *	cp;

		if ( NFthFiles )
			cp = FthFiles->f_name;
		else
			cp = NULLSTR;

		exec_addr(address, raddr, user, cp, (Ulong)0, DataLength);
		return;
	}

	for ( fp = FthFiles, start = 0 ; fp != (FthFD_p)0 ; fp = fp->f_next )
	{
		end = start + fp->f_length;
		exec_addr(address, raddr, user, fp->f_name, start, end);
		start = end;
	}
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
	static Ulong	filestart;
	static Ulong	fileend;

	switch ( type )
	{
	case end_cmd:
		fileend = val.cv_number;
		MsgHdrSize = fileend - filestart;
		MesgLength += MsgHdrSize;
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

	default:
		break;	/* gcc -Wall */
	}

	(void)AddCmd(&DataCmds, type, val);

	return true;
}



/*
**	Extract first node in route == Origin
*/

bool
get_origin(a1, tt, origin)
	char *	a1;
	Ulong	tt;
	char *	origin;
{
	Trace2(1, "MHSnet origin %s", origin);

	origin = StripTypes(origin);

	if ( strccmp(origin, SourceAddress) != STREQUAL )
		Origin = concat("-O", origin, NULLSTR);

	free(origin);
	return false;	/* No more */
}



/*
**	Add address list element.
*/

void
list(address, raddr)
	char *	address;
	char *	raddr;
{
	register List *	lp;

	Trace3(1, "list(%s, %s)", address, raddr);

	for ( lp = A_list ; lp != (List *)0 ; lp = lp->next )
		if ( strccmp(address, lp->addr) == STREQUAL )
			return;

	lp = Talloc(List);
	lp->next = A_list;
	A_list = lp;
	lp->addr = address;
	lp->raddr = StripTypes(raddr);
}



/*
**	Split destination into lists depending on contents.
*/

void
makelists(ap)
	register char *	ap;
{
	register char *	np;

	Trace2(1, "makelists(%s)", ap);

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
		if ( ExplAddress == NULLSTR )
		{
			b_list(--ap);
			break;
		}

		for ( ;; )
		{
			if ( (np = strchr(ap, ATYP_EXPLICIT)) == NULLSTR )
			{
				list(FirstName(ap), ap);
				break;
			}

			*np = '\0';
			if ( *ap == ATYP_DESTINATION )
				ap++;

			if
			(
				strccmp(ExplAddress, ap) != STREQUAL
				&&
				strccmp(HomeAddress, ap) != STREQUAL
			)
			{
				*np = ATYP_EXPLICIT;
				if ( *--ap == ATYP_DESTINATION )
					ap--;
				b_list(ap);
				break;
			}

			ap = ++np;
		}

		break;

	case ATYP_BROADCAST:
		list(FirstName(LinkAddress), LinkAddress);
		break;

	default:
		list(FirstName(ap), ap);
		break;
	}
}



/*
**	Check FTP message and pass on if all correct.
*/

void
passftp(fd)
	register int		fd;		/* Contains header */
{
	FthReason		fth_reason;
	static char		fth_error[]	= english("File transfer protocol header \"%s\" error");

	Trace2(1, "passftp(%d)", fd);

	if ( MsgHdrSize == HdrLength )
	{
		while ( (fd = open(FtpMesgHdr, O_READ)) == SYSERROR )
			Syserror(CouldNot, ReadStr, FtpMesgHdr);
	}
	else
		FTPhdrEnd = MsgHdrSize - HdrLength;

	if ( (fth_reason = ReadFtHeader(fd, FTPhdrEnd)) != fth_ok )
		Error(fth_error, FTHREASON(fth_reason));

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
		Error(fth_error, FTHREASON(fth_baddatacrc));

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

	if ( Returned )
	{
		(void)GetFthTo(false);	/* Build UQFthTo list */
		FthUsers = Talloc0(FthUlist);
		FthUsers->u_name = FthFrom;
		NFthUsers = 1;
		FthFrom = UQFthTo;
		UQFthTo = FthUsers->u_name;
	}
	else
	if ( GetFthTo(false) == 0 )
	{
		RetVal = EX_DATAERR;
		Error(english("addressing error - no users in list \"%s\""), UQFthTo);
		return;	/* No users! */
	}

	Trace2(1, "userlist: %s", UQFthTo);

	(void)GetFthFiles();

	if ( Returned || HdrPart[0] == '\0' )
		DataLength = FtDataLength;
	else
		DataLength = PartsLength;

	/*
	**	Convert destination into a set of foreign network destinations.
	*/

	makelists(HdrDest);

	/*
	**	Pass copy of message for each sub-address.
	*/

	passftplist();	/* Try exact match first */

	MatchRegion = true;

	passftplist();	/* Try region match on any remaining */

	MatchRegion = false;
}



/*
**	Check FTP addresses and pass on if match.
**
**	What really needs to be done here, is for each address to be routed,
**	and the route matched against the link. But this ignores explicitly
**	re-routed addresses, and the task of matching original address (in
**	FtHeader) with final address (from Header) is almost impossible!
*/

void
passftplist()
{
	register FthUlist *	up;
	register char *		cp;
	register List *		lp;
	register bool		match;

	Trace1(1, "passftplist()");

	for ( lp = A_list ; lp != (List *)0 ; lp = lp->next )
	{
		/*
		**	Deliver as per user addresses.
		*/

		Trace2(1, " ADDRESS: %s", lp->raddr);

		for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
		{
			if ( up->u_name == NULLSTR )
				continue;

			cp = up->u_dest;

			Trace3(1, "  USER:   %s@%s", up->u_name, (cp==NULLSTR)?NullStr:cp);

			if ( !Returned && ExplAddress != NULLSTR && (cp == NULLSTR || StringAtHome(cp)) )
				continue;

			match = false;

			if ( cp == NULLSTR || Returned )
				match = true;
			else
			if ( AddressMatch(cp, lp->raddr) )
				match = true;
			else
			{
				Addr *	ap;

				cp = newstr(cp);
				ap = StringToAddr(cp, NO_STA_MAP);
				if ( AddressMatch(UnTypAddress(ap), lp->raddr) )
					match = true;
				FreeAddr(&ap);
				free(cp);
			}

			if ( match )
			{
				if ( u_list(lp->raddr, up->u_name) )
					ftp_exec_addr(lp->addr, lp->raddr, up->u_name);

				up->u_name = NULLSTR;
			}
		}
	}
}



/*
**	Check state (or other) message and pass on if all correct.
*/

void
passother()
{
	register List *	lp;

	Trace1(1, "passother()");

	if ( !Returned && HdrPart[0] != '\0' )
	{
		if
		(
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

		DataLength = PartsLength;
	}

	/*
	**	Convert destination into a set of foreign network destinations.
	*/

	makelists(HdrDest);

	/*
	**	Pass copy of message for each sub-address.
	*/

	for ( lp = A_list ; lp != (List *)0 ; lp = lp->next )
		exec_addr(lp->addr, lp->raddr, NULLSTR, NULLSTR, (Ulong)0, DataLength);
}



void
setflags()
{
	register char *	ep;

	if ( (ep = GetEnv(ENV_FLAGS)) != NULLSTR )
	{
		EnvFlags = concat("-F", ep, NULLSTR);
		free(ep);
	}

	if ( (ep = GetEnv(ENV_DUP)) != NULLSTR )
	{
		EnvDup = concat("-D", FirstName(ep), NULLSTR);
		free(ep);
	}

	if ( (ep = GetEnv(ENV_ERR)) != NULLSTR )
	{
		EnvErr = concat("-E", CleanError(ep), NULLSTR);
		free(ep);
	}

	if ( (ep = GetEnv(ENV_ID)) != NULLSTR )
	{
		EnvID = concat("-I", ep, NULLSTR);
		free(ep);
	}

	if ( (ep = GetEnv(ENV_TRUNC)) != NULLSTR )
	{
		EnvTrunc = concat("-T", FirstName(ep), NULLSTR);
		free(ep);
	}

	if ( strchr(LinkName, '/') == NULLSTR )
		LinkName = concat("-L", LinkName, NULLSTR);
	else
		LinkName = NULLSTR;
}



void
setids(vap)
	VarArgs *	vap;
{
	if ( ChUid )
		SetUser(FOREIGNUID, FOREIGNGID);
}



/*
**	Setup file for error/trace messages.
*/

void
setTraceLog(file)
	char *		file;
{
	register FILE *	fd;

	(void)fflush(TraceFd);

	while ( (fd = fopen(file, "a")) == NULL )
	{
		(void)unlink(file);
		(void)close(create(file));
	}

	TraceFd = fd;

#	if	FCNTL == 1 && O_APPEND != 0
	(void)fcntl
	(
		fileno(fd),
		F_SETFL,
		fcntl(fileno(fd), F_GETFL, 0) | O_APPEND
	);
#	endif

	Trace(0, "__");
}



/*
**	Keep list of user+address, return true if unique.
*/

bool
u_list(address, user)
	char *	address;
	char *	user;
{
	register List *	lp;

	Trace3(1, "u_list(%s, %s)", address, user);

	for ( lp = U_list ; lp != (List *)0 ; lp = lp->next )
	{
		if ( strccmp(address, lp->addr) != STREQUAL )
			continue;

		if ( strccmp(user, lp->raddr) != STREQUAL )
			continue;

		return false;
	}

	lp = Talloc(List);
	lp->next = U_list;
	U_list = lp;
	lp->addr = address;
	lp->raddr = user;

	return true;
}
