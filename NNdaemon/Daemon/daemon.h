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


#define		DOWNTIME	(5*60)	/* Wait this long before marking link down */
#define		Nstreams	NSTREAMS /* No longer a parameter */

#define		BatchMode	BatchTime

#include	"VCdaemon/cook.h"

/*
**	Global parameters for daemon functions
*/

Extern int	BatchTime;		/* Wait this long before stopping */
Extern long	BlockCount;		/* Writes BLOCKED */
Extern int	BufferOutput;		/* Buffer output into this sized writes */
Extern bool	Cook;			/* Cooked mode for 7-bit circuits */
Extern bool	DieOnDown;		/* Terminate on link down */
Extern int	Fstream;		/* First stream to use */
Extern bool	HalfDuplex;		/* True if daemon in half-duplex mode */
Extern int	HoldAcks;		/* Overrides protocol's ACK window */
Extern char *	HomeNode;		/* Our node name */
Extern int	IntraPktDelay;		/* Maximum intra-packet delay */
Extern char *	LinkNode;		/* The remote node's name */
Extern long	MaxRunTime;		/* Maximum duration of connection */
Extern long	MinSpeed;		/* Minimum speed prepared to accept */
Extern bool	NoAdjust;		/* Don't adjust protocol parameters */
Extern bool	NoUpDown;		/* Don't generate link UP/DOWN messages */
Extern int	PktZ;			/* Packet size on link */
extern int	UpdateRate;		/* Status file update frequency */
Extern bool	Verbose;		/* Report repeated control packets */
Extern ssize_t	(*WriteO) _FA_((int, const void *, size_t));	/* Function to do output */

extern char *	MessageHandler;		/* Name of message handler process */
extern char *	NewstateHandler;	/* Name of state change process */
extern char *	BadHandler;		/* Name of bad message handler process */

extern char *	FinishReason;		/* Reason to be printed on termination */

/*
**	Protocol parameters
*/

extern long	AllMessages;		/* Messages transferred since the epoch */
extern int	Nbufs;			/* Number of buffers in use */
extern char	Quality;		/* Maximum quality selected to transmit */
extern long	Speed;			/* Notional bytes/sec. for link */
extern bool	UseCrc;			/* True if CRC in use on link */
extern int	Wbufcount;		/* Current contents of output buffer */

/*
**	Directory and file names.
*/

extern char *	CmdsName;		/* Directory for outbound message command files */
Extern char *	CmdsRouteFile;		/* File name for routing inbound message command files */
Extern char *	CmdsTempFile;		/* File name for making inbound message command files */
Extern char *	LinkDir;		/* Directory in SPOOLDIR for daemon status files */
Extern char *	RouterDir;		/* Directory for inbound messages ready for routing */
extern char *	Statusfile;		/* File name for state info. */
extern char	Template[];		/* Last component of node-node command file name */
Extern char *	WorkDir;		/* Directory for inbound messages */

/*
**	State file update commands
*/

typedef enum
{
	up_date, up_force, up_finish, up_error, up_opening
}
		StCom;

extern void	Update _FA_((StCom));

/*
**	Daemon functions
*/

extern ssize_t	RCwrite _FA_((int, const void *, size_t)),
		Rwrite _FA_((int, const void *, size_t));

extern void	Close _FA_((int, char *));
extern int	Create _FA_((char *));
extern int	Open _FA_((char *));
extern int	OpenJ _FA_((int, char *));
extern int	Read _FA_((int, char *, int, char *));
extern int	ReadJ _FA_((int, int, char *, int, char *));
extern int	RWflush _FA_((int));
extern Ulong	Seek _FA_((int, Ulong, int, char *));
extern Ulong	SeekJ _FA_((int, int, Ulong, int, char *));
extern void	Unlink _FA_((char *));
extern int	Write _FA_((int, char *, int, char *));
extern int	WriteJ _FA_((int, int, char *, int, char *));
extern void	Write_Status _FA_((void));

extern void	PassState _FA_((int));

extern int	strrcmp _FA_((char *, char *));

/*
**	Parameters for VCdaemon statusfile compatibility.
*/

#define	STINCHAN	0
#define	STOUTCHAN	1

#define STLINKDOWN	0
#define STLINKUP	1

typedef enum
{
	chn_idle, chn_starting, chn_active, chn_ending, chn_ended
}
		ChState;

#define	CHN_ACTIVE	chn_active		/* Message being transmitted */
#define	CHN_ENDED	chn_ended		/* Message terminated */
#define	CHN_ENDING	chn_ending		/* Message terminating */
#define	CHN_IDLE	chn_idle		/* Cold start only */
#define	CHN_STARTING	chn_starting		/* Message starting */

extern int	NNInitStatus _FA_((int));
extern void	StID _FA_((int, int, char *, long, long, Time_t));
extern void	StIncChan _FA_((int, int, int));
extern void	StIncMsg _FA_((int, int));
extern void	StLinkState _FA_((int));
extern void	StChState _FA_((int, int, ChState));
extern void	StRateTime _FA_((Ulong, Ulong));
extern void	StVCdata _FA_((int, int));
extern void	StVCnak _FA_((int));
extern void	StVCpkt _FA_((int));
extern void	StVCskip _FA_((int));
