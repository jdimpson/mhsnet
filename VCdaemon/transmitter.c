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
**	Routines to process outbound packets.
*/

#define	FILE_CONTROL

#include	"global.h"
#include	"debug.h"
#include	"driver.h"
#include	"packet.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"pktstats.h"
#include	"status.h"


void		RecvBad _FA_((int, int, int, Ulong, Uchar *));
void		RecvEOMA _FA_((int, int, int, Ulong, Uchar *));
void		RecvLEOT _FA_((int, int, int, Ulong, Uchar *));
void		RecvLMQE _FA_((int, int, int, Ulong, Uchar *));
void		RecvNAK _FA_((int, int, int, Ulong, Uchar *));
void		RecvSOMA _FA_((int, int, int, Ulong, Uchar *));

vFuncp		WriteReceivers[MAXPKTTYPE]	=
{
	RecvBad,	/* DATA */
	RecvNAK,
	RecvBad,	/* SOM */
	RecvSOMA,
	RecvBad,	/* EOM */
	RecvEOMA,
	RecvLMQE,
	RecvLEOT
};



/*
**	Check for active outbound channels.
*/

static bool
OutActive()
{
	register Chan *	chnp;
	register int	i;

	for ( chnp = Channels, i = NCHANNELS ; --i >= 0 ; chnp++ )
		if ( chnp->ch_state != CH_ENDED && chnp->ch_state != CH_IDLE )
			return true;

	return false;
}



/*
**	Unexpected packet type received on in-pipe.
*/

/*ARGSUSED*/
void
RecvBad(chan, type, size, addr, data)
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	Fatal5("RecvBad type %d size %d addr %lu on channel %d", type, size, (PUlong)addr, chan);
}



/*
**	Message transmitted ok.
*/

void
RecvEOMA(chan, type, size, addr, data)
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	register Chan *	chnp = &Channels[chan];

#	ifdef	lint
	type = type;
#	endif

	Trace2(1, "RecvEOMA on channel %d", chan);

	if ( size != EOMADATASIZE )
	{
		chnp->ch_statsout(wch_badeoma)++;
		return;
	}

	if ( strncmp(A_EOMAID(data), chnp->ch_msgid, MSGIDLENGTH) != STREQUAL || addr != chnp->ch_msgsize )
	{
		DODEBUG(static char mid[MSGIDLENGTH+1]; (void)strncpy(mid, A_EOMAID(data), MSGIDLENGTH));
		DODEBUG( if ( chnp->ch_state != CH_IDLE && chnp->ch_state != CH_STARTING ) )
			Report7(
				RepFmt, chan, ChnStateStr(chnp),
				english("Unexpected EOMA"),
				mid, (PUlong)chnp->ch_msgsize, (PUlong)addr
			);
		chnp->ch_statsout(wch_unxeoma)++;
		return;
	}

	PacketTime = Time;
	NewState(CHLINKUP);

	if ( chnp->ch_state != CH_ENDING )
	{
		/*
		**	Restarted message already received.
		*/

		chnp->ch_statsout(wch_duprestart)++;
	}
/*	else	Count this anyway */
	{
		DMNSTATSINC(DS_MESSAGES);
		chnp->ch_statsout(wch_messages)++;
	}

	chnp->ch_state = CH_IDLE;		/* FindMessage() will clean up */

	Write_Status();
}



/*
**	EOT from reader -- finish.
*/

/*ARGSUSED*/
void
RecvLEOT(chan, type, size, addr, data)
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	Trace2(1, "RecvLEOT(%d)", chan);

	if ( chan == REOT )
		FinishReason = &FinRemterm;

	Finish = true;
}



/*
**	Control packet from reader:
**
**	SYNC -- start,
**	MQE  -- finish,
**	IDLE -- adjust parameters based on throughput at other end,
**	CTRL -- mark link down or signal no space.
*/

/*ARGSUSED*/
void
RecvLMQE(chan, type, size, addr, data)
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	Trace5(
		1,
		"RecvLMQE %s/%ld for link=%.*s.",
		(chan==SYNC)?"SYNC":(chan==IDLE)?"IDLE":(chan==CTRL)?"CTRL":"MQE",
		(PUlong)addr, size, A_MQELINK(data)
	);

	PacketTime = Time;

	switch ( chan )
	{
	case SYNC:
		Started = true;		/* Both ends have seen SYNC */
		NewState(CHLINKUP);
		break;

	case MQE:
		if ( TurnAround )
		{
			extern char *	StrStatus();

			if
			(
				HomeAdLength == size
				&&
				strnccmp(HomeAddress, A_MQELINK(data), HomeAdLength) == STREQUAL
			)
			{
				OutOnly = false;
				Status.st_flags &= ~CHOUTONLY;
				InOnly = true;
				Status.st_flags |= CHINONLY;
			}
			else
			{
				InOnly = false;
				Status.st_flags &= ~CHINONLY;
				OutOnly = true;
				Status.st_flags |= CHOUTONLY;

				TurnTime = Time + TurnDelay;
			}

			Trace1(1, StrStatus());
			Write_Status();
		}
		break;

	case IDLE:
		AdjPktSize(addr);
		break;

	default:	/* CTRL */
		switch ( (int)addr )
		{
		case CTRL_LINKDOWN:
			NewState(CHLINKDOWN);
			break;

		case CTRL_NOSPACE:
			if ( !OutActive() )
			{
				FinishReason = &FinLowSpace;
				Finish = true;
			}
			break;

		default:
			Report2(english("Unexpected CTRL type %lu"), (PUlong)addr);
		}
	}
}



/*
**	NAK for a cntrl/data packet -- re-transmit it.
*/

void
RecvNAK(chan, type, size, addr, data)
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	register Chan *	chnp = &Channels[chan];
	register Ulong	save_addr;
	register Ulong	gapend;
	register Ulong	len;
	CHflags		save_flags;
	int		save_size;

	type = G_NAKTYPE(data);
	len = G_NAKLENGTH(data);

	Trace5(
		1, "RecvNAK on channel %d for packet type %d addr %lu length %lu",
		chan, type, (PUlong)addr, (PUlong)len
	);

	if ( size != NAKDATASIZE || type != PKTDATATYPE )
	{
		char *	cp;

		if ( size != NAKDATASIZE )
		{
			cp = english("Bad NAK size");
			Report2("RecvNAK pkt size %d", size);
		}
		else
		{
			cp = english("Bad NAK type");
			Report2("RecvNAK for type %d", type);
		}
			
		Report7(
			RepFmt, chan, ChnStateStr(chnp),
			cp, chnp->ch_msgid, (PUlong)len, (PUlong)addr
		);

		chnp->ch_statsout(wch_badnak)++;
		return;
	}

	PacketTime = Time;

	/** Resend data **/

	if ( chnp->ch_state == CH_IDLE || chnp->ch_msgaddr < addr )
	{
		Report7(
			RepFmt, chan, ChnStateStr(chnp),
			english("Unexpected NAK DATA"),
			chnp->ch_msgid,
			(PUlong)len, (PUlong)addr
		);
		chnp->ch_statsout(wch_unxnak)++;
		return;
	}

	if ( (gapend = addr + len) > chnp->ch_msgsize )
	{
		Report7(
			RepFmt, chan, ChnStateStr(chnp),
			english("Bad NAK DATA"),
			chnp->ch_msgid,
			(PUlong)len, (PUlong)addr
		);
		chnp->ch_statsout(wch_badnak)++;
		return;
	}

	if ( chnp->ch_state == CH_ENDED )
		return;

	save_addr = chnp->ch_msgaddr;
	chnp->ch_msgaddr = addr;

	if ( len == chnp->ch_msgsize )
	{
		Report7(
			RepFmt,
			chan,
			ChnStateStr(chnp),
			english("OUT restarted by NAK"),
			chnp->ch_msgid,
			(PUlong)len,
			(PUlong)addr
		);
		chnp->ch_state = CH_ACTIVE;
		chnp->ch_flags &= ~CH_EOF;
		chnp->ch_statsout(wch_datanak)++;
		if ( (len *= ACT_MULT) > ActiveBytes )
			ActiveBytes = 0;
		else
			ActiveBytes -= len;
		return;
	}

	/*
	**	Re-send NAKed data.
	**	NB: COULD BE LONG IF NAK IS FOR ACCUMULATED ERRORS.
	*/

	save_flags = chnp->ch_flags;
	if ( (save_size = OutDataSize) > len )
		OutDataSize = len;
	
	chnp->ch_flags &= ~CH_EOF;	/* No longer true */
	chnp->ch_flags |= CH_IGNBUF;	/* Bypass current buffer */

	while ( chnp->ch_msgaddr < gapend && SendData(chnp) );

	chnp->ch_flags &= ~(CH_EOF|CH_IGNBUF);
	chnp->ch_flags |= (save_flags&CH_EOF);
	OutDataSize = save_size;

	chnp->ch_msgaddr = save_addr;
	chnp->ch_statsout(wch_datanak)++;
}



/*
**	Message start acknowledged, seek up to new address, and continue sending data.
*/

void
RecvSOMA(chan, type, size, addr, data)
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	register Chan *	chnp = &Channels[chan];

#	ifdef	lint
	type = type;
#	endif

	Trace2(1, "RecvSOMA on channel %d", chan);

	if ( size != SOMADATASIZE )
	{
		chnp->ch_statsout(wch_badsoma)++;
		return;
	}

	PacketTime = Time;
	NewState(CHLINKUP);

	if
	(
		strncmp(A_SOMAID(data), chnp->ch_msgid, MSGIDLENGTH) != STREQUAL
		||
		addr > chnp->ch_msgsize
	)
	{
		DODEBUG(static char mid[MSGIDLENGTH+1]; (void)strncpy(mid, A_SOMAID(data), MSGIDLENGTH));
		Report7(
			RepFmt, chan, ChnStateStr(chnp),
			english("Unexpected SOMA"),
			mid, (PUlong)chnp->ch_msgsize, (PUlong)addr
		);
		chnp->ch_statsout(wch_unxsoma)++;
		return;
	}

	if ( addr )
	{
		chnp->ch_statsout(wch_restart)++;
		FlushRData(chnp, addr);		/* Sets chnp->ch_msgaddr = addr */
		Report7(
			RepFmt,
			chan,
			ChnStateStr(chnp),
			english("OUT restarted by SOMA"),
			chnp->ch_msgid,
			(PUlong)chnp->ch_msgsize,
			(PUlong)chnp->ch_msgaddr
		);
	}

	if ( chnp->ch_msgaddr < chnp->ch_msgsize )
	{
		DataChannels |= (1 << chnp->ch_number);
		chnp->ch_flags &= ~CH_EOF;
		chnp->ch_state = CH_ACTIVE;
	}
	else
	{
		chnp->ch_state = CH_ENDING;
		chnp->ch_count = MAX_EOMS;
		Send_EOM(chnp);
	}

	Write_Status();
}



/*
**	Signal message termination by writing EOM packet to virtual circuit.
*/

void
Send_EOM(chnp)
	register Chan *	chnp;
{
	S_EOMDATA(PktWriteData, chnp->ch_msgid);

	(void)WritePkt
	(
		chnp->ch_number, PKTEOMTYPE, EOMDATASIZE, chnp->ch_msgsize,
		VCfd,
		VCname
	);

	chnp->ch_cntrltime = Time + EOM_TIMEOUT;

	if ( Status.st_flags & CHLINKDOWN )
		chnp->ch_cntrltime += DOWN_TIMEOUT;

	chnp->ch_count--;

	DataChannels &= ~(1 << chnp->ch_number);
}



/*
**	Send `end of transmission' indicator.
*/

void
SendEOT(chan)
	int	chan;	/* REOT or LEOT */
{
	int	fd;
	char *	name;

	Trace2(1, "SendEOT(%d)", chan);

	if ( chan == REOT )
	{
		fd = VCfd;
		name = VCname;
	}
	else
	{
		fd = PinFd;
		name = PinName;
	}

	(void)WritePkt(chan, PKTEOTTYPE, 0, (Ulong)0, fd, name);
}



/*
**	Send `comms. sync.' or `message queue empty' indicator.
*/

int
SendMQE(chan, in)
	int	chan;	/* MQE, IDLE or SYNC */
	bool	in;	/* True if from reader to writer */
{
	int	fd;
	char *	name;

	Trace3(1, "SendMQE(%s, %s)", (chan==MQE)?"MQE":(chan==SYNC)?"SYNC":(chan==IDLE)?"IDLE":"CTRL", in?"in":"out");

	if ( in )
	{
		fd = PinFd;
		name = PinName;
	}
	else
	{
		fd = VCfd;
		name = VCname;
	}

	S_MQEDATA(PktWriteData, HomeAddress, HomeAdLength);

	return WritePkt(chan, PKTMQETYPE, HomeAdLength, (Ulong)0, fd, name);
}



/*
**	Signal message start by writing SOM packet to virtual circuit.
*/

void
SendSOM(chnp)
	register Chan *	chnp;
{
	register Ulong	waittime;
	extern int	WriteAheadSecs;

	if ( Time <= chnp->ch_msgtime )
		waittime = 0;
	else
		waittime = Time - chnp->ch_msgtime;

	S_SOMDATA(PktWriteData, waittime, chnp->ch_msgid, LinkAddress, LinkAdLength);

	(void)WritePkt
	(
		chnp->ch_number, PKTSOMTYPE, SOMMDATASIZE+LinkAdLength, chnp->ch_msgsize,
		VCfd,
		VCname
	);

	chnp->ch_cntrltime = Time + SOM_TIMEOUT + WriteAheadSecs;

	if ( Status.st_flags & CHLINKDOWN )
		chnp->ch_cntrltime += DOWN_TIMEOUT;
}



/*
**	Start a new message transmitting.
*/

int
StartMessage(chnp, sendsom)
	register Chan *	chnp;
	bool		sendsom;
{
	chnp->ch_state = CH_STARTING;
	chnp->ch_flags &= ~(CH_EOF|CH_IGNBUF);

	if ( chnp->ch_msgaddr == 0 )
		chnp->ch_count = InitDataCount;		/* Send ahead */
	else
		chnp->ch_count = 0;

	if ( !Started || !sendsom )
		return 0;

	SendSOM(chnp);

	Write_Status();

	return 1;
}



/*
**	Write something for active channel.
*/

int
WriteChan(chnp)
	register Chan *	chnp;
{
	Trace3(3, "WriteChan %d state %d", chnp->ch_number, chnp->ch_state);

	switch ( chnp->ch_state )
	{
	default:
		Fatal2("WriteChan state %d", chnp->ch_state);
		return 0;

	case CH_STARTING:
		if
		(
			chnp->ch_cntrltime <= Time
			||
			(
				chnp->ch_count == 0
				&&
				IdleTime >= MIN_SOM_TIMEOUT
				&&
				!(Status.st_flags & CHLINKDOWN)
			)
		)
		{
			if ( Verbose && !(Status.st_flags & CHLINKDOWN) )
				Report7(
					RepFmt,
					chnp->ch_number,
					ChnStateStr(chnp),
					english("Repeat SOM"),
					chnp->ch_msgid,
					(PUlong)chnp->ch_msgsize,
					(PUlong)chnp->ch_msgaddr
				);
			SendSOM(chnp);
			break;
		}
		if ( chnp->ch_count == 0 )
			return 0;
		--chnp->ch_count;
		if ( !(chnp->ch_flags & CH_EOF) )
		{
			(void)SendData(chnp);
			return 1;
		}
		return 0;

	case CH_ACTIVE:
		if ( !(chnp->ch_flags & CH_EOF) )
		{
			(void)SendData(chnp);
			return 1;
		}
		chnp->ch_flags &= ~CH_EOF;
		chnp->ch_state = CH_ENDING;
		chnp->ch_count = MAX_EOMS;
		Send_EOM(chnp);
		Write_Status();
		return 1;

	case CH_ENDING:
		if
		(
			chnp->ch_cntrltime <= Time
			||
			(
				IdleTime >= MIN_EOM_TIMEOUT
				&&
				!(Status.st_flags & CHLINKDOWN)
			)
		)
		{
			if ( chnp->ch_count == 0 )
				Error(english("Too many EOMs on chan %d"), chnp->ch_number);

			if ( Verbose && !(Status.st_flags & CHLINKDOWN) )
				Report7(
					RepFmt,
					chnp->ch_number,
					ChnStateStr(chnp),
					english("Repeat EOM"),
					chnp->ch_msgid,
					(PUlong)chnp->ch_msgsize,
					(PUlong)chnp->ch_msgaddr
				);

			Send_EOM(chnp);
			break;
		}
		return 0;

	case CH_ENDED:
		return 0;
	}

	return 1;
}
