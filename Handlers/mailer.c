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


static char	sccsid[]	= "@(#)mailer.c	1.28 05/12/16";

/*
**	Process mail from message received from network.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	RECOVER
#define	SIGNALS
#define	STDIO
#define	TIME

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

#include	<ndir.h>

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
**	Miscellaneous
*/

char *	AckMessage;		/* Message for ACK */
unsigned Alarm;			/* Original ALARM setting - passed to children */
char *	CmdsFile;		/* Queued commands file */
CmdHead	Commands;		/* Describing FTP part of message when spooling */
CmdHead	Cleanup;		/* Files to be unlinked */
bool	CRCIn;			/* Data CRC present */
CmdHead	DataCmds;		/* Describing FTP part of message */
long	FTPhdrEnd;		/* Seek address for FTP header in MesgHdr */
char *	FwdNode;		/* Node(s) at which message was forwarded */
char *	HomeUnTypedName;	/* Untyped name of this node */
int	IgnMailerStatus	= IGNMAILERSTATUS;
char *	LastRecvd;		/* Site last subject of Received: line */
int	MailFrom	= MAIL_FROM;
char *	MesgHdr;		/* Name of message file containing FtHeader */
Ulong	MesgLength;		/* Length of whole message */
bool	NoOpt;			/* Unoptimised message transmission */
CmdHead	PartCmds;		/* Describing all parts received so far */
int	RetVal		= EX_OK;
int	ShowRoute	= SHOW_ROUTE;
char *	SourceAddress;		/* Type stripped source address */
Ulong	TravelTime;
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
int	ValidateMail	= VALIDATEMAIL;
char *	WorkName;		/* Template for spooling files in WORKDIR */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"IGNMAILERSTATUS",	(char **)&IgnMailerStatus,	PINT},
	{"MAILER",		&MAILER,			PSTRING},
	{"MAILERARGS",		(char **)&MAILERARGS,		PVECTOR},
	{"MAIL_FROM",		(char **)&MailFrom,		PINT},
	{"SHOW_ROUTE",		(char **)&ShowRoute,		PINT},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
	{"VALIDATEMAIL",	(char **)&ValidateMail,		PINT},
};


#define	Fprintf		(void)fprintf

bool	checkusers _FA_((void));
void	child_alarm _FA_((VarArgs *));
void	cleanup _FA_((void));
void	forward _FA_((char *, char *, char *));
bool	getcommand _FA_((CmdT, CmdV));
void	mailem _FA_((void));
bool	printroute _FA_((char *, Ulong, char *, bool, FILE *));
void	recvack _FA_((void));
void	returnmsg _FA_((FILE *));
void	sendack _FA_((void));



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register int	fd;
	FthReason	reason;
	static char	ftherror[]	= english("Mail transfer protocol header \"%s\" error");

	if ( (Alarm = alarm(0)) )
		(void)alarm(Alarm+5);

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
	TruncNode = GetEnv(ENV_TRUNC);

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
	InitCmds(&PartCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, english("read commands from"), "stdin");

	while ( (fd = open(MesgHdr, O_READ)) == SYSERROR )
		Syserror(CouldNot, OpenStr, MesgHdr);

	if ( (reason = ReadFtHeader(fd, FTPhdrEnd)) != fth_ok )
		Error(ftherror, FTHREASON(reason));

	(void)GetFthFiles();			/* Don't care if not wok */

	(void)close(fd);

	FtDataLength = DataLength - FthLength;	/* In case multi-file message */

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	CmdsFile = concat(SPOOLDIR, ROUTINGDIR, Template, NULLSTR);
	WorkName = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	if ( Ack )
		recvack();
	else
	if ( checkusers() )
	{
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
			RetVal = EX_DATAERR;
			Error(ftherror, FTHREASON(fth_baddatacrc));
			Recover(ert_finish);
		}
		else
		if
		(
			Returned
			||
			SavHdrPart == NULLSTR
			||
			AllParts(&DataCmds, (Ulong)0, FtDataLength, MesgLength, &PartCmds, WorkName, Name)
		)
		{
			if ( !Returned && SavHdrPart != NULLSTR )
			{
				FreeCmds(&DataCmds);
				FtDataLength = 0;
				DataSize = PartsLength;
			}
			else
				DataSize = DataLength;

			mailem();
		}
	}

	cleanup();

	exit(RetVal);
}



/*
**	Extract users from file transfer header,
**	identify any users at this host,
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
	Passwd			to;

	if ( Returned )
	{
		(void)GetFthTo(false);	/* Build UQFthTo */
		FthUsers = Talloc0(FthUlist);
		FthUsers->u_name = FthFrom;
		NFthUsers = 1;
	}
	else
	if ( GetFthTo(true) == 0 )
	{
		RetVal = EX_NOUSER;
		Error(english("addressing error - no users at \"%s\" in list \"%s\""), HomeUnTypedName, UQFthTo);
		return false;	/* No users at this node */
	}

	local = false;

	Recover(ert_return);

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
	{
		if ( !Returned && (fp = Forwarded(Name, up->u_name, SavHdrSource)) != (Forward *)0 )
		{
			forward(fp->address, fp->sub_addr, up->u_name);
			up->u_name = NULLSTR;
			NFthUsers--;
			continue;
		}

		if ( strccmp(up->u_name, "postmaster") == STREQUAL )
		{
			up->u_name = POSTMASTER;
			ShowRoute = 1;
		}

		if ( ValidateMail && !GetUid(&to, up->u_name) )
		{
			if ( !Broadcast )
			{
				RetVal = EX_NOUSER;	/* Return message and reason to source */
				Error(english("User \"%s\" does not exist at %s"), up->u_name, HomeUnTypedName);
			}

			up->u_name = NULLSTR;
			NFthUsers--;
			continue;
		}

		up->u_dest = HomeUnTypedName;
		local = true;
	}

	Recover(ert_finish);

	return local;
}



/*
**	Set alarm timeout for children.
*/

void
child_alarm(vap)
	VarArgs *	vap;
{
	(void)alarm(Alarm);
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
**	Forward mail to newaddress for user.
**
**	(`router' will check for routing loops if include original HdrRoute.)
*/

void
forward(newaddress, newto, user)
	char *		newaddress;
	char *		newto;
	char *		user;
{
	register char *	cp;
	register CmdV	cmdv;
	register long	size;
	register int	fd;

	FreeCmds(&Commands);

	LinkCmds(&DataCmds, &Commands, (Ulong)0, FtDataLength, WorkName, StartTime);

	fd = create(UniqueName(WorkName, SavHdrID[0], NoOpt, FtDataLength, StartTime));

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

	fd = create(UniqueName(WorkName, HdrQuality, NoOpt, FtDataLength+size, StartTime));

	(void)WriteCmds(&Commands, fd, WorkName);
	
	(void)close(fd);

	/*
	**	Queue commands for router.
	*/

#	if	PRIORITY_ROUTING == 1
	move(WorkName, UniqueName(CmdsFile, HdrQuality, NoOpt, (Ulong)FtDataLength+size, StartTime));
#	else	/* PRIORITY_ROUTING == 1 */
	move(WorkName, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, StartTime));
#	endif	/* PRIORITY_ROUTING == 1 */

	if ( Parent )
		(void)kill(Parent, SIGWAKEUP);	/* Wakeup needed if we are invoked by sub-router */
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
**	Mail identified users,
**	and acknowledge the data if requested.
*/

void
mailem()
{
	register FthUlist *	up;
	register FILE *		fd;
	register char *		errs = NULLSTR;
	register int		nusers;
	register int		i;
	register bool		piperr;
	register bool		newline;
	ExBuf			exbuf;

	if ( Returned )
		SenderName = POSTMASTERNAME;
	else
		SenderName = FthFrom;

	Recover(ert_return);

	for ( up = FthUsers, nusers = NFthUsers ; nusers > 0 ; )
	{
		if ( Returned )
		{
			errs = ExSourceAddress;
			ExSourceAddress = ExHomeAddress;	/* For MAILERARGS */
		}

		FIRSTARG(&exbuf.ex_cmd) = MAILER;
		ExpandArgs(&exbuf.ex_cmd, MAILERARGS);

		if ( Returned )
			ExSourceAddress = errs;

		for
		(
			;
			NARGS(&exbuf.ex_cmd) < MAXVARARGS && up != (FthUlist *)0
			;
			up = up->u_next
		)
		{
			if ( up->u_name == NULLSTR )
				continue;

			NEXTARG(&exbuf.ex_cmd) = up->u_name;
			nusers--;
		}

		fd = (FILE *)Execute(&exbuf, child_alarm, ex_pipe, SYSERROR);

		if ( ShowRoute )
		{
			TravelTime = 0;
			FreeStr(&LastRecvd);
			LastRecvd = newstr(ExHomeAddress);
			(void)ExRevRoute(SavHdrSource, SavHdrRoute, HomeAddress, printroute, fd);
		}

		if ( MailFrom )
			Fprintf
			(
				fd,
				"From %s@%s %s",
				SenderName,
				Returned?ExHomeAddress:ExSourceAddress,
				ctime(&StartTime)
			);

		if ( Returned )
			returnmsg(fd);

		(void)fflush(fd);

		piperr = (bool)ferror(fd);

		if ( !CollectData(&PartCmds, (Ulong)0, PartsLength, fileno(fd), "pipe") )
			piperr = true;
		else
		if ( !CollectData(&DataCmds, (Ulong)0, FtDataLength, fileno(fd), "pipe") )
			piperr = true;

		newline = false;

		if ( (errs = DupNode) != NULLSTR )
		{
			char *	cp = StripTypes(errs);

			if ( !newline )
			{
				Fprintf(fd, "\n\n");
				newline = true;
			}

			Fprintf
			(
				fd,
				english("(%s: this mail may have been duplicated at \"%s\""),
				POSTMASTERNAME,
				cp
			);

			free(cp);

			for ( i = 0 ; i < 4 && (cp = GetEnvNext(ENV_DUP, errs)) != NULLSTR ; i++ )
			{
				free(errs);
				errs = StripTypes(cp);
				Fprintf(fd, english(" and \"%s\""), errs);
				free(errs);
				errs = cp;
			}

			Fprintf(fd, english(" due to link problems.)\n"));

			free(errs);
			DupNode = GetEnv(ENV_DUP);
		}

		if ( (errs = FwdNode) != NULLSTR )
		{
			char *	cp;

			if ( !newline )
			{
				Fprintf(fd, "\n\n");
				newline = true;
			}

			Fprintf
			(
				fd,
				english("(%s: this mail was forwarded by \"%s\""),
				POSTMASTERNAME,
				errs
			);

			for ( i = 0 ; i < 4 && (cp = GetEnvNext(ENV_FWD, errs)) != NULLSTR ; i++ )
			{
				free(errs);
				Fprintf(fd, ", \"%s\"", cp);
				errs = cp;
			}

			Fprintf(fd, ".)\n");

			free(errs);
			FwdNode = GetEnv(ENV_FWD);
		}

		if ( ReqAck )
		{
			if ( !newline )
			{
				Fprintf(fd, "\n\n");
				newline = true;
			}

			Fprintf(fd, english("(%s: delivery acknowledged.)\n"), POSTMASTERNAME);
		}

		(void)fflush(fd);

		if ( ferror(fd) )
			piperr = true;

		if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
		{
			if
			(
				IgnMailerStatus == 0
				||
				(IgnMailerStatus == 2 && ExStatus != EX_NOUSER)
				||
				piperr
			)
			{
				if ( ExStatus & 0377 )
					RetVal = ExStatus;
				else
					RetVal = EX_SOFTWARE;
				Error(StringFmt, errs);
			}
			free(errs);
		}
	}

	if ( ReqAck && NFthUsers > 0 && RetVal == EX_OK )
		sendack();	/* send ack for each user here */

	Recover(ert_finish);
}



/*
**	Print out each node in route (presented newest first).
*/

bool
printroute(from, time, to, last, fd)
	char *		from;
	Ulong		time;
	char *		to;
	bool		last;
	FILE *		fd;
{
	time_t		rdate;
	static char	buf[200];

	from = StripTypes(from);

	if ( strcmp(from, LastRecvd) != STREQUAL )
	{
		free(LastRecvd);
		LastRecvd = newstr(from);

		if ( TravelTime )
			fputs(buf, fd);
	}

	if ( time == 0 )
		time = 1;
	TravelTime += time;
	rdate = Time - TravelTime;

	if ( last )
		Fprintf(fd, english("Received: from %s by MHSnet id %s; %s"), from, SavHdrID, rfc822time(&rdate));
	else
		(void)sprintf(buf, english("Received: by %s with MHSnet; %s"), from, rfc822time(&rdate));

	free(from);
	return true;
}



/*
**	Receive an acknowledgement generated by 'sendack()'.
**
**	Notify sender of acknowledgement.
*/

void
recvack()
{
	register FILE *		fd;
	register char *		errs;
	ExBuf			exbuf;

	(void)GetFthTo(false);			/* Build UQFthTo */

	SenderName = POSTMASTERNAME;		/* Ack from "Postmaster@ExHomeAddress" */

	errs = ExSourceAddress;
	ExSourceAddress = ExHomeAddress;	/* For BINMAILARGS */

	FIRSTARG(&exbuf.ex_cmd) = BINMAIL;
	ExpandArgs(&exbuf.ex_cmd, BINMAILARGS);

	ExSourceAddress = errs;

	NEXTARG(&exbuf.ex_cmd) = FthFrom;	/* Ack to original sender */

	fd = (FILE *)Execute(&exbuf, NULLVFUNCP, ex_pipe, SYSERROR);

	if ( MailFrom )
		Fprintf
		(
			fd,
			"From %s@%s %s",
			POSTMASTERNAME,
			ExHomeAddress,
			ctime(&StartTime)
		);

	if ( MAIL_RFC822_HDR )
	{
		Fprintf(fd, "From: %s@%s\n", POSTMASTERNAME, ExHomeAddress);
		Fprintf(fd, "Date: %s", rfc822time(&StartTime));
	}

	if ( MAIL_TO )
		Fprintf(fd, "To: %s@%s\n", FthFrom, ExHomeAddress);

	if ( (errs = EnvError) != NULLSTR )
	{
		Fprintf
		(
			fd,
			english("Subject: NEGATIVE ACKNOWLEDGEMENT for mail sent to %s\n\nYour mail was not delivered at %s for the following reason:-\n%s\n\n"),
			UQFthTo,
			ExSourceAddress,
			CleanError(errs)
		);
	}
	else
		Fprintf
		(
			fd,
			english("Subject: Acknowledgement for mail sent to %s\n\nThe mail was delivered at %s on %s"),
			UQFthTo,
			ExSourceAddress,
			ctime(&StartTime)
		);

	if ( (errs = GetEnv(ENV_ID)) != NULLSTR )
	{
		Fprintf(fd, english("\nOriginal message ID was \"%s\"\n"), errs);
		free(errs);
	}

	if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
	{
		Error(StringFmt, errs);
		free(errs);
	}
}



/*
**	Make up message for returned mail.
*/

void
returnmsg(fd)
	FILE *		fd;
{
	register char *	cp;
	register int	i;

	if ( MAIL_RFC822_HDR )
	{
		Fprintf(fd, "From: %s@%s\n", SenderName, ExHomeAddress);
		Fprintf(fd, "Date: %s", rfc822time(&StartTime));
	}

	if ( MAIL_TO )
		Fprintf(fd, "To: %s@%s\n", FthFrom, ExHomeAddress);

	Fprintf
	(
		fd, 
		english("Subject: Undelivered mail returned from %s\n\n"),
		ExSourceAddress
	);

	for ( i = 0 ; i < 79 ; i++ )
		putc('*', fd);

	if ( (cp = GetEnv(ENV_ID)) == NULLSTR )
		cp = newstr(HdrID);

	Fprintf
	(
		fd,
		english("\nMAIL SENT TO \"%s\"\nRETURNED FROM \"%s\"\nMESSAGE ID WAS \"%s\"\n"),
		UQFthTo,
		ExSourceAddress,
		cp
	);

	free(cp);

	if ( (cp = EnvError) != NULLSTR )
		Fprintf(fd, english("\nFailure explanation follows :-\n%s\n"), CleanError(cp));

	if ( (cp = TruncNode) != NULLSTR )
	{
		char *	st = StripTypes(cp);

		Fprintf(fd, english("\nMessage was truncated at \"%s\"\n"), st);
		free(st);
	}

	for ( i = 0 ; i < 79 ; i++ )
		putc('*', fd);

	Fprintf(fd, "\n\n");
}



/*
**	Send an acknowledgement message to source.
*/

void
sendack()
{
	register long		size;
	register CmdV		cmdv;
	int			fd;

	AckMessage = newstr(WorkName);
	fd = create(UniqueName(AckMessage, CHOOSE_QUALITY, false, DataSize, Time));

	SetFthFiles();
	SetFthTo();	/* Only users at this node */

	FthType[0] = FTH_TYPE;

	while ( (size = WriteFtHeader(fd, (long)0)) == SYSERROR )
		Syserror(CouldNot, WriteStr, AckMessage);

	HdrDest = SavHdrSource;
	HdrEnv = MakeEnv(ENV_ID, HdrID, NULLSTR);	/* Old ID saved */
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

	free(HdrEnv);
	HdrEnv = SavHdrEnv;

	size += HdrLength;

	FreeCmds(&Commands);

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = AckMessage, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = size, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = AckMessage, cmdv));

	fd = create(UniqueName(WorkName, HdrQuality, false, (Ulong)size, Time));

	(void)WriteCmds(&Commands, fd, WorkName);

	(void)close(fd);

	/*
	**	Queue commands for router.
	*/

#	if	PRIORITY_ROUTING == 1
	move(WorkName, UniqueName(CmdsFile, HdrQuality, NoOpt, (Ulong)size, Time));
#	else	/* PRIORITY_ROUTING == 1 */
	move(WorkName, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time));
#	endif	/* PRIORITY_ROUTING == 1 */

	if ( Parent )
		(void)kill(Parent, SIGWAKEUP);	/* Wakeup needed if we are invoked by sub-router */
}
