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
**	General purpose handler for local delivery of non-standard FTP messages.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	RECOVER
#define	SIGNALS
#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"forward.h"
#include	"ftheader.h"
#include	"header.h"
#include	"params.h"
#include	"spool.h"
#include	"sub_proto.h"
#include	"sysexits.h"

/*
**	Because ExpandArgs() not called,
**	and some loaders don't load bss:
*/
#undef	Extern
#define	Extern
#define	ExternU
#include	"expand.h"

#include	<ndir.h>

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
**	Miscellaneous
*/

char *	AckMessage;		/* Message for ACK */
unsigned Alarm;			/* Original ALARM setting - passed to children */
char *	CmdsFile;		/* Queued commands file */
CmdHead	Commands;		/* Describing data part of message when spooling */
CmdHead	Cleanup;		/* Files to be unlinked */
CmdHead	DataCmds;		/* Describing data part of message */
char *	HandlerProg;		/* Name of handler control program */
char *	HomeUnTypedName;	/* Untyped name of this node */
Ulong	MesgLength;		/* Length of whole message */
bool	NoOpt;			/* Unoptimised message transmission */
CmdHead	PartCmds;		/* Describing all parts received so far */
int	RetVal		= EX_OK;
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	WorkName;		/* Template for spooling files in WORKDIR */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"HANDLERPROG",		&HandlerProg,			PSPOOL},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};


void	child_alarm _FA_((VarArgs *));
void	cleanup _FA_((void));
void	forward _FA_((char *));
bool	getcommand _FA_((CmdT, CmdV));
void	process _FA_((void));
void	sendack _FA_((void));



void
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register int	fd;
	Forward *	fp;

	if ( Alarm = alarm(0) )
		(void)alarm(Alarm+5);

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

	if ( HandlerProg == NULLSTR )
		HandlerProg = concat(SPOOLDIR, LIBDIR, Name, ".sh", NULLSTR);

	HdrID = SavHdrID;
	HdrEnv = SavHdrEnv;
	HdrFlags = SavHdrFlags;
	HdrPart = SavHdrPart;
	HdrRoute = SavHdrRoute;
	HdrSource = SavHdrSource;
	HdrTt = SavHdrTt;
	HdrTtd = SavHdrDt;

	StartTime = Time - SavHdrTt;

	NoOpt = (bool)((SavHdrFlags & HDR_NOOPT) != 0);
	if ( !(Returned = (bool)((SavHdrFlags & HDR_RETURNED) != 0)) )
	{
		Ack = (bool)((SavHdrFlags & HDR_ACK) != 0);
		ReqAck = (bool)((SavHdrFlags & HDR_RQA) != 0);
	}

	ExHomeAddress = StripTypes(HomeAddress);
	ExSourceAddress = StripTypes(SavHdrSource);

	InitCmds(&Cleanup);
	InitCmds(&Commands);
	InitCmds(&DataCmds);
	InitCmds(&PartCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, english("read commands from"), "stdin");

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	CmdsFile = concat(SPOOLDIR, ROUTINGDIR, Template, NULLSTR);
	WorkName = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	if ( !Returned && (fp = Forwarded(Name, Name, SavHdrSource)) != (Forward *)0 )
		forward(fp->address);
	else
	if
	(
		Returned
		||
		HdrPart == NULLSTR
		||
		AllParts(&DataCmds, (Ulong)0, DataLength, MesgLength, &PartCmds, WorkName, Name)
	)
	{
		if ( !Returned && HdrPart != NULLSTR )
		{
			FreeCmds(&DataCmds);
			DataLength = 0;	/* Included in PartsLength */
		}

		DataSize = DataLength + PartsLength;

		process();
	}

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

	for ( cmdp = PartCmds.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
			(void)unlink(cmdp->cd_value);

	for ( cmdp = Cleanup.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
			(void)unlink(cmdp->cd_value);
}



/*
**	Called from the errors routines to cleanup
*/

void
finish(error)
	int	error;
{
	if ( AckMessage != NULLSTR )
		(void)unlink(AckMessage);

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
	}

	(void)AddCmd(&DataCmds, type, val);

	return true;
}



/*
**	Pass data to HandlerProg,
**	and acknowledge the data if requested.
*/

void
process()
{
	register FILE *		fd;
	register char *		cp;
	register char *		errs;
	ExBuf			exbuf;
	char			numbuf[NUMERICARGLEN];

	Recover(ert_return);

	FIRSTARG(&exbuf.ex_cmd) = HandlerProg;

	if ( Returned )
		NEXTARG(&exbuf.ex_cmd) = "-r";

	if ( Ack )
		NEXTARG(&exbuf.ex_cmd) = concat("-A", GetEnv(ENV_ID), NULLSTR);

	if ( (errs = GetEnv(ENV_DUP)) != NULLSTR )
	{
		cp = EmptyString;

		do
			cp = concat(cp, StripTypes(errs), ";", NULLSTR);
		while
			( (errs = GetEnvNext(ENV_DUP, errs)) != NULLSTR );

		NEXTARG(&exbuf.ex_cmd) = concat("-D", cp, NULLSTR);
	}

	if ( (errs = GetEnv(ENV_ERR)) != NULLSTR )
		NEXTARG(&exbuf.ex_cmd) = concat("-E", CleanError(errs), NULLSTR);

	if ( (errs = GetEnv(ENV_FLAGS)) != NULLSTR )
	{
		cp = EmptyString;

		do
			cp = concat(cp, errs, ";", NULLSTR);
		while
			( (errs = GetEnvNext(ENV_FLAGS, errs)) != NULLSTR );

		NEXTARG(&exbuf.ex_cmd) = concat("-F", cp, NULLSTR);
	}

	if ( (errs = GetEnv(ENV_ID)) != NULLSTR )
		NEXTARG(&exbuf.ex_cmd) = concat("-I", errs, NULLSTR);

	if ( (errs = GetEnv(ENV_FWD)) != NULLSTR )
	{
		cp = EmptyString;

		do
			cp = concat(cp, StripTypes(errs), ";", NULLSTR);
		while
			( (errs = GetEnvNext(ENV_FWD, errs)) != NULLSTR );

		NEXTARG(&exbuf.ex_cmd) = concat("-W", cp, NULLSTR);
	}

	NEXTARG(&exbuf.ex_cmd) = ExHomeAddress;
	NEXTARG(&exbuf.ex_cmd) = HomeAddress;
	NEXTARG(&exbuf.ex_cmd) = ExSourceAddress;
	NEXTARG(&exbuf.ex_cmd) = SavHdrSource;
	NEXTARG(&exbuf.ex_cmd) = SavHdrID;

	(void)sprintf(numbuf, "%lu", (PUlong)DataSize);
	NEXTARG(&exbuf.ex_cmd) = numbuf;

	fd = (FILE *)Execute(&exbuf, child_alarm, ex_pipe, SYSERROR);

	(void)CollectData(&PartCmds, (Ulong)0, PartsLength, fileno(fd), PipeStr);
	(void)CollectData(&DataCmds, (Ulong)0, DataLength, fileno(fd), PipeStr);

	if ( (errs = ExClose(&exbuf, fd)) != NULLSTR )
	{
		Error(StringFmt, errs);
		free(errs);

		if ( (RetVal = ExStatus & 0377) == 0 )
			RetVal = EX_SOFTWARE;
	}

	if ( ReqAck && RetVal == EX_OK )
		sendack();	/* Send ack */

	Recover(ert_finish);
}



/*
**	Send an acknowledgement message to source.
*/

void
sendack()
{
	register CmdV	cmdv;
	int		fd;

	AckMessage = newstr(WorkName);
	fd = create(UniqueName(AckMessage, CHOOSE_QUALITY, false, (Ulong)DataLength, Time));

	HdrDest = SavHdrSource;
	HdrEnv = MakeEnv(ENV_ID, HdrID, NULLSTR);	/* Old ID saved */
	HdrFlags = HDR_ACK|HDR_NORET;

	if ( NoOpt )
		HdrFlags |= HDR_NOOPT;

	if ( SavHdrFlags & HDR_REGISTERED )
		HdrFlags |= HDR_REGISTERED;

	HdrHandler = Name;
	HdrID = AckMessage + strlen(SPOOLDIR) + strlen(WORKDIR);
	HdrPart = NULLSTR;
	HdrQuality = HdrID[0];
	HdrRoute = NULLSTR;
	HdrSource = HomeAddress;
	HdrSubpt = UNK_PROTO;	/* Don't really know what this was */
	HdrTt = 0;
	HdrTtd = 0;
	HdrType = HDR_TYPE2;

	while ( WriteHeader(fd, (long)0, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, AckMessage);

	(void)close(fd);

	free(HdrEnv);
	HdrEnv = SavHdrEnv;

	FreeCmds(&Commands);

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = AckMessage, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = HdrLength, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = AckMessage, cmdv));

	fd = create(UniqueName(WorkName, HdrQuality, false, (Ulong)HdrLength, Time));

	(void)WriteCmds(&Commands, fd, WorkName);

	(void)close(fd);

	/*
	**	Queue commands for router.
	*/

#	if	PRIORITY_ROUTING == 1
	move(WorkName, UniqueName(CmdsFile, HdrQuality, NoOpt, (Ulong)HdrLength, Time));
#	else	/* PRIORITY_ROUTING == 1 */
	move(WorkName, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time));
#	endif	/* PRIORITY_ROUTING == 1 */

	if ( Parent )
		(void)kill(Parent, SIGWAKEUP);	/* Wakeup needed if we are invoked by sub-router */
}
