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
**	Routines that deal with command files.
*/

#define	FILE_CONTROL
#define	STAT_CALL

#include	"global.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"spool.h"

#include	"Stream.h"
#include	"daemon.h"
#include	"driver.h"
#include	"AQ.h"
#include	"CQ.h"
#include	"bad.h"


#define	SUN3FILTER	"_lib/filter43"	/* Link `filter' for SUN III links */

extern bool	NotSun3;

static Uint	CmdFlags;
static DQl_p	FirstP;
static DQl_p	LastP;
static char *	CmdLinkName;
static Ulong	MesgLength;
static DQl_p*	NextPp	= &FirstP;
static Ulong	Timeout;

bool		getcommand _FA_((CmdT, CmdV));
void		SendCmds _FA_((CmdHead *, char, Ulong, Time_t));
static void	FreeDQ _FA_((DQl_p *));
static DQl_p	NewDQ _FA_((void));
static void	RouteMesg _FA_((int));

extern int	Psend _FA_((int));



/*
**	Start a new message on channel.
**	Discard old message desciptors, if any.
**	Find new message and send a SOM for it.
*/

void
NewMessage(chan)
	AQarg		chan;
{
	register Str_p	strp = &outStreams[(int)chan];
	register DQl_p	dqlp;
	register DQl_p	next;
	bool		update = false;
	bool		Again = false;
	bool		Unlink;

	Trace3(1, "NewMessage for channel %d state %d", (int)chan, strp->str_state);

	if ( strp->str_state != STR_ENDED && strp->str_state != STR_ERROR )
	{
		if ( strp->str_id[0] != '\0' )
			Again = true;	/* Try to send again */
		Unlink = false;		/* Commands no longer valid */
	}
	else
	if ( access(strp->str_cmdf, 0) == SYSERROR )
		Unlink = false;		/* Commands no longer valid (possibly re-routed) */
	else
		Unlink = true;		/* Message sent ok */

	do
	{
		if ( (dqlp = strp->str_descr.dq_first) != (DQl_p)0 )
		{
			strp->str_descr.dq_first = (DQl_p)0;

			do
			{
				if ( dqlp->ql_range == 0 )
				{
					/*
					**	Special action.
					*/
					if ( dqlp->ql_base == 0 && Unlink )
						/*
						**	Unlink.
						*/
						(void)unlink(dqlp->ql_filename);
				}

				next = dqlp->ql_next;
				free(dqlp->ql_filename);
				dqlp->ql_next = DQfreelist;
				DQfreelist = dqlp;
			}
				while ( (dqlp = next) != (DQl_p)0 );

			update = true;
		}

		if ( !Again && !FindCommand((int)chan) )
		{
			if ( strp->str_state != STR_EMPTY )
				SendMQE((int)chan);

			if ( update )
				Update(up_force);
			return;
		}

		Again = false;
		Unlink = true;
	}
		while ( !RdCommand((int)chan) );

	SendSOM((int)chan);

	StID(STOUTCHAN, (int)chan, strp->str_id, strp->str_size, strp->str_posn, strp->str_time);

	Update(up_force);
}



/*
**	Read a command file into memory, and set up descriptor list.
*/

bool
RdCommand(chan)
	int		chan;
{
	register Str_p	strp = &outStreams[chan];
	bool		val;
	extern Time_t	RdFileTime;

	Trace4(1, "RdCommand \"%s\" for channel %d state %d", strp->str_cmdf, chan, strp->str_state);

	CmdFlags = 0;
	MesgLength = 0;
	Timeout = MAXTIMEOUT;
	FreeStr(&CmdLinkName);

	if ( (val = ReadCmds(strp->str_cmdf, getcommand)) == false )
	{
		Report3(CouldNot, english("read commands in"), strp->str_cmdf);
		BadMessage((AQarg)chan, bm_format);
		FreeDQ(&FirstP);	/* Leave files for inspection */
	}
	else
	if ( CmdLinkName == NULLSTR || strcmp(CmdLinkName, LinkNode) != STREQUAL )
	{
		val = false;
		Report3("Bad link name \"%s\" in \"%s\"", CmdLinkName, strp->str_cmdf);
		BadMessage((AQarg)chan, bm_badlink);
		FreeDQ(&FirstP);	/* Leave files for inspection */
	}
	else
	{
		Trace3(2, "Message setup on channel %d size %lu", chan, (PUlong)MesgLength);

		strp->str_posn = 0;
		strp->str_time = RdFileTime;
		strp->str_size = MesgLength;

		if ( (Timeout += RdFileTime) <= Time )
		{
			char	buf[TIMESTRLEN];

			Trace3(
				1, "Message \"%s\" timed out by %s",
				strp->str_id, TimeString(Time-Timeout, buf)
			);

			if ( CmdFlags & NAK_ON_TIMEOUT )
			{
				RouteMesg(chan);
				FreeDQ(&FirstP);	/* Leave message files for returning */
			}

			val = false;	/* FindMessage() will cleanup */
		}
	}

	strp->str_descr.dq_first = FirstP;
	
	FirstP = (DQl_p)0;
	LastP = (DQl_p)0;
	NextPp = &FirstP;

	return val;
}



/*
**	Process a command from commands file.
*/

bool
getcommand(type, val)
	CmdT		type;
	CmdV		val;
{
	register DQl_p	pp;

	switch ( type )
	{
	case end_cmd:
		if ( (pp = LastP) == (DQl_p)0 )
			return false;
		pp->ql_range = val.cv_number - pp->ql_base;
		MesgLength += pp->ql_range;
		break;

	case file_cmd:
		pp = NewDQ();
		pp->ql_filename = newstr(val.cv_name);
		pp->ql_range = 1;	/* Mark as non-special */
		break;

	case flags_cmd:
		CmdFlags |= val.cv_number;
		break;

	case linkname_cmd:
		CmdLinkName = newstr(val.cv_name);
		break;

	case start_cmd:
		if ( (pp = LastP) == (DQl_p)0 )
			return false;
		pp->ql_base = val.cv_number;
		break;

	case timeout_cmd:
		if ( val.cv_number < Timeout )
			Timeout = val.cv_number;
		break;

	case unlink_cmd:
		pp = NewDQ();
		pp->ql_filename = newstr(val.cv_name);
		pp->ql_base = 0;	/* Means  */
		pp->ql_range = 0;	/* Unlink */

	default:
		break;	/* gcc -Wall */
	}

	return true;
}



/*
**	Free Part list.
*/

static void
FreeDQ(ppp)
	register DQl_p*	ppp;
{
	register DQl_p	pp;

	Trace1(3, "FreeDQ");

	while ( (pp = *ppp) != (DQl_p)0 )
	{
		*ppp = pp->ql_next;
		free(pp->ql_filename);
		pp->ql_next = DQfreelist;
		DQfreelist = pp;
	}
}



/*
**	Allocate new Part into list.
*/

static DQl_p
NewDQ()
{
	register DQl_p	pp;

	Trace1(3, "NewDQ");

	if ( (pp = DQfreelist) != (DQl_p)0 )
	{
		DQfreelist = pp->ql_next;
		(void)strclr((char *)pp, sizeof(DQel));
	}
	else
		pp = Talloc0(DQel);

	*NextPp = pp;
	NextPp = &pp->ql_next;
	LastP = pp;

	return pp;
}



/*
**	Fetch stream data for a channel,
**	but first see if there are any blocked control messages to send.
**	Called from Precv().
*/

int
fillPkt(chan, data, size)
	int		chan;
	char *		data;
	int		size;
{
	register Str_p	strp = &outStreams[chan];

	Trace6(
		2,
		"fillPkt(%d) for channel %d state %d size %d posn %lu",
		size,
		chan,
		outStreams[chan].str_state,
		outStreams[chan].str_size,
		(PUlong)outStreams[chan].str_posn
	);

	if ( flushCq(chan, false) )
		return 0;

	if
	(
		strp->str_state == STR_ACTIVE
		&&
		(
			size <= strp->str_count
			||
			(size = strp->str_count) > 0
		)
	)
	for(;;)
	{
		register int	n;

		if ( (n = read(strp->str_fd, data, size)) > 0 )
		{
			outByteCount += n;
			strp->str_posn += n;
			if ( (strp->str_count -= n) == 0 )
				qAction(PosnStream, (AQarg)chan);
			StIncChan(STOUTCHAN, chan, n);
			return n;
		}

		if ( n == 0 )
		{
			BadMessage((AQarg)chan, bm_pastEOF);
			return 0;
		}

		Syserror("read on \"%s\" stream %d", strp->str_fname, chan);
		if ( BatchMode )
			finish(SYSERROR);

		(void)lseek(strp->str_fd, strp->str_posn, 0);
	}

	return 0;
}



/*
**	Position a stream in a message.
**	If at the end, send an EOM, set state to END.
*/

void
PosnStream(chan)
	AQarg		chan;
{
	register Str_p	strp = &outStreams[(int)chan];
	register DQl_p	dqlp;
	register long	count = 0;

	Trace3(1, "PosnStream for channel %d state %d", (int)chan, strp->str_state);

	if ( strp->str_fd > 0 )
	{
		(void)close(strp->str_fd);
		strp->str_fd = 0;
	}

	for ( dqlp = strp->str_descr.dq_first ; dqlp != (DQl_p)0 ; dqlp = dqlp->ql_next )
	{
		if ( (count += dqlp->ql_range) > strp->str_posn )
		{
			if ( (strp->str_fd = open(dqlp->ql_filename, O_READ)) == SYSERROR )
			{
				BMReason	reason;

				Report3(
					"cannot open message data file \"%s\" for message \"%s\""
					,dqlp->ql_filename
					,strp->str_cmdf
				);

				strp->str_fd = 0;

				if ( access(dqlp->ql_filename, 0) == 0 )
					reason = bm_readdata;
				else
					reason = bm_nodata;

				BadMessage((AQarg)chan, reason);
				return;
			}

			strp->str_count = count - strp->str_posn;

			(void)lseek(strp->str_fd, dqlp->ql_range + dqlp->ql_base - strp->str_count, 0);

			strp->str_state = STR_ACTIVE;
			StChState(STOUTCHAN, (int)chan, CHN_ACTIVE);

			while ( Psend((int)chan) > 0 );	/* Get the ball rolling ... */

			return;
		}
	}

	if ( count < strp->str_posn )
		Fatal4(
			"\"posn\"(%lu) > \"size\"(%lu) for message \"%s\""
			,(PUlong)strp->str_posn
			,(PUlong)count
			,strp->str_cmdf
		);

	/*
	**	Have reached end of message, send EOM
	*/

	SendEOM((int)chan);
}



/*
**	Pass link up/down message to router.
*/

void
PassState(state)
	int		state;
{
	register CmdV	cmdv;
	CmdHead		commands;

	Report2(english("New state ==> %s"), (state==DLINK_DOWN)?english("down"):english("up"));

	if ( BatchMode && state == DLINK_DOWN )
	{
		Finish = true;
		return;
	}

	if ( !NoUpDown )
	{
		InitCmds(&commands);

		(void)AddCmd(&commands, linkname_cmd, (cmdv.cv_name = LinkNode, cmdv));
		(void)AddCmd(&commands, pid_cmd, (cmdv.cv_number = Pid, cmdv));
		(void)AddCmd(&commands, flags_cmd, (cmdv.cv_number = (state==DLINK_DOWN)?LINK_DOWN:LINK_UP, cmdv));
		if ( !NotSun3 )
			(void)AddCmd(&commands, filter_cmd, (cmdv.cv_name = SUN3FILTER, cmdv));

		SendCmds(&commands, STATE_QUALITY, (Ulong)0, Time);
	}

	if ( DieOnDown && state == DLINK_DOWN )
		Finish = true;
}



/*
**	Queue timed-out commandsfile in ROUTEDIR for returning.
*/

static void
RouteMesg(chan)
	int	chan;
{
	char *	rn;

#	if	PRIORITY_ROUTING == 1
	rn = UniqueName(CmdsRouteFile, outStreams[chan].str_cmdid[0], NOOPTNAME, MesgLength, RdFileTime);
#	else	/* PRIORITY_ROUTING == 1 */
	rn = UniqueName(CmdsRouteFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, RdFileTime);
#	endif	/* PRIORITY_ROUTING == 1 */
	move(outStreams[chan].str_cmdf, rn);
	(void)DaemonActive(RouterDir, true);
}



/*
**	Pass commands to router.
*/

void
#if	ANSI_C
SendCmds(CmdHead * cmdh, char qual, Ulong size, Time_t time)
#else	/* ANSI_C */
SendCmds(cmdh, qual, size, time)
	CmdHead *	cmdh;
	char		qual;
	Ulong		size;
	Time_t		time;
#endif	/* ANSI_C */
{
	register int	fd;

	/*
	**	Make a command file for this message in a temporary directory.
	*/

	fd = create(UniqueName(CmdsTempFile, CHOOSE_QUALITY, OPTNAME, size, time));

	if ( !WriteCmds(cmdh, fd, CmdsTempFile) )
		Fatal3(CouldNot, english("write commands to"), CmdsTempFile);

	Close(fd, CmdsTempFile);

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
