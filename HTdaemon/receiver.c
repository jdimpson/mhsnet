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
**	Routines to process inbound packets.
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


void		PassInPkt _FA_((int, int, int, int, Ulong, Uchar *));
void		RecvData _FA_((int, int, int, int, Ulong, Uchar *));
void		Recv_EOM _FA_((int, int, int, int, Ulong, Uchar *));
void		RecvREOT _FA_((int, int, int, int, Ulong, Uchar *));
void		RecvRMQE _FA_((int, int, int, int, Ulong, Uchar *));
void		Recv_SOM _FA_((int, int, int, int, Ulong, Uchar *));

vFuncp		ReadReceivers[MAXPKTTYPE]	=
{
	RecvData,
	PassInPkt,	/* NAK */
	Recv_SOM,
	PassInPkt,	/* SOMA */
	Recv_EOM,
	PassInPkt,	/* EOMA */
	RecvRMQE,
	RecvREOT
};

static bool	OwnSync;	/* True after SYNC from local transmitter has looped back */
static bool	RemSync;	/* True after SYNC from remote transmitter has been passed back */
static bool	NoMoreSync;	/* True after first SOM/IDLE/CTRL */

static void	SendLowSpace _FA_((void));



/*
**	Close off any open messages.
*/

static void
AbortInMesgs(reason)
	char *	reason;
{
	register Chan *	chnp;
	register int	i;
	register bool	update;

	for ( update = false, chnp = Channels, i = NCHANNELS ; --i >= 0 ; chnp++ )
	{
		if ( chnp->ch_flags & CH_MSGFILE )
		{
			Close(chnp->ch_msgfd, CLOSE_NOSYNC, chnp->ch_msgfilename);
			Unlink(chnp->ch_msgfilename);
			chnp->ch_flags &= ~CH_MSGFILE;
		}

		chnp->ch_flags &= ~CH_LOWSPACE;

		if ( chnp->ch_state != CH_ENDED && chnp->ch_state != CH_IDLE )
		{
			Report7(
				RepFmt,
				chnp->ch_number,
				ChnStateStr(chnp),
				reason,
				chnp->ch_msgid,
				(PUlong)chnp->ch_msgsize,
				(PUlong)chnp->ch_msgaddr
			);

			chnp->ch_statsin(rch_abort)++;
			chnp->ch_state = CH_ENDED;
			chnp->ch_goodaddr = 0;

			update = true;
		}
	}

	if ( update )
		Write_Status();
}



/*
**	Abort on echo-detection.
*/

static void
AbortEcho()
{
	FinishReason = &FinVCEcho;
	Error(FinVCEcho.reason);
}



/*
**	Wind up message.
*/

void
EndMesg(chnp)
	register Chan *	chnp;
{
	if ( chnp->ch_flags & CH_MSGFILE )
	{
		FlushWData(chnp);
		Close(chnp->ch_msgfd, CLOSE_SYNC, chnp->ch_msgfilename);
		chnp->ch_flags &= ~CH_MSGFILE;
	}

	RecvMessage(chnp);

	SendEOMA(chnp);

	chnp->ch_flags &= ~CH_RDUP;

	chnp->ch_state = CH_ENDED;
	chnp->ch_statsin(rch_messages)++;
	DMNSTATSINC(DS_MESSAGES);

	Write_Status();
}



/*
**	Writer control packet received -- pass to writer's input pipe.
*/

void
PassInPkt(dir, chan, type, size, addr, data)
	int		dir;
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	Trace5(1, "PassInPkt dir %d type %d size %d on channel %d", dir, type, size, chan);

	if ( dir == Direction )
		AbortEcho();

	if ( !Started )
		return;

	if ( size )
		bcopy((char *)data, (char *)PktWriteData, size);

	(void)WritePkt
	(
		chan, type, size, addr,
		PinFd,
		PinName
	);

	if ( !InOnly )
	{
		LMQEreceived = false;	/* Indicate no longer idle */
		Status.st_flags &= ~CHLMQERCVD;
		Trace1(1, StrStatus());
	}
}



/*
**	Data packet in - write to message file.
*/

void
RecvData(dir, chan, type, size, addr, data)
	int		dir;
	int		chan;
	int		type;
	register int	size;
	register Ulong	addr;
	Uchar *		data;
{
	register Chan *	chnp = &Channels[chan];
	register Ulong	endd = addr + size;
	static int	last_size;

	Trace5(2, "RecvData size %d addr %lu on channel %d state %d", size, (PUlong)addr, chan, chnp->ch_state);

	if ( dir == Direction )
		AbortEcho();

	if ( !Started )
		return;

#	ifdef	lint
	type = type;
#	endif

	if ( chnp->ch_state != CH_ACTIVE && chnp->ch_state != CH_ENDING )
	{
		chnp->ch_statsin(rch_baddata)++;	/* Lost SOM / already ended and restart? */
		return;
	}

	if ( chnp->ch_msgsize < endd )
	{
		Report7(
			RepFmt, chan, ChnStateStr(chnp),
			english("Bad DATA"),
			chnp->ch_msgid,
			(PUlong)size, (PUlong)addr
		);
		chnp->ch_statsin(rch_badaddr)++;
		return;
	}

	if ( !(chnp->ch_flags & CH_MSGFILE) )
	{
		SendEOT(LEOT);		/* Tell writer to finish */
		Fatal2("No message file open for RecvData on channel %d", chan);
		return;
	}
	
	PacketTime = Time;
	IDLETime = Time;		/* The other end is active */
	ActiveBytes += size * ACT_MULT;
	DMNSTATSINCC(DS_DATA, size);
	if ( size == last_size )
		Status.st_pktdatasize = size;
	last_size = size;

	while ( FreeFSpace < size )
	{
		CheckFree = Time + FREESCANRATE;
		if ( FSFree(SPOOLDIR, (Ulong)size) )
			break;

		SendLowSpace();

		if ( chnp->ch_flags & CH_LOWSPACE )
			return;

		Report7(
			RepFmt, chan, ChnStateStr(chnp),
			english("LOW SPACE"),
			chnp->ch_msgid, (PUlong)size, (PUlong)addr
		);

		chnp->ch_flags |= CH_LOWSPACE;

		Write_Status();
		LowSpace(chan);
		return;
		/* NOTREACHED */
	}

	FreeFSpace -= size;

	if ( chnp->ch_flags & CH_LOWSPACE )
	{
		Report7(
			RepFmt, chan, ChnStateStr(chnp),
			english("IN restarted SPACE OK"),
			chnp->ch_msgid, (PUlong)size, (PUlong)addr
		);

		chnp->ch_flags &= ~CH_LOWSPACE;
	}

	chnp->ch_statsin(rch_data) += size;

	if ( chnp->ch_msgaddr != addr )
	{
		if ( chnp->ch_msgaddr > addr )
		{
			if ( !FillGap(chnp, addr, (Ulong)size) && chnp->ch_msgaddr >= endd )
			{
				chnp->ch_statsin(rch_dupdata)++;
				return;
			}

			WriteData(chnp, (char *)data, size, addr);

			if ( chnp->ch_msgaddr < endd )
				chnp->ch_msgaddr = endd;

			if
			(
				chnp->ch_state == CH_ENDING
				&&
				chnp->ch_gaplist == (Gap *)0
			)
				EndMesg(chnp);

			return;
		}

		QueueGap(chnp, addr);
	}
	else
	if ( chnp->ch_gaplist == (Gap *)0 )
	{
		if ( chnp->ch_goodaddr < addr )
			chnp->ch_goodaddr = addr;
	}
	else
		(void)FillGap(chnp, addr, (Ulong)size);

	if ( chnp->ch_chkpttime < (Time - NAK_TIMEOUT) )
	{
		if ( chnp->ch_gaplist != (Gap *)0 )
			(void)NAKGaps(chnp, (Ulong)(InitDataCount*Status.st_pktdatasize));
		chnp->ch_chkpttime = Time;
	}

	WriteData(chnp, (char *)data, size, addr);

	chnp->ch_msgaddr = endd;
}



/*
**	End of message indicator - check for missing data, process message.
*/

void
Recv_EOM(dir, chan, type, size, addr, data)
	int		dir;
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	register Chan *	chnp = &Channels[chan];
	register Ulong	naksize;

	Trace2(1, "Recv_EOM on channel %d", chan);

	if ( dir == Direction )
		AbortEcho();

	if ( !Started )
		return;

#	ifdef	lint
	type = type;
#	endif

	if ( size != EOMDATASIZE )
	{
		chnp->ch_statsin(rch_badeom)++;
		return;
	}

	PacketTime = Time;
	IDLETime = Time;		/* The other end is active */

	if
	(
		strncmp(A_EOMID(data), chnp->ch_msgid, MSGIDLENGTH) != STREQUAL
		||
		addr != chnp->ch_msgsize
	)
	{
		Report
		(
			RepFmt,
			chnp->ch_number,
			ChnStateStr(chnp),
			english("Last message"),
			chnp->ch_msgid,
			(PUlong)chnp->ch_msgsize,
			(PUlong)chnp->ch_msgaddr
		);
		chnp->ch_statsin(rch_unxeom)++;
		Report
		(
			RepFmt,
			chan,
			ChnStateStr(chnp),
			english("Unexpected EOM"),
			A_EOMID(data),
			(PUlong)chnp->ch_msgsize,
			(PUlong)addr
		);
		return;
	}

	if ( chnp->ch_state == CH_ENDED )
	{
		SendEOMA(chnp);
		chnp->ch_statsin(rch_xeom)++;	/* EOMA got lost? */
		return;
	}

	if ( chnp->ch_state == CH_STARTING )	/* What happened to SOM?? */
	{
		AbortInMesgs(english("IN aborted by unexpected EOM"));	/* Close off any open messages. */

		Error(english("Unexpected EOM"));
		return;
	}

	naksize = (ActiveBytes / ActiveTime ) * EOM_TIMEOUT;

	if ( chnp->ch_msgaddr < addr )
	{
		QueueGap(chnp, addr);

		if ( chnp->ch_msgaddr == 0 )
		{
			/*
			**	QueueGap will have sent NAK for whole message
			**	-- puts transmitter back into CH_ACTIVE mode.
			*/

			chnp->ch_eomcount++;
			(void)NAKGaps(chnp, naksize);	/* For repeated EOMs */
			return;
		}

		chnp->ch_msgaddr = addr;
	}

	chnp->ch_state = CH_ENDING;
	chnp->ch_eomcount++;

	if ( NAKGaps(chnp, naksize) )
		return;

	EndMesg(chnp);
}



/*
**	End of transmission.
*/

void
RecvREOT(dir, chan, type, size, addr, data)
	int		dir;
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	Trace1(1, "RecvREOT");

	if ( dir == Direction )
		AbortEcho();

#	ifdef	lint
	data = data;
#	endif

	if ( !OwnSync || !RemSync )
		return;

	PacketTime = Time;

	/** Tell reader to finish **/

	Finish = true;
	FinishReason = &FinRemterm;

	/** Tell writer to finish **/

	(void)WritePkt
	(
		chan, type, size, addr,
		PinFd,
		PinName
	);
}



/*
**	Message queue empty (or SYNC/IDLE) indicator - check which end.
*/

void
RecvRMQE(dir, chan, type, size, addr, data)
	int		dir;
	int		chan;
	int		type;
	int		size;
	Ulong		addr;
	Uchar *		data;
{
	Trace4(
		1,
		"RecvRMQE %s for link=%.*s.",
		(chan==SYNC)?"SYNC":(chan==IDLE)?"IDLE":"MQE",
		size,
		A_MQELINK(data)
	);

	if ( dir == Direction )
	{
		if ( chan == SYNC )
			return;		/* Just ignore echoed SYNCs here */
		AbortEcho();
	}

	PacketTime = Time;
	IDLETime = Time;		/* The other end is active */

	if
	(
		HomeAdLength != size
		||
		strnccmp(HomeAddress, A_MQELINK(data), HomeAdLength) != STREQUAL
	)
	{
		/*
		**	Check address.
		*/

		if
		(
			LinkAdLength != size
			||
			strnccmp(LinkAddress, A_MQELINK(data), LinkAdLength) != STREQUAL
		)
		{
			/*
			**	No idea whom this is from!
			*/

			FinishReason = &FinMismatch;

			Report5(
				"%s: \"%s\" != \"%.*s\"",
				FinMismatch.reason,
				LinkAddress,
				size,
				A_MQELINK(data)
			);

			Error(FinMismatch.reason);
			return;
		}

		/** From other end's writer **/

		if ( chan == SYNC )
		{
			if ( NoMoreSync )
			{
				FinishReason = &FinSync;/* Other end re-started without clearing circuit? */
				Error(FinSync.reason);	/* MUST TERMINATE HERE -- until receiver/tranmitter */
				return;			/* fixed to reset message states (unexpected EOM?) */
			}

			RemSync = true;
		}
		else
		if ( chan == IDLE )
		{
			NoMoreSync = true;

			/** Transmitter can adjust packet size **/

			(void)WritePkt(chan, type, 0, addr, PinFd, PinName);
			return;
		}

		/*
		**	Pass back to other end's reader.
		*/

		S_MQEDATA(PktWriteData, (char *)data, size);

		(void)WritePkt(chan, type, size, addr, PoutFd, PoutName);

		if ( chan == MQE )
		{
			NoMoreSync = true;
			RMQEreceived = true;	/* Indicate remote message queue empty */
			Status.st_flags |= CHRMQERCVD;
			Trace1(1, StrStatus());
		}
		else
		if ( chan != CTRL )		/* CTRL to switch directions faster than MQE */
			return;

		NoMoreSync = true;
		DataChannels = 0;

		if ( TurnAround )		/* Switch directions */
		{
			/** Inform transmitter **/
			(void)WritePkt(MQE, type, size, addr, PinFd, PinName);

			OutOnly = true;
			Status.st_flags |= CHOUTONLY;
			InOnly = false;
			Status.st_flags &= ~CHINONLY;
			Trace1(1, StrStatus());
			Write_Status();
		}

		AbortInMesgs(english("IN aborted by MQE"));	/* Close off any open messages. */

		Trace1(1, "Remote message queue empty");
	}
	else
	if ( chan == SYNC )
	{
		if ( !RemSync )
			return;				/* Ignore until we have seen other */

		if ( !OwnSync )
		{
			(void)SendMQE(SYNC, SENDLOCAL);	/* Transmitter can start messages */
			Status.st_flags &= ~(CHLINKDOWN|CHLINKSTART);
			Write_Status();
		}

		OwnSync = true;
		Started = true;
		SendIDLE = Time + Idle_Timeout;		/* Ensure suitable gap before 1st IDLE */
		return;
	}
	else
	if ( chan == MQE || chan == CTRL )
	{
		if ( chan == MQE )
		{
			LMQEreceived = true;		/* Indicate local idle */
			Status.st_flags |= CHLMQERCVD;
			Trace1(1, StrStatus());
		}

		if ( TurnAround )			/* Switch directions */
		{
			(void)SendMQE(MQE, SENDLOCAL);	/* Inform transmitter */
			OutOnly = false;
			Status.st_flags &= ~CHOUTONLY;
			InOnly = true;
			Status.st_flags |= CHINONLY;
			Trace1(1, StrStatus());
			Write_Status();
		}

		Trace1(1, "Local message queue empty");
	}
	else
	{
		/*
		**	IDLE echoed on link!
		*/
		return;			/* Just ignore it */
	}

	if ( RMQEreceived && LMQEreceived && BatchTime != 0 )
	{
		Finish = true;		/* Tell reader to finish */
		SendEOT(LEOT);		/* Tell writer to finish */
	}
}



/*
**	Start of message indicator - open new message file.
*/

void
Recv_SOM(dir, chan, type, size, addr, data)
	int		dir;
	int		chan;
	int		type;
	int		size;
	Ulong		addr;	/* Size of message */
	Uchar *		data;
{
	register Chan *	chnp = &Channels[chan];
	register char *	msgid;

	Trace5(1, "Recv_SOM on channel %d, id=%.*s, size=%d.", chan, MSGIDLENGTH, A_SOMID(data), addr);

	if ( dir == Direction )
		AbortEcho();

#	ifdef	lint
	type = type;
#	endif

	if ( size <= SOMMDATASIZE )
	{
		chnp->ch_statsin(rch_badsom)++;
		return;
	}

	if ( !Started )
		return;
	NoMoreSync = true;

	PacketTime = Time;
	IDLETime = Time;	/* The other end is active */
	Status.st_flags &= ~(CHLINKDOWN|CHLINKSTART);

	if ( OutOnly )
		return;		/* Just ignore the attempt */

	RMQEreceived = false;	/* Indicate no longer idle */
	Status.st_flags &= ~CHRMQERCVD;
	Trace1(1, StrStatus());

	msgid = A_SOMID(data);

	if ( strncmp(msgid, chnp->ch_msgid, MSGIDLENGTH) == STREQUAL && addr == chnp->ch_msgsize )
	{
		if ( chnp->ch_state == CH_ENDED )
		{
			chnp->ch_statsin(rch_endrestart)++;
			SendEOMA(chnp);	/* May get data anyway */
			Report7(
				RepFmt, chan, ChnStateStr(chnp),
				english("IN restart ignored"),
				msgid, (PUlong)addr, (PUlong)chnp->ch_msgaddr
			);
			return;
		}

		if ( !(chnp->ch_flags & CH_MSGFILE) )
		{
			if ( (chnp->ch_msgfd = open(chnp->ch_msgfilename, O_WRITE)) == SYSERROR )
			{
				chnp->ch_statsin(rch_duprestart)++;
				chnp->ch_flags |= CH_RDUP;
				Report7(
					RepFmt, chan, ChnStateStr(chnp),
					english("IN restarted (NOFILE - DUP?)"),
					msgid, (PUlong)addr, (PUlong)chnp->ch_msgaddr
				);
				goto newmsg;
			}

			chnp->ch_flags |= CH_MSGFILE;

			chnp->ch_msgaddr = Seek(chnp->ch_msgfd, (Ulong)0, 2, chnp->ch_msgfilename);

			if ( chnp->ch_msgaddr > chnp->ch_goodaddr )
				chnp->ch_msgaddr = Seek(chnp->ch_msgfd, chnp->ch_goodaddr, 0, chnp->ch_msgfilename);

			chnp->ch_bufstart = chnp->ch_bufend = chnp->ch_msgaddr;

			FlushGaps(chnp);
		}

		if ( !(chnp->ch_flags & CH_LOWSPACE) )
		{
			chnp->ch_statsin(rch_restart)++;

			if ( Verbose )
			Report7(
				RepFmt, chan, ChnStateStr(chnp),
				english("IN restarted"),
				msgid, (PUlong)addr, (PUlong)chnp->ch_msgaddr
			);
		}
	}
	else
	{
		if ( chnp->ch_flags & CH_MSGFILE )
		{
			Close(chnp->ch_msgfd, CLOSE_NOSYNC, chnp->ch_msgfilename);
			Unlink(chnp->ch_msgfilename);
		}

		if ( chnp->ch_state != CH_ENDED && chnp->ch_state != CH_IDLE )
		{
			Report7(
				RepFmt,
				chnp->ch_number,
				ChnStateStr(chnp),
				english("IN aborted by SOM"),
				chnp->ch_msgid,
				(PUlong)chnp->ch_msgsize,
				(PUlong)chnp->ch_msgaddr
			);

			chnp->ch_statsin(rch_abort)++;
			chnp->ch_state = CH_ENDED;	/* In case Unlink error */

			if ( !(chnp->ch_flags & CH_MSGFILE) )
				(void)unlink(chnp->ch_msgfilename);	/* Just in case */
		}
newmsg:
		(void)strncpy(chnp->ch_msgidp, msgid, MSGIDLENGTH);	/* Onto end of filename */
		(void)strncpy(chnp->ch_msgid, msgid, MSGIDLENGTH);	/* And into status */
		chnp->ch_bufstart = 0;
		chnp->ch_bufend = 0;
		chnp->ch_msgaddr = 0;
		chnp->ch_msgsize = addr;
		chnp->ch_goodaddr = addr;
		(void)unlink(chnp->ch_msgfilename);			/* In case crash or `readerstatus' was removed */
		chnp->ch_msgfd = Create(chnp->ch_msgfilename);
		chnp->ch_flags |= CH_MSGFILE;
		chnp->ch_flags &= ~CH_LOWSPACE;
		FlushGaps(chnp);
	}

	chnp->ch_msgtime = Time - G_SOMTIME(data);

	while ( FreeFSpace < (chnp->ch_msgsize - chnp->ch_msgaddr) )
	{
		CheckFree = Time + FREESCANRATE;
		if ( FSFree(SPOOLDIR, (Ulong)(chnp->ch_msgsize - chnp->ch_msgaddr)) )
			break;

		chnp->ch_state = CH_STARTING;

		SendLowSpace();

		if ( chnp->ch_flags & CH_LOWSPACE )
			return;

		Report7(
			RepFmt, chan, ChnStateStr(chnp),
			english("LOW SPACE"),
			msgid, (PUlong)addr, (PUlong)chnp->ch_msgaddr
		);

		chnp->ch_flags |= CH_LOWSPACE;

		Write_Status();
		LowSpace(chan);
		return;		/* Just ignore the attempt */
		/* NOTREACHED */
	}

	if ( chnp->ch_flags & CH_LOWSPACE )
	{
		Report7(
			RepFmt, chan, ChnStateStr(chnp),
			english("IN restarted SPACE OK"),
			msgid, (PUlong)addr, (PUlong)chnp->ch_msgaddr
		);

		chnp->ch_flags &= ~CH_LOWSPACE;
	}

	S_SOMADATA(PktWriteData, msgid);

	(void)WritePkt
	(
		chan, PKTSOMATYPE, SOMADATASIZE, chnp->ch_msgaddr,
		PoutFd,
		PoutName
	);

	chnp->ch_state = CH_ACTIVE;
	chnp->ch_chkpttime = Time;
	chnp->ch_eomcount = 0;

	Write_Status();

	DataChannels |= (1 << chnp->ch_number);
}



/*
**	Signal successful message reception by writing EOMA packet to writer's output pipe.
*/

void
SendEOMA(chnp)
	register Chan *	chnp;
{
	S_EOMADATA(PktWriteData, chnp->ch_msgid);

	(void)WritePkt
	(
		chnp->ch_number, PKTEOMATYPE, EOMADATASIZE, chnp->ch_msgsize,
		PoutFd,
		PoutName
	);

	DataChannels &= ~(1 << chnp->ch_number);
}



/*
**	Let writer know of reader's problem.
*/

static void
SendLowSpace()
{
	if ( BatchTime == 0 || Time < LowSpaceTime )
		return;

	S_MQEDATA(PktWriteData, HomeAddress, HomeAdLength);

	(void)WritePkt
	(
		CTRL, PKTMQETYPE, HomeAdLength, (Ulong)CTRL_NOSPACE,
		PinFd, PinName
	);

	LowSpaceTime = Time + BatchTime;
}



/*
**	NAK an input packet by writing to writer's output pipe.
**
**	Impose a maximum size for each NAK,
**	and a maximum count of NAKS, to avoid stalling transmitters.
*/

#define	MAXNAKSIZE	(4*OutDataSize)	/* At most 4K per NAK to avoid stalling remote transmitter */
#define	MAXNAKBYTES	OutDataSize	/* Total NAK packet bytes to avoid stalling local transmitter */

Ulong
SendNAK(chnp, type, length, addr)
	Chan *		chnp;
	int		type;
	Ulong		length;
	Ulong		addr;
{
	register Ulong	len;
	register Ulong	maxlen = MAXNAKSIZE;
	register int	bytes = MAXNAKBYTES;
	register Ulong	sent = 0;

	while ( length != 0 && bytes > 0 )
	{
		if
		(
			(len = length) > maxlen
			&&
			(	/* Allow one NAK for whole message */
				addr != 0
				||
				len < chnp->ch_msgsize
			)
		)
			len = maxlen;

		S_NAKDATA(PktWriteData, type, len);

		bytes -= WritePkt
			 (
				chnp->ch_number, PKTNAKTYPE, NAKDATASIZE, addr,
				PoutFd,
				PoutName
			 );

		chnp->ch_statsin(rch_datanak)++;

		addr += len;
		sent += len;
		length -= len;
	}

	return sent;
}
