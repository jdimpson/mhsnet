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


static char	sccsid[]	= "@(#)reporter.c	1.15 05/12/16";

/*
**	Process news items from message received from network.
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
#include	"exec.h"
#include	"expand.h"
#include	"forward.h"
#include	"header.h"
#include	"params.h"
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
**	Miscellaneous.
*/

unsigned Alarm;			/* Original ALARM setting - passed to children */
CmdHead	Commands;		/* Describing data part of message when spooling */
CmdHead	Cleanup;		/* Files to be unlinked */
int	IgnNewsErr	= NEWSIGNERR;
CmdHead	DataCmds;		/* Describing data part of message */
Ulong	MesgLength;		/* Length of whole message */
bool	NoOpt;			/* No queue optimisation requested */
CmdHead	PartCmds;		/* Describing all parts received so far */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	WorkName;		/* Template for spooling files in WORKDIR */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"NEWSARGS",		(char **)&NEWSARGS,		PVECTOR},
	{"NEWSCMDS",		&NEWSCMDS,			PSPOOL},
	{"NEWSEDITOR",		&NEWSEDITOR,			PSTRING},
	{"NEWSIGNERR",		(char **)&IgnNewsErr,		PINT},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

#define	Fprintf		(void)fprintf

void	child_alarm _FA_((VarArgs *));
void	cleanup _FA_((void));
void	forward _FA_((char *));
bool	getcommand _FA_((CmdT, CmdV));
void	sendinfiles _FA_((void));



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	register Forward *	fp;

	if ( (Alarm = alarm(0)) )
		(void)alarm(Alarm+5);

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

	StartTime = Time - SavHdrTt;

	DataName = SenderName = UserName = "news";

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

	if ( SavHdrFlags & HDR_RETURNED )
		Error(english("news returned from \"%s\""), ExSourceAddress);

	if ( SavHdrFlags & HDR_ACK )
		Error(english("news acknowledged from \"%s\""), ExSourceAddress);

	NoOpt = (bool)((SavHdrFlags & HDR_NOOPT) != 0);

	HdrID = SavHdrID;
	HdrEnv = SavHdrEnv;
	HdrFlags = SavHdrFlags;
	HdrPart = SavHdrPart;
	HdrRoute = SavHdrRoute;
	HdrSource = SavHdrSource;
	HdrTt = SavHdrTt;
	HdrTtd = SavHdrDt;

	InitCmds(&Cleanup);
	InitCmds(&Commands);
	InitCmds(&DataCmds);
	InitCmds(&PartCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, english("read commands from"), "stdin");

	if ( (fp = Forwarded(Name, Name, SavHdrSource)) != (Forward *)0 )
		forward(fp->address);
	else
	if
	(
		NEWSEDITOR != NULLSTR
		&&
		(
			SavHdrPart == NULLSTR
			||
			AllParts(&DataCmds, (Ulong)0, DataLength, MesgLength, &PartCmds, WorkName, Name)
		)
	)
	{
		if ( SavHdrPart != NULLSTR )
		{
			FreeCmds(&DataCmds);	/* All in ``PartCmds'' */
			DataLength = 0;
			DataSize = PartsLength;
		}
		else
			DataSize = DataLength;

		sendinfiles();
	}

	cleanup();

	exit(EX_OK);
	return 0;
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

	for ( cmdp = PartCmds.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
			(void)unlink(cmdp->cd_value);

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
	(void)exit(error);
}



/*
**	Forward news to newaddress.
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

	LinkCmds(&DataCmds, &Commands, (Ulong)0, DataLength, WorkName, StartTime);

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
	static Ulong	filestart;

	switch ( type )
	{
	case end_cmd:
		MesgLength += val.cv_number - filestart;
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
**	Pass data on to the local news editor.
*/

void
sendinfiles()
{
	register char *	cp;
	register FILE *	fd;
	register char *	errs;
	bool		piperr = false;
	ExBuf		exbuf;

	if ( (cp = GetEnv(ENV_FLAGS)) == NULLSTR)
	{
		FIRSTARG(&exbuf.ex_cmd) = NEWSEDITOR;
	}
	else
	{
		register char *	np;
		register char * sp;
		char		buf[BUFSIZ];

		if ( (sp = strpbrk(cp, "\t ")) != NULLSTR )
			*sp++ = '\0';

		NARGS(&exbuf.ex_cmd) = 0;

		if ( NEWSCMDS != NULLSTR )
		{
			if ( (fd = fopen(NEWSCMDS, "r")) == NULL )
			{
				Syserror(CouldNot, OpenStr, NEWSCMDS);
				return;
			}

			while ( fgets(buf, sizeof buf, fd) != NULL )
			{
				if ( buf[0] == '#' )
					continue;

				if ( (np = strchr(buf, '\n')) != NULLSTR )
					*np = '\0';

				if ( (np = strchr(buf, '\t')) == NULLSTR )
					continue;

				*np++ = '\0';

				if ( strcmp(cp, buf) != STREQUAL )
					continue;

				(void)SplitArg(&exbuf.ex_cmd, np);

				break;
			}

			if ( feof(fd) )
			{
				(void)fclose(fd);
				Error(english("Unknown news command \"%s\""), cp);
				return;
			}

			(void)fclose(fd);
		}
		else
			FIRSTARG(&exbuf.ex_cmd) = cp;

		if ( sp != NULLSTR )
			(void)SplitArg(&exbuf.ex_cmd, sp);

		free(cp);
	}

	ExpandArgs(&exbuf.ex_cmd, NEWSARGS);

	fd = (FILE *)Execute(&exbuf, child_alarm, ex_pipe, SYSERROR);

	if ( !CollectData(&PartCmds, (Ulong)0, PartsLength, fileno(fd), "pipe") )
		piperr = true;
	else
	if ( !CollectData(&DataCmds, (Ulong)0, DataLength, fileno(fd), "pipe") )
		piperr = true;

	(void)fflush(fd);

	if ( ferror(fd) )
		piperr = true;

	if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
	{
		if ( !IgnNewsErr || piperr )
			Error(StringFmt, errs);
		free(errs);
	}
}
