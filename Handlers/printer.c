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


static char	sccsid[]	= "@(#)printer.c	1.20 05/12/16";

/*
**	Process print jobs from message received from network.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	RECOVER
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
#include	"forward.h"
#include	"ftheader.h"
#include	"header.h"
#include	"params.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"sub_proto.h"
#include	"sysexits.h"
#if	SUN3 == 1
#include	"sun3.h"
#endif	/* SUN3 == 1 */


/*
**	Parameters set from arguments.
*/

char *	HomeAddress;			/* Typed address of this node */
char *	LinkAddress;			/* Message arrived from this node */
char *	Name		= sccsid;	/* Program invoked name */
int	Parent;				/* Router pid if invoked by sub-router */
char *	SavHdrEnv	= EmptyString;	/* Message header fields... */
Ulong	SavHdrFlags;			/*  */
char *	SavHdrID;			/*  */
char *	SavHdrPart;			/*  */
char *	SavHdrRoute;			/*  */
char *	SavHdrSource;			/*  */
Ulong	SavHdrTt;			/*  */
Ulong	SavHdrDt;			/*  */
int	Traceflag;			/* Global tracing control */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(b, &Broadcast, 0),
	Arg_long(D, &DataLength, 0, english("data-length"), 0),
	Arg_string(E, &SavHdrEnv, 0, english("hdr-env"), OPTARG),
	Arg_long(F, &SavHdrFlags, 0, english("hdr-flags"), OPTARG),
	Arg_string(H, &HomeAddress, 0, english("home"), 0),
	Arg_string(I, &SavHdrID, 0, english("ID"), 0),
	Arg_string(L, &LinkAddress, 0, english("link"), OPTARG),
	Arg_long(M, &SavHdrTt, 0, english("travel-time"), 0),
	Arg_long(N, &SavHdrDt, 0, english("time-to-die"), OPTARG),
	Arg_string(P, &SavHdrPart, 0, english("part"), OPTARG),
	Arg_string(R, &SavHdrRoute, 0, english("route"), 0),
	Arg_string(S, &SavHdrSource, 0, english("source"), 0),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_int(W, &Parent, 0, english("router-pid"), OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Miscellaneous.
*/

char *	AckMessage;		/* Message for ACK */
char *	CmdsFile;		/* Queued commands file */
CmdHead	Commands;		/* Describing FTP part of message when spooling */
CmdHead	Cleanup;		/* Files to be unlinked */
bool	CRCIn;			/* Data CRC present */
CmdHead	DataCmds;		/* Describing FTP part of message */
FthFD_p*FilesList;		/* Sorted vector of file descriptors */
long	FTPhdrEnd;		/* Seek address for FTP header in MesgHdr */
char *	FwdNode;		/* Node(s) at which message was forwarded */
CmdHead	HdrCmds;		/* Describing new header for message */
char *	HomeUnTypedName;	/* Untyped name of this node */
char *	MesgHdr;		/* Name of message file containing FtHeader */
Ulong	MesgLength;		/* Length of whole message */
bool	NoOpt;			/* Unoptimised message transmission */
CmdHead	PartCmds;		/* Describing all parts received so far */
int	RetVal		= EX_OK;
char *	SourceAddress;		/* Type stripped source address */
char *	SpoolName;		/* Template for spooling commands in FILESDIR */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	WorkName;		/* Template for spooling files in WORKDIR */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"PRINTER",		&PRINTER,			PSTRING},
	{"PRINTERARGS",		(char **)&PRINTERARGS,		PVECTOR},
	{"PRINTORIGINS",	&PRINTORIGINS,			PSTRING},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};


#define	Fprintf		(void)fprintf

int	byname _FA_((const void *, const void *));
bool	checkusers _FA_((void));
void	cleanup _FA_((void));
void	forward _FA_((char *));
char *	fwduser _FA_((char *, char *, char *));
bool	getcommand _FA_((CmdT, CmdV));
void	printfiles _FA_((void));
void	recvack _FA_((void));
void	returnmsg _FA_((FILE *, char *));
void	sendack _FA_((char *));
void	sortfiles _FA_((void));



int
main(argc, argv)
	register int	argc;
	register char *	argv[];
{
	register int	fd;
	Forward *	fp;
	FthReason	reason;
	static char	ftherror[]	= english("File transfer protocol header \"%s\" error");

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

	HdrID = SavHdrID;
	HdrEnv = SavHdrEnv;
	HdrFlags = SavHdrFlags;
	HdrPart = SavHdrPart;
	HdrRoute = SavHdrRoute;
	HdrSource = SavHdrSource;
	HdrTt = SavHdrTt;
	HdrTtd = SavHdrDt;

	DupNode = GetEnv(ENV_DUP);
	EnvError = GetEnv(ENV_ERR);
	FwdNode = GetEnv(ENV_FWD);

	NoOpt = (bool)((SavHdrFlags & HDR_NOOPT) != 0);
	if ( !(Returned = (bool)((SavHdrFlags & HDR_RETURNED) != 0)) )
	{
		Ack = (bool)((SavHdrFlags & HDR_ACK) != 0);
		ReqAck = (bool)((SavHdrFlags & HDR_RQA) != 0);
	}

	StartTime = Time - SavHdrTt;

	HomeUnTypedName = StripTypes(HomeAddress);
	SourceAddress = StripTypes(SavHdrSource);
#	if	SUN3 == 1
	if ( Sun3 )
	{
		ExHomeAddress = StripDomain(newstr(HomeUnTypedName), OzAu, Au, false);
		ExLinkAddress = StripDomain(newstr(LinkAddress), OzAu, Au, false);
		ExSourceAddress = StripDomain(newstr(SourceAddress), OzAu, Au, false);
	}
	else
#	endif	/* SUN3 == 1 */
	{
		ExHomeAddress = HomeUnTypedName;
		ExLinkAddress = StripTypes(LinkAddress);
		ExSourceAddress = SourceAddress;
	}

	InitCmds(&Cleanup);
	InitCmds(&Commands);
	InitCmds(&DataCmds);
	InitCmds(&HdrCmds);
	InitCmds(&PartCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, english("read commands from"), "stdin");

	while ( (fd = open(MesgHdr, O_READ)) == SYSERROR )
		Syserror(CouldNot, OpenStr, MesgHdr);

	if
	(
		(reason = ReadFtHeader(fd, FTPhdrEnd)) != fth_ok
		||
		(reason = GetFthFiles()) != fth_ok
	)
		Error(ftherror, FTHREASON(reason));

	(void)close(fd);

	FtDataLength = DataLength - FthLength;	/* In case multi-file message */

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	CmdsFile = concat(SPOOLDIR, ROUTINGDIR, Template, NULLSTR);
	SpoolName = concat(SPOOLDIR, FILESDIR, Template, NULLSTR);
	WorkName = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	if ( Ack || Returned )
	{
		sortfiles();
		recvack();
	}
	else
	if ( (fp = Forwarded(Name, Name, SavHdrSource)) != (Forward *)0 )
		forward(fp->address);
	else
	if ( checkusers() )
	{
		if ( PRINTER == NULLSTR )
			sendack(concat(HomeUnTypedName, " has no printer", NULLSTR));
		else
		if ( InList(CanAddrMatch, SourceAddress, PRINTORIGINS, NULLSTR) )
		{
			if ( FthType[0] & FTH_DATACRC )
				CRCIn = true;

			if
			(
				CRCIn
				&&
				!CheckData(&DataCmds, (Ulong)0, FtDataLength, &DataCrc)
			)
			{
				Recover(ert_return);
				RetVal = EX_DATAERR;
				Error(ftherror, FTHREASON(fth_baddatacrc));
				Recover(ert_finish);
			}
			else
			if
			(
				SavHdrPart == NULLSTR
				||
				AllParts(&DataCmds, (Ulong)0, FtDataLength, MesgLength, &PartCmds, WorkName, Name)
			)
			{
				if ( SavHdrPart != NULLSTR )
				{
					FreeCmds(&DataCmds);
					FtDataLength = 0;
					DataSize = PartsLength;
				}
				else
					DataSize = DataLength;

				printfiles();
			}
		}
		else
			sendack(concat(ExSourceAddress, " is not a legal origin for print jobs", NULLSTR));
	}

	cleanup();

	exit(RetVal);
}



/*
**	Alphabetically
*/

int
#ifdef	ANSI_C
byname(const void *fpp1, const void *fpp2)
#else	/* ANSI_C */
byname(fpp1, fpp2)
	char *	fpp1;
	char *	fpp2;
#endif	/* ANSI_C */
{
	return strcmp((*(FthFD_p *)fpp1)->f_name, (*(FthFD_p *)fpp2)->f_name);
}



/*
**	Extract users from file transfer header,
**	and forward message if indicated.
**
**	Return true for non-forwarded users at this host.
*/

bool
checkusers()
{
	register FthUlist *	up;
	register Forward *	fp;
	register bool		local;

	if ( Returned )
	{
		FthUsers = Talloc0(FthUlist);
		FthUsers->u_name = FthFrom;
		NFthUsers = 1;
	}
	else
	if ( GetFthTo(true) == 0 )
	{
		Error(english("addressing error - no users at \"%s\" in list \"%s\""), HomeUnTypedName, UQFthTo);
		return false;	/* No users at this node */
	}

	local = false;

	Recover(ert_return);

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
	{
		if ( !Returned && (fp = Forwarded(Name, up->u_name, SavHdrSource)) != (Forward *)0 )
		{
			if ( (up->u_dir = fwduser(fp->address, fp->sub_addr, up->u_name)) == NULLSTR )
			{
				up->u_name = NULLSTR;
				NFthUsers--;
				continue;
			}

			up->u_dest = HomeUnTypedName;
			local = true;
			continue;
		}

		up->u_dir = NULLSTR;
		up->u_dest = HomeUnTypedName;
		local = true;
	}

	Recover(ert_finish);

	return local;
}



/*
**	Do unlink commands.
*/

void
cleanup()
{
	register Cmd *	cmdp;

	for ( cmdp = Cleanup.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
			(void)unlink(cmdp->cd_value);
}



/*
**	Called from the errors routines to cleanup
*/

void
finish(error)
	int	error;
{
	if ( !ExInChild && RetVal != EX_OK )
		error = RetVal;

	if ( AckMessage != NULLSTR )
		(void)unlink(AckMessage);

	(void)exit(error);
}



/*
**	Forward message to newaddress.
**
**	(``router'' will check for routing loops if include original HdrRoute.)
*/

void
forward(newaddress)
	char *		newaddress;
{
	register CmdV	cmdv;
	register int	fd;

	FreeCmds(&Commands);

	LinkCmds(&DataCmds, &Commands, (Ulong)0, DataLength, WorkName, Time);

	fd = create(UniqueName(WorkName, SavHdrID[0], NoOpt, DataLength, Time));

	HdrDest = newaddress;
	HdrEnv = concat(SavHdrEnv, MakeEnv(ENV_FWD, ExHomeAddress, NULLSTR), NULLSTR);
	HdrFlags = SavHdrFlags;
	HdrHandler = Name;

	if ( (HdrID = SavHdrID) == NULLSTR || HdrID[0] == '\0' )
		HdrID = WorkName + strlen(SPOOLDIR) + strlen(WORKDIR);

	HdrPart = SavHdrPart;
	HdrQuality = HdrID[0];
	HdrRoute = SavHdrRoute;
	HdrSource = SavHdrSource;
	HdrSubpt = UNK_PROTO;	/* Don't really know what this was */
	HdrTt = SavHdrTt;
	HdrTtd = SavHdrDt;
	HdrType = HDR_TYPE2;

	while ( WriteHeader(fd, (long)0, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, WorkName);

	(void)close(fd);

	free(HdrEnv);
	HdrEnv = SavHdrEnv;

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = WorkName, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = HdrLength, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = WorkName, cmdv));

	fd = create(UniqueName(WorkName, HdrQuality, NoOpt, (Ulong)HdrLength, Time));

	(void)WriteCmds(&Commands, fd, WorkName);

	(void)close(fd);

	/*
	**	Queue commands for router.
	*/

#	if	PRIORITY_ROUTING == 1
	move(WorkName, UniqueName(CmdsFile, HdrQuality, NoOpt, (Ulong)HdrLength, StartTime));
#	else	/* PRIORITY_ROUTING == 1 */
	move(WorkName, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, StartTime));
#	endif	/* PRIORITY_ROUTING == 1 */

	if ( Parent )
		(void)kill(Parent, SIGWAKEUP);	/* Wakeup needed if we are invoked by sub-router */
}



/*
**	Forward files to newaddress for user.
**
**	(``router'' will check for routing loops if include original HdrRoute.)
*/

char *
fwduser(newaddress, newto, user)
	char *		newaddress;
	char *		newto;
	char *		user;
{
	register char *	cp;
	register CmdV	cmdv;
	register long	size;
	register int	fd;

	if ( newto == NULLSTR && newaddress[0] == '/' )
		return newaddress;	/* Spool dir for files */

	FreeCmds(&Commands);

	LinkCmds(&DataCmds, &Commands, (Ulong)0, FtDataLength, WorkName, StartTime);

	fd = create(UniqueName(WorkName, SavHdrID[0], OPTNAME, (Ulong)FtDataLength, StartTime));

	SetFthFiles();
	FthTo = newto;

	FthType[0] = FTH_TYPE;
	if ( CRCIn )
		FthType[0] |= FTH_DATACRC;

	while ( (size = WriteFtHeader(fd, (long)0)) == SYSERROR )
		Syserror(CouldNot, WriteStr, WorkName);

	HdrDest = newaddress;

	cp = concat(user, "@", HomeUnTypedName, NULLSTR);
	HdrEnv = concat(SavHdrEnv, MakeEnv(ENV_FWD, cp, NULLSTR), NULLSTR);

	HdrFlags = SavHdrFlags;
	HdrHandler = Name;

	if ( (HdrID = SavHdrID) == NULLSTR || HdrID[0] == '\0' )
		HdrID = WorkName + strlen(SPOOLDIR) + strlen(WORKDIR);

	HdrPart = SavHdrPart;
	HdrQuality = HdrID[0];
	HdrRoute = SavHdrRoute;
	HdrSource = SavHdrSource;
	HdrSubpt = FTP;
	HdrTt = SavHdrTt;
	HdrTtd = SavHdrDt;
	HdrType = HDR_TYPE2;

	while ( WriteHeader(fd, size, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, WorkName);
	
	(void)close(fd);

	free(HdrEnv);
	HdrEnv = SavHdrEnv;
	free(cp);

	size += HdrLength;

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = WorkName, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = size, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = WorkName, cmdv));

	fd = create(UniqueName(WorkName, HdrQuality, OPTNAME, (Ulong)FtDataLength+size, StartTime));

	(void)WriteCmds(&Commands, fd, WorkName);
	
	(void)close(fd);

	/*
	**	Queue commands for router.
	*/

#	if	PRIORITY_ROUTING == 1
	move(WorkName, UniqueName(CmdsFile, HdrQuality, OPTNAME, (Ulong)FtDataLength+size, StartTime));
#	else	/* PRIORITY_ROUTING == 1 */
	move(WorkName, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, StartTime));
#	endif	/* PRIORITY_ROUTING == 1 */

	if ( Parent )
		(void)kill(Parent, SIGWAKEUP);	/* Wakeup needed if we are invoked by sub-router */

	return NULLSTR;
}



/*
**	Process a command from commands file.
*/

bool
getcommand(type, val)
	CmdT		type;
	CmdV		val;
{
	static char *	filename;
	static Ulong	filestart;

	switch ( type )
	{
	case end_cmd:
		if ( (MesgLength += val.cv_number - filestart) >= DataLength )
		{
			if ( MesgHdr != NULLSTR )
				break;

			MesgHdr = filename;
			FTPhdrEnd = val.cv_number - (MesgLength - DataLength);
			break;
		}
		break;

	case file_cmd:
		filename = newstr(val.cv_name);
		break;

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
**	Check and acknowledge the data if requested,
**	and pass the print jobs to the local print spooler with any optional flags.
*/

void
printfiles()
{
	register char *	cp;
	register FILE *	fd;
	char *		errs;
	ExBuf		exbuf;

	Recover(ert_return);

	SenderName = FthFrom;
	UserName = UQFthTo;
	DataName = FthFiles->f_name;

	FIRSTARG(&exbuf.ex_cmd) = PRINTER;

	ExpandArgs(&exbuf.ex_cmd, PRINTERARGS);

	if ( (cp = GetEnv(ENV_FLAGS)) != NULLSTR )
	{
		(void)SplitArg(&exbuf.ex_cmd, cp);
		free(cp);
	}

	fd = (FILE *)Execute(&exbuf, NULLVFUNCP, ex_pipe, SYSERROR);

	(void)CollectData(&PartCmds, (Ulong)0, PartsLength, fileno(fd), "pipe");
	(void)CollectData(&DataCmds, (Ulong)0, FtDataLength, fileno(fd), "pipe");

	if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
	{
		sendack(errs);		/* Send NAK */
		free(errs);
	}
	else
	if ( ReqAck )
		sendack(NULLSTR);	/* Send ACK */

	Recover(ert_finish);
}



/*
**	Receive an acknowledgement generated by 'sendack()'.
**
**	Notify sender of acknowledgement.
*/

void
recvack()
{
	register int		i;
	register FILE *		fd;
	register FthFD_p *	fpp;
	char *			errs;
	ExBuf			exbuf;
	char			date[DATESTRLEN];

	(void)GetFthTo(false);	/* Build UQFthTo */

	SenderName = POSTMASTERNAME;		/* Ack from "Postmaster@ExHomeAddress" */
	errs = ExSourceAddress;
	ExSourceAddress = ExHomeAddress;	/* For BINMAILARGS */

	FIRSTARG(&exbuf.ex_cmd) = BINMAIL;
	ExpandArgs(&exbuf.ex_cmd, BINMAILARGS);

	ExSourceAddress = errs;

	NEXTARG(&exbuf.ex_cmd) = FthFrom;	/* Ack to original sender */

	fd = (FILE *)Execute(&exbuf, NULLVFUNCP, ex_pipe, SYSERROR);
	
	if ( MAIL_RFC822_HDR )
	{
		Fprintf(fd, "From: %s@%s\n", POSTMASTERNAME, ExHomeAddress);
		Fprintf(fd, "Date: %s", rfc822time(&StartTime));
	}

	if ( MAIL_TO )
		Fprintf(fd, "To: %s@%s\n", FthFrom, HomeUnTypedName);

	if ( (errs = EnvError) != NULLSTR || Returned )
		returnmsg(fd, errs);
	else
	{
		Fprintf
		(
			fd,
			english("Subject: Acknowledgement received for %s printed at %s\n\n"),
			NFthFiles>1?english("jobs"):FthFiles->f_name,
			ExSourceAddress
		);

		Fprintf
		(
			fd,
			"The following print %s spooled on %.12s at %s:\n      Size   Spool time    Name\n",
			NFthFiles>1?english("jobs were"):english("job was"),
			DateString(StartTime, date),
			ExSourceAddress
		);

		for ( fpp = FilesList, i = NFthFiles ; --i >= 0 ; fpp++ )
		{
			Fprintf
			(
				fd,
				"%10lu %.15s %s\n",
				(PUlong)(*fpp)->f_length,
				DateString((*fpp)->f_time, date),
				(*fpp)->f_name
			);
		}
	}

	if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
	{
		Error(StringFmt, errs);
		free(errs);
	}
}



/*
**	Make up message for returned files.
*/

void
returnmsg(fd, err_mesg)
	FILE *	fd;
	char *	err_mesg;
{
	Fprintf
	(
		fd, 
		english("Subject: Print spooler failed at %s\n\n"),
		ExSourceAddress
	);

	Fprintf
	(
		fd,
		english("PRINT JOB \"%s\" NOT SPOOLED AT \"%s\"\n\n"),
		FthFiles->f_name,
		ExSourceAddress
	);

	if ( err_mesg != NULLSTR )
		Fprintf(fd, english("Failure explanation follows :-\n%s\n"), CleanError(err_mesg));
}



/*
**	Send an acknowledgement message to source.
*/

void
sendack(err_mesg)
	char *			err_mesg;
{
	register FthFD_p	fp;
	register long		size;
	register CmdV		cmdv;
	int			fd;

	/*
	**	Make file for headers.
	*/

	AckMessage = newstr(WorkName);
	fd = create(UniqueName(AckMessage, CHOOSE_QUALITY, OPTNAME, (Ulong)MAX_MESG_DATA, Time));

	/*
	**	Make FTP header.
	*/

	for ( fp = FthFiles ; fp != (FthFD_p)0 ; fp = fp->f_next )
		fp->f_time = Time;

	SetFthFiles();
	SetFthTo();	/* Only users at this node */

	FthType[0] = FTH_TYPE;

	while ( (size = WriteFtHeader(fd, (long)0)) == SYSERROR )
		Syserror(CouldNot, WriteStr, AckMessage);

	/*
	**	Make message header.
	*/

	HdrDest = SavHdrSource;

	HdrEnv = MakeEnv(ENV_ID, HdrID, NULLSTR);	/* Old ID saved */

	if ( err_mesg != NULLSTR )
		HdrEnv = concat(HdrEnv, MakeEnv(ENV_ERR, StripErrString(err_mesg), NULLSTR), NULLSTR);

	HdrFlags = HDR_ACK|HDR_NORET;

	if ( NoOpt )
		HdrFlags |= HDR_NOOPT;

	if ( SavHdrFlags & HDR_REGISTERED )
		HdrFlags |= HDR_REGISTERED;

	HdrHandler = Name;
	HdrID = AckMessage + strlen(SPOOLDIR) + strlen(WORKDIR);
	HdrPart = NULLSTR;
	HdrQuality = HdrID[0];
	HdrRoute = NULLSTR;
	HdrSource = HomeAddress;
	HdrSubpt = FTP;
	HdrTt = 0;
	HdrTtd = 0;
	HdrType = HDR_TYPE2;

	while ( WriteHeader(fd, size, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, AckMessage);
	
	(void)close(fd);

	if ( HdrEnv != NULLSTR )
		free(HdrEnv);
	HdrEnv = SavHdrEnv;

	size += HdrLength;

	/*
	**	Write commands.
	*/

	FreeCmds(&Commands);

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = AckMessage, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = size, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = AckMessage, cmdv));

	fd = create(UniqueName(WorkName, HdrQuality, OPTNAME, (Ulong)size, Time));
	(void)WriteCmds(&Commands, fd, WorkName);
	(void)close(fd);

	/*
	**	Queue commands for router.
	*/

#	if	PRIORITY_ROUTING == 1
	move(WorkName, UniqueName(CmdsFile, HdrQuality, OPTNAME, (Ulong)size, Time));
#	else	/* PRIORITY_ROUTING == 1 */
	move(WorkName, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time));
#	endif	/* PRIORITY_ROUTING == 1 */

	if ( Parent )
		(void)kill(Parent, SIGWAKEUP);	/* Wakeup needed if we are invoked by sub-router */
}



/*
**	Sort files in message.
*/

void
sortfiles()
{
	register FthFD_p	fp;
	register FthFD_p *	fpp;

	FilesList = fpp = (FthFD_p*)Malloc(sizeof(FthFD_p)*NFthFiles);

	for ( fp = FthFiles ; fp != (FthFD_p)0 ; fp = fp->f_next )
		*fpp++ = fp;
	
	if ( NFthFiles > 1 )
		qsort((char *)FilesList, NFthFiles, sizeof(FthFD_p), byname);
}
