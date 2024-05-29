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
**	Configuration data required by packet protocol driver
*/

#define	NPKTBUFS	(SEQMOD/2)	/* Multi-buffered protocol */
#define	A_I_COUNT	2		/* Number of timeouts before a channel is deemed to be idle */
#ifdef	ENproto
#undef	VCBUFSIZE
#endif

#ifndef	PBUFSIZE
#if	defined(NNstrpr) || defined(NNstrpr2)
#define	PBUFSIZE	512
#endif
#ifdef	ENproto
#define	PBUFSIZE	8192
#endif
#ifdef	PNproto
#define	PBUFSIZE	1024
#endif
#else	/* PBUFSIZE */
#if	PBUFSIZE < 512
#undef	PBUFSIZE
#define	PBUFSIZE	512
#endif
#endif	/* PBUFSIZE */

typedef unsigned short	Timo_t;

typedef struct 
{					/* Function to be called to ... */ 
	void	(*pg_qPkt_p)();		/*  queue a data packet for transmission */
	void	(*pg_qCpkt_p)();	/*  queue a control packet for transmission */
	int	(*pg_fillPkt_p)();	/*  get more data for transmission */
	void	(*pg_recvData_p)();	/*  pass back data from packet */
	void	(*pg_recvControl_p)();	/*  pass back control data from packet */
	void	(*pg_rTimeout_p)();	/*  indicate remote timeout on idle channel */
	void	(*pg_rReset_p)();	/*  reset receiver */
	void	(*pg_xReset_p)();	/*  reset transmitter */
	void	(*pg_Rread_p)();	/*  read data from remote */
	Timo_t	pg_Itimo;		/* Idle timeout */
	Timo_t	pg_Rtimo;		/* Receive timeout */
	short	pg_xsize;		/* Optimum packet size */
	short	pg_xmax;		/* Maximum transmit packet size (set by remote) */
	short	pg_nbufs;		/* Number of buffers/channel in use */
	short	pg_ovrhd;		/* Packet protocol overhead */
	short	pg_protoT;		/* Protocol type - see below */
	short	pg_nchans;		/* Number of channels in use */
	short	pg_fchan;		/* First channel in use */
	short	pg_idle;		/* Last packet received was an IDLE */
	short	pg_pbmax;		/* Max amount of space to use in i/o buffers */
	short	pg_holdacks;		/* Only ACK every ``holdacks+1'' packets */
	Ulong	pg_inpkts;		/* Total packets received */
	Ulong	pg_outpkts;		/* Total packets transmitted */
	Ulong	pg_inbytes;		/* Total data bytes received */
	Ulong	pg_outbytes;		/* Total data bytes transmitted */
	short	pg_rsize;		/* Current receive packet size (set by remote) */
	short	pg_rnbufs;		/* Current receive bufferring (set by remote) */
}
			Pconf;

Extern Pconf		Pconfig;

#define	PqPkt		Pconfig.pg_qPkt_p
#define	PqCpkt		Pconfig.pg_qCpkt_p
#define	PfillPkt	Pconfig.pg_fillPkt_p
#define	PrecvData	Pconfig.pg_recvData_p
#define	PrecvControl	Pconfig.pg_recvControl_p
#define	PrTimeout	Pconfig.pg_rTimeout_p
#define	PrReset		Pconfig.pg_rReset_p
#define	PxReset		Pconfig.pg_xReset_p
#define	PRread		Pconfig.pg_Rread_p
#define	PItimo		Pconfig.pg_Itimo
#define	PRtimo		Pconfig.pg_Rtimo
#define	PXsize		Pconfig.pg_xsize
#define	PXmax		Pconfig.pg_xmax
#define	Pnbufs		Pconfig.pg_nbufs
#define	Poverhead	Pconfig.pg_ovrhd
#define	PprotoT		Pconfig.pg_protoT
#define	Pnchans		Pconfig.pg_nchans
#define	Pfchan		Pconfig.pg_fchan
#define	Plastidle	Pconfig.pg_idle
#define	PBufMax		Pconfig.pg_pbmax
#define	PHoldAcks	Pconfig.pg_holdacks
#define	PinPkts		Pconfig.pg_inpkts
#define	PoutPkts	Pconfig.pg_outpkts
#define	PinBytes	Pconfig.pg_inbytes
#define	PoutBytes	Pconfig.pg_outbytes
#define	PRsize		Pconfig.pg_rsize
#define	PRnbufs		Pconfig.pg_rnbufs

/*
**	Protocol types.
*/

#ifndef	PNproto
#define	PT_CRC		(PKTCHK_M<<PKTCHK_S)	/* Bit set if uses 16-bit CRC */
#else
#define	PT_CRC		0x200
#endif

#define	PT_DATAGRAM	0x100			/* Uses datagrams */

/*
**	Protocol Handler routines
*/

extern bool
			Pinit(),
			Ptimo();

extern int
			Psend(),
			Psendc();

extern void
			PRreset(),
			PXreset(),
			Pflush(),
			Pidle(),
			Preset(),
			Precv(),
			PsendCpkt(),
			Psendpkt();
