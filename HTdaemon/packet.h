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
**	HT packet format:-
**
**		___________________________________________________________________
**		!         !      !      !     !      !     !              !       !
**	Field: 	! ADDRESS ! TYPE ! CHAN ! DIR ! SIZE ! CRC !     DATA     ! [CRC] !
**		!         !      !      !     !      !     !              !       !
**	Bits:	!    32   !   3  !   2  !  1  !  18  !  16 ! (0-131072)*8 ! 16/32 !
**		!         !      !      !     !      !     !              !       !
**		-------------------------------------------------------------------
*/

#define	MAXPKTDATASIZE	131072
#define	MINPKTDATASIZE	8
#define	MAXCRCSIZE	1024
#define	MAXPKTCHAN	4
#define	MAXPKTTYPE	8

/*
**	Structure of a packet.
*/

typedef struct
{
	Uchar	pkt_addr[4];
	Uchar	pkt_type[3];
#	define	PKTHDRSIZE	 7
	Uchar	pkt_hcrc[CRC_SIZE];
	Uchar	pkt_data[1];		/* Can be MAXPKTDATASIZE */
/*	Uchar	pkt_dcrc[CRC32_SIZE];	*/
}
			Packet;

#define	MAXPKTSIZE		(PKTHDRSIZE+MAXPKTDATASIZE+CRC_SIZE+CRC32_SIZE)

/*
**	Macros to access a packet.
*/

#define	A_PKTTYPE(P)		(((Packet *)(P))->pkt_type)
#define	A_PKTADDR(P)		(((Packet *)(P))->pkt_addr)
#define	A_PKTDATA(P)		(((Packet *)(P))->pkt_data)

#define	G_PKTTYPE(P)		(((A_PKTTYPE(P)[0])>>5)&07)
#define	G_PKTCHAN(P)		(((A_PKTTYPE(P)[0])>>3)&03)
#define	G_PKTDIR(P)		(((A_PKTTYPE(P)[0])>>2)&01)
#define	G_PKTSIZE(P)		((((A_PKTTYPE(P)[0])&03)<<16) | ((A_PKTTYPE(P)[1])<<8) | (A_PKTTYPE(P)[2]))
#define	G_PKTADDR(P)		((Ulong)(A_PKTADDR(P)[0])\
					|(((Ulong)(A_PKTADDR(P)[1]))<<8)\
					|(((Ulong)(A_PKTADDR(P)[2]))<<16)\
					|(((Ulong)(A_PKTADDR(P)[3]))<<24))

#define	G_PKTHCRC(P)		tcrc((char *)(P), PKTHDRSIZE)
#define	G_PKTDCRC(P,S)		((S>MAXCRCSIZE)?tcrc32((char *)A_PKTDATA(P), S):tcrc((char *)A_PKTDATA(P), S))

/*
**	Macros to make a packet.
*/

#define	S_PKTTYPE(P,T)		A_PKTTYPE(P)[0] = (A_PKTTYPE(P)[0]&0037) | ((T)<<5)
#define	S_PKTCHAN(P,C)		A_PKTTYPE(P)[0] = (A_PKTTYPE(P)[0]&0347) | ((C)<<3)
#define	S_PKTDIR(P,D)		A_PKTTYPE(P)[0] = (A_PKTTYPE(P)[0]&0373) | ((D)<<2)
#define	S_PKTSIZE(P,S)		A_PKTTYPE(P)[0] = (A_PKTTYPE(P)[0]&0374) | ((S)>>16); \
					A_PKTTYPE(P)[1] = (S)>>8; \
					A_PKTTYPE(P)[2] = (S)
#define	S_PKTHDR(P,T,C,D,S)	A_PKTTYPE(P)[0] = ((T)<<5) | ((C)<<3) | ((D)<<2) | ((S)>>16); \
					A_PKTTYPE(P)[1] = (S)>>8; \
					A_PKTTYPE(P)[2] = (S)
#define	S_PKTADDR(P,A)		A_PKTADDR(P)[0] = (A); \
					A_PKTADDR(P)[1] = ((A)>>8); \
					A_PKTADDR(P)[2] = ((A)>>16); \
					A_PKTADDR(P)[3] = ((A)>>24)
#define	S_PKTHCRC(P)		(void)crc((char *)(P), PKTHDRSIZE)
#define	S_PKTDCRC(P,S)		((S>MAXCRCSIZE)?(void)crc32((char *)A_PKTDATA(P), S):(void)crc((char *)A_PKTDATA(P), S))

/*
**	Packet types.
*/

enum
{
	pktdatatype, pktnaktype,
	pktsomtype, pktsomatype,
	pkteomtype, pkteomatype,
	pktmqetype, pkteottype
};

#define	PKTDATATYPE	((int)pktdatatype)	/* Addr: start address for data */
#define	PKTNAKTYPE	((int)pktnaktype)	/* Addr: start address; Data: type & size of bad packet */
#define	PKTSOMTYPE	((int)pktsomtype)	/* Addr: message size; Data: message ID, wait time */
#define	PKTSOMATYPE	((int)pktsomatype)	/* Addr: start address; Data: message ID */
#define	PKTEOMTYPE	((int)pkteomtype)	/* Addr: message size; Data: message ID */
#define	PKTEOMATYPE	((int)pkteomatype)	/* Addr: message size; Data: message ID */
#define	PKTMQETYPE	((int)pktmqetype)	/* Addr: 0; Data: link name */
#define	PKTEOTTYPE	((int)pkteottype)	/* Addr: 0 */

/*
**	NAK packets have a TYPE and LENGTH in the data field.
*/

#define	G_NAKTYPE(P)		((((Uchar *)(P))[0])&07)
#define	G_NAKLENGTH(P)		(((Ulong)((Uchar *)(P))[1])\
					|(((Ulong)((Uchar *)(P))[2])<<8)\
					|(((Ulong)((Uchar *)(P))[3])<<16)\
					|(((Ulong)((Uchar *)(P))[4])<<24))

#define	S_NAKTYPE(P,T)		((Uchar *)(P))[0] = ((Uchar)T)
#define	S_NAKLENGTH(P,L)	((Uchar *)(P))[1] = (L); \
					((Uchar *)(P))[2] = ((L)>>8); \
					((Uchar *)(P))[3] = ((L)>>16); \
					((Uchar *)(P))[4] = ((L)>>24)
#define	S_NAKDATA(P,T,L)	S_NAKTYPE(P,T); S_NAKLENGTH(P,L)

#define	NAKDATASIZE		5

/*
**	SOM packets have a queue time and message ID in the data field.
*/

#define	G_SOMTIME(P)		(((Ulong)((Uchar *)(P))[0])\
					|(((Ulong)((Uchar *)(P))[1])<<8)\
					|(((Ulong)((Uchar *)(P))[2])<<16)\
					|(((Ulong)((Uchar *)(P))[3])<<24))
#define	A_SOMID(P)		((char *)&(((Uchar *)(P))[4]))

#define	S_SOMTIME(P,A)		((Uchar *)(P))[0] = (A); \
					((Uchar *)(P))[1] = ((A)>>8); \
					((Uchar *)(P))[2] = ((A)>>16); \
					((Uchar *)(P))[3] = ((A)>>24)
#define	S_SOMID(P,D)		(void)strncpy(A_SOMID(P), (D), MSGIDLENGTH)
#define	S_SOMDATA(P,A,D)	S_SOMTIME(P,A); S_SOMID(P,D)

#define	SOMMDATASIZE		(4+MSGIDLENGTH)

/*
**	SOMA packets have a message ID in the data field.
*/

#define	A_SOMAID(P)		((char *)((Uchar *)(P)))

#define	S_SOMAID(P,D)		(void)strncpy(A_SOMAID(P), (D), MSGIDLENGTH)
#define	S_SOMADATA(P,D)		S_SOMAID(P,D)

#define	SOMADATASIZE		MSGIDLENGTH

/*
**	EOM packets have a message ID in the data field.
*/

#define	A_EOMID(P)		((char *)((Uchar *)(P)))

#define	S_EOMID(P,D)		(void)strncpy(A_EOMID(P), (D), MSGIDLENGTH)
#define	S_EOMDATA(P,D)		S_EOMID(P,D)

#define	EOMDATASIZE		MSGIDLENGTH

/*
**	EOMA packets have a message ID in the data field.
*/

#define	A_EOMAID(P)		((char *)((Uchar *)(P)))

#define	S_EOMAID(P,D)		(void)strncpy(A_EOMAID(P), (D), MSGIDLENGTH)
#define	S_EOMADATA(P,D)		S_EOMAID(P,D)

#define	EOMADATASIZE		MSGIDLENGTH

/*
**	Control packets have a link name in the data field.
*/

#define	A_MQELINK(P)		((char *)&(((Uchar *)(P))[0]))

#define	S_MQELINK(P,L,S)	(void)strncpy(A_MQELINK(P), (L), (S))
#define	S_MQEDATA(P,L,S)	S_MQELINK(P,L,S)

#define	MQEMDATASIZE		0			/* (minimum) */

#define	MQE			0			/* Channel for MQE */
#define	SYNC			1			/* Channel for SYNC */
#define	IDLE			2			/* Channel for IDLE */
#define	CTRL			3			/* Channel for CTRL */

/*
**	`CTRL' packets also have type in address field.
*/

#define	CTRL_LINKDOWN		0			/* Signal link down */
#define	CTRL_NOSPACE		1			/* Signal no space */

/*
**	EOT packets have no data field.
*/

#define	EOTDATASIZE		0

#define	REOT			0			/* Channel for remote EOT */
#define	LEOT			1			/* Channel for local EOT */


/*
**	Packet tracing.
*/

#if	PROTO_STATS >= 1

typedef enum
{
	dir_vcin, dir_vcout, dir_pipin, dir_pipout
}
	PktDir;

#define	TracePkt(D,S,C,R)	if(PktTraceflag)_TracePkt(D,S,C,R)
extern void		_TracePkt _FA_((PktDir, Uchar *, char *, bool));

#define	TraceData(N,S,D)	if(Traceflag>=4||PktTraceflag>=2)_TraceData(N,S,D)
extern void		_TraceData _FA_((char *, int, char *));

#else	/* PROTO_STATS >= 1 */

#define	TracePkt(D,S,C,R)
#define	TraceData(N,S,D)

#endif	/* PROTO_STATS >= 1 */

Extern int		PktTraceflag;

/*
**	Externals.
*/

extern Uchar *		PktWriteData;
extern long		PktBufSize;
extern long		PktDatSize;

extern void		AdjPktSize _FA_((Ulong));
extern long		MaxPktSize _FA_((long));
extern int		WritePkt _FA_((int, int, int, Ulong, int, char *));
extern void		InitPktData _FA_((void));
