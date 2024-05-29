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
**	Functions that send control messages.
*/

#include	"global.h"
#include	"debug.h"

#include	"Stream.h"
#include	"daemon.h"
#include	"driver.h"
#include	"proto.h"



/*
**	Send a Start-Of-Message
*/

void
SendSOM(chan)
	int		chan;
{
	register Str_p	strp = &outStreams[chan];
	register Ulong	wait_time;
	register int	size;
	char		Control[1+STREAMIDZ+1+ULONG_SIZE+1+TIME_SIZE+1];

	Trace5(
		 1
		,"SendSOM for channel %d id \"%s\" size %lu wait %lu"
		,chan
		,strp->str_id
		,(PUlong)strp->str_size
		,(PUlong)(LastTime - strp->str_time)
	);

	strp->str_state = STR_START;
	StChState(STOUTCHAN, chan, CHN_STARTING);

	if ( LastTime < strp->str_time )
		wait_time = 0;	/* Someone screwed the date, ho, hum */
	else
		wait_time = LastTime - strp->str_time;

#	if	SPRF_SIZE != 0
	size = 1 + sprintf
#	else
		(void)sprintf
#	endif
		(
			 Control
			,"%c%s%c%lu%c%lu"
			,SOM
			,strp->str_id
			,NULLCH
			,(PUlong)strp->str_size
			,NULLCH
			,(PUlong)wait_time
		);

#	if	SPRF_SIZE == 0
	size = 1 + strlen(Control);
#	endif	/* SPRF_SIZE == 0 */

	qControl(chan, Control, size);
}



/*
**	Send a SOM_ACCEPT
*/

void
SndSOMA(chan)
	int		chan;
{
	register Str_p	strp = &inStreams[chan];
	register int	size;
	char		Control[1+STREAMIDZ+1+ULONG_SIZE+1];

	Trace4(1, "SndSOMA for channel %d id \"%s\" posn %lu", chan, strp->str_id, (PUlong)strp->str_posn);

	Receiving++;

	SetSpeed();

	strp->str_state = STR_ACTIVE;
	StChState(STINCHAN, chan, CHN_ACTIVE);

#	if	SPRF_SIZE != 0
	size = 1 + sprintf
#	else
		(void)sprintf
#	endif
		(
			 Control
			,"%c%s%c%lu"
			,SOM_ACCEPT
			,strp->str_id
			,NULLCH
			,(PUlong)strp->str_posn
		);

#	if	SPRF_SIZE == 0
	size = 1 + strlen(Control);
#	endif	/* SPRF_SIZE == 0 */

	qControl(chan, Control, size);
}



/*
**	Send an End-Of-Message
*/

void
SendEOM(chan)
	int		chan;
{
	register Str_p	strp = &outStreams[chan];
	register int	size;
	char		Control[1+STREAMIDZ+1];

	Trace3(1, "SendEOM for channel %d id \"%s\"", chan, strp->str_id);

	strp->str_state = STR_ENDING;
	StChState(STOUTCHAN, chan, CHN_ENDING);

	/*	This is too optimistic
	**	- should realy be done when transmit queue flushes
	**	- so wait for Update to fix.
	**
	**	if ( --Transmitting < 0 )
	**		Transmitting = 0;
	*/

#	if	SPRF_SIZE != 0
	size = 1 + sprintf
#	else
		(void)sprintf
#	endif
		(
			 Control
			,"%c%s"
			,EOM
			,strp->str_id
		);

#	if	SPRF_SIZE == 0
	size = 1 + strlen(Control);
#	endif	/* SPRF_SIZE == 0 */

	qControl(chan, Control, size);
}


void
SndEOMA(chan)
	int		chan;
{
	register Str_p	strp = &inStreams[chan];
	register int	size;
	char		Control[1+STREAMIDZ+1];

	Trace3(1, "SndEOMA for channel %d id \"%s\"", chan, strp->str_id);

	strp->str_state = STR_ENDED;
	StChState(STINCHAN, chan, CHN_ENDED);

#	if	SPRF_SIZE != 0
	size = 1 + sprintf
#	else
		(void)sprintf
#	endif
		(
			 Control
			,"%c%s"
			,EOM_ACCEPT
			,strp->str_id
		);

#	if	SPRF_SIZE == 0
	size = 1 + strlen(Control);
#	endif	/* SPRF_SIZE == 0 */

	qControl(chan, Control, size);
}



/*
**	Send a q-empty indicator.
*/

void
SendMQE(chan)
	int	chan;
{
	char	Control;

	Trace2(1, "SendMQE for channel %d", chan);

	outStreams[chan].str_state = STR_EMPTY;
	StChState(STOUTCHAN, chan, CHN_IDLE);

	Control = MQ_EMPTY;

	qControl(chan, &Control, 1);
}
