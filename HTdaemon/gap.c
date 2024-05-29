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


#include	"global.h"
#include	"debug.h"
#include	"driver.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"packet.h"
#include	"pktstats.h"
#include	"status.h"

/*
**	Gap management.
*/

static Gap *	GapFreeList;



/*
**	(Partially) fill in any gap.
*/

bool
FillGap(chnp, gapstart, gapsize)
	Chan *		chnp;
	register Ulong	gapstart;
	Ulong		gapsize;
{
	register Gap *	gp;
	register Gap **	gpp;
	register Ulong	gapend;
	register Gap *	ngp;
	bool		retval = false;

	Trace4(1, "FillGap on channel %d at %lu size %lu", chnp->ch_number, (PUlong)gapstart, (PUlong)gapsize);

	DODEBUG(if(gapsize==0)Fatal("FillGap 0!"));

	gapend = gapstart + gapsize;

	for ( gpp = &chnp->ch_gaplist ; (gp = *gpp) != (Gap *)0 ; )
	{
		if ( gapstart >= gp->end_gap )
		{
			gpp = &gp->next_gap;
			continue;
		}

		if ( gapend <= gp->start_gap )
			break;

		if ( gapstart <= gp->start_gap )
		{
			if ( gapend >= gp->end_gap )
			{
				*gpp = gp->next_gap;
				gp->next_gap = GapFreeList;
				GapFreeList = gp;
			}
			else
			{
				gp->start_gap = gapend;
				gpp = &gp->next_gap;
			}

			if ( (ngp = chnp->ch_gaplist) == (Gap *)0 )
				chnp->ch_goodaddr = chnp->ch_msgsize;
			else
				chnp->ch_goodaddr = ngp->start_gap;

			if ( gapend <= gp->end_gap )
				return true;

			retval = true;
			continue;
		}

		if ( gapend >= gp->end_gap )
		{
			if ( gapend == gp->end_gap )
			{
				gp->end_gap = gapstart;
				return true;
			}

			gp->end_gap = gapstart;
			retval = true;
			gpp = &gp->next_gap;
			continue;
		}

		/*
		**	Create a hole.
		*/

		if ( (ngp = GapFreeList) == (Gap *)0 )
			ngp = Talloc(Gap);
		else
			GapFreeList = ngp->next_gap;

		ngp->start_gap = gapend;
		ngp->end_gap = gp->end_gap;
		gp->end_gap = gapstart;
		ngp->send_gap = gp->send_gap;
		ngp->time_gap = gp->time_gap;

		ngp->next_gap = gp->next_gap;
		gp->next_gap = ngp;

		return true;
	}

	return retval;
}



/*
**	Flush any outstanding gaps.
*/

void
FlushGaps(chnp)
	register Chan *	chnp;
{
	register Gap *	gp;

	while ( (gp = chnp->ch_gaplist) != (Gap *)0 )
	{
		chnp->ch_gaplist = gp->next_gap;
		gp->next_gap = GapFreeList;
		GapFreeList = gp;
	}
}



/*
**	NAK any existing gaps in response to EOMs, or that have timed out.
*/

int
NAKGaps(chnp, maxlength)
	Chan *		chnp;
	Ulong		maxlength;
{
	register Gap *	gp;
	register int	count;
	register Time_t	timeout;

	timeout = Time - NAK_TIMEOUT;

	for
	(
		gp = chnp->ch_gaplist, count = 0 ;
		gp != (Gap *)0 && maxlength > 0 ;
		gp = gp->next_gap, count++
	)
	{
		if ( gp->send_gap < chnp->ch_eomcount )
			gp->send_gap++;
		else
		if ( gp->time_gap > timeout )
			continue;

		maxlength -= SendNAK(chnp, PKTDATATYPE, gp->end_gap - gp->start_gap, gp->start_gap);

		gp->time_gap = Time;
	}

	return count;
}



/*
**	Note and NAK new gap, (coalesce adjacent gaps).
*/

void
QueueGap(chnp, gapend)
	Chan *		chnp;
	Ulong		gapend;	/* Should be monotonically increasing */
{
	register Gap *	gp;
	register Gap **	gpp;
	register Ulong	gapstart;
	register Gap *	ngp;
	bool		new;
	static char	gaprep[]	= english("queue gap < previous!");

	gapstart = chnp->ch_msgaddr;

	Trace4(1, "QueueGap on channel %d at %lu size %lu", chnp->ch_number, (PUlong)gapstart, (PUlong)(gapend-gapstart));

	DODEBUG(if((gapend-gapstart)==0)Fatal("QueueGap 0!"));

	for ( gpp = &chnp->ch_gaplist ; (gp = *gpp) != (Gap *)0 ; gpp = &gp->next_gap )
	{
		if ( gapstart > gp->end_gap )
			continue;

		if ( gapend < gp->start_gap )
		{
			Report7(
				RepFmt,
				chnp->ch_number,
				ChnStateStr(chnp),
				gaprep,
				chnp->ch_msgid,
				(PUlong)gapstart,
				(PUlong)gp->start_gap
			);
			break;
		}

		new = false;

		if ( gapstart < gp->start_gap )
		{
			Report7(
				RepFmt,
				chnp->ch_number,
				ChnStateStr(chnp),
				gaprep,
				chnp->ch_msgid,
				(PUlong)gapstart,
				(PUlong)gp->start_gap
			);
			gp->start_gap = gapstart;
			new = true;
		}

		if ( gapend > gp->end_gap )
		{
			while ( (ngp = gp->next_gap) != (Gap *)0 )
			{
				Report7(
					RepFmt,
					chnp->ch_number,
					ChnStateStr(chnp),
					gaprep,
					chnp->ch_msgid,
					(PUlong)gapstart,
					(PUlong)ngp->start_gap
				);

				if ( gapend < ngp->start_gap )
					break;

				if ( gapend < ngp->end_gap )
					gapend = ngp->end_gap;

				gp->next_gap = ngp->next_gap;
				ngp->next_gap = GapFreeList;
				GapFreeList = ngp;
			}

			gp->end_gap = gapend;
			goto nak;
		}

		if ( new )
			goto nak;
		return;
	}

	if ( (gp = GapFreeList) == (Gap *)0 )
		gp = Talloc(Gap);
	else
		GapFreeList = gp->next_gap;

	gp->start_gap = gapstart;
	gp->end_gap = gapend;
	gp->time_gap = Time;
	gp->send_gap = 1;

	gp->next_gap = *gpp;
	*gpp = gp;
nak:
	if ( gapstart < chnp->ch_goodaddr )
	{
		chnp->ch_goodaddr = gapstart;
		Write_Status();
	}

	(void)SendNAK(chnp, PKTDATATYPE, gapend - gapstart, gapstart);
}
