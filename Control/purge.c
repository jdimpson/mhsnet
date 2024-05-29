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


static char	sccsid[]	= "@(#)purge.c	1.32 05/12/16";

/*
**	Purge the work/spool directories by scanning the command directories.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	SIGNALS
#define	STDIO
#define	ERRNO

#include	"global.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"sysexits.h"

#include	<ndir.h>


/*
**	Default parameters.
*/

#define	SAFE_AGE	(24*60*60L)	/* Don't remove younger files */
#define	MAX_DIR_LEVEL	16		/* Default max directory nesting searched (fds?) */
#define	MAX_ERRORS	32		/* Default max errors before exit */

/*
**	Parameters set from arguments.
*/

bool	All;				/* Remove all unreferenced/timed-out files -- no exceptions! */
bool	CleanFiles;			/* Clean FILESDIR as well */
bool	CleanTrace;			/* Clean TRACEDIR as well */
bool	DestroyDirs;			/* Remove empty directories in MPMSGSDIR */
Ulong	FilesAge;			/* Age for FILESDIR */
bool	Locks;				/* Remove lock files */
int	MaxDirLevel	= MAX_DIR_LEVEL;/* Max. directory depth */
int	MaxErrors	= MAX_ERRORS;	/* Max errors before exit */
char *	Name		= sccsid;	/* Program invoked name */
bool	NoMail;				/* Don't bitch (at least not by mail) */
bool	ReRoutFlag;			/* Try routing _pending files again */
Ulong	ReRoutAge	= SAFE_AGE;
bool	Silent;				/* No waffle */
int	Traceflag;			/* Trace level */
bool	Warnings;			/* Whinge */

AFuncv	getFilesAge _FA_((PassVal, Pointer));	/* For FILESDIR alternate aging */
AFuncv	getRouteAge _FA_((PassVal, Pointer));	/* For PENDINGDIR alternate aging */
AFuncv	getSearchName _FA_((PassVal, Pointer));	/* For files to be found only */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(a, &All, 0),
	Arg_bool(d, &DestroyDirs, 0),
	Arg_bool(f, &CleanFiles, 0),
	Arg_bool(l, &Locks, 0),
	Arg_bool(m, &NoMail, 0),
	Arg_bool(r, &ReRoutFlag, 0),
	Arg_bool(s, &Silent, 0),
	Arg_bool(t, &CleanTrace, 0),
	Arg_bool(w, &Warnings, 0),
	Arg_long(F, 0, getFilesAge, english("files-age-days"), OPTARG|OPTVAL),
	Arg_int(M, &MaxErrors, 0, english("max errors"), OPTARG),
	Arg_long(R, 0, getRouteAge, english("reroute-age-hours"), OPTARG|OPTVAL),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_noflag(0, getSearchName, english("search-name"), OPTARG|MANY),
	Arg_end
};

/*
**	Structure to count references to a file.
*/

#define	WORKNAMELEN	UNIQUE_NAME_LENGTH

typedef struct data_file * DF_p;

typedef struct data_file
{
	DF_p	df_great;		/* Branches of binary tree */
	DF_p	df_less;
	char	df_name[WORKNAMELEN+1];	/* File name in a work directory */
	Ushort	df_refs;		/* References */
	Ushort	df_length;		/* Length of name */
}
	DF;

/*
**	Hashing parameters.
*/

#ifndef	HASH_SIZE
#define	HASH_SIZE	127
#endif

/*
**	Structure for work directory info.
*/

typedef struct Workdir *	WD_p;

typedef struct Workdir
{
	WD_p	next;
	WD_p	great;
	WD_p	less;
	char *	name;			/* After SPOOLDIR, ends in '/' */
	int	length;
	DF_p	table[HASH_SIZE];
}
	Workdir;

WD_p	WorkDirs;
WD_p	WDirsTable[HASH_SIZE];		/* All work directories to be purged */
DF_p	SearchTable[HASH_SIZE];		/* Names to be matched */

/*
**	Miscellaneous info.
*/

char *	BadCmdRsn;			/* returned by get_cmd() */
CmdHead	Commands;			/* Commands from current commandfile */
Uint	CmdFlags;			/* Flags from command files */
char *	DotLock;			/* Lock file postfix */
#if	DEBUG
long	Entries, HashDups;
#endif
Passwd	Invoker;			/* Person invoked by */
bool	Links;				/* Flag for searchcoms() to expect special directories */
char *	LockName;			/* My lock file name */
bool	LockSet;			/* True if lockfile set */
char *	Mail_fmt;			/* Arg for mailpr() */
char *	Mail_a1;			/* Arg for mailpr() */
char *	Mail_a2;			/* Arg for mailpr() */
char	MaxName[MAXNAMLEN+1];		/* Maximum length directory entry */
Ulong	MesgLength;			/* Total bytes in current message */
Ulong	OverrideTtd;			/* Substitute age for a message */
bool	Purge;				/* Clean out work directories */
bool	RemoveDirs;			/* Clean out empty directories in "searchcoms()" */
char *	RouteFile;			/* Spool name for rerouted messages */
Time_t	SafeTime;			/* Files must be younger than this */
bool	SearchFiles;			/* Explicit file name arg used */
bool	SearchRef;			/* Explicit name matched */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
Ulong	Time_to_die;			/* Seconds to go for command file */
bool	Timeouts;			/* Flag for searchcoms() to removed time-out messages */

int	DRemoved;			/* Count of empty directories removed */
int	LRemoved;			/* Count of inactive lock files removed */
int	TRemoved;			/* Count of timed-out files removed */
int	URemoved;			/* Count of unreferenced files removed */

#define	Fprintf	(void)fprintf
#define	dots(A)		(A[0]=='.'&&(A[1]==0||(A[1]=='.'&&A[2]==0)))
#define	FORCE		true
#define	CHECK		false

/*
**	Routines
*/

DF_p	enter _FA_((char *, DF_p *));
void	find _FA_((void));
bool	get_cmd _FA_((CmdT, CmdV));
int	hashname _FA_((char *));
DF_p	lookup _FA_((char *, DF_p *));
void	mail _FA_((char *, char *, char *));
void	mailpr _FA_((FILE *));
void	nak_msg _FA_((char *, int));
char *	plural _FA_((int, int));
void	purge _FA_((void));
bool	Remove _FA_((char *, int *, bool));
int	searchcoms _FA_((char *, int));
WD_p	workdir _FA_((char *));
void	workfile _FA_((char *));

extern SigFunc	sigcatch;


int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register int	i;

	InitParams();

	DoArgs(argc, argv, Usage);

	GetInvoker(&Invoker);
	SetUser(NetUid, NetGid);

	if ( geteuid() != NetUid || !(Invoker.P_flags & P_SU) )
	{
		Warn(english("No permission."));
		exit(EX_NOPERM);
	}

	setbuf(stdout, SObuf);

	SafeTime = Time;
	if ( !All )
		SafeTime -= SAFE_AGE;

	SpoolDirLen = strlen(SPOOLDIR);

	(void)sprintf(MaxName, "%*s", MAXNAMLEN, EmptyString);
	(void)sprintf(Template, "%*s", (int)(sizeof Template - 1), EmptyString);
	RouteFile = concat(SPOOLDIR, ROUTINGDIR, Template, NULLSTR);

	InitCmds(&Commands);

	if ( chdir(SPOOLDIR) == SYSERROR )
	{
		Syserror(CouldNot, "chdir", SPOOLDIR);
		exit(EX_OSFILE);
	}

	(void)signal(SIGHUP, sigcatch);
	(void)signal(SIGINT, sigcatch);
	(void)signal(SIGQUIT, sigcatch);
	(void)signal(SIGTERM, sigcatch);

	DotLock = concat(".", LOCKFILE, NULLSTR);
	LockName = concat(LIBDIR, "purge", DotLock, NULLSTR);

	if ( !SetLock(LockName, Pid, false) )
	{
		Warn(english("%s (%d) already running on %s."), Name, LockPid, LockNode);
		exit(EX_UNAVAILABLE);
	}

	LockSet = true;

	(void)workdir(WORKDIR);		/* Add known work directories to be purged */

	Purge = true;

	find();				/* References to work files in workdirs */

	if ( Purge || SearchFiles )
		purge();

	if ( !Silent )
	{
		if ( (i = DRemoved + LRemoved + TRemoved + URemoved) )
		{
			(void)fprintf
			(
				stdout,
				english("%d fil%s removed ("),
				i, plural('e', i)
			);
			if ( DRemoved )
			{
				(void)fprintf
				(
					stdout,
					english("%d empty director%s"),
					DRemoved, plural('y', DRemoved)
				);
				i = 1;
			}
			else
				i = 0;
			if ( LRemoved )
			{
				if ( i )
					(void)fprintf(stdout, ", ");
				(void)fprintf
				(
					stdout,
					english("%d old lock fil%s"),
					LRemoved, plural('e', LRemoved)
				);
				i = 1;
			}
			if ( TRemoved )
			{
				if ( i )
					(void)fprintf(stdout, ", ");
				(void)fprintf
				(
					stdout,
					english("%d timed-out"),
					TRemoved
				);
				i = 1;
			}
			if ( URemoved )
			{
				if ( i )
					(void)fprintf(stdout, ", ");
				(void)fprintf
				(
					stdout,
					english("%d unreferenced"),
					URemoved
				);
			}
			(void)fprintf(stdout, ").\n");
		}
		else
		if ( !SearchFiles )
			(void)fprintf(stdout, english("No files removed.\n"));
	}

	(void)fflush(stdout);

	Trace3(1, "%lu entries, %lu hash duplications", (PUlong)Entries, (PUlong)HashDups);

	if ( ReRoutFlag )
	{
		Time -= ReRoutAge;
		(void)ReRoute(PENDINGDIR, ROUTINGDIR);
	}

	finish(EX_OK);
	return 0;
}



/*
**	Enter a file name in hash table,
**	and increment reference count.
*/

DF_p
enter(namep, table)
	char *		namep;
	DF_p *		table;
{
	register char *	cp1;
	register char *	cp2;
	register int	c;
	register DF_p	dfp;
	register DF_p *	dfpp;
	register int	len;

	if ( (len = strlen(namep)) == 0 || len > WORKNAMELEN )
	{
		Trace3(1, "enter name \"%s\" bad size %d", namep, len);
		return (DF_p)0;
	}

	Trace2(3, "enter \"%s\"", namep);

#	if	DEBUG == 2
	if ( Traceflag >= 4 )
	{
		Traceflag -= 4;
		if ( lookup(namep, table) == (DF_p)0 )
			Trace(4, "\"%s\" hash = %#x", namep, hashname(namep));
		Traceflag += 4;
	}
#	endif	/* DEBUG == 2 */

	for
	(
		dfpp = &table[hashname(namep)] ;
		(dfp = *dfpp) != (DF_p)0 ;
		dfpp = (c>0) ? &dfp->df_great : &dfp->df_less
	)
	{
#		if	DEBUG == 2
		if ( Traceflag >= 3 )
		{
			Traceflag -= 3;
			if ( lookup(namep, table) == (DF_p)0 )
				Trace(3, "hash duplication \"%s\"", dfp->df_name);
			Traceflag += 3;
		}
		HashDups++;
#		endif	/* DEBUG == 2 */

		if ( (c = len - dfp->df_length) != 0 )
			continue;

		for
		(
			cp1 = namep+len, cp2 = &dfp->df_name[len] ;
			cp1 != namep ;
		)
			if ( (c = *--cp1 - *--cp2) )
				break;

		if ( c == 0 && cp1 == namep )
		{
			dfp->df_refs++;
			return dfp;
		}
	}

	DODEBUG(Entries++);

	*dfpp = dfp = Talloc(DF);

	dfp->df_great = (DF_p)0;
	dfp->df_less = (DF_p)0;
	(void)strcpy(dfp->df_name, namep);
	dfp->df_length = len;
	dfp->df_refs = 1;

	Trace2(2, "enter new name \"%s\"", namep);

	return dfp;
}



/*
**	Search all possible command directories
**	for references to files in work directories.
*/

void
find()
{
	register struct direct *direp;
	register DIR *		dirp;
	register char *		cp;
	char			maxname[MAXNAMLEN+1];

	Trace2(1, "find(%s)", SPOOLDIR);

	if ( (dirp = opendir(SPOOLDIR)) == NULL )
	{
		Syserror(CouldNot, OpenStr, SPOOLDIR);
		return;
	}

	/*
	**	Find the link directories.
	*/

	Timeouts = true;
	Links = true;

	while ( (direp = readdir(dirp)) != NULL )
	{
		Trace2(3, "readdir==>\"%s\"", direp->d_name);

		if ( direp->d_name[0] != WORKFLAG && direp->d_name[0] != '.' )
		{
			cp = strcpyend(maxname, direp->d_name);
			*cp++ = '/';
			*cp++ = '\0';

			(void)searchcoms(maxname, 0);
		}
	}

	closedir(dirp);

	/*
	**	Search the holding directories.
	*/

	Links = false;

	if ( DestroyDirs )
		RemoveDirs = true;

	(void)searchcoms(MPMSGSDIR, 0);

	RemoveDirs = false;

	(void)searchcoms(REROUTEDIR, 0);
	(void)searchcoms(PENDINGDIR, 0);

	if ( !CleanFiles )
		Timeouts = false;
	else
		OverrideTtd = FilesAge;

	(void)searchcoms(FILESDIR, 0);

	OverrideTtd = 0;

	if ( CleanTrace )
		Timeouts = true;
	else
		Timeouts = false;

	(void)searchcoms(TRACEDIR, 0);

	Timeouts = false;

	(void)searchcoms(BADDIR, 0);
	(void)searchcoms(ROUTINGDIR, 0);
}



/*
**	Cleanup for error routines
*/

void
finish(error)
	int	error;
{
	if ( LockSet )
		(void)unlink(LockName);

	(void)exit(error);
}



/*
**	Function called from "ReadCmds" to process a command.
**
**	Remember any "unlink" file names that are in a workdir.
*/

bool
get_cmd(type, value)
	CmdT		type;
	CmdV		value;
{
	register char *	cp;
	static Ulong	mesgstart;

	switch ( type )
	{
	case end_cmd:
		MesgLength += value.cv_number - mesgstart;
		break;

	case flags_cmd:
		CmdFlags |= value.cv_number;
		break;

	case start_cmd:
		mesgstart = value.cv_number;
		break;

	case timeout_cmd:
		if ( Time_to_die == 0 || value.cv_number < Time_to_die )
		{
			if ( OverrideTtd > 0 )
				Time_to_die = OverrideTtd;
			else
				Time_to_die = value.cv_number;
		}
		break;

	case unlink_cmd:
		cp = value.cv_name;
		if
		(
			strncmp(cp, SPOOLDIR, SpoolDirLen) != STREQUAL
			||
			strchr(&cp[SpoolDirLen], '/') == NULLSTR
		)
		{
			register char *	dp;
			register int	r;

			BadCmdRsn = newprintf
				    (
					english("strange name in command file: \"%.*s\""),
					SpoolDirLen + 4*WORKNAMELEN,
					cp
				    );

			if ( Warnings )
				Warn(BadCmdRsn);

			/*
			**	Error if file exists but is unreadable,
			**	or parent directory is unwriteable.
			*/

			if
			(
				*cp != '/'
				||
				(access(cp, 0) != SYSERROR && access(cp, 4) == SYSERROR)
			)
				return false;

			if ( (dp = strrchr(cp, '/')) == cp )
				return false;

			*dp = '\0';
			if ( access(cp, 0) != SYSERROR )
				r = access(cp, 1);
			else
				r = 0;
			*dp = '/';

			if ( r == SYSERROR )
				return false;
		}
		else
			value.cv_name += SpoolDirLen;

		(void)AddCmd(&Commands, type, value);
		break;

	default:
		break;	/* gcc -Wall */
	}

	return true;
}



/*
**	Turn on FILESDIR aging.
*/

/*ARGSUSED*/
AFuncv
getFilesAge(val, arg)
	PassVal	val;
	Pointer	arg;
{
	if ( val.l > 0 )
		FilesAge = val.l * DAY;

	CleanFiles = true;

	return ACCEPTARG;
}



/*
**	Turn on PENDINGDIR aging.
*/

/*ARGSUSED*/
AFuncv
getRouteAge(val, arg)
	PassVal	val;
	Pointer	arg;
{
	if ( val.l > 0 )
		ReRoutAge = val.l * HOUR;

	ReRoutFlag = true;

	return ACCEPTARG;
}



/*
**	Process specific file name args.
*/

/*ARGSUSED*/
AFuncv
getSearchName(val, arg)
	PassVal	val;
	Pointer	arg;
{
	if ( enter(val.p, SearchTable) == (DF_p)0 )
		return ARGERROR;

	SearchFiles = true;

	return ACCEPTARG;
}



/*
**	Calculate hash for a file name.
*/

int
hashname(namep)
	char *		namep;
{
	register char *	cp = namep;
	register int	hash = 0;
	register int	c;

	while ( (c = *cp++) )
	{
		hash += hash;
		hash += c;
	}

	Trace3(4, "\"%s\" hash = %#x", namep, hash);

	if ( hash < 0 )
		hash = -hash;

	return hash % HASH_SIZE;
}



/*
**	Look-up file name in hash table
*/

DF_p
lookup(namep, table)
	char *		namep;
	DF_p *		table;
{
	register char *	cp1;
	register char *	cp2;
	register int	c;
	register DF_p	dfp;
	register int	len;

	if ( (len = strlen(namep)) == 0 )
		return (DF_p)0;

	for
	(
		dfp = table[hashname(namep)] ;
		dfp != (DF_p)0 ;
		dfp = (c>0) ? dfp->df_great : dfp->df_less
	)
	{
		if ( (c = len - dfp->df_length) != 0 )
			continue;

		for
		(
			cp1 = namep+len, cp2 = &dfp->df_name[len] ;
			cp1 != namep ;
		)
			if ( (c = *--cp1 - *--cp2) )
				break;

		if ( c == 0 && cp1 == namep )
			return dfp;
	}

	return (DF_p)0;
}



/*
**	Send mail to NCC_ADMIN about exception condition.
*/

void
mail(f, a1, a2)
	char *		f;
	char *		a1;
	char *		a2;
{
	static int	errcount;

	if ( NoMail )
	{
		if ( !Silent )
			Warn(f, a1, a2);
	}
	else
	{
		Mail_fmt = f;
		Mail_a1 = a1;
		Mail_a2 = a2;

		if ( (f = MailNCC(mailpr, NCC_ADMIN)) != NULLSTR )
		{
			Warn(StringFmt, f);
			free(f);
		}
	}

	if ( ++errcount >= MaxErrors )
	{
		Mail_fmt = english("Too many errors - exiting");
		(void)MailNCC(mailpr, NCC_ADMIN);
		Error(Mail_fmt);
	}

	Purge = false;
}



/*
**	Called from "MailNCC()" to produce body of mail.
*/

void
mailpr(fd)
	FILE *	fd;
{
	Fprintf(fd, english("Subject: network spool area exception needs attention.\n\n"));

	Fprintf(fd, Mail_fmt, Mail_a1, Mail_a2);

	Fprintf(fd, english("\n\nPlease examine the file/directory for errors and remove it.\n"));
}



/*
**	Explicit file name found in command file.
*/

void
matchcmd(name)
	char *	name;
{
	(void)fprintf(stdout, english("%s%s (reference)\n"), SPOOLDIR, name);
}



/*
**	Explicit file name matched.
*/

void
matchfile(name)
	char *	name;
{
	(void)fprintf(stdout, "%s%s\n", SPOOLDIR, name);
	SearchRef = true;
}



/*
**	Bounce a timed-out message that requested a NAK.
*/
/*ARGSUSED*/
void
nak_msg(name, quality)
	char *	name;
	int	quality;
{
	static char	mesg[]	= english("%sre-routing \"%s\" -- NAK_ON_TIMEOUT");

	if ( Traceflag )
	{
		Trace3(1, mesg, "NOT ", name);
	}
	else
	{
		if ( Warnings )
			Warn(mesg, EmptyString, name);

#		if	PRIORITY_ROUTING == 1
		move(name, UniqueName(RouteFile, quality, NOOPTNAME, MesgLength, RdFileTime));
#		else	/* PRIORITY_ROUTING == 1 */
		move(name, UniqueName(RouteFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, RdFileTime));
#		endif	/* PRIORITY_ROUTING == 1 */
	}
}



/*
**	Return "s" if ``num'' != 1.
*/

char *
plural(suffix, num)
	int	suffix;
	int	num;
{
	switch ( suffix )
	{
	case 'y':
		return num == 1 ? english("y") : english("ies");
	case 'e':
		return num == 1 ? english("e") : english("es");
	default:
		return num == 1 ? EmptyString : english("s");
	}
}



/*
**	Now remove any file in a work directory for which there is no reference.
*/

void
purge()
{
	register char *		dp;
	register DF_p		dfp;
	register struct direct *direp;
	register WD_p		wdp;
	DIR *			dirp;

	Trace1(1, "purge()");

	for ( wdp = WorkDirs ; wdp != (WD_p)0 ; wdp = wdp->next )
	{
		if ( (dirp = opendir(wdp->name)) == NULL )
		{
			(void)SysWarn(CouldNot, OpenStr, wdp->name);
			continue;
		}

		Trace2(3, "purge directory \"%s\"", wdp->name);

		dp = newnstr(wdp->name, wdp->length+WORKNAMELEN+1);

		while ( (direp = readdir(dirp)) != NULL )
		{
			if
			(
				!dots(direp->d_name)
				&&
				(int)strlen(direp->d_name) <= WORKNAMELEN
				&&
				(
					(dfp = lookup(direp->d_name, wdp->table)) == (DF_p)0
					||
					dfp->df_refs == 0
				)
			)
			{
				(void)strcpy(dp+wdp->length, direp->d_name);

				(void)Remove(dp, &URemoved, CHECK);
			}
		}

		free(dp);

		closedir(dirp);
	}
}



/*
**	Remove file.
*/

bool
Remove(name, countp, force)
	char *		name;
	int *		countp;
	bool		force;
{
	register char *	cp;
	struct stat	statb;
	static char	mesg[]	= english("%sremoving \"%s\" -- %s");

	if ( SearchFiles )
	{
		if ( (cp = strrchr(name, '/')) != NULLSTR )
			cp++;
		else
			cp = name;

		if ( lookup(cp, SearchTable) != (DF_p)0 )
			matchfile(name);

		return false;
	}

	if ( stat(name, &statb) == SYSERROR )
	{
		if ( Warnings )
			(void)SysWarn(CouldNot, StatStr, name);
		return false;
	}

	if
	(
		(statb.st_mode & S_IFMT) != S_IFREG
		&&
#		ifdef	S_IFALK
		(statb.st_mode & S_IFMT) != S_IFALK
		&&
#		endif	/* S_IFALK */
		(
			!RemoveDirs
			||
			(statb.st_mode & S_IFMT) != S_IFDIR
		)
	)
	{
		Warn(mesg, english("NOT "), name, english("not a regular file/directory"));
		return false;
	}

	if ( !force && statb.st_mtime >= SafeTime )
	{
		Trace4(1, mesg, "NOT ", name, "too new");
		return false;
	}
	else
	if ( Warnings )
		Warn
		(
			mesg,
			DODEBUG(Traceflag?english("NOT "):)EmptyString,
			name,
			(countp==&URemoved)?english("unreferenced"):english("timedout")
		);
	else
		Trace4(1, mesg, "NOT ", name, EmptyString);
	
	if ( !Traceflag )
	{
		if ( (statb.st_mode & S_IFMT) == S_IFDIR )
		{
			if ( RmDir(name) )
			{
				DRemoved++;
				return true;
			}
		}
		else
		if ( unlink(name) == SYSERROR )
			(void)SysWarn(CouldNot, UnlinkStr, name);
		else
		{
			(*countp)++;
			return true;
		}
	}

	return false;
}



/*
**	Search a directory for command files and enter names of data files.
*/

int
searchcoms(dir, level)
	char *			dir;
	int			level;
{
	register DIR *		dirp;
	register struct direct *direp;
	register Cmd *		cmdp;
	register char *		cp;
	register char *		ep;
	register char *		fp;
	register char *		mp;
	int			file_count;
	bool			isdir;
	struct stat		statb;

	Trace3(1, "searchcoms(%s, %d)", dir, level);

	if ( level > MaxDirLevel )
	{
		mail(english("Directory search level too deep at \"%s%s\""), SPOOLDIR, dir);
		return 1;
	}

	if ( (dirp = opendir(dir)) == NULL )
	{
		if ( errno != ENOENT )
		{
			dir = newnstr(dir, strlen(dir)-1);	/* Remove trailing '/' */
			if ( StringMatch(dir, "core") != NULLSTR )
				mail(english("found \"%s%s\""), SPOOLDIR, dir);
			else
			{
				(void)SysWarn(CouldNot, OpenStr, dir);
				mail(english("Unable to read directory \"%s%s\""), SPOOLDIR, dir);
			}
			free(dir);
		}
		return 1;
	}

	fp = concat(dir, MaxName, Slash, NULLSTR);
	ep = &fp[strlen(dir)];
	file_count = 0;

	while ( (direp = readdir(dirp)) != NULL )
	{
		/*
		**	Find valid command file.
		*/

		if ( dots(direp->d_name) )
			continue;

		file_count++;

		cp = strcpyend(ep, direp->d_name);

		while ( stat(fp, &statb) == SYSERROR )
		{
			if ( errno == ENOENT )
				goto loop;	/* Probably removed by active daemon */
			Syserror(CouldNot, StatStr, fp);
		}

		if ( (statb.st_mode & S_IFMT) == S_IFDIR )
			isdir = true;
		else
			isdir = false;

		if
		(
			Links && isdir
			&&
			(
				strcmp(direp->d_name, LINKSPOOLNAME) == STREQUAL
				||
				strcmp(direp->d_name, LINKMSGSINNAME) == STREQUAL
			)
		)
		{
			/*
			**	Add new work directory.
			*/

			*cp++ = '/';
			*cp++ = '\0';
			(void)workdir(fp);
			continue;
		}

		if
		(
			Locks && !isdir
			&&
			(mp = StringMatch(direp->d_name, &DotLock[1])) != NULLSTR
			&&	/* "*lock*" */
			(mp == direp->d_name || strcmp(mp-1, DotLock) == STREQUAL)
				/* "lock|*.lock" */
		)
		{
			/*
			**	Remove lock file.
			*/

			if ( All || !DaemonActive(dir, false) )
			{
				if ( Remove(fp, &LRemoved, All?FORCE:CHECK) )
					file_count--;
				else
				if ( All )
					Error(english("Unable to remove lock file \"%s\""), fp);
			}

			continue;
		}

		SearchRef = false;

		if ( Links && !isdir && strcmp(direp->d_name, READERSTATUS) == STREQUAL )
		{
			/*
			**	Add active incoming messages.
			*/

			if ( !GetInStatusFiles(dir, workfile) && Purge )
				mail(english("Could not read status file in \"%s%s\""), SPOOLDIR, dir);
			if ( SearchRef )
				matchcmd(fp);
			continue;
		}

		if ( isdir )
		{
			/*
			**	Search new commands directory.
			*/

			*cp++ = '/';
			*cp++ = '\0';
			if ( searchcoms(fp, level+1) == 0 && RemoveDirs && Remove(fp, &DRemoved, CHECK) )
				file_count--;
			continue;
		}

		if ( direp->d_name[0] < STATE_QUALITY || direp->d_name[0] > LOWEST_QUALITY )
		{
			if ( StringMatch(direp->d_name, "core") != NULLSTR )
				mail(english("found \"%s%s\""), SPOOLDIR, fp);
			continue;
		}

		Trace2(2, "found \"%s\"", direp->d_name);

		CmdFlags = 0;
		MesgLength = 0;
		Time_to_die = 0;
		FreeStr(&BadCmdRsn);

		FreeCmds(&Commands);

		if ( !ReadCmds(fp, get_cmd) )
		{
			static char	badcmd[] = english("Commands file \"%s%s\" contains garbage");

			if ( RdFileSize == 0 && RdFileTime == 0 )
				continue;	/* File disappeared! */

			if ( Warnings )
			{
				Warn(badcmd, SPOOLDIR, fp);
				Purge = false;
			}
			else
			{
				if ( (cp = BadCmdRsn) != NULLSTR )
					cp = concat(fp, "\n", cp, NULLSTR);
				else
					cp = newstr(fp);

				mail(badcmd, SPOOLDIR, cp);

				free(cp);
			}

			continue;
		}

		if ( !SearchFiles && Timeouts && Time_to_die > 0 && Time > (RdFileTime + Time_to_die) )
		{
			Trace5(1, "\"%s\" timed out: time=%lu, mtime=%lu, ttd=%lu", fp, (PUlong)Time, (PUlong)RdFileTime, (PUlong)Time_to_die);

			if ( CmdFlags & NAK_ON_TIMEOUT )
			{
				nak_msg(fp, direp->d_name[0]);
				file_count--;
			}
			else
			if ( Remove(fp, &TRemoved, FORCE) )
			{
				file_count--;

				for ( cmdp = Commands.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
					if ( cmdp->cd_type == unlink_cmd )
						(void)Remove(cmdp->cd_value, &TRemoved, FORCE);
			}

			continue;
		}

		for ( cmdp = Commands.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
			if ( cmdp->cd_type == unlink_cmd && cmdp->cd_value[0] != '/' )
				workfile(cmdp->cd_value);

		if ( SearchFiles )
		{
			if ( SearchRef )
				matchcmd(fp);

			workfile(fp);
		}
loop:;	}

	free(fp);

	closedir(dirp);

	return file_count;
}



/*
**	Catch system termination and wind up.
*/

void
sigcatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);

	finish(EX_OK);
}



/*
**	Add a directory to work directory table.
*/

WD_p
workdir(dir)
	char *		dir;
{
	register char *	cp1;
	register char *	cp2;
	register int	c;
	register WD_p 	wdp;
	register WD_p *	wdpp;
	register int	length = strlen(dir);

	Trace2(2, "workdir(%s)", dir);

	DODEBUG(
		if ( length == 0 || dir[0] == '/' || dir[length-1] != '/' )
			Fatal("Bad name passed to workdir(%s)", dir);
	);

	for
	(
		wdpp = &WDirsTable[hashname(dir)] ;
		(wdp = *wdpp) != (WD_p)0 ;
		wdpp = (c>0) ? &wdp->great : &wdp->less
	)
	{
		if ( (c = length - wdp->length) != 0 )
			continue;

		for
		(
			cp1 = dir+length, cp2 = &wdp->name[length] ;
			cp1 != dir ;
		)
			if ( (c = *--cp1 - *--cp2) )
				break;

		if ( c == 0 && cp1 == dir )
			return wdp;
	}

	*wdpp = wdp = Talloc0(Workdir);

	wdp->next = WorkDirs;
	WorkDirs = wdp;

	wdp->name = newstr(dir);
	wdp->length = length;

	Trace2(1, "new workdir(%s)", dir);

	return wdp;
}



/*
**	Enter a workfile name, and any new work directory.
*/

void
workfile(name)
	char *		name;
{
	register char *	cp;
	register int	c;
	register WD_p	wdp;

	Trace2(2, "workfile(%s)", name);

	if ( (cp = strrchr(name, '/')) != NULLSTR )
	{
		if ( SearchFiles && lookup(cp+1, SearchTable) != (DF_p)0 )
			matchfile(name);

		c = *++cp;
		*cp = '\0';
		wdp = workdir(name);
		*cp = c;

		if ( enter(cp, wdp->table) != (DF_p)0 )
			return;
	}

	Fatal2(english("bad name passed to workfile(): \"%s\""), name);
}
