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
**	Process `whois' query from remote site for a user on our system.
*/

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
**	CONFIGURATION
*/

#define	MINPATTERN	2

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
**	Query.
*/

char *	Pattern;		/* Query */
char *	RemUser;		/* Name of user requesting details */

/*
**	Miscellaneous.
*/

CmdHead	Cleanup;		/* Files to be unlinked */
char *	CmdsFile;		/* Queued commands file */
CmdHead	Commands;		/* Describing data part of message when spooling */
CmdHead	DataCmds;		/* Describing data part of message */
char *	MesgData;		/* Name of message file containing data */
bool	NoOpt;			/* Unoptimised message transmission */
int	RetVal	= EX_OK;	/* Value to return to receiver */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	WorkName;		/* Template for spooling files in WORKDIR */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"TRACEFLAG",	(char **)&Traceflag,	PINT},
	{"WHOISARGS",	(char **)&WHOISARGS,	PVECTOR},
	{"WHOISFILE",	&WHOISFILE,		PSTRING},
	{"WHOISPROG",	&WHOISPROG,		PSTRING},
};

#define	Fprintf		(void)fprintf

void	cleanup _FA_((void));
void	forward _FA_((char *));
bool	getcommand _FA_((CmdT, CmdV));
void	mailit _FA_((char *));
void	set_user _FA_((VarArgs *));
void	whois _FA_((char *));



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

	DataName = SenderName = UserName = "query_server";
	DataSize = DataLength;

	StartTime = Time - SavHdrTt;

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
		Error(english("query acknowledged from \"%s\""), ExSourceAddress);

	HdrEnv = SavHdrEnv;

	InitCmds(&Cleanup);
	InitCmds(&DataCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, english("read commands from"), "stdin");

	Recover(ert_finish);

	if ( SavHdrFlags & HDR_RETURNED )
		mailit(GetEnv(ENV_ERR));
	else
	if ( (fp = Forwarded(Name, Name, SavHdrSource)) != (Forward *)0 )
		forward(fp->address);
	else
	if ( SavHdrPart == NULLSTR )
		whois(GetEnv(ENV_FLAGS));
	else
		Error(english("Not expecting multi-part message!"));

	cleanup();

	finish(RetVal);
	return 0;
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
**	Called from the error routines to cleanup.
*/

void
finish(error)
	int	error;
{
	(void)exit(error);
}



/*
**	Forward message to newaddress.
**
**	(`router' will check for routing loops if include original HdrRoute.)
*/

void
forward(newaddress)
	char *		newaddress;
{
	register CmdV	cmdv;
	register int	fd;

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	CmdsFile = concat(SPOOLDIR, ROUTINGDIR, Template, NULLSTR);
	WorkName = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	NoOpt = (bool)((SavHdrFlags & HDR_NOOPT) != 0);

	FreeCmds(&Commands);

	LinkCmds(&DataCmds, &Commands, (Ulong)0, DataLength, WorkName, Time);

	fd = create(UniqueName(WorkName, SavHdrID[0], NoOpt, DataLength, Time));

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
	HdrSubpt = UNK_PROTO;	/* Don't really know what this was */
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

	fd = create(UniqueName(WorkName, HdrQuality, NoOpt, (Ulong)HdrLength, Time));

	(void)WriteCmds(&Commands, fd, WorkName);

	(void)close(fd);

	/*
	**	Queue commands for router.
	*/

#	if	PRIORITY_ROUTING == 1
	move(WorkName, UniqueName(CmdsFile, HdrQuality, NoOpt, (Ulong)HdrLength, StartTime));
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
	register char *	cp;

	if ( type == unlink_cmd )
		(void)AddCmd(&Cleanup, type, val);
		return true;

	cp = AddCmd(&DataCmds, type, val);

	if ( MesgData == NULLSTR && type == file_cmd )
		MesgData = cp;	/* Only expecting one non-header file */

	return true;
}



/*
**	Mail user results of remote query.
*/

void
mailit(mesg)
	char *		mesg;
{
	register char *	cp;
	register FILE *	fd;
	register char *	result;
	ExBuf		exbuf;

	cp = StripErrString(mesg);

	if
	(
		(result = strchr(cp, '\n')) == NULLSTR
		||
		(*result++ = '\0', (int)strlen(cp) < (MINPATTERN+2))
		||
		(UserName = strrchr(cp, ';')) == NULLSTR
		||
		(*UserName++ = '\0', (int)strlen(UserName) < 1)
	)
	{
		result = mesg;

		if ( DataLength == 0 )
		{
			Error("Malformed reply: \"%s\".", result);
			return;
		}

		UserName = ReadFile(MesgData);
		UserName[DataLength] = '\0';

		result = StripErrString(result);
		cp = NULLSTR;
	}

	FIRSTARG(&exbuf.ex_cmd) = BINMAIL;

	if ( StringAtHome(SavHdrSource) )
		ExpandArgs(&exbuf.ex_cmd, BINMAILARGS);

	NEXTARG(&exbuf.ex_cmd) = UserName;

	fd = (FILE *)Execute(&exbuf, set_user, ex_pipe, SYSERROR);

	if ( MAIL_RFC822_HDR )
	{
		Fprintf(fd, "From: %s@%s\n", SenderName, ExSourceAddress);
		Fprintf(fd, "Date: %s", rfc822time(&StartTime));
	}

	if ( MAIL_TO )
		Fprintf(fd, "To: %s@%s\n", UserName, ExHomeAddress);

	Fprintf(fd, english("Subject: Result of your `whois' query at %s\n\n"), ExSourceAddress);

	if ( cp != NULLSTR )
		Fprintf(fd, english("Your query was \"%s\".\n"), cp);

	if ( (int)strlen(result) > 4 )
		Fprintf(fd, english("The result was:-\n\n%s\n"), result);
	else
		Fprintf(fd, english("There was no match.\n"));

	if ( (cp = GetEnv(ENV_ID)) != NULLSTR )
		Fprintf(fd, english("\nThe request ID was %s.\n"), cp);

	if ( (cp = ExClose(&exbuf, fd)) != NULLSTR )
		Error(StringFmt, cp);
}



/*
**	Become network id for local mail delivery.
*/
/*ARGSUSED*/
void
set_user(vap)
	VarArgs *	vap;
{
	SetUser(NetUid, NetGid);
}



/*
**	Check validity of query,
**	run pattern over data,
**	and write result to <stderr>.
*/

void
whois(query)
	char *	query;
{
	FILE *	fd;
	char *	errs;
	ExBuf	exbuf;

	RetVal = EX_TEMPFAIL;	/* Always fails */

	if
	(
		query == NULLSTR
		||
		(UserName = strrchr(query, ';')) == NULLSTR
		||
		(*UserName++ = '\0', (int)strlen(UserName) < 1)
		||
		(int)strlen(query) < MINPATTERN
	)
	{
		if ( (errs = GetEnv(ENV_FLAGS)) == NULLSTR )
			errs = NullStr;
		Fprintf(stderr, english("%s;%s\nMalformed query.\n"), errs, SenderName);
		return;
	}

	if
	(
		WHOISPROG == NULLSTR
		||
		access(WHOISPROG, 1) == SYSERROR
		||
		(WHOISFILE != NULLSTR && access(WHOISFILE, 4) == SYSERROR)
	)
	{
		Fprintf(stderr, english("Information not available.\n"));
		return;
	}

	FIRSTARG(&exbuf.ex_cmd) = WHOISPROG;
	ExpandArgs(&exbuf.ex_cmd, WHOISARGS);
	NEXTARG(&exbuf.ex_cmd) = query;

	if ( WHOISFILE != NULLSTR )
		NEXTARG(&exbuf.ex_cmd) = WHOISFILE;

	Fprintf(stderr, "%s;%s\n", query, UserName);
	(void)fflush(stderr);

	fd = (FILE *)Execute(&exbuf, set_user, ex_pipo, fileno(stderr));

	if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
	{
		if ( ExStatus == 1 )
			Fprintf(stderr, english("No match.\n"));
		else
			Fprintf(stderr, "%s", StripErrString(errs));
	}
}
