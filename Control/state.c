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
**	Manage state and routing files.
*/

#define	STAT_CALL
#define	FILE_CONTROL
#define	LOCKING
#define	RECOVER
#define	SIGNALS
#define	STDIO
#define	ERRNO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"exec.h"
#include	"handlers.h"
#include	"link.h"
#include	"Passwd.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"route.h"
#include	"routefile.h"
#include	"spool.h"
#include	"statefile.h"
#include	"sysexits.h"



/*
**	Parameters set from arguments.
*/

bool	Commands;			/* Read commandsfile after statefile */
bool	CmdsFromStdin;			/* Read commands from stdin */
int	CTraceflag;			/* Commands trace level */
bool	ExportState;			/* Force export statefile creation */
bool	IgnoreCRC;			/* Ignore CRC in statefile */
char *	Name		= sccsid;	/* Program invoked name */
bool	NoRequest;			/* Don't generate request */
bool	NoState;			/* Don't look at default statefile */
#	ifdef	MALLOC_SIZES
bool	PrintMSizes;			/* Print malloced size histogram */
#	endif	/* MALLOC_SIZES */
bool	Query;				/* Address and comment on stdout */
Time_t	SourceDate;			/* Date of message at source */
char *	SourceAddr;			/* Source for imported statefile */
bool	StFromStdin;			/* Read statefile from stdin */
bool	StToStdout;			/* Write statefile to stdout */
int	Traceflag;			/* Trace level */
Ulong	Ttd;				/* Optional expiry time for any state message generated */
bool	UpdateRoute;			/* Update routing tables */
bool	UpdateState;			/* Update state file */
bool	Warnings;			/* For warnings rather than termination on errors */

/*
**	Arguments.
*/

AFuncv	getCommands _FA_((PassVal));
AFuncv	getRoute _FA_((PassVal));
AFuncv	getState _FA_((PassVal));

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(c, &CmdsFromStdin, 0),
	Arg_bool(e, &IgnoreCRC, 0),
	Arg_bool(i, &StFromStdin, 0),
#	ifdef	MALLOC_SIZES
	Arg_bool(m, &PrintMSizes, 0),
#	endif	/* MALLOC_SIZES */
	Arg_bool(n, &NoRequest, 0),
	Arg_bool(o, &StToStdout, 0),
	Arg_bool(q, &Query, 0),
	Arg_bool(r, &UpdateRoute, 0),
	Arg_bool(s, &UpdateState, 0),
	Arg_bool(w, &Warnings, 0),
	Arg_bool(x, &ExportState, 0),
	Arg_string(C, 0, getCommands, english("commandsfile"), OPTARG|OPTVAL),
	Arg_long(D, &SourceDate, 0, english("sourcedate"), OPTARG),
	Arg_string(F, &SourceAddr, 0, english("sourceaddr"), OPTARG),
	Arg_string(R, 0, getRoute, english("routefile"), OPTARG),
	Arg_string(S, 0, getState, english("statefile"), OPTARG|OPTVAL),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_int(U, &CTraceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_long(Z, &Ttd, 0, english("seconds-to-die"), OPTARG),
	Arg_end
};

/*
**	Miscellaneous definitions.
*/

Passwd	Invoker;			/* Person invoked by */
char *	Mail_mesg;			/* Passes data to mail print routines */
bool	NewRoute;			/* Alternate `routefile' specified */
FILE *	StateFd;			/* File descriptor for statefile */

void	mailpr _FA_((FILE *));
void	mail_err _FA_((FILE *));
char *	plural _FA_((int, long));
bool	rm _FA_((char *));
bool	state _FA_((void));

#define	Fprintf		(void)fprintf

#define	NEEDSTATE	(Commands||CmdsFromStdin||ExportState||Query||StFromStdin||StToStdout||UpdateRoute||UpdateState)

#define	CHECKSTATE	(ChangeState||ExportState||StToStdout||UpdateRoute||UpdateState)

extern Ulong	RouteSize _FA_((void));
extern void	SetSRoute _FA_((char *));



/*
**	Argument processing.
*/

int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	InitParams();

	SpoolDirLen = strlen(SPOOLDIR);

	(void)umask(022);	/* Override 027 in InitParams() */

	DoArgs(argc, argv, Usage);

	SetUlimit();

	GetInvoker(&Invoker);
	SetUser(NetUid, NetGid);

	if ( geteuid() != NetUid || !(Invoker.P_flags & P_SU) )
	{
		Warn(english("No permission."));
		exit(EX_NOPERM);
	}

	setbuf(stdout, SObuf);

	if ( !state() )
		exit(EX_DATAERR);

#	ifdef	MALLOC_SIZES
	if ( PrintMSizes )
		Print_MS(stderr);
#	endif	/* MALLOC_SIZES */

	exit(EX_OK);
	return 0;
}



/*
**	Cleanup for error routines.
*/

void
finish(error)
	int	error;
{
	if ( !ExInChild )
		NewState(StateFd, false);

	(void)exit(error);
}



AFuncv
getCommands(val)
	PassVal		val;
{
	register char *	cp = val.p;

	if ( *cp != '\0' )
		SetCommands(cp);

	Commands = true;
	return ACCEPTARG;
}



AFuncv
getRoute(val)
	PassVal		val;
{
	SetSRoute(val.p);
	NewRoute = true;
	return ACCEPTARG;
}



AFuncv
getState(val)
	PassVal		val;
{
	register char *	cp = val.p;

	if ( *cp != '\0' )
		(void)SetState(val.p);
	else
		NoState = true;

	NoRequest = true;

	return ACCEPTARG;
}



/*
**	Called from "MailNCC()" to produce body of mail about routing table size.
*/

void
mailpr(fd)
	FILE *	fd;
{
	Fprintf(fd, english("Subject: network routing tables reduced %s.\n\n"), Mail_mesg);

	Fprintf
	(
		fd,
		english("\
The routing tables have been reduced in size %s.\n"),
		Mail_mesg
	);

	if ( SourceAddr != NULLSTR )
		Fprintf
		(
			fd,
			english("\n\
This was probably caused by a topology update message\n\
from \"%s\".\n"),
			StripTypes(SourceAddr)
		);
	else
		Fprintf
		(
			fd,
			english("\n\
Please check the state data in \"%s%s%s\"\n\
for problems by invoking:\n\t%s -ws\n"),
			SPOOLDIR,
			STATEDIR,
			STATEFILE,
			STATE
		);
}



/*
**	Called from "MailNCC()" to produce body of mail about sitefile problem.
*/

void
mail_err(fd)
	FILE *	fd;
{
	Fprintf(fd, english("Subject: local site information problem.\n\n"));

	Fprintf
	(
		fd,
		english("\
There was a problem installing the local site information\n\
in the site data-base hierarchy for the following reason:\n\
%s\n\n"),
		Mail_mesg
	);

	Fprintf
	(
		fd,
		english("\
This is a non-urgent problem that prevents correct reports\n\
from \"netmap\" for the relevant site\n(unless the \"-i\" flag is used).\n")
	);
}



/*
**	Read in state file and check it, or read route file,
**	print and/or update statefile, and/or write routefile.
*/

bool
state()
{
	bool		ccval = true;
	bool		cival = true;
	extern Region *	SourceRegion;


	if ( NEEDSTATE )
	{
		if ( Warnings || StToStdout )
			Recover(ert_return);

		MakeTopRegion();
		InitTypeMap();

		if ( !NoState )
			StateFd = ReadState(UpdateState?for_writing:for_reading, IgnoreCRC);

		if ( HomeRegion != (Region *)0 && HomeRegion->rg_state == 0 )
			HomeRegion->rg_state = Time;

		if ( StFromStdin && SourceAddr != NULLSTR && HomeRegion != (Region *)0 && !AnteState )
		{
#			if	SUN3 == 1
			register int	c;

			(void)ungetc(c = getc(stdin), stdin);	/* Backward compatibility with SUN III */

			if ( c != EOF && (c & TOKEN_C) == 0 )
				R3state(stdin, true, SourceAddr, SourceDate, IgnoreCRC);
			else
#			endif	/* SUN3 == 1 */
				Rstate(stdin, true, SourceAddr, SourceDate, IgnoreCRC);

			if ( AnteState )
				while ( getc(stdin) != EOF );	/* Flush input */
		}

		if ( AnteState )
		{
			if ( !NoState )
				NewState(StateFd, false);

			if
			(
				SourceRegion != (Region *)0
				&&
				SourceRegion->rg_state >= SourceDate
				&&
				SourceRegion->rg_state < Time
			)
				return true;	/* Ignore out-of-date message */

			return false;	/* Can't do anything with data at this point */
		}

		NewComment = false;
		ChangeState = false;
		ChangeRegion = HomeRegion;
		Check_Conflict = true;

		if ( CmdsFromStdin )
			cival = Rcommands(stdin, Warnings);

		if ( Commands || CHECKSTATE )
			ccval = RdStCommands(Warnings);

		Recover(ert_finish);

		if ( CHECKSTATE )
		{
			CheckComment();
			CheckState(Warnings);

			if ( StFromStdin && SourceRegion != (Region *)0 )
			{
				if ( SourceRegion->rg_route[FASTEST] == (Region *)0 )
					Error(english("Source region \"%s\" unreachable"), SourceRegion->rg_name);

				if ( RemovedRegions )
					Warn
					(
						english("%ld nod%s unreachable"),
						(long)RemovedRegions,
						plural('e', RemovedRegions)
					);
			}
		}
	}

	if ( UpdateState )
	{
		if ( StateFd == NULL || HomeRegion == (Region *)0 )
			Error(english("No state to update"));

		(void)signal(SIGHUP, SIG_IGN);
		(void)signal(SIGINT, SIG_IGN);
		(void)signal(SIGTERM, SIG_IGN);

		if ( ChangeState || HomeRegion->rg_state == 0 )
			HomeRegion->rg_state = Time;

		Write_State(StateFd, local_state);
		NewState(StateFd, true);
		StateFd = NULL;
	}

	if ( UpdateRoute )
	{
		Ulong	old_size = RouteSize();
		Ulong	min_size;
		Ulong	new_size;

		if ( SourceAddr != NULLSTR )
			min_size = old_size / 2;	/* 50% */
		else
			min_size = (old_size * 3) / 4;	/* 75% */

		new_size = WriteRoute();

		if ( new_size < min_size && !NewRoute )
		{
			char *	errs;

			Mail_mesg = newprintf
					(
						english("from %lu to %lu [%d%%]"),
						(PUlong)old_size,
						(PUlong)new_size,
						(int)(((old_size-new_size)*100)/old_size)
					);

			Warn(english("Routing tables reduced %s!"), Mail_mesg);

			if ( (errs = MailNCC(mailpr, NCC_ADMIN)) != NULLSTR )
			{
				Warn(StringFmt, errs);
				free(errs);
			}

			free(Mail_msg);
		}
	}

	if ( Query )
	{
		extern char *	RSrcAddr;

		if ( RSrcAddr != NULLSTR )
		{
			Fprintf(stdout, "%s", RSrcAddr);
			putc('\n', stdout);

			if ( SourceComment != NULLSTR )
				Fprintf(stdout, "%s", SourceComment);
		}
		else
		{
			if ( HomeRegion != (Region *)0 )
				Fprintf(stdout, "%s", HomeRegion->rg_name);
			putc('\n', stdout);

			if ( HomeComment != NULLSTR )
				Fprintf(stdout, "%s", HomeComment);
		}

		(void)fflush(stdout);
	}

	if ( StToStdout )
		Write_State(stdout, site_state);

	if ( (ChangeState && UpdateState && !NoRequest) || ExportState )
	{
		register char *	cp;
		register char *	ep;
		register FILE *	fd;
		char *		st;
		char		Template[UNIQUE_NAME_LENGTH+1];

		(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
		ep = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
		(void)UniqueName(ep, CHOOSE_QUALITY, OPTNAME, (Ulong)1, Time);

		if ( (fd = fdopen(create(ep), "w")) == NULL )
			Syserror(CouldNot, OpenStr, ep);
		else
		{
			Write_State(fd, export_state);
			(void)fclose(fd);
			(void)chmod(ep, 0444);
		}

		cp = concat(SPOOLDIR, STATEDIR, EXPORTFILE, NULLSTR);
		(void)unlink(cp);
		move(ep, cp);
		free(cp);

		if ( (fd = fdopen(create(ep), "w")) == NULL )
			Syserror(CouldNot, OpenStr, ep);
		else
		{
			Write_State(fd, site_state);
			(void)fclose(fd);
			(void)chmod(ep, 0444);
		}

		cp = concat(SPOOLDIR, STATEDIR, SITEFILE, NULLSTR);
		move(ep, cp);
		free(ep);

		ep = newstr(HomeRegion->rg_name);
		st = StripTypes(ep);
		do
		{
			char *	statefile;
			char *	path;

			if ( (path = DomainToPath(ep)) == NULLSTR )
				goto out;

			statefile = concat(SPOOLDIR, STATEDIR, MSGSDIR, path, NULLSTR);
			free(path);

			if ( unlink(statefile) != SYSERROR || errno == ENOENT || rm(statefile) )
				while ( link(cp, statefile) == SYSERROR )
					if
					(
						!CheckDirs(statefile)
						&&
						!SysWarn("Can't link \"%s\" to \"%s\"", cp, statefile)
					)
						break;	/* Some other invokation already got there */

			free(statefile);
out:
			free(ep);
			ep = st;
			st = NULLSTR;
		}
			while ( ep != NULLSTR );

		free(cp);
	}

	if ( ChangeState && (UpdateState || ExportState) && !NoRequest )
	{
		register char *	cp;
		register char *	errs;
		char		nbuf[NUMERICARGLEN];
		static ExBuf	ea;

		FIRSTARG(&ea.ex_cmd) = concat(SPOOLDIR, LIBDIR, REQUESTER, NULLSTR);

		/*
		**	Export site info. to links and visible region.
		*/

		NEXTARG(&ea.ex_cmd) = "-beln";

		if ( Ttd != 0 )
			NEXTARG(&ea.ex_cmd) = NumericArg(nbuf, 'Z', Ttd);

		if ( ChangeRegion == (Region *)0 )
			cp = EmptyString;
		else
		if ( ChangeRegion->rg_route[FASTEST] == (Region *)0 )
			cp = VisibleName;
		else
		if ( InRegion(ChangeRegion, VisRegion) )
			cp = ChangeRegion->rg_name;
		else
			cp = VisibleName;

		if ( cp != NULLSTR && cp[0] != '\0' )
		{
			NEXTARG(&ea.ex_cmd) = "-R";
			NEXTARG(&ea.ex_cmd) = cp;
		}

		if ( (errs = Execute(&ea, NULLVFUNCP, ex_exec, SYSERROR)) != NULLSTR )
		{
			Error(StringFmt, errs);
			free(errs);
		}

		if ( NETADMIN >= 1 )
		{
			errs = concat(SPOOLDIR, STATEDIR, LOGFILE, NULLSTR);
			StderrLog(errs);
			free(errs);
			if ( ChangeRegion == (Region *)0 )
				errs = EmptyString;
			else
				errs = ChangeRegion->rg_name;
			if ( cp == NULLSTR )
				cp = EmptyString;
			Report(english("state broadcast: change in region %s => *.%s"), errs, cp);
		}
	}

	if ( ccval && cival )
		return true;

	return false;
}



/*
**	Return "s" if ``num'' != 1.
*/

char *
plural(suffix, num)
	int		suffix;
	long		num;
{
	static char	s[3];

	if ( num != 1 )
		switch ( suffix )
		{
		case 'y':	return english("ies");
		default:
			s[0] = suffix;
			s[1] = 's';
			s[2] = '\0';
			return s;
		}

	s[0] = suffix;
	s[1] = '\0';
	return s;
}



/*
**	Deal with pre-existing file.
*/

bool
rm(file)
	char *		file;
{
	register char *	cp;
	register char *	s;

	switch ( errno )
	{
	case ENOTDIR:	/* Remove file by stripping trailing '/'s */
		for ( s = newstr(file) ;; )
		{
			if ( (cp = strrchr(s, '/')) == NULLSTR )
				break;
			*cp = '\0';
			if ( access(s, 0) != SYSERROR )
			{
				file = s;
				goto renm;
			}
		}
		free(s);
		break;

	case EISDIR:
	case EPERM:	/* Rename directory */
renm:		s = concat(file, ",", NULLSTR);
		if ( access(s, 0) != SYSERROR )
		{
			free(s);
			break;
		}
		move(file, s);

		Mail_mesg = newprintf("the pre-existing file\n\t%s\nwas renamed as\n\t%s", file, s);

		if ( (cp = MailNCC(mail_err, NCC_ADMIN)) != NULLSTR )
		{
			Warn(StringFmt, cp);
			free(cp);
		}

		free(Mail_mesg);
		free(s);
		return true;
	}

	Mail_mesg = newprintf("the pre-existing file\n\t%s\nprevents installation of new version\n(error: %s).", file, ErrnoStr(errno));

	if ( (cp = MailNCC(mail_err, NCC_ADMIN)) != NULLSTR )
	{
		Warn(StringFmt, cp);
		free(cp);
	}

	free(Mail_mesg);
	return false;
}
