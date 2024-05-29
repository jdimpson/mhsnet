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
**	Format of status structure/file.
*/

#define	NCHANNELS	4

/*
**	Status structure.
*/

typedef struct
{
	Chan		st_channels[NCHANNELS];
	Time_t		st_start;		/* Start time */
	Time_t		st_now;			/* Current time */
	Time_t		st_lastpkt;		/* Time last packet transmitted */
	Ulong		st_act_rawbytes;	/* Decayed active raw bytes received */
	Ulong		st_act_bytes;		/* Decayed active data bytes received */
	Ulong		st_act_time;		/* Decayed active time elapsed */
	Ulong		st_act_count;		/* Decays */
	Ulong		st_datarate;		/* Current active data byte rate */
	int		st_pid;
	long		st_pktdatasize;		/* Copy of OutDataSize / Recent InDataSize */
	char		st_version[8];		/* CHANVERSION */
	Uint		st_flags;		/* CHDMNSTATS | CHPROSTATS etc. */
#	if	VCDAEMON_STATS == 1
	DmnStat		st_dmnstats[DS_NSTATS];
#	endif
#	if	PROTO_STATS == 1
	PktStat		st_pktstats[PS_NSTATS];
#	endif
}
			VCstatus;

Extern VCstatus		Status;			/* After fork(), there is one for each direction */

#define	MIN_STATUS_SIZE(status)	((int)(((unsigned long)&status.st_flags) - ((unsigned long)&status)) + (unsigned long)(sizeof(status.st_flags)))

Extern int		StatusFd;		/* Set after fork by InitStatus() */
Extern char *		Status_File;		/* Name of status file for reader/writer */
Extern Time_t		StatusTime;		/* Time when status file should be updated */
extern int		UpdateTime;		/* Status files update rate (seconds) */

#define	ActiveBytes	Status.st_act_bytes
#define	ActiveRawBytes	Status.st_act_rawbytes
#define	ActiveTime	Status.st_act_time
#define	CTime		Status.st_now
#define	Channels	Status.st_channels
#define	DecayCount	Status.st_act_count
#define	PacketTime	Status.st_lastpkt
#define	StartTime	Status.st_start

#define	CHDMNSTATS	0001			/* Bit set in st_flags if st_dmnstats present */
#define	CHPROSTATS	0002			/* Bit set in st_flags if st_pktstats present */
#define	CHCOOKEDVC	0004			/* Bit set in st_flags if VC cooked */
#define	CHDATACRC16	0010			/* Bit set in st_flags if packet data CRC16 used */
#define	CHLINKDOWN	0020			/* Bit set in st_flags if link is down */
#define	CHLINKUP	0			/* Pseudo flag */
#define	CHLINKSTART	0040			/* Bit set in st_flags before link comes up */
#define	CHOUTONLY	0100			/* Bit set if outbound */
#define	CHINONLY	0200			/* Bit set if inbound */
#define	CHCOOKTYP	0x0f00			/* Bits for cooking protocol */
#define	CHDATACRC32	0x1000			/* Bit set in st_flags if packet data CRC32 used */
#define	CHTURNAROUND	0x2000			/* Bit set if in half-duplex mode */
#define	CHRMQERCVD	0x4000			/* Bit set for remote MQE received */
#define	CHLMQERCVD	0x8000			/* Bit set for local MQE received */

#define	INSTATUS	false
#define	OUTSTATUS	true

#define	ACT_MULT	100			/* Multiply all additions */
#define	STATUS_UPDATE	10			/* Default status files update rate */

/*
**	Routines.
*/

extern int	InitStatus _FA_((bool));
extern void	Write_Status _FA_((void));
