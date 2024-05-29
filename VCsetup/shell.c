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


static char	sccsid[]	= "@(#)shell.c	1.54 05/12/16";

/*
**	Shell program for an incoming network connection.
**
**	SETUID => ROOT
*/

#define	RECOVER
#define	SIGNALS
#define	STAT_CALL
#define	STDIO
#define	ERRNO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#undef	OPTVAL
#define	ARGOPTVAL	0002	/* KLUDGE - OPTVAL is defined in socket.h! */
#include	"debug.h"
#include	"exec.h"
#include	"link.h"
#include	"routefile.h"
#include	"params.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"sysexits.h"

#include	"setup.h"
#include	"shell.h"

#if	UDP_IP == 1 || TCP_IP == 1
#ifdef	TWG_IP
#include	<sys/twg_config.h>
#endif
#ifndef	_TYPES_
#include	<sys/types.h>
#endif
#if	EXCELAN == 1
#include	<sys/socket.h>
#include	<netinet/in.h>
#else	/* EXCELAN == 1 */
#if	BSD4 >= 2 || BSD_IP == 1
#ifdef	M_XENIX
#ifndef	scounix
#include	<sys/types.tcp.h>
#endif
#endif	/* M_XENIX */
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#else	/* BSD4 >= 2 || BSD_IP == 1 */
#include	<socket.h>
#include	<in.h>
#include	<netdb.h>
#endif	/* BSD4 >= 2 || BSD_IP == 1 */
#endif	/* EXCELAN == 1 */
#endif	/* UDP_IP == 1 || TCP_IP == 1 */

#include	<crypt.h>


/*
**	Arguments.
*/

char *	Name	= sccsid;
#if	EXCELAN == 1
char *	Saddr;			/* Socket address passed by inetd */
#endif	/* EXCELAN == 1 */
int	Traceflag;

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_int(T, &Traceflag, getInt1, english("trace level"), OPTARG|ARGOPTVAL),
#	if	EXCELAN == 1
	Arg_noflag(&Saddr, 0, english("socket address"), OPTARG),
#	endif	/* EXCELAN == 1 */
	Arg_ignnomatch,	/* Funny args when invoked by inetd */
	Arg_end
};

/*
**	Daemon arguments.
*/

VarArgs	DmnArgs	=
{
	1,
	{NULLSTR},	/* Becomes deamon name below */
};

char *	DfltParams	= DFLTPARAMS;

/*
**	Communications strings.
*/

char	AlreadyActive[]	= ALREADYACTIVE;
char	BadPassword[]	= BADPASSWORD;
char	DaemonIs[]	= DAEMONIS;
char	DaemonStarts[]	= DAEMONSTARTS;
char	DefaultStarts[]	= DEFAULTSTARTS;
char	Disallowed[]	= DISALLOWED;
char	HmNameIs[]	= HOMENAMEIS;
char	InActive[]	= INACTIVE;
char	Params[]	= PARAMS;
char	PasswordIs[]	= PASSWORDIS;
char	QueryDaemon[]	= QUERYDAEMON;
char	QueryHome[]	= QUERYHOME;
char	QueryParams[]	= QUERYPARAMS;
char	QueryPassword[]	= QUERYPASSWORD;
char	ShellError[]	= SHELLERROR;
char	VCparamsAre[]	= VCPARAMS;

/*
**	Miscellaneous.
*/

jmp_buf	Alrm_jmp;
char	ChdirStr[]	= "chdir";
int	IGN_ETCPASSWD;		/* Ignore "/etc/passwd" entries, and check network password */
Link	LinkData;
char *	LockFile;
bool	LockSet;		/* True if lockfile made */
char *	Mail_mesg;
bool	PromConn;		/* True if promiscuous connection */
char *	ReadError;		/* Error value from `readfd0()' */
VCtype *ShellType;
bool	TimedOut;		/* True if read timeout */
#if	EXCELAN == 1
long	Ssockaddr;		/* From `inetd' supplied argument */
#endif	/* EXCELAN == 1 */
char *	VCparSet;		/* VC parameters, for (*vcp)(VCpar) */

/*
**	Configurable parameters.
*/

ParTb	ShParams[] =
{
	{"DFLTPARAMS",		&DfltParams,			PSTRING},
	{"IGN_ETCPASSWD",	(char **)&IGN_ETCPASSWD,	PINT},
	{"NICEDAEMON",		(char **)&NICEDAEMON,		PINT},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
	{"VCDAEMON",		&DmnArgs.va_args[0],		PSPOOL},
};

/*
**	Routines.
*/

void		getparams _FA_((char *));
void		mail_err _FA_((FILE *));
char *		matchbuf _FA_((char *, char *));
bool		password _FA_((bool));
bool		promiscuous _FA_((char *));
bool		readargs _FA_((char *));
char *		readfd0 _FA_((char *, char *));
void		set_daemon _FA_((char *));
void		writefd1 _FA_((char *));

extern SigFunc		sigcatch;

#define	Fprintf	(void)fprintf



int
main(argc, argv)
	int			argc;
	char *			argv[];
{
	register char *		cp;
	register VCtype *	tp;
	register int		i;
	register char *		linkname;
	char *			oname;

	(void)close(2);			/* `stderr' not necessarily anywhere interesting */
	if ( dup(1) == SYSERROR )
	{
		(void)dup(0);
		(void)dup(0);
	}

	InitParams();	/* Sets Time and Pid and umask(027) */

	DoArgs(argc, argv, Usage);

	if ( *Name == '-' )
		Name++;

	if ( chdir(SPOOLDIR) == SYSERROR )
	{
		Syserror(CouldNot, ChdirStr, SPOOLDIR);
		finish(EX_OSFILE);
	}

	GetParams("daemon", ShParams, sizeof ShParams);

	GetParams(Name, ShParams, sizeof ShParams);

#	if	EXCELAN == 1
	if ( Saddr != NULLSTR && (cp = strchr(Saddr, '.')) != NULLSTR )
	{
		*cp = '\0';
		Ssockaddr = htonl(xtol(Saddr));
	}
#	endif	/* EXCELAN == 1 */

	StderrLog(concat(CALLDIR, Name, ".", LOGFILE, NULLSTR));	/* Until we know who is calling */

	Recover(ert_return);

	for ( tp = Shells ; (cp = tp->name) != NULLSTR ; tp++ )
	{
		if ( (cp = strrchr(cp, '/')) != NULLSTR )
			cp++;
		else
			cp = tp->name;

		if ( strcmp(Name, cp) == STREQUAL )
		{
			cp = (*tp->func)(tp);
			ShellType = tp;
			if ( tp->VCname != NULLSTR )
				tp->VCname = concat(tp->descrip, ": ", tp->VCname, NULLSTR);
			else
				tp->VCname = tp->descrip;
			break;
		}
	}

	if ( cp == NULLSTR )
	{
		Error(CouldNot, english("get name of link for shell"), Name);
		finish(EX_USAGE);
	}

	oname = Name;
	Name = concat(oname, " ", cp, NULLSTR);

	if ( RecoverV(ert_here) )
	{
		Recover(ert_finish);

		Mail_mesg = ErrString;

		if ( LinkData.ln_rname == NULLSTR )
			LinkData.ln_rname = cp;

		if ( (cp = MailNCC(mail_err, NCC_ADMIN)) != NULLSTR )
		{
			Warn(StringFmt, cp);
			free(cp);
		}

		finish(EX_SOFTWARE);
	}

	SetUlimit();

#	ifndef	QNX
	(void)nice(NICEDAEMON);
#	endif	/* QNX */

	GetNetUid();

	SetUser(NetUid, NetGid);

	Recover(ert_return);

	(void)signal(SIGHUP, sigcatch);

	linkname = newstr(cp);

	for ( ;; )
	{
		Addr *	ap;

		ap = StringToAddr(linkname, false);

		if ( FindLink(TypedName(ap), &LinkData) )
			break;

		FreeAddr(&ap);

		if ( linkname[0] == '9' )
		{
			if ( tp->reply && promiscuous(cp) )
				break;

			if ( ReadError != NULLSTR )
				Mail_mesg = ReadError;
			else
				Mail_mesg = newprintf(CouldNot, english("find link for"), cp);
			if ( LinkData.ln_rname == NULLSTR )
				LinkData.ln_rname = cp;

			if ( (!TimedOut || NETADMIN > 1) && (cp = MailNCC(mail_err, NCC_ADMIN)) != NULLSTR )
			{
				Warn(StringFmt, cp);
				free(cp);
			}

			Error(Mail_mesg);
			free(Mail_mesg);
			finish(EX_NOHOST);
		}

		free(linkname);

		linkname = concat("9=", cp, NULLSTR);	/* Match NODE, rather than random domain */
	}

	free(Name);
	Name = concat(oname, " ", LinkData.ln_rname, NULLSTR);

	linkname = cp;	/* Save for later */

	if ( ARG(&DmnArgs, 0) == NULLSTR )
		set_daemon(tp->daemon);

	if ( LinkData.ln_flags & FL_NOCHANGE )
		NEXTARG(&DmnArgs) = "-u";	/* No UP/DOWN messages from daemon */

	NEXTARG(&DmnArgs) = tp->params;
	NEXTARG(&DmnArgs) = concat("-H", HomeName, NULLSTR);
	NEXTARG(&DmnArgs) = concat("-L", LinkData.ln_rname, NULLSTR);
	NEXTARG(&DmnArgs) = concat("-N", LinkData.ln_name, NULLSTR);
	NEXTARG(&DmnArgs) = concat("-K", tp->VCname, NULLSTR);

	if ( !DaemonActive(LIBDIR, false) )
	{
		writefd1(InActive);
		Report(english("network inactive"));
		finish(EX_OK);
	}

	if ( !password(tp->reply) )
	{
		writefd1(BadPassword);
		if ( ReadError != NULLSTR )
			Report(ReadError);
		else
			Report(english("bad password"));
		finish(EX_OK);
	}

	while ( chdir(LinkData.ln_name) == SYSERROR )
	{
		cp = concat(LinkData.ln_name, Slash, NULLSTR);
		if ( !CheckDirs(cp) )
		{
			char *	errs;

			Mail_mesg = newprintf(CouldNot, ChdirStr, LinkData.ln_name);

			if ( (errs = MailNCC(mail_err, NCC_ADMIN)) != NULLSTR )
			{
				Warn(StringFmt, errs);
				free(errs);
			}

			Syserror(Mail_mesg);
			free(Mail_mesg);
		}
		free(cp);
	}

	LockFile = concat(LINKCMDSNAME, Slash, LOCKFILE, NULLSTR);

	if ( !SetLock(LockFile, Pid, false) )
	{
		writefd1(AlreadyActive);
		Report(english("daemon already active"));
		finish(EX_OK);
	}

	LockSet = true;

	Report
	(
		english("%scall as \"%s\" on \"%s\""),
		PromConn?"promiscuous ":EmptyString,
		linkname,
		tp->VCname
	);

	StderrLog(LOGFILE);

	Trace3(1, "Pid=%d, getpid()=%d", Pid, getpid());

	if ( !Shell(SHELLPROG, StripTypes(LinkData.ln_rname), ARG(&DmnArgs, 0), 1) )
		finish(EX_OK);

	if ( (cp = tp->mesg) != NULLSTR )
	{
		if ( tp->reply )
			getparams(cp);
		else
			writefd1(cp);
	}

	if ( !readargs(tp->args) )
		(void)readargs(PARAMSFILE);

	if ( tp->vcp != (vFuncp)0 )
		(*tp->vcp)(VCparSet);

	for ( i = 3 ; close(i) != SYSERROR || i < 9 ; i++ );

	ARG(&DmnArgs, NARGS(&DmnArgs)) = NULLSTR;

	(void)execve(ARG(&DmnArgs, 0), &ARG(&DmnArgs, 0), StripEnv());

	Mail_mesg = newprintf(CouldNot, "execve", ARG(&DmnArgs, 0));

	if ( (cp = MailNCC(mail_err, NCC_ADMIN)) != NULLSTR )
	{
		Warn(StringFmt, cp);
		free(cp);
	}

	Syserror(Mail_mesg);
	free(Mail_mesg);

	finish(EX_OSFILE);
	return 0;
}



/*
**	Catch alarm signals.
*/

void
alrmcatch(sig)
	int	sig;
{
	(void)signal(sig, alrmcatch);
	(void)longjmp(Alrm_jmp, 1);
}



/*
**	Cleanup after Error()s.
*/

void
finish(error)
	int		error;
{
	static bool	finished;

	if ( !finished )
	{
		finished = true;

		if ( LockSet )
			(void)unlink(LockFile);

		if ( error != EX_OK && error != EX_HANGUP )
			writefd1(ShellError);
	}

	exit(error);
}



/*
**	Read link for params message.
*/

void
getparams(mesg)
	char *		mesg;
{
	register int	count;
	register char *	params;
	char		buf[MAX_LINE_LEN];

	for ( count = 10 ;; )
	{
		if ( --count == 0 )
		{
			Report(english("parameter negotiation timeout, using DFLTPARMS"));
			(void)SplitArg(&DmnArgs, DfltParams);
			writefd1(DefaultStarts);
			return;
		}

		writefd1(mesg);

		if ( (params = readfd0(buf, Params)) != NULLSTR )
			break;

		/** May be VCPARAMS arg ... **/

		if ( (params = matchbuf(buf, VCparamsAre)) != NULLSTR )
		{
			VCparSet = newstr(params);	/* Execute just before daemon */

			if ( mesg != QueryDaemon )
			{
				mesg = QueryDaemon;
				count = 10;
			}

			continue;
		}

		/** May be DAEMON name ... **/

		if ( (params = matchbuf(buf, DaemonIs)) != NULLSTR )
		{
			set_daemon(params);

			if ( mesg != QueryParams )
			{
				mesg = QueryParams;
				count = 10;
			}

			continue;
		}
	}

	/*
	**	Set up params.
	*/

	if ( SplitArg(&DmnArgs, params) > MAXVARARGS )
	{
		Error(english("Too many arguments in remote PARAMS"));
		finish(EX_USAGE);
	}

	writefd1(DaemonStarts);
}



/*
**	Send mail to local guru about failed chdir (called from MailNCC()).
*/

void
mail_err(fd)
	FILE *	fd;
{
	char *	cp1;
	char *	cp2;

	cp1 = StripTypes(HomeName);
	Fprintf(fd, english("Subject: network connection problem on \"%s\"\n\n"), cp1);
	free(cp1);

	if ( ShellType == (VCtype *)0 )
		cp1 = cp2 = Unknown;
	else
	{
		cp1 = ShellType->descrip;
		cp2 = ShellType->VCname;
	}

	Fprintf
	(
		fd,
		english("\
\"%s\" was unable to accept a \"%s\" connection request\n\
from \"%s\" on %s.\n\n\
The reason given was:\n%s\n\n"),
		Name,
		cp1,
		LinkData.ln_rname,
		cp2,
		Mail_mesg
	);
}



/*
**	Match line with expected leader, and check optional CRC.
*/

char *
matchbuf(buf, leader)
	char *		buf;
	char *		leader;
{
	register char *	cp;
	register char *	bp;
	int		leaderlen = strlen(leader);

	Trace5(2, "matchbuf(%*s, %s) len %d", leaderlen, buf, leader, leaderlen);

	FreeStr(&ReadError);

	/*
	**	Check for leader.
	*/

	if ( (bp = StringMatch(buf, leader)) == NULLSTR )
	{
		ReadError = newprintf(english("Expected:\n\t%s\nGot:\n\t%s"), leader, ExpandString(buf, -1));
		return NULLSTR;
	}

	bp += leaderlen;

	Trace2(3, "matchbuf() ==> \"%s\"", ExpandString(bp, -1));

	/*
	**	Check for optional CRC
	*/

	if ( strchr(bp, '{') != NULLSTR || strchr(bp, '}') != NULLSTR  )
	{
		char *	np;

		if
		(
			(cp = strchr(bp, '{')) == NULLSTR
			||
			*--cp != ' '
			||
			(np = strchr(cp, '}')) == NULLSTR
		)
		{
			ReadError = newprintf(english("Malformed CRC:\n\t%s"), buf);
			return NULLSTR;
		}

		np[1] = '\0';

		*cp = '\0';
		np = StrCRC16(bp);
		*cp = ' ';

		Trace3(2, "\"%s\" == \"%s\"?", cp, np);

		if ( strcmp(cp, np) != STREQUAL )
		{
			ReadError = newprintf(english("Bad CRC:\n\t%s\nExpected:\n\t ... %s"), buf, np);
			return NULLSTR;
		}

		*cp = '\0';
	}

	Trace3(1, "matchbuf(%s) ==> \"%s\"", leader, bp);

	return bp;
}



/*
**	Check if password file exists - check password.
*/

bool
password(canquery)
	bool		canquery;
{
	register char *	word;
	register int	count;
	Passwd		pw;
	char		buf[MAX_LINE_LEN];

	if ( !GetPassword(&pw, LinkData.ln_rname) )
		return true;

	if ( pw.P_crypt[0] == '\0' )
		return true;

	if ( !IGN_ETCPASSWD && !PromConn && pw.P_name[0] == ATYP_BROADCAST )
		return true;	/* We have already logged in ok */

	if ( !canquery )
	{
		ReadError = newstr(english("Password needed, but can't negotiate."));
		return false;
	}

	count = 10;
	do
	{
		if ( --count == 0 )
			return false;

		writefd1(QueryPassword);
	}
	while
		( (word = readfd0(buf, PasswordIs)) == NULLSTR );

	if ( strcmp(pw.P_crypt, crypt(word, pw.P_crypt)) == STREQUAL )
		return true;

	ReadError = newprintf(english("Wrong password: \"%s\""), word);

	return false;
}



/*
**	Read link for homename message, and check
**	if `promiscuous' connections allowed.
*/

bool
promiscuous(pname)
	char *		pname;	/* Contains a ':' if reverse name lookup failed in a_tcp() */
{
	register char *	name;
	register int	count;
	Passwd		pw;
	char		buf[MAX_LINE_LEN];

	count = 10;
	do 
	{
		if ( --count == 0 )
			return false;

		writefd1(QueryHome);
	}
	while
		( (name = readfd0(buf, HmNameIs)) == NULLSTR );

	/*
	**	Set up link data.
	*/

	PromConn = true;

	if ( FindLink(name, &LinkData) && strchr(pname, ':') == NULLSTR )
		return true;

	if ( !GetPassword(&pw, name) )
	{
		writefd1(Disallowed);
		Report(english("promiscuous call from %s as \"%s\" disallowed"), name, pname);
		finish(EX_OK);
		return false;
	}

	LinkData.ln_rname = newstr(name);
	LinkData.ln_name = DomainToPath(name);

	return true;
}



/*
**	Extract arguments from passed file,
**	and set them up as arguments to the daemon.
*/

bool
readargs(file)
	char *		file;
{
	register char *	ap;

	Trace2(1, "readargs(%s)", file);

	if ( (ap = ReadFile(file)) == NULLSTR )
		return false;

	if ( SplitArg(&DmnArgs, ap) > MAXVARARGS )
	{
		Error(english("Too many arguments in \"%s\""), file);
		finish(EX_USAGE);
	}

	free(ap);

	return true;
}



/*
**	Read a line from fd 0.
*/

char *
readfd0(buf, leader)
	register char *	buf;
	char *		leader;
{
	register int	i;
	register int	j;
	static char	eof[] = english("Circuit EOF");
	static char	fd0[] = "fd 0";

	Trace2(2, "readfd0(%s)", leader);

	FreeStr(&ReadError);

	if ( setjmp(Alrm_jmp) )
	{
		ReadError = newstr(english("Circuit timed out."));
		TimedOut = true;
		return NULLSTR;
	}

	(void)signal(SIGALRM, alrmcatch);
	(void)alarm((unsigned)PARAMSTIMEOUT);

	if ( ShellType->VCrdtype == DATAGRAM )
	{
		register char	c;
flush:
		while ( (i = read(0, buf, MAX_LINE_LEN-1)) <= 0 )
		{
			if ( i == 0 )
			{
				(void)alarm((unsigned)0);
				ReadError = newstr(eof);
				return NULLSTR;
			}
			Syserror(CouldNot, ReadStr, fd0);
		}

		for ( j = 0 ; j < i ; j++ )
		{
			if ( (c = buf[j] & 0x7f) == 0 )
				buf[j] = ' ';
			else
			if ( c == '\r' )
				buf[j] = '\n';
			else
				buf[j] = c;
		}

		while ( i > 0 && buf[i-1] == '\n' )
			i--;

		if ( i == 0 || buf[i] != '\n' )
			goto flush;
	}
	else
	for ( i = 0 ; i < MAX_LINE_LEN ; )
	{
		char	c;

		while ( read(0, &c, 1) <= 0 )
		{
			if ( i == 0 )
			{
				(void)alarm((unsigned)0);
				ReadError = newstr(eof);
				return NULLSTR;
			}
			Syserror(CouldNot, ReadStr, fd0);
		}

		if ( (c &= 0x7f) == 0 )
			continue;

		if ( c == '\r' )
			c = '\n';

		if ( c == '\n' )
		{
			if ( i == 0 )
				continue;
			break;
		}

		buf[i++] = c;
	}

	buf[i] = '\0';

	(void)alarm((unsigned)0);

	Trace2(1, "readfd0() ==> \"%s\"", ExpandString(buf, i));

	return matchbuf(buf, leader);
}



/*
**	Set daemon pathname.
*/

void
set_daemon(daemon)
	char *	daemon;
{
	if ( daemon == NULLSTR || daemon == '\0' )
		daemon = VCDAEMON;

	if ( daemon[0] == '/' )
		ARG(&DmnArgs, 0) = daemon;
	else
	if ( strchr(daemon, '/') != NULLSTR )
		ARG(&DmnArgs, 0) = concat(SPOOLDIR, daemon, NULLSTR);
	else
		ARG(&DmnArgs, 0) = concat(SPOOLDIR, LIBDIR, daemon, NULLSTR);
}



/*
**	Catch HANGUP signals.
*/

void
sigcatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	Error("SIGHUP from %s", (ShellType==(VCtype *)0)?Unknown:ShellType->VCname);
	finish(EX_HANGUP);
}



/*
**	Write fd 1.
*/

void
writefd1(s)
	char *		s;
{
	register int	n = strlen(s);

	Trace3(1, "writefd1 (len %d) ==> %s", n, s);

	if ( setjmp(Alrm_jmp) )
	{
		Error("fd 1 write timed out");
		finish(EX_IOERR);
	}

	(void)signal(SIGALRM, alrmcatch);
	(void)alarm((unsigned)PARAMSTIMEOUT);

	if ( write(1, s, n) != n )
	{
		(void)alarm((unsigned)0);
		Syserror(CouldNot, WriteStr, "fd 1");
		finish(EX_OSERR);
	}

	(void)alarm((unsigned)0);
}
