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
**	Get a user's files spooled from network.
**
**	SETUID ==> ROOT
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	RECOVER
#define	SIGNALS
#define	STDIO
#define	ERRNO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"ftheader.h"
#include	"forward.h"
#include	"handlers.h"
#include	"header.h"
#include	"link.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sysexits.h"

#include	<ndir.h>


/*
**	Parameters set from arguments.
*/

bool	AllUsers;		/* Select all messages */
bool	BaseName;		/* Use `basename file` when creating */
bool	CheckCRC;		/* Check CRC of data (if present) */
bool	Delete;			/* Delete the lot */
bool	First;			/* Only get first file with this same name */
bool	Forwarding;		/* Get forwarding info */
bool	Keep;			/* Don't delete any files got */
bool	List;			/* List all files found and exit */
bool	Mkdir;			/* Create directories as needed */
char *	Name	= sccsid;	/* Program invoked name */
bool	NoSMdate;		/* Don't set modify date of retrieved files */
Time_t	Older;			/* Only list details of messages older than this */
bool	Reverse;		/* Reverse order of sort */
bool	ReportFiles;		/* Just report number of files spooled */
char *	Sender;			/* Restrict selection to messages from ``sender'' */
bool	Silent;			/* Don't prompt for user response, assume "yes" */
char *	Source;			/* Restrict selection to messages from ``source'' */
bool	Stdout;			/* Copy named file to stdout */
int	Traceflag;		/* Global tracing control */
char *	User;			/* User's name for files to be retrieved (root only) */
bool	Verbose;		/* Lots of verbiage */
bool	Yes;			/* Assume answer "yes" to all queries */

AFuncv	getAll _FA_((PassVal, Pointer));		/* And set "sort-by-user" */
AFuncv	getDelete _FA_((PassVal, Pointer));		/* Accept if string is "elete" */
AFuncv	getFile _FA_((PassVal, Pointer));		/* Set specific file name */
AFuncv	getOlder _FA_((PassVal, Pointer));		/* Files older than "days" */
AFuncv	getSBdelivery _FA_((PassVal, Pointer));		/* Sort by delivery time */
AFuncv	getSBmodtime _FA_((PassVal, Pointer));		/* Sort by modify time */
AFuncv	getSBname _FA_((PassVal, Pointer));		/* Sort by name */
AFuncv	getSBsendtime _FA_((PassVal, Pointer));		/* Sort by transmit time */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_string(d, 0, getDelete, english("elete"), OPTARG|OPTVAL),
	Arg_bool(1, &First, 0),
	Arg_bool(a, &AllUsers, getAll),
	Arg_bool(b, &BaseName, 0),
	Arg_bool(c, &CheckCRC, 0),
	Arg_bool(d, &Mkdir, 0),
	Arg_bool(e, 0, getSBdelivery),
	Arg_bool(f, &Forwarding, 0),
	Arg_bool(k, &Keep, 0),
	Arg_bool(l, &List, 0),
	Arg_bool(m, 0, getSBmodtime),
	Arg_bool(n, 0, getSBname),
	Arg_bool(o, &Stdout, 0),
	Arg_bool(r, &Reverse, 0),
	Arg_bool(s, &Silent, 0),
	Arg_bool(t, 0, getSBsendtime),
	Arg_bool(u, &NoSMdate, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_bool(x, &ReportFiles, 0),
	Arg_bool(y, &Yes, 0),
	Arg_string(F, &Source, 0, english("source"), OPTARG),
	Arg_long(O, 0, getOlder, english("days"), OPTARG),
	Arg_string(S, &Sender, 0, english("sender"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_string(U, &User, 0, english("user"), OPTARG),
	Arg_noflag(0, getFile, english("filename"), OPTARG|MANY),
	Arg_end
};

/*
**	Default sort order is by modify time.
*/

typedef enum
{
	sb_null, sb_modify, sb_delivery, sb_name, sb_to, sb_sent
}
	Sort_t;

/*
**	Deleted list.
*/

typedef struct Del *	Del_p;

typedef struct Del
{
	Del_p	dl_next;
	int	dl_number;
}
	Del;

Del_p	Del_freelist;		/* Cache of Dels */
Del_p	Del_list;		/* Current message deleted list */

/*
**	File names list.
*/

typedef struct FN_list *FN_p;

typedef struct FN_list
{
	FN_p	fn_next;
	char *	fn_name;
}
	FN_el;

/*
**	Message list.
*/

typedef struct M_List *ML_p;

typedef struct M_List
{
	ML_p	m_next;
	Time_t	m_sent;		/* Transmit time for message */
	Time_t	m_time;		/* Delivery time for message */
	Time_t	m_timeout;	/* Timeout time for message */
	Ulong	m_length;	/* Length of data in message */
	Ulong	m_tlength;	/* Length of data truncated from beginning of message */
	CmdHead	m_parts;	/* List of message files */
	FthFD_p	m_files;	/* List of files in message */
	char *	m_name;		/* Command file name of message */
	char *	m_from;		/* Sender */
	char *	m_source;	/* Source */
	char *	m_to;		/* Recipients */
	char *	m_route;	/* Route */
	char *	m_error;	/* Possible RETURN reason */
	char *	m_trunc;	/* Possible truncation */
	char *	m_id;		/* Possible original request ID */
	char *	m_deleted;	/* Boolean array for newly deleted files */
	char *	m_origdel;	/* Boolean array for previously deleted files */
	int	m_nfiles;	/* Number of files in list */
	int	m_flags;
}
	MesList;

ML_p	MessageList;		/* List of messages found for user */
int	NMessages;		/* Number of messages found */

#define	DELETE(mp,i)	mp->m_deleted[i/8] |= (1<<(i%8))
#define	DELETED(mp,i)	(mp->m_deleted[i/8] & (1<<(i%8)))

#define	ODELETE(mp,i)	mp->m_origdel[i/8] |= (1<<(i%8))
#define	ODELETED(mp,i)	(mp->m_origdel[i/8] & (1<<(i%8)))

#define	MSG_RETURNED	001
#define	MSG_TRUNCATED	002
#define	MSG_CRCERROR	004
#define	MSG_MULTIPART	010

typedef struct
{
	ML_p	fl_mp;		/* Pointer to message containing file */
	FthFD_p	fl_fp;		/* Pointer to file descriptor */
	int	fl_in;		/* Index of file in message */
}
	*FL_p;

FL_p	FilesList;		/* Sorted array of file descriptors */
int	NFiles;			/* Total number of files found */

/*
**	File getting commands:
*/

typedef enum
{
	r_yes, r_no, r_delete, r_rename, r_basename, r_append, r_cd, r_mkdir, r_quit, r_keep
}
	Cmdtyp;

typedef struct
{
	char *	c_name;
	Cmdtyp	c_type;
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
	{english("append"),	r_append},
	{english("basename"),	r_basename},
	{english("cd"),		r_cd},
	{english("delete"),	r_delete},
	{english("keep"),	r_keep},
	{english("mkdir"),	r_mkdir},
	{english("no"),		r_no},
	{english("quit"),	r_quit},
	{english("rename"),	r_rename},
	{english("x"),		r_quit},
	{english("yes"),	r_yes},
};

#define	CMDZ	(sizeof(Command))
#define	NCMDS	((sizeof Cmds)/CMDZ)

#define	MAXCOMLEN	8
#define	MINCOMLEN	1

/*
**	Miscellaneous
*/

bool	Append;			/* Append to existing file of same name */
int	ComLen;			/* Length of user's command */
char *	CmdFileName;		/* Template for command file */
char *	CurrentFile;		/* Should be cleaned up on error/INTR */
CmdHead	DataCmds;		/* Data commands from command file */
Addr *	ForwAddr;		/* Address for forwarding files */
char *	ForwDir;		/* Directory for spooling files */
FN_p	FileNames;		/* Particular files to be retrieved */
char *	Files_Dir;		/* Where commands about files are spooled */
int	FilesDirLen;		/* Length of above */
int	FilesMoved;		/* Count of files delivered to user */
Ulong	FtpHdrEnd;		/* End of data in message file containing FTP header */
char *	FtpHdrFile;		/* Message file containing FTP header */
char *	HdrFile;		/* Message file containing header */
char *	HomeAddress;		/* Typeless address of this node */
bool	Isatty;			/* Fd 0 is a terminal */
Ulong	MesgLength;		/* Length of data in current message */
Time_t	MesgTimeout;		/* Timeout time for current message */
char *	NextWord;		/* From stdin */
char	NoPerm[]	= english("No permission.");
bool	Returned;		/* True if message has been returned */
bool	Restricted;		/* Restricted user */
Sort_t	Sortflag;		/* Sort order */
long	Tb;			/* Size of message */
Passwd	To;			/* Details of invoker */
Ulong	Tt;			/* Travel time for message */
char	UidStr[UID_LENGTH+1];	/* Ascii value of uid */

#define	FILE_EXISTS	01
#define	FILE_LINKS	02
#define	FILE_NOTREG	04
#define	FILE_ISDIR	010

#define	MAXNAMELEN	132
#define	MIN_SPEED	10		/* Don't show speeds slower than this */
#define	Fprintf		(void)fprintf
#define	Fflush		(void)fflush

bool	checkperm _FA_((char *, char *, Time_t *, int *));
int	compare _FA_((const void *, const void *));
bool	copyfile _FA_((FL_p, bool));
int	countfiles _FA_((void));
void	deletefile _FA_((FL_p));
bool	findfiles _FA_((void));
void	finish _FA_((int));
void	FreeDeleted _FA_((void));
void	GetDeleted _FA_((ML_p));
void	getfilename _FA_((void));
char *	getname _FA_((char *, char *));
void	handlebad _FA_((char *, char *));
void	ignore_signals _FA_((VarArgs *));
void	interact _FA_((void));
void	movefiles _FA_((void));
void	NewDeleted _FA_((Ulong));
bool	printroute _FA_((char *, Ulong, char *));
char *	readmessage _FA_((char *));
int	readword _FA_((char *, int, bool));
void	removemessages _FA_((void));
Cmdtyp	response _FA_((bool));
int	SBdelivery _FA_((char *, char *));
int	SBmodify _FA_((char *, char *));
int	SBname _FA_((char *, char *));
int	SBsend _FA_((char *, char *));
int	SBto _FA_((char *, char *));
void	set_forward _FA_((void));
void	sortfiles _FA_((void));
void	trunc_length _FA_((ML_p));
void	writeDeleted _FA_((ML_p));

extern SigFunc	sigcatch;



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register char *	cp;
	int		omask;

	omask = umask(0);	/* Set by InitParams() */

	(void)alarm((unsigned)0);
	InitParams();

	GetNetUid();
	GetInvoker(&To);

	(void)umask(omask);	/* Restore user's umask */

	DoArgs(argc, argv, Usage);

	if ( Silent && !Yes && !Delete && !Stdout && !ReportFiles )
		Error(english("Do what silently?"));

	if
	(
		(Stdout && Yes)
		||
		(List && (Delete || Silent || Yes))
		||
		(AllUsers && User != NULLSTR)
	)
		Error(english("Ambiguous arguments."));

	if ( User != NULLSTR )
	{
		if ( !(To.P_flags & P_SU) )
			Error(NoPerm);

		if ( !GetUid(&To, User) )
			Error(english("Passwd error for user \"%s\": %s"), User, To.P_error);
	}

	if ( AllUsers && !(To.P_flags & P_SU) )
		Error(english("no permission to use \"-a\" flag"));

	if ( !(To.P_flags & P_CANRECV) )
		Restricted = true;

	if ( !ReadRoute(NOSUMCHECK) )
		Error(english("No routefile."));

	if ( geteuid() != 0 )
		Error(english("Must be setuid to root."));

	if ( Forwarding )
	{
		set_forward();
		exit(EX_OK);
	}

	if
	(
		((cp = Source) != NULLSTR && Sender == NULLSTR)
		||
		(Source == NULLSTR && (cp = Sender) != NULLSTR)
	)
	{
		register char *	cp2;

		if ( (cp2 = strchr(cp, '@')) != NULLSTR )
		{
			Sender = cp;
			*cp2++ = '\0';
			Source = cp2;
		}	
	}

	if ( Source != NULLSTR )
		Source = TypedAddress(StringToAddr(Source, NO_STA_MAP));

	HomeAddress = StripTypes(HomeName);
	Files_Dir = concat(SPOOLDIR, FILESDIR, NULLSTR);
	FilesDirLen = strlen(Files_Dir);
	CmdFileName = strcpy(Malloc(FilesDirLen+UNIQUE_NAME_LENGTH+1), Files_Dir);

	(void)EncodeNum(UidStr, (Ulong)To.P_uid, 3);

	Trace2(1, "User id \"%s\"", UidStr);

	InitCmds(&DataCmds);

	if ( !findfiles() )
	{
		register Forward *	fp;

		if ( Silent || ReportFiles || AllUsers )
			exit(EX_OK);

		if ( (fp = GetForward(FTPHANDLER, To.P_name)) != (Forward *)0 )
			Fprintf(stderr, english("Your files are being forwarded to %s\n"), fp->sub_addr);
		else
			Fprintf(stderr, english("No files found.\n"));

		exit(EX_NOINPUT);
	}

	if ( ReportFiles && FileNames != (FN_p)0 )
	{
		sortfiles();
		NFiles = countfiles();
	}

	if ( !Silent && !Stdout )
	{
		setbuf(stdout, SObuf);
		Fprintf(stdout, english("%d file%s spooled.\n"), NFiles, NFiles==1?EmptyString:english("s"));
	}

	if ( ReportFiles )
		exit(NFiles>=127 ? 127 : NFiles);

	sortfiles();

	if ( !Silent && !Stdout )
	{
		if ( (SigFunc *)signal(SIGINT, SIG_IGN) != SIG_IGN )
			(void)signal(SIGINT, sigcatch);
		Isatty = (bool)isatty(0);

		interact();
	}
	else
		movefiles();

	finish(EX_OK);
	return 0;
}



/*
**	Check write permissions on file or in parent directory.
**	Return true if write allowed.
*/

bool
checkperm(file, source, mtime, flagsp)
	char *		file;
	char *		source;
	Time_t *	mtime;
	int *		flagsp;
{
	register char *	cp;
	register char *	ep;
	bool		write_ok;
	struct stat	statb;

	if
	(
		Restricted
		&&
		(LOCALSEND == 0 || strccmp(source, HomeAddress) != STREQUAL)
		&&
		(LOCAL_NODES == NULLSTR || !InList(CanAddrMatch, source, LOCAL_NODES, NULLSTR))
	)
	{
		Fprintf(stdout, english("\n\t(%s is not a legal source)"), source);
		write_ok = false;
	}
	else
		write_ok = true;

	if ( stat(file, &statb) != SYSERROR )
	{
		*flagsp = FILE_EXISTS;
		*mtime = statb.st_mtime;

		if ( statb.st_nlink > 1 )
			*flagsp |= FILE_LINKS;

		if ( (statb.st_mode & S_IFMT) != S_IFREG )
		{
			*flagsp |= FILE_NOTREG;
			if ( (statb.st_mode & S_IFMT) == S_IFDIR )
				*flagsp |= FILE_ISDIR;
		}

		if
		(
			(statb.st_mode & S_IFMT) != S_IFREG
			||
			access(file, 2) == SYSERROR
		)
			write_ok = false;

		return write_ok;
	}

	*flagsp = 0;

#	if	BSD4 >= 2 || SYSVREL >= 4
	if ( lstat(file, &statb) != SYSERROR )
		return false;
#	endif	/* BSD4 >= 2 || SYSVREL >= 4 */

	/* File doesn't exist - check directory */

	if ( Mkdir )
	{
		register char *	xp;

		for ( xp = NULLSTR ;; )
		{
			if ( (ep = strrchr(file, '/')) != NULLSTR )
			{
				if ( ep == file )
					cp = "/";
				else
					cp = file;
				*ep = '\0';
			}
			else
				cp = ".";
			if ( xp != NULLSTR )
				*xp = '/';
			Trace3(2, "checkperm file=%s cp=%s", file, cp);
			if ( stat(cp, &statb) != SYSERROR )
			{
				if ( (statb.st_mode & S_IFMT) != S_IFDIR
					|| access(cp, 2) == SYSERROR )
				{
					if ( ep != NULLSTR )
						*ep = '/';
					return false;
				}
				if ( ep != NULLSTR )
					*ep = '/';
				break;
			}
			if ( cp != file )
			{
				if ( ep != NULLSTR )
					*ep = '/';
				return false;
			}
			xp = ep;
		}
	}

	if ( (ep = strrchr(file, '/')) != NULLSTR )
	{
		if ( ep == file )
			cp = "/";
		else
			cp = file;
		*ep = '\0';
	}
	else
		cp = ".";

	if
	(
		write_ok
		&&
		(
			stat(cp, &statb) == SYSERROR
			||
			(statb.st_mode & S_IFMT) != S_IFDIR
			||
			access(cp, 2) == SYSERROR
		)
		&&
		(
			errno != ENOENT
			||
			ep == NULLSTR
			||
			!Mkdir
			||
			(*ep = '/', !MkDirs(cp, To.P_uid, To.P_gid))
		)
	)
		write_ok = false;

	if ( ep != NULLSTR )
		*ep = '/';
	
	return write_ok;
}



/*
**	Match name in FthUsers.
*/

bool
checkusers(name)
	char *			name;
{
	register FthUlist *	up;

	Trace2(1, "checkusers(%s)", name);

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
		if ( strcmp(name, up->u_name) == STREQUAL )
			return true;

	return false;
}



/*
**	Compare two commands
*/

int
#ifdef	ANSI_C
compare(const void *cmdp1, const void *cmdp2)
#else	/* ANSI_C */
compare(cmdp1, cmdp2)
	char *	cmdp1;
	char *	cmdp2;
#endif	/* ANSI_C */
{
	return strncmp(((Cmd_p)cmdp1)->c_name, ((Cmd_p)cmdp2)->c_name, ComLen);
}



/*
**	Copy file from message to file-system.
*/

bool
copyfile(flp, exists)
	register FL_p		flp;
	bool			exists;
{
	register FthFD_p	fp;
	register Ulong		l;
	register Ulong		e;
	register int		ofd;
	vFuncp			errf;
	char			dateb[DATESTRLEN];
	char			timeb[TIMESTRLEN];

	for
	(
		l = 0, fp = flp->fl_mp->m_files ;
		fp != flp->fl_fp && fp != (FthFD_p)0 ;
		fp = fp->f_next
	)
		l += fp->f_length;

	DODEBUG(if(fp!=flp->fl_fp)Fatal1("bad copyfile"));

	Trace4(1, "copyfile \"%s\" size %ld mode 0%o", fp->f_name, (long)fp->f_length, fp->f_mode);

	if ( Yes || !Isatty )
		errf = (vFuncp)Error;
	else
		errf = (vFuncp)Warn;

	e = l + fp->f_length;

	if ( flp->fl_mp->m_flags & MSG_TRUNCATED )
	{
		if ( l < flp->fl_mp->m_tlength )
		{
			l = 0;

			if ( !Stdout && exists )
			{
				(*errf)(english("File truncated -- not copied (try rename)"));
				return false;
			}

			if ( e <= flp->fl_mp->m_tlength )
			{
				(*errf)(english("File not in message -- not copied"));
				return false;
			}
		}
		else
			l -= flp->fl_mp->m_tlength;

		e -= flp->fl_mp->m_tlength;
	}

	if ( e > flp->fl_mp->m_length )
	{
		if ( (flp->fl_mp->m_flags & (MSG_RETURNED|MSG_MULTIPART)) == (MSG_RETURNED|MSG_MULTIPART) )
		{
			(*errf)(english("File not in multi-part message -- not copied"));
			return false;
		}

		Fatal1(english("couldn't find message part for file"));
	}

	if ( !CheckData(&flp->fl_mp->m_parts, (Ulong)0, flp->fl_mp->m_length, (Crc_t *)0) )
	{
		(*errf)(english("Message unreadable -- not copied"));
		return false;
	}

	if ( Stdout )
		ofd = 1;
	else
	{
		if ( BaseName )
		{
			char *	cp;

			if ( (cp = strrchr(fp->f_name, '/')) != NULLSTR )
				fp->f_name = cp + 1;
		}

		if ( Append && exists )
		{
			while ( (ofd = open(fp->f_name, O_WRITE)) == SYSERROR )
				Syserror(CouldNot, OpenStr, fp->f_name);
			(void)lseek(ofd, (long)0, 2);
		}
		else
		{
			while ( (ofd = creat(fp->f_name, (fp->f_mode)&FTH_MODES)) == SYSERROR )
				if ( !Mkdir || !MkDirs(fp->f_name, To.P_uid, To.P_gid) )
					Syserror(CouldNot, CreateStr, fp->f_name);
			CurrentFile = fp->f_name;
		}

		if ( !exists )
		{
			int		gid = To.P_gid;
#			if	BSD4 > 1 || V8 == 1
			struct stat	statb;

			if ( fstat(ofd, &statb) != SYSERROR && statb.st_gid != 0 )
				gid = statb.st_gid;
#			endif	/* BSD4 > 1 || V8 == 1 */

			if ( chown(fp->f_name, To.P_uid, gid) == SYSERROR )
			{
				(void)SysWarn(CouldNot, "chown", fp->f_name);
				if ( chmod(fp->f_name, ((fp->f_mode)&FTH_MODES)|0444) == SYSERROR )
					(void)SysWarn(CouldNot, "chmod", fp->f_name);
				else
					(void)SysWarn("%s: mode %o", fp->f_name, ((fp->f_mode)&FTH_MODES)|0444);
			}
		}
	}

	if ( !CollectData(&flp->fl_mp->m_parts, l, e, ofd, fp->f_name) )
	{
		if ( !Stdout && !exists )
			(void)unlink(fp->f_name);
		return false;
	}

	CurrentFile = NULLSTR;

	if ( Stdout )
		return true;

	(void)close(ofd);

	if ( !NoSMdate )
	{
		if ( fp->f_time <= Time )
			SMdate(fp->f_name, fp->f_time);
		else
			Warn
			(
				"Modify time of \"%s\" was %s in future [%s]",
				fp->f_name,
				TimeString(fp->f_time - Time, timeb),
				DateString(fp->f_time, dateb)
			);
	}

	return true;
}



/*
**	Count matches of selected files against those found.
*/

int
countfiles()
{
	register FL_p		flp;
	register FN_p		fnp;
	register int		i;
	register int		j;

	for ( flp = FilesList, i = NFiles, j = 0 ; --i >= 0 ; flp++ )
		for ( fnp = FileNames ; fnp != (FN_p)0 ; fnp = fnp->fn_next )
			if ( strcmp(flp->fl_fp->f_name, fnp->fn_name) == STREQUAL )
			{
				j++;
				break;
			}

	return j;
}



/*
**	Mark a file deleted from message.
*/

void
deletefile(flp)
	FL_p	flp;
{
	if ( Keep )
		return;
	Trace3(2, "deletefile \"%s\" [%d]", flp->fl_fp->f_name, flp->fl_in);
	DELETE(flp->fl_mp, flp->fl_in);
	FilesMoved++;
}



/*
**	Find messages spooled for this user and extract list of filenames.
*/

bool
findfiles()
{
	register char *			np;
	register ML_p			mp;
	register struct direct *	direp;
	register DIR *			dirp;
	char *				errs;

	Trace2(1, "findfiles() in \"%s\"", Files_Dir);

	if ( (dirp = opendir(Files_Dir)) == NULL )
	{
		Syserror(CouldNot, ReadStr, Files_Dir);
		exit(EX_OSFILE);
	}

	Recover(ert_return);

	np = &CmdFileName[FilesDirLen];

	while ( (direp = readdir(dirp)) != NULL )
	{
		Trace2(4, "Found file \"%s\"", direp->d_name);

		if ( strlen(direp->d_name) != UNIQUE_NAME_LENGTH )
			continue;

		Trace3(3, "Found uid \"%.*s\"", UID_LENGTH, &direp->d_name[UID_POSN]);

		if
		(
			!AllUsers
			&&
			strncmp(UidStr, &direp->d_name[UID_POSN], UID_LENGTH) != STREQUAL
		)
			continue;

		Trace2(2, "Found spooled message \"%s\"", direp->d_name);

		(void)strcpy(np, direp->d_name);

		FreeDeleted();
		MesgLength = 0;
		MesgTimeout = 0;

		if ( (errs = readmessage(CmdFileName)) != NULLSTR )
		{
			handlebad(CmdFileName, errs);
			continue;
		}

		Trace3(1, "Found %d files in message to \"%s\"", NFthFiles, FthTo);

		if 
		( 
			(!AllUsers && !Returned && !InFthTo(To.P_name))
			||
			(Older != (Time_t)0 && RdFileTime >= Older)
			||
			(Sender != NULLSTR && strcmp(Sender, FthFrom) != STREQUAL)
			||
			(Source != NULLSTR && !AddressMatch(HdrSource, Source))
		)
		{
			FreeFthFiles();
			continue;
		}

		mp = Talloc(MesList);
		mp->m_next = MessageList;
		MessageList = mp;
		NMessages++;

		mp->m_files = FthFiles;
		mp->m_flags = 0;
		mp->m_from = newstr(FthFrom);
		mp->m_id = GetEnv(ENV_ID);
		mp->m_length = MesgLength - (HdrLength + FthLength);
		mp->m_name = newstr(np);
		mp->m_nfiles = NFthFiles;
		mp->m_source = StripTypes(HdrSource);
		mp->m_time = RdFileTime;
		mp->m_sent = RdFileTime - HdrTt;
		mp->m_timeout = MesgTimeout;
		mp->m_tlength = 0;
		mp->m_to = newstr(FthTo);

		InitCmds(&mp->m_parts);
		MoveCmds(&DataCmds, &mp->m_parts);

		if ( Returned )
		{
			mp->m_flags |= MSG_RETURNED;
			if ( HdrPart != NULLSTR && HdrPart[0] != '\0' )
				mp->m_flags |= MSG_MULTIPART;
			mp->m_error = GetEnv(ENV_ERR);
			mp->m_route = GetEnv(ENV_ROUTE);
			if ( (mp->m_trunc = GetEnv(ENV_TRUNC)) != NULLSTR )
				trunc_length(mp);
		}
		else
		{
			mp->m_error = NULLSTR;
			mp->m_route = newstr(HdrRoute);
			mp->m_trunc = NULLSTR;

			if ( HdrPart != NULLSTR && HdrPart[0] != '\0' )
				mp->m_flags |= MSG_MULTIPART;	/* CRC/travel stats lost */
			else
			if
			(
				CheckCRC
				&&
				(FthType[0] & FTH_DATACRC)
				&&
				!CheckData(&mp->m_parts, (Ulong)0, mp->m_length, &DataCrc)
			)
				mp->m_flags |= MSG_CRCERROR;
		}

		FthFiles = (FthFD_p)0;
		NFiles += NFthFiles;
		NFthFiles = 0;

		GetDeleted(mp);
		Trace2(2, "Total %d files", NFiles);
	}

	closedir(dirp);

	Recover(ert_finish);

	return NMessages > 0 ? true : false;
}



/*
**	Called from the errors routines to cleanup
*/

void
finish(error)
	int	error;
{
/*	if ( CurrentFile != NULLSTR )		NOT SURE ABOUT WISDOM OF THIS --
**	{
**		Trace2(3, "unlinking \"%s\"", CurrentFile);
**		(void)unlink(CurrentFile);	SOMETHING MAY BE BETTER THAN NOTHING!
**	}
*/
	if ( FilesMoved )
	{
		FilesMoved = 0;	/* Avoid recursion! */
		removemessages();
	}

	(void)exit(error);
}



/*
**	Free deleted list.
*/

void
FreeDeleted()
{
	register Del_p	dp;

	Trace1(3, "FreeDeleted()");

	while ( (dp = Del_list) != (Del_p)0 )
	{
		Del_list = dp->dl_next;
		dp->dl_next = Del_freelist;
		Del_freelist = dp;
	}
}



/*
**	Process a command from a commands file.
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
	case deleted_cmd:
		NewDeleted(val.cv_number);
		return true;

	case end_cmd:
		fileend = val.cv_number;
		MesgLength += fileend - filestart;
		break;

	case file_cmd:
		FtpHdrFile = HdrFile;
		FtpHdrEnd = fileend;
		HdrFile = AddCmd(&DataCmds, type, val);
		return true;

	case start_cmd:
		filestart = val.cv_number;
		break;

	case timeout_cmd:
		MesgTimeout = RdFileTime + val.cv_number;
		return true;

	case unlink_cmd:
		break;

	default:
		return true;
	}

	(void)AddCmd(&DataCmds, type, val);

	return true;
}



/*
**	Set AllUsers and sort by user.
*/

/*ARGSUSED*/
AFuncv
getAll(val, arg)
	PassVal	val;
	Pointer	arg;
{
	if ( Sortflag == sb_null )
		Sortflag = sb_to;
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
		return ACCEPTARG;
	}

	return REJECTARG;
}



/*
**	Set up deleted files boolean arrays (from Del_list.)
*/

void
GetDeleted(mp)
	register ML_p	mp;
{
	register Del_p	dp;
	register int	n;

	Trace2(1, "GetDeleted(%s)", mp->m_name);

	n = (mp->m_nfiles+7)/8;

	mp->m_deleted = Malloc0(n);
	mp->m_origdel = Malloc0(n);

	for ( dp = Del_list ; dp != (Del_p)0 ; dp = dp->dl_next )
	{
		if ( (n = dp->dl_number) >= mp->m_nfiles )
			Fatal3(english("bad deleted number (%d) in commandfile \"%s\""), n, mp->m_name);

		if ( DELETED(mp, n) )
		{
			Trace3(3, "ignore DUP delete file %d from %s", n, mp->m_name);
		}
		else
		{
			Trace3(3, "delete file %d from %s", n, mp->m_name);
			DELETE(mp, n);
			ODELETE(mp, n);
			NFiles--;
		}
	}

	Trace3(2, "GetDeleted(%s) => %d files", mp->m_name, NFiles);
}



/*
**	Set specific file name
*/

/*ARGSUSED*/
AFuncv
getFile(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register FN_p	fnp;
	register Addr *	ap;

	if ( Forwarding )
	{
		if ( val.p[0] == '/' )
		{
			if ( ForwDir != NULLSTR )
				return (AFuncv)english("only one spooling directory specifiable");

			ForwDir = val.p;
			return ACCEPTARG;
		}

		if ( (ap = AddrFromUser(val.p)) == (Addr *)0 )
			return (AFuncv)english("invalid address");

		AddAddr(&ForwAddr, ap);

		return ACCEPTARG;
	}

	fnp = Talloc(FN_el);

	fnp->fn_next = FileNames;
	FileNames = fnp;

	fnp->fn_name = val.p;

	if ( Sortflag == sb_null )
		Sortflag = sb_name;

	return ACCEPTARG;
}



/*
**	Get days.
*/

/*ARGSUSED*/
AFuncv
getOlder(val, arg)
	PassVal	val;
	Pointer	arg;
{
	Older = Time - (Time_t)(val.l * DAY);
	return ACCEPTARG;
}



/*
**	Sort by delivery time.
*/

/*ARGSUSED*/
AFuncv
getSBdelivery(val, arg)
	PassVal	val;
	Pointer	arg;
{
	Sortflag = sb_delivery;
	return ACCEPTARG;
}



/*
**	Sort by modify time.
*/

/*ARGSUSED*/
AFuncv
getSBmodtime(val, arg)
	PassVal	val;
	Pointer	arg;
{
	Sortflag = sb_modify;
	return ACCEPTARG;
}



/*
**	Sort by name.
*/

/*ARGSUSED*/
AFuncv
getSBname(val, arg)
	PassVal	val;
	Pointer	arg;
{
	Sortflag = sb_name;
	return ACCEPTARG;
}



/*
**	Sort by transmit time.
*/

/*ARGSUSED*/
AFuncv
getSBsendtime(val, arg)
	PassVal	val;
	Pointer	arg;
{
	Sortflag = sb_sent;
	return ACCEPTARG;
}



/*
**	Get a file name
*/

char *
getname(type, dflt)
	char *	type;
	char *	dflt;
{
	char *	cp;
	char	name[MAXNAMELEN+1];

	if ( (cp = NextWord) != NULLSTR )
	{
		NextWord = NULLSTR;
		return cp;
	}

	for ( ;; )
	{
		if ( dflt != NULLSTR )
			Fprintf(stdout, english("New %s [%s] ? "), type, dflt);
		else
			Fprintf(stdout, english("New %s ? "), type);

		if ( readword(name, sizeof name, false) > 0 )
			return newstr(name);

		if ( dflt != NULLSTR )
			return newstr(dflt);
	}
}



/*
**	Pass commandsfile describing bad message to "badhandler".
*/

void
handlebad(badcmds, reason)
	char *		badcmds;
	char *		reason;
{
	register CmdV	cmdv;
	register int	fd;
	char *		errs;
	char *		cmdtemp;
	char *		badfile;
	char		template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
	CmdHead		cmds;
	ExBuf		exargs;

	Trace3(1, "handlebad(%s, %s)", badcmds, reason);

	Recover(ert_return);

	(void)sprintf(template, "%*s", (int)((sizeof template) - 1), EmptyString);

	cmdtemp = concat(SPOOLDIR, WORKDIR, template, NULLSTR);
	move(badcmds, UniqueName(cmdtemp, CHOOSE_QUALITY, OPTNAME, MesgLength, Time));

	while ( (fd = open(cmdtemp, O_RDWR)) == SYSERROR )
		Syserror(CouldNot, OpenStr, cmdtemp);

	/*
	**	Add reason for later analysis.
	*/

	InitCmds(&cmds);

	(void)AddCmd(&cmds, bounce_cmd, (cmdv.cv_name = reason, cmdv));

	if ( RdFileTime > 0 && (cmdv.cv_number = Time - RdFileTime) > 0 )
		(void)AddCmd(&cmds, traveltime_cmd, cmdv);

	if ( !WriteCmds(&cmds, fd, cmdtemp) )
		finish(EX_OSERR);

	FreeCmds(&cmds);

	(void)close(fd);

	/*
	**	Move commands to BADDIR.
	*/

	badfile = concat(SPOOLDIR, BADDIR, template, NULLSTR);
	move(cmdtemp, UniqueName(badfile, CHOOSE_QUALITY, OPTNAME, MesgLength, Time));

	FIRSTARG(&exargs.ex_cmd) = concat(SPOOLDIR, LIBDIR, BADHANDLER, NULLSTR);

	NEXTARG(&exargs.ex_cmd) = "-C";
	NEXTARG(&exargs.ex_cmd) = badfile;
	NEXTARG(&exargs.ex_cmd) = "-E";
	NEXTARG(&exargs.ex_cmd) = reason;
	NEXTARG(&exargs.ex_cmd) = "-H";
	NEXTARG(&exargs.ex_cmd) = HomeName;
	NEXTARG(&exargs.ex_cmd) = "-I";
	NEXTARG(&exargs.ex_cmd) = Name;

	if ( (errs = Execute(&exargs, ignore_signals, ex_exec, SYSERROR)) != NULLSTR )
	{
		/*
		**	A total disaster!
		*/

		Error(StringFmt, errs);
		finish(EX_OSERR);
	}

	free(ARG(&exargs.ex_cmd, 0));
	free(badfile);
	free(cmdtemp);

	Recover(ert_finish);
}



/*
**	Print header for list.
*/

#define	FROMATLEN	25
#define	TOATLEN		14

void
header(hdraddrfmt, hdrsizefmt)
	char *	hdraddrfmt;
	char *	hdrsizefmt;
{
	char *	cp;

	Fprintf
	(
		stdout, hdraddrfmt,
		FROMATLEN, english("From")
	);

	if ( Verbose )
	{
		Fprintf
		(
			stdout, hdraddrfmt,
			TOATLEN, (Sortflag==sb_sent)?english("  Transmitted"):english("   Delivered")
		);

		cp = english("  Modified");
	}
	else
	{
		if ( AllUsers )
			Fprintf
			(
				stdout, hdraddrfmt,
				TOATLEN, english("  To")
			);

		if ( Sortflag == sb_delivery )
			cp = english("  Delivered");
		else
		if ( Sortflag == sb_sent )
			cp = english(" Transmitted");
		else
			cp = english("  Modified");
	}

	Fprintf
	(
		stdout, hdrsizefmt,
		english("Size"),
		DATESTRLEN-1, cp,
		english("File")
	);
}



/*
**	Ignore signals for badhandler.
*/

void
ignore_signals(vap)
	VarArgs *	vap;
{
	(void)signal(SIGTERM, SIG_IGN);
	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGHUP, SIG_IGN);
}



/*
**	Interact with user to retrieve files.
**
**	"from@at[ to] size modtime/delivertime file"
*/

void
interact()
{
	register FthFD_p	fp;
	register ML_p		mp;
	register FL_p		flp;
	register int		i;
	register int		len;
	Cmdtyp			cmd;
	Time_t			ptime;
	bool			hdr_done;
	bool			first;
	char			fromat[FROMATLEN+1];
	char			dateb[DATESTRLEN];
	char			timeb[TIMESTRLEN];
	static char		hdraddrfmt[]	= "%-*s";
	static char		hdrsizefmt[]	= " %10s  %-*s  %s\n";
	static char		sizefmt[]	= " %10ld  %s  %s";

	fromat[FROMATLEN] = '\0';
	hdr_done = false;

	for ( flp = FilesList, i = NFiles ; --i >= 0 ; flp++ )
	{
		bool	nowrite;
		int	flags;
		Time_t	mtime;

		fp = flp->fl_fp;
		mp = flp->fl_mp;

		if ( FileNames != (FN_p)0 )
		{
			register FN_p	fnp;

			for ( fnp = FileNames ; fnp != (FN_p)0 ; fnp = fnp->fn_next )
				if ( strcmp(fp->f_name, fnp->fn_name) == STREQUAL )
				{
					if ( First )
						fnp->fn_name[0] = '\0';
					break;
				}

			if ( fnp == (FN_p)0 )
				continue;
		}
		else
		if ( First )
			i = 0;

		if ( BaseName )
		{
			char *	cp;

			if ( (cp = strrchr(fp->f_name, '/')) != NULLSTR )
				fp->f_name = cp + 1;
		}

		(void)strncpy(fromat, (mp->m_flags&MSG_RETURNED)?english("RETURNED"):mp->m_from, FROMATLEN);
		if ( (len = strlen(fromat)) <= (FROMATLEN-2) )
		{
			fromat[len++] = '@';
			(void)strncpy(&fromat[len], mp->m_source, FROMATLEN-len);
		}

		first = true;
again:
		if ( !hdr_done )
		{
			header(hdraddrfmt, hdrsizefmt);
			hdr_done = true;
		}

		Fprintf(stdout, hdraddrfmt, FROMATLEN, fromat);

		if ( Verbose )
		{
			Fprintf(stdout, "  %s", DateString((Sortflag==sb_sent)?mp->m_sent:mp->m_time, dateb));
			ptime = fp->f_time;
		}
		else
		{
			if ( AllUsers )
				Fprintf(stdout, "  %-*.*s", TOATLEN-2, TOATLEN-2, mp->m_to);

			if ( Sortflag == sb_delivery )
				ptime = mp->m_time;
			else
			if ( Sortflag == sb_sent )
				ptime = mp->m_sent;
			else
				ptime = fp->f_time;
		}

		Fprintf
		(
			stdout,
			sizefmt,
			(long)fp->f_length,
			DateString(ptime, dateb),
			fp->f_name
		);

		if ( first && Verbose )
		{
			Fprintf(stdout, english("\n\tSent to\t%s"), mp->m_to);
			Fprintf(stdout, english("\n\tfrom\t%s@%s"), mp->m_from, mp->m_source);
			Tt = 0;
			if ( mp->m_error != NULLSTR || (mp->m_flags & MSG_MULTIPART) )
				Tb = 0;
			else
				Tb = mp->m_length;
			(void)ExRoute(mp->m_source, mp->m_route, printroute);
			if ( (long)Tt > 0 )
				Fprintf(stdout, english(".\n\tTravel + queue time: %s."), TimeString(Tt, timeb));

			if ( mp->m_id != NULLSTR )
				Fprintf(stdout, english("\n\t(Original request ID was %s)"), mp->m_id);
		}

		if ( first && (Verbose || !List) )
		{
			if ( mp->m_error != NULLSTR )
				Fprintf(stdout, english("\n\tFailure reason follows:-\n%s"), mp->m_error);
		}

		if ( first && mp->m_timeout != 0 )
		{
			if ( mp->m_timeout <= Time )
				Fprintf(stdout, english("\n\tWARNING! -- file is about to time-out."));
			else
			if ( mp->m_timeout < (Time + DAY) )
				Fprintf
				(
					stdout, english("\n\tWARNING! -- file will time-out in %s."),
					TimeString(mp->m_timeout - Time, timeb)
				);
		}

		if ( List )
		{
			putc('\n', stdout);
			continue;
		}

		nowrite = checkperm(fp->f_name, mp->m_source, &mtime, &flags) ? false : true;

		if
		(
			flags
			||
			nowrite
			||
			(
				(mp->m_flags & MSG_TRUNCATED)
				&&
				(fp->f_mode & FTH_NOT_IN_MESG)
			)
			||
			(mp->m_flags & MSG_CRCERROR)
		)
			putc('\n', stdout);

		if ( flags & FILE_EXISTS )
			Fprintf
			(
				stdout,
				english(" (%salready exists)"),
				mtime > fp->f_time
					? english("later version ")
					: EmptyString
			);

		if ( flags & FILE_ISDIR )
			Fprintf(stdout, english(" (directory)"));
		else
		if ( flags & FILE_NOTREG )
			Fprintf(stdout, english(" (not regular file)"));
		else
		if ( flags & FILE_LINKS )
			Fprintf(stdout, english(" (more than 1 link)"));

		if ( nowrite )
			Fprintf(stdout, english(" (no write permission)"));

		if ( (mp->m_flags & MSG_TRUNCATED) && (fp->f_mode & FTH_NOT_IN_MESG) )
		{
			char *	cp = StripTypes(mp->m_trunc);

			Fprintf(stdout, english(" (FILE TRUNCATED AT %s)"), cp);
			free(cp);
		}

		if ( mp->m_flags & MSG_CRCERROR )
			Fprintf(stdout, english(" (DATA CRC ERROR)"));
		else
		if ( CheckCRC && (mp->m_flags & MSG_MULTIPART) )
			Fprintf(stdout, english(" (CAN'T CHECK MULTI-PART MESSAGE CRC)"));

		Append = false;
		first = false;

		Fprintf(stdout, " ? ");

		switch ( cmd = response(nowrite) )
		{
			char *	cp;
			char *	cp2;

		case r_quit:
			finish(EX_OK);

		case r_no:
			continue;

		case r_cd:
			if ( (cp = NextWord) == NULLSTR )
				cp = newstr(To.P_dir);
			if ( chdir(cp) == SYSERROR )
				(void)SysWarn(CouldNot, "chdir", cp);
			else
				Fprintf(stdout, "%s\n", cp);
			free(cp);
			goto again;

		case r_mkdir:
			if ( (cp = strrchr(fp->f_name, '/')) != NULLSTR )
				cp = newnstr(fp->f_name, cp-fp->f_name);
			cp2 = getname(english("directory"), cp);
			if ( cp != NULLSTR )
				free(cp);
			cp = concat(cp2, Slash, NULLSTR);
			{
				bool	savmkdir = Mkdir;

				Mkdir = true;
				if ( !checkperm(cp, HomeAddress, &mtime, &flags) )
					(void)SysWarn(CouldNot, "mkdir", cp2);
				Mkdir = savmkdir;
			}
			free(cp);
			free(cp2);
			goto again;

		case r_rename:
			BaseName = false;
			fp->f_name = getname(english("name"), NULLSTR);
			goto again;

		case r_basename:
			if ( (cp = strrchr(fp->f_name, '/')) != NULLSTR )
				fp->f_name = cp + 1;
			goto again;

		case r_append:
			Append = true;
		case r_keep:
		case r_yes:
			if ( !checkperm(fp->f_name, mp->m_source, &mtime, &flags) )
				goto again;
			if ( !copyfile(flp, (flags&FILE_EXISTS)?true:false) )
				goto again;
			if ( cmd == r_keep )
				continue;
			break;

		case r_delete:
			;
		}

		deletefile(flp);
	}

	if ( List )
		Fflush(stdout);
}



/*
**	Move all/requested files.
*/

void
movefiles()
{
	register FL_p	flp;
	register FN_p	fnp;
	register int	i;
	Time_t		mtime;
	int		flags;

	Trace1(1, "movefiles()");

	for ( flp = FilesList, i = NFiles ; --i >= 0 ; flp++ )
	{
		if ( (fnp = FileNames) != (FN_p)0 )
		{
			do
				if ( strcmp(flp->fl_fp->f_name, fnp->fn_name) == STREQUAL )
				{
					if
					(
						!First
						&&
						!Silent
						&&
						i > 0
						&&
						strcmp(flp[1].fl_fp->f_name, fnp->fn_name) == STREQUAL
					)
						Error(english("more than 1 file called \"%s\""), fnp->fn_name);

					if ( First )
						fnp->fn_name[0] = '\0';
					break;
				}
			while
				( (fnp = fnp->fn_next) != (FN_p)0 );

			if ( fnp == (FN_p)0 )
				continue;
		}
		else
		if ( First )
			i = 0;

		if ( Stdout || !Delete )
		{
			if
			(
				(Stdout && !Restricted)
				||
				checkperm(flp->fl_fp->f_name, flp->fl_mp->m_source, &mtime, &flags)
			)
			{
				if ( !copyfile(flp, (flags&FILE_EXISTS)?true:false) )
					continue;
			}
			else
				Error(english("\"%s\": no permission"), flp->fl_fp->f_name);
		}

		if ( !Stdout || Delete )
			deletefile(flp);
	}
}



/*
**	Add new file number to deleted list.
*/

void
NewDeleted(n)
	Ulong		n;
{
	register Del_p	dp;

	Trace2(3, "NewDeleted(%lu)", (PUlong)n);

	if ( (dp = Del_freelist) != (Del_p)0 )
		Del_freelist = dp->dl_next;
	else
		dp = Talloc(Del);

	dp->dl_next = Del_list;
	Del_list = dp;

	dp->dl_number = n;
}



/*
**	Print out each node in route, and accumulate travel-time.
*/

/*ARGSUSED*/
bool
printroute(from, time, to)
	char *		from;
	Ulong		time;
	char *		to;
{
	register Ulong	ul;
	char		tbuf[TIMESTRLEN];

	if ( Tt == (Ulong)0 )
		Fprintf(stdout, english("\n\tvia\t"));
	else
		Fprintf(stdout, ",\n\t\t");
	if ( time == 0 )
		time = 1;
	Tt += time;
	to = StripTypes(to);
	Fprintf(stdout, "%s", to);
	free(to);
	Fprintf(stdout, english(" (delay %s)"), TimeString(time, tbuf));
	if ( (ul = Tb/time) >= MIN_SPEED )
		Fprintf(stdout, english(" (%lu bytes/sec.)"), (PUlong)ul);
	return true;
}



/*
**	Read a spooled message.
**
**	Returns NULLSTR (ok) or error string.
*/

char *
readmessage(name)
	char *		name;
{
	register int	fd;
	FthReason	fthr;
	HdrReason	hr;
	char *		errs;
	static char	errf[] = "%s: %s";

	Trace2(1, "Read message \"%s\"", name);

	FreeCmds(&DataCmds);
	HdrFile = NULLSTR;

	while ( !ReadCmds(name, getcommand) )
	{
		fd = errno;
		errs = newprintf(CouldNot, ReadStr, name);
		errno = fd;

		if ( !SysWarn(errs) )
			return newprintf(errf, errs, ErrnoStr(errno));
	}

	if ( HdrFile == NULLSTR )
	{
		errs = newprintf(english("No header for message \"%s\""), name);
		(void)SysWarn(errs);
		return errs;
	}

	if ( (fd = open(HdrFile, O_READ)) == SYSERROR )
	{
		errs = newprintf(CouldNot, ReadStr, HdrFile);
		(void)SysWarn(errs);
		return newprintf(errf, errs, ErrnoStr(errno));
	}

	if ( (hr = ReadHeader(fd)) != hr_ok )
	{
		(void)close(fd);
		errs = newprintf(english("Message \"%s\" header \"%s\" error"), HdrFile, HeaderReason(hr));
		Warn(errs);
		return errs;
	}

	if ( DataLength == 0 )	/* FtpHeader in previous file */
	{
		(void)close(fd);

		if ( FtpHdrFile == NULLSTR )
		{
			errs = newprintf(english("Message \"%s\" FTP header missing"), HdrFile);
			Warn(errs);
			return errs;
		}

		if ( (fd = open(FtpHdrFile, O_READ)) == SYSERROR )
		{
			errs = newprintf(CouldNot, ReadStr, FtpHdrFile);
			(void)SysWarn(errs);
			return newprintf(errf, errs, ErrnoStr(errno));
		}

		DataLength = FtpHdrEnd;
		HdrFile = FtpHdrFile;
	}

	if 
	(
		(fthr = ReadFtHeader(fd, (long)DataLength)) != fth_ok
		||
		(fthr = GetFthFiles()) != fth_ok
	)
	{
		(void)close(fd);
		errs = newprintf(english("Message \"%s\" file transfer header \"%s\" error"), HdrFile, FTHREASON(fthr));
		Warn(errs);
		return errs;
	}

	(void)close(fd);

	if ( HdrFlags & HDR_RETURNED )
		Returned = true;
	else
		Returned = false;

	Trace5(
		2, "Read message \"%s\", msglength %lu, fthlength %d, hdrlength %d",
		name, (PUlong)MesgLength, (int)FthLength, (int)HdrLength
	);

	return NULLSTR;
}



/*
**	Read a line from user
*/

int
readword(word, size, command)
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

	if ( !Yes && !Delete )
	{
		Fflush(stdout);

		n = sizeof(buf)-1;
		cp = buf;
		chop = false;

		for ( ;; )
		{
			if ( (i = read(0, cp, Isatty?n:1)) <= 0 )
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
			cp = &buf[strspn(buf, space)];

			FreeStr(&NextWord);

			if ( (NextWord = strpbrk(cp, space)) != NULLSTR )
			{
				*NextWord++ = '\0';
				NextWord += strspn(NextWord, space);
				if ( *NextWord != '\0' )
				{
					n = strlen(NextWord);
					while ( strspn(&NextWord[--n], space) > 0 )
						 NextWord[n] = '\0';
					NextWord = newstr(NextWord);
				}
				else
					NextWord = NULLSTR;
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
		(void)strcpy(word, Yes?english("yes"):english("delete"));
		n = strlen(word);
	}
	else
	{
		word[0] = '\0';
		n = 0;
	}

	if ( Yes || Delete || !Isatty )
	{
		Fprintf(stdout, "%s\n", word);
		Fflush(stdout);
	}

	return n;
}



/*
**	Delete any messages that have been emptied.
*/

void
removemessages()
{
	register ML_p	mp;

	Trace1(1, "removemessages()");

	for ( mp = MessageList ; mp != (ML_p)0 ; mp = mp->m_next )
		writeDeleted(mp);
}



/*
**	Read a response from user
*/

Cmdtyp
response(not_yes)
	bool		not_yes;
{
	register Cmd_p	cmdp;
	register int	i;
	register int	l;
	Command		cmd;
	char		com[MAXCOMLEN+60];

	cmd.c_name = com;

	for ( ;; )
	{
		if
		(
			(ComLen = readword(com, sizeof com, true)) >= MINCOMLEN
			&&
			ComLen <= MAXCOMLEN
			&&
			(cmdp = (Cmd_p)bsearch((char *)&cmd, (char *)Cmds, NCMDS, CMDZ, compare)) != (Cmd_p)0
		)
		{
			if ( !not_yes || (cmdp->c_type != r_yes && cmdp->c_type != r_append) )
				return cmdp->c_type;
		}

		if ( Yes || !Isatty )
			Error(english("illegal response \"%s\", use manual interaction"), com);

		Fprintf(stdout, english("(Respond: "));

		for ( i = 0, cmdp = Cmds ; cmdp < &Cmds[NCMDS] ; cmdp++ )
		{
			if ( not_yes && (cmdp->c_type == r_yes || cmdp->c_type == r_append) )
				continue;

			if ( i )
				Fprintf(stdout, ", ");

			if ( (i += (l = 4+strlen(cmdp->c_name))) > 60 )
			{
				Fprintf(stdout, "\n\t  ");
				i = l;
			}

			Fprintf(stdout, "\"%s\"", cmdp->c_name);
		}

		Fprintf(stdout, ") ? ");
	}
}



/*
**	Comparison routine for qsort(FilesList) :- by delivery time.
*/

int
SBdelivery(flp1, flp2)
	char *		flp1;
	char *		flp2;
{
	register FL_p	fp1;
	register FL_p	fp2;

	if ( Reverse )
	{
		fp1 = (FL_p)flp2;
		fp2 = (FL_p)flp1;
	}
	else
	{
		fp1 = (FL_p)flp1;
		fp2 = (FL_p)flp2;
	}

	if ( fp1->fl_mp->m_time > fp2->fl_mp->m_time )
		return 1;

	if ( fp1->fl_mp->m_time < fp2->fl_mp->m_time )
		return -1;

	return 0;
}



/*
**	Comparison routine for qsort(FilesList) :- by modify time, else by name.
*/

int
SBmodify(flp1, flp2)
	char *		flp1;
	char *		flp2;
{
	register FL_p	fp1;
	register FL_p	fp2;

	if ( Reverse )
	{
		fp1 = (FL_p)flp2;
		fp2 = (FL_p)flp1;
	}
	else
	{
		fp1 = (FL_p)flp1;
		fp2 = (FL_p)flp2;
	}

	if ( fp1->fl_fp->f_time > fp2->fl_fp->f_time )
		return 1;

	if ( fp1->fl_fp->f_time < fp2->fl_fp->f_time )
		return -1;

	return SBname(flp1, flp2);
}



/*
**	Comparison routine for qsort(FilesList) :- by file name, else by delivery time.
*/

int
SBname(flp1, flp2)
	char *		flp1;
	char *		flp2;
{
	register FL_p	fp1;
	register FL_p	fp2;
	register int	val;

	if ( Reverse )
	{
		fp1 = (FL_p)flp2;
		fp2 = (FL_p)flp1;
	}
	else
	{
		fp1 = (FL_p)flp1;
		fp2 = (FL_p)flp2;
	}

	if ( (val = strcmp(fp1->fl_fp->f_name, fp2->fl_fp->f_name)) == 0 )
		return SBdelivery(flp1, flp2);
	else
		return val;
}



/*
**	Comparison routine for qsort(FilesList) :- by transmit time.
*/

int
SBsend(flp1, flp2)
	char *		flp1;
	char *		flp2;
{
	register FL_p	fp1;
	register FL_p	fp2;

	if ( Reverse )
	{
		fp1 = (FL_p)flp2;
		fp2 = (FL_p)flp1;
	}
	else
	{
		fp1 = (FL_p)flp1;
		fp2 = (FL_p)flp2;
	}

	if ( fp1->fl_mp->m_sent > fp2->fl_mp->m_sent )
		return 1;

	if ( fp1->fl_mp->m_sent < fp2->fl_mp->m_sent )
		return -1;

	return 0;
}



/*
**	Comparison routine for qsort(FilesList) :- by receiver name, else by file name.
*/

int
SBto(flp1, flp2)
	char *		flp1;
	char *		flp2;
{
	register FL_p	fp1;
	register FL_p	fp2;
	register int	val;

	if ( Reverse )
	{
		fp1 = (FL_p)flp2;
		fp2 = (FL_p)flp1;
	}
	else
	{
		fp1 = (FL_p)flp1;
		fp2 = (FL_p)flp2;
	}

	if ( (val = strcmp(fp1->fl_mp->m_to, fp2->fl_mp->m_to)) == 0 )
		return SBname(flp1, flp2);
	else
		return val;
}



/*
**	Check and set forwarding address.
*/

void
set_forward()
{
	if ( ForwAddr == (Addr *)0 && ForwDir == NULLSTR )
	{
		Trace2(1, "set_forward() %s", NullStr);
		SetForward(FTPHANDLER, To.P_name, NULLSTR, NULLSTR);
		return;
	}

	if ( ForwAddr != (Addr *)0 && ForwDir != NULLSTR )
		Error(english("Ambiguous forwarding."));

	if ( ForwDir != NULLSTR )
	{
		struct stat	statb;

		if ( access(ForwDir, 2) == SYSERROR )
			Syserror(CouldNot, WriteStr, ForwDir);

		if ( stat(ForwDir, &statb) == SYSERROR )
			Syserror(CouldNot, StatStr, ForwDir);

		if ( (statb.st_mode & S_IFMT) != S_IFDIR )
			Error(english("%s: not a directory."), ForwDir);

		Error(english("Directory spooling disabled."));	/** NEEDS FIXES AND CHECKS TO filer **/

		SetForward(FTPHANDLER, To.P_name, ForwDir, NULLSTR);
		return;
	}

	Trace2(1, "set_forward() \"%s\"", UnTypAddress(ForwAddr));

	if ( !(To.P_flags & P_CANSEND) )
		Error(english("no permission to forward network messages"));

	(void)CheckAddr(&ForwAddr, &To, Error, true);

	if ( AddrAtHome(&ForwAddr, false, false) && checkusers(To.P_name) )
		Error(english("Forwarding loop."));

	SetFthTo();

	SetForward(FTPHANDLER, To.P_name, TypedAddress(ForwAddr), FthTo);

	FreeAddr(&ForwAddr);
}



/*
**	Catch signals to clean up.
*/

void
sigcatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	putc('\n', stdout);
	Fflush(stdout);
	finish(sig);
}



/*
**	Sort all files found according to sorting preference.
*/

void
sortfiles()
{
	register int		i;
	register FthFD_p	fp;
	register FL_p		flp;
	register ML_p		mp;
	int			(*cfuncp)();

	Trace1(1, "sortfiles()");

	FilesList = flp = (FL_p)Malloc(sizeof(*flp) * NFiles);

	for ( mp = MessageList ; mp != (ML_p)0 ; mp = mp->m_next )
		for ( fp = mp->m_files, i = 0 ; fp != (FthFD_p)0 ; fp = fp->f_next, i++ )
		{
			if ( DELETED(mp, i) )
				continue;

			flp->fl_in = i;
			flp->fl_fp = fp;
			flp->fl_mp = mp;
			flp++;

			DODEBUG(if((flp-FilesList)>NFiles)Fatal1("bad NFiles"));
		}

	switch ( Sortflag )
	{
	case sb_delivery:	cfuncp = SBdelivery;	break;
	case sb_null:
	case sb_modify:		cfuncp = SBmodify;	break;
	case sb_name:		cfuncp = SBname;	break;
	case sb_sent:		cfuncp = SBsend;	break;
	case sb_to:		cfuncp = SBto;		break;
	}

	qsort((char *)FilesList, NFiles, sizeof(*flp), cfuncp);
}



/*
**	Set ``m_tlength'' to be amount by which message has been truncated.
*/

void
trunc_length(mp)
	register ML_p		mp;
{
	register FthFD_p	fp;
	register long		l;

	Trace2(1, "trunc_length(%s)", mp->m_name);

	for ( l = 0, fp = mp->m_files ; fp != (FthFD_p)0 ; fp = fp->f_next )
	{
		l += fp->f_length;
		fp->f_mode &= ~FTH_NOT_IN_MESG;
	}

	mp->m_tlength = l - mp->m_length;

	for ( l = 0, fp = mp->m_files ; fp != (FthFD_p)0 ; fp = fp->f_next )
	{
		if ( l < mp->m_tlength )
			fp->f_mode |= FTH_NOT_IN_MESG;
		else
			break;

		l += fp->f_length;
	}

	mp->m_flags |= MSG_TRUNCATED;
}



/*
**	Unlink any files marked in message part list.
*/

void
UnlinkParts(mp)
	register ML_p	mp;
{
	register Cmd *	cmdp;

	Trace2(2, "UnlinkParts(%s)", mp->m_name);

	for ( cmdp = mp->m_parts.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
		{
			Trace2(3, "unlinking \"%s\"", cmdp->cd_value);
			(void)unlink(cmdp->cd_value);
		}
}



/*
**	If files have been deleted from message,
**	if there are undeleted files left, write delete commands,
**	otherwise delete entire message.
*/

void
writeDeleted(mp)
	register ML_p	mp;
{
	register int	i;
	register int	fd;
	register CmdV	cmdv;
	bool		deleted = false;
	bool		undeleted = false;
	CmdHead		commands;

	Trace2(1, "writeDeleted(%s)", mp->m_name);

	for ( i = mp->m_nfiles ; --i >= 0 ; )
		if ( DELETED(mp, i) )
			deleted = true;
		else
			undeleted = true;

	(void)strcpy(&CmdFileName[FilesDirLen], mp->m_name);
	
	if ( !undeleted )
	{
		Trace2(2, "removing message \"%s\"", mp->m_name);
		Trace2(3, "unlinking \"%s\"", CmdFileName);
		(void)unlink(CmdFileName);
		UnlinkParts(mp);
		return;
	}

	if ( !deleted )
		return;

	InitCmds(&commands);

	for ( i = mp->m_nfiles ; --i >= 0 ; )
		if ( DELETED(mp, i) && !ODELETED(mp, i) )
		{
			(void)AddCmd(&commands, deleted_cmd, (cmdv.cv_number = i, cmdv));
			deleted = false;
		}

	if ( deleted )
		return;

	while ( (fd = open(CmdFileName, O_WRITE)) == SYSERROR )
		Syserror(CouldNot, OpenStr, CmdFileName);

	(void)WriteCmds(&commands, fd, CmdFileName);

	(void)close(fd);

	SMdate(CmdFileName, mp->m_time);

	FreeCmds(&commands);
}
