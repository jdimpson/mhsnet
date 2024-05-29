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
**	Message streams
*/

#define	NSTREAMS	3		/* Number of simultaneous streams in each direction */
#define	STREAMIDZ  UNIQUE_NAME_LENGTH	/* Maximum length of a stream identifier */

/*
**	Message transmission descriptor
*/

typedef struct DQel
{
	struct DQel *	ql_next;	/* Next descriptor in queue */
	char *	ql_filename;		/* Filename for data */
	long	ql_base;		/* Start address in file */
	long	ql_range;		/* Length of data from "start" */
}
			DQel;

typedef DQel *		DQl_p;
Extern DQl_p		DQfreelist;	/* Free list of descriptors */

typedef struct
{
	long	dq_count;		/* Count of data left in current descriptor */
	DQl_p	dq_first;		/* First descriptor in queue */
	Time_t	dq_time;		/* Time of last control message */
	char *	dq_cmdf;		/* The message's command file */
	char *	dq_cmdfp;		/* Start of id in command file name */
	char	dq_id[STREAMIDZ+1];	/* Last component of dq_cmdf for recovery */
}
			DQhead;

/*
**	Message reception data
*/

typedef struct
{
	char *	rh_fn;			/* Work file name */
	char	rh_id[STREAMIDZ+1];	/* Last component of rh_fn for recovery */
}
			Rhead;

/*
**	Message stream control data, receive/transmit
*/

typedef union
{
	Rhead	dt_rh;			/* Filenames for receive */
	DQhead	dt_dqh;			/*  or transmission descriptor queue */
}
			Data;

typedef struct
{
	Data	str_data;		/* Pointer to data for files */
	long	str_size;		/* The message's size */
	long	str_posn;		/* Current position in message */
	Ulong	str_bytes;		/* Bytes transferred */
	Ulong	str_messages;		/* Messages transferred */
	Time_t	str_time;		/* Time at start of message */
	short	str_fd;			/* Unix file descriptor for current data file */
	char	str_state;		/* State of stream */
	char	str_flags;		/* Flags for stream */
	char	str_id[STREAMIDZ+1];	/* The message's "id" */
}
			Stream;

typedef Stream *	Str_p;

#define	str_recv	str_data.dt_rh
#define	str_fname	str_recv.rh_fn
#define	str_descr	str_data.dt_dqh
#define	str_count	str_descr.dq_count
#define	str_sent	str_descr.dq_time
#define	str_cmdf	str_descr.dq_cmdf
#define	str_cmdfp	str_descr.dq_cmdfp
#define	str_cmdid	str_descr.dq_id

/*
**	Stream states
*/

enum
{
	str_idle, str_wait, str_start, str_active,
	str_ending, str_ended, str_empty,
	str_error, str_inactive, str_again
};

#define	STR_IDLE	(int)str_idle	/* Stream is inactive */
#define	STR_WAIT	(int)str_wait	/* Stream has message awaiting reception */
#define	STR_START	(int)str_start	/* Stream has message awaiting transmission */
#define	STR_ACTIVE	(int)str_active	/* Stream has message in transmission */
#define	STR_ENDING	(int)str_ending	/* Stream has message terminating */
#define	STR_ENDED	(int)str_ended	/* Stream has completed message handling */
#define	STR_EMPTY	(int)str_empty	/* Stream has no messages waiting */
#define	STR_ERROR	(int)str_error	/* Stream has an error condition */
#define	STR_INACTIVE	(int)str_inactive /* Stream is unused */
#define	STR_AGAIN	(int)str_again	/* Message may have been received twice */

/*
**	Stream flags.
*/

enum {
	str_dup,			/* This message may be a duplicate */
	str_warned			/* This message has incurred a warning */
};

#define	STR_DUP		(1<<(int)str_dup)
#define	STR_WARNED	(1<<(int)str_warned)

/*
**	State structure.
*/

typedef struct
{
	char	linkstate;		/* State of link */
	char	procstate;		/* State of daemon */
	short	procid;			/* Process ID */
	long	speed;			/* Minimum effective bytes/second of link */
	long	maxspeed;		/* Maximum effective bytes/second of link */
	short	packetsize;		/* Current packet data size */
	short	nbufs;			/* Current window size */
	short	recvtimo;		/* Current receive timeout */
	Ulong	inbytes;		/* Bytes received recently */
	Ulong	outbytes;		/* Bytes transmitted recently */
	Time_t	lasttime;		/* Beginning of "recently" */
	Ulong	allbytes;		/* Total bytes transferred */
	Ulong	allmessages;		/* Total messages transferred */
	Time_t	starttime;		/* Daemon start time */
	Time_t	thistime;		/* Status file write time */
	Ulong	activetime;		/* Total time while active */
	Ulong	inpkts;			/* Total packets received */
	Ulong	outpkts;		/* Total packets transmitted */
	Stream	streams[2][NSTREAMS];	/* I/O status */
	char	version[8];		/* SCCSID */
}
			NN_state;

Extern NN_state		NNstate;

#define	INSTREAMS	0
#define	OUTSTREAMS	1

#define	inStreams	NNstate.streams[INSTREAMS]
#define	outStreams	NNstate.streams[OUTSTREAMS]

#define	inByteCount	NNstate.inbytes
#define	outByteCount	NNstate.outbytes

#define	StreamSCCSID	"1.5"

/*
**	Link states
*/

enum {	link_down, link_up };

#define	DLINK_DOWN	(int)link_down
#define	DLINK_UP	(int)link_up

/*
**	Daemon states
*/

enum {	proc_idle, proc_running, proc_error, proc_opening, proc_calling };

#define	PROC_IDLE	(int)proc_idle
#define	PROC_RUNNING	(int)proc_running
#define	PROC_ERROR	(int)proc_error
#define	PROC_OPENING	(int)proc_opening
#define	PROC_CALLING	(int)proc_calling
