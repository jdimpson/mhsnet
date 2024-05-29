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


#define	TURN_AROUND	(10*60)	/* Turn-around time for half-duplex mode in secs */

/*
**	Deal with control message from link.
**	All messages have an "id",
**	some may also have a size,
**	and a SOM has a wait time.
*/

#include	"global.h"
#include	"debug.h"

#include	"Stream.h"
#include	"daemon.h"
#include	"driver.h"
#include	"AQ.h"
#include	"CQ.h"
#include	"proto.h"


static Time_t	XmitTime;	/* Last time finished transmitting */

static bool	StartInStream _FA_((Str_p));
extern int	Psend _FA_((int));



void
recvControl(chan, data, size)
	int		chan;
	char *		data;
	int		size;
{
	register int	i;
	register char *	cp;
	register Str_p	strp;
	register int	items = 0;
	char *		item[3];

	Trace4(
		 1
		,"recvControl for channel %d inState %d outState %d"
		,chan
		,inStreams[chan].str_state
		,outStreams[chan].str_state
	);

	if ( data[0] != MQ_EMPTY )
	{
		/*
		**	Data in control message are null terminated strings.
		*/

		size--;

		cp = &data[1];
		item[0] = cp;

		for ( items = 0, i = 0 ; i < size ; i++, cp++ )
			if ( (*cp & 0177) == '\0' )
			{
				*cp = '\0';
				if ( ++items >= 3 )
					break;
				item[items] = cp+1;
			}

		if
		(
			items == 0
			||
			(item[1] - item[0]) > (13+1)
			||
			i < (size-1)
		)
		{
			Fatal4("bad control message from channel %d: \"%.*s\"", chan, size, &data[1]);
			return;
		}

		Trace3(1, "recvControl \"%d\" id=\"%s\"", data[0], item[0]);
	}
	else
		Trace1(1, "recvControl MQ_EMPTY");

	switch ( data[0] )
	{
	case SOM:
		/*
		**	Start-Of-Message.
		**	"data" contains a message "id", its size, and its wait time.
		*/
		
		if ( items != 3 )
		{
			Fatal4(
				 "bad SOM format from channel %d: \"%.*s\""
				,chan
				,size
				,&data[1]
			);
			return;
		}

		Trace3(1, "SOM size %s wait %s", item[1], item[2]);

		strp = &inStreams[chan];

		switch ( strp->str_state )
		{
		case STR_ENDING:
			Report4(
				"SOM received for id \"%s\" while stream %d ending id \"%s\"",
				item[0],
				chan,
				strp->str_id
			);
			return;	/* EOMA will sort things out ... */

		case STR_AGAIN:
		case STR_ENDED:
			if
			(
				strcmp(strp->str_id, item[0]) == STREQUAL
				&&
				strp->str_size == atol(item[1])
			)
			{
				if ( strp->str_state == STR_ENDED )
				{
					SndEOMA(chan);
					return;
				}
				strp->str_flags |= STR_DUP;
			}
		case STR_IDLE:
		case STR_WAIT:
		case STR_START:
		case STR_EMPTY:
			(void)strcpy(strp->str_id, item[0]);
			strp->str_size = atol(item[1]);
			strp->str_time = LastTime - atol(item[2]);
			strp->str_posn = 0;

			StID(STINCHAN, chan, strp->str_id, strp->str_size, strp->str_posn, strp->str_time);

			if
			(
				strp->str_state == STR_WAIT
				||
				strp->str_state == STR_START
			)
				return;

			(void)StartInStream(strp);

			return;

		case STR_ACTIVE:
			if
			(
				strcmp(strp->str_id, item[0]) == STREQUAL
				&&
				strp->str_size == atol(item[1])
				&&
				strp->str_posn <= strp->str_size
			)
			{
				Report5(
					 "Input message \"%s\" for stream %d restarted at %lu, size %lu"
					,strp->str_id
					,chan
					,(PUlong)strp->str_posn
					,(PUlong)strp->str_size
				);

				break;
			}

			Report5(
				 "Input message \"%s\" on stream %d aborted at %lu, size %lu"
				,strp->str_id
				,chan
				,(PUlong)strp->str_posn
				,(PUlong)strp->str_size
			);

			(void)strcpy(strp->str_id, item[0]);
			strp->str_size = atol(item[1]);
			strp->str_time = LastTime - atol(item[2]);
#ifdef	notdef
			if ( strp->str_posn > strp->str_size )
			{
				(void)unlink(strp->str_fname);
#endif	/* notdef */
				(void)close(strp->str_fd);
				strp->str_fd = 0;
				strp->str_posn = 0;
				(void)StartInStream(strp);
				return;
#ifdef	notdef
			}

			(void)lseek(strp->str_fd, (long)0, 0);

			strp->str_posn = 0;
			StID(STINCHAN, chan, strp->str_id, strp->str_size, strp->str_posn, strp->str_time);
			break;
#endif	/* notdef */

		default:
			Fatal2("bad stream state %d", strp->str_state);
		}

		SndSOMA(chan);
		return;

	case SOM_ACCEPT:
		/*
		**	SOM Accept.
		**	"data" contains a message id and its restart address.
		*/

		if ( items != 2 )
		{
			Fatal2("bad SOM_ACCEPT format for stream %d", chan);
			return;
		}

		Trace2(1, "SOM_ACCEPT posn %s", item[1]);

		strp = &outStreams[chan];

		if
		(
			strp->str_state != STR_START
			||
			strcmp(strp->str_id, item[0]) != STREQUAL
		)
		{
			Report3("unexpected SOM_ACCEPT on stream %d, state %d", chan, strp->str_state);

			return;
		}

		Transmitting++;

		SetSpeed();

		if ( (strp->str_posn = atol(item[1])) > 0 )
		{
			if ( strp->str_posn > strp->str_size )
				Fatal4("message \"%s\" bad restart postion %lu on size %lu", strp->str_id, (PUlong)strp->str_posn, (PUlong)strp->str_size);
			Report4("Output message \"%s\" on stream %d restarted at %lu", strp->str_id, chan, (PUlong)strp->str_posn);
		}

		StID(STOUTCHAN, chan, strp->str_id, strp->str_size, strp->str_posn, strp->str_time);

		qAction(PosnStream, (AQarg)chan);
		return;

	case EOM:
		/*
		**	End-Of-Message.
		*/

		if ( items != 1 )
		{
			Fatal2("bad EOM format for stream %d", chan);
			return;
		}

		Trace1(1, "EOM");

		strp = &inStreams[chan];

		if
		(
			strcmp(strp->str_id, item[0]) != STREQUAL
			||
			strp->str_state != STR_ACTIVE
			||
			strp->str_size != strp->str_posn
		)
		{
			Report2("unexpected EOM on stream %d", chan);
			return;
		}

		(void)close(strp->str_fd);
		strp->str_fd = 0;

		if ( --Receiving < 0 )
			Receiving = 0;

		strp->str_state = STR_ENDING;
		StChState(STINCHAN, chan, CHN_ENDING);

		qAction(EndMessage, (AQarg)chan);
		return;

	case EOM_ACCEPT:
		/*
		**	EOM Accept.
		**	May get this if restarting
		**	and message has already been accepted.
		*/

		if ( items != 1 )
		{
			Fatal2("bad EOM_ACCEPT format for stream %d", chan);
			return;
		}

		Trace1(1, "EOM_ACCEPT");

		XmitTime = LastTime;

		strp = &outStreams[chan];

		if
		(
			(
				strp->str_state != STR_START
				&&
				strp->str_state != STR_ENDING
			)
			||
			strcmp(strp->str_id, item[0]) != STREQUAL
		)
		{
			Report3("unexpected EOM_ACCEPT on stream %d, state %d", chan, strp->str_state);

			return;	/* Wait for a timeout to set things right */
		}

		strp->str_messages++;
		NNstate.allmessages++;
		strp->str_bytes += strp->str_posn;
		StIncMsg(STOUTCHAN, chan);
		strp->str_state = STR_ENDED;
		StChState(STOUTCHAN, chan, CHN_ENDED);

		if ( Transmitting > 0 )	/* 'Cos not done in SendEOM */
			Transmitting--;	/*  but update will fix */

		if ( HalfDuplex && Transmitting == 0 )
		{
			/** Try switching directions **/

			for
			(
				strp = &inStreams[Fstream] ;
				strp < &inStreams[Nstreams] ;
				strp++
			)
				if ( strp->str_state == STR_WAIT )
					(void)StartInStream(strp);

			/** But give it something to think about **/
		}

		qAction(NewMessage, (AQarg)chan);
		return;

	case MQ_EMPTY:
		/*
		**	Remote message queue for this channel is empty.
		*/

		strp = &inStreams[chan];

		switch ( strp->str_state )
		{
		case STR_WAIT:
		case STR_START:
		case STR_ACTIVE:
		case STR_ENDING:
			Report3(
				"unexpected MQ_EMPTY on stream %d, input message \"%s\" aborted",
				chan,
				strp->str_id
			);
			if ( strp->str_state == STR_ACTIVE )
			{
#ifdef	notdef
				(void)unlink(strp->str_fname);
#endif	/* notdef */
				(void)close(strp->str_fd);
				strp->str_fd = 0;
				strp->str_posn = 0;
			}
		default:
			strp->str_state = STR_EMPTY;
			StChState(STINCHAN, chan, CHN_IDLE);
		}

		return;

	default:
		Fatal3("unrecognised control for stream %d = %d", chan, data[0]);
	}
}



/*
**	Start a new input message
**	-- if in half-duplex mode, ensure not already transmitting,
**	   and, if about to, arbitrate. (SUN I - all is forgiven!)
*/

static bool
StartInStream(strp)
	register Str_p	strp;
{
	register Str_p	strp2;

	if ( HalfDuplex && !Receiving )
	{
		if ( Transmitting )
		{
			strp->str_state = STR_WAIT;
			return false;
		}

		if ( XmitTime == 0 )
			XmitTime = LastTime;	/* Needs to go in main? */

		for
		(
			strp2 = &outStreams[Fstream] ;
			strp2 < &outStreams[Nstreams] ;
			strp2++
		)
			if
			(
				strp2->str_state == STR_START
				&&
				strrcmp(strp->str_id, strp2->str_id) < 0
			)
			{
				strp->str_state = STR_WAIT;
				return false;
			}
	}

	strp->str_state = STR_START;
	StChState(STINCHAN, (int)(strp-inStreams), CHN_STARTING);

	qAction(MakeTemp, (AQarg)(strp-inStreams));

	return true;
}



/*
**	Timeout from remote on idle transmit channel
*/

void
rTimeout(chan)
	int		chan;
{
	Trace4(1, "rTimeout on channel %d, instate %d, outstate %d", chan, inStreams[chan].str_state, outStreams[chan].str_state);

	(void)flushCq(chan, false);

	switch ( outStreams[chan].str_state )
	{
	case STR_START:
		break;					/* SOM is on its way ... */

	case STR_IDLE:
	case STR_EMPTY:
	case STR_ERROR:
		qAction(NewMessage, (AQarg)chan);	/* Scan for a new message */
		break;
	
	case STR_ACTIVE:
		while ( Psend(chan) > 0 );		/* Maybe protocol needs prodding */
		break;

	case STR_ENDED:					/* Patience -- we are being slow */
	case STR_ENDING:				/* Patience -- other end is being slow */
		break;
	}

	if ( inStreams[chan].str_state == STR_WAIT )
		(void)StartInStream(&inStreams[chan]);	/* Deadlock breaker */
}



/*
**	Reset on receive channel from remote: reset receiver.
*/

void
rReset(chan)
	int		chan;
{
	register Str_p	strp = &inStreams[chan];

	Trace3(1, "rReset on channel %d, instate %d", chan, strp->str_state);

	(void)flushCq(chan, false);

	if ( strp->str_state == STR_ACTIVE )
		strp->str_posn = lseek(strp->str_fd, (long)0, 2);
}



/*
**	Reset on transmit channel from remote: reset transmitter.
*/

void
xReset(chan)
	int	chan;
{
	Trace3(1, "xReset on channel %d, outstate %d", chan, outStreams[chan].str_state);

	(void)flushCq(chan, false);	/* SHOULD be TRUE -- but bug if resets received in wrong order with SOMA queued between. */

	switch ( outStreams[chan].str_state )
	{
	case STR_EMPTY:
		outStreams[chan].str_state = STR_IDLE;
	case STR_IDLE:
	case STR_ERROR:
		qAction(NewMessage, (AQarg)chan);
	case STR_ENDED:
		break;

	default:
		SendSOM(chan);
	}
}
