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
**	Protocol parameters
*/

#define	MINPKTSIZE		16		/* Minimum packet size */
#define	MAXCNTLDATAZ		128		/* Max data in a control message */
#define	IDLE_TIMEOUT		61		/* Default idle timeout (also maximum value for RECEIVE_TIMEOUT) */
#define	RECEIVE_TIMEOUT		5		/* Default (minimum) receive timeout */
#define	IDLE_SCANRATE		5		/* Maximum idle scanrate */
#define	ACTIVE_SCANRATE		2		/* Minimum active scanrate */
#define	UPDATE_TIME		20		/* Perform house-keeping this often */
#define	BATCH_TIMEOUT		(1*IDLE_SCANRATE) /* Terminate if nothing happens */
#define	NULLCH			0200		/* Old stdio can't handle nulls */

Extern bool	Alarm;				/* TRUE if alarm call pending */
Extern bool	Finish;				/* True if batchmode terminating */
Extern Time_t	LastTime;			/* Latest time in seconds */
Extern int	ScanRate;			/* ScanRate for alarm() calls */
Extern int	Receiving;			/* Count of active receive streams */
Extern int	Transmitting;			/* Count of active send streams */
Extern unsigned	UpdateTime;			/* Trigger for background Update */

/*
**	Control alarm calls.
*/

#define	ALARM_ON(A)		{(void)alarm((unsigned)A);Alarm=true;}
#define	ALARM_OFF()		if(Alarm){(void)alarm(0);Alarm=false;}

#define	IOALRMON(A)	ALARM_ON(A)
#define	IOALRMOFF()	ALARM_OFF()

/*
**	Driver functions called from protocol handler
*/

extern int
		fillPkt _FA_((int, char *, int));

extern void
		recvData _FA_((int, char *, int)),
		recvControl _FA_((int, char *, int)),
		rTimeout _FA_((int)),
		rReset _FA_((int)),
		xReset _FA_((int)),
		qCpkt _FA_((char *, int)),
		qPkt _FA_((char *, int)),
		RCread _FA_((char *, int, bool, bool)),
		Rread _FA_((char *, int, bool, bool));

/*
**	Other driver functions
*/

extern void
		driver _FA_((void)),
		FixStreams _FA_((void)),
		NewState _FA_((int, bool)),
		qControl _FA_((int, char *, int)),
		SendEOM _FA_((int)),
		SendMQE _FA_((int)),
		SendSOM _FA_((int)),
		SetSpeed _FA_((void)),
		SndEOMA _FA_((int)),
		SndSOMA _FA_((int));

extern bool
		FindCommand _FA_((int)),
		RdCommand _FA_((int));
