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


static char	sccsid[]	= "@(#)sendmsg.c	1.28 05/12/16";

/*
**	Send a message.
**
**	SETUID ==> ROOT.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	LOCKING
#define	SIGNALS
#define	STDIO
#define	STDARGS

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"handlers.h"
#include	"header.h"
#include	"link.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sub_proto.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


char	ShortUsage[]	= english("[-a] -A<handler> [<destination> ...] [-] [[-F]<file> ...]");

#define	MIN_SPLIT	8192L	/* Messages smaller than this not split */

/*
**	Parameters set from arguments.
*/

bool	Ack;			/* Acknowledgement of delivery required */
char *	EnvString;		/* String for inclusion in header environment */
bool	IgnAddrErr;		/* Ignore address errors if at least one is correct */
char *	Name	= sccsid;	/* Program invoked name */
bool	NoCall;			/* Don't auto-call for this message */
bool	NoOpt;			/* Don't optimise message delivery */
bool	NoRet;			/* Don't return message on error detection */
bool	NoSplit;		/* Don't split messages into multi-parts */
bool	NoUsage;		/* Don't write Usage message */
char *	OldID;			/* Insert into header environment */
char	Quality;		/* User's choice */
bool	ReportID;		/* Report ID generated on stdout */
bool	RevCharge;		/* Reverse charging for message */
bool	Silent;			/* Don't Warn() */
int	Traceflag;		/* Global tracing control */

AFuncv	getAddr _FA_((PassVal, Pointer));	/* Destination address (or file) */
AFuncv	getDest _FA_((PassVal, Pointer));	/* Destination address */
AFuncv	getFile _FA_((PassVal, Pointer));	/* File name */
AFuncv	getFil_ _FA_((PassVal, Pointer));	/* File from stdin */
AFuncv	getFilX _FA_((PassVal, Pointer));	/* File to be sent in-place */
AFuncv	getID _FA_((PassVal, Pointer));		/* Check environment ID */
AFuncv	getMMD _FA_((PassVal, Pointer));	/* MaxMesgData >= MIN_SPLIT */
AFuncv	getQuality _FA_((PassVal, Pointer));	/* Message priority */
AFuncv	getSource _FA_((PassVal, Pointer));	/* Change source address */
AFuncv	getSUbool _FA_((PassVal, Pointer));	/* Check user is P_SU */
AFuncv	getTt _FA_((PassVal, Pointer));		/* Set travel time */
AFuncv	getTtd _FA_((PassVal, Pointer));	/* Set time-to-die */
AFuncv	getUseP _FA_((PassVal, Pointer));	/* Different privileges */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(a, &Ack, 0),
	Arg_bool(d, &NoCall, 0),
	Arg_bool(i, &IgnAddrErr, 0),
	Arg_bool(m, &NoSplit, getSUbool),
	Arg_bool(o, &NoOpt, 0),
	Arg_bool(q, &ReportID, 0),
	Arg_bool(r, &NoRet, 0),
	Arg_bool(s, &Silent, 0),
	Arg_bool(u, &NoUsage, 0),
	Arg_bool(x, &RevCharge, getSUbool),
	Arg_string(A, &HdrHandler, 0, english("handler"), 0),
	Arg_string(D, 0, getDest, english("dest[,dest...]"), OPTARG|MANY),
	Arg_string(E, &EnvString, 0, english("environment"), OPTARG),
	Arg_string(F, 0, getFile, english("file"), OPTARG|MANY),
	Arg_string(I, &OldID, 0, english("ID"), OPTARG),
	Arg_long(M, &MaxMesgData, getMMD, english("message part size"), OPTARG),
	Arg_string(O, 0, getSource, english("source address"), OPTARG|OPTVAL),
	Arg_char(P, &Quality, getQuality, english("priority"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_string(V, 0, getUseP, english("priv-user"), OPTARG|MANY),
	Arg_string(X, 0, getFilX, english("in-place file"), OPTARG|MANY),
	Arg_long(Y, &HdrTt, getTt, english("travel-time"), OPTARG),
	Arg_long(Z, &HdrTtd, getTtd, english("time-to-die"), OPTARG),
	Arg_noflag(0, getAddr, english("dest[,dest...]"), OPTARG|MANY),
	Arg_minus(0, getFil_),
	Arg_noflag(0, getFile, english("file"), OPTARG|MANY),
	Arg_end
};

/*
**	File types.
*/

typedef enum
{
	copy_t, xmit_t, stdin_t
}
	File_t;

/*
**	Files list.
*/

typedef struct File	File;

struct File
{
	File *	f_next;
	char *	f_name;
	Ulong	f_length;
	File_t	f_type;
};

File *	Files;			/* Files to be included in message */
File **	Fend	= &Files;

/*
**	Miscellaneous.
*/

char	Buffer[8192];		/* For reading files through */
char *	CmdsFile;		/* Name of queued commands file describing message */
char *	CmdsTemp;		/* Name of work commands file describing message */
CmdHead	Commands;		/* Commands describing data files for message */
Addr *	Destinations;		/* List of destinations */
int	ExitCode;		/* If not 0, exit status from finish() */
int	FileCount;		/* Number of input files */
bool	ForceSplit;		/* Set if partition size argument given */
long	FreeSpace;		/* 1/2 space left on SPOOLDIR f/s in bytes */
Passwd	From;			/* Details of user */
bool	HadAddrErr;		/* Ignore address errors selected, and error noticed */
Addr *	Home_Addr;		/* HomeAddress in parts */
char *	HdrTemp;		/* Name of file containing message header */
char *	Message;		/* Contents */
char	MinQuality;		/* Minimum quality -- per message type */
char	NoPerm[]	= english("no permission");
Passwd	Perm;			/* Details of validation user */
bool	ShortEx;		/* Explain with short usage */
CmdHead	SpoolCmds;		/* Commands describing message parts for transmission */
bool	SU;			/* True if invoker has P_SU privileges */
char	Template[UNIQUE_NAME_LENGTH+1];

#define	ADDRESS_LIST	":@"	/* Check addresses don't have users by mistake */

void	addfile _FA_((char *, File_t));
void	AddrErr _FA_((char *, ...));
void	checkdestinations _FA_((void));
bool	checkfree _FA_((char *, Ulong));
void	checkperm _FA_((char *, Addr *, Passwd *));
void	copyfiles _FA_((void));
int	Create _FA_((char *, Ulong));
void	do_cmds _FA_((Ulong));
AFuncv	getdest _FA_((char *));
void	spool _FA_((void));

extern SigFunc	sigcatch;



int
main(argc, argv)
	int			argc;
	char *			argv[];
{
	register Handler *	handler;

	(void)alarm((unsigned)0);
	InitParams();

	InitCmds(&Commands);
	InitCmds(&SpoolCmds);

	SetUlimit();

	GetNetUid();
	GetInvoker(&From);

	if ( From.P_flags & P_SU )
		SU = true;

	Perm = From;

	if ( !ReadRoute(NOSUMCHECK) )
	{
		ExitCode = EX_NOHOST;
		Error(english("no routefile."));
	}

	Home_Addr = StringToAddr(newstr(HomeName), NO_STA_MAP);
	HdrSource = TypedAddress(Home_Addr);

	if ( EvalArgs(argc, argv, Usage, argerror) < 0 )
	{
		if ( !NoUsage )
		{
			if ( ShortEx )
				(void)fprintf(stderr, english("Usage: \"%s %s\"\n"), Name, ShortUsage);
			else
				(void)fprintf(stderr, "%s\n", ArgsUsage(Usage));
		}

		finish(EX_USAGE);
	}

	checkdestinations();	/* Set "HdrDest" */

	checkperm(HdrDest, Destinations, &From);

	if ( FileCount == 0 )
		addfile(NULLSTR, stdin_t);

	if
	(
		((handler = GetHandler(HdrHandler)) == (Handler *)0)
		?
		(!(From.P_flags & P_OTHERHANDLERS) && !NoRet)
		:
		(
			handler->proto_type != UNK_PROTO
			||
			(handler->restricted != '0' && !SU)
		)
	)
	{
		ExitCode = EX_NOPERM;
		Error(english("illegal handler."));
	}

	if ( handler != (Handler *)0 )
	{
		if ( handler->list != NULLSTR )
		{
			if ( !InList(CanAddrMatch, UnTypAddress(Destinations), handler->list, NULLSTR) )
			{
				ExitCode = EX_NOPERM;
				Error(english("illegal %s site."), handler->descrip);
			}
		}

		MinQuality = handler->quality;

		if ( handler->order == NEVER_ORDER )
			NoOpt = false;
		else
		if ( handler->order == ALWAYS_ORDER )
			NoOpt = true;

		HdrSubpt = handler->proto_type;
	}
	else
		HdrSubpt = UNK_PROTO;

	if ( NoCall )
		HdrFlags |= HDR_NO_AUTOCALL;
	if ( NoRet )
		HdrFlags |= HDR_NORET;
	if ( NoOpt )
		HdrFlags |= HDR_NOOPT;
	if ( Ack )
		HdrFlags |= HDR_RQA;
	if ( RevCharge )
		HdrFlags |= HDR_REV_CHARGE;

	if ( EnvString != NULLSTR && EnvString[0] != '\0' )
		HdrEnv = MakeEnv(ENV_FLAGS, EnvString, NULLSTR);
	else
		HdrEnv = EmptyString;

	if ( OldID != NULLSTR )
		HdrEnv = concat(HdrEnv, MakeEnv(ENV_ID, OldID, NULLSTR), NULLSTR);

	if ( (SigFunc *)signal(SIGHUP, SIG_IGN) != SIG_IGN )
		(void)signal(SIGHUP, sigcatch);
	if ( (SigFunc *)signal(SIGINT, SIG_IGN) != SIG_IGN )
		(void)signal(SIGINT, sigcatch);
	if ( (SigFunc *)signal(SIGQUIT, SIG_IGN) != SIG_IGN )
		(void)signal(SIGQUIT, sigcatch);
	(void)signal(SIGTERM, sigcatch);

	(void)umask(077);

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);

	Message = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

#	if	V8 == 0 && SYSTEM == 0
	(void)setruid(NetUid);	/* To check daemon read permission for -X files */
#	endif	/* V8 == 0 && SYSTEM == 0 */

	(void)checkfree(SPOOLDIR, (Ulong)sizeof(Buffer));

	copyfiles();

	spool();

	if ( ReportID )
		(void)fprintf(stdout, "%s\n", HdrID);

	exit(EX_OK);
	return 0;
}



/*
**	Add a file name into appropriate list, and extract details.
*/

void
addfile(file, type)
	char *		file;
	File_t		type;
{
	register File *	fp;
	struct stat	statb;

	if ( type != stdin_t && (file == NULLSTR || file[0] == '\0') )
	{
		ExitCode = EX_USAGE;
		Error(english("null file name in argument list."));
		return;
	}

	if ( type == xmit_t && file[0] != '/' )
	{
		ExitCode = EX_USAGE;
		Error(english("need full path name for directly transmitted file."));
		return;
	}

	if ( file == NULLSTR )
	{
		file = english("stdin");

		while ( fstat(0, &statb) == SYSERROR )
			if ( !SysWarn(CouldNot, StatStr, file) )
				finish(EX_DATAERR);
	}
	else
	while
	(
		access(file, 04) == SYSERROR
		||
		stat(file, &statb) == SYSERROR
	)
		if ( !SysWarn(CouldNot, ReadStr, file) )
			finish(EX_DATAERR);

	if ( (statb.st_mode & S_IFMT) == S_IFDIR )
	{
		if ( !Silent )
			Warn(english("directory \"%s\" ignored."), file);
		return;
	}

	fp = Talloc(File);
	fp->f_next = (File *)0;
	fp->f_name = file;
	fp->f_length = statb.st_size;
	fp->f_type = type;

	*Fend = fp;
	Fend = &fp->f_next;

	FileCount++;
}



/*
**	Argument address error processing.
*/
/*VARARGS1*/
void
#ifdef	ANSI_C
AddrErr(char *fmt, ...)
{
#else	/* ANSI_C */
AddrErr(va_alist)
	va_dcl
{
	char *	fmt;
#endif	/* ANSI_C */
	va_list vp;
	char *	a1;
	char *	a2;

#	ifdef	ANSI_C
	va_start(vp, fmt);
#	else	/* ANSI_C */
	va_start(vp);
	fmt = va_arg(vp, char *);
#	endif	/* ANSI_C */
	a1 =  va_arg(vp, char *);
	a2 =  va_arg(vp, char *);
	va_end(vp);

	if ( IgnAddrErr )
	{
		HadAddrErr = true;
		if ( !Silent )
			Warn(fmt, a1, a2);
	}
	else
	{
		ExitCode = EX_NOHOST;
		Error(fmt, a1, a2);
	}
}



/*
**	Make HdrDest list.
*/

void
checkdestinations()
{
	Trace1(1, "checkdestinations");

	ExitCode = EX_NOHOST;

	From.P_region = NULLSTR;	/* Already checked in getdest() */

	HdrDest = CheckAddr(&Destinations, &From, AddrErr, true);

	if ( Destinations == (Addr *)0 || HdrDest == NULLSTR || HdrDest[0] == '\0' )
	{
		Error(english("no legal destination specified."));
		return;
	}

	if ( !ForceSplit && HdrDest[0] == ATYP_DESTINATION && FindLink(&HdrDest[1], (Link *)0) )
		NoSplit = true;

	ExitCode = EX_OK;

	Trace2(1, "HdrDest \"%s\"", HdrDest);
}



/*
**	Check free space.
*/

bool
checkfree(file, space)
	char *		file;	/* For f/s */
	Ulong		space;	/* Bytes */
{
	static char	mesg[]	= english("not enough space left in \"%s\".");
	static bool	warned;

	if ( (FreeSpace -= space) > 0 )
		return true;

	if ( !FSFree(file, space) )
	{
		if ( !SU )
		{
			ExitCode = EX_TEMPFAIL;
			Error(mesg, SPOOLDIR);
		}

		if ( !warned )
		{
			warned = true;
			if ( !Silent )
				Warn(mesg, SPOOLDIR);
		}

		(void)sleep(20);
		FreeSpace = 0;
		return false;
	}

	FreeSpace = FreeFSpace / 2;	/* Can take 1/2 space left */
	FreeSpace -= space;
	return true;
}



/*
**	Check current user privileges.
*/

void
checkperm(dest, addr, user)
	char *	dest;
	Addr *	addr;
	Passwd *user;
{
	Trace4(1, "checkperm(%s, %s, %#x)", dest, UnTypAddress(addr), user->P_flags);

	if
	(
		!(user->P_flags & P_CANSEND)
		&&
		(LOCALSEND == 0 || strccmp(dest, HdrSource) != STREQUAL)
		&&
		(LOCAL_NODES == NULLSTR || !InList(CanAddrMatch, UnTypAddress(addr), LOCAL_NODES, NULLSTR))
	)
	{
		ExitCode = EX_NOPERM;
		Error(english("\"%s\" has no permission to send network messages"), user->P_name);
	}
}



/*
**	Read files and copy into message.
*/

void
copyfiles()
{
	register char *	cp;
	register int	r;
	register int	n;
	register int	ifd;
	register int	ofd		= -1;
	register File *	fp;
	register Ulong	datalength	= 0;
	register bool	copy		= false;
	register CmdV	cmdv;
	bool		checkcopy;

	for ( fp = Files ; fp != (File *)0 ; fp = fp->f_next )
	{
		if ( fp->f_type == xmit_t )
		{
			if ( fp->f_length == 0 )
				continue;

			if ( copy )
			{
				(void)close(ofd);
				do_cmds(datalength);
				copy = false;
			}

			while ( access(fp->f_name, 04) == SYSERROR )
				if ( !SysWarn(CouldNot, ReadStr, fp->f_name) )
					finish(EX_DATAERR);

			(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = fp->f_name, cmdv));
			(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
			(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = fp->f_length, cmdv));

			DataLength += fp->f_length;

			continue;
		}

		if ( !copy )
		{
			ofd = Create(Message, MAX_MESG_DATA);
			(void)chown(Message, NetUid, NetGid);
			datalength = 0;
			copy = true;
		}

		checkcopy = true;

		if ( fp->f_type == stdin_t )
		{
			ifd = 0;
		}
		else
		{
			while ( (ifd = open(fp->f_name, O_READ)) == SYSERROR )
				if ( !SysWarn(CouldNot, OpenStr, fp->f_name) )
					finish(EX_DATAERR);
			if ( checkfree(SPOOLDIR, fp->f_length) )
				checkcopy = false;
		}

		for ( ;; )
		{
			while ( (n = read(ifd, Buffer, sizeof Buffer)) > 0 )
			{
				datalength += n;
				cp = Buffer;

				while ( checkcopy && (FreeSpace -= n) <= 0 )
					if ( checkfree(SPOOLDIR, (Ulong)n) )
						break;

				while ( (r = write(ofd, cp, n)) != n )
				{
					if ( r == SYSERROR )
						Syserror(CouldNot, WriteStr, Message);
					else
					{
						cp += r;
						n -= r;
					}
				}
			}

			if ( n == 0 )
				break;

			if ( !SysWarn(CouldNot, ReadStr, fp->f_name) )
				finish(EX_DATAERR);
		}

		if ( ifd != 0 )
			(void)close(ifd);
	}

	if ( copy )
	{
		(void)close(ofd);
		do_cmds(datalength);
	}
}



/*
**	Special creat(2).
*/

int
Create(file, size)
	char *	file;
	Ulong	size;
{
	int	fd;

	while
	(
#		if	defined(O_CREAT) && defined(O_FREE)
		(fd = open(
#		else	/* defined(O_CREAT) && defined(O_FREE) */
		(fd = creat(
#		endif	/* defined(O_CREAT) && defined(O_FREE) */
				UniqueName(file, HdrQuality, NoOpt, size, Time),
#				if	defined(O_CREAT) && defined(O_FREE)
				O_FREE|O_CREAT|O_TRUNC|O_EXCL|O_WRITE,
#				endif	/* defined(O_CREAT) && defined(O_FREE) */
				0600
		))
		== SYSERROR
	)
		if ( !CheckDirs(file) )
			Syserror(CouldNot, CreateStr, file);

	return fd;
}



/*
**	Add commands to describe current message fragment.
*/

void
do_cmds(datalength)
	Ulong		datalength;
{
	register CmdV	cmdv;

	if ( datalength == 0 )
		return;

	DataLength += datalength;

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = Message, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = datalength, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = Message, cmdv));
}



/*
**	Called from the errors routines to cleanup.
*/

void
finish(error)
	int		error;
{
	register int	i;
	register Cmd *	cmdp;

	if ( CmdsFile != NULLSTR )
		(void)unlink(CmdsFile);

	if ( CmdsTemp != NULLSTR )
		(void)unlink(CmdsTemp);

	if ( HdrTemp != NULLSTR )
		(void)unlink(HdrTemp);

	if ( Message != NULLSTR )
		(void)unlink(Message);

	for ( cmdp = Commands.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
			(void)unlink(cmdp->cd_value);

	for ( i = 0 ; i < 9 ; i++ )
		(void)UnLock(i);
	
	if ( ExitCode != 0 )
		error = ExitCode;

	(void)exit(error);
}



/*
**	Address/filename argument.
*/

/*ARGSUSED*/
AFuncv
getAddr(val, arg)
	register PassVal	val;
	Pointer			arg;
{
	if ( FileCount && Destinations != (Addr *)0 )
		return REJECTARG;

	if ( access(val.p, 0) != SYSERROR )
	{
		if ( Destinations == (Addr *)0 )
		{
			if ( HadAddrErr )
				return ACCEPTARG;

			ShortEx = true;
			return english("must specify address before files");
		}

		return REJECTARG;
	}

	return getdest(val.p);
}



/*
**	Destination address.
*/

/*ARGSUSED*/
AFuncv
getDest(val, arg)
	PassVal		val;
	Pointer		arg;
{
	return getdest(val.p);
}



/*
**	A destination must be in one of the following forms:
**
**	'domain[.domain...]'						address
**	'*.address'							broadcast
**	'{address|broadcast}!{address|broadcast}...'			explicit
**	'{address|broadcast|explicit},{address|broadcast|explicit}...'	multicast
**
**	(See "address.h" for allowable formats.)
*/

AFuncv
getdest(s)
	register char *	s;
{
	Addr *		ap;

	Trace2(1, "getdest \"%s\"", s);

	if ( s == NULLSTR || *s == '\0' )
		return (AFuncv)english("null address argument?");

	if ( strpbrk(s, ADDRESS_LIST) != NULLSTR )
	{
		NoUsage = true;
		return (AFuncv)english("illegal destination format");
	}

	ap = StringToAddr(s, STA_MAP);

	if ( ap == (Addr *)0 )
	{
		NoUsage = true;
		return (AFuncv)english("illegal destination format");
	}

	(void)CheckAddr(&ap, &Perm, AddrErr, true);

	checkperm(TypedAddress(ap), ap, &Perm);

	AddAddr(&Destinations, ap);

	return ACCEPTARG;
}



/*
**	Explicit filename.
*/

/*ARGSUSED*/
AFuncv
getFile(val, arg)
	PassVal		val;
	Pointer		arg;
{
	addfile(val.p, copy_t);
	return ACCEPTARG;
}



/*
**	File from stdin.
*/

/*ARGSUSED*/
AFuncv
getFil_(val, arg)
	PassVal		val;
	Pointer		arg;
{
	addfile(NULLSTR, stdin_t);
	return ACCEPTARG;
}



/*
**	In-place filename.
*/

/*ARGSUSED*/
AFuncv
getFilX(val, arg)
	PassVal		val;
	Pointer		arg;
{
	addfile(val.p, xmit_t);
	return ACCEPTARG;
}



#if	0	/* OldID can be anything */
/*
**	Set message identifier.
*/

/*ARGSUSED*/
AFuncv
getID(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if
	(
		val.p[0] < HIGHEST_QUALITY
		||
		val.p[0] > LOWEST_QUALITY
		||
		strlen(val.p) != UNIQUE_NAME_LENGTH
	)
	{
		NoUsage = true;
		return (AFuncv)english("illegal message ID");
	}

	return ACCEPTARG;
}
#endif	/* 0 */



/*
**	Set message part split size.
*/

/*ARGSUSED*/
AFuncv
getMMD(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.l < MIN_SPLIT )
	{
		(void)sprintf(Buffer, english("minimum value is %lu"), (PUlong)MIN_SPLIT);
		NoUsage = true;
		return (AFuncv)Buffer;
	}

	if ( val.l > MAX_MESG_DATA && !SU )
	{
		(void)sprintf(Buffer, english("no permission for values larger than %lu"), (PUlong)MAX_MESG_DATA);
		NoUsage = true;
		return (AFuncv)Buffer;
	}

	ForceSplit = true;

	return ACCEPTARG;
}



/*
**	Set message priority.
*/

/*ARGSUSED*/
AFuncv
getQuality(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.c < HIGHEST_QUALITY || val.c > LOWEST_QUALITY )
	{
		(void)sprintf(Buffer, english("priority: %c (high) to %c (low)"), HIGHEST_QUALITY, LOWEST_QUALITY);
		NoUsage = true;
		return (AFuncv)Buffer;
	}

	return ACCEPTARG;
}



/*
**	Change source address.
*/

/*ARGSUSED*/
AFuncv
getSource(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Addr *		ap;
	Link		link;

	if ( val.p[0] == '\0' )
		return ACCEPTARG;

	ap = StringToAddr(newstr(val.p), STA_MAP);

	if ( FindAddr(ap, &link, FASTEST) == wh_none && !NoRet )
	{
		NoUsage = true;
		return (AFuncv)english("unknown address");
	}

	if ( !SU && !AddrAtHome(&ap, false, true) )
	{
		FreeAddr(&ap);
		NoUsage = true;
		return (AFuncv)NoPerm;
	}

	FreeAddr(&ap);

	ap = StringToAddr(val.p, NO_STA_MAP);

	if ( ADDRTYPE(ap) != ATYP_DESTINATION )
	{
		FreeAddr(&ap);
		NoUsage = true;
		return (AFuncv)english("illegal source address");
	}

	HdrSource = newstr(TypedAddress(ap));

	FreeAddr(&ap);
	return ACCEPTARG;
}



/*
**	Check priviledged boolean.
*/

/*ARGSUSED*/
AFuncv
getSUbool(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( !SU )
		*(bool *)arg = false;

	return ACCEPTARG;
}



/*
**	Set travel time.
*/

/*ARGSUSED*/
AFuncv
getTt(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( !SU )
	{
		NoUsage = true;
		return (AFuncv)NoPerm;
	}

	if ( val.l <= 0 )
		return ARGERROR;

	return ACCEPTARG;
}



/*
**	Set time-to-die.
*/

/*ARGSUSED*/
AFuncv
getTtd(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.l <= 0 )
		return ARGERROR;

	return ACCEPTARG;
}



/*
**	Change invoker privileges.
*/
/*ARGSUSED*/
AFuncv
getUseP(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.p[0] == '\0' )
		return ACCEPTARG;

	if
	(
		!SU
		&&
		strcmp(val.p, From.P_user) != STREQUAL
	)
	{
		NoUsage = true;
		return (AFuncv)NoPerm;
	}

	if ( !GetUid(&Perm, val.p) )
		return (AFuncv)Perm.P_error;

	GetPrivs(&Perm);

	return ACCEPTARG;
}



/*
**	Catch signals and call finish
*/

void
sigcatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	finish(sig);
}



/*
**	Write headers and queue commands file(s) for router.
*/

void
spool()
{
	register Ulong	l;
	register Ulong	start;
	register CmdV	cmdv;
	register int	fd;
	register char *	routingdir;
	int		maxparts;
	int		partno;
	char		part_address[2*(ULONG_SIZE+1)];

	SetUser(NetUid, NetGid);

	HdrTemp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	HdrQuality = CHOOSE_QUALITY;
	fd = Create(HdrTemp, DataLength);

	HdrID = newstr(&HdrTemp[strlen(SPOOLDIR) + strlen(WORKDIR)]);
	HdrQuality = HdrID[0];

	if ( HdrQuality < MinQuality )
	{
		HdrQuality = MinQuality;
		HdrID[0] = MinQuality;
	}

	if ( Quality >= HdrQuality || (SU && Quality != '\0') )
	{
		HdrQuality = Quality;
		HdrID[0] = Quality;
	}

	HdrType = HDR_TYPE2;

	routingdir = concat(SPOOLDIR, ROUTINGDIR, NULLSTR);
	CmdsFile = concat(routingdir, Template, NULLSTR);
	CmdsTemp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	if ( !NoSplit && DataLength > (MaxMesgData+MIN_SPLIT) )
	{
		HdrFlags |= HDR_NOOPT;	/* So that parts can arrive in order */
		NoOpt = true;
		maxparts = (DataLength+(MaxMesgData-(MIN_SPLIT+1))) / MaxMesgData;
	}
	else
	{
		MaxMesgData = DataLength;
		maxparts = 1;
	}

	for ( start = 0, partno = 1 ;; partno++ )
	{
		if ( (l = DataLength-start) > (MaxMesgData+MIN_SPLIT) )
			l = MaxMesgData;

		if ( maxparts > 1 )
		{
			(void)sprintf(part_address, "%d:%d", partno, maxparts);
			HdrPart = part_address;
		}

		FreeCmds(&SpoolCmds);

		/*
		**	Data.
		*/

		if ( l != 0 )
			LinkCmds(&Commands, &SpoolCmds, start, start+l, CmdsTemp, Time);

		/*
		**	Message header.
		*/

		while ( WriteHeader(fd, (long)0, 0) == SYSERROR )
			Syserror(CouldNot, WriteStr, HdrTemp);

		(void)close(fd);

		(void)AddCmd(&SpoolCmds, file_cmd, (cmdv.cv_name = HdrTemp, cmdv));
		(void)AddCmd(&SpoolCmds, start_cmd, (cmdv.cv_number = 0, cmdv));
		(void)AddCmd(&SpoolCmds, end_cmd, (cmdv.cv_number = HdrLength, cmdv));
		(void)AddCmd(&SpoolCmds, unlink_cmd, (cmdv.cv_name = HdrTemp, cmdv));

		if ( (cmdv.cv_number = HdrTtd) > 0 )
		{
			(void)AddCmd(&SpoolCmds, timeout_cmd, cmdv);
			if ( Ack )
				(void)AddCmd(&SpoolCmds, flags_cmd, (cmdv.cv_number = NAK_ON_TIMEOUT, cmdv));
		}

		/*
		**	Commands file.
		*/

		fd = Create(CmdsTemp, DataLength);

		(void)WriteCmds(&SpoolCmds, fd, CmdsTemp);

		(void)close(fd);

		/*
		**	Queue commands for router.
		*/

#		if	PRIORITY_ROUTING == 1
		move(CmdsTemp, UniqueName(CmdsFile, HdrQuality, NoOpt, (DataLength+HdrLength), Time));
#		else	/* PRIORITY_ROUTING == 1 */
		move(CmdsTemp, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time));
#		endif	/* PRIORITY_ROUTING == 1 */

		if ( (start += l) == DataLength )
			break;

		fd = Create(HdrTemp, DataLength);
	}

	(void)unlink(Message);

	if ( !DaemonActive(routingdir, true) && !Silent )
		Warn(english("routing daemon not running."));
}
