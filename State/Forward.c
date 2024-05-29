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
**	Routines to handle link forwarding.
*/

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"

#ifdef	FORWARDTABLE

static FLink *	FreeList;	/* FLink cache */
static int	FwdBufSize;	/* Length of forwarding buffer */
static int	FwdElSize;	/* Number of bytes per forward number */
static Index	LastBufSize;
static int	LastLinkCount;
static Region **Links;		/* Vector to access pointers to links */



static void
InitFwd()
{
	FwdElSize = (LinkCount+127)/128;
	FwdBufSize = FwdElSize * LinkCount;

	Trace3(1, "InitFwd FwdElSize=%d, FwdBufSize=%d", FwdElSize, FwdBufSize);
}



void
InitFwdBuf(c)
	int	c;
{
	Trace2(1, "InitFwdBuf(%#o)", c);

	InitFwd();

	if ( LastBufSize < FwdBufSize )
	{
		free(FwdBuf);
		FwdBuf = NULLSTR;
	}

	if ( FwdBuf == NULLSTR )
	{
		FwdBuf = Malloc(FwdBufSize+1);
		LastBufSize = FwdBufSize;
	}

	FwdBuf[0] = c;
}



void
InitFwdLinks()
{
	register NLink *	lp;
	register Region **	rpp;

	Trace1(1, "InitFwdLinks");

	InitFwd();

	if ( LastLinkCount < LinkCount && Links != (Region **)0 )
	{
		free(Links);
		Links = (Region **)0;
	}

	if ( (rpp = Links) == (Region **)0 )
	{
		Links = rpp = (Region **)Malloc(LinkCount * sizeof(Region **));
		LastLinkCount = LinkCount;
	}

	for ( lp = HomeRegion->rg_links ; lp != (NLink *)0 ; lp = lp->nl_next )
		*rpp++ = lp->nl_to;
}



void
NewFwd(rp, fp)
	Region *		rp;
	Region *		fp;
{
	register FLink *	flp;

	Trace3(3, "NewFwd(%s, %s)", rp->rg_name, fp->rg_name);

	if ( (flp = FreeList) != (FLink *)0 )
		FreeList = flp->fl_next;
	else
		flp = Talloc(FLink);

	flp->fl_to = fp;
	flp->fl_next = (FLink *)0;

	*rp->rg_lfwd = flp;
	rp->rg_lfwd = &flp->fl_next;
}



void
RmFwd(rp, fp)
	Region *		rp;
	Region *		fp;
{
	register FLink **	flpp;
	register FLink *	flp;

	Trace3(2, "RmFwd(%s, %s)", rp->rg_name, fp->rg_name);

	for ( flpp = &rp->rg_fwd ; (flp = *flpp) != (FLink *)0 ; flpp = &flp->fl_next )
		if ( flp->fl_to == fp )
		{
			*flpp = flp->fl_next;

			if ( rp->rg_lfwd == &flp->fl_next )
				rp->rg_lfwd = flpp;

			flp->fl_next = FreeList;
			FreeList = flp;

			break;
		}
}



bool
SetFwd(rp, vec, length)
	Region *	rp;
	register char *	vec;
	int		length;
{
	register int	linkno;
	register int	i;

	Trace4(2, "SetFwd(%s, %#x, %d)", rp->rg_name, (int)*(char *)vec, length);

	if ( FwdElSize == 0 )
		InitFwdLinks();

	if ( length > FwdBufSize )
		return false;

	while ( --length >= 0 )
	{
		for ( i = FwdElSize, linkno = 0 ; ; )
		{
			linkno |= *vec++;
			if ( --i <= 0 )
				break;
			linkno <<= 7;
		}

		if ( linkno >= LinkCount )
			return false;

		NewFwd(rp, Links[linkno]);
	}

	return true;
}



void
SetFwdBuf(linkno)
	register int	linkno;
{
	register int	i;

	Trace2(2, "SetFwdBuf(%d)", linkno);

	if ( (FwdBufCnt + FwdElSize) > FwdBufSize )
		Fatal1("Too many forwarding links");

	for ( i = FwdElSize ; ; )
	{
		FwdBuf[++FwdBufCnt] = linkno & 0177;

		if ( --i <= 0 )
			break;

		linkno >>= 7;
	}
}



/*
**	Sort forwarding list for each region into name order.
*/

void
SortFwd(rp)
	register Region *	rp;
{
	register FLink *	fp;
	register FLink **	fpp;
	register int		i;
	register bool		rescan;

	Trace2(2, "SortFwd(%s)", rp->rg_name);

	if ( rp->rg_fwd != (FLink *)0 )
	{
		/*
		**	Bubble sort as mostly pre-ordered.
		*/

		do
		{
			rescan = false;

			for
			(
				fpp = &rp->rg_fwd ;
				(fp = *fpp)->fl_next != (FLink *)0 ;
				fpp = &(*fpp)->fl_next
			)
				if
				(
					(i = strccmp
					     (
						fp->fl_to->rg_name,
						fp->fl_next->fl_to->rg_name
					     ))
					>= 0
				)
				{
					*fpp = fp->fl_next;
					if ( i == 0 )
					{
						fp->fl_next = FreeList;
						FreeList = fp;
						Report3(
							"Removed duplicate forward from \"%s\" to \"%s\"",
							rp->rg_name,
							fp->fl_to->rg_name
						);
					}
					else
					{
						fp->fl_next = fp->fl_next->fl_next;
						(*fpp)->fl_next = fp;
						rescan = true;
					}
				}
		}
			while ( rescan );

		rp->rg_lfwd = &fp->fl_next;
	}
}

#else	/* FORWARDTABLE */

_null_fwd(){};

#endif	/* FORWARDTABLE */
