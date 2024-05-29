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
**	Check consistency of graph.
*/

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"


static bool	Warnings;

extern bool	BreakPseudo;

static bool	CSlink1 _FA_((Region *));
static bool	CSlink2 _FA_((Region *));



/*
**	Check region tree, and set routes for broadcasting.
*/

void
CheckState(warn)
	bool	warn;
{
	Trace1(1, "CheckState()");

	if ( HomeRegion == TopRegion )
	{
		Error("null state");
		return;
	}

	Warnings = warn;
	RemovedRegions = 0;

	/*
	**	Sort and check links.
	*/

	if ( CSlink1(TopRegion) )	/* Returns true if home link added */
		SortLinks(HomeRegion);

	do
	{
		SetLinkCount();

		/*
		**	Initialise forwarding and regions tables.
		*/

		if ( LinkCount == 0 )
			AllRegTable = NULLSTR;
		else
			AllRegTable = Malloc0((int)(((AllRegCount * LinkCount) + 7) / 8));

		/*
		**	Find routes for broadcast to all reachable regions,
		**	and build "reachable" table for broadcasting.
		*/

		Paths(HomeRegion, RF_REGLINK, FASTEST);
	}
	while
	(
		/*
		**	Check links, remove any that point at unreachable regions.
		*/

		CSlink2(TopRegion)	/* Returns true if home link removed */
	);
}



/*
**	Break all links to removed regions, sort links.
*/

static bool
CSlink1(rp)
	register Region *	rp;
{
	register NLink *	lp;
	register NLink **	lpp;
	register Ulong		count;
	register bool		pseudo;
	register bool		val = false;

	do
	{
		count = 0;
		pseudo = false;

		for ( lpp = &rp->rg_links ; (lp = *lpp) != (NLink *)0 ; )
		{
			if ( (lp->nl_flags & FL_REMOVE) || (lp->nl_to->rg_flags & RF_REMOVE) )
			{
				UnLinkP(rp, lpp);
				continue;
			}

			lpp = &lp->nl_next;

			if ( lp->nl_flags & FL_PSEUDO )
				pseudo = true;
			else
				count++;

			lp->nl_flags &= ~FL_MARK;
		}

		if ( count == 1 )
			rp->rg_flags |= RF_LEAF;	/* Leaf node with one link */

		if ( pseudo && (BreakPseudo || count == 1) )
			BreakPLinks(rp);	/* No point in PSEUDO links from host with only one real link */

		if ( rp->rg_links != (NLink *)0 )
			SortLinks(rp);

		if ( rp->rg_down == (Region *)0 )
			continue;

		if ( rp->rg_flags & RF_LINK )
		{
			if ( rp->rg_flags & RF_LINKFROM )
				continue;

			Trace2(2, "add \"nolink\" from home to %s", rp->rg_name);

			MakeLink(HomeRegion, rp)->nl_flags |= FL_NOLINK;

			val = true;
		}
		else
		if ( CSlink1(rp->rg_down) )
			val = true;
	}
	while
		( (rp = rp->rg_next) != (Region *)0 );

	return val;
}



/*
**	Break links to unreachable regions,
**	break pseudo links to real nodes.
*/

static bool
CSlink2(rp)
	register Region *	rp;
{
	register NLink *	lp;
	register NLink **	lpp;
	register bool		remove;
	register bool		val = false;

	do
	{
		if ( rp->rg_route[FASTEST] == (Region *)0 )
		{
			if ( !(rp->rg_flags & RF_REMOVE) )
			{
				bool	warn = Warnings;

				if ( rp->rg_links != (NLink *)0 )
				{
					RemovedRegions++;
					warn = true;
				}

				if ( warn )
					Warn
					(
						english("Region \"%s\" unreachable"),
						(rp->rg_name[0]=='\0')?english("*WORLD*"):rp->rg_name
					);
			}

			if ( rp->rg_flags & RF_ALIAS )
			{
				RemEntMap(rp->rg_name, AliasHash);
				RemEntMap(rp->rg_name, RegMapHash);
			}

			if ( rp->rg_flags & RF_LINK )
				val = true;

			rp->rg_flags |= RF_REMOVE;	/* Don't bitch another time around */

			remove = true;
		}
		else
			remove = false;

		for ( lpp = &rp->rg_links ; (lp = *lpp) != (NLink *)0 ; )
		{
			register Region *	rp2 = lp->nl_to;
			register NLink *	lp2 = (NLink *)0;

			if
			(
				remove
				||
				(
					(lp->nl_flags & FL_PSEUDO)
					&&
					!(rp2->rg_flags & RF_PSEUDO)
				)
				||
				rp2->rg_route[FASTEST] == (Region *)0
			)
			{
				UnLinkP(rp, lpp);
				continue;
			}

			if
			(
				rp->rg_state == 0
				&&
				(lp2 = IsLinked(rp2, rp)) == (NLink *)0
			)
			{
				Warn(english("removing uni-directional link:\n\tfrom uncorroborated node \"%s\" to \"%s\""), rp->rg_name, rp2->rg_name);
				UnLinkP(rp, lpp);
				continue;
			}

			lpp = &lp->nl_next;

			if
			(
				!Warnings
				||
				(lp->nl_flags & (FL_MARK|FL_PSEUDO))
				||
				(rp->rg_flags & RF_LEAF)
				||
				(rp2->rg_flags & RF_LEAF)
			)
				continue;

			/** Check that link parameters are same in each direction **/

			if ( lp2 != (NLink *)0 || (lp2 = IsLinked(rp2, rp)) != (NLink *)0 )
			{
				char		tbuf[TIMESTRLEN];
				char		tbuf2[TIMESTRLEN];
				static char	mesgs[]		=
english("unequal link %ss:\n\t\"%s\" to \"%s\" => %s\n\t\"%s\" to \"%s\" => %s");
				static char	mesglu[]	=
english("unequal link costs:\n\t\"%s\" to \"%s\" => %lu\n\t\"%s\" to \"%s\" => %lu");

				lp2->nl_flags |= FL_MARK;

				if ( lp->nl_cost != lp2->nl_cost )
					Warn
					(
						mesglu,
						rp->rg_name, rp2->rg_name,
						(PUlong)lp->nl_cost,
						rp2->rg_name, rp->rg_name,
						(PUlong)lp2->nl_cost
					);

				if ( lp->nl_delay != lp2->nl_delay )
					Warn
					(
						mesgs, english("delay"),
						rp->rg_name, rp2->rg_name,
						TimeString(lp->nl_delay, tbuf),
						rp2->rg_name, rp->rg_name,
						TimeString(lp2->nl_delay, tbuf2)
					);

				if ( (lp->nl_flags & FL_ROUTING) != (lp2->nl_flags & FL_ROUTING) )
				{
					char *	cp;
					char *	cp2;

					cp = newstr(StringFlags((Flags)(lp->nl_flags & FL_ROUTING)));
					cp2 = newstr(StringFlags((Flags)(lp2->nl_flags & FL_ROUTING)));

					Warn
					(
						mesgs, english("flag"),
						rp->rg_name, rp2->rg_name,
						cp,
						rp2->rg_name, rp->rg_name,
						cp2
					);

					free(cp2);
					free(cp);
				}

				continue;
			}

			Warn(english("uni-directional link:\n\t\"%s\" to \"%s\""), rp->rg_name, rp2->rg_name);
		}

		if ( rp->rg_down != (Region *)0 && !(rp->rg_flags & RF_LINK) )
			if ( CSlink2(rp->rg_down) )
				val = true;
	}
	while
		( (rp = rp->rg_next) != (Region *)0 );

	return val;
}
