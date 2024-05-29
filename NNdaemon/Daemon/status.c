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
**	Manage new status files for NNdaemon.
*/

#define	FILE_CONTROL
#define	STAT_CALL

#include	"global.h"
#include	"debug.h"
#include	"spool.h"

#include	"daemon.h"

#define	MSGIDLENGTH	UNIQUE_NAME_LENGTH

#include	"VCdaemon/packet.h"
#include	"VCdaemon/chnstats.h"
#include	"VCdaemon/channel.h"
#include	"VCdaemon/dmnstats.h"
#include	"VCdaemon/pktstats.h"
#include	"VCdaemon/status.h"


/*
**	Version of statusfile should be:
*/

static char	ChanVersion[]	= CHANVERSION;

/*
**	Declare all data.
*/

static VCstatus	Stati[2];
static int	StatiFd[2];
static char *	StatiFn[2];



/*
**	Read in any old status file and fix up data, or initialise new status.
*/

int
NNInitStatus(out)
	register int		out;
{
	register Chan *		chnp;
	register int		i;
	register VCstatus *	stp;

	stp = &Stati[out];

	StatiFn[out] = out ? WRITERSTATUS : READERSTATUS;

	Trace2(1, "NNInitStatus \"%s\"", StatiFn[out]);

	i = MIN_STATUS_SIZE(Stati[0]);

	if
	(
		(StatiFd[out] = open(StatiFn[out], O_RDWR)) == SYSERROR
		||
		Read(StatiFd[out], (char *)stp, i, StatiFn[out]) != i
		||
		strcmp(stp->st_version, ChanVersion) != STREQUAL
	)
	{
		if ( StatiFd[out] != SYSERROR )
		{
			Report1(english("status file version mis-match"));
			Close(StatiFd[out], StatiFn[out]);
			(void)strclr((char *)stp, i);
		}

		(void)unlink(StatiFn[out]);
		StatiFd[out] = Create(StatiFn[out]);
		(void)chmod(StatiFn[out], 0644);
	}

	stp->st_pid = Pid;
	stp->st_pktdatasize = PktZ;
	(void)strcpy(stp->st_version, ChanVersion);

	stp->st_flags = CHLINKDOWN|CHLINKSTART;
	if ( Cook )
		stp->st_flags |= CHCOOKEDVC;
	if ( UseCrc )
		stp->st_flags |= CHDATACRC16;
#	if	VCDAEMON_STATS == 1
	stp->st_flags |= CHDMNSTATS;
	(void)strclr((char *)stp->st_dmnstats, sizeof(stp->st_dmnstats));
#	endif
#	if	PROTO_STATS == 1
	stp->st_flags |= CHPROSTATS;
	(void)strclr((char *)stp->st_pktstats, sizeof(stp->st_pktstats));
#	endif

	stp->st_start = stp->st_now = Time;	/* Initialise times kept in Status */

	stp->st_act_count = 0;
	stp->st_lastpkt = 0;

	for ( chnp = stp->st_channels, i = 0 ; i < NCHANNELS ; i++, chnp++ )
	{
		chnp->ch_number = i;
		chnp->ch_flags = 0;

		if ( out )
		{
			chnp->ch_partlist = (Part *)0;
			chnp->ch_current = (Part *)0;
			chnp->ch_cntrltime = 0;

			chnp->ch_cmdfilename = NULLSTR;
			chnp->ch_msgidp = NULLSTR;

			chnp->ch_statsout(wch_datanak) = 0;

			if ( chnp->ch_msgid[0] == '\0' )
			{
				chnp->ch_state = CH_IDLE;
				chnp->ch_msgid[0] = '\0';
				chnp->ch_msgaddr = 0;
				chnp->ch_goodaddr = 0;
			}
		}
		else
		{
			if ( chnp->ch_state == CH_ACTIVE || chnp->ch_state == CH_ENDING )
				chnp->ch_state = CH_STARTING;

			chnp->ch_gaplist = (Gap *)0;
			chnp->ch_eomcount = 0;

			chnp->ch_msgfilename = NULLSTR;
			chnp->ch_msgidp = NULLSTR;

			chnp->ch_statsin(rch_datanak) = 0;
		}

		chnp->ch_msgid[MSGIDLENGTH] = '\0';

		chnp->ch_databuffer = NULLSTR;
		chnp->ch_bufstart = 0;
		chnp->ch_bufend = 0;
	}

	return 0;
}



/*
**	Change a message ID.
*/

void
StID(out, chan, id, size, posn, stime)
	int		out;
	int		chan;
	char *		id;
	long		size;
	long		posn;
	Time_t		stime;
{
	register Chan *	chnp = &Stati[out].st_channels[chan];

	(void)strcpy(chnp->ch_msgid, id);
	chnp->ch_msgsize = size;
	chnp->ch_goodaddr = posn;
	chnp->ch_msgaddr = posn;
	chnp->ch_msgtime = stime;
}



/*
**	Increment counters for a channel.
*/

void
StIncChan(out, chan, count)
	int			out;
	int			chan;
	int			count;
{
	register VCstatus *	stp = &Stati[out];
	register Chan *		chnp = &stp->st_channels[chan];

	chnp->ch_msgaddr += count;
	chnp->ch_goodaddr = chnp->ch_msgaddr;
	if ( out )
		chnp->ch_statsout(wch_data) += count;
	else
		chnp->ch_statsin(rch_data) += count;
	stp->st_lastpkt = Time;
	stp->st_dmnstats[DS_DATA] += count;
}



/*
**	Increment message counts for a channel.
*/

void
StIncMsg(out, chan)
	int	out;
	int	chan;
{
	if ( out )
		Stati[STOUTCHAN].st_channels[chan].ch_statsout(wch_messages)++;
	else
		Stati[STINCHAN].st_channels[chan].ch_statsin(rch_messages)++;
	Stati[out].st_dmnstats[DS_MESSAGES]++;
}



/*
**	Change link state.
*/

void
StLinkState(up)
	int	up;
{
	if ( up )
	{
		Stati[0].st_flags &= ~(CHLINKDOWN|CHLINKSTART);
		Stati[1].st_flags &= ~(CHLINKDOWN|CHLINKSTART);
	}
	else
	{
		Stati[0].st_flags |= CHLINKDOWN;
		Stati[1].st_flags |= CHLINKDOWN;
	}
}



/*
**	Change channel state.
*/

void
StChState(out, chan, state)
	int	out;
	int	chan;
	ChState	state;
{
	Stati[out].st_channels[chan].ch_state = (CHstate)state;
}



/*
**	Set VC data rates.
*/

void
StRateTime(bytes, rtime)
	Ulong	bytes;
	Ulong	rtime;
{
	rtime *= 2;	/* 1/2 in each direction */
	Stati[0].st_act_rawbytes = bytes;
	Stati[1].st_act_rawbytes = bytes;
	Stati[0].st_act_bytes = bytes;
	Stati[1].st_act_bytes = bytes;
	Stati[0].st_act_time = rtime;
	Stati[1].st_act_time = rtime;
	Stati[0].st_act_count++;
	Stati[1].st_act_count++;
}



/*
**	Increment VC data counts.
*/

void
StVCdata(out, count)
	int	out;
	int	count;
{
	Stati[out].st_dmnstats[DS_VCDATA] += count;
}



/*
**	Increment VC output error.
*/

void
StVCnak(out)
	int	out;
{
	if ( out )
		Stati[STOUTCHAN].st_pktstats[PS_BADDCRC]++;
	else
		Stati[STINCHAN].st_pktstats[PS_BADDCRC]++;
}



/*
**	Increment VC packet counts.
*/

void
StVCpkt(out)
	int	out;
{
	if ( out )
		Stati[STOUTCHAN].st_pktstats[PS_XPKTS]++;
	else
		Stati[STINCHAN].st_pktstats[PS_RPKTS]++;
}



/*
**	Increment VC input error.
*/

void
StVCskip(count)
	int	count;
{
	Stati[STINCHAN].st_pktstats[PS_SKIPBYTE] += count;
}



/*
**	Write out current status.
*/

void
Write_Status()
{
	register int	i;
	Ulong		change_time;
	static Ulong	last_data[2];
	static Time_t	last_time;

	DODEBUG(char	tbuf[DATETIMESTRLEN]);

	Trace4(
		1,
		"Write_Status \"%s\" & \"%s\", Time= %s",
		StatiFn[0], StatiFn[1],
		DateTimeStr(Time, tbuf)+9
	);

	change_time = Time - last_time;

	for ( i = 0 ; i < 2 ; i++ )
	{
		if ( change_time != 0 )
		{
			Stati[i].st_datarate = (Stati[i].st_dmnstats[DS_DATA] - last_data[i]) / change_time;
			last_data[i] = Stati[i].st_dmnstats[DS_DATA];
		}

		Stati[i].st_now = Time;
		(void)Seek(StatiFd[i], (long)0, 0, StatiFn[i]);
		Write(StatiFd[i], (char *)&Stati[i], sizeof(VCstatus), StatiFn[i]);
	}

	last_time = Time;
}
