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


/*
**	The 'Ant' algorithm for calculating all shortest paths from a node:-
**
**	Create one ant to arrive at the starting node,
**		and put it at the head of a priority list;
**
**	while there are ants in the priority list
**	{
**		if node for the first ant has not been visited
**		{
**			mark node as visited, and set its shortest path;
**
**			for each edge to an unvisited node
**			{
**				create an ant to travel to that	node
**					through a distance proportional
**					to the cost of traversing the edge;
**
**				sort the ant into the priority list
**					based on distance;
**			}
**		}
**
**		remove first ant from list;
**	}
*/

/*
**	Use probabilistic skip list for sort, with p = 1/4.
*/

#define	MAXLEVEL	10		/* log4(1024K) */
#define	MINLEVEL	 5		/* log4(1K) */

/*
**	Structure for active Ant list
*/

typedef struct Ant	Ant;

struct Ant
{
	Ulong	ant_dist;		/* Accumulated link distance */
	Ant *	ant_prev;		/* Previous ant in route */
	Region *ant_region;		/* Destination */
	Region *ant_route;		/* Original link */
	Region *ant_from;		/* Came via */
	Region *ant_restrict;		/* Routing restriction */
	int	ant_linkno;		/* Link number of route */
	Flags	ant_flags;		/* Link flags */
	Ant *	ant_next[MAXLEVEL];	/* Next ant in priority list */
	/* On end so we need only allocate enough pointers for level */
};


static Ant *		freelist[MAXLEVEL];	/* Ant cache for each level */

extern bool		Warnings;



/*
**	Calculate shortest paths to all nodes from 'home'.
*/

void
Paths(home, type, route)
	Region *		home;
	RFlags			type;
	int			route;
{
	register int		i;
	register Ant *		dap;
	register Ant *		sap;
	register Ant *		nap;
	register Ant *		alist;
	register NLink *	lp;
	register Region *	rp;

	bool			pr_route = (bool)((type & RF_PRINTROUTE) != 0);
	bool			reach = (bool)((type & RF_REGLINK) != 0);
	bool			first_ant = true;
	bool			search_link;

	Ant			head;		/* Skip list */
	Ant *			next[MAXLEVEL];
	int			level;

	DODEBUG(register int	ant_sorts=0; int osorts);
	DODEBUG(int edges_traversed=0; int ants_in_list=1; int max_ants=0);
	DODEBUG(int regions_reached=0; int edges_searched=0; int ants_created=1);

	Trace6(
		1, "Paths(%s, %#x%s%s, %s)",
		home->rg_name,
		type,
		pr_route?"[ROUTE]":EmptyString,
		reach?"[REACH]":EmptyString,
		route==FASTEST?"FAST":"CHEAP"
	);

	DODEBUG(if(pr_route&&reach)Fatal("incompatible types in Paths()"));

	if ( !pr_route && (alist = freelist[0]) != (Ant *)0 )
	{
		freelist[0] = alist->ant_next[0];
		(void)strclr((char *)alist, sizeof(Ant)-(MAXLEVEL-1)*sizeof(Ant *));
	}
	else
	{
		alist = (Ant *)Malloc0(sizeof(Ant)-(MAXLEVEL-1)*sizeof(Ant *));
		if ( Traceflag < 2 )
			SRand((int)Time);
	}

	alist->ant_route = home;
	alist->ant_region = home;
	alist->ant_from = home;
	alist->ant_linkno = -1;

	level = 0;

	for ( i = MAXLEVEL ; --i > 0 ; )
		head.ant_next[i] = (Ant *)0;

	head.ant_next[0] = alist;

	do
	{
		Trace3(3, "distance = %lu, ants %d", (PUlong)alist->ant_dist, ants_in_list);

		rp = alist->ant_region;

		DODEBUG(edges_traversed++);
		Trace6(
			3,
			"ant from %9.9s at %9.9s (route %9.9s), restrict %9.9s, dist %10lu",
			alist->ant_route->rg_name,
			rp->rg_name,
			rp->rg_route[route]==(Region *)0
				? EmptyString
				: rp->rg_route[route]->rg_name,
			alist->ant_restrict==(Region *)0
				? EmptyString
				: alist->ant_restrict->rg_name,
			(PUlong)alist->ant_dist
		);

		search_link = false;

		do
		{
			if ( rp->rg_route[route] == (Region *)0 )
			{
				/*
				**	Found a shortest path to this region.
				*/

				DODEBUG(regions_reached++);
real_path:
				Trace4(
					2,
					"shortest path to %9.9s via %9.9s dist %10lu",
					rp->rg_name,
					alist->ant_route->rg_name,
					(PUlong)alist->ant_dist
				);

				search_link = true;

				rp->rg_dist[route] = alist->ant_dist;
				rp->rg_found[route] = alist->ant_restrict;
				if ( pr_route )
					rp->rg_route[route] = (Region *)alist;
				else
					rp->rg_route[route] = alist->ant_route;

				rp->rg_flags &= ~(RF_PSEUDO|RF_MARK);

				if ( alist->ant_flags & FL_PSEUDO )
					rp->rg_flags |= RF_PSEUDO;

				if ( alist->ant_flags & FL_FOREIGN )
					rp->rg_flags |= RF_MARK;	/* Not real route */
			}
			else
			if ( (rp->rg_flags & RF_MARK) && !(alist->ant_flags & FL_FOREIGN) )
				goto real_path;		/* Override less desirable route */
			else
			{
				if
				(
					rp->rg_found[route] != alist->ant_restrict
					&&
					InRegion(rp->rg_found[route], alist->ant_restrict)
				)
				{
					search_link = true;
					rp->rg_found[route] = alist->ant_restrict;
				}

				if
				(
					route == FASTEST
					&&
					rp->rg_route[route] != alist->ant_route
					&&
					alist->ant_dist == rp->rg_dist[route]
					&&
					!pr_route
				)
				{
					rp->rg_flags |= RF_DUPROUTE;

					if ( Warnings )
					Warn
					(
						english("Duplicate shortest paths:\n\tfrom \"%s\" to \"%s\"\n\t via \"%s\"\n\t and \"%s\""),
						home->rg_name,
						(rp->rg_name[0] == '\0') ? "*WORLD*" : rp->rg_name,
						rp->rg_route[route]->rg_name,
						alist->ant_route->rg_name
					);
				}
			}

			if ( alist->ant_linkno >= 0 && alist->ant_dist < DEAD_DELAY )
			{
				/*
				**	Set bit in AllRegTable for each region
				**	reachable from 'home' via this link.
				*/

				register int	i;

				i = (LinkCount * rp->rg_number) + alist->ant_linkno;
				AllRegTable[i/8] |= 1 << (i%8);

				Trace5(3, "Set bit %4d for %9.9s no. %3d link %2d", i, rp->rg_name, rp->rg_number, alist->ant_linkno);
			}

			if ( rp == alist->ant_restrict || rp == alist->ant_region->rg_visible )
				break;
		}
		while
			( (rp = rp->rg_up) != (Region *)0 );

		if ( first_ant )
		{
			first_ant = false;
			alist->ant_route = (Region *)0;
		}

		/*
		**	Search all edges to unfound regions.
		*/

		if ( search_link && !(alist->ant_flags & FL_FOREIGN) )
		for
		(
			lp = alist->ant_region->rg_links ;
			lp != (NLink *)0 ;
			lp = lp->nl_next
		)
		{
			register Ulong	dist;

			if ( (rp = lp->nl_to) == alist->ant_from )
				continue;
			if ( lp->nl_flags & FL_NOLINK )
				continue;

			DODEBUG(edges_searched++);

			do
			{
				Region *	fp;

				if ( (fp = rp->rg_route[route]) == (Region *)0 )
					goto do_link;

				if
				(
					fp != alist->ant_restrict
					&&
					InRegion(fp, alist->ant_restrict)
				)
					goto do_link;

				if ( rp == alist->ant_restrict || rp == lp->nl_to->rg_visible )
					break;
			}
			while
				( (rp = rp->rg_up) != (Region *)0 );

			continue;
do_link:
			DODEBUG(osorts = ant_sorts);
			DODEBUG(ant_sorts += level);

			/*
			**	New edge to travel, calculate distance.
			**
			**	(CostLink always >0.)
			*/

			if ( (dist = alist->ant_dist + CostLink(lp, route)) <= alist->ant_dist )
			{
				dist = MAX_ULONG;	/* Overflow */

				if ( Warnings )
				Warn
				(
					english("Max. link weight exceeded for link from \"%s\" to \"%s\""),
					alist->ant_region->rg_name,
					lp->nl_to->rg_name
				);

				/*
				**	Sort ant to end of priority list.
				*/

				for ( sap = &head, i = level ; i >= 0 ; i-- )
				{
					while
					(
						(dap = sap->ant_next[i]) != (Ant *)0
						&&
						dist >= dap->ant_dist
					)
					{
						DODEBUG(ant_sorts++);
						sap = dap;
					}

					next[i] = sap;
				}
			}
			else
			{
				/*
				**	Sort ant into priority list.
				*/

				for ( sap = &head, i = level ; i >= 0 ; i-- )
				{
					while
					(
						(dap = sap->ant_next[i]) != (Ant *)0
						&&
						dist > dap->ant_dist
					)
					{
						DODEBUG(ant_sorts++);
						sap = dap;
					}

					next[i] = sap;
				}
			}

			/*
			**	Choose level.
			*/

			for ( i = 0 ; (Rand() & 0x7fff) < 0x1fff ; )	/* `p' = 1/4 */
				if ( ++i > level )
				{
					Trace2(2, "New level %d", i);

					if ( i < MAXLEVEL )
					{
						level = i;
						next[i] = &head;
						if ( i >= MINLEVEL )
							break;		/* fix dice */
						continue;
					}

					i = level;
					break;
				}

			/*
			**	Set up new ant at appropriate level.
			*/

			DODEBUG(ants_created++);
			DODEBUG(if(++ants_in_list>max_ants)max_ants=ants_in_list);

			if ( !pr_route && (nap = freelist[i]) != (Ant *)0 )
				freelist[i] = nap->ant_next[0];
			else
				nap = (Ant *)Malloc(sizeof(Ant)-(MAXLEVEL-1-i)*sizeof(Ant *));

			rp = lp->nl_to;
			nap->ant_region = rp;
			nap->ant_from = alist->ant_region;
			nap->ant_prev = alist;

			nap->ant_dist = dist;

			if ( (nap->ant_flags = (lp->nl_flags & FL_PSEUDO)) )
				nap->ant_restrict = rp;
			else
			{
				nap->ant_restrict = MinRegion(alist->ant_restrict, lp->nl_restrict);
				if ( !InRegion(alist->ant_region->rg_visible, rp->rg_visible) )
					nap->ant_restrict = MinRegion(nap->ant_restrict, rp->rg_visible);
			}

			if ( !InRegion(rp, alist->ant_restrict) )
				nap->ant_flags |= FL_FOREIGN;

			if ( (nap->ant_route = alist->ant_route) == (Region *)0 )
			{
				nap->ant_route = rp;	/* A 'home' link */

				if ( reach )
					nap->ant_linkno = ((Link *)(nap->ant_route->rg_down))->ln_index;
				else
					nap->ant_linkno = -1;
			}
			else
				nap->ant_linkno = alist->ant_linkno;

			/*
			**	Insert into list.
			*/

			do
			{
				nap->ant_next[i] = next[i]->ant_next[i];
				next[i]->ant_next[i] = nap;
			}
			while
				( --i >= 0 );

			Trace7(
				4,
				"new ant from %9.9s (link %2d) via %9.9s to %9.9s dist %10lu sorts %10lu",
				nap->ant_route->rg_name,
				nap->ant_linkno,
				alist->ant_region->rg_name,
				nap->ant_region->rg_name,
				(PUlong)nap->ant_dist,
				(PUlong)ant_sorts-osorts
			);
		}

		/*
		**	Remove first ant from head of list.
		*/

		for ( i = 0 ; i <= level ; i++ )
			if ( head.ant_next[i] == alist )
				head.ant_next[i] = alist->ant_next[i];
			else
				break;

		DODEBUG(if(i==0)Fatal("ant head error"));

		if ( --i == level )
			while ( level && head.ant_next[level] == (Ant *)0 )
			{
				level--;
				Trace2(2, "New level %d", level);
			}

		alist->ant_next[0] = freelist[i];
		freelist[i] = alist;

		DODEBUG(ants_in_list--);
	}
	while
		( (alist = head.ant_next[0]) != (Ant *)0 );

	DODEBUG(if(ants_in_list)Fatal("%d ants marooned",ants_in_list));

	DODEBUG(
		if ( Traceflag == 0 )
			return;
		Trace
		(
			1,
			"\tedges traversed = %d, edges searched = %d, regions reached = %d",
			edges_traversed,
			edges_searched,
			regions_reached
		);
		Trace
		(
			1,
			"\tants created = %d, max ants = %d, ant sorts = %d",
			ants_created,
			max_ants,
			ant_sorts
		);
	);
}



/*
**	Call function with regions on route to this region.
*/

void
PathRoutes(rp, funcp, top)
	Region *	rp;
	vFuncp		funcp;
	bool		top;
{
	register Ant *	a1;
	register Ant *	a2;
	register Ulong	d2;

	if ( (a1 = (Ant *)rp) == (Ant *)0 )
	{
		Trace3(4, "PathRoutes(%s,,%d)", NullStr, top)

		(*funcp)((Ulong )0, (Ulong )0, (char *)0);
	}
	else
	{
		Trace3(4, "PathRoutes(%s,,%d)", a1->ant_region->rg_name, top)

		if ( top )
			(*funcp)(a1->ant_dist, a1->ant_dist, a1->ant_region->rg_name);

		if ( (a2 = a1->ant_prev) == (Ant *)0 )
			d2 = 0;
		else
			d2 = a2->ant_dist;

		for ( ;; )
		{
			(*funcp)(a1->ant_dist, d2, a1->ant_from->rg_name);
	
			if ( a1->ant_from == HomeRegion || (a1 = a2) == (Ant *)0 )
				return;

			if ( (a2 = a2->ant_prev) == (Ant *)0 )
				d2 = 0;
			else
				d2 = a2->ant_dist;
		}
	}
}
