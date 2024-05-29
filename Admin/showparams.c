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


static char	sccsid[]	= "@(#)showparams.c	1.46 05/12/16";

/*
**	Show parameter settings.
*/

#define	PASSWD_USED
#define	STDIO
#define	TIME

#include	"global.h"
#include	"Args.h"
#include	"debug.h"
#include	"handlers.h"
#include	"licence.h"
#include	"link.h"
#include	"params.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */

#include	<ctype.h>

#if	AUSAS == 0
#include	<grp.h>

extern struct group *	getgrnam _FA_((const char *));
#endif	/* AUSAS == 0 */


/*
**	Parameters set from arguments.
*/

bool	All;					/* Show all parameters */
bool	ChkLicence;				/* Check licence */
bool	Defaults;				/* Show default settings */
bool	ShowFreeSpace;				/* Show FSFree() setting */
bool	ShowVersion;				/* Show Version[] */
char *	Name	= sccsid;
bool	NoWarnings;
char *	THandler;				/* Params for particular program */
char *	Param;					/* Optional parameter to be matched */
int	Traceflag;
bool	Verbose;

/*
**	Arguments.
*/

AFuncv	getHandler _FA_((PassVal, Pointer, char *));
AFuncv	getParam _FA_((PassVal, Pointer, char *));

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, &All, 0),
	Arg_bool(d, &Defaults, 0),
	Arg_bool(f, &ShowFreeSpace, 0),
	Arg_bool(l, &ChkLicence, 0),
	Arg_bool(n, &ShowVersion, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_bool(w, &NoWarnings, 0),
	Arg_string(A, &THandler, 0, english("handler"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_noflag(0, getHandler, english("handler"), OPTARG),
	Arg_noflag(0, getParam, english("match"), OPTARG|MANY),
	Arg_end
};

/*
**	Settable parameters.
**
**	(These must be extracted from the various handlers.)
*/

int	ABORT;
bool	Ack;
char *	DfltParams	= "-cB -D16 -R300 -S10 -X30";
char *	Editor		= "/bin/ed";
int	ERROR		= EX_OK;
int	FilesExpireDays	= FILESEXPIREDAYS;
char *	FgnRProg;
char *	FilterProg;
char *	FOREIGNUSERNAME;
char *	HndlrProg;
int	IGN_ETCPASSWD;
int	IgnMailerStatus	= IGNMAILERSTATUS;
int	IgnNewsErr	= NEWSIGNERR;
int	KeepState	= KEEPSTATE;
char *	LogName;
int	MailFrom	= MAIL_FROM;
int	MaxListCols	= 80;
Ulong	MaxListSize;
bool	MessageID;
bool	No_lr;
bool	No_X;
bool	NoRet;
int	NveTimeDelay	= NVETIMEDELAY;
char *	Pager		= "more";
char *	PasswdSort	= "sort -t. -k2nr %s -o %s";
int	Port		= 1989;
bool	Received;
char *	SendAckARGS	= "-au -Amailer -NMail";
char *	SendARGS	= "-u -Amailer -NMail";
char *	Sender		= "sendfile";
char *	Server		= "_lib/tcpshell";
char *	Service		= "mhsnet";
int	ShowRoute	= SHOW_ROUTE;
char *	SourceAddr;	/* `From: ' site address for mail */
int	UUCPlowerdev	= UUCPLOWERDEV;
int	UUCPmlockdev	= UUCPMLOCKDEV;
int	UUCPstrpid	= UUCPSTRPID;
int	ValidateMail	= VALIDATEMAIL;
char *	VCdaemon;

ParTb	Params[] =
{
	{"ABORT",		(char **)&ABORT,		PINT},
	{"ACKMAIL",		(char **)&Ack,			PINT},
	{"ADDRESS",		&SourceAddr,			PSTRING},
	{"DFLTPARAMS",		&DfltParams,			PSTRING},
	{"EDITOR",		&Editor,			PSTRING},
	{"ERROR",		(char **)&ERROR,		PINT},
	{"FILESERVERLOG",	&FILESERVERLOG,			PSPOOL},
	{"FILESERVER_MAXC",	(char **)&MaxListCols,		PINT},
	{"FILESERVER_MAXL",	(char **)&MaxListSize,		PLONG},
	{"FILESERVER_NOLR",	(char **)&No_lr,		PINT},
	{"FILESERVER_NOX",	(char **)&No_X,			PINT},
	{"FILESEXPIREDAYS",	(char **)&FilesExpireDays,	PINT},
	{"FILTERPROG",		&FilterProg,			PSPOOL},
	{"FOREIGNUSERNAME",	&FOREIGNUSERNAME,		PSTRING},
	{"GETFILE",		&GETFILE,			PSTRING},
	{"HANDLERPROG",		&HndlrProg,			PSPOOL},
	{"IGNMAILERSTATUS",	(char **)&IgnMailerStatus,	PINT},
	{"IGN_ETCPASSWD",	(char **)&IGN_ETCPASSWD,	PINT},
	{"KEEPSTATE",		(char **)&KeepState,		PINT},
	{"LOGFILE",		&LogName,			PSTRING},
	{"MAILER",		&MAILER,			PSTRING},
	{"MAILERARGS",		(char **)&MAILERARGS,		PVECTOR},
	{"MAIL_FROM",		(char **)&MailFrom,		PINT},
	{"MESSAGEID",		(char **)&MessageID,		PINT},
	{"NEWSARGS",		(char **)&NEWSARGS,		PVECTOR},
	{"NEWSCMDS",		&NEWSCMDS,			PSPOOL},
	{"NEWSEDITOR",		&NEWSEDITOR,			PSTRING},
	{"NEWSIGNERR",		(char **)&IgnNewsErr,		PINT},
	{"NICEDAEMON",		(char **)&NiceDaemon,		PINT},
	{"NORET",		(char **)&NoRet,		PINT},
	{"NVETIMECHANGE",	(char **)&NveChangeMax,		PINT},
	{"NVETIMEDELAY",	(char **)&NveTimeDelay,		PINT},
	{"PAGER",		&Pager,				PSTRING},
	{"PASSWDSORT",		&PasswdSort,			PSTRING},
	{"PORT",		(char **)&Port,			PINT},
	{"PRINTER",		&PRINTER,			PSTRING},
	{"PRINTERARGS",		(char **)&PRINTERARGS,		PVECTOR},
	{"PRINTORIGINS",	&PRINTORIGINS,			PSTRING},
	{"PUBLICFILES",		&PUBLICFILES,			PSPOOL},
	{"RECEIVED",		(char **)&Received,		PINT},
	{"RECEIVER",		&FgnRProg,			PSPOOL},
	{"SENDACKARGS",		&SendAckARGS,			PSTRING},
	{"SENDARGS",		&SendARGS,			PSTRING},
	{"SENDER",		&Sender,			PSPOOL},
	{"SERVER",		&Server,			PSPOOL},
	{"SERVERGROUP",		&SERVERGROUP,			PSTRING},
	{"SERVERUSER",		&SERVERUSER,			PSTRING},
	{"SERVICE",		&Service,			PSTRING},
	{"SHOW_ROUTE",		(char **)&ShowRoute,		PINT},
	{"STATERNOTLIST",	&STATERNOTLIST,			PSTRING},
#	ifdef	SUN3SPOOLDIR
	{"SUN3LIBDIR",		&SUN3LIBDIR,			PDIR},
	{"SUN3SPOOLDIR",	&SUN3SPOOLDIR,			PDIR},
	{"SUN3STATEP",		&SUN3STATEP,			PSTRING},
	{"SUN3STATER",		&SUN3STATER,			PSTRING},
	{"SUN3USERNAME",	&SUN3USERNAME,			PSTRING},
	{"SUN3WORKDIR",		&SUN3WORKDIR,			PDIR},
#	endif	/* SUN3SPOOLDIR */
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
	{"UUCPLCKDIR",		&UUCPLCKDIR,			PDIR},
	{"UUCPLCKPRE",		&UUCPLCKPRE,			PSTRING},
	{"UUCPLOCKS",		(char **)&UUCPLOCKS,		PINT},
	{"UUCPLOWERDEV",	(char **)&UUCPlowerdev,		PINT},
	{"UUCPMLOCKDEV",	(char **)&UUCPmlockdev,		PINT},
	{"UUCPSTRPID",		(char **)&UUCPstrpid,		PINT},
	{"VALIDATEMAIL",	(char **)&ValidateMail,		PINT},
	{"VCDAEMON",		&VCdaemon,			PSPOOL},
	{"WHOISARGS",		(char **)&WHOISARGS,		PVECTOR},
	{"WHOISFILE",		&WHOISFILE,			PSTRING},
	{"WHOISPROG",		&WHOISPROG,			PSTRING},
};

/*
**	Parameter matching list
*/

typedef struct MatPar	MatPar;
struct MatPar
{
	MatPar *next;
	char *	param;
};

MatPar *	FindParams;

/*
**	Miscellaneous
*/

#define	Printf	(void)printf

void	checknames _FA_((ParTb *, int));
void	print _FA_((ParTb *, int, char *));
void	setHandler _FA_((void));



/*
**	Argument processing.
*/

int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	InitTrace();	/* Must be done before 1st Trace */

	if ( EvalArgs(argc, argv, Usage, argerror) < 0 )
	{
		InitParams();
		usagerror(ArgsUsage(Usage));
	}

	FOREIGNUSERNAME = NETUSERNAME;

	if ( !Defaults )
	{
		if ( !NoWarnings )
			CheckParams = true;
		InitParams();
	}
	else
	{
		All = true;
		Pid = getpid();
		Time = time((time_t *)0);
	}

	FgnRProg = concat(LIBDIR, "spooler.sh", NULLSTR);
	VCdaemon = concat(LIBDIR, VCDAEMON, NULLSTR);
	Sender = concat(LIBDIR, Sender, NULLSTR);

	setbuf(stdout, SObuf);

	if ( ChkLicence )
	{
		char *	sn;
		char *	st;
		int	val = EX_OK;
		Link	link;

		if ( !ReadRoute(SUMCHECK) )
			Error(english("Could not read routing tables."));

		if ( (sn = LicenceNumber) == NULLSTR )
			sn = EmptyString;

		st = HomeName;

		Printf(english("Site: %s, Licence: %s, "), st, sn);

		if ( strchr(sn, LICENCE_CHAR_LINK) != NULLSTR && LinkCount > 0 )
		{
			GetLink(&link, 0);
			Printf(english("Link: %s, "), link.ln_rname);
		}
		else
			link.ln_rname = NULLSTR;

		val = EX_DATAERR;

		switch ( CheckLicence(HomeName, sn, (int)LinkCount, link.ln_rname) )
		{
		case LICENCE_OK:
			val = EX_OK;
			Printf(english("ok.\n"));
			break;
		case LICENCE_BAD:
			Printf(english("incorrect (null).\n"));
			break;
		case LICENCE_EXPIRED:
			Printf(english("expired.\n"));
			break;
		case LICENCE_LINK:
			Printf(english("illegal link.\n"));
			break;
		default:
			Printf(english("incorrect.\n"));
			break;
		}

		exit(val);
	}

	if ( ShowVersion )
	{
		Printf("%s\n", Version);
		exit(EX_OK);
	}

	if ( ShowFreeSpace )
	{
		int	val = EX_OK;

		if ( !FSFree(SPOOLDIR, (Ulong)0) )
		{
			val = EX_UNAVAILABLE;

			Printf
			(
				english("NO SPACE on %s (MINSPOOLFSFREE=%lu Kbytes)\n"),
				SPOOLDIR,
				(PUlong)MINSPOOLFSFREE
			);
		}
		else
		{
			if ( Verbose )
				Printf(english("MINSPOOLFSFREE=%lu Kbytes\n"), (PUlong)MINSPOOLFSFREE);

			Printf
			(
#				if LONG_LONG
				english("available free space on %s is %llu Kbytes\n"),
#				else	/* LONG_LONG */
				english("available free space on %s is %lu Kbytes\n"),
#				endif	/* LONG_LONG */
				SPOOLDIR,
				(PUllong)(FreeFSpace/1024)
			);
		}

		exit(val);
	}

	if ( THandler != NULLSTR || FindParams != (MatPar *)0 )
	{
		register MatPar *	mp;

		for ( mp = FindParams ; mp != (MatPar *)0 ; mp = mp->next )
			print(NetParams, SizeNetParams, mp->param);

		if ( THandler != NULLSTR )
		{
			if ( !Defaults )
			{
				setHandler();
				if ( !NoWarnings )
					CheckParams = true;
				GetParams(THandler, Params, sizeof Params);
			}
			print(Params, sizeof Params, NULLSTR);
		}
	}
	else
		print(NetParams, SizeNetParams, NULLSTR);

	exit(EX_OK);
}



/*
**	Check user/group names.
*/

void
checknames(tp, n)
	register ParTb *tp;
	register int	n;
{
	register char *	cp;
	static char	ws[] = english("parameter \"%s\": %s \"%s\" does not exist!");

	Trace1(1, "checknames()");

	for ( n /= sizeof(ParTb) ; --n >= 0 ; tp++ )
		if
		(
			tp->type == PSTRING
			&&
			(cp = *tp->val) != NULLSTR
		)
		{
			if ( StringMatch(tp->id, "USER") != NULLSTR )
			{
				Passwd	pw;

				Trace3(2, "check user \"%s=%s\"", tp->id, cp);

				if ( !GetUid(&pw, cp) )
					Warn(ws, tp->id, english("user"), cp);
				else
					FreeUid(&pw);
			}
#			if	AUSAS == 0
			else
			if ( StringMatch(tp->id, "GROUP") != NULLSTR )
			{
				Trace3(2, "check group \"%s=%s\"", tp->id, cp);

				if ( getgrnam(cp) == (struct group *)0 )
					Warn(ws, tp->id, english("group"), cp);
			}
#			endif	/* AUSAS == 0 */
		}
}



/*
**	Clean up.
*/

void
finish(error)
	int	error;
{
	exit(error);
}



/*
**	Set different handler.
*/

AFuncv
getHandler(value, argp, string)
	PassVal			value;
	Pointer			argp;
	char *			string;
{
	register char *		cp;
	register int		c;

	if ( THandler != NULLSTR )
		return REJECTARG;

	/** If all upper-case, == a var to be matched **/

	for ( cp = value.p ; (c = *cp++) != '\0' ; )
		if ( islower(c) )
		{
			THandler = value.p;
			return ACCEPTARG;
		}

	return REJECTARG;	/* A var to be matched */
}



/*
**	Remember parameter to match.
*/

AFuncv
getParam(value, argp, string)
	PassVal			value;
	Pointer			argp;
	char *			string;
{
	register MatPar *	mp;

	mp = Talloc(MatPar);
	mp->param = value.p;
	mp->next = FindParams;
	FindParams = mp;

	return ACCEPTARG;
}



/*
**	Print out changed parameters.
*/

void
print(tp, n, m)
	register ParTb *tp;
	register int	n;
	char *		m;
{
	register char **cpp;
	register char *	cp;
	register int	i;
	static char	space[]	= " \t\n";

	if ( !NoWarnings )
		checknames(tp, n);

	for ( n /= sizeof(ParTb) ; --n >= 0 ; tp++ )
	{
		if ( m != NULLSTR )
		{
			if ( StringMatch(tp->id, m) == NULLSTR )
				continue;
		}
		else
			if ( !All && !tp->set )
				continue;

		Printf("%s=", tp->id);

		switch ( tp->type )
		{
		case PLONG:	Printf("%ld", *(long *)tp->val);	break;
		case PINT:	Printf("%d", *(int *)tp->val);		break;
		case PDIR:
		case PSPOOL:
		case PSTRING:
			if ( (cp = *tp->val) == NULLSTR )
				break;
			if ( strpbrk(cp, space) != NULLSTR )
				Printf("\"%s\"", cp);
			else
				Printf("%s", cp);
			break;
		case PVECTOR:
			if ( (cpp = *(char ***)tp->val) == (char **)0 )
				break;
			for ( i = 0 ; *cpp != NULLSTR ; i++, cpp++ )
			{
				if ( i )
					putchar(',');
				Printf("\"%s\"", *cpp);
			}
		}

		if ( All && tp->set && Verbose )
			Printf(" *");

		putchar('\n');
	}
}



/*
**	Set different handler.
*/

void
setHandler()
{
	register Handler *	hp;

	if
	(
		(hp = GetHandler(THandler)) != (Handler *)0
		||
		(hp = GetDHandler(THandler)) != (Handler *)0
	)
		THandler = hp->handler;
}
