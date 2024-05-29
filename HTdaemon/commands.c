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


/*
**	Commandfile processing.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	ERRNO

#include	"global.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"spool.h"
#include	"sysexits.h"

#include	"driver.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"pktstats.h"
#include	"status.h"



/*
**	Declare local data for building parts list.
*/

static char *	BadCmdReason;
static Uint	CmdFlags;
static Part *	FirstP;
static Part *	LastP;
static char *	CmdLinkName;
static Ulong	MesgLength;
static Part **	NextPpp	= &FirstP;
static Part *	PartFreeList;
static Ulong	Timeout;

static Part *	NewPart _FA_((void));
static void	SendCmds _FA_((CmdHead *, int, Ulong, Time_t));



/*
**	Queue misunderstood commandsfile in BADDIR.
*/

void
BadMesg(chnp, reason)
	register Chan *	chnp;
	char *		reason;
{
	char *		errs;
	ExBuf		exargs;

	if ( access(chnp->ch_cmdfilename, 0) != SYSERROR )
	{
		register CmdV	cmdv;
		register int	fd;
		VarArgs *	vap;
		CmdHead		cmds;
		struct stat	statb;

		move
		(
			chnp->ch_cmdfilename,
			UniqueName
			(
				BadFile,
				chnp->ch_msgid[0],
				NOOPTNAME,
				(Ulong)chnp->ch_msgsize,
				(Ulong)chnp->ch_msgtime
			)
		);

		if ( (fd = open(BadFile, O_WRITE)) != SYSERROR )
		{
			InitCmds(&cmds);
			(void)AddCmd(&cmds, pid_cmd, (cmdv.cv_number = Pid, cmdv));
			(void)AddCmd(&cmds, bounce_cmd, (cmdv.cv_name = reason, cmdv));

			if ( fstat(fd, &statb) != SYSERROR && Time > statb.st_mtime )
			{
				cmdv.cv_number = Time - statb.st_mtime;
				(void)AddCmd(&cmds, traveltime_cmd, cmdv);
			}

			(void)WriteCmds(&cmds, fd, BadFile);

			(void)close(fd);
			FreeCmds(&cmds);
		}

		Report5(
			english("commandsfile \"%s\" passed to %s in \"%s\", reason \"%s\""),
			chnp->ch_cmdfilename,
			BadHandler,
			BadFile,
			reason
		);

		vap = &exargs.ex_cmd;
		FIRSTARG(vap) = BadHandler;
		NEXTARG(vap) = "-C";
		NEXTARG(vap) = BadFile;
		NEXTARG(vap) = "-E";
		NEXTARG(vap) = reason;
		NEXTARG(vap) = "-H";
		NEXTARG(vap) = HomeAddress;
		NEXTARG(vap) = "-I";
		NEXTARG(vap) = Name;
		NEXTARG(vap) = "-L";
		NEXTARG(vap) = LinkAddress;

		if ( (errs = Execute(&exargs, (Ex_fp)IgnoreSignals, ex_exec, SYSERROR)) != NULLSTR )
		{
			/*
			**	A total disaster!
			*/

			Error(StringFmt, errs);

			finish(EX_OSERR);
		}
	}

	UnlinkParts(chnp, false);	/* Prevent unlinks */
	chnp->ch_state = CH_ENDED;	/* FindMessage() will cleanup */
}



/*
**	Process a command from commands file.
*/

bool
getcommand(type, val)
	CmdT		type;
	CmdV		val;
{
	register Part *	pp;

	switch ( type )
	{
	case end_cmd:
		if ( (pp = LastP) == (Part *)0 )
		{
			BadCmdReason = "\"end\" before \"file\"";
			return false;
		}
		MesgLength += val.cv_number - pp->filestart_part;
		pp->msgend_part = MesgLength;
		Trace5(
			4, "part start %lu end %lu filestart %lu name \"%s\"",
			(PUlong)pp->msgstart_part,
			(PUlong)pp->msgend_part,
			(PUlong)pp->filestart_part,
			pp->filename_part
		);
		break;

	case file_cmd:
		pp = NewPart();
		pp->unlink_part = false;
		pp->filename_part = newstr(val.cv_name);
		break;

	case flags_cmd:
		CmdFlags |= val.cv_number;
		break;

	case linkname_cmd:
		CmdLinkName = newstr(val.cv_name);
		break;

	case start_cmd:
		if ( (pp = LastP) == (Part *)0 )
		{
			BadCmdReason = "\"start\" before \"file\"";
			return false;
		}
		pp->filestart_part = val.cv_number;
		pp->msgstart_part = MesgLength;
		break;

	case timeout_cmd:
		if ( val.cv_number < Timeout )
			Timeout = val.cv_number;
		break;

	case unlink_cmd:
		for ( pp = FirstP ; pp != (Part *)0 ; pp = pp->next_part )
			if ( strcmp(val.cv_name, pp->filename_part) == STREQUAL )
			{
				pp->unlink_part = true;
				return true;
			}

		pp = NewPart();
		pp->filename_part = newstr(val.cv_name);
		pp->unlink_part = true;

	default:
		break;	/* gcc -Wall */
	}

	return true;
}



/*
**	Free Part list.
*/

void
FreeParts(ppp)
	register Part **ppp;
{
	register Part *	pp;

	Trace1(3, "FreeParts");

	while ( (pp = *ppp) != (Part *)0 )
	{
		*ppp = pp->next_part;
		free(pp->filename_part);
		pp->next_part = PartFreeList;
		PartFreeList = pp;
	}
}



/*
**	Pass LOW SPACE message to router.
**
**	(Only called from reader.)
*/

void
LowSpace(chan)
	int		chan;
{
	register CmdV	cmdv;
	CmdHead		commands;

	InitCmds(&commands);

	(void)AddCmd(&commands, linkname_cmd, (cmdv.cv_name = LinkAddress, cmdv));
	(void)AddCmd(&commands, pid_cmd, (cmdv.cv_number = Pid, cmdv));
	(void)AddCmd(&commands, flags_cmd, (cmdv.cv_number = LOW_SPACE, cmdv));
	(void)AddCmd(&commands, traveltime_cmd, (cmdv.cv_number = chan, cmdv));

	SendCmds(&commands, STATE_QUALITY, (Ulong)0, Time);
}



/*
**	Allocate new Part into list.
*/

static Part *
NewPart()
{
	register Part *	pp;

	Trace1(3, "NewPart");

	if ( (pp = PartFreeList) != (Part *)0 )
	{
		PartFreeList = pp->next_part;
		(void)strclr((char *)pp, sizeof(Part));
	}
	else
		pp = Talloc0(Part);

	*NextPpp = pp;
	NextPpp = &pp->next_part;
	LastP = pp;

	return pp;
}



/*
**	Pass link up/down message to router.
**
**	(Only called from writer.)
*/

void
NewState(state)
	int		state;
{
	register CmdV	cmdv;
	CmdHead		commands;

	if ( (Status.st_flags & CHLINKDOWN) == state )
		return;

	Status.st_flags &= ~(CHLINKDOWN|CHLINKSTART);
	Status.st_flags |= state;

	Report2(english("New state ==> %s"), (state==CHLINKUP)?english("up"):english("down"));

	if ( BatchTime > 0 && state == CHLINKDOWN )
	{
		FinishReason = &FinRdtimeout;
		Finish = true;
		return;
	}

	if ( !NoUpDown )
	{
		InitCmds(&commands);

		(void)AddCmd(&commands, linkname_cmd, (cmdv.cv_name = LinkAddress, cmdv));
		(void)AddCmd(&commands, pid_cmd, (cmdv.cv_number = Pid, cmdv));
		(void)AddCmd(&commands, flags_cmd, (cmdv.cv_number = (state==CHLINKUP)?LINK_UP:LINK_DOWN, cmdv));

		SendCmds(&commands, STATE_QUALITY, (Ulong)0, Time);
	}

	if ( DieOnDown && state == CHLINKDOWN )
	{
		FinishReason = &FinRdtimeout;
		Finish = true;
	}
}



/*
**	Pass received message to router.
*/

void
RecvMessage(chnp)
	register Chan *	chnp;
{
	register CmdV	cmdv;
	register Ulong	ttime;
	CmdHead		commands;

	Trace3(1, "Receive message \"%s\" on channel %d", chnp->ch_msgid, chnp->ch_number);

	InitCmds(&commands);

	if ( Time > chnp->ch_msgtime )
		ttime = Time - chnp->ch_msgtime;
	else
		ttime = 1;

	(void)AddCmd(&commands, file_cmd, (cmdv.cv_name = chnp->ch_msgfilename, cmdv));
	(void)AddCmd(&commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&commands, end_cmd, (cmdv.cv_number = chnp->ch_msgsize, cmdv));
	(void)AddCmd(&commands, unlink_cmd, (cmdv.cv_name = chnp->ch_msgfilename, cmdv));
	(void)AddCmd(&commands, linkname_cmd, (cmdv.cv_name = LinkAddress, cmdv));
	(void)AddCmd(&commands, traveltime_cmd, (cmdv.cv_number = ttime, cmdv));
	(void)AddCmd(&commands, pid_cmd, (cmdv.cv_number = Pid, cmdv));

	if ( chnp->ch_flags & CH_RDUP )
		(void)AddCmd(&commands, flags_cmd, (cmdv.cv_number = MESG_DUP, cmdv));

	SendCmds(&commands, chnp->ch_msgid[0], (Ulong)chnp->ch_msgsize, chnp->ch_msgtime);
}



/*
**	Queue timed-out commandsfile in ROUTEDIR for returning.
*/

void
RouteMesg(chnp)
	Chan *	chnp;
{
	char *	rn;

#	if	PRIORITY_ROUTING == 1
	rn = UniqueName(CmdsRouteFile, chnp->ch_msgid[0], NOOPTNAME, MesgLength, RdFileTime);
#	else	/* PRIORITY_ROUTING == 1 */
	rn = UniqueName(CmdsRouteFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, RdFileTime);
#	endif	/* PRIORITY_ROUTING == 1 */
	move(chnp->ch_cmdfilename, rn);
	(void)DaemonActive(RouterDir, true);
}



/*
**	Pass commands to router.
*/

static void
SendCmds(cmdh, qual, size, time)
	CmdHead *	cmdh;
	int		qual;
	Ulong		size;
	Time_t		time;
{
	register int	fd;

	/*
	**	Make a command file for this message in a temporary directory.
	*/

	fd = Create(UniqueName(CmdsTempFile, CHOOSE_QUALITY, OPTNAME, size, time));

	if ( !WriteCmds(cmdh, fd, CmdsTempFile) )
		Fatal3(CouldNot, english("write commands to"), CmdsTempFile);

	Close(fd, CLOSE_SYNC, CmdsTempFile);

	FreeCmds(cmdh);

	/*
	**	Move commands into routing directory, and wakeup router.
	*/

#	if	PRIORITY_ROUTING == 1
	if ( qual == STATE_QUALITY )
		size = 0;	/* State messages */
#	else	/* PRIORITY_ROUTING == 1 */
	qual = STATE_QUALITY;
	time = Time;
	size = 0;
#	endif	/* PRIORITY_ROUTING == 1 */

	move(CmdsTempFile, UniqueName(CmdsRouteFile, qual, NOOPTNAME, size, time));

	(void)DaemonActive(RouterDir, true);
}



/*
**	Set new channel message parts list into Channel.
*/

bool
SetupParts(chnp)
	register Chan *	chnp;
{
	bool		val;
	extern Time_t	RdFileTime;

	Trace2(1, "SetupParts from \"%s\"", chnp->ch_cmdfilename);

	FreeParts(&chnp->ch_partlist);

	CmdFlags = 0;
	MesgLength = 0;
	Timeout = MAXTIMEOUT;
	FreeStr(&CmdLinkName);
	BadCmdReason = english("bad format");

	if ( (val = ReadCmds(chnp->ch_cmdfilename, getcommand)) == false )
	{
		char	errs[100];

		if ( errno != ENOENT )
			(void)SysWarn(CouldNot, english("read commands in"), chnp->ch_cmdfilename);
		(void)sprintf(errs, english("bad commands: %s (%s)"), BadCmdReason, ErrnoStr(errno));
		BadMesg(chnp, errs);
		FreeParts(&FirstP);	/* Leave files for inspection */
	}
	else
	if ( CmdLinkName == NULLSTR || strcmp(CmdLinkName, LinkAddress) != STREQUAL )
	{
		val = false;
		Report3("Bad link name \"%s\" in \"%s\"", CmdLinkName, chnp->ch_cmdfilename);
		BadMesg(chnp, english("bad link name"));
		FreeParts(&FirstP);	/* Leave files for inspection */
	}
	else
	{
		Trace4(
			2, "Message setup on channel %d size %lu addr %lu",
			chnp->ch_number, (PUlong)MesgLength, (PUlong)chnp->ch_msgaddr
		);

		chnp->ch_msgtime = RdFileTime;
		chnp->ch_msgsize = MesgLength;

		if ( (Timeout += RdFileTime) <= Time )
		{
			DODEBUG(char	buf[TIMESTRLEN]);

			Trace3(
				1, "Message \"%s\" timed out by %s",
				chnp->ch_msgid, TimeString(Time-Timeout, buf)
			);

			if ( CmdFlags & NAK_ON_TIMEOUT )
			{
				RouteMesg(chnp);
				FreeParts(&FirstP);	/* Leave message files for returning */
			}

			val = false;	/* FindMessage() will cleanup */
		}
	}

	chnp->ch_partlist = FirstP;
	chnp->ch_current = FirstP;
	
	FirstP = (Part *)0;
	LastP = (Part *)0;
	NextPpp = &FirstP;

	return val;
}



/*
**	Unlink message parts.
*/

void
UnlinkParts(chnp, unlnk)
	Chan *		chnp;
	bool		unlnk;
{
	register Part *	pp;

	for ( pp = chnp->ch_partlist ; pp != (Part *)0 ; pp = pp->next_part )
		if ( pp->unlink_part )
		{
			if ( unlnk )
			{
				Trace2(2, "unlink message part \"%s\"", pp->filename_part);
				(void)unlink(pp->filename_part);
			}
			pp->unlink_part = false;
		}
}
