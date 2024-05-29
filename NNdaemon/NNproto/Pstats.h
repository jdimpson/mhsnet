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
**	Channel conversation statistics
*/

#if	PROTO_STATS == 1

#define	PSTATISTICS	1
#define	PSTATSDESC	1

#endif	/* PROTO_STATS */

#if	PSTATISTICS == 1

typedef struct 
{
#	if	PSTATSDESC == 1
	char *	ss_descp;
#	endif
	long	ss_count;
}
			Statistic;

enum
{
	 ss_badhdr, ss_badsize, ss_badcrc
	,ss_outseq, ss_skpseq, ss_rdup
	,ss_lostack, ss_badack, ss_badnak
	,ss_nakpkt, ss_timopkt
	,ss_badcntl, ss_badxst, ss_badchan
	,ss_rpkts, ss_xpkts
	,ss_nstats
};

#define	PS_BADHDR	(int)ss_badhdr		/* Header inconsistent */
#define	PS_BADSIZE	(int)ss_badsize		/* Packet size too large */
#define	PS_BADCRC	(int)ss_badcrc		/* CRC error */
#define	PS_OUTSEQ	(int)ss_outseq		/* Packet out of sequence */
#define	PS_SKPSEQ	(int)ss_skpseq		/* Packet skipped */
#define	PS_RDUP		(int)ss_rdup		/* Duplicate packet received */
#define	PS_LOSTACK	(int)ss_lostack		/* ACK for packet lost */
#define	PS_BADACK	(int)ss_badack		/* ACK for non-existent packet */
#define	PS_BADNAK	(int)ss_badnak		/* NAK for non-existent packet */
#define	PS_NAKPKT	(int)ss_nakpkt		/* Retransmitted by NAK */
#define	PS_TIMOPKT	(int)ss_timopkt		/* Retransmitted by timeout */
#define	PS_BADCNTL	(int)ss_badcntl		/* Unrecognised control packet */
#define	PS_BADXST	(int)ss_badxst		/* State/acknowledge out of sync */
#define	PS_BADCHAN	(int)ss_badchan		/* Channel number out of range */
#define	PS_RPKTS	(int)ss_rpkts		/* Packets received */
#define	PS_XPKTS	(int)ss_xpkts		/* Packets transmitted */
#define	PS_NSTATS	(int)ss_nstats

Extern Statistic	Pstats[PS_NSTATS]
#if	PSTATSDESC && PSTATSDATA
=
{
	 {"bad header"}
	,{"bad size"}
	,{"bad crc"}
	,{"out of sequence"}
	,{"lost packets"}
	,{"duplicate packets received"}
	,{"multiple/lost ack"}
	,{"bad ack"}
	,{"bad nak"}
	,{"retransmitted by NAK"}
	,{"retransmitted by timeout"}
	,{"bad control"}
	,{"bad xstate"}
	,{"bad channel"}
	,{"packets received"}
	,{"packets transmitted"}
}
#endif	/* PSTATSDATA */
;

#define	PSTATS(A)	Pstats[A].ss_count++
#define	PDESC(A)	Pstats[A].ss_descp

#else	/* PSTATISTICS */

#define	PSTATS(A)
#define	PDESC(A)

#endif	/* PSTATISTICS */
