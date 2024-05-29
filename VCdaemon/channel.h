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
**	Half-duplex channel state.
*/

typedef enum
{
	ch_idle, ch_starting, ch_active, ch_ending, ch_ended
}
		CHstate;

#define	CH_ACTIVE	ch_active		/* Message being transmitted */
#define	CH_ENDED	ch_ended		/* Message terminated */
#define	CH_ENDING	ch_ending		/* Message terminating */
#define	CH_IDLE		ch_idle			/* Cold start only */
#define	CH_STARTING	ch_starting		/* Message starting */

/*
**	Activity flags.
*/

enum
{
	ch_msgfile, ch_rdup, ch_eof, ch_ignbuf, ch_warned, ch_lowspace
};

typedef Uint	CHflags;

#define	CH_EOF		(1<<(int)ch_eof)	/* Writer message-part file at EOF */
#define	CH_IGNBUF	(1<<(int)ch_ignbuf)	/* Ignore buffer and do direct i/o */
#define	CH_LOWSPACE	(1<<(int)ch_lowspace)	/* SOM refused -- low space */
#define	CH_MSGFILE	(1<<(int)ch_msgfile)	/* Message file open */
#define	CH_RDUP		(1<<(int)ch_rdup)	/* Inbound message may be duplicate */
#define	CH_WARNED	(1<<(int)ch_warned)	/* Don't be boring */

/*
**	Receive message data gap management.
*/

typedef struct Gap	Gap;

struct Gap
{
	Gap *	next_gap;
	Ulong	start_gap;	/* Address in message */
	Ulong	end_gap;	/* Address in message */
	Time_t	time_gap;	/* NAK sent time */
	int	send_gap;	/* NAK count */
};

/*
**	Transmit message file management.
*/

typedef struct Part	Part;

struct Part
{
	Part *	next_part;
	char *	filename_part;		/* Name of file */
	Ulong	filestart_part;		/* Start of message data in file */
	Ulong	msgstart_part;		/* Address in message of first data in file */
	Ulong	msgend_part;		/* Address in message of last data in file */
	Ulong	fileaddr_part;		/* Current address in file */
	bool	unlink_part;		/* True if file should be unlinked */
};

/*
**	Disjoint read/write structures.
*/

typedef struct
{
	Gap *	rc_gaplist;		/* Gaps in read data */
	int	rc_eomcount;		/* EOMs received for this message */
	char *	rc_msgfilename;		/* Inbound message filename (last componenent == msgid) */
	Time_t	rc_cntrltime;		/* Time of last NAK checkpoint */
	Ulong	rc_goodaddr;		/* Message intact up to this address */
	RCHstat	rc_stats;		/* Statistics */
}
		RChan;

typedef struct
{
	Part *	wc_partlist;		/* Files making up message */
	Part *	wc_current;		/* Current Part */
	char *	wc_cmdfilename;		/* Commandsfile describing message (last componenent == msgid) */
	Time_t	wc_cntrltime;		/* Time of next EOM/SOM */
	int	wc_count;		/* Window count */
	WCHstat	wc_stats;		/* Statistics */
}
		WChan;

typedef union
{
	RChan	ch_r;
	WChan	ch_w;
}
		RWChan;

/*
**	Status structure for uni-directional channel.
*/

typedef struct
{
	CHstate	ch_state;
	CHflags	ch_flags;
	int	ch_number;

	RWChan	ch_rw;			/* Read/Write specific data */

	Ulong	ch_msgaddr;		/* Current postion in message */
	int	ch_msgfd;
	char	ch_msgid[MSGIDLENGTH+1];
	char *	ch_msgidp;		/* Pointer to last component of cmd/msg filename */
	Ulong	ch_msgsize;		/* Total size of message */
	Time_t	ch_msgtime;		/* Queue time of message */

	char *	ch_databuffer;
	Ulong	ch_bufstart;		/* Current address of databuffer */
	Ulong	ch_bufend;		/* End address in databuffer */
}
		Chan;

#define	CHANVERSION	"1.4"

#define	ch_chkpttime	ch_rw.ch_r.rc_cntrltime
#define	ch_cmdfilename	ch_rw.ch_w.wc_cmdfilename
#define	ch_cntrltime	ch_rw.ch_w.wc_cntrltime
#define	ch_count	ch_rw.ch_w.wc_count
#define	ch_current	ch_rw.ch_w.wc_current
#define	ch_eomcount	ch_rw.ch_r.rc_eomcount
#define	ch_gaplist	ch_rw.ch_r.rc_gaplist
#define	ch_goodaddr	ch_rw.ch_r.rc_goodaddr
#define	ch_msgfilename	ch_rw.ch_r.rc_msgfilename
#define	ch_partlist	ch_rw.ch_w.wc_partlist
#define	ch_statsin(S)	ch_rw.ch_r.rc_stats[(int)S]
#define	ch_statsout(S)	ch_rw.ch_w.wc_stats[(int)S]

/*
**	Routines.
*/

extern void	BadMesg _FA_((Chan *, char *));
extern int	CheckInPktFile _FA_((Chan *));
extern int	CheckOutPktFile _FA_((Chan *));
extern char *	ChnStateStr _FA_((Chan *));
extern bool	FillGap _FA_((Chan *, Ulong, Ulong));
extern bool	FindMessage _FA_((Chan *));
extern void	FlushGaps _FA_((Chan *));
extern void	FlushRData _FA_((Chan *, Ulong));
extern void	FlushWData _FA_((Chan *));
extern void	FreeParts _FA_((Part **));
extern int	NAKGaps _FA_((Chan *, Ulong));
extern void	QueueGap _FA_((Chan *, Ulong));
extern int	ReadData _FA_((Chan *, char *, int));
extern void	RecvMessage _FA_((Chan *));
extern bool	SendData _FA_((Chan *));
extern void	Send_EOM _FA_((Chan *));
extern void	SendEOMA _FA_((Chan *));
extern void	Send_SOM _FA_((Chan *));
extern Ulong	SendNAK _FA_((Chan *, int, Ulong, Ulong));
extern bool	SetupParts _FA_((Chan *));
extern int	StartMessage _FA_((Chan *, bool));
extern void	UnlinkParts _FA_((Chan *, bool));
extern int	WriteChan _FA_((Chan *));
extern void	WriteData _FA_((Chan *, char *, int, Ulong));
