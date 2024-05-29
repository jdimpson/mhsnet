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
**	Send data/control to remote
*/

#include	"global.h"

#include	"Pconfig.h"
#include	"Packet.h"
#include	"Channel.h"
#include	"Pstats.h"
#include	"Proto.h"
#include	"Debug.h"


static bool	PsendBooked;


/*
**	Find an empty packet,
**	 fill it with data from the channel,
**	 and queue it for transmission.
**	Mark the channel active.
**
**	Assumes "channel" is in range.
*/

int
Psend(channel)
	int		channel;
{
	register Chn_p	chnp = &Channels[channel];
	register Pks_p	pksp = chnp->chn_nxpkt;
	register int	i;
	register int	j;

	if ( chnp->chn_xstate == CHNS_RESET || chnp->chn_rstate == CHNS_RESET )
		return 0;

	for ( i = Pnbufs ; --i >= 0 ; )
	{
		if ( pksp->pks_state == XPS_NULL )
		{
			if ( PBufMax <= PXsize && chnp->chn_xbfc == 0 )
			{
				if ( (i = (*PfillPkt)(channel, pksp->pks_pkt.pkt_data, PXsize)) <= 0 )
					return i;
			}
			else
			for ( i = 0 ; i < PXsize ; i += j )
			{
				if ( (j = chnp->chn_xbfc) <= 0 )
				{
					if ( PBufMax <= PXsize )
						break;	/* Sync up */

					if ( i > 0 )
						PsendBooked = true;

					if ( (j = (*PfillPkt)(channel, chnp->chn_xbuf, PBufMax)) <= 0 )
					{
						if ( i > 0 )
						{
							PsendBooked = false;
							break;
						}
						return j;
					}

					PsendBooked = false;
					
					chnp->chn_xbfp = chnp->chn_xbuf;
					chnp->chn_xbfc = j;
				}

				if ( j > (PXsize - i) )
					j = (PXsize - i);
				
				bcopy(chnp->chn_xbfp, &pksp->pks_pkt.pkt_data[i], j);

				chnp->chn_xbfp += j;
				chnp->chn_xbfc -= j;
			}

			Psendpkt
			(
				pksp,
				(PKTDATATYP << PKTTYP_S)
				 | (channel << PKTCHN_S)
				 | ((chnp->chn_xseq++ & PKTSEQ_M) << PKTSEQ_S),
				i
			);

			chnp->chn_xstate = CHNS_ACTIVE;
			PoutBytes += i;
			return i;
		}

		if ( ++pksp >= &chnp->chn_xpkts[Pnbufs] )
			pksp = chnp->chn_xpkts;
	}

	return 0;
}



/*
**	Find an empty packet,
**	 fill it with control data from the channel,
**	 and queue it for transmission.
**	Mark the channel active.
*/

int
Psendc(channel, data, size)
	int		channel;
	char *		data;
	int		size;
{
	register int	i;
	register Chn_p	chnp = &Channels[channel];
	register Pks_p	pksp = chnp->chn_nxpkt;

	if
	(
		(unsigned)channel >= Pnchans
		||
		channel < Pfchan
		||
		size <= 0
		||
		(*data & CONTROL)
		||
		size > PKTDATAZ
	)
		return -1;					/* Illegal control packet */

	if
	(
		PsendBooked
		||
		chnp->chn_xbfc > 0
		||
		chnp->chn_xstate == CHNS_RESET
		||
		chnp->chn_rstate == CHNS_RESET
	)
		return 0;					/* Output blocked */

	for ( i = Pnbufs ; --i >= 0 ; )
	{
		if ( pksp->pks_state == XPS_NULL )
		{
			bcopy(data, pksp->pks_pkt.pkt_data, size);

			Psendpkt
			(
				pksp,
				(PKTCNTLTYP << PKTTYP_S)
				 | (channel << PKTCHN_S)
				 | ((chnp->chn_xseq++ & PKTSEQ_M) << PKTSEQ_S),
				size
			);

			chnp->chn_xstate = CHNS_ACTIVE;
			return 1;
		}

		if ( ++pksp >= &chnp->chn_xpkts[Pnbufs] )
			pksp = chnp->chn_xpkts;
	}

	return 0;						/* Output blocked */
}




/*
**	Add header and CRC to a data packet and queue it for transmission
*/

void
Psendpkt(pksp, hdr, size)
	register Pks_p	pksp;
	int		hdr;
	register int	size;
{
#	ifdef	PNproto
	pksp->pks_pkt.pkt_hdr[PKTHDR] = hdr;
#	else
	pksp->pks_pkt.pkt_hdr[PKTHDR] = PprotoT|hdr;
#	endif

#	if	defined(NNstrpr) || defined(NNstrpr2)
	pksp->pks_pkt.pkt_hdr[PKTSIZE] = size;
#	ifdef	NNstrpr2
	pksp->pks_pkt.pkt_hdr[PKTNSIZE] = ~size;
#	endif	/* NNstrpr2 */
#	endif
#	if	defined(ENproto) || defined(PNproto)
	pksp->pks_pkt.pkt_hdr[PKTSIZE0] = size;
	pksp->pks_pkt.pkt_hdr[PKTSIZE1] = size >> 8;
#	endif

	size += PKTHDRZ;

#	ifndef	ENproto
#	ifndef	PNproto
	if ( PprotoT & PT_CRC )
#	endif
	{
		(void)crc((char *)&pksp->pks_pkt, size);
		size += PKTCRCZ;
	}
#	endif	/* ENproto */

	(*PqPkt)((char *)&pksp->pks_pkt, size);

	pksp->pks_size = size;
	pksp->pks_xtime = 0;
	pksp->pks_state = XPS_WAIT;

	Logpkt(&pksp->pks_pkt, PLOGOUT);
	PSTATS(PS_XPKTS);
	Plastidle = 0;
	PoutPkts++;
}




/*
**	Add header and CRC to a control packet and queue it for transmission.
*/

void
PsendCpkt(hdr, size, data)
	int		hdr;
	register int	size;
	int		data;
{
	Cntlpkt		cpkt;

#	ifdef	PNproto
	cpkt.cpk_hdr[PKTHDR] = hdr;
#	else
	cpkt.cpk_hdr[PKTHDR] = PprotoT|hdr;
#	endif

#	if	defined(NNstrpr) || defined(NNstrpr2)
	cpkt.cpk_hdr[PKTSIZE] = size;
#	ifdef	NNstrpr2
	cpkt.cpk_hdr[PKTNSIZE] = ~size;
#	endif	/* NNstrpr2 */
#	endif
#	if	defined(ENproto) || defined(PNproto)
	cpkt.cpk_hdr[PKTSIZE0] = size & 0xff;
	cpkt.cpk_hdr[PKTSIZE1] = size >> 8;
#	endif

	if ( size >= 1 )
	switch ( data )
	{
#	ifdef	PNproto
	case TIMEOUT:
			cpkt.cpk_data[3] = PXsize >> 8;
			cpkt.cpk_data[2] = PXsize & 0xff;
			cpkt.cpk_data[1] = Pnbufs;
			cpkt.cpk_data[0] = data;
			break;
#	endif	/* PNproto */

	case XMT_RESET:
#			if	defined(ENproto) || defined(PNproto)
			cpkt.cpk_data[3] = PKTDATAZ >> 8;
			cpkt.cpk_data[2] = PKTDATAZ & 0xff;
#			else	/* defined(ENproto) || defined(PNproto) */
			cpkt.cpk_data[2] = PKTDATAZ;
#			endif	/* defined(ENproto) || defined(PNproto) */
			cpkt.cpk_data[1] = NPKTBUFS;
	default:	cpkt.cpk_data[0] = data;
	}

	size += PKTHDRZ;

#	ifndef	ENproto
#	ifndef	PNproto
	if ( PprotoT & PT_CRC )
#	endif
	{
		(void)crc((char *)&cpkt, size);
		size += PKTCRCZ;
	}
#	endif	/* ENproto */

	(*PqCpkt)((char *)&cpkt, size);

	Logpkt((Pkt_p)&cpkt, PLOGOUT);
	PSTATS(PS_XPKTS);
	Plastidle = 0;
	PoutPkts++;
}
