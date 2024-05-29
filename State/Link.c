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
#include	"link.h"
#include	"route.h"
#include	"routefile.h"


static NLink *	FreeList;

extern bool	NoChange;



/*
**	Remove all links to/from a region.
*/

void
BreakLinks(rp)
	register Region *	rp;
{
	register NLink *	lp;
	register Region *	orp;

	Trace2(1, "BreakLinks(%s)", rp->rg_name);

	while ( (lp = rp->rg_links) != (NLink *)0 )
	{
		orp = lp->nl_to;
		UnLink(rp, orp);
		UnLink(orp, rp);
	}
}



/*
**	Remove PSEUDO links from a region.
*/

void
BreakPLinks(rp)
	Region *		rp;
{
	register NLink **	lpp;
	register NLink *	lp;

	Trace2(1, "BreakPLinks(%s)", rp->rg_name);

	NoChange = true;

	for ( lpp = &rp->rg_links ; (lp = *lpp) != (NLink *)0 ; )
		if ( lp->nl_flags & FL_PSEUDO )
			UnLinkP(rp, lpp);
		else
			lpp = &lp->nl_next;

	NoChange = false;
}



/*
**	Calculate link 'distance' according to routing parameters.
*/

Ulong
CostLink(lp, route)
	register NLink *	lp;
	int			route;
{
	register long		delay;

	if ( lp->nl_flags & FL_PSEUDO )
	{
		delay = MAX_LONG;
		goto out;
	}

	if ( lp->nl_flags & FL_DEAD )
		delay = DEAD_DELAY;
	else
	if ( lp->nl_flags & FL_DOWN )
		delay = DOWN_DELAY;
	else
	if ( (delay = lp->nl_delay) < MIN_WEIGHT )
		delay = MIN_WEIGHT;

	if ( route == CHEAPEST )
	{
#		if	MUL_DELAY_TO_COST != 1
		if ( (delay *= MUL_DELAY_TO_COST) < 0 )
			delay = MAX_LONG;
#		endif	/* MUL_DELAY_TO_COST != 1 */

#		if	DIV_DELAY_TO_COST != 1
		delay /= DIV_DELAY_TO_COST;
#		endif	/* DIV_DELAY_TO_COST != 1 */

		if ( lp->nl_cost > 1 )
			delay *= lp->nl_cost;
		if ( delay <= 0 || delay > (MAX_LONG/8) )	/* Overflow */
			delay = (MAX_LONG/8);
	}

out:	Trace3(5, "CostLink --> %s = %lu", lp->nl_to->rg_name, (PUlong)delay);

	return (Ulong)delay;
}



/*
**	If rp1 is linked to rp2, return pointer to link.
*/

NLink *
IsLinked(rp1, rp2)
	Region *		rp1;
	register Region *	rp2;
{
	register NLink *	lp;

	for
	(
		lp = rp1->rg_links ;
		lp != (NLink *)0 ;
		lp = lp->nl_next
	)
		if ( lp->nl_to == rp2 )
			break;

	Trace4(5, "IsLinked(%s, %s) ==> %s", rp1->rg_name, rp2->rg_name, (lp==(NLink *)0)?"FALSE":"TRUE");

	return lp;
}



/*
**
*/

bool
LegalLink(rp1, rp2)
	register Region *	rp1;
	register Region *	rp2;
{
	register Region *	rp;

	for ( rp = rp1 ; rp != (Region *)0 ; rp = rp->rg_up )
		if ( rp == rp2 )
			return false;

	for ( rp = rp2 ; rp != (Region *)0 ; rp = rp->rg_up )
		if ( rp == rp1 )
			return false;

	return true;
}



/*
**	Make link from region 1 to region 2 and return pointer.
*/

NLink *
MakeLink(rp1, rp2)
	register Region *	rp1;
	register Region *	rp2;
{
	register NLink *	lp;

	Trace3(3, "MakeLink(\"%s\" -> \"%s\")", rp1->rg_name, rp2->rg_name);

	if ( (lp = FreeList) != (NLink *)0 )
	{
		FreeList = lp->nl_next;
		(void)strclr((char *)lp, sizeof(NLink));
	}
	else
		lp = Talloc0(NLink);

	*rp1->rg_llink = lp;
	rp1->rg_llink = &lp->nl_next;

	lp->nl_to = rp2;

	if ( rp1 == HomeRegion || rp2 == HomeRegion )
		lp->nl_flags = FL_HOME;

	return lp;
}



/*
**	Set indices of links to home.
*/

void
SetLinkCount()
{
	register NLink *	lp;
	register Index		i;

	Trace1(1, "SetLinkCount");

	for
	(
		i = 0, lp = HomeRegion->rg_links ;
		lp != (NLink *)0 ;
		lp = lp->nl_next
	)
	{
		if ( !(lp->nl_to->rg_flags & RF_LINK) )
			Fatal2("Bad home link -> \"%s\"", lp->nl_to->rg_name);

		Trace3(2, "%s index %d", lp->nl_to->rg_name, i);

		((Link *)(lp->nl_to->rg_down))->ln_index = i++;
	}

	LinkCount = i;
}



/*
**	Sort links for each region into name order.
*/

void
SortLinks(rp)
	register Region *	rp;
{
	register NLink *	lp;
	register NLink **	lpp;
	register int		i;
	register bool		rescan;

	Trace2(2, "SortLinks(%s)", rp->rg_name);

	if ( rp->rg_links != (NLink *)0 )
	{
		/*
		**	Bubble sort as mostly pre-ordered.
		*/

		do
		{
			rescan = false;

			for
			(
				lpp = &rp->rg_links ;
				(lp = *lpp)->nl_next != (NLink *)0 ;
				lpp = &(*lpp)->nl_next
			)
				if
				(
					(i = strccmp
					     (
						lp->nl_to->rg_name,
						lp->nl_next->nl_to->rg_name
					     ))
					>= 0
				)
				{
					*lpp = lp->nl_next;
					if ( i == 0 )
					{
						lp->nl_next = FreeList;
						FreeList = lp;
						Report3(
							"Removed duplicate link from \"%s\" to \"%s\"",
							rp->rg_name,
							lp->nl_to->rg_name
						);
					}
					else
					{
						Trace3(3, "sort links %s<>%s", lp->nl_to->rg_name, lp->nl_next->nl_to->rg_name);
						lp->nl_next = lp->nl_next->nl_next;
						(*lpp)->nl_next = lp;
						rescan = true;
					}
				}
		}
			while ( rescan );

		rp->rg_llink = &lp->nl_next;
	}
}



/*
**	Remove link from region rp1 to region rp2.
*/

void
UnLink(rp1, rp2)
	register Region *	rp1;
	Region *		rp2;
{
	register NLink **	lpp;
	register NLink *	lp;

	Trace3(3, "UnLink(%s, %s)", rp1->rg_name, rp2->rg_name);

	for ( lpp = &rp1->rg_links ; (lp = *lpp) != (NLink *)0 ; lpp = &lp->nl_next )
		if ( lp->nl_to == rp2 )
		{
			UnLinkP(rp1, lpp);
			break;
		}
}



/*
**	Remove link (*lpp) from region rp.
*/

void
UnLinkP(rp, lpp)
	register Region *	rp;
	register NLink **	lpp;
{
	register NLink *	lp;
	register Region *	orp;
	register Entry *	ep;

	lp = *lpp;
	orp = lp->nl_to;

	Trace4(
		2, "UnLinkP(%s, %s)%s",
		rp->rg_name,
		orp->rg_name,
		(lp->nl_flags & FL_PSEUDO)?" [pseudo]":EmptyString
	);

	*lpp = lp->nl_next;

	if ( rp->rg_llink == &lp->nl_next )
		rp->rg_llink = lpp;

	lp->nl_next = FreeList;
	FreeList = lp;

	if ( rp == HomeRegion )
	{
		rp = orp;
		Trace3(1, "BreakLink(%s, %s)", HomeName, rp->rg_name);
		rp->rg_flags &= ~RF_LINKFROM;
	}
	else
	if ( orp == HomeRegion )
	{
		Trace3(1, "BreakLink(%s, %s)", rp->rg_name, HomeName);
		rp->rg_flags &= ~RF_LINKTO;
	}
	else
		return;

	SetChange(MinRegion(HomeRegion->rg_visible, rp->rg_visible));

	if ( (rp->rg_flags & (RF_LINK|RF_LINKFROM|RF_LINKTO)) != RF_LINK )
		return;

	rp->rg_flags &= ~RF_LINK;

	if
	(
		(ep = Lookup(((Link *)rp->rg_down)->ln_name, RegMapHash)) != (Entry *)0
		&&
		ep->MAP == rp->rg_name
	)
		ep->MAP = NULLSTR;	/* Lose it? */

	rp->rg_down = (Region *)0;
}
