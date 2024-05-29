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


static char	sccsid[]	= "@(#)fileserver.c	1.38 06/01/21";

/*
**	Process request from remote site for a file from our system.
**
**	SETUID -> root
*/

#define	MAXFILES	(MAXVARARGS-4)

#define	FILE_CONTROL
#define	LOCKING
#define	RECOVER
#define	SIGNALS
#define	STAT_CALL
#define	STDARGS
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
#include	"header.h"
#include	"ndir.h"
#include	"params.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"stats.h"
#include	"sub_proto.h"
#include	"sysexits.h"
#if	SUN3 == 1
#include	"sun3.h"
#endif	/* SUN3 == 1 */

#if	AUSAS == 0
#include	<grp.h>
#endif	/* AUSAS == 0 */

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
**	Parameters read from header of file
*/

char *	Options;		/* Service related options */
char *	RemUser;		/* Name of user requesting service */
char *	Service;		/* Service requested */

/*
**	The services offered
*/

void	sendfiles _FA_((bool));
void	sendlist _FA_((bool));

typedef struct
{
	char *	name;
	vFuncp	server;
	bool	param;
}
	ServiceType;

ServiceType	Serves[] =
{
	{	"SendFile",	(vFuncp)sendfiles,	false	},
	{	"List",		(vFuncp)sendlist,	false	},
	{	"ListVerbose",	(vFuncp)sendlist,	true	},
	{	NULLSTR						}
};

/*
**	Structure for holding individual file requests.
*/

typedef struct FileL	FileL;
struct FileL
{
	FileL *	next;
	char *	name;	/* Relative name requested */
	char *	path;	/* Absolute pathname */
	Ulong	size;
	bool	inpl;	/* Only set if -X possible */
};

FileL *	FileList;
FileL **EFileList	= &FileList;

/*
**	Miscellaneous.
*/

#define	LINELEN		80

CmdHead	Cleanup;		/* Files to be unlinked */
CmdHead	DataCmds;		/* Describing FTP part of message */
char *	FwdNode;		/* Node(s) at which message was forwarded */
long	ListSize;		/* Size of list(verbose) output */
int	MaxListCols	= LINELEN; /* Max `List' colums per line */
Ulong	MaxListSize;		/* Max `List' size sent */
char *	MesgData;		/* Name of message file containing data */
int	No_lr;			/* Don't allow recursion */
int	No_X;			/* Don't allow -X (in-place file copies) */
char *	PublicDir;		/* Place where files are stored */
bool	Recurse;		/* Should be set from "Options" */
int	RetVal	= EX_OK;	/* Value to return to receiver */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
bool	Truncated;		/* Listing exceeded `MaxListSize' */
bool	Verbose;		/* Set from "Service" */
char *	WorkName;		/* Template for spooling files in WORKDIR */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"FILESERVERLOG",	&FILESERVERLOG,			PSPOOL},
	{"FILESERVER_MAXC",	(char **)&MaxListCols,		PINT},
	{"FILESERVER_MAXL",	(char **)&MaxListSize,		PLONG},
	{"FILESERVER_NOLR",	(char **)&No_lr,		PINT},
	{"FILESERVER_NOX",	(char **)&No_X,			PINT},
	{"PUBLICFILES",		&PUBLICFILES,			PSPOOL},
	{"SERVERGROUP",		&SERVERGROUP,			PSTRING},
	{"SERVERUSER",		&SERVERUSER,			PSTRING},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

#define	Fprintf		(void)fprintf

char *	check_perm _FA_((void));
void	cleanup _FA_((void));
void	do_request _FA_((void));
char *	file_str _FA_((FILE *));
void	flist _FA_((char *, FILE *, bool));
void	forward _FA_((char *));
bool	getcommand _FA_((CmdT, CmdV));
FILE *	get_ff_hdr _FA_((void));
void	mail_back _FA_((char *));
void	return_error _FA_((int, char *, ...));
void	set_user _FA_((VarArgs *));
void	vlist _FA_((char *, FILE *, int));
void	writestats _FA_((void));

#if	AUSAS == 0
extern struct group *	getgrnam _FA_((const char *));
#endif	/* AUSAS == 0 */



int
main(argc, argv)
	int			argc;
	char *			argv[];
{
	register Forward *	fp;

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

	GetNetUid();

	if ( getuid() != NetUid )
		Error(english("No permission."));

	StartTime = Time - SavHdrTt;

	DataName = SenderName = UserName = "fileserver";
	DataSize = DataLength;

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	WorkName = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

#	if	SUN3 == 1
	if ( Sun3 )
	{
		ExSourceAddress = StripDomain(newstr(SavHdrSource), OzAu, Au, false);
		ExLinkAddress = StripDomain(newstr(LinkAddress), OzAu, Au, false);
		ExHomeAddress = StripDomain(newstr(HomeAddress), OzAu, Au, false);
	}
	else
#	endif	/* SUN3 == 1 */
	{
		ExSourceAddress = StripTypes(SavHdrSource);
		ExLinkAddress = StripTypes(LinkAddress);
		ExHomeAddress = StripTypes(HomeAddress);
	}

	if ( SavHdrFlags & HDR_ACK )
		Error(english("file request acknowledged from \"%s\""), ExSourceAddress);

	HdrEnv = SavHdrEnv;
	FwdNode = GetEnv(ENV_FWD);

	InitCmds(&Cleanup);
	InitCmds(&DataCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, english("read commands from"), "stdin");

	Recover(ert_finish);

	if ( SavHdrFlags & HDR_RETURNED )
		mail_back(GetEnv(ENV_ERR));
	else
	if ( SavHdrPart != NULLSTR )
		Error(english("Not expecting multi-part message!"));
	else
	if ( (fp = Forwarded(Name, Name, SavHdrSource)) != (Forward *)0 )
	{
		while ( setgid(NetGid) == SYSERROR && geteuid() == 0 )
			if ( !SysWarn(CouldNot, "setgid", "group") )
				break;
		while ( setuid(NetUid) == SYSERROR && geteuid() == 0 )
			Syserror(CouldNot, "setuid", "user");
		forward(fp->address);
	}
	else
		do_request();

	cleanup();

	finish(RetVal);
	return 0;
}



/*
**	Check local access rights for ``RemUser''.
**
**	Read ``publicfiles'', format is:-
**
**	"user[@address][<tab>directory]\n"
*/

char *
check_perm()
{
	register char *	cp;
	register char *	hp;
	register char *	st = StripTypes(SavHdrSource);
	register FILE *	fd;
	char		buf[BUFSIZ];

	while ( (fd = fopen(PUBLICFILES, "r")) == NULL )
		Syserror(CouldNot, ReadStr, PUBLICFILES);

	while ( fgets(buf, sizeof buf, fd) != NULLSTR )
	{
		if ( buf[0] == '#' )
			continue;				/* Ignore comment */

		if ( (cp = strchr(buf, '\n')) != NULLSTR )
			*cp = '\0';

		if ( (cp = strpbrk(buf, " \t")) != NULLSTR )	/* Directory name */
		{
			*cp++ = '\0';
			cp += strspn(cp, " \t");
			if ( *cp == '\0' )
				cp = NULLSTR;
		}

		if ( (hp = strchr(buf, '@')) != NULLSTR )	/* Address */
		{
			*hp++ = '\0';
			if ( hp[0] != '*' || hp[1] == '.' )
			{
				Addr *	ap = StringToAddr(hp, true);

				if ( !AddressMatch(UnTypAddress(ap), st) )
				{
					FreeAddr(&ap);
					continue;
				}

				FreeAddr(&ap);
			}
		}

		if ( buf[0] != '*' && strcmp(buf, RemUser) != STREQUAL )
			continue;

		if ( cp == NULLSTR )
			break;

		Trace2(1, "check_perm resultis \"%s\"", cp);

		free(st);
		(void)fclose(fd);
		return newstr(cp);
	}

	free(st);
	(void)fclose(fd);

	return_error(EX_UNAVAILABLE, english("You have no access permissions at %s"), ExHomeAddress);

	return NULLSTR;
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
**	Check validity of file request, and send files back.
*/

void
do_request()
{
	register char *		cp;
	register char * 	dp;
	register FileL *	fp;
	register ServiceType *	sp;
	register FILE *		fd;
	int			id;
	struct stat		statb;

	if ( PUBLICFILES == NULLSTR )
	{
		return_error
		(
			EX_UNAVAILABLE,
			english("There are no remotely accessible files at %s."),
			ExHomeAddress
		);
		return;
	}

	if ( stat(PUBLICFILES, &statb) == SYSERROR )
	{
		return_error(EX_UNAVAILABLE, english("No public files are currently available."));
		return;
	}

	fd = get_ff_hdr();	/* Sets RemUser (needed by check_perm()), Service, Options */

	switch ( statb.st_mode & S_IFMT )
	{
	default:
		(void)fclose(fd);
		Error(english("%s -- unrecognised file type 0%o"), PUBLICFILES, statb.st_mode);
		return;

	case S_IFDIR:
		PublicDir = PUBLICFILES;
		break;

	case S_IFREG:
		if ( (PublicDir = check_perm()) == NULLSTR )
		{
			(void)fclose(fd);
			return;
		}
		break;
	}

#	if	AUSAS == 0
	if ( SERVERGROUP != NULLSTR )
	{
		register struct group * gp;

		if ( (gp = getgrnam(SERVERGROUP)) == (struct group *)0 )
		{
			(void)fclose(fd);
			Error(english("Group %s does not exist!"), SERVERGROUP);
			return;
		}

		if ( gp->gr_gid == 0 && NetGid != 0 )
		{
			(void)fclose(fd);
			Error(english("fileserver gid 0"));
			return;
		}

		id = gp->gr_gid;
	}
	else
		id = NetGid;

	while ( setgid(id) == SYSERROR && geteuid() == 0 )
		if ( !SysWarn(CouldNot, "setgid", "group") )
			break;
#	endif	/* AUSAS == 0 */

	if ( SERVERUSER != NULLSTR )
	{
		Passwd	pw;

		if ( !GetUid(&pw, SERVERUSER) )
		{
			(void)fclose(fd);
			Error(english("User %s does not exist!"), SERVERUSER);
			return;
		}

#		if	AUSAS == 1
		if ( (id = pw.P_gid) == 0 && NetGid != 0 )
		{
			(void)fclose(fd);
			Error(english("fileserver gid 0"));
			return;
		}

		while ( setgid(id) == SYSERROR && geteuid() == 0 )
			if ( !SysWarn(CouldNot, "setgid", "group") )
				break;
#		endif	/* AUSAS == 1 */

		if ( (id = pw.P_uid) == 0 && NetUid != 0 )
		{
			(void)fclose(fd);
			Error(english("fileserver uid 0"));
			return;
		}
	}
	else
		id = NetUid;

	while ( setuid(id) == SYSERROR && geteuid() == 0 )
		Syserror(CouldNot, "setuid", "user");

	if ( chdir(PublicDir) == SYSERROR )
	{
		return_error(EX_UNAVAILABLE, english("Public files are inaccessible."));
		(void)fclose(fd);
		return;
	}

	if ( strcmp(PublicDir, Slash) == STREQUAL )
		PublicDir = EmptyString;

	for ( sp = Serves ; (cp = sp->name) != NULLSTR ; sp++ )
		if ( strcmp(Service, cp) == STREQUAL )
			break;

	if ( sp == (ServiceType *)0 )
	{
		return_error(EX_UNAVAILABLE, english("Service \"%s\" is not implemented at %s"), Service, ExHomeAddress);
		(void)fclose(fd);
		return;
	}

	Recover(ert_return);

	for ( fp = (FileL *)0 ;; )
	{
		if ( (cp = file_str(fd)) == NULLSTR )
			break;

		if ( *cp == '/' )
			if ( *++cp == '\0' )
				*--cp = '.';

		if ( strncmp(cp, "../", 3) == STREQUAL || strcmp(cp, "..") == STREQUAL )
			dp = cp;
		else
			dp = StringMatch(cp, "/..");

		if ( dp != NULLSTR )
		{
			return_error(EX_DATAERR, english("Illegal \"..\" in filename <%s>"), cp);
			continue;
		}

		if ( access(cp, 4) == SYSERROR || stat(cp, &statb) == SYSERROR )
		{
			return_error(EX_UNAVAILABLE, english("%s of %s refused - unavailable"), Service, cp);
			continue;
		}

		if ( (statb.st_mode & S_IFMT) == S_IFDIR && sp->server == (vFuncp)sendfiles )
		{
			return_error(EX_DATAERR, english("%s is a directory - not returned"), cp);
			continue;
		}

		if ( statb.st_size == 0 && sp->server == (vFuncp)sendfiles )
		{
			return_error(EX_DATAERR, english("%s is zero-length - not returned"), cp);
			continue;
		}

		fp = Talloc0(FileL);
		*EFileList = fp;
		EFileList = &fp->next;
		fp->name = cp;
		fp->path = concat(PublicDir, Slash, cp, NULLSTR);
		fp->size = statb.st_size;

		if
		(
			!No_X
			&&
			sp->server == (vFuncp)sendfiles
			&&
			(statb.st_mode & 4)
			&&
			(PublicDir[0] == '/' || PublicDir[0] == '\0')
		)
			fp->inpl = true;
	}

	(void)fclose(fd);

	if ( fp == (FileL *)0 )
	{
		if ( RetVal == EX_OK )
			return_error(EX_NOINPUT, english("No files requested."));
		return;
	}

	Recover(ert_finish);

	(void)(*sp->server)(sp->param);
}



/*
**	Extract next null-terminated string from message data.
*/

char *
file_str(fd)
	register FILE * fd;
{
	register int	c;
	register char * cp;
	register int	i;
	char		string[128];

	for ( cp = string, i = sizeof string ; --i >= 0 ; )
	{
		if ( (c = getc(fd)) == EOF )
			return NULLSTR;

		if ( (*cp++ = c) == '\0' )
		{
			if ( (cp - string) == 1 )
				return NULLSTR;

			return newstr(string);
		}
	}

	return NULLSTR;
}



/*
**	Called at end to cleanup.
*/

void
finish(error)
	int	error;
{
	if ( !ExInChild )
	{
		writestats();

		if ( RetVal != EX_OK )
			error = RetVal;
	}

	(void)exit(error);
}



/*
**	Output a filename.
*/

void
flist(file, fd, isdir)
	register char *	file;
	register FILE *	fd;
	bool		isdir;
{
	register int	l;
	register int	len;
	register char *	cp;
	static int	linelen = 0;

	if ( file == NULLSTR )
	{
		if ( linelen > 0 )
		{
			(void)putc('\n', fd);
			ListSize += 1;
		}
		linelen = 0;
		return;
	}

	if ( !isdir && (cp = strrchr(file, '/')) != NULLSTR )
		file = ++cp;

	/*
	**	Output in tidy columns.
	*/

	if ( (l = strlen(file)) < 16 )
		len = 16;
	else
	if ( l < 32 )
		len = 32;
	else
	if ( l < 48 )
		len = 48;
	else
	if ( l < 64 )
		len = 64;
	else
		len = LINELEN;

	if ( (linelen += len) > LINELEN )
	{
		(void)putc('\n', fd);
		ListSize += 1;
		linelen = len;
	}
	else
	if ( linelen > (LINELEN-16) )
		len = 1;

	Fprintf(fd, "%-*s", len, file);
	ListSize += (len==1)?l:len;
}



/*
**	Forward request to newaddress.
**
**	(``router'' will check for routing loops if include original HdrRoute.)
*/

void
forward(newaddress)
	char *		newaddress;
{
	register char *	cp;
	register CmdV	cmdv;
	register int	fd;
	CmdHead		Commands;
	bool		NoOpt;

	InitCmds(&Commands);

	LinkCmds(&DataCmds, &Commands, (Ulong)0, DataLength, WorkName, StartTime);

	NoOpt = (bool)((SavHdrFlags & HDR_NOOPT) != 0);
	fd = create(UniqueName(WorkName, SavHdrID[0], NoOpt, (Ulong)DataLength, StartTime));

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
	HdrSubpt = UNK_PROTO;
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

	fd = create(UniqueName(WorkName, HdrQuality, NoOpt, (Ulong)DataLength+HdrLength, StartTime));

	(void)WriteCmds(&Commands, fd, WorkName);
	
	(void)close(fd);

	/*
	**	Queue commands for router.
	*/

	cp = concat(SPOOLDIR, ROUTINGDIR, Template, NULLSTR);
#	if	PRIORITY_ROUTING == 1
	move(WorkName, UniqueName(cp, HdrQuality, NoOpt, (Ulong)DataLength+HdrLength, StartTime));
#	else	/* PRIORITY_ROUTING == 1 */
	move(WorkName, UniqueName(cp, STATE_QUALITY, NOOPTNAME, (Ulong)0, StartTime));
#	endif	/* PRIORITY_ROUTING == 1 */
	free(cp);

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
	switch ( type )
	{
	case file_cmd:
		if ( MesgData == NULLSTR )
			MesgData = newstr(val.cv_name);	/* Only expecting one non-header file */
		break;

	case unlink_cmd:
		(void)AddCmd(&Cleanup, type, val);
		return true;

	default:
		break;	/* gcc -Wall */
	}

	AddCmd(&DataCmds, type, val);

	return true;
}



/*
**	Open message data file and extract file request header details.
*/

FILE *
get_ff_hdr()
{
	register FILE *	fd;

	while ( (fd = fopen(MesgData, "r")) == (FILE *)0 )
		Syserror(CouldNot, OpenStr, MesgData);

	RemUser = file_str(fd);
	Service = file_str(fd);
	Options = file_str(fd);

	if ( Service == NULLSTR || RemUser == NULLSTR || getc(fd) != '\0' )
	{
		return_error(EX_DATAERR, english("Bad file request header"));
		finish(EX_DATAERR);
	}

	Trace4(1, "get_ff_hdr: RemUser=\"%s\", Service=\"%s\", Options=\"%s\"", RemUser, Service, Options);

	return fd;
}



/*
**	Mail user results of remote query.
*/

void
mail_back(mesg)
	char *		mesg;
{
	register char *	cp;
	register FILE *	fd;
	register char *	errs;
	ExBuf		exbuf;

	(void)fclose(get_ff_hdr());

	if ( SERVERUSER != NULLSTR )
		SenderName = SERVERUSER;
	else
		SenderName = "Fileserver";

	errs = ExSourceAddress;
	ExSourceAddress = ExHomeAddress;	/* For BINMAILARGS */

	FIRSTARG(&exbuf.ex_cmd) = BINMAIL;
	ExpandArgs(&exbuf.ex_cmd, BINMAILARGS);

	ExSourceAddress = errs;

	NEXTARG(&exbuf.ex_cmd) = RemUser;

	fd = (FILE *)Execute(&exbuf, set_user, ex_pipe, SYSERROR);

	if ( MAIL_RFC822_HDR )
	{
		Fprintf(fd, "From: %s@%s\n", SenderName, ExHomeAddress);
		Fprintf(fd, "Date: %s", rfc822time(&StartTime));
	}

	if ( MAIL_TO )
		Fprintf(fd, "To: %s@%s\n", RemUser, ExHomeAddress);

	Fprintf(fd, english("Subject: Your request for files from %s\n\n"), ExSourceAddress);

	if ( (errs = FwdNode) != NULLSTR )
	{
		register int	i;

		Fprintf
		(
			fd,
			english("\n(%s:- This request was forwarded from \"%s\""),
			POSTMASTERNAME,
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

	Fprintf(fd, english("Some or all of the files you requested from %s\n"), ExSourceAddress);
	Fprintf(fd, english("are not available. "));

	if ( mesg != NULLSTR && mesg[0] != '\0' )
		Fprintf(fd, english("The reason given was:\n\n%s\n"), CleanError(mesg));
	else
		Fprintf(fd, english("No reason was given.\n"));

	if ( (cp = GetEnv(ENV_ID)) != NULLSTR )
		Fprintf(fd, english("\nThe request ID was %s.\n"), cp);

	if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
	{
		Error(StringFmt, errs);
		free(errs);
	}
}



/*
**	Capture error for log
*/

/*VARARGS*/
void
#ifdef	ANSI_C
return_error(int err, char *fmt, ...)
{
#else	/* ANSI_C */
return_error(va_alist)
	va_dcl
{
	char *		fmt;
#endif	/* ANSI_C */
	register FileL *fp;
	va_list		vp;

	fp = Talloc0(FileL);
	*EFileList = fp;
	EFileList = &fp->next;

#	ifdef	ANSI_C
	va_start(vp, fmt);
	RetVal = err;
#	else	/* ANSI_C */
	va_start(vp);
	RetVal = va_arg(vp, int);
	fmt = va_arg(vp, char *);
#	endif	/* ANSI_C */

	fp->name = newvprintf(fmt, vp);
	fp->path = concat(english("ERROR - "), fp->name, NULLSTR);

#	ifdef	ANSI_C
	va_end(vp);
	va_start(vp, fmt);
#	endif	/* ANSI_C */

	MesgV("error", fmt, vp);

	va_end(vp);

	putc('\n', ErrorFd);
	(void)fflush(ErrorFd);
}



/*
**	Send files requested.
*/

void
sendfiles(param)
	bool		param;
{
	register FileL *list;
	register char *	errs;
	int		arg_mark;
	char		nbuf[NUMERICARGLEN];
	ExBuf		exbuf;

	Trace2(1, "sendfiles(%s)", param?"true":"false");

	FIRSTARG(&exbuf.ex_cmd) = concat(SPOOLDIR, LIBDIR, "sendfile", NULLSTR);

	errs = EmptyString;
	if ( Options != NULLSTR )
	{
		if ( strchr(Options, 'I') != NULLSTR )
			NEXTARG(&exbuf.ex_cmd) = concat("-I", SavHdrID, NULLSTR);

		if ( strchr(Options, 'S') != NULLSTR )
			errs = "n";
		if ( strchr(Options, 'C') != NULLSTR )
			errs = concat("c", errs, NULLSTR);
	}
	NEXTARG(&exbuf.ex_cmd) = concat("-", errs, "rxQ", RemUser, "@", SavHdrSource, NULLSTR);

	if ( Traceflag != 0 )
		NEXTARG(&exbuf.ex_cmd) = NumericArg(nbuf, 'T', (Ulong)Traceflag);

	list = FileList;

	for ( arg_mark = NARGS(&exbuf.ex_cmd) ; list != (FileL *)0 ; NARGS(&exbuf.ex_cmd) = arg_mark )
	{
		do
		{
			if ( list->size == 0 )
				continue;

			if ( NARGS(&exbuf.ex_cmd) >= (MAXVARARGS-3) )
				break;

			NEXTARG(&exbuf.ex_cmd) = "-N";
			NEXTARG(&exbuf.ex_cmd) = list->name;

			if ( list->inpl )
				NEXTARG(&exbuf.ex_cmd) = "-X";

			NEXTARG(&exbuf.ex_cmd) = list->path;
		}
		while
			( (list = list->next) != (FileL *)0 );

		if ( (errs = Execute(&exbuf, NULLVFUNCP, ex_exec, SYSERROR)) != NULLSTR )
		{
			if ( ExStatus & 0377 )
				RetVal = ExStatus;
			else
				RetVal = EX_SOFTWARE;
			Error(StringFmt, errs);
			free(errs);
		}
	}
}



/*
**	Send list of files.
*/

void
sendlist(verbose)
	bool		verbose;
{
	register FileL *list;
	register FILE *	fd;
	register char *	cp;
	char		nbuf[NUMERICARGLEN];
	ExBuf		exbuf;

	Trace2(1, "sendlist(%s)", verbose?"true":"false");

	Verbose = verbose;

	FIRSTARG(&exbuf.ex_cmd) = concat(SPOOLDIR, LIBDIR, "sendfile", NULLSTR);

	cp = EmptyString;
	if ( Options != NULLSTR )
	{
		if ( strchr(Options, 'I') != NULLSTR )
			NEXTARG(&exbuf.ex_cmd) = concat("-I", SavHdrID, NULLSTR);

		if ( strchr(Options, 'S') != NULLSTR )
			cp = "n";
		if ( strchr(Options, 'C') != NULLSTR )
			cp = concat("c", cp, NULLSTR);
	}
	NEXTARG(&exbuf.ex_cmd) = concat("-", cp, "rxQ", RemUser, "@", SavHdrSource, NULLSTR);

	if ( Traceflag != 0 )
		NEXTARG(&exbuf.ex_cmd) = NumericArg(nbuf, 'T', (Ulong)Traceflag);
	NEXTARG(&exbuf.ex_cmd) = "-NPublicFileList";

	fd = (FILE *)Execute(&exbuf, NULLVFUNCP, ex_pipe, SYSERROR);

	Fprintf
	(
		fd, english("Results of %slist request at %s on %s\n"),
		(verbose ? "verbose " : EmptyString), ExHomeAddress, ctime(&Time)
	);

	if ( (cp = FwdNode) != NULLSTR )
	{
		char *	cp2;
		int	i;

		Fprintf
		(
			fd,
			english("\n(%s:- This request was forwarded from \"%s\""),
			POSTMASTERNAME,
			cp
		);

		for ( i = 0 ; i < 4 && (cp2 = GetEnvNext(ENV_FWD, cp)) != NULLSTR ; i++ )
		{
			free(cp);
			Fprintf(fd, ", \"%s\"", cp2);
			cp = cp2;
		}

		Fprintf(fd, ".)\n\n");

		free(cp);
		FwdNode = GetEnv(ENV_FWD);
	}

	if ( Options != NULLSTR && strchr(Options, 'R') != NULLSTR )
	{
		if ( No_lr )
			Fprintf(fd, english("\n(Directory recursion disallowed.)\n\n"));
		else
			Recurse = true;
	}

	Fprintf
	(
		fd, "%-9s %10s %-*s %s\n\n",
		english("Type"), english("Size"), DATESTRLEN-1, english("    Date"), english("Name")
	);

	for ( list = FileList ; list != (FileL *)0 ; list = list->next )
	{
		ListSize = 2*LINELEN;	/* To count header */
		vlist(list->path, fd, 0);
		list->size = ListSize;
	}

	if ( Truncated )
	{
		Fprintf(fd, english("\nListing truncated: too large.\n"));
		Fprintf(fd, english("Fetch top-level file \"ls-lRt.Z\" instead."));
	}

	if ( (cp = ExClose(&exbuf, fd)) != NULLSTR )
	{
		Error(StringFmt, cp);
		free(cp);
	}
}



/*
**	Become network id for local mail delivery.
*/

void
set_user(args)
	VarArgs *	args;
{
	SetUser(NetUid, NetGid);
}



/*
**	Output verbose list of files.
*/

void
vlist(file, fd, level)
	register char *		file;
	register FILE *		fd;
	int			level;
{
	register DIR *		dd;
	register struct direct *dp;
	register char *		cp;
	register int		i;
	register bool		isdir;
	char			datebuf[DATESTRLEN];
	struct stat		statb;

	if ( MaxListSize && ListSize > MaxListSize )
	{
		Truncated = true;
		return;
	}

	if ( stat(file, &statb) == SYSERROR )
	{
		Fprintf(fd, english("%s: nonexistent\n"), file);
		return;
	}

	if ( access(file, 4) == SYSERROR )
		return;		/* Don't list inaccessible files */

	if ( (statb.st_mode & S_IFMT) == S_IFDIR )
		isdir = true;
	else
		isdir = false;

	if ( !Verbose && level && !isdir )
		flist(file, fd, isdir);
	else
	{
		/*
		**	Note: we expressly do not tell owner, link count, permissions ..
		*/

		flist(NULLSTR, fd, false);

		if ( isdir )
			cp = english("directory");
		else
			cp = english("file");

		Fprintf(fd, "%-9s %10lu %s ", cp, (PUlong)statb.st_size, DateString((Time_t)statb.st_mtime, datebuf));

#		define	LEN	34

		if ( (i = strlen(file)) > (MaxListCols-LEN) && MaxListCols > 0 )
		{
			Fprintf(fd, "...%s\n", file+i-(MaxListCols-LEN-3));
			i = MaxListCols-LEN;
		}
		else
			Fprintf(fd, "%s\n", file);

		ListSize += i+LEN+1;	/* +1 for newline */
	}

	if ( (!Recurse && level) || !isdir )
		return;

	if ( (dd = opendir(file)) == NULL )
	{
		flist(NULLSTR, fd, false);
		Fprintf(fd, english("Contents unavailable.\n\n"));
		return;
	}

	for ( i = 0 ; (dp = readdir(dd)) != NULL ; )
	{
		if ( dp->d_name[0] == '.' )
			continue;

		if ( (Verbose || Recurse) && level < 5 )
		{
			if ( i == 0 && !Verbose )
			{
				flist(NULLSTR, fd, false);
				Fprintf(fd, english("Contents:\n"));
			}
			vlist((cp = concat(file, Slash, dp->d_name, NULLSTR)), fd, level+1);
			free(cp);
		}
		else
		{
			if ( i == 0 )
			{
				flist(NULLSTR, fd, false);
				Fprintf(fd, english("Contents:\n"));
			}
			flist(dp->d_name, fd, false);
		}

		i++;
	}

	if ( i )
	{
		flist(NULLSTR, fd, false);
		(void)putc('\n', fd);
	}

	closedir(dd);
}



/*
**	Write out stats record in the same format as that used in STATSFILE.
*/

void
writestats()
{
	register FileL *	list;
	register FILE *		fd;
	struct stat		statb;

	Trace2(1, "writestats() %s", FileList->path);

	if ( FILESERVERLOG == NULLSTR )
		return;

	if
	(
		stat(FILESERVERLOG, &statb) == SYSERROR
		||
		(fd = fopen(FILESERVERLOG, "a")) == NULL
	)
	{
		Trace3(1, "Can't open %s: %s", FILESERVERLOG, ErrnoStr(errno));
		return;
	}

#	if	AUTO_LOCKING != 1
	if ( Lock(FILESERVERLOG, fileno(fd), for_writing) == SYSERROR )
	{
		(void)fclose(fd);
		return;
	}
#	endif	/* AUTO_LOCKING != 1 */

	(void)fseek(fd, 0L, 2);

	if ( Service == NULLSTR )
		Service = Unknown;
	if ( RemUser == NULLSTR )
		RemUser = Unknown;

	for ( list = FileList ; list != (FileL *)0 ; list = list->next )
		Fprintf
		(
			fd,
			"%c%c%lu%c%s%c%lu%c%s@%s%c%s%c%s%c",
			ST_F_S_LOG, ST_RE_SEP,
			(PUlong)Time, ST_RE_SEP,
			SavHdrID, ST_RE_SEP,
			(PUlong)list->size, ST_RE_SEP,
			RemUser, ExSourceAddress, ST_RE_SEP,
			Service, ST_RE_SEP,
			list->path, ST_SEP
		);

#	if	AUTO_LOCKING != 1
	(void)fflush(fd);
	UnLock(fileno(fd));
#	endif	/* AUTO_LOCKING != 1 */

	(void)fclose(fd);
}
