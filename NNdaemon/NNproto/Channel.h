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
**	A packet and its state
*/

#ifndef	NPKTBUFS
#include	"Pconfig.h"
#endif
#ifndef	PKTHDR
#include	"Packet.h"
#endif

typedef struct 
{
	Packet		pks_pkt;		/* Space for a packet */
	short		pks_state;		/* State of transmission */
	short		pks_size;		/* Real size of packet */
	Timo_t		pks_xtime;		/* Time since transmission */
}
			Pktstate;

typedef Pktstate *	Pks_p;

/*
**	Transmit packet states
*/

enum {	xps_null, xps_wait, xps_nak };

#define	XPS_NULL	(int)xps_null		/* Empty packet */
#define	XPS_WAIT	(int)xps_wait		/* Packet awaiting acknowledgement */
#define	XPS_NAK		(int)xps_nak		/* Packet has been received out of sequence */

/*
**	Receive packet states
*/

enum {	rps_null, rps_nak };

#define	RPS_NULL	(int)rps_null		/* Empty packet */
#define	RPS_NAK		(int)rps_nak		/* Packet has been received out of sequence */

/*
**	Data for controlling a Channel
*/

typedef struct 
{
	Pktstate	chn_xpkts[NPKTBUFS];	/* Packets being transmitted */
	Pktstate	chn_rpkts[NPKTBUFS-1];	/* Packets received, but NAKed */
	Pks_p		chn_nxpkt;		/* Next packet to be acknowledged */
	char *		chn_xbfp;		/* Position in xmit buffer */
	char *		chn_rbfp;		/* Position in recv buffer */
	Timo_t		chn_rtime;		/* Time since last data packet received */
	Timo_t		chn_artime;		/* Time since any packet received */
	short		chn_rnak;		/* Count of packets in "rpkts" */
	short		chn_rseq;		/* Next receive sequence number */
	short		chn_xseq;		/* Next transmit sequence number */
	short		chn_rstate;		/* Receive channel state */
	short		chn_xstate;		/* Transmit channel state */
	short		chn_timo;		/* Receive timeout count */
#	ifdef	PNproto
	short		chn_heldacks;		/* Outstanding acknowledgements */
#	endif
	short		chn_xbfc;		/* Bytes to go from buffer */
	short		chn_rbfc;		/* Space left in buffer */
	char		chn_xbuf[PBUFSIZE];	/* Xmit buffer */
	char		chn_rbuf[PBUFSIZE];	/* Recv buffer */
}
			Channel;

typedef Channel *	Chn_p;

extern Channel		Channels[];		/* A structure for each channel */

#define	chn_xtime	chn_xseq		/* Re-use xseq as xtime while CHNS_RESET */

/*
**	Channel transmitter states
*/

enum {	chns_idle, chns_active, chns_reset };

#define	CHNS_IDLE	(int)chns_idle		/* Nothing being transmitted */
#define	CHNS_ACTIVE	(int)chns_active	/* Transmission is active */
#define	CHNS_RESET	(int)chns_reset		/* Local channel has been reset */
