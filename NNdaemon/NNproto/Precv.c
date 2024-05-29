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
**	Receive data from remote, and disassemble packets
*/

#include	"global.h"

#include	"Pconfig.h"
#include	"Packet.h"
#include	"Channel.h"
#include	"Pstats.h"
#include	"Proto.h"
#include	"Debug.h"


#if	DEBUG >= 1
int			BadHdrs;
#endif

static Packet		Pkt;
static int		Sequence;
static int		Channo;
static Chn_p		Chnp;
static int		Datasize;

#define	HDR		Pkt.pkt_hdr[PKTHDR]
#if	defined(NNstrpr) || defined(NNstrpr2)
#define	SIZ		Pkt.pkt_hdr[PKTSIZE]
#define	NSIZ		Pkt.pkt_hdr[PKTNSIZE]
#define	DATASIZE	BYTE(SIZ)
#define	DATANSIZE	BYTE(NSIZ)
#endif
#ifdef	ENproto
#define	SIZ0		Pkt.pkt_hdr[PKTSIZE0]
#define	SIZ1		Pkt.pkt_hdr[PKTSIZE1]
#define	DATASIZE	(BYTE(SIZ0) | (BYTE(SIZ1) << 8))
#endif
#ifdef	PNproto
#define	SIZ		Pkt.pkt_hdr[PKTSIZE1]
#define	NSIZ		Pkt.pkt_hdr[PKTSIZE0]
#define	DATASIZE	(((BYTE(SIZ)/*&PKTSIZ_M*/)<<(8/*-PKTSIZ_S*/)) | BYTE(NSIZ))
#else	/* PNproto */
#define	CRCUNMATCHED	((PprotoT^HDR)&PT_CRC)
#define	CRCPRESENT	(HDR&(PKTCHK_M<<PKTCHK_S))
#endif	/* PNproto */
#define	CHANNEL		((HDR>>PKTCHN_S)&PKTCHN_M)
#define	SEQUENCE	((HDR>>PKTSEQ_S)&PKTSEQ_M)
#define	TYPE		((HDR>>PKTTYP_S)&PKTTYP_M)
#define	ACK		(PKTACKTYP<<PKTTYP_S)
#define	NAK		(PKTNAKTYP<<PKTTYP_S)
#define	DATA0		BYTE(Pkt.pkt_data[0])
#define	DATA1		BYTE(Pkt.pkt_data[1])
#define	DATA2		BYTE(Pkt.pkt_data[2])
#define	DATA3		BYTE(Pkt.pkt_data[3])
#define	Nextseq		Chnp->chn_rseq

static bool
			Movenak(),
			Retry(),
			Skipseq();

static void
			FlushData(),
			Rack(),
			Rcontrol(),
			Rnak(),
			Reply(),
			BufferData();

extern void		StVCnak(),
			StVCpkt(),
			StVCskip(/*(int)*/);



void
Precv()
{
#	ifdef	ENproto
	/*
	**	Read header, turn on/off timeout
	*/

	(*PRread)((char *)&Pkt, sizeof Pkt, true, true);	

	if ( (Datasize = DATASIZE) > PKTDATAZ )
	{
		PSTATS(PS_BADHDR);
#		if	DEBUG >= 1
		if ( BadHdrs++ == 0 )
			Logpkt(&Pkt, PLOGIN);
#		endif	/* DEBUG */
		StVCskip(sizeof Pkt);
		return;
	}

#	else	/* ENproto */

	register int	datacount;

	/*
	**	Read header, turn on timeout
	*/

	(*PRread)((char *)&Pkt, PKTHDRZ, true, false);	

	while
	(
#		ifdef	PNproto
		(datacount = DATASIZE) > PRsize	/* Probably means that PNproto shouldn't have dynamically adjusting sizes */
#		else	/* PNproto */
		(datacount = DATASIZE) > PKTDATAZ
#		ifdef	NNstrpr2
		||
		DATANSIZE != ((~datacount)&0xff)
#		endif
		||
		CRCUNMATCHED
#		endif
	)
	{
		StVCskip(1);
		PSTATS(PS_BADHDR);
#		if	DEBUG >= 1
		if ( BadHdrs++ == 0 )
			Logpkt(&Pkt, PLOGIN);
#		endif	/* DEBUG */
		HDR = SIZ;	/* Shift bytes up */
#		ifdef	NNstrpr
		(*PRread)(&SIZ, 1, false, false);
#		endif
#		if	defined(NNstrpr2) || defined(PNproto)
		SIZ = NSIZ;
		(*PRread)(&NSIZ, 1, false, false);
#		endif
	}

	Datasize = datacount;

#	ifndef	PNproto
	if ( CRCPRESENT )
#	endif
		datacount += PKTCRCZ;

	/*
	**	Read data, turn off timeout
	*/

	(*PRread)(Pkt.pkt_data, datacount, false, true);

	Logpkt(&Pkt, PLOGIN);
	PinPkts++;

#	if	DEBUG >= 1
	if ( BadHdrs )
	{
		Tracehdrs(BadHdrs);
		Dumphist(PDESC(PS_BADHDR));
		BadHdrs = 0;
	}
#	endif	/* DEBUG */

	if
	(
#		ifndef	PNproto
		CRCPRESENT
		&&
#		endif
		crc((char *)&Pkt, datacount+PKTHDRZ-PKTCRCZ)
	)
	{
		StVCskip(datacount+PKTHDRZ);
		PSTATS(PS_BADCRC);
		Dumphist(PDESC(PS_BADCRC));
		return;
	}
#	endif	/* ENproto */

	if ( (Channo = CHANNEL) >= Pnchans || Channo < Pfchan )
	{
		PSTATS(PS_BADCHAN);
		Dumphist(PDESC(PS_BADCHAN));
		return;
	}

	PSTATS(PS_RPKTS);
	StVCpkt(0);

	Chnp = &Channels[Channo];

	Chnp->chn_artime = 0;

	Sequence = SEQUENCE;
	Plastidle = 0;

	switch ( TYPE )
	{
	case PKTCNTLTYP:
		if ( DATA0 & CONTROL )
		{
			Rcontrol();
			break;
		}
	case PKTDATATYP:
		if ( Chnp->chn_rstate == CHNS_RESET )
			break;

		Chnp->chn_rtime = 0;

		if ( Sequence == Nextseq )
		{
			do
			{
				Nextseq = (Sequence+1) & PKTSEQ_M;

				if ( TYPE == PKTDATATYP )
					BufferData();
				else
				{
					FlushData();
					(*PrecvControl)(Channo, Pkt.pkt_data, Datasize);
#					ifdef	PNproto
					Chnp->chn_heldacks = PHoldAcks;	/* Force next ACK */
#					endif	/* PNproto */
				}
			}
			while
				( Chnp->chn_rnak && Movenak() );

			Reply(ACK);
		}
		else
		if ( Retry() )
		{
			PSTATS(PS_RDUP);
			Sequence = (Nextseq-1) & PKTSEQ_M;
			Reply(ACK);
		}
		else
		if ( Skipseq() )
		{
			PSTATS(PS_SKPSEQ);
			Reply(NAK);
			StVCnak(0);
		}
		else
		{
			PSTATS(PS_OUTSEQ);
			Dumphist(PDESC(PS_OUTSEQ));
			Preset(Channo);	/* Bad news... */
		}
		break;

	case PKTACKTYP:
		if ( Chnp->chn_xstate != CHNS_RESET )
			Rack(0);
		break;

	case PKTNAKTYP:
		if ( Chnp->chn_xstate != CHNS_RESET )
		{
			StVCnak(1);
			Rnak();
		}
		break;
	}
}



/*
**	Buffer the data
*/

static void
BufferData()
{
	if ( Datasize > Chnp->chn_rbfc )
		FlushData();

	PinBytes += Datasize;

	if ( Datasize >= PBufMax )
	{
		(*PrecvData)(Channo, Pkt.pkt_data, Datasize);
		return;
	}

	bcopy(Pkt.pkt_data, Chnp->chn_rbfp, Datasize);

	Chnp->chn_rbfp += Datasize;
	Chnp->chn_rbfc -= Datasize;
}



/*
**	Flush buffered data
*/

static void
FlushData()
{
	register int	size;

	if ( (size = PBufMax-Chnp->chn_rbfc) > 0 )
		(*PrecvData)(Channo, Chnp->chn_rbuf, size);

	Chnp->chn_rbfp = Chnp->chn_rbuf;
	Chnp->chn_rbfc = PBufMax;
}



/*
**	Flush all the buffers.
*/

void
Pflush()
{
	for
	(
		Channo = Pfchan, Chnp = &Channels[Pfchan] ;
		Channo < Pnchans ;
		Channo++, Chnp++
	)
		if ( Chnp->chn_rbfp > Chnp->chn_rbuf )
		{
			FlushData();

#			ifdef	PNproto
			if ( Chnp->chn_heldacks > 0 )
			{
				Chnp->chn_heldacks = 0;

				PsendCpkt
				(
					ACK
					 | (Channo<<PKTCHN_S)
					 | (((Nextseq-1)&PKTSEQ_M)<<PKTSEQ_S),
					0,
					0
				);
			}
#			endif	/* PNproto */
		}
}



/*
**	Is this packet a valid re-transmission?
*/

static bool
Retry()
{
	register char *	lastp = &SeqTable[Nextseq+SEQMOD-1];
	register char *	nextp = lastp - (NPKTBUFS-1);

	do
		if ( *nextp == Sequence )
			return true;
	while
		( ++nextp <= lastp );

	return false;
}



/*
**	Is this a valid subsequent packet after a skipped sequence?
*/

static bool
Skipseq()
{
	register char *	lastp = &SeqTable[Nextseq+NPKTBUFS-1];
	register char *	nextp = &SeqTable[Nextseq+1];

	do
		if ( *nextp == Sequence )
		{
			/*
			**	It is indeed, hold it in an empty slot.
			*/

			register Pks_p	pksp;

			for
			(
				pksp = Chnp->chn_rpkts ;
				pksp < &Chnp->chn_rpkts[NPKTBUFS-1] ;
				pksp++
			)
				if ( pksp->pks_state == RPS_NULL )
				{
					pksp->pks_pkt = Pkt;
					pksp->pks_state = RPS_NAK;
					Chnp->chn_rnak++;
					return true;
				}
				else
				if ( ((pksp->pks_pkt.pkt_hdr[PKTHDR]>>PKTSEQ_S)&PKTSEQ_M) == Sequence )
					return true;

			/*
			**	Oops, no empty slot, should probably PANIC.
			*/

			return false;
		}
	while
		( ++nextp <= lastp );

	return false;
}



/*
**	Move any NAKed packets that are now in sequence into packet.
*/

static bool
Movenak()
{
	register Pks_p	pksp;

	for
	(
		pksp = Chnp->chn_rpkts ;
		pksp < &Chnp->chn_rpkts[NPKTBUFS-1] ;
		pksp++
	)
		if
		(
			pksp->pks_state == RPS_NAK
			&&
			((pksp->pks_pkt.pkt_hdr[PKTHDR]>>PKTSEQ_S)&PKTSEQ_M) == Nextseq
		)
		{
			pksp->pks_state = RPS_NULL;
			Pkt = pksp->pks_pkt;
			Sequence = Nextseq;
			Datasize = DATASIZE;
			Chnp->chn_rnak--;
			return true;
		}

	return false;
}



/*
**	Send a reply packet
*/

static void
Reply(type)
	int	type;
{
#	ifdef	PNproto
	if ( type == ACK )
	{
		if ( ++Chnp->chn_heldacks <= PHoldAcks )
			return;
		
		Chnp->chn_heldacks = 0;
	}
	else
		Chnp->chn_heldacks = PHoldAcks;	/* Force next ACK */
#	endif	/* PNproto */

	PsendCpkt
	(
		type | (Channo<<PKTCHN_S) | (Sequence<<PKTSEQ_S),
		0,
		0
	);
}



/*
**	This and all lesser sequenced packets ok
*/

static void
Rack(hit)
	int		hit;
{
	register Pks_p	pksp = Chnp->chn_nxpkt;
	register char *	lastp = &SeqTable[Sequence+SEQMOD];
	register char *	nextp = lastp - (Pnbufs-1);

	if ( Datasize == 0 )
	do
	{
		if ( *nextp == ((pksp->pks_pkt.pkt_hdr[PKTHDR] >> PKTSEQ_S) & PKTSEQ_M) )
		{
			if ( pksp->pks_state != XPS_NULL )
			{
				pksp->pks_state = XPS_NULL;
				hit++;
			}

			if ( ++pksp >= &Chnp->chn_xpkts[Pnbufs] )
				pksp = Chnp->chn_xpkts;

			Chnp->chn_nxpkt = pksp;
			(void)Psend(Channo);
		}
	}
	while
		( ++nextp <= lastp );

	if ( hit )
	{
#		if	PSTATISTICS == 1
		Pstats[PS_LOSTACK].ss_count += hit-1;
#		endif	/* PSTATISTICS */
	}
	else
	{
		PSTATS(PS_BADACK);
		Dumphist(PDESC(PS_BADACK));
	}
}



/*
**	Mark this packet NAKed, and retransmit the previous packet.
*/

static void
Rnak()
{
	register Pks_p	pksp = Chnp->chn_nxpkt;
	register char *	lastp = &SeqTable[Sequence+SEQMOD];
	register char *	nextp = lastp - (Pnbufs-1);
	register int	hit = 0;

	if ( Datasize == 0 )
	do
	{
		if ( *nextp == ((pksp->pks_pkt.pkt_hdr[PKTHDR] >> PKTSEQ_S) & PKTSEQ_M) )
		{
			if ( pksp->pks_state == XPS_WAIT )
			{
				if ( nextp == lastp )
				{
					pksp->pks_state = XPS_NAK;
					break;
				}
				else
				if ( nextp == (lastp-1) )
				{
					pksp->pks_xtime = 0;
					(*PqPkt)((char *)&pksp->pks_pkt, pksp->pks_size);
					Logpkt(&pksp->pks_pkt, PLOGOUT);
					PSTATS(PS_NAKPKT);
					PoutPkts++;
				}

				hit++;
			}
			else
			if ( pksp->pks_state == XPS_NAK )
				hit++;

			if ( ++pksp >= &Chnp->chn_xpkts[Pnbufs] )
				pksp = Chnp->chn_xpkts;
		}
	}
	while
		( ++nextp <= lastp );

	if ( !hit )
	{
		PSTATS(PS_BADNAK);
		Dumphist(PDESC(PS_BADNAK));
	}
}



/*
**	Process a control packet.
*/

static void
Rcontrol()
{
	register Pks_p	pksp;
	register int	i;
	register int	hit;

	switch ( DATA0 )
	{
	case TIMEOUT:
		if ( Datasize != TIMO_SIZ )
			break;

		if ( Chnp->chn_xstate == CHNS_RESET )
		{
			if ( Chnp->chn_rstate != CHNS_RESET || Chnp->chn_xtime <= 1 )
				return;

			Chnp->chn_xtime = 1;

			PsendCpkt
			(
				(PKTCNTLTYP<<PKTTYP_S) | (Channo<<PKTCHN_S),
				REQ_R_SIZ,
				REQ_RESET
			);

			Chnp->chn_rtime = 1;

			PsendCpkt
			(
				(PKTCNTLTYP<<PKTTYP_S) | (Channo<<PKTCHN_S),
				XMT_R_SIZ,
				XMT_RESET
			);

			return;
		}

#		ifdef	PNproto
		PRsize = (DATA3<<8)|DATA2;
		PRnbufs = DATA1;
#		endif	/* PNproto */

		/*
		**	Use sequence to acknowledge packets.
		*/

		Datasize = 0;
		Rack(1);

		/*
		**	Re-transmit any packets that have been waiting long enough;
		**	but if there are no waiting packets,
		**	call higher level function, and mark channel idle.
		*/

		for ( hit = 0, pksp = Chnp->chn_nxpkt, i = Pnbufs ; --i >= 0 ; )
		{
			if ( pksp->pks_state == XPS_WAIT )
			{
				if ( pksp->pks_xtime >= PRtimo )
				{
					pksp->pks_xtime = 0;
					(*PqPkt)((char *)&pksp->pks_pkt, pksp->pks_size);
					Logpkt(&pksp->pks_pkt, PLOGOUT);
					PSTATS(PS_TIMOPKT);
					PoutPkts++;
				}

				hit++;
			}

			if ( ++pksp >= &Chnp->chn_xpkts[Pnbufs] )
				pksp = Chnp->chn_xpkts;
		}

		if ( !hit )
		{
			Chnp->chn_xstate = CHNS_IDLE;
			(*PrTimeout)(Channo);
		}
		else
		if ( hit < Pnbufs )
			while ( Psend(Channo) > 0 );

		return;

	case XMT_RESET:
		if ( Datasize != XMT_R_SIZ )
			break;

		if ( (int)DATA1 < (int)Pnbufs )
			Pnbufs = DATA1;

#		if	defined(ENproto) || defined(PNproto)
		i = (DATA3<<8)|DATA2;
#		endif
#		if	defined(NNstrpr) || defined(NNstrpr2)
		i = DATA2;
#		endif

		if ( i < PXmax )
		{
			PXmax = i;

			if ( i < PXsize )
				PXsize = i;
		}

		/*
		**	Send an acknowledgement
		*/

		PsendCpkt
		(
			(PKTCNTLTYP<<PKTTYP_S)|(Channo<<PKTCHN_S),
			ACK_R_SIZ,
			ACK_RESET
		);

		PXreset(Channo);

		return;

	case ACK_RESET:
		if ( Datasize != ACK_R_SIZ )
			break;

		PRreset(Channo);

		return;

	case REQ_RESET:
		if ( Datasize != REQ_R_SIZ )
			break;

		if ( Chnp->chn_rstate != CHNS_RESET )
			Preset(Channo);

		return;

	case IDLE:
		Plastidle = 1;
		return;
	}

	PSTATS(PS_BADCNTL);
	Dumphist(PDESC(PS_BADCNTL));
}
