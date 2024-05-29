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
**	Update the status and statistics file.
*/

#define	FILE_CONTROL
#define	STAT_CALL

#include	"global.h"
#include	"debug.h"

#include	"Pconfig.h"

#include	"daemon.h"
#include	"Stream.h"
#include	"AQ.h"
#include	"driver.h"


static int	StateFd;		/* Initialised by "ReadState()" */
long		AllMessages;		/* Messages transferred since the epoch */

#if	0
float		DecayRate	= 0.99711605;	/* 4 minute 1/2 life */
#endif	/* 0 */
#define		DECAYMULT	 997		/* about the same thing */
#define		DECAYDIV	1000
Ulong		RateBytes;			/* Decaying bytes transferred */
Ulong		RateTime	= 1;		/* Decaying time for RateBytes */



void
Update(flag)
	StCom		flag;
{
	register Str_p	strp;
	register int	etime;
	bool		all_done;
	static short	idle_time;
	static short	done_time;

	switch ( flag )
	{
	case up_date:
		if ( NNstate.lasttime == LastTime )
			return;
		break;

	case up_error:
		NNstate.procstate = PROC_ERROR;
		break;

	case up_finish:
		LastTime++;	/* Ensure > last time written */
		NNstate.procid = 0;
		break;

	case up_opening:
		NNstate.procstate = PROC_OPENING;
		break;

	case up_force:
		break;
	}
	
	Trace2(1, "Update(%d)", (int)flag);

	UpdateTime = 0;
	NNstate.thistime = LastTime;
	inByteCount = PinBytes;
	PinBytes = 0;
	outByteCount = PoutBytes;
	PoutBytes = 0;

	if
	(
		(
			StateFd == SYSERROR
			&&
			(StateFd = creat(Statusfile, 0644)) == SYSERROR
		)
		||
		lseek(StateFd, (long)0, 0) == SYSERROR
		||
		write(StateFd, (char *)&NNstate, sizeof NNstate) != sizeof NNstate
	)
	{
		Syserror(Statusfile);
		return;
	}

	if ( flag == up_finish || flag == up_opening || flag == up_error )
	{
		Write_Status();	/* VCdaemon compat. status files. */
		return;
	}

	if ( (etime = LastTime - NNstate.lasttime) > 0 )
	{
		register long	b = outByteCount+inByteCount;

		Trace2(2, "Update etime %d", etime);

		if
		(
			etime > 9
			&&
			(b/etime) > NNstate.maxspeed
		)
			NNstate.maxspeed = b/etime;

		if ( (Receiving || Transmitting) && NNstate.linkstate != DLINK_DOWN )
		{
/*			if ( MinSpeed > 0 )	*/
			{
				register int	t = etime;
	
				RateTime *= 100;	/* Normalise */
	
				while ( t-- > 0 )	/* Non-floating point decay */
				{
					RateBytes *= DECAYMULT;
					RateBytes /= DECAYDIV;
					RateTime *= DECAYMULT;
					RateTime /= DECAYDIV;
				}
	
				RateTime /= 100;
	
				RateTime += etime;
			}

			NNstate.activetime += etime;

			Trace5(
				2,
				"Update allb %lu, actim %lu, rb %lu, rt %lu",
				(PUlong)(NNstate.allbytes + b),
				(PUlong)(NNstate.activetime),
				(PUlong)(RateBytes + b),
				(PUlong)RateTime
			);
		}

		RateBytes += b;
		NNstate.allbytes += b;
		StRateTime(RateBytes, RateTime);

		inByteCount = 0;
		outByteCount = 0;
		NNstate.lasttime = LastTime;
	}

	all_done = true;
	Receiving = 0;

	for ( strp = &inStreams[Fstream] ; strp < &inStreams[Nstreams] ; strp++ )
	{
		if ( strp->str_state == STR_ACTIVE )
			Receiving++;
		else
		if ( strp->str_state == STR_EMPTY || strp->str_state == STR_INACTIVE )
			continue;

		all_done = false;
	}

	Transmitting = 0;

	for ( strp = &outStreams[Fstream] ; strp < &outStreams[Nstreams] ; strp++ )
	{
		if ( strp->str_state == STR_ACTIVE )
			Transmitting++;
		else
		if ( strp->str_state == STR_EMPTY || strp->str_state == STR_INACTIVE )
			continue;

		all_done = false;
	}

	if ( (Receiving || Transmitting) && NNstate.linkstate != DLINK_DOWN )
	{
		idle_time = 0;
		done_time = 0;
		NNstate.procstate = PROC_RUNNING;
	}
	else
	if ( (idle_time += etime) > 0 )
	{
		Trace3(1, "Update IDLE time %d, done_time %d", idle_time, done_time);

		NNstate.procstate = PROC_IDLE;

		if ( all_done )
		{
			done_time += etime;

			if ( BatchMode && done_time >= BatchTime )
			{
				Finish = true;
				Write_Status();	/* VCdaemon compat. status files. */
				return;
			}
		}
		else
		{
			static Ulong	lastall;

			done_time = 0;

			if ( NNstate.allmessages == lastall )
			{
				if ( BatchMode && lastall && idle_time > 240 )
				{
					FinishReason = "Remote SLOW";
					Finish = true;
					Write_Status();	/* VCdaemon compat. status files. */
					return;
				}
			}
			else
				idle_time = 0;

			lastall = NNstate.allmessages;
		}
	}

	SetSpeed();

	Write_Status();	/* VCdaemon compat. status files. */
}



/*
**	Read in the status file and queue function to make consistent.
**	Return old process id.
*/

int
ReadState()
{
	register int	count;
	struct stat	statb;

	Trace1(1, "ReadState()");

	qAction(FixStreams, (AQarg)0);

	if ( access(Statusfile, 6) == SYSERROR )
	{
		StateFd = SYSERROR;
		(void)unlink(Statusfile);
		goto out;
	}

	for
	(
		count = 0
		;
		(StateFd = open(Statusfile, O_RDWR)) == SYSERROR
		;
	)
		if ( ++count > 3 )
			goto out;
		else
			(void)sleep(2);

	while ( (count = StateFd) <= 2 )
	{
		if ( (StateFd = dup(count)) == SYSERROR )
		{
			(void)close(count);
			goto out;
		}

		(void)close(count);

		while ( open(DevNull, O_RDWR) == SYSERROR )
		{
			Syserror(DevNull); /* (you wouldn't believe how silly some systems can be...) */
			if ( BatchMode )
				finish(SYSERROR);
		}
	}

	for
	(
		count = 0
		;
		fstat(StateFd, &statb) == SYSERROR
		||
		statb.st_size != sizeof NNstate
		||
		read(StateFd, (char *)&NNstate, sizeof NNstate) != sizeof NNstate
		;
	)
	{
		if ( ++count > 3 )
			goto out;
		else
		{
			(void)sleep(2);
			(void)lseek(StateFd, (long)0, 0);
		}
	}

	if ( strcmp(NNstate.version, StreamSCCSID) == STREQUAL )
		return NNstate.procid;

out:
	(void)strclr((char *)&NNstate, sizeof NNstate);

	(void)strcpy(NNstate.version, StreamSCCSID);

	if ( StateFd != SYSERROR )
	{
		(void)close(StateFd);
		StateFd = SYSERROR;
	}

	return 0;
}



/*
**	Make state self consistent
*/

void
FixStreams()
{
	register Str_p	strp;
	register int	i;

	Trace1(1, "FixStreams");

	for ( strp = inStreams, i = 0 ; strp < &inStreams[NSTREAMS] ; i++, strp++ )
	{
		AllMessages += strp->str_messages;
		strp->str_flags = 0;

		if ( strp->str_recv.rh_id[0] != '\0' )
		{
			strp->str_fname = concat
					  (
						  WorkDir
						 ,strp->str_recv.rh_id
						 ,NULLSTR
					  );

			if
			(
				strp->str_state == STR_ENDED
				||
				(strp->str_fd = open(strp->str_fname, O_WRITE)) == SYSERROR
			)
			{
				if ( strp->str_state == STR_ENDING )
					strp->str_state = STR_AGAIN;
				else
				if ( strp->str_state != STR_ENDED )
					goto inbad;
				strp->str_fd = 0;
			}
			else
			{
				Trace4(
					2,
					"Found active message \"%s\" for stream %d, id \"%s\"",
					strp->str_fname,
					i,
					strp->str_id
				);

				strp->str_posn = lseek(strp->str_fd, (long)0, 2);

				strp->str_state = STR_ACTIVE;

				StChState(STOUTCHAN, i, CHN_ACTIVE);
				StID(STINCHAN, i, strp->str_id, strp->str_size, strp->str_posn, strp->str_time);
			}
		}
		else
		{
			strp->str_fname = NULLSTR;
inbad:
			strp->str_fd = 0;

			if ( i < Fstream || i >= Nstreams )
				strp->str_state = STR_INACTIVE;
			else
				strp->str_state = STR_IDLE;

			StChState(STINCHAN, i, CHN_IDLE);
		}
	}

	for ( strp = outStreams, i = 0 ; strp < &outStreams[NSTREAMS] ; i++, strp++ )
	{
		AllMessages += strp->str_messages;
		strp->str_descr.dq_first = (DQl_p)0;
		strp->str_fd = 0;
		strp->str_cmdf = concat(CmdsName, Slash, Template, NULLSTR);
		strp->str_cmdfp = strrchr(strp->str_cmdf, '/') + 1;
		(void)strcpy(strp->str_cmdfp, strp->str_cmdid);

		if
		(
			strp->str_id[0] != '\0'
			&&
			RdCommand(i)
		)
		{
			StChState(STOUTCHAN, i, CHN_STARTING);
			StID(STOUTCHAN, i, strp->str_id, strp->str_size, strp->str_posn, strp->str_time);

			strp->str_state = STR_START;
			strp->str_posn = 1;	/* Indicate STR_RESTART */
		}
		else
		{
			strp->str_id[0] = '\0';

			if ( i < Fstream || i >= Nstreams )
				strp->str_state = STR_INACTIVE;
			else
				strp->str_state = STR_IDLE;

			StChState(STOUTCHAN, i, CHN_IDLE);
		}
	}

	(void)NNInitStatus(STINCHAN);	/* VCdaemon compat status files */
	(void)NNInitStatus(STOUTCHAN);

	Update(up_force);
}



/*
**	Execute change-state handler.
**
**	Attempt to dampen rapid changes.
*/

void
NewState(newstate, force)
	int		newstate;
	bool		force;
{
	static Time_t	change_time;
	static int	real_state	= -1;

	Trace4(1, "NewState(%d, %d) real_state=%d", newstate, force, real_state);

	StLinkState((newstate==DLINK_DOWN)?STLINKDOWN:STLINKUP);

	if ( newstate == NNstate.linkstate )
	{
		if
		(
			newstate == real_state
			||
			(
				newstate == DLINK_DOWN
				&&
				!force
				&&
				(
					BatchMode
					||
					(LastTime - change_time) < DOWNTIME
				)
			)
		)
		{
			Trace2(2, "NewState change_time=%lu", (PUlong)change_time);
			return;
		}
	}
	else
	{
		NNstate.linkstate = newstate;

		Update(up_force);

		if ( !force )
		{
			change_time = LastTime;
			return;
		}
	}

	if ( newstate == real_state )
		return;

	PassState(newstate);

	real_state = newstate;
	change_time = LastTime;
}
