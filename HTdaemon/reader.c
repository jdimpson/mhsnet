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
**	Reader process.
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
#include	"packet.h"
#include	"pktstats.h"
#include	"status.h"
#include	"spool.h"

#include	<setjmp.h>


static char	ReadCookedName[]	= english("cooked virtual circuit");
static char	ReadName[]		= english("virtual circuit");
extern jmp_buf	VCerrorJmp;
bool		WriterBlocked;		/* Alarm on pipe to writer */

extern SigFunc	alarm_catch;
static void	check_idle _FA_((void));
static void	read_cleanup _FA_((void));
int		VCreader _FA_((int, char *, int, char *));




void
alarm_catch(sig)
	int	sig;
{
	(void)signal(sig, alarm_catch);

	Trace3(1, "alarm_catch(%d) Jump=%d", sig, Jump);

	Time = time((time_t *)0);	/* Set current Time */

	if ( WriterBlocked )
	{
		Jump = true;
		Finish = true;
		FinishReason = &FinWrtrBlocked;
	}
	else
		check_idle();

	if ( Jump )
	{
		Jump = false;
		longjmp(VCerrorJmp, EINTR);
	}
}



static void
check_idle()
{
	register Ulong	u;
	register int	len;
	static int	dead_count;

	if ( IDLETime > 0 && IDLETime < (Time - LinkDownTime) )
	{
		IDLETime = Time;
		(void)SendMQE(CTRL, SENDLOCAL);	/* Send DOWN packet to writer */

		if ( !(Status.st_flags & CHLINKDOWN) )
		{
			Report1(english("Circuit timed out"));
			Status.st_flags |= CHLINKDOWN;
			Write_Status();
		}

		if ( DieOnDown )
		{
			if ( ++dead_count > 3 )
				Finish = true;
			FinishReason = &FinRdtimeout;
		}
	}

	if ( SendIDLE <= Time && Started )
	{
		len = HomeAdLength;
		S_MQEDATA(PktWriteData, HomeAddress, len);

		if ( (u = ActiveRawBytes / ActiveTime) == 0 )
			u = 1;

		(void)WritePkt(IDLE, PKTMQETYPE, len, u, PoutFd, PoutName);

		SendIDLE = Time + Idle_Timeout;
	}
}



void
reader(inpipefd, outpipefd)
	int	inpipefd;
	int	outpipefd;
{
	int	en;

	Trace3(1, "reader(%d, %d)", inpipefd, outpipefd);

	(void)close(1);
	VCfd = 0;				/* For reading packets from VC */
	PinFd = inpipefd;			/* For writing packets for writer */
	PoutFd = outpipefd;			/* For writing packets for VC */

	PktFuncsp = ReadReceivers;		/* For packets from VC */
	WriteFuncp = Write;			/* For packets to writer */

	IDLETime = Time + MAX_SYNC_TIME;
	LowSpaceTime = Time + BatchTime;	/* Wait before first signal */

	if ( !TurnAround )
	{
		if ( OutOnly )
		{
			RMQEreceived = true;
			Status.st_flags |= CHRMQERCVD;
		}
		else
		if ( InOnly )
		{
			LMQEreceived = true;
			Status.st_flags |= CHLMQERCVD;
		}
	}

	/*
	**	Use alarm to detect dead circuit.
	*/

	SendIDLE = Time + Idle_Timeout;

	(void)signal(SIGALRM, alarm_catch);

	/*
	**	Read and initialise Status structure.
	*/

	(void)InitStatus(INSTATUS);

	/*
	**	Read virtual circuit for something to do.
	*/

	if ( (en = setjmp(VCerrorJmp)) )
	{
		read_cleanup();

		if ( Finish )
			return;

		if ( (errno = en) != EINTR )
		{
			/*
			**	Read error on virtual circuit.
			*/

			FinishReason = &FinReaderror;
			Syserror(FinReaderror.reason);
			return;
		}
	}

	PacketReader(VCreader, VCfd, (CookVC!=(CookT *)0)?ReadCookedName:ReadName);

	Trace2(
		1,
		"reader:- PacketReader returns, reason \"%s\"",
		(FinishReason==(ExCode *)0) ? "<none>" : FinishReason->reason
	);

	read_cleanup();
}



/*
**	Flush message buffers.
*/

static void
read_cleanup()
{
	register Chan *	chnp;
	register int	i;

	for ( chnp = Channels, i = NCHANNELS ; --i >= 0 ; chnp++ )
		if ( chnp->ch_flags & CH_MSGFILE )
			FlushWData(chnp);
}



#if	XTI == 1
/*
** xti_read reads data from an XTI pipe and strips the leading status byte.
** Trying to read a buffer of the specified size gives bad headers, so
** we'll read size+1 bytes instead...
*/

static int
xti_read(fd, data, size)
	int		fd;
	char *		data;
	int		size;
{
	int	n;
	static int	t_localflags = 0;	/* Don't use any flags. */
	char *		buf;

	buf = Malloc(size+1);
	n = t_rcv(fd, buf, size+1, &t_localflags);
	/* t_rcv (and read() from an XTI pipe) prepends a status byte to
	** the data read. We'll remove that data by using bcopy() to move
	** the data back by a byte and returning one less than the actual
	** data read.
	*/
	bcopy(buf+1, data, n>1 ? n : size);
	free(buf);

	return n>0 ? n-1 : n;
}
#endif	/* XTI == 1 */



/*
**	Read the virtual circuit with op.sys error checks.
*/

/*ARGSUSED*/
int
VCreader(fd, data, size, name)
	int		fd;
	char *		data;
	int		size;
	char *		name;
{
	register int	n;
	register Ulong	u;
	static int	count_read0;

	Trace3(2, "VCreader size %d, time %lu", size, (PUlong)Time);

	DODEBUG(if(size==0)Fatal("VCreader size 0"));

	if ( Finish )
		return 0;

	Jump = true;

	if ( Idle_Timeout > 0 )
	{
		check_idle();

		if ( SendIDLE < (Time+MINSLEEP+1) )
			n = MINSLEEP+1;
		else
			n = SendIDLE - Time;

		(void)alarm((unsigned)n);
	}

	for ( ;; )
	{
#		if	XTI == 1
		if ( (n = (xti_connect ? xti_read(fd, data, size)
				       : read(fd, data, size))) == 0
			&& !Finish )
#		else	/* XTI == 1 */
		if ( (n = read(fd, data, size)) == 0 && !Finish )
#		endif	/* XTI == 1 */
		{
#			ifdef	APOLLO
read0:
#			endif	/* APOLLO */

			Report1("VC READ ZERO");
			DMNSTATSINC(DS_VCREAD0);

			if ( ++count_read0 > MAXREAD0 && DieOnDown )
			{
				Jump = false;
				longjmp(VCerrorJmp, EIO);
				break;
			}

			n = alarm((unsigned)0);
			(void)sleep(MINSLEEP);
			Time += MINSLEEP;

			if ( n < (MINSLEEP+1) )
			{
				Jump = false;
				longjmp(VCerrorJmp, EINTR);
				break;
			}

			(void)alarm((unsigned)n-MINSLEEP);
			continue;
		}

		if ( Finish )
		{
			n = 0;
			break;
		}

		if ( n < 0 )
		{
			if ( errno == EINTR )
				continue;

#			ifdef	APOLLO
			if ( errno == EIO )
				goto read0;
#			endif	/* APOLLO */

			Jump = false;
			longjmp(VCerrorJmp, errno);
			break;
		}

		count_read0 = 0;

		if ( (Time = time((time_t *)0)) < CTime )	/* -ve time increment? */
		{
			if ( (u = CTime - Time) > NVETIMECHANGE )
			{
				Jump = false;
				FinishReason = &FinNegtime;
				Error(FinNegtime.reason);
				n = 0;
				break;
			}
			DecayTime -= u;
			CTime = Time;
		}

		if ( MaxRunTime && MaxRunTime <= Time )
		{
			FinishReason = &FinMaxrun;
			n = 0;
			break;
		}

		if ( (u = Time - CTime) != 0 )
		{
			if ( DataChannels != 0 )
			{
				ActiveTime += u * ACT_MULT;

				if ( MinSpeed > 0 && (ActiveRawBytes/ActiveTime) < MinSpeed )
				{
					FinishReason = &FinRemSlow;
					n = 0;
					break;
				}

				if ( Time >= DecayTime )
					DecayActive();
			}
			else
				DecayTime += u;

			if ( NeedSignal )
			{
				(void)kill(OtherPid, SIGWRITER);	/* Wakeup writer */
				LastSignalTime = Time;
				NeedSignal = false;
			}

			if ( (CTime = Time) >= StatusTime )
				Write_Status();
		}

		if ( FreeFSpace == 0 || CheckFree <= Time )
		{
			(void)FSFree(SPOOLDIR, (Ulong)0);
			CheckFree = Time + FREESCANRATE;
		}

		DMNSTATSINCC(DS_VCDATA, n);
		TraceData("VC read ", n, data);

		if ( CookVC == (CookT *)0 || (n = (*CookVC->uncook)(data, n)) > 0 )
			break;
	}

	Jump = false;

	return n;
}
