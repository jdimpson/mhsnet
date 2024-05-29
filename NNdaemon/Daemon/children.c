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
**	Routines that handle child processes
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	STDIO

#include	"global.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"spool.h"
#include	"sysexits.h"

#define	BM_DATA
#include	"AQ.h"
#include	"bad.h"
#include	"daemon.h"
#include	"Stream.h"
#include	"driver.h"


extern void	closeall _FA_((VarArgs *));
extern void	SendCmds _FA_((CmdHead *, char, Ulong, Time_t));



/*
**	Terminate reception, pass message to handler.
*/

void
EndMessage(chan)
	AQarg		chan;
{
	register CmdV	cmdv;
	register Ulong	ttime;
	CmdHead		commands;
	register Str_p	strp = &inStreams[(int)chan];

	Trace3(1, "EndMessage for channel %d state %d", (int)chan, strp->str_state);

	NNstate.allmessages++;	/* Let driver know things are progressing */

	Trace3(1, "Receive message \"%s\" on channel %d", strp->str_id, (int)chan);

	InitCmds(&commands);

	if ( Time <= strp->str_time )
		ttime = 1;
	else
		ttime = Time - strp->str_time;

	(void)AddCmd(&commands, file_cmd, (cmdv.cv_name = strp->str_fname, cmdv));
	(void)AddCmd(&commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&commands, end_cmd, (cmdv.cv_number = strp->str_size, cmdv));
	(void)AddCmd(&commands, unlink_cmd, (cmdv.cv_name = strp->str_fname, cmdv));
	(void)AddCmd(&commands, linkname_cmd, (cmdv.cv_name = LinkNode, cmdv));
	(void)AddCmd(&commands, traveltime_cmd, (cmdv.cv_number = ttime, cmdv));
	(void)AddCmd(&commands, pid_cmd, (cmdv.cv_number = Pid, cmdv));

	if ( strp->str_flags & STR_DUP )
		(void)AddCmd(&commands, flags_cmd, (cmdv.cv_number = MESG_DUP, cmdv));

	SendCmds(&commands, strp->str_recv.rh_id[0], (Ulong)strp->str_size, strp->str_time);

	SndEOMA((int)chan);

	strp->str_messages++;
	strp->str_bytes += strp->str_posn;
	strp->str_flags = 0;
	StIncMsg(STINCHAN, chan);

	Update(up_force);
}



/*
**	Move offending command file to bad directory,
**	and set bad message handler onto it.
**	Then reset the channel and queue NewMessage.
*/

void
BadMessage(chan, reason)
	AQarg		chan;
	BMReason	reason;
{
	register Str_p	strp = &outStreams[(int)chan];
	char *		mesg;
	ExBuf		exargs;

	Trace3(1, "BadMessage for channel %d state %d", (int)chan, strp->str_state);

	mesg = BMExplanations[(int)reason];

	Report4("bad message on chan %d \"%s\" - %s", chan, strp->str_cmdf, mesg);

	if ( reason != bm_nodata && access(strp->str_cmdf, 0) == 0 )
	{
		register CmdV	cmdv;
		register int	fd;
		VarArgs *	vap;
		CmdHead		cmds;
		struct stat	statb;
		char *		badname;

		if ( (badname = strrchr(strp->str_cmdf, '/')) != NULLSTR )
			badname++;
		else
			badname = strp->str_cmdf;
		badname = concat(SPOOLDIR, BADDIR, badname, NULLSTR);

		move
		(
			strp->str_cmdf,
			UniqueName
			(
				badname,
				strp->str_cmdid[0],
				NOOPTNAME,
				(Ulong)strp->str_size,
				(Ulong)strp->str_time
			)
		);

		if ( (fd = open(badname, O_WRITE)) != SYSERROR )
		{
			InitCmds(&cmds);
			(void)AddCmd(&cmds, pid_cmd, (cmdv.cv_number = Pid, cmdv));
			(void)AddCmd(&cmds, bounce_cmd, (cmdv.cv_name = mesg, cmdv));

			if ( fstat(fd, &statb) != SYSERROR )
			{
				cmdv.cv_number = Time - statb.st_mtime;
				(void)AddCmd(&cmds, traveltime_cmd, cmdv);
			}

			(void)WriteCmds(&cmds, fd, badname);

			(void)close(fd);
			FreeCmds(&cmds);
		}

		Report3("bad message \"%s\" moved to \"%s\"", strp->str_cmdf, badname);

		vap = &exargs.ex_cmd;
		FIRSTARG(vap) = BadHandler;
		NEXTARG(vap) = "-C";
		NEXTARG(vap) = badname;
		NEXTARG(vap) = "-E";
		NEXTARG(vap) = mesg;
		NEXTARG(vap) = "-H";
		NEXTARG(vap) = HomeNode;
		NEXTARG(vap) = "-I";
		NEXTARG(vap) = Name;
		NEXTARG(vap) = "-L";
		NEXTARG(vap) = LinkNode;

		if ( (mesg = Execute(&exargs, closeall, ex_exec, SYSERROR)) != NULLSTR )
		{
			/*
			**	A total disaster!
			*/

			Error(StringFmt, mesg);

			finish(EX_OSERR);
		}

		free(badname);
	}
	else
		Report2("bad message \"%s\" non-existent", strp->str_cmdf);

	strp->str_state = STR_ERROR;
}
