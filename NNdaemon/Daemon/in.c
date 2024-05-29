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
**	Functions to handle temporary files for incoming messages.
*/

#include	"global.h"
#include	"debug.h"
#include	"spool.h"

#include	"AQ.h"
#include	"daemon.h"
#include	"Stream.h"
#include	"driver.h"


/*
**	Make a temporary file into which to receive message,
**	send an SOM_ACCEPT.
*/

void
MakeTemp(chan)
	AQarg		chan;
{
	register Str_p	strp = &inStreams[(int)chan];

	Trace3(1, "MakeTemp for channel %d state %d", (int)chan, strp->str_state);

	if ( strp->str_state != STR_START )
		return;

	if ( strp->str_fname != NULLSTR )
		free(strp->str_fname);

	strp->str_fname = UniqueName
			  (
				 concat(WorkDir, Template, NULLSTR)
				,CHOOSE_QUALITY
				,NOOPTNAME
				,(Ulong)strp->str_size
				,LastTime
			  );

	/*
	**	Save last component of work file name for possible recovery
	*/

	(void)strcpy(strp->str_recv.rh_id, strrchr(strp->str_fname, '/')+1);

	strp->str_fd = Create(strp->str_fname);

	strp->str_posn = 0;

	SndSOMA((int)chan);

	StID(STINCHAN, (int)chan, strp->str_recv.rh_id, strp->str_size, strp->str_posn, strp->str_time);

	Update(up_force);
}



/*
**	Receive stream data from a channel.
**	Called from Precv().
*/

void
recvData(chan, data, size)
	int		chan;
	char *		data;
	int		size;
{
	register Str_p	strp = &inStreams[chan];
	register int	s;

	Trace5(2, "recvData for channel %d state %d size %d posn %lu", chan, strp->str_state, size, (PUlong)strp->str_posn);

	if ( strp->str_state == STR_ACTIVE && (s = size) > 0 )
	{
		WriteJ(chan, strp->str_fd, data, s, strp->str_fname);

		inByteCount += s;
		StIncChan(STINCHAN, chan, s);

		if ( (strp->str_posn += s) > strp->str_size )
			Fatal4("chan %d bad posn %lu, size %lu", chan, (PUlong)strp->str_posn, (PUlong)strp->str_size);
	}
}
