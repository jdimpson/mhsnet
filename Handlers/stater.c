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


static char	sccsid[]	= "@(#)stater.c	1.40 05/12/16";

/*
**	Process state messages from network.
*/

#define	FILE_CONTROL
#define	RECOVER
#define	STAT_CALL
#define	SIGNALS
#define	STDIO
#define	ERRNO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"expand.h"
#include	"handlers.h"
#include	"header.h"
#include	"link.h"
#include	"params.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"
#include	"sub_proto.h"
#include	"sysexits.h"
#if	SUN3 == 1
#include	"sun3.h"
#endif	/* SUN3 == 1 */


/*
**	Parameters set from arguments.
*/

char *	HomeAddress;		/* Name of this node */
char *	LinkName;		/* Message arrived from this node */
char *	Name	= sccsid;	/* Program invoked name */
int	Parent;			/* Router pid if invoked by sub-router */
int	Traceflag;		/* Global tracing control */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(b, &Broadcast, 0),
	Arg_long(D, &DataLength, 0, english("data-length"), 0),
	Arg_string(E, &HdrEnv, 0, english("hdr-env"), OPTARG),
	Arg_long(F, &HdrFlags, 0, english("hdr-flags"), OPTARG),
	Arg_string(H, &HomeAddress, 0, english("home-address"), 0),
	Arg_string(I, &HdrID, 0, english("ID"), 0),
	Arg_string(L, &LinkName, 0, english("link"), OPTARG),
	Arg_long(M, &HdrTt, 0, english("travel-time"), 0),
/*	Arg_string(P, &HdrPart, 0, english("partno"), OPTARG),		*/
/*	Arg_string(R, &HdrRoute, 0, english("route"), 0),		*/
	Arg_string(S, &HdrSource, 0, english("source"), 0),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_int(W, &Parent, 0, english("router-pid"), OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Miscellaneous
*/

int	ABORT		= 0;		/* Terminate with prejudice */
unsigned Alarm;			/* Original ALARM setting - passed to children */
char *	ChangedRegions;			/* Formatted differences (DEBUGGING) */
int	ChildPid;			/* Pid of child process */
CmdHead	Cleanup;			/* Unlink commands off stdin */
char *	CmdsFile;			/* File containing spooled commands */
char *	CmdsTmp;			/* File for building commands */
CmdHead	DataCmds;			/* Data commands off stdin */
int	ERROR		= EX_OK;	/* Terminate with error */
char	Explicit[]	= {ATYP_EXPLICIT, '\0'};
char *	HdrFile;			/* File containing header */
Addr *	Home_Addr;			/* Construct for home address */
int	KeepState	= KEEPSTATE;	/* Build data-base of state messages */
char *	Mail_mesg;			/* String for mail_(err|unlink) */
Ulong	MesgLength;			/* Total length of message */
bool	NewRegion;			/* True if routing tables changed */
int	RetVal		= EX_OK;
char *	SLink;				/* Source untyped link */
char *	SName;				/* Source exported-typed name */
char *	SourceComment;			/* Source details */
bool	SourceIsHome;			/* Source address is us */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	TmpExp;				/* Link to state info. */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"ABORT",		(char **)&ABORT,		PINT},
	{"ERROR",		(char **)&ERROR,		PINT},
	{"KEEPSTATE",		(char **)&KeepState,		PINT},
	{"STATERNOTLIST",	&STATERNOTLIST,			PSTRING},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

#ifdef	SUN3SPOOLDIR

/*
**	Configurable SUN3 parameters.
*/

ParTb	Sun3Params[] =
{
	{"SUN3SPOOLDIR",	&SUN3SPOOLDIR,			PDIR},
	{"SUN3STATEP",		&SUN3STATEP,			PSTRING},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

#endif	/* SUN3SPOOLDIR */

#define	Fprintf		(void)fprintf

void	child_alarm _FA_((VarArgs *));
void	cleanup _FA_((void));
char *	FirstName _FA_((char *));
bool	getcommand _FA_((CmdT, CmdV));
char *	getdoms _FA_((char *));
char *	gethier _FA_((char *));
void	mail_err _FA_((FILE *));
void	mail_new _FA_((FILE *));
void	mail_ret _FA_((FILE *));
void	mail_unlink _FA_((FILE *));
bool	MissingType _FA_((char *));
bool	read_cmds _FA_((void));
void	reply _FA_((void));
void	sigcatch _FA_((int));
bool	state _FA_((char *));
bool	too_frequent _FA_((void));
void	whinge _FA_((char *, char *));



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	char *	errs;

	if ( (Alarm = alarm(0)) )
		(void)alarm(Alarm+5);

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

#	ifdef	SUN3SPOOLDIR
	GetParams(SUN3PARAMS, Sun3Params, sizeof Sun3Params);
#	endif	/* SUN3SPOOLDIR */

	StartTime = Time;
	if ( Broadcast )
		StartTime -= HdrTt;		/* Otherwise requested */

	if ( InList(CanAddrMatch, HdrSource, STATERNOTLIST, NULLSTR) && too_frequent() )
		exit(EX_OK);

	ExHomeAddress = StripTypes(HomeAddress);
	ExLinkAddress = StripTypes(LinkName);
	ExSourceAddress = ExHomeAddress;	/* Source of mail is here */

	if ( HdrFlags & HDR_RETURNED )
	{
		if ( (errs = MailNCC(mail_ret, NCC_ADMIN)) != NULLSTR )
			Error(StringFmt, errs);

		exit(EX_OK);
	}

	SourceIsHome = StringAtHome(HdrSource);

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);

	InitCmds(&DataCmds);
	InitCmds(&Cleanup);

	if ( DataLength && read_cmds() && SLink != NULLSTR )
	{
		if ( (errs = MailNCC(mail_new, NCC_MANAGER)) != NULLSTR )
		{
			Warn(StringFmt, errs);
			free(errs);
		}
	}

	if ( !SourceIsHome && (HdrFlags & HDR_ENQ) )
		reply();

#	ifdef	SUN3SPOOLDIR
	if
	(
		StringMatch(HdrEnv, NOTSUN3) == NULLSTR
		&&
		DataCmds.ch_count >= 3
	)
		passin();
#	endif	/* SUN3SPOOLDIR */

	if ( ABORT )
		abort();

	if ( RetVal == EX_OK )
		RetVal = ERROR;

	if ( RetVal == EX_OK )
		cleanup();

	exit(RetVal);
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

	for ( cmdp = Cleanup.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
			(void)unlink(cmdp->cd_value);
}



/*
**	Called from the errors routines to cleanup.
*/

void
finish(error)
	int	error;
{
	if ( ExInChild )
		(void)exit(error);

	if ( TmpExp != NULLSTR )
		(void)unlink(TmpExp);

	if ( HdrFile != NULLSTR )
		(void)unlink(HdrFile);

	if ( CmdsTmp != NULLSTR )
		(void)unlink(CmdsTmp);

	if ( CmdsFile != NULLSTR )
		(void)unlink(CmdsFile);

	if ( ABORT )
		abort();

	if ( error == EX_OK )
		error = ERROR;

	(void)exit(error);
}



/*
**	Process a command from commands file.
*/

bool
getcommand(type, val)
	CmdT	type;
	CmdV	val;
{
	static long	filestart;

	switch ( type )
	{
	case end_cmd:
		MesgLength += val.cv_number - filestart;
	case file_cmd:
	case start_cmd:
		filestart = val.cv_number;	/* Sequence is file, start, end, right? */
		(void)AddCmd(&DataCmds, type, val);
		break;

	case unlink_cmd:
		(void)AddCmd(&Cleanup, type, val);
		break;

	default:
		break;	/* gcc -Wall */
	}

	return true;
}



/*
**	Send mail to local guru about netstate error (called from MailNCC()).
*/

void
mail_err(fd)
	register FILE *	fd;
{
	register char *	cp = StripTypes(HdrSource);
	register char *	ep;

	Fprintf(fd, english("Subject: failed state message from \"%s\"\n"), cp);

	putc('\n', fd);

	Fprintf
	(
		fd,
		english("The program \"%s\"\nproduced the following error message:\n\n%s\n"),
		STATE,
		ep = StripErrString(Mail_mesg)
	);

	putc('\n', fd);
	free(ep);

	if ( KeepState )
	{
		Fprintf
		(
			fd,
			english("\
You are advised to check \"%s%s%s\"\n\
for conflicts, and then use\n\
		netincorp %s\n\
to re-process the failed state message.\n"),
			SPOOLDIR, STATEDIR, COMMANDSFILE,
			HdrSource
		);

		putc('\n', fd);

		Fprintf
		(
			fd,
			english("\
Otherwise, use\n\
		netmap -vate %s\n\
to examine the state message, and then resolve the problem\n\
by discussing it with the network manager at \"%s\"."),
			HdrSource,
			cp
		);
	}
	else
	{
		Fprintf
		(
			fd,
			english("\
You are advised to check \"%s%s%s\"\n\
for conflicts, and/or resolve the problem\n\
by discussing it with the network manager at \"%s\"."),
			SPOOLDIR, STATEDIR, COMMANDSFILE,
			cp
		);
	}

	putc('\n', fd);
	free(cp);
}



/*
**	Send mail to local guru about new site (called from MailNCC()).
*/

void
mail_new(fd)
	register FILE *	fd;
{
	register char *	cp;

	if ( NewRegion && SName != NULLSTR )
	{
		cp = StripTypes(SName);
		Fprintf(fd, english("Subject: New region \"%s\" added to routing tables\n"), cp);
	}
	else
	{
		cp = StripTypes(HdrSource);
		Fprintf(fd, english("Subject: New region \"%s\" added to statefile\n"), cp);
	}

	putc('\n', fd);
	free(cp);

	if (  NETADMIN >= 2 && ChangedRegions != NULLSTR )
		Fprintf(fd, "\t[change: %s\n\t source: %s]\n\n", ChangedRegions, HdrSource);

	Fprintf(fd, english("Reached via link:\t%s\n"), SLink);
	if ( NewRegion )
		Fprintf(fd, english("Routing table size:\t%lu bytes\n"), (PUlong)Route_size);

	putc('\n', fd);

	if ( (cp = SourceComment) != NULLSTR )
	{
		register char	c;

		Fprintf(fd, english("Region description:"));

		do
		{
			putc('\n', fd);
			putc('\t', fd);
			while ( (c = *cp++) != '\n' && c != '\0' )
				putc(c, fd);
		}
		while
			( c != '\0' );

		putc('\n', fd);
		putc('\n', fd);
	}

	if ( NewRegion && (cp = SName) != NULLSTR )
		Fprintf
		(
			fd,
			english("\
To forget this region,\n\
add the following commands to \"%s%s%s\":\n\
	remove	%s\n"),
			SPOOLDIR, STATEDIR, COMMANDSFILE,
			cp
		);
}



/*
**	Send mail to local guru about returned state message (called from MailNCC()).
*/

void
mail_ret(fd)
	register FILE *	fd;
{
	register char *	cp = StripTypes(HdrSource);
	char *		dest;

	if ( (dest = GetEnv(ENV_DEST)) != NULLSTR && dest[0] != '\0' )
	{
		char *	dp = StripTypes(dest);

		Fprintf(fd, english("Subject: state message sent to \"%s\" returned from \"%s\"\n\n"), dp, cp);

		free(dp);
	}
	else
		Fprintf(fd, english("Subject: state message returned from \"%s\"\n\n"), cp);

	if ( dest != NULLSTR )
		free(dest);
	free(cp);

	if ( (cp = GetEnv(ENV_ERR)) == NULLSTR )
		cp = newstr(english("(no reason)."));

	Fprintf(fd, english("The reason given was:\n%s"), cp);

	putc('\n', fd);
	free(cp);
}



/*
**	Send mail to local guru about failed unlink (called from MailNCC()).
*/

void
mail_unlink(fd)
	register FILE *	fd;
{
	register char *	cp = StripTypes(HdrSource);

	Fprintf(fd, english("Subject: problem for state message from \"%s\"\n"), cp);

	putc('\n', fd);
	free(cp);

	Fprintf
	(
		fd,
		english("\
The network was unable to create a file for this state message,\n\
the reason given was:\n%s\n\n"),
		Mail_mesg
	);

	Fprintf
	(
		fd,
		english("\
This is a non-urgent problem, and is probably caused by\n\
a pre-existing state message with a similar name,\n\
or by a site changing its address. You can avoid having\n\
this problem recur by removing the current file/directory.")
	);

	putc('\n', fd);
}



#ifdef	SUN3SPOOLDIR
/*
**	Pass state message to local site Sun3 side.
*/

passin()
{
	register FILE *	fd;
	register char *	errs;
	ExBuf		ea;
	Link		linkd;

	if ( SourceIsHome || SUN3STATEP == NULLSTR || SUN3SPOOLDIR == NULLSTR )
		return;

	(void)CheckRoute(NOSUMCHECK);	/* Re-load routing tables changed by state message */

	if
	(
		FindLink(HdrSource, &linkd)
		&&
		access(SUN3STATEP, 1) != SYSERROR
	)
	{
		char *	source = StripDomain(newstr(HdrSource), OzAu, Au, false);
		char *	name = FirstName(source);
		char *	home = FirstName(HomeAddress);
		char *	doms = getdoms(source);

		/** Add local links if necessary **/

		FIRSTARG(&ea.ex_cmd) = SUN3STATEP;
		NEXTARG(&ea.ex_cmd) = "-CORSc";	/* The 'O' is to prevent a state message being generated! */

		fd = (FILE *)Execute(&ea, child_alarm, ex_pipe, SYSERROR);

		ChildPid = ea.ex_pid;
		(void)signal(SIGALRM, sigcatch);

		Fprintf
		(
			fd,
			"add\t%s\nhierarchy\t%s\t%s\ndomain\t%s\t%s\nlink\t%s,%s\tmsg,%cdead,%cdown,%cint\n",
			name,
			name, gethier(source),
			name, doms,
			name, home,
			(linkd.ln_flags&FL_DEAD)?'+':'-',
			(linkd.ln_flags&FL_DOWN)?'+':'-',
			(linkd.ln_delay>=1800)?'+':'-'
		);

		if ( (errs = ExClose(&ea, fd)) != NULLSTR )
		{
			Warn(StringFmt, errs);
			free(errs);
		}

		ChildPid = 0;

		free(doms); free(home); free(name); free(source);
	}

	errs = HomeAddress;
	if ( errs[0] == ATYP_DESTINATION )
		errs++;

	FIRSTARG(&ea.ex_cmd) = concat(SPOOLDIR, LIBDIR, SUN4_3SPOOLER, NULLSTR);
	NEXTARG(&ea.ex_cmd) = "-D";
	NEXTARG(&ea.ex_cmd) = errs;
	NEXTARG(&ea.ex_cmd) = "-H";
	NEXTARG(&ea.ex_cmd) = errs;
	NEXTARG(&ea.ex_cmd) = "-L";
	NEXTARG(&ea.ex_cmd) = errs;
	NEXTARG(&ea.ex_cmd) = "-N";
	NEXTARG(&ea.ex_cmd) = errs;

	fd = (FILE *)Execute(&ea, child_alarm, ex_pipe, SYSERROR);

	ChildPid = ea.ex_pid;
	(void)signal(SIGALRM, sigcatch);

	(void)WriteCmds(&DataCmds, fileno(fd), PipeStr);

	if ( (errs = ExClose(&ea, fd)) != NULLSTR )
	{
		if ( ExStatus != EX_UNAVAILABLE )
			Error(StringFmt, errs);
		free(errs);
	}

	ChildPid = 0;
}



/*
**	Return name of first domain in string.
*/

char *
FirstName(acp)
	char *		acp;
{
	register char *	cp;
	register char *	cp1;
	register char *	cp2;

	if ( (cp = acp) == NULLSTR )
		return newstr(EmptyString);

	if ( (cp2 = strchr(cp, DOMAIN_SEP)) != NULLSTR )
		*cp2 = '\0';

	if ( (cp1 = strchr(cp, TYPE_SEP)) != NULLSTR )
		cp1++;
	else
		cp1 = cp;

	cp = newstr(cp1);

	if ( cp2 != NULLSTR )
		*cp2 = DOMAIN_SEP;

	return cp;
}



/*
**	Get domains from type-stripped address.
*/

char *
getdoms(name)
	char *		name;
{
	register char *	cp;
	register char *	dp;

	if ( (dp = strchr(name, DOMAIN_SEP)) != NULLSTR )
	{
		dp = cp = newstr(++dp);
		while ( (cp = strchr(cp, DOMAIN_SEP)) != NULLSTR )
			*cp++ = ',';
		return dp;
	}

	return newstr(EmptyString);
}



/*
**	Get hierarchy from type-stripped address.
*/

char *
gethier(name)
	char *		name;
{
	register char *	cp;

	if ( (cp = strchr(name, DOMAIN_SEP)) != NULLSTR )
		return ++cp;

	return EmptyString;
}
#endif	/* SUN3SPOOLDIR */



/*
**	Read commands off stdin.
*/

bool
read_cmds()
{
	register char *	sp;
	register char *	pp;
	register int	fd;
	char *		xp;
	bool		inform = false;
	bool		stripped;
	struct stat	statb;
#	if	TRUNCDIRNAME == 1
	char		sbuf[14+1];
#	endif	/* TRUNCDIRNAME == 1 */

	if
	(
		(sp = HdrSource) == NULLSTR
		||
		*sp++ == '\0'	/* Skip ATYP_DESTINATION */
		||
		*sp == '\0'
		||
		DataLength == 0
		||
		!ReadFdCmds(stdin, getcommand)
		||
		SourceIsHome
	)
		return false;

	sp = StripTypes(sp);

#	if	TRUNCDIRNAME == 1
	sp = strncpy(sbuf, sp, 14);
	sp[14] = '\0';
#	endif	/* TRUNCDIRNAME == 1 */

	pp = concat(SPOOLDIR, STATEDIR, ERRORSDIR, sp, NULLSTR);
	(void)unlink(pp);

	if ( state(pp) )
		inform = true;

	if
	(
		stat(pp, &statb) != SYSERROR
		&&
		statb.st_size == 0
	)
		(void)unlink(pp);

	free(pp);

	(void)ReRoute(REROUTEDIR, ROUTINGDIR);

	if ( !KeepState )
		return inform;

	if ( MissingType(HdrSource) )
	{
		Trace2(2, "MissingType(%s) => TRUE", HdrSource);

		xp = StripTypes(HdrSource);
		stripped = true;
	}
	else
	{
		Trace2(3, "MissingType(%s) => FALSE", HdrSource);

		xp = newstr(HdrSource);
		stripped = false;
	}

	if ( (sp = DomainToPath(xp)) == NULLSTR )
	{
		free(xp);
		return inform;
	}

	free(xp);
	xp = concat(SPOOLDIR, STATEDIR, MSGSDIR, NULLSTR);

	pp = concat(xp, sp, NULLSTR);

	Trace2(2, "stat(%s)", pp);

	if ( stat(pp, &statb) != SYSERROR )
	{
		Trace2(3, "stat(%s) => EXISTS", pp);

		if ( statb.st_mtime > StartTime && (statb.st_mode & S_IFMT) != S_IFDIR )
			goto out;	/* Don't overwrite newer data */

		if ( unlink(pp) == SYSERROR )
			whinge(pp, UnlinkStr);
	}
	else
	if ( errno == ENOTDIR )
	{
		whinge(pp, CreateStr);

		if ( NETADMIN >= 2 && SName != NULLSTR )
			inform = true;	/* Indicate site that has renamed itself */
	}
	else
	if ( errno == ENOENT && NETADMIN >= 2 && SName != NULLSTR )
		inform = true;	/* Indicate first state message from each site */

	if ( (fd = createn(pp)) != SYSERROR )
	{
		char *	lp;

		Trace2(3, "created %s", pp);

		if ( NETADMIN >= 1 )
			(void)CollectData(&DataCmds, (Ulong)0, MesgLength, fd, pp);
		else
			(void)CollectData(&DataCmds, (Ulong)0, DataLength, fd, pp);
		(void)close(fd);
		(void)chmod(pp, 0640);	/* Enable read by group */
		if ( StartTime != 0 )
			SMdate(pp, StartTime);

		/** Link to untyped name **/

		if ( !stripped )
		{
			lp = StripTypes(HdrSource);
			free(sp);
			sp = DomainToPath(lp);
			free(lp);
			lp = concat(xp, sp, NULLSTR);

			Trace3(3, "compare %s <> %s", pp, lp);
			if ( strcmp(pp, lp) != STREQUAL )
			{
				Trace3(3, "link %s -> %s", pp, lp);

				(void)unlink(lp);
				while ( link(pp, lp) == SYSERROR )
					if
					(
						!CheckDirs(lp)
						&&
						!SysWarn("Can't link \"%s\" to \"%s\"", pp, lp)
					)
					{
						whinge(lp, "link");
						break;
					}
			}
			free(lp);
		}
	}
	else
		whinge(pp, CreateStr);

out:
	free(pp);
	free(xp);
	free(sp);

	return inform;
}



/*
**	Reply to a request for local statefile.
*/

void
reply()
{
	register int	fd;
	register CmdV	cmdv;
	register char *	exf;
	Addr *		ap;
	CmdHead		commands;
	struct stat	statb;

	if ( (exf = GetEnv(ENV_FLAGS)) != NULLSTR && strcmp(exf, STATE_ENQ_ALL) == STREQUAL )
		exf = STATEFILE;	/* Return all state info. */
	else
	if ( Broadcast )
		exf = SITEFILE;		/* Return site-specific state info. for broadcast request */
	else
		exf = EXPORTFILE;	/* Return condensed state info. for specific request */

	exf = concat(SPOOLDIR, STATEDIR, exf, NULLSTR);
	TmpExp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	while ( stat(exf, &statb) == SYSERROR )
		if ( !SysWarn(CouldNot, StatStr, exf) )
		{
			FreeStr(&TmpExp);
			free(exf);
			return;
		}

	if ( (DataLength = statb.st_size) == 0 )
	{
		Warn(english("%s is zero length"), exf);
		FreeStr(&TmpExp);
		free(exf);
		return;
	}

	make_link(exf, UniqueName(TmpExp, CHOOSE_QUALITY, OPTNAME, (Ulong)1, Time));

	free(exf);

	/*
	**	Make the header.
	*/

	HdrFile = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	CmdsTmp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	CmdsFile = concat(SPOOLDIR, ROUTINGDIR, Template, NULLSTR);

	fd = create(UniqueName(HdrFile, CHOOSE_QUALITY, OPTNAME, DataLength, Time));

	if ( LinkName != NULLSTR )
	{
		ap = StringToAddr(exf = concat(LinkName, Explicit, HdrSource, NULLSTR), STA_MAP);
		HdrDest = TypedAddress(ap);
	}
	else
	{
		ap = (Addr *)0;
		HdrDest =  HdrSource;
	}

	HdrEnv = MakeEnv(ENV_ID, HdrID, NULLSTR);	/* Old ID saved */
	HdrFlags = HDR_NO_AUTOCALL|HDR_NORET|HDR_REV_CHARGE;
	HdrHandler = STATEHANDLER;
	HdrID = HdrFile + strlen(SPOOLDIR) + strlen(WORKDIR);
	HdrPart = NULLSTR;
	HdrQuality = HdrID[0];	/* Reply messages travel second-class */
	HdrRoute = NULLSTR;
	HdrSource = HomeAddress;
	HdrSubpt = STATE_PROTO;
	HdrTt = 0;
	HdrTtd = 0;		/* No timeout on replies */
	HdrType = HDR_TYPE2;

	while ( WriteHeader(fd, (long)0, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, HdrFile);

	(void)close(fd);

	/*
	**	Make commands file describing message.
	*/

	InitCmds(&commands);

	(void)AddCmd(&commands, file_cmd, (cmdv.cv_name = TmpExp, cmdv));
	(void)AddCmd(&commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&commands, end_cmd, (cmdv.cv_number = DataLength, cmdv));
	(void)AddCmd(&commands, unlink_cmd, (cmdv.cv_name = TmpExp, cmdv));

	(void)AddCmd(&commands, file_cmd, (cmdv.cv_name = HdrFile, cmdv));
	(void)AddCmd(&commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&commands, end_cmd, (cmdv.cv_number = HdrLength, cmdv));
	(void)AddCmd(&commands, unlink_cmd, (cmdv.cv_name = HdrFile, cmdv));

	/*
	**	Write the commands into a temporary file.
	*/

	fd = create(UniqueName(CmdsTmp, CHOOSE_QUALITY, OPTNAME, (DataLength+HdrLength), Time));

	(void)WriteCmds(&commands, fd, CmdsTmp);

	(void)close(fd);

	FreeCmds(&commands);

	/*
	**	Queue commands for processing by router.
	*/

#	if	PRIORITY_ROUTING == 1
	move(CmdsTmp, UniqueName(CmdsFile, CHOOSE_QUALITY, OPTNAME, (DataLength+HdrLength), Time));
#	else	/* PRIORITY_ROUTING == 1 */
	move(CmdsTmp, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time));
#	endif	/* PRIORITY_ROUTING == 1 */

	if ( Parent )
		(void)kill(Parent, SIGWAKEUP);	/* Wakeup needed if we are invoked by sub-router */

	Trace2(1, "queue_commands in \"%s\"", CmdsFile);

	/*
	**	Prevent error from clobbering.
	*/

	if ( ap != (Addr *)0 )
	{
		FreeAddr(&ap);
		free(exf);
	}

	FreeStr(&CmdsFile);
	FreeStr(&CmdsTmp);
	FreeStr(&HdrFile);
	FreeStr(&TmpExp);
}



void
sigcatch(sig)
	int	sig;
{
	if ( ChildPid != 0 )
		(void)kill(ChildPid, SIGTERM);
}



/*
**	State change processing, pass data to `STATE'.
*/

bool
state(errfile)
	char *		errfile;
{
	register int	ofd  = 0;
	register char *	cp;
	register char *	oname = NULLSTR;
	register FILE *	fd;
	register char *	errs;
	Addr *		ap;
	Link		slink;
	bool		retval = false;
	char		stime[TIME_SIZE+1];
	ExBuf		ea;

	(void)sprintf(stime, "%lu", (PUlong)StartTime);

	/*
	**	First check if we can `see' source.
	*/

	if ( NETADMIN >= 1 )
	{
		ap = StringToAddr(cp = StripTypes(HdrSource), NO_STA_MAP);
		oname = newstr(TypedName(ap));
		FreeAddr(&ap);
		free(cp);

		/*
		**	Make a temporary file for details.
		*/
		
		cp = UniqueName(concat(TMPDIR, Template, NULLSTR), CHOOSE_QUALITY, OPTNAME, (Ulong)0, Time);
		ofd = creater(cp);
		(void)unlink(cp);
		free(cp);
	}

	/*
	**	Process the information into state and routing files.
	*/

	FIRSTARG(&ea.ex_cmd) = STATE;
	if ( NETADMIN >= 1 )
		NEXTARG(&ea.ex_cmd) = "-iqrsxC";
	else
		NEXTARG(&ea.ex_cmd) = "-irsxC";
	NEXTARG(&ea.ex_cmd) = "-D";
	NEXTARG(&ea.ex_cmd) = stime;
	NEXTARG(&ea.ex_cmd) = "-F";
	NEXTARG(&ea.ex_cmd) = HdrSource + 1;

	KeepErrFile = errfile;

	if ( NETADMIN >= 1 )
		fd = (FILE *)Execute(&ea, child_alarm, ex_pipo, ofd);
	else
		fd = (FILE *)Execute(&ea, child_alarm, ex_pipe, SYSERROR);

	KeepErrFile = NULLSTR;

	ChildPid = ea.ex_pid;
	(void)signal(SIGALRM, sigcatch);

	if ( !CollectData(&DataCmds, (Ulong)0, DataLength, fileno(fd), PipeStr) )
		(void)kill(ea.ex_pid, SIGTERM);

	if ( (Mail_mesg = ExClose(&ea, fd)) != NULLSTR )
	{
		ChildPid = 0;

		if ( (errs = MailNCC(mail_err, NCC_ADMIN)) != NULLSTR )
		{
			Recover(ert_return);
			Error(StringFmt, errs);
			Recover(ert_finish);
			free(errs);
			RetVal = EX_DATAERR;
		}

		if ( NETADMIN >= 1 )
		{
			free(oname);
			(void)close(ofd);
		}

		free(Mail_mesg);
		return retval;
	}

	ChildPid = 0;

	if ( NETADMIN == 0 )
		return retval;

	/*
	**	Pick up address, comment.
	*/

	(void)lseek(ofd, (long)0, 0);

	if ( (errs = ReadFd(ofd)) != NULLSTR )
	{
		if ( (cp = strchr(errs, '\n')) != NULLSTR )
		{
			*cp++ = '\0';
			SourceComment = newstr(cp);

			if ( (int)strlen(errs) > 0 )
			{
				ap = StringToAddr(errs, NO_STA_MAP);
				cp = newstr(TypedAddress(ap));	/* For SUN III state messages */
				FreeAddr(&ap);
			}
			else
				cp = NULLSTR;
		}

		free(errs);
	}
	else
		cp = NULLSTR;

	/*
	**	Now check if translation of source has changed.
	*/

	if ( !CheckRoute(NOSUMCHECK) )
	{
		if ( cp != NULLSTR )
			HdrSource = cp;

		free(oname);

		(void)close(ofd);
		return retval;
	}

	ap = StringToAddr((errs = StripTypes(HdrSource)), NO_STA_MAP);

	if ( (HdrSource = cp) == NULLSTR )
		HdrSource = newstr(TypedAddress(ap));

	cp = TypedName(ap);

	if ( strcmp(oname, cp) != STREQUAL && strlen(oname) < strlen(cp) )
	{
		NewRegion = true;
		retval = true;
		ChangedRegions = newprintf("%s <> %s", oname, cp);
	}

	free(oname);

	if ( FindAddr(ap, &slink, FASTEST) == wh_link )
		SLink = StripTypes(slink.ln_rname);

	/*
	**	Find first typed domain.
	*/

	if ( (oname = strchr(cp, TYPE_SEP)) != NULLSTR )
	{
		if ( --oname != cp )
		{
			cp = newstr(oname);

			FreeAddr(&ap);

			ap = StringToAddr(cp, NO_STA_MAP);
			SName = ExpTypName(ap);
			free(cp);
		}
		else
			SName = ExpTypName(ap);

		if ( StringMatch(HomeAddress, oname) != NULLSTR )
		{
			FreeStr(&SName);
			NewRegion = false;
		}
	}

	free(errs);
	FreeAddr(&ap);
	(void)close(ofd);

	return retval;
}



/*
**	Check last state message from this site old enough to ignore.
*/

bool
too_frequent()
{
	char *		sp;
	char *		pp;
	struct stat	statb;

	if ( (sp = DomainToPath(HdrSource)) == NULLSTR )
		return true;

	pp = concat(SPOOLDIR, STATEDIR, MSGSDIR, sp, NULLSTR);

	free(sp);

	if ( stat(pp, &statb) == SYSERROR )
	{
		free(pp);
		return false;
	}

	free(pp);

	if ( statb.st_mtime > (StartTime - HOUR) )
		return true;

	return false;
}



void
whinge(file, what)
	char *	file;
	char *	what;
{
	char *	errs;
	char *	cers;

	if ( NETADMIN == 0 )
		return;

	errs = newprintf(CouldNot, what, file);
	Mail_mesg = cers = newprintf("%s: %s", errs, ErrnoStr(errno));
	free(errs);

	if ( (Mail_mesg = MailNCC(mail_unlink, NCC_ADMIN)) != NULLSTR )
	{
		Warn(StringFmt, Mail_mesg);
		free(Mail_mesg);
	}

	free(cers);
}



/*
**	FALSE if any (internal) type missing.
*/

bool
MissingType(s)
	char *		s;
{
	register char *	cp;

	if ( (cp = s) == NULLSTR || cp[0] == '\0' )
		return false;

	do
	{
		if ( cp[0] == DOMAIN_SEP )
			cp++;

		if ( cp[1] != TYPE_SEP )
			return true;
	}
	while
		( (cp = strchr(cp, DOMAIN_SEP)) != NULLSTR );

	return false;
}
