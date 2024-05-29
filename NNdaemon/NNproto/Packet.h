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
**	Node-Node Packet Description
*/

#define	PKTHDR		0		/* Header info in byte 0 */

#ifdef	NNstrpr
#define	PKTSIZE		1		/* Size of data in byte 1 */
#define	PKTHDRZ		2		/* Header size */
#define	PKTDATAZ	128		/* Maximum data size */
#define	PKTCRCZ		2		/* CRC-16 */
#endif	/* NNstrpr */

#ifdef	NNstrpr2
#define	PKTSIZE		1		/* Size of data in byte 1 */
#define	PKTNSIZE	2		/* Complement of size in byte 2 */
#define	PKTHDRZ		3		/* Header size */
#define	PKTDATAZ	128		/* Maximum data size */
#define	PKTCRCZ		2		/* CRC-16 */
#endif	/* NNstrpr2 */

#ifdef	ENproto
#define	PKTSIZE0	1		/* Low size of data in byte 1 */
#define	PKTSIZE1	2		/* High size of data in byte 2 */
#define	PKTHDRZ		3		/* Header size */
#ifdef	pdp11
#define	PKTDATAZ	512		/* Maximum data size */
#else
#define	PKTDATAZ	1024		/* Maximum data size */
#endif
#define	PKTCRCZ		0		/* No CRC-16 */
#endif	/* ENproto */

#ifdef	PNproto
#define	PKTSIZE0	2		/* Low size of data in byte 2 */
#define	PKTSIZE1	1		/* High size of data in byte 1 */
#define	PKTSIZ_M	7		/* 3 bits at bottom of byte 1 */
#define	PKTSIZ_S	0
#define	PKTHDRZ		3		/* Header size */
#ifdef	pdp11
#define	PKTDATAZ	512		/* Maximum data size */
#else
#define	PKTDATAZ	1024		/* Maximum data size */
#endif
#define	PKTCRCZ		2		/* CRC-16 */
#endif	/* PNproto */

/*
**	Definition of bit fields in header byte 0
**	'_S' is low bit position in byte,
**	'_M' is mask for field.
**	(if only bit fields in C were m/c independent, sigh ...)
*/

#ifndef	PNproto
#define	PKTCHK_M	1		/* CRC present bit */
#define	PKTCHK_S	7
#define	PKTTYP_M	3		/* Type */
#define	PKTTYP_S	5
#define	PKTCHN_M	3		/* Channel number */
#define	PKTCHN_S	3
#define	PKTSEQ_M	7		/* Sequence number */
#define	PKTSEQ_S	0
#else	/* PNproto */
#define	PKTTYP_M	3		/* Type */
#define	PKTTYP_S	6
#define	PKTCHN_M	3		/* Channel number */
#define	PKTCHN_S	4
#define	PKTSEQ_M	017		/* Sequence number */
#define	PKTSEQ_S	0
#endif	/* PNproto */

#define	MAXCHANS	(PKTCHN_M+1)	/* Number of channels */
#define	SEQMOD		(PKTSEQ_M+1)	/* Range for sequence number */

/*
**	Packet Types
*/

#define	PKTDATATYP	0		/* Packet contains message data */
#define	PKTACKTYP	1		/* Packet acknowledgement */
#define	PKTNAKTYP	2		/* Packet negative acknowledgement */
#define	PKTCNTLTYP	3		/* Packet contains control data */

/*
**	Structure for a maximum sized packet
*/

typedef struct 
{
	char	pkt_hdr[PKTHDRZ];
	char	pkt_data[PKTDATAZ];
#	ifndef	ENproto
	char	pkt_crc[PKTCRCZ];
#	endif
}
			Packet;

typedef Packet *	Pkt_p;

/*
**	Structure for out-of-band control packets
*/

#if	defined(NNstrpr) || defined(NNstrpr2)
#define	CNTLDATAZ	3
#endif
#if	defined(ENproto) || defined(PNproto)
#define	CNTLDATAZ	4
#endif

typedef struct 
{
	char	cpk_hdr[PKTHDRZ];
	char	cpk_data[CNTLDATAZ];
#	ifndef	ENproto
	char	cpk_crc[PKTCRCZ];
#	endif
}
			Cntlpkt;

/*
**	Structure for in-band reply packets
*/

typedef struct 
{
	char	rpk_hdr[PKTHDRZ];
#	ifndef	ENproto
	char	rpk_crc[PKTCRCZ];
#	endif
}
			Replypkt;

/*
**	Table of ordered sequence numbers
*/

Extern char		SeqTable[2*SEQMOD];
