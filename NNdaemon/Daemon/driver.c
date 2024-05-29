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
**	Protocol handler driver
*/

#define	RECOVER
#define	SIGNALS
#define	SELECT
#define	TIME
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"spool.h"

#if	BSD4 >= 2
#include	<sys/time.h>
#endif

#include	"Channel.h"

#include	"daemon.h"
#undef	Extern
#define	Extern
#define	ExternU
#include	"driver.h"
#include	"AQ.h"
#include	"CQ.h"
#include	"PQ.h"
#include	"Stream.h"

Channel		Channels[NSTREAMS];
CQhead		ControlQ[NSTREAMS];
AQhead		ActionQ			= { (AQl_p)0, &ActionQ.aq_first };

jmp_buf		IOerrorbuf;		/* Global I/O error recovery */
jmp_buf		QTimeouts;		/* Global action queue timeout recovery */

extern Ulong	RateBytes;		/* Decaying bytes transferred */
extern Ulong	RateTime;		/* Decaying time for RateBytes */

static jmp_buf	RTimeouts;		/* Read timeout receovery */
static unsigned	ElapsedTime;
static bool	inQ;			/* True if in queue handlers */
static unsigned	WTimeout;		/* Period for write timeouts */
static Time_t	XscanTime;		/* Time to scan message queue */

#if	DEBUG >= 1
#include	<stdio.h>
#include	"Debug.h"
#endif

extern SigFunc	alarmcatch;



void
driver()
{
	register int	i;
	Time_t		block_time = 0;
	int		block_count = 0;
#	if	BSD4 >= 2
	static struct timeval nowait = { 0, 0 };
#	endif	
#	if	BSD4 >= 2 || V8 == 1
	fd_set		read_fds, write_fds;

#	if	V8 == 1
	(void)strclr((char *)&read_fds, sizeof(fd_set));
	(void)strclr((char *)&write_fds, sizeof(fd_set));
#	else
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
#	endif	
#	endif	

	NNstate.linkstate = DLINK_DOWN;
	NNstate.maxspeed = 0;
	NNstate.allbytes = 0;
	NNstate.allmessages = 0;
	inByteCount = 0;
	outByteCount = 0;

	/*
	**	Initialise queues
	*/

	for ( i = 0 ; i < NSTREAMS ; i++ )
		ControlQ[i].cq_last = &ControlQ[i].cq_first;

	for ( i = 0 ; i < NPKTQ ; i++ )
		PktQ[i].pq_last = &PktQ[i].pq_first;

	/*
	**	Initialise function pointers for handler
	*/

	PqPkt = qPkt;
	PqCpkt = qCpkt;
	PfillPkt = fillPkt;
	PrecvData = recvData;
	PrecvControl = recvControl;
	PrTimeout = rTimeout;
	PrReset = rReset;
	PxReset = xReset;
	if ( Cook || CookVC != (CookT *)0 )
		PRread = RCread;
	else
		PRread = Rread;

	/*
	**	Catch I/O errors (1=read, 2=write).
	**	Close link.
	*/

	if ( (i = setjmp(IOerrorbuf)) )
	{
		ALARM_OFF();

		if ( i < 3 )
			Report3("%s error on remote: %s", (i==1)?"read":"write", ErrnoStr(errno));
		else
			Report2("IO error on channel %d", i-3);

		/*
		**	Flush the actions
		*/

		{
			register AQl_p	aqlp;

			inQ = true;
			NoAction = true;

			while ( (aqlp = ActionQ.aq_first) != (AQl_p)0 )
			{
				ActionQ.aq_first = aqlp->aql_next;
				aqlp->aql_next = AQfreelist;
				AQfreelist = aqlp;
				(*aqlp->aql_funcp)(aqlp->aql_arg.qu_arg);
			}

			ActionQ.aq_last = &ActionQ.aq_first;

			NoAction = false;
			inQ = false;
		}

		/*
		**	Flush the output.
		*/

		Wbufcount = 0;

		for ( i = 0 ; i < NPKTQ ; i++ )
		{
			register PQl_p	pqlp;

			while ( (pqlp = PktQ[i].pq_first) != (PQl_p)0 )
			{
				PktQ[i].pq_first = pqlp->pql_next;
				pqlp->pql_next = PQfreelist;
				PQfreelist = pqlp;
			}

			PktQ[i].pq_last = &PktQ[i].pq_first;
		}

		/*
		**	Flush the control queues.
		*/

		for ( i = 0 ; i < NSTREAMS ; i++ )
		{
			register CQl_p	cqlp;

			while ( (cqlp = ControlQ[i].cq_first) != (CQl_p)0 )
			{
				ControlQ[i].cq_first = cqlp->cql_next;
				cqlp->cql_next = CQfreelist;
				CQfreelist = cqlp;
			}

			ControlQ[i].cq_last = &ControlQ[i].cq_first;
		}

		/*
		**	Shut down the link
		*/

		NewState(DLINK_DOWN, false);

		FinishReason = "Remote I/O ERROR";
		return;
	}

	/*
	**	Initialise protocol parameters
	*/

	PItimo = IDLE_TIMEOUT;
	PprotoT = (int)UseCrc;
	Pnchans = Nstreams;
	Pfchan = Fstream;
	PXsize = PktZ;
	PRtimo = RECEIVE_TIMEOUT;
	Pnbufs = Nbufs;
	/* PBufMax = ((Speed+511)/512)*512; MADNESS!! */
	PHoldAcks = HoldAcks;

	/*
	**	Initialise protocol
	*/

	if ( !Pinit() )
	{
		Fatal1("bad protocol initialisation");
		return;
	}

	PktZ = PXsize;	/* Remember maximum allowed */

	SetSpeed();

	NNstate.inpkts = 0;
	NNstate.outpkts = 0;

	/*
	**	On "Rread" timeout call Ptimo
	*/

	(void)alarmcatch(0);
	XscanTime = LastTime + IDLE_SCANRATE;

	if ( setjmp(RTimeouts) )
	{
		Trace2(1, "READ TIMEOUT %lu", (PUlong)ElapsedTime);

		Pflush();

Timeout:
		DODEBUG(if(Traceflag)MesgN("TIME",NULLSTR));

		if ( block_time > 0 && (block_time + PRtimo) <= LastTime )
		{
			block_time = 0;

			if ( ++block_count > 3 )
			{
				block_count = 0;
				errno = 0;
				longjmp(IOerrorbuf, 2);
			}
		}

		if
		(
			BatchMode
			&&
			MinSpeed > 0
			&&
			(LastTime - NNstate.starttime) > 240
			&&
			(
				RateTime == 0
				||
				(RateBytes/RateTime) < MinSpeed
			)
		)
		{
			FinishReason = "Remote SLOW";
			return;
		}

		if ( block_time == 0 && Ptimo((Timo_t)ElapsedTime) )
		{
			NewState(DLINK_DOWN, false);

			if ( BatchMode )
			{
				FinishReason = "Remote TIMEOUT";
				return;
			}
		}
		else
		if ( BatchMode )
		{
			if ( AllMessages )
			{
				if ( NNstate.allmessages )
					NewState(DLINK_UP, false);
				else
				if ( NNstate.inpkts > 6 )
					NNstate.linkstate = DLINK_UP;
			}
			else
			if ( NNstate.inpkts > 6 )
				NewState(DLINK_UP, true);	/* First time ever */
		}
		else
		if ( NNstate.inpkts > 6 )
			NewState(DLINK_UP, false);

		if ( (UpdateTime += ElapsedTime) >= UpdateRate )
			Update(up_date);
	}

	ElapsedTime = 0;

	/*
	**	Driver loop
	*/

	for( ;; )
	{
		/*
		**	Fast message queue scan if idle
		*/

		if ( !Transmitting )
		{
			if ( XscanTime <= LastTime )
			{
				for ( i = Fstream ; i < Nstreams ; i++ )
				switch ( outStreams[i].str_state )
				{
				case STR_IDLE:
				case STR_EMPTY:
				case STR_ERROR:
					/** Scan for a new message **/
					NewMessage((AQarg)i);
				}

				XscanTime = LastTime + IDLE_SCANRATE;
			}
		}
		else
			XscanTime = LastTime + IDLE_SCANRATE;

		DODEBUG(
			if ( Alarm )
			{
				(void)alarm(0);
				Alarm = false;
				Report1("Alarm left set on entry to de-PacketQ");
			}
		);

		inQ = true;

		if ( block_time == 0 && !Finish )
		for ( ;; )
		{
			/*
			**	Transmit all queued packets,
			**	 unless both transmitting and receiving,
			**	 in which case transmit up to first data packet only.
			*/

			register PQl_p	pqlp;
			register int	n;
			register int	outcount = 0;

			if ( setjmp(QTimeouts) )
			{
				Trace1(1, "write BLOCKED");
				block_time = time((time_t *)0);
				BlockCount++;
				break;
			}

			for ( i = 0 ; i < NPKTQ ; i++ )
			while ( (pqlp = PktQ[i].pq_first) != (PQl_p)0 )
			{
				register char *	pktp = pqlp->pql_pktp;
				register int	size = pqlp->pql_size;

				DODEBUG(Logpkt((Pkt_p)pktp, PLOGSEND));

				if ( !Alarm )
					ALARM_ON(WTimeout);

				while ( (n = (*WriteO)(1, pktp, size)) != size )
				{
					if ( n <= 0 )
					{
						if ( n == SYSERROR )
						{
#							if	DEBUG >= 1
							if ( errno == EINTR )
								continue;
#							endif	/* DEBUG >= 1 */
							longjmp(IOerrorbuf, 2);
						}
						ALARM_OFF();
						longjmp(QTimeouts, 1);
					}

					pktp += n;
					size -= n;
					StVCdata(STOUTCHAN, n);
				}

				Trace2(2, "Write %d", pqlp->pql_size);

				StVCdata(STOUTCHAN, n);
				StVCpkt(STOUTCHAN);
				NNstate.outpkts++;
				outcount++;
				block_count = 0;

				if ( (PktQ[i].pq_first = pqlp->pql_next) == (PQl_p)0 )
				{
					PktQ[i].pq_last = &PktQ[i].pq_first;
					n = 0;
				}
				else
					n = 1;

				pqlp->pql_next = PQfreelist;
				PQfreelist = pqlp;

				if
				(
					n	/* More packets on Q */
					&&
					i != PCNTRLQ
					&&
					Transmitting
					&&
					Receiving
				)
				{
#					if	BSD4 >= 2 || V8 == 1
					if ( Wbufcount )
						(void)RWflush(1);

#					if	V8 == 1
					FD_SET(1, read_fds);
					FD_SET(1, write_fds);
#					else	/* V8 == 1 */
					FD_SET(1, &read_fds);
					FD_SET(1, &write_fds);
#					endif	/* V8 == 1 */

					if
					(
						select
						(
							1+1,
							&read_fds,
							&write_fds,
#							if	V8 == 1
							0
#							else
							(fd_set *)0,
							&nowait
#							endif
						) < 0
					)
						goto break2;

					if
					(
#						if	V8 == 1
						FD_ISSET(1, read_fds)
						||
						!FD_ISSET(1, write_fds)
#						else	/* V8 == 1 */
						FD_ISSET(1, &read_fds)
						||
						!FD_ISSET(1, &write_fds)
#						endif	/* V8 == 1 */
					)
#					endif	/* BSD4 >= 2 || V8 == 1 */
						goto break2;
				}
			}

			if
			(
				!HalfDuplex
				&&
				Receiving
				&&
				outcount == 0
				&&
				Plastidle == 0
				&&
				HoldAcks == 0
			)
			{
				Pidle();
				continue;
			}
break2:
			if ( Wbufcount )
				(void)RWflush(1);

			ALARM_OFF();
			break;
		}

		/*
		**	Perform any queued actions
		*/

		{
			register AQl_p	aqlp;
			register AQl_p	next;

			DODEBUG(
				if ( Alarm )
				{
					(void)alarm(0);
					Alarm = false;
					Report1("Alarm left set on entry to de-ActionQ");
				}
			);

			if ( (aqlp = ActionQ.aq_first) != (AQl_p)0 )
			{
				ActionQ.aq_first = (AQl_p)0;
				ActionQ.aq_last = &ActionQ.aq_first;

				do
				{
				DODEBUG(
					vFuncp	funcp = aqlp->aql_funcp;
					AQarg	arg = aqlp->aql_arg.qu_arg;
				);
					next = aqlp->aql_next;
					aqlp->aql_next = AQfreelist;
					AQfreelist = aqlp;

				DODEBUG(
					if ( setjmp(QTimeouts) )
						Report3("QTimeout in func %#lx(%#lx)", (PUlong)funcp, (PUlong)arg);
					else
					{
				);

					(*aqlp->aql_funcp)(aqlp->aql_arg.qu_arg);

				DODEBUG(
						if ( Alarm )
						{
							(void)alarm(0);
							Alarm = false;
							Report3("Alarm left set by func %#lx(%#lx)", (PUlong)funcp, (PUlong)arg);
						}
					}
				);
				}
					while ( (aqlp = next) != (AQl_p)0 );
			}
		}

		inQ = false;

		/*
		**	If all done, return.
		*/

		if ( Finish )
		{
			(void)sleep(2);	/* Drain last packet */
			return;
		}

		if ( MaxRunTime && MaxRunTime <= Time )
		{
			FinishReason = "Max runtime exceeded";
			return;
		}

		/*
		**	Call protocol receiver to read remote
		*/

		Precv();

		/*
		**	Perform timeout scan if necessary
		*/

		{
			register Time_t	thistime = time((time_t *)0);

			/*
			**	Some deadshit may have reversed time
			*/

			if ( thistime > LastTime )
			{
				i = thistime - LastTime;
				LastTime = thistime;
				Time = thistime;

				if
				(
					(ElapsedTime += i) >= ScanRate
					||
					ElapsedTime >= UPDATE_TIME
				)
					goto Timeout;
			}
			else
			if ( (LastTime-thistime) > 10 )
			{
				FinishReason = "-ve time change";
				return;	/* Reboot ... */
			}
		}
	}
}



/*
**	Catch alarm signals from system
*/

void
alarmcatch(sig)
	int		sig;
{
	register Time_t	thistime;

	(void)signal(SIGALRM, alarmcatch);

	thistime = time((time_t *)0);

	Trace5(1, "alarmcatch(%d), time=%lu, Elapsed=%u, Last=%lu", sig, (PUlong)thistime, ElapsedTime, (PUlong)LastTime);

	if ( thistime > LastTime )
		ElapsedTime += thistime - LastTime;

	LastTime = thistime;
	Time = thistime;
	Alarm = false;

	if ( sig )
	{
		if ( inQ )
			longjmp(QTimeouts, 1);
		else
			longjmp(RTimeouts, 1);
	}
}



/*
**	Recalculate PRtimo if set speed of link differs from the
**	effective speed by more than 20%.
*/

void
SetSpeed()
{
	register int	xmax;
	register int	nxpkts;
	register int	rsize;

	if ( !NoAdjust && NNstate.activetime > 60 )
	{
		register int	speed;
		register int	diff;

		speed = NNstate.allbytes/NNstate.activetime;

		if ( (diff = speed-Speed) < 0 )
			diff = -diff;

		if ( diff > Speed/5 )
			Speed += (speed-Speed) / 5;
	}

	if ( PXmax < PktZ )
		xmax = PXmax;
	else
		xmax = PktZ;

	rsize = Receiving * PRnbufs * (PRsize+2*Poverhead);
	nxpkts = Transmitting * Pnbufs;

	if ( (Transmitting + Receiving) == 0 )
		nxpkts = Nstreams - Fstream;

	for ( ;; )
	{
		PRtimo = (
				rsize			/* Size of receive packets in flight */
				+ nxpkts		/* Number of transmit packets in flight ahead */
				* (PXsize+2*Poverhead)	/* Size of packet + ack */
				+ Speed-1		/* Round up */
			 )
			 / Speed			/* Divide by speed */
			 + 1;				/* Add safety factor */

		if ( Cook )
			PRtimo *= 2;			/* Allow for possible expansion of packets */

		PRtimo += 2*IntraPktDelay;		/* Sub-carrier delays */

		if ( NoAdjust )
			break;

		if ( PRtimo > IDLE_TIMEOUT )
		{
			if ( PXsize <= MINPKTSIZE )
			{
				PXsize = MINPKTSIZE;
				if ( PRtimo > IDLE_TIMEOUT )
					PRtimo = IDLE_TIMEOUT;
				break;
			}

			PXsize /= 2;
		}
		else
		if ( PRtimo < RECEIVE_TIMEOUT )
		{
			if ( PXsize >= xmax )
			{
				PRtimo = RECEIVE_TIMEOUT;
				PXsize = xmax;
				break;
			}

			PXsize *= 2;
		}
		else
			break;
	}

	if ( (rsize = (IDLE_SCANRATE-4) + PRtimo * 2) > IDLE_TIMEOUT )
		PItimo = rsize;
	else
		PItimo = IDLE_TIMEOUT;

	/*
	**	Set write timeout large enough to write all transmit packets.
	*/

	WTimeout = (nxpkts * (PXsize+2*Poverhead) + Speed-1) / Speed + 1;
	if ( Cook )
		WTimeout *= 2;
	WTimeout += IntraPktDelay;
	if ( WTimeout < RECEIVE_TIMEOUT )
		WTimeout = RECEIVE_TIMEOUT;

	/*
	**	Set ScanRate (receive timeout) large enough to receive whole packet.
	*/

	if ( (Transmitting + Receiving) == 0 )
		ScanRate = IDLE_SCANRATE;
	else
	{
		ScanRate = PXsize + Poverhead;
		if ( Cook )
			ScanRate *= 2;
		if ( (ScanRate = (ScanRate+Speed-1)/Speed + 1) < ACTIVE_SCANRATE )
			ScanRate = ACTIVE_SCANRATE;

		ScanRate += IntraPktDelay;
	}

	NNstate.recvtimo = PRtimo;
	NNstate.packetsize = PXsize;
	NNstate.speed = Speed;
	NNstate.nbufs = Pnbufs;
}
