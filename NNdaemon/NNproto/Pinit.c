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
**	Initialise Channel structures, and prod remote.
*/

#include	"global.h"

#undef	Extern				/* All the declarations */
#define	Extern
#define	ExternU
#include	"Pconfig.h"
#include	"Packet.h"
#include	"Channel.h"
#include	"Proto.h"
#define	PSTATSDATA		1	/* Declare strings if configured */
#include	"Pstats.h"
#include	"Debug.h"


bool
Pinit()
{
	register int	i;

	/*
	**	Check Pconfig structure has been set-up correctly.
	*/

	if
	(
		   PqPkt == NULLVFUNCP
		|| PqCpkt == NULLVFUNCP
		|| PfillPkt == NULLFUNCP
		|| PrecvData == NULLVFUNCP
		|| PrecvControl == NULLVFUNCP
		|| PrTimeout == NULLVFUNCP
		|| PrReset == NULLVFUNCP
		|| PxReset == NULLVFUNCP
		|| PRread == NULLVFUNCP
		|| PItimo == 0
		|| PRtimo == 0
		|| Pnchans == 0
		|| (unsigned)Pnchans > MAXCHANS
		|| (unsigned)Pfchan >= Pnchans	
	)
	{
		return false;
	}

	SETDEBUGTIME();

#	if	MAX_CTL_SIZ > CNTLDATAZ
		**** YEUCH!! ****
#	endif

	if ( Pnbufs <= 0 || Pnbufs > NPKTBUFS )
		Pnbufs = NPKTBUFS;

	if ( PHoldAcks <= 0 || PHoldAcks > (Pnbufs-1) )
		if ( (PHoldAcks = (Pnbufs-4)) < 0 )
			PHoldAcks = 0;

	if ( PXsize <= 0 || PXsize > PKTDATAZ )
		PXsize = PKTDATAZ;
	
	PRsize = PXsize;
	PRnbufs = Pnbufs;

	if ( PBufMax <= 0 || PBufMax > PBUFSIZE )
		PBufMax = PBUFSIZE;

	PXmax = PKTDATAZ;

	Poverhead = PKTHDRZ;

#	if	defined(NNstrpr) || defined(NNstrpr2)
	if ( PprotoT )
	{
		PprotoT = PT_CRC;
		Poverhead += PKTCRCZ;
	}
#	endif
#	ifdef	ENproto
	PprotoT = PT_DATAGRAM;
#	endif
#	ifdef	PNproto
	Poverhead += PKTCRCZ;
	PprotoT = PT_CRC;
#	endif

	for ( i = 0 ; i < sizeof SeqTable ; i++ )
		SeqTable[i] = i % SEQMOD;

	for ( i = Pfchan ; i < Pnchans ; i++ )
	{
		Channels[i].chn_xstate = CHNS_RESET;
		Channels[i].chn_xtime = 2 * PItimo;	/* 1st timeout fast */

		PsendCpkt
		(
			(PKTCNTLTYP<<PKTTYP_S)|(i<<PKTCHN_S),
			REQ_R_SIZ,
			REQ_RESET
		);

		Preset(i);
	}

	return true;
}



/*
**	Reset remote transmitter
*/

void
Preset(chan)
	int		chan;
{
	register Chn_p	chnp;
	
	chnp = &Channels[chan];

	chnp->chn_artime = 0;
	chnp->chn_rtime = 1;
	chnp->chn_timo = A_I_COUNT;	/* Start off idle */

	/*
	**	Send a reset packet
	*/

	if ( chan >= Pfchan )
		PsendCpkt
		(
			(PKTCNTLTYP<<PKTTYP_S)|(chan<<PKTCHN_S),
			XMT_R_SIZ,
			XMT_RESET
		);

	/*
	**	Await acknowledgement
	*/

	chnp->chn_rstate = CHNS_RESET;
}



/*
**	Reset receive channel
*/

void
PRreset(chan)
	int		chan;
{
	register Chn_p	chnp;
	register int	i;
	
	chnp = &Channels[chan];

	chnp->chn_rnak = 0;
	chnp->chn_rseq = 0;

	for ( i = 0 ; i < (NPKTBUFS-1) ; i++ )
		chnp->chn_rpkts[i].pks_state = RPS_NULL;

	chnp->chn_rbfp = chnp->chn_rbuf;
	chnp->chn_rbfc = PBufMax;

	chnp->chn_rstate = CHNS_IDLE;

	/*
	**	Reset higher level
	*/

	(*PrReset)(chan);
}



/*
**	Reset transmit channel
*/

void
PXreset(chan)
	int		chan;
{
	register Chn_p	chnp;
	register int	i;
	
	chnp = &Channels[chan];

	chnp->chn_xseq = 0;
	chnp->chn_nxpkt = chnp->chn_xpkts;

	for ( i = 0 ; i < NPKTBUFS ; i++ )
		chnp->chn_xpkts[i].pks_state = XPS_NULL;

	chnp->chn_xbfp = chnp->chn_xbuf;
	chnp->chn_xbfc = 0;

	chnp->chn_xstate = CHNS_IDLE;

	/*
	**	Reset higher level
	*/

	(*PxReset)(chan);
}



/*
**	Perform receive timeout scan
**	- send TIMEOUT packet for any channels that have not received data recently.
*/

bool
Ptimo(elapsed)
	Timo_t		elapsed;
{
	register Chn_p	chnp;
	register int	channel;
	int		inactive = Pfchan;
	int		rs_timo = 3 * PItimo;
	int		ia_timo = rs_timo + PItimo;

	for ( chnp = &Channels[Pfchan], channel = Pfchan ; channel < Pnchans ; chnp++, channel++ )
	{
		/*
		**	Transmitter timeouts.
		*/

		if ( chnp->chn_xstate == CHNS_RESET )
		{
			if ( (int)(chnp->chn_xtime += elapsed) > rs_timo )
			{
				chnp->chn_xtime = 1;

				PsendCpkt
				(
					(PKTCNTLTYP<<PKTTYP_S) | (channel<<PKTCHN_S),
					REQ_R_SIZ,
					REQ_RESET
				);
			}
		}
		else
		{
			register Pks_p	pksp;

			for ( pksp = chnp->chn_xpkts ; pksp < &chnp->chn_xpkts[NPKTBUFS] ; pksp++ )
				pksp->pks_xtime += elapsed;
		}

		/*
		**	Receiver timeouts.
		*/

		if ( chnp->chn_rstate == CHNS_RESET )
		{
			register int	timo;

			if ( chnp->chn_xstate != CHNS_RESET )
				timo = PItimo;	/* Remote lost our XMT_RESET? */
			else
				timo = rs_timo;

			if ( (int)(chnp->chn_rtime += elapsed) > timo )
			{
				chnp->chn_rtime = 1;

				PsendCpkt
				(
					(PKTCNTLTYP<<PKTTYP_S) | (channel<<PKTCHN_S),
					XMT_R_SIZ,
					XMT_RESET
				);
			}
		}
		else
		{
			register int	timo;

			if ( chnp->chn_rtime == 0 )
			{
				chnp->chn_timo = 0;
				timo = PRtimo;
			}
			else
			if ( chnp->chn_timo < A_I_COUNT )
			{
				timo = PRtimo;
			}
			else
			{
				chnp->chn_timo = A_I_COUNT;
				timo = PItimo;
			}

			if ( (int)elapsed > timo )
				timo = (int)elapsed;

			if ( (int)(chnp->chn_rtime += elapsed) > timo )
			{
				chnp->chn_rtime = 1;
				chnp->chn_timo++;
#				ifdef	PNproto
				chnp->chn_heldacks = 0;	/* TIMEOUT will ACK */
#				endif	/* PNproto */

				PsendCpkt
				(
					(PKTCNTLTYP<<PKTTYP_S)
					 | (channel<<PKTCHN_S)
					 | (((chnp->chn_rseq-1)&PKTSEQ_M)<<PKTSEQ_S),
					TIMO_SIZ,
					TIMEOUT
				);
			}
		}

		/*
		**	Total timeout.
		*/

		if ( (int)chnp->chn_artime > ia_timo )
			inactive++;
		else
			chnp->chn_artime += elapsed;
	}

	return (bool)(inactive == Pnchans);
}



/*
**	Just send a packet to promt remote.
*/

void
Pidle()
{
	Plastidle = 1;

	if ( Channels[Pfchan].chn_xstate == CHNS_RESET )
		return;

	PsendCpkt((PKTCNTLTYP<<PKTTYP_S)|(Pfchan<<PKTCHN_S), IDLE_SIZ, IDLE);
}
