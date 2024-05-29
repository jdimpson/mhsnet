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


static char	sccsid[]	= "@(#)filer.c	1.33 05/12/16";

/*
**	Process files from message received from network.
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
char *	CmdsFile;		/* Queued commands file */
CmdHead	Commands;		/* Describing FTP part of message when spooling */
CmdHead	Cleanup;		/* Files to be unlinked */
bool	CRCIn;			/* Data CRC present */
CmdHead	DataCmds;		/* Describing FTP part of message */
int	FilesExpireDays	= FILESEXPIREDAYS;
FthFD_p*FilesList;		/* Sorted vector of file descriptors */
long	FTPhdrEnd;		/* Seek address for FTP header in MesgHdr */
char *	FwdNode;		/* Node(s) at which message was forwarded */
CmdHead	HdrCmds;		/* Describing new header for message */
char *	HomeUnTypedName;	/* Untyped name of this node */
char *	MesgHdr;		/* Name of message file containing FtHeader */
Ulong	MesgLength;		/* Length of whole message */
char *	NewHdr;			/* File for new message header */
Ulong	NewHdrLength;		/* Length of NewHdr */
bool	NakIn;			/* Don't mail recipients */
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
	{"FILESEXPIREDAYS",	(char **)&FilesExpireDays,	PINT},
	{"GETFILE",		&GetfileStr,			PSTRING},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

/*
**	File notification format.
*/

char	ffmt[]		= "  0%3o %10lu  %s  %s\n";
char	fhdr[]		= english("  Mode       Size   Modify time  Name\n");


#define	Fprintf		(void)fprintf
#undef	DIRECTORY_SPOOLING	/* This stuff isn't programmed yet */


int	byname _FA_((const void *, const void *));
bool	checkusers _FA_((void));
void	cleanup _FA_((void));
char *	forward _FA_((char *, char *, char *));
bool	getcommand _FA_((CmdT, CmdV));
void	mailem _FA_((void));
void	print_id _FA_((FILE *));
void	recvack _FA_((void));
void	returnmsg _FA_((FILE *));
void	sendack _FA_((char *));
void	sortfiles _FA_((void));
void	spoolfiles _FA_((void));



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register int	fd;
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
		NakIn = (bool)((SavHdrFlags & HDR_NAK) != 0);
	}

	StartTime = Time - SavHdrTt;

	HomeUnTypedName = StripTypes(HomeAddress);
	SourceAddress = StripTypes(SavHdrSource);
#	if	SUN3 == 1
	if ( Sun3 )
	{
		ExHomeAddress = StripDomain(newstr(HomeAddress), OzAu, Au, false);
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

	if ( Ack )
	{
		sortfiles();
		recvack();
	}
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
			Error(ftherror, FTHREASON(fth_baddatacrc));
			RetVal = EX_DATAERR;
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
			if ( Returned || SavHdrPart == NULLSTR )	/* Whole message to PartsCmds */
			{
				LinkCmds(&DataCmds, &PartCmds, (Ulong)0, DataLength, WorkName, StartTime);
				DataSize = FtDataLength;
				PartsLength = DataLength;
			}
			else						/* Add FTP header to PartsCmds */
			{
				LinkCmds(&DataCmds, &PartCmds, FtDataLength, DataLength, WorkName, StartTime);
				DataSize = PartsLength;
				PartsLength += FthLength;
			}

			spoolfiles();
			sortfiles();
			mailem();
		}
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
			if ( (up->u_dir = forward(fp->address, fp->sub_addr, up->u_name)) == NULLSTR )
			{
				up->u_name = NULLSTR;
				NFthUsers--;
				continue;
			}

#			ifdef	DIRECTORY_SPOOLING
			up->u_dest = HomeUnTypedName;
			local = true;
			continue;
#			else	/* DIRECTORY_SPOOLING */
			up->u_dir = NULLSTR;
#			endif	/* DIRECTORY_SPOOLING */
		}

		if ( !GetUid(&to, up->u_name) )
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

		up->u_uid = to.P_uid;
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
**	Forward files to newaddress for user.
**
**	(``router'' will check for routing loops if include original HdrRoute.)
*/

char *
forward(newaddress, newto, user)
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

	fd = create(UniqueName(WorkName, SavHdrID[0], NoOpt, (Ulong)FtDataLength, StartTime));

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

	fd = create(UniqueName(WorkName, HdrQuality, NoOpt, (Ulong)FtDataLength+size, StartTime));

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
**	Mail identified users,
**	and acknowledge the data if requested.
*/

void
mailem()
{
	register FthUlist *	up;
	register FthFD_p *	fpp;
	register FILE *		fd;
	register int		i;
	register char *		errs = NULLSTR;
	register int		nusers;
	char *			to_list;
	char *			x_list;
	char *			spooldir;
	ExBuf			exbuf;

	if ( Returned )
	{
		SenderName = POSTMASTERNAME;
		x_list = concat(FthFrom, "@", ExHomeAddress, NULLSTR);
	}
	else
	{
		SenderName = FthFrom;
		x_list = UQFthTo;
	}

	if ( MAIL_TO )
		to_list = Malloc(strlen(UQFthTo) + NFthUsers*(strlen(HomeUnTypedName)+2));

	Recover(ert_return);

	if ( !NakIn )
	for ( up = FthUsers, nusers = NFthUsers ; nusers > 0 ; )
	{
		if ( Returned )
		{
			errs = ExSourceAddress;
			ExSourceAddress = ExHomeAddress;	/* For BINMAILARGS */
		}

		FIRSTARG(&exbuf.ex_cmd) = BINMAIL;
		ExpandArgs(&exbuf.ex_cmd, BINMAILARGS);

		if ( Returned )
			ExSourceAddress = errs;

		for
		(
			i = 0, spooldir = NULLSTR, errs = to_list ;
			NARGS(&exbuf.ex_cmd) < MAXVARARGS && up != (FthUlist *)0 ;
			up = up->u_next
		)
		{
			if ( up->u_name == NULLSTR )
				continue;

#			ifdef	DIRECTORY_SPOOLING
			if ( up->u_dir != NULLSTR && i )
				break;
#			endif	/* DIRECTORY_SPOOLING */

			if ( MAIL_TO )
			{
				if ( errs != to_list )
				{
					*errs++ = ',';
					*errs++ = ' ';
				}
				errs = strcpyend(errs, up->u_name);
				*errs++ = '@';
				errs = strcpyend(errs, ExHomeAddress);
			}

			NEXTARG(&exbuf.ex_cmd) = up->u_name;
			nusers--;
			i++;

#			ifdef	DIRECTORY_SPOOLING
			if ( (spooldir = up->u_dir) != NULLSTR )
				break;
#			endif	/* DIRECTORY_SPOOLING */
		}

		if ( i == 0 )
			break;

		fd = (FILE *)Execute(&exbuf, NULLVFUNCP, ex_pipe, SYSERROR);

		if ( MAIL_RFC822_HDR )
		{
			Fprintf(fd, "From: %s@%s\n", SenderName, Returned?ExHomeAddress:ExSourceAddress);
			Fprintf(fd, "Date: %s", rfc822time(&StartTime));
		}

		if ( MAIL_TO )
		{
			Fprintf(fd, "To: %s\n", to_list);
			if ( strcmp(to_list, x_list) != STREQUAL )
				Fprintf(fd, "X-To: %s\n", x_list);
		}
		else
			Fprintf(fd, "X-To: %s\n", x_list);

		if ( Returned )
			Fprintf
			(
				fd, 
				english("Subject: Undelivered file%s returned from %s\n"),
				(NFthFiles>1)?"s":"",
				ExSourceAddress
			);
		else
		if ( NFthFiles > 1 )
			Fprintf
			(
				fd,
				english("Subject: Files from %s@%s\n"),
				FthFrom,
				ExSourceAddress
			);
		else
			Fprintf
			(
				fd,
				english("Subject: \"%s\" from %s@%s\n"),
				FthFiles->f_name,
				FthFrom,
				ExSourceAddress
			);

		Fprintf(fd, "MIME-Version: 1.0\n");
		Fprintf(fd, "Content-Type: Multipart/Mixed; Boundary=\"NextPart\"\n\n--NextPart\n\n");

		if ( Returned )
			returnmsg(fd);

		if ( (errs = FwdNode) != NULLSTR )
		{
			char *	cp;

			Fprintf
			(
				fd,
				english("(%s:- %s forwarded by \"%s\""),
				POSTMASTERNAME,
				(NFthFiles>1)?english("These files were"):english("This file was"),
				errs
			);

			for ( i = 0 ; i < 4 && (cp = GetEnvNext(ENV_FWD, errs)) != NULLSTR ; i++ )
			{
				free(errs);
				Fprintf(fd, ", \"%s\"", cp);
				errs = cp;
			}

			Fprintf(fd, ".)\n\n");

			free(errs);
			FwdNode = GetEnv(ENV_FWD);
		}

		Fprintf
		(
			fd,
			english("The following %s been %s to you at %s:\n%s"),
			(NFthFiles>1)?english("files have"):english("file has"),
			Returned?english("returned"):english("sent"),
			HomeUnTypedName,
			fhdr
		);

		for ( fpp = FilesList, i = NFthFiles ; --i >= 0 ; fpp++ )
		{
			char	date[DATESTRLEN];

			Fprintf
			(
				fd,
				ffmt,
				(*fpp)->f_mode & FTH_MODES,
				(PUlong)(*fpp)->f_length,
				DateString((*fpp)->f_time, date),
				(*fpp)->f_name
			);
		}

#		ifdef	DIRECTORY_SPOOLING
		if ( spooldir != NULLSTR )
			Fprintf
			(
				fd,
				english("\n%s been spooled in the directory \"%s\".\n\n"),
				(NFthFiles>1)?english("These files have"):english("This file has"),
				spooldir
			);
		else
#		endif	/* DIRECTORY_SPOOLING */
			Fprintf
			(
				fd,
				english("\nPlease use \"%s\" to retrieve %s,\n(as otherwise %s will be deleted after %d days.)\n\n"),
				GETFILE,
				(NFthFiles>1)?english("these files"):english("this file"),
				(NFthFiles>1)?english("they"):english("it"),
				FilesExpireDays
			);

		if ( ReqAck )
			Fprintf(fd, english("(%s:- Delivery acknowledged.)\n\n"), POSTMASTERNAME);

		if ( (errs = DupNode) != NULLSTR )
		{
			char *	cp = StripTypes(errs);

			Fprintf
			(
				fd,
				english("(%s:- %s may have been duplicated at \"%s\""),
				POSTMASTERNAME,
				(NFthFiles>1)?english("These files"):english("This file"),
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

			Fprintf(fd, english(" due to link problems.)\n\n"));

			free(errs);
			DupNode = GetEnv(ENV_DUP);
		}

		print_id(fd);

		Fprintf(fd, english("\
Below is the data which will enable a MIME compliant mail reader to retrieve\n\
each delivered file by starting \"%s\" with appropriate parameters.\n\
\n\
DO NOT save the attachments themselves, as they will contain no data!\n"), GETFILE);

		for ( fpp = FilesList, i = NFthFiles ; --i >= 0 ; fpp++ )
			Fprintf(fd, "\n--NextPart\nContent-Type: application/x-netget; host=\"%s\"; name=\"%s\"\n", HomeUnTypedName, (*fpp)->f_name);

		Fprintf(fd, "\n--NextPart--\n");

		if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
		{
			if ( !(SavHdrFlags & HDR_NORET) )
				sendack(errs);
			free(errs);
			break;
		}
	}

	if ( ReqAck && NFthUsers > 0 )
		sendack(NULLSTR);	/* send ack for each user here */

	Recover(ert_finish);

	free(to_list);
}



/*
**	Make up new header file.
*/

void
newheader()
{
	register CmdV	cmdv;
	register int	fd;

	HdrDest = HomeAddress;
	HdrEnv = SavHdrEnv;
	HdrFlags = SavHdrFlags;
	HdrHandler = Name;
	HdrID = SavHdrID;
	HdrPart = SavHdrPart;
	HdrQuality = SavHdrID[0];
	HdrRoute = SavHdrRoute;
	HdrSource = SavHdrSource;
	HdrSubpt = FTP;
	HdrTt = SavHdrTt;
	HdrTtd = SavHdrDt;
	HdrType = HDR_TYPE2;

	fd = create(NewHdr = newstr(UniqueName(WorkName, HdrQuality, NoOpt, MesgLength, StartTime)));

	while ( WriteHeader(fd, (long)0, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, NewHdr);
	
	(void)close(fd);

	NewHdrLength = HdrLength;

	(void)AddCmd(&HdrCmds, file_cmd, (cmdv.cv_name = NewHdr, cmdv));
	(void)AddCmd(&HdrCmds, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&HdrCmds, end_cmd, (cmdv.cv_number = HdrLength, cmdv));
	(void)AddCmd(&HdrCmds, unlink_cmd, (cmdv.cv_name = NewHdr, cmdv));
}



/*
**	Print out original message ID if present.
*/

void
print_id(fd)
	FILE *		fd;
{
	register char *	cp;
	register char *	np;

	if ( (cp = GetEnv(ENV_ID)) == NULLSTR )
		return;

	if ( (np = strchr(cp, '/')) != NULLSTR )
		*np = '\0';	/* Remove Part No. */

	Fprintf(fd, english("\nOriginal message ID was \"%s\"\n\n"), cp);
	free(cp);
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
	register char *		errs;
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

	if ( (errs = EnvError) != NULLSTR )
		Fprintf
		(
			fd,
			english("Subject: NEGATIVE ACKNOWLEDGEMENT for file%s sent to %s\n\nDelivery confirmation failed for the following reason:-\n%s\n\n"),
			(NFthFiles>1)?english("s"):EmptyString,
			UQFthTo,
			CleanError(errs)
		);
	else
		Fprintf
		(
			fd,
			english("Subject: Acknowledgement for file%s sent to %s\n\n"),
			(NFthFiles>1)?english("s"):EmptyString,
			UQFthTo
		);

	Fprintf
	(
		fd,
		english("The following %s delivered at %s on %s:\n%s"),
		(NFthFiles>1)?english("files were"):english("file was"),
		ExSourceAddress,
		DateString(StartTime, date),
		fhdr
	);

	for ( fpp = FilesList, i = NFthFiles ; --i >= 0 ; fpp++ )
	{
		Fprintf
		(
			fd,
			ffmt,
			(*fpp)->f_mode & FTH_MODES,
			(PUlong)(*fpp)->f_length,
			DateString((*fpp)->f_time, date),
			(*fpp)->f_name
		);
	}

	print_id(fd);

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
returnmsg(fd)
	FILE *		fd;
{
	register char *	cp;
	register int	i;

	for ( i = 0 ; i < 79 ; i++ )
		putc('*', fd);

	if ( (cp = GetEnv(ENV_ID)) == NULLSTR )
	{
		if ( SavHdrPart == NULLSTR )
			cp = newstr(HdrID);
		else
			cp = concat(HdrID, Slash, SavHdrPart, NULLSTR);
	}

	Fprintf
	(
		fd,
		english("\nFILE%s SENT TO \"%s\"\nRETURNED FROM \"%s\"\nMESSAGE ID WAS \"%s\"\n"),
		(NFthFiles>1)?"S":"",
		UQFthTo,
		ExSourceAddress,
		cp
	);

	free(cp);

	if ( (cp = EnvError) != NULLSTR )
		Fprintf(fd, english("\nFailure explanation follows :-\n%s\n"), CleanError(cp));

	if ( (cp = GetEnv(ENV_TRUNC)) != NULLSTR )
	{
		char *	st = StripTypes(cp);

		Fprintf(fd, english("\nMessage was truncated at \"%s\"\n"), st);
		free(st);
		free(cp);
	}

	for ( i = 0 ; i < 79 ; i++ )
		putc('*', fd);

	Fprintf(fd, "\n\n");
}



/*
**	Send an acknowledgement message to source.
*/

void
sendack(mesg)
	char *			mesg;
{
	register long		size;
	register CmdV		cmdv;
	int			fd;
	static bool		acksent;

	if ( acksent )
		return;
	acksent = true;

	/*
	**	Make file for headers.
	*/

	AckMessage = newstr(WorkName);
	fd = create(UniqueName(AckMessage, CHOOSE_QUALITY, NoOpt, (Ulong)MAX_MESG_DATA, Time));

	/*
	**	Make FTP header.
	*/

	SetFthFiles();
	SetFthTo();	/* Only users at this node */

	FthType[0] = FTH_TYPE;

	while ( (size = WriteFtHeader(fd, (long)0)) == SYSERROR )
		Syserror(CouldNot, WriteStr, AckMessage);

	/*
	**	Make message header.
	*/

	HdrDest = SavHdrSource;

	if ( SavHdrPart == NULLSTR )
		HdrEnv = MakeEnv(ENV_ID, HdrID, NULLSTR);	/* Old ID saved */
	else
		HdrEnv = MakeEnv(ENV_ID, concat(HdrID, Slash, SavHdrPart, NULLSTR), NULLSTR);

	if ( mesg != NULLSTR )
		HdrEnv = concat(HdrEnv, MakeEnv(ENV_ERR, StripErrString(mesg), NULLSTR), NULLSTR);

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

	fd = create(UniqueName(WorkName, HdrQuality, NoOpt, (Ulong)size, Time));
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



/*
**	Sort files in message
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



/*
**	Make links to the message in the holding directory for each user.
*/

void
spoolfiles()
{
	register FthUlist *	up;
	register int		fd;
	CmdV			FileTimeout;
	CmdV			NakOnTimeout;

	newheader();

	FileTimeout.cv_number = ((Ulong)FilesExpireDays)*24L*60L*60L;
	NakOnTimeout.cv_number = NAK_ON_TIMEOUT;

	Recover(ert_return);

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
	{
		if ( up->u_name == NULLSTR )
			continue;

#		ifdef	DIRECTORY_SPOOLING
		if ( up->u_dir != NULLSTR )
		{
			/** Need to "create" and copy in each file **/
			continue;
		}
#		endif	/* DIRECTORY_SPOOLING */

		FreeCmds(&Commands);

		if ( up->u_next == (FthUlist *)0 )	/* Last user */
		{
			CopyCmds(&PartCmds, &Commands);
			CopyCmds(&HdrCmds, &Commands);
		}
		else
		{
			LinkCmds(&PartCmds, &Commands, (Ulong)0, PartsLength, WorkName, StartTime);
			LinkCmds(&HdrCmds, &Commands, (Ulong)0, NewHdrLength, WorkName, StartTime);
		}

		(void)AddCmd(&Commands, timeout_cmd, FileTimeout);
		if ( ReqAck )
			(void)AddCmd(&Commands, flags_cmd, NakOnTimeout);

		fd = create(UniqueName(WorkName, HdrQuality, NoOpt, MesgLength, Time));
		(void)WriteCmds(&Commands, fd, WorkName);
		(void)close(fd);		/* Ensure file written before move */

		Pid = up->u_uid;		/* Encode uid as "Pid" in Template */

		move(WorkName, UniqueName(SpoolName, SavHdrID[0], NoOpt, MesgLength, Time));
	}

	Recover(ert_finish);
}
