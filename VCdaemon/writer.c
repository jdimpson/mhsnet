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
**	Writer process.
*/

#define	SIGNALS
#define	TIME
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"driver.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"pktstats.h"
#include	"status.h"
#include	"packet.h"
#include	"handlers.h"
#include	"spool.h"

#include	<setjmp.h>


jmp_buf		WakeJmp;

extern jmp_buf	IOerrorJmp;
extern jmp_buf	VCerrorJmp;

static Time_t	MQETime;		/* Send MQE packet after this time */
static int	MQEDelay;		/* Gap between MQE packets */
static bool	PktFileIn;
static int	SyncTime;		/* Time spent syncing... */
static bool	WakeJmped;		/* longjmp'ed from JmpWakeCatch() */
static bool	Woken;			/* True if SIGWAKEUP */

#ifdef	SIG_SETMASK
static sigset_t newset;
static sigset_t oldset;
#endif	/* SIG_SETMASK */

int		CookWrite _FA_((int, char *, int, char *));
int		PktFileReader _FA_((int, char *, int, char *));
int		VCWrite _FA_((int, char *, int, char *));
extern SigFunc	WakeupCatch;
extern SigFunc	JmpWakeCatch;

static int	CheckPipes _FA_((void));
static int	DoChans _FA_((int *));
static bool	DoTime _FA_((int, int, int));
#if	DEBUG >= 2
char *		StrStatus _FA_((void));
#endif	/* DEBUG >= 2 */



void
writer(inpipefd, outpipefd)
	int		inpipefd;
	int		outpipefd;
{
	register int	n;
	register int	done;
	register int	last_done;
	int		empty;

	Trace3(1, "writer(%d, %d)", inpipefd, outpipefd);

	(void)close(0);
	VCfd = 1;				/* For writing packets to VC */
	VCname = "write virtual circuit";
	PinFd = inpipefd;			/* For reading in-bound packets from reader */
	PoutFd = outpipefd;			/* For    "    out-bound   "      "     "   */

	BadFile = concat(SPOOLDIR, BADDIR, Template, NULLSTR);
	BadHandler = concat(SPOOLDIR, LIBDIR, BADHANDLER, NULLSTR);

	if ( (MQETime = BatchTime) > 0 )
		n = BATCH_SLEEP_TIME;
	else
		n = SLEEP_TIME;
	MQETime += Time + MAX_SYNC_TIME;	/* Don't send before started */
	if ( TurnAround )
	{
		TurnTime = Time;	/* ASAP */
		TurnDelay = n = BATCH_SLEEP_TIME;
		MQEDelay = SLEEP_TIME;
	}
	else
	if ( Idle_Timeout == 0 )
		MQEDelay = DAY;
	else
		MQEDelay = n;

	if ( SleepTime == 0 )
		SleepTime = n;
	if ( SleepTime < MINSLEEP )
		SleepTime = MINSLEEP;

	SyncTime = 0;

	PktFuncsp = WriteReceivers;

	if ( CookVC != (CookT *)0 )
		WriteFuncp = CookWrite;
	else
		WriteFuncp = VCWrite;

#	ifdef	SIG_SETMASK
	sigemptyset(&newset);
#	endif	/* SIG_SETMASK */

	/*
	**	Read and initialise Status, and restart any active messages.
	*/

	(void)InitStatus(OUTSTATUS);
	last_done = 1;

	/*
	**	VC error recovery.
	*/

	if ( (errno = setjmp(VCerrorJmp)) )
	{
		(void)SysWarn("VC write error");

		if ( Finish )
			return;

		/*
		**	Write error on virtual circuit.
		*/

		FinishReason = &FinWriterror;
		Error(FinWriterror.reason);
		return;
	}

	/*
	**	File I/O error recovery.
	*/

	if ( (n = setjmp(IOerrorJmp)) )
		BadMesg(&Channels[--n], "message truncated?");

	/*
	**	SYNC up.
	*/

	while ( !Started )
	{
		if ( SyncTime > MAX_SYNC_TIME && BatchTime > 0 )
		{
			FinishReason = &FinRemsync;
			return;
		}

		n = SendMQE(SYNC, SENDREMOTE);

		if ( NomSpeed > 0 )
		{	/* Allow for echoing remote SYNC */
			if ( (n = (n + n + NomSpeed - 1) / NomSpeed) < MINSLEEP )
				n = MINSLEEP;
		}
		else
			n = MINSLEEP;
		n -= sleep(n);
		SyncTime += n;

		(void)CheckPipes();

		if ( DoTime(0, 0, 0) )
			return;
	}

	MQETime = Time + BatchTime;

	/*
	**	Scan for something to do.
	*/

	while ( !Finish )
	{
		done = 0;
		done += CheckPipes();
		done += DoChans(&empty);

		if ( done == 0 && (done = CheckPipes()) == 0 )
		{
			DODEBUG(char tbuf[DATETIMESTRLEN]);

			if ( last_done || empty < NCHANNELS )
				last_done = MINSLEEP;
			else
				last_done = SleepTime;

			if ( !Woken )
			{
				WakeJmped = false;

				if ( !setjmp(WakeJmp) )
				{
					(void)signal(SIGWAKEUP, JmpWakeCatch);
#					if	SIGWRITER != SIGWAKEUP
					(void)signal(SIGWRITER, JmpWakeCatch);
#					endif	/* SIGWRITER != SIGWAKEUP */

#					ifdef	SIG_SETMASK
					(void)sigprocmask(SIG_SETMASK, &newset, &oldset);
#					endif	/* SIG_SETMASK */

					if ( !Woken )
					{
					/*	Trace3(
					**		2,
					** this		"%s writer sleep %d",
					** gets		DateTimeStr(time((time_t *)0),
					** interrupted!			tbuf),
					**		last_done
					**	);
					*/
						(void)sleep(last_done);
					}

					/** Always ignore **/

					(void)signal(SIGWAKEUP, SIG_IGN);
#					if	SIGWRITER != SIGWAKEUP
					(void)signal(SIGWRITER, SIG_IGN);
#					endif	/* SIGWRITER != SIGWAKEUP */
				}

				Trace2(2, "%s writer wakeup",
						DateTimeStr(time((time_t *)0), tbuf));
			}
		}
		else
		{
			last_done = 0;
			IdleTime = 0;
		}

		if ( DoTime(done, empty, last_done) )
			return;

		last_done = done;
	}
}



/*
**	Read any data in pipes from reader.
*/

static int
CheckPipes()
{
	Trace1(3, "CheckPipes");

	if ( Finish )
		return 1;

	if ( CanReadPipe(PoutFd, PoutName) )
	{
		register int	n;

		if ( (n = Read(PoutFd, (char *)PktWriteData, PktDatSize, PoutName)) == 0 )
			Error("Reader pipe EOF");

		TracePkt(dir_pipout, PktWriteData, EmptyString, true);
		TracePkt(dir_vcout, PktWriteData, EmptyString, true);

		(void)(*WriteFuncp)(VCfd, (char *)PktWriteData, n, VCname);

		PacketTime = Time;
	}

	if ( (PktFileIn = CanReadPipe(PinFd, PinName)) )
	{
		PacketReader(PktFileReader, PinFd, PinName);
		return 1;
	}

	return 0;
}



/*
**	Cook output to virtual circuit.
*/

int
CookWrite(fd, data, size, name)
	int		fd;
	char *		data;
	register int	size;
	char *		name;
{
	register int	maxsize;
	static char *	buffer;
	static int	bufsize;

	if ( size < PktBufSize )
		maxsize = PktBufSize;
	else
		maxsize = size;

	SETMAXCOOKSIZE(maxsize);

	if ( bufsize < maxsize )
	{
		bufsize = maxsize;

		if ( buffer != NULLSTR )
			free(buffer);

		buffer = Malloc(maxsize);
	}

	size = (*CookVC->cook)(data, size, buffer);

	return VCWrite(fd, buffer, size, name);
}



/*
**	Scan channels for something to do.
*/

static int
DoChans(emptyp)
	int *		emptyp;
{
	register Chan *	chnp;
	register int	n;
	register int	done = 0;
	register int	i;
	register char	q = Quality;
	register int	empty = 0;	/* Work-around "(*emptyp)++" for SPARC */
	bool		ign_sigs;

	Trace2(2, "DoChans(%d)", *emptyp);

	Woken = false;

	if ( Finish )
	{
		*emptyp = NCHANNELS;
		return 1;
	}

	ign_sigs = true;

	(void)signal(SIGWAKEUP, WakeupCatch);
#	if	SIGWRITER != SIGWAKEUP
	(void)signal(SIGWRITER, WakeupCatch);
#	endif	/* SIGWRITER != SIGWAKEUP */

#	ifdef	SIG_SETMASK
	(void)sigprocmask(SIG_SETMASK, &newset, &oldset);
#	endif	/* SIG_SETMASK */

	for ( chnp = Channels, n = NCHANNELS ; --n >= 0 ; chnp++ )
	{
		if ( done && chnp->ch_msgid[0] > q && chnp->ch_state != CH_ENDING )
			continue;	/* Concentrate on higher priority messages */

		if ( chnp->ch_state == CH_IDLE || chnp->ch_state == CH_ENDED )
		{
			if ( !FindMessage(chnp) )
			{
				empty++;
				continue;
			}

			if ( InOnly )
			{
				(void)StartMessage(chnp, false);
				continue;
			}

			if ( ign_sigs )
			{
				(void)signal(SIGWAKEUP, SIG_IGN);
#				if	SIGWRITER != SIGWAKEUP
				(void)signal(SIGWRITER, SIG_IGN);
#				endif	/* SIGWRITER != SIGWAKEUP */
				ign_sigs = false;
			}

			if ( (i = StartMessage(chnp, true)) > 0 )
			{
				done += i;
				q = chnp->ch_msgid[0];
			}
		}
		else
		if ( chnp->ch_msgid[0] <= q || chnp->ch_state == CH_ENDING )
		{
			if ( InOnly )
				continue;

			if ( ign_sigs )
			{
				(void)signal(SIGWAKEUP, SIG_IGN);
#				if	SIGWRITER != SIGWAKEUP
				(void)signal(SIGWRITER, SIG_IGN);
#				endif	/* SIGWRITER != SIGWAKEUP */
				ign_sigs = false;
			}

			if ( (i = WriteChan(chnp)) > 0 )
			{
				done += i;
				q = chnp->ch_msgid[0];
			}
		}
		else
			empty++;
	}

	if ( ign_sigs )
	{
		(void)signal(SIGWAKEUP, SIG_IGN);
#		if	SIGWRITER != SIGWAKEUP
		(void)signal(SIGWRITER, SIG_IGN);
#		endif	/* SIGWRITER != SIGWAKEUP */
	}

	if ( Woken )
		done = 1;

	*emptyp = empty;
	return done;
}



/*
**	Progress time and time-dependant variables.
*/

static bool
DoTime(done, empty, sleep)
	int		done;
	int		empty;
	int		sleep;
{
	register Ulong	u;

	if ( (Time = time((time_t *)0)) < CTime )		/* -ve time increment? */
	{
		if ( (u = CTime - Time) > NVETIMECHANGE )
		{
			FinishReason = &FinNegtime;
			Error(FinNegtime.reason);
			return true;
		}
		DecayTime -= u;
		if ( sleep )
			IdleTime += sleep;
		CTime = Time;
	}

	Trace5(2, "DoTime(%d, %d, %d) ==> %lu", done, empty, sleep, (PUlong)Time);

	if ( Finish )
		return true;

	if ( MaxRunTime && MaxRunTime <= Time )
	{
		FinishReason = &FinMaxrun;
		return true;
	}

	if ( empty == NCHANNELS )
	{
		/*
		**	All queues empty,
		**	``OutOnly'' set false by returning MQE/CTRL in RecvLMQE(),
		**	(which is used as `ACK received' flag).
		*/

		DataChannels = 0;

		if ( MQETime <= Time )
		{
			MQETime = Time + MQEDelay;
			(void)SendMQE(MQE, SENDREMOTE);
			if ( TurnAround )
			{
				TurnTime = Time + TurnDelay;
				InOnly = true;
				Status.st_flags |= CHINONLY;
				Trace1(1, StrStatus());
			}
		}
		else
		if ( TurnAround && OutOnly && TurnTime <= Time )
		{
			(void)SendMQE(CTRL, SENDREMOTE);
			TurnTime = Time + TurnDelay;
			InOnly = true;			/* Prevent further output */
			Status.st_flags |= CHINONLY;
			Trace1(1, StrStatus());
		}
	}
	else
	if ( done > 0 && DataChannels > 0 )
	{
		MQETime = Time + BatchTime;
		TurnTime = Time;
	}

	if ( (u = Time - CTime) > 0 )
	{
		if ( DataChannels != 0 )
		{
			ActiveTime += u * ACT_MULT;

			if ( MinSpeed > 0 && (ActiveRawBytes/ActiveTime) < MinSpeed )
			{
				FinishReason = &FinRemSlow;
				return true;
			}

			if ( Time >= DecayTime )
				DecayActive();
		}
		else
			DecayTime += u;

		if ( sleep )
			IdleTime += u;

		CTime = Time;
	}

	if ( StatusTime <= Time )
		Write_Status();

	return false;
}



/*
**	Read packets from reader.
*/

int
PktFileReader(fd, addr, size, name)
	int		fd;
	char *		addr;
	register int	size;
	char *		name;
{
	Trace3(3, "PktFileReader size %d PktFileIn %d.", size, (int)PktFileIn);

	if ( !PktFileIn )
		return 0;

	PktFileIn = false;

	return Read(fd, addr, size, name);
}


#if	XTI == 1
/*
**	Write to XTI file descriptor.
**	Writing to an XTI circuit requires the addition of a leading status byte
*/

static int
xti_write(fd, data, size)
	int		fd;
	char *		data;
	int		size;
{
	char		* buf;
	register int	n;

	/* This isn't a terribly efficient way of doing this - the malloc in
	** particular generates huge overheads - but the alternative isn't
	** much better. (The alternative is to generate a buffer on the
	** stack, but we can't rely on any particular maximum size.)
	*/

	Trace4(8, "xti_write(%d, %x, %d)", fd, data, size);
	buf = Malloc(size + 1);
	bcopy(data, buf+1, size);
	buf[0] = 0;		/* Assume default status of 0. */

	n = t_snd(fd, buf, size+1, 0);
				/* Send data but don't return until we... */

	free(buf);		/* ... free up the buffer. */
	return n>1 ? n-1 : n;
}
#endif	/* XTI == 1 */



/*
**	Write to virtual circuit.
*/

int
VCWrite(fd, data, size, name)
	int		fd;
	char *		data;
	int		size;
	char *		name;
{
	register int	n;
	register int	r;
	register int	s = size;

#	ifdef	lint
	name = name;
#	endif

	TraceData("VC write", s, data);

	if ( (r = s) == 0 )
	{
		longjmp(VCerrorJmp, EIO);
		return size;
	}

	Jump = true;

	if ( DataChannels != 0 )
		ActiveRawBytes += s * ACT_MULT;
	DMNSTATSINCC(DS_VCDATA, s);

#if	XTI == 1
	while ( xti_connect ? ((n = xti_write(fd, data, s)) != s )
			    : ((n = write(fd, data, s)) != s ) )
#else	/* XTI == 1 */
	while ( (n = write(fd, data, s)) != s )
#endif	/* XTI == 1 */
	{
		if ( n == 0 )
		{
			if ( --r >= 0 )
				continue;
			Jump = false;
			longjmp(VCerrorJmp, EIO);
			return size;
		}

		if ( n < 0 )
		{
			if ( errno == EINTR )
				continue;
#			ifdef	APOLLO
			if ( errno == EIO )
				continue;
#			endif	/* APOLLO */
#			if	XTI == 1
			Trace3(1, "xti=%d/%d", xti_connect, t_errno);
#			endif	/* XTI == 1 */
			Jump = false;
			longjmp(VCerrorJmp, errno);
			return size;
		}

		DMNSTATSINC(DS_VCPARTWRITE);

		data += n;
		s -= n;
	}

	Jump = false;
	return size;
}


/*
**	Catch SIGWAKEUP/SIGWRITER, and longjmp.
*/

void
JmpWakeCatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
#	if	SIGWRITER != SIGWAKEUP
	if ( sig == SIGWAKEUP )
		sig = SIGWRITER;
	else
		sig = SIGWAKEUP;
	(void)signal(sig, SIG_IGN);
#	endif	/* SIGWRITER != SIGWAKEUP */
	(void)alarm(0);
	if ( WakeJmped )
		return;
	WakeJmped = true;
	(void)longjmp(WakeJmp, sig);
}



/*
**	Catch SIGWAKEUP/SIGWRITER, and remember.
*/

void
WakeupCatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
#	if	SIGWRITER != SIGWAKEUP
	if ( sig == SIGWAKEUP )
		sig = SIGWRITER;
	else
		sig = SIGWAKEUP;
	(void)signal(sig, SIG_IGN);
#	endif	/* SIGWRITER != SIGWAKEUP */
	(void)alarm(0);
	Woken = true;
}



#if	DEBUG >= 2
static struct
{
	Uint	bit;
	char *	name;
}
	StatusFlags[] =
{
	{CHOUTONLY,	"CHOUTONLY"},
	{CHINONLY,	"CHINONLY"},
	{CHTURNAROUND,	"CHTURNAROUND"},
	{CHRMQERCVD,	"CHRMQERCVD"},
	{CHLMQERCVD,	"CHLMQERCVD"}
};

#define	NFLAGS	((sizeof StatusFlags)/(sizeof StatusFlags[0]))


char *
StrStatus()
{
	register int	j, k;
	register char *	cp;
	static char	buf[64];

	cp = strcpyend(buf, "Status:");

	for ( j = 1, k = 0 ; k < NFLAGS ; k++, j <<= 1 )
		if ( Status.st_flags & StatusFlags[k].bit )
		{
			*cp++ = ' ';
			cp = strcpyend(cp, StatusFlags[k].name);
		}

	return buf;
}
#endif	/* DEBUG >= 2 */
