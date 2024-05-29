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


#include	"cook.h"

/*
**	Parameters.
*/

#define	IDLE_TIMEOUT		40			/* Default period between IDLE packets */
#define	DOWN_TIMEOUT		60			/* Retry time for anything if link down */
#define	SOM_TIMEOUT		20			/* Retry time for a SOM control */
#define	MIN_SOM_TIMEOUT		5			/* Retry time for a SOM control if only 1 channel */
#define	EOM_TIMEOUT		15			/* Retry time for an EOM control */
#define	MIN_EOM_TIMEOUT		8			/* Retry time for an EOM control if only 1 channel */
#define	MAX_EOMS		20			/* More than this is problem */
#define	MIN_TIMEOUT		(3*SOM_TIMEOUT)		/* Minimum reader VC timeout */
#define	NAK_TIMEOUT		60			/* Retry time for outstanding NAKS */

#ifndef	FILEBUFFERSIZE
#define	FILEBUFFERSIZE		1024			/* Buffer file system I/O */
#endif
#define	FILEBUFFERMASK		(FILEBUFFERSIZE-1)	/* FILEBUFFERSIZE must be a power of 2 */

#define	INTLDATAPKTS		8			/* Default starting window size */
#define	MAXREAD0		10			/* Max. consecutive 0 reads from VC */
#define	MSGIDLENGTH		UNIQUE_NAME_LENGTH
#define	SETMAXCOOKSIZE(S)	S*=2; S+=S/78+2		/* Size of buffer needed for cooking */

#define	LINK_DOWN_TIME		(3*Idle_Timeout+10)	/* How long to wait before marking link down */
#define	SLEEP_TIME		60			/* How long to pause if nothing to do */
#define	BATCH_SLEEP_TIME	10			/* How long to pause if nothing to do and in batch mode */
#define	MAX_SYNC_TIME		60			/* How long to wait before giving up sync attempt */

#define	SENDLOCAL		true			/* SendMQE 2nd parameter */
#define	SENDREMOTE		false

#define	CLOSE_SYNC		true			/* 2nd arg for Close() */
#define	CLOSE_NOSYNC		false

#define	FREESCANRATE		60			/* Max period between FSFree calls */

/*
**	Finish reasons.
*/

typedef struct
{
	char *	reason;
	int	code;
}
		ExCode;

extern ExCode	FinChilderror;
extern ExCode	FinLowSpace;
extern ExCode	FinHangup;
extern ExCode	FinMaxrun;
extern ExCode	FinMismatch;
extern ExCode	FinNegtime;
extern ExCode	FinReaderror;
extern ExCode	FinRdtimeout;
extern ExCode	FinRemSlow;
extern ExCode	FinRemsync;
extern ExCode	FinRemterm;
extern ExCode	FinSigpipe;
extern ExCode	FinSync;
extern ExCode	FinUnxsignal;
extern ExCode	FinVCEcho;
extern ExCode	FinWrtrBlocked;
extern ExCode	FinWriterror;
extern ExCode	Finished;

/*
**	Writer signalling.
*/

#ifndef	SIGWRITER
#if	BSD4 >= 2 || defined(signal)
#define	SIGWRITER	SIGWAKEUP
#else	/* BSD4 >= 2 || defined(signal) */
#define	SIGWRITER	SIGFPE
#endif	/* BSD4 >= 2 || defined(signal) */
#endif

Extern Time_t	LastSignalTime;		/* Time last writer signal from reader */
Extern bool	NeedSignal;		/* Reader needs to send a signal to writer */

/*
**	Local peculiarities.
*/

#if	XTI == 1
Extern bool	xti_connect;
extern int	t_sync();
extern	int	t_rcv();
extern	int	t_snd();
extern	int	t_errno;
#endif	/* XTI == 1 */

/*
**	Common data.
*/

Extern char *	BadFile;		/* File name template for dumping bad commandsfiles */
Extern char *	BadHandler;		/* Name of bad commandsfile handler */
Extern Ulong	BatchTime;		/* Time to run after nothing more to send */
Extern Time_t	CheckFree;		/* Time to check free space */
extern char	CmdsName[];		/* Commands directory */
Extern char *	CmdsRouteFile;		/* File name template for routing message commands */
Extern char *	CmdsTempFile;		/* File name template for creating message commands */
Extern int	DataChannels;		/* Channels with active messages */
Extern Time_t	DecayTime;		/* Next time to decay */
Extern bool	DieOnDown;		/* Terminate if link goes down */
Extern int	Direction;		/* Sets bit in out-bound packets, checks in-bound for echo */
Extern bool	Finish;			/* Wind up */
Extern ExCode *	FinishReason;		/* Explanation structure */
Extern char *	HomeAddress;		/* Address (real name) of us */
Extern int	HomeAdLength;		/* Length of HomeAddress */
Extern Time_t	IDLETime;		/* Time last IDLE packet received */
extern int	Idle_Timeout;		/* IDLE timeout for dead circuit detection */
Extern int	IdleTime;		/* Seconds since we sent anything */
Extern int	InitDataCount;		/* `Send ahead' count */
Extern bool	InOnly;			/* No messages out */
Extern bool	Jump;			/* VCerrorJmp if set */
Extern char *	LinkAddress;		/* Address (real name) of link */
Extern int	LinkAdLength;		/* Length of LinkAddress */
Extern Time_t	LinkDownTime;		/* Link down if no IDLE packet seen */
Extern char *	LinkName;		/* Name of link directory to work in */
Extern bool	LMQEreceived;		/* Indicate local idle */
Extern Time_t	LowSpaceTime;		/* Next time to let writer know of reader problem */
Extern Ulong	MaxRunTime;		/* Stop after this time */
Extern long	MinSpeed;		/* Terminate if throughput less bytes/sec. */
extern char	MsgsInName[];		/* Receiving messages */
extern char *	Name;			/* Name of program */
Extern bool	NoAdjust;		/* Don't send throughput data in IDLE packets */
Extern bool	NoUpDown;		/* Don't generate link UP/DOWN messages */
Extern long	NomSpeed;		/* Nominal bytes/sec. for circuit */
Extern bool	OldDaemon;		/* Version 1.0 daemon at other end */
extern int	OtherPid;		/* Process ID of other daemon process */
extern long	OutDataSize;		/* Size of output data packet data */
Extern bool	OutOnly;		/* No messages in */
extern int	Parallel;		/* Parallel daemon number */
extern int	Pid;			/* Process ID of this daemon process */
Extern int	PinFd;			/* File descriptor of in-bound pipe */
extern char	PinName[];		/* Name of pipe for in-bound packets */
Extern vFuncp *	PktFuncsp;		/* Array of routines to process packet types */
Extern int	PoutFd;			/* File descriptor of out-bound pipe */
extern char	PoutName[];		/* Name of pipe for out-bound packets */
extern char	Quality;		/* Maximum quality selected to transmit */
extern char	RepFmt[];		/* Format for message i/o reports */
Extern bool	RMQEreceived;		/* Indicate remote idle */
Extern char *	RouterDir;		/* Routing directory for routing messages */
Extern Time_t	SendIDLE;		/* Time to send next IDLE */
extern char	Slash[];		/* Directory aid */
Extern int	SleepTime;		/* Intervals between writer idle scans */
Extern char *	SLinkAddress;		/* Stripped address of link */
Extern bool	Started;		/* Indicate comms. in sync. */
extern char	Template[];		/* UniqueName component */
extern Time_t	Time;			/* Current time for Lib routines */
Extern bool	TurnAround;		/* 1/2 duplex operation */
Extern int	TurnDelay;		/* Gap between TURN packets */
Extern Time_t	TurnTime;		/* Send TURN (CTRL) packet after this time */
extern bool	UseDataCRC;		/* True if CRC on data */
Extern int	VCfd;			/* Descriptor for accessing virtual circuit */
Extern char *	VCname;			/* Name of virtual circuit */
Extern bool	Verbose;		/* Report repeated control packets */
Extern Funcp	WriteFuncp;		/* Routine to write packets to a file */

extern vFuncp	ReadReceivers[];	/* Array of reader routines for processing packets */
extern vFuncp	WriteReceivers[];	/* Array of writer routines for processing packets */

/*
**	Routines.
*/

extern void	Close _FA_((int, bool, char *));
extern int	Create _FA_((char *));
extern void	DecayActive _FA_((void));
extern void	IgnoreSignals _FA_((void));
extern void	LowSpace _FA_((int));
extern void	NewState _FA_((int));
extern int	Open _FA_((char *));
extern int	OpenJ _FA_((int, char *));
extern void	PacketReader _FA_((Funcp, int, char *));
extern int	Read _FA_((int, char *, int, char *));
extern int	ReadJ _FA_((int, int, char *, int, char *));
extern Ulong	Seek _FA_((int, Ulong, int, char *));
extern Ulong	SeekJ _FA_((int, int, Ulong, int, char *));
extern void	SendEOT _FA_((int));
extern int	SendMQE _FA_((int, bool));
extern void	SigCatch _FA_((int));
extern char *	StrStatus _FA_((void));
extern int	Uncook _FA_((char *, int));
extern void	Unlink _FA_((char *));
extern int	Write _FA_((int, char *, int, char *));
extern bool	ZeroLength _FA_((char *));
