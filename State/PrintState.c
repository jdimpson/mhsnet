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


#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"
#include	"commands.h"


extern bool	DistAsTime;

static Entry **	PList;
static Entry ** PListp;
static RFlags	PMLmatch;
static FILE *	PrStFd;
static bool	PrStPseudo;
static int	PrStRoute;
static int	PrStType;
static char *	Searchmap;
static char *	Searchtype;

extern int	MLcompare _FA_((char *, char *));
int		PMLcompare _FA_((char *, char *));
static Index	PMLcountTree _FA_((Entry *));
static void	PMLfromTree _FA_((Entry *));
static Index	PrMakeList _FA_((Entry ***, Entry **, Index));
static void	SearchMap _FA_((char *, Entry **));
static void	SearchTree _FA_((Entry *));

static void	PrStComment _FA_((char *, char *));
void		PrStDist _FA_((Ulong, Ulong, char *));
static bool	PrStRegion _FA_((Region *));

#ifdef	FORCE_PRINT_ROUTES
static void	PrForceRoutes _FA_((void));
static void	pr_force_routes _FA_((void));
#endif	/* FORCE_PRINT_ROUTES */

#define	Fprintf	(void)fprintf

#define	LINK_NAME_SIZE	32



#ifdef	notdef
/*
**	Return name of first domain in string in copy.
*/

char *
FNameCopy(copy, string)
	char *		copy;
	char *		string;
{
	register int	c;
	register char *	sp;
	register char *	cp;
	register char *	tp;

	Trace3(5, "FNameCopy(%#lx, %s)", (PUlong)copy, string);

	sp = string;
	cp = tp = copy;

	do
		if ( (c = *sp++) == TYPE_SEP )
			cp = tp;
		else
		if ( (*cp++ = c) == DOMAIN_SEP )
			break;
	while
		( c != '\0' );

	if ( c != '\0' )
		*--cp = '\0';

	Trace3(4, "FNameCopy(%s) ==> %s", string, copy);

	return copy;
}
#endif	/* notdef */



/*
**	Print out State file from contents of sorted region list.
*/

void
PrintState(fd, match, type)
	FILE *			fd;
	register RFlags		match;
	int			type;
{
	register Entry **	epp;
	register int		i;
/*	register Region *	rp;
*/
	Trace4(1, "PrintState(%d, %#lx, %#x)", fileno(fd), (PUlong)match, type);

	if ( (PMLmatch = match) == (RFlags)0 )
		return;

	PrStFd = fd;
	PrStType = type;

	if ( type & PRINT_ROUTE )
	{
		PrStRoute = CHEAPEST;

		if ( type & PRINT_FASTEST )
		{
			PrStRoute = FASTEST;
			Paths(HomeRegion, RF_PRINTROUTE, FASTEST);
		}

		if ( type & PRINT_CHEAPEST )
			Paths(HomeRegion, RF_PRINTROUTE, CHEAPEST);
	}
	else
	if ( type & PRINT_VERBOSE )
	{
		Paths(HomeRegion, (RFlags)0, FASTEST);
		Paths(HomeRegion, (RFlags)0, CHEAPEST);
	}

#	ifdef	FORCE_PRINT_ROUTES
	if ( type & (PRINT_VERBOSE|PRINT_ROUTE) )
		PrForceRoutes();
#	endif	/* FORCE_PRINT_ROUTES */

	RegionCount = i = PrMakeList(&RegionList, RegionHash, RegionCount);

	for ( epp = RegionList ; --i >= 0 ; epp++ )
	{
/*		if ( (rp = (*epp)->REGION) == TopRegion )
**			continue;
*/
		if ( PrStRegion((*epp)->REGION) && (type & PRINT_LINKS) && i )
			putc('\n', PrStFd);
	}
}



/*
**	Print a region.
*/

static bool
PrStRegion(rp)
	register Region *	rp;
{
	register FILE *		fd = PrStFd;
	Time_t			date;
	char			datestr[DATESTRLEN];
	extern Time_t		Time;

	Trace2(2, "PrStRegion(%s)", (rp->rg_name[0]=='\0')?"* WORLD *":rp->rg_name);

	if ( rp->rg_links == (NLink *)0 && !(PrStType & (PRINT_VERBOSE|PRINT_ALL|PRINT_ROUTE)) )
		return false;

	Fprintf(fd, "%s\n", (rp->rg_name[0]=='\0')?"* WORLD *":rp->rg_name);

	if ( PrStType & PRINT_VERBOSE )
	{
		register Region *	rrp1;

		if ( (rrp1 = rp->rg_visible) != (Region *)0 )
			Fprintf(fd, english("\tVisible: %s\n"), rrp1->rg_name);

		if ( !(PrStType & PRINT_ROUTE) && (rrp1 = rp->rg_route[FASTEST]) != HomeRegion )
		{
			register Region *	rrp2;

			rrp2 = rp->rg_route[CHEAPEST];

			if ( rrp1 == rrp2 )
			{
				if ( rrp1 != (Region *)0 )
					Fprintf(fd, english("\tBoth routes: %s\n"), rrp1->rg_name);
			}
			else
			{
				if ( rrp1 != (Region *)0 )
					Fprintf(fd, english("\tFast  route: %s\n"), rrp1->rg_name);

				if ( rrp2 != (Region *)0 )
					Fprintf(fd, english("\tCheap route: %s\n"), rrp2->rg_name);
			}
		}

		if ( (PrStType & PRINT_HOME) && (rp->rg_flags & RF_HOME) )
			Fprintf(fd, english("\tHOME\n"));

		if ( rp->rg_flags & RF_FOREIGN )
			Fprintf(fd, english("\tFOREIGN\n"));

		if ( rp->rg_flags & RF_PSEUDO )
			Fprintf(fd, english("\tPSEUDO\n"));

		if ( (date = rp->rg_state) )
		{
			if ( date > Time )	/* Catch bad date changes */
				date = Time;
			Fprintf(fd, "\t\t%s\n", DateString(date, datestr));
		}

		if ( rp == HomeRegion && HomeComment != NULLSTR )
			PrStComment("\t\t", HomeComment);

		if ( rp->rg_flags & RF_LINK )
		{
			register Link *	lp = (Link *)(rp->rg_down);

			if ( lp->ln_flags & FL_LINKNAME )
				Fprintf(fd, english("\t\tName: \"%s\"\n"), lp->ln_name);
			if ( lp->ln_caller != NULLSTR && lp->ln_caller[0] != '\0' )
				Fprintf(fd, english("\t\tCaller: \"%s\"\n"), lp->ln_caller);
			if ( lp->ln_filter != NULLSTR && lp->ln_filter[0] != '\0' )
				Fprintf(fd, english("\t\tFilter: \"%s\"\n"), lp->ln_filter);
			if ( lp->ln_spooler != NULLSTR && lp->ln_spooler[0] != '\0' )
				Fprintf(fd, english("\t\tSpooler: \"%s\"\n"), lp->ln_spooler);
		}
	}

	if ( (PrStType & PRINT_LINKS) && rp->rg_links != (NLink *)0 )
	{
		register NLink *	lp;

		SortLinks(rp);

		for ( lp = rp->rg_links ; lp != (NLink *)0 ; lp = lp->nl_next )
		{
			char	tbuf[TIMESTRLEN];

			if ( lp->nl_flags & FL_NOLINK )
				continue;

			if ( !(PrStType & PRINT_VERBOSE) && (lp->nl_flags & FL_PSEUDO) )
				continue;

			Fprintf(fd, "\t-> %-*s", LINK_NAME_SIZE, lp->nl_to->rg_name);

			if ( PrStType & PRINT_VERBOSE )
			{
				register Ulong	l;
				register Flags	f;

				if ( (f = (lp->nl_flags & ~FL_INTERNAL)) )
				{
					putc(' ', fd);
					(void)PrintFlags(fd, f);
				}
				if ( (l = lp->nl_cost) )
					Fprintf(fd, english(" cost %lu"), (PUlong)l);
				if ( (l = lp->nl_delay) )
					Fprintf(fd, english(" delay %s"), TimeString(l, tbuf));
/*				if ( (l = lp->nl_speed) )
**					Fprintf(fd, english(" speed %lu"), (PUlong)l);
**				if ( (l = lp->nl_traffic) )
**					Fprintf(fd, english(" traffic %lu"), (PUlong)l);
*/				if ( lp->nl_restrict != (Region *)0 )
					Fprintf(fd, english(" restrict=\"%s\""), lp->nl_restrict->rg_name);
			}

			if ( PrStType & PRINT_ROUTE )
			{
				Ulong	cost;
				bool	fastest	= false;
				char	tbuf[TIMESTRLEN];

				Fprintf(fd, " [");
				if ( PrStType & PRINT_FASTEST )
				{
					cost = CostLink(lp, FASTEST);
					if ( DistAsTime )
						Fprintf(fd, "%s", TimeString(cost, tbuf));
					else
						Fprintf(fd, "%lu", (PUlong)cost);
					fastest = true;
				}
				if ( PrStType & PRINT_CHEAPEST )
				{
					if ( fastest )
						Fprintf(fd, "(F)] [");
					cost = CostLink(lp, CHEAPEST);
					if ( DistAsTime )
						Fprintf(fd, "%s", TimeString(cost, tbuf));
					else
						Fprintf(fd, "%lu", (PUlong)cost);
					if ( fastest )
						Fprintf(fd, "(C)");
				}
				putc(']', fd);
			}

			putc('\n', fd);
		}
	}

	if ( (PrStType & PRINT_ROUTE) && rp != HomeRegion )
		for ( PrStRoute = 0 ; PrStRoute < NROUTES ; PrStRoute++ )
		{
			if ( (((PrStRoute==FASTEST)?PRINT_FASTEST:PRINT_CHEAPEST) & PrStType) == 0 )
				continue;

			if ( rp->rg_flags & RF_PSEUDO )
				PrStPseudo = true;
			else
				PrStPseudo = false;

			PathRoutes(rp->rg_route[PrStRoute], PrStDist, (rp->rg_links==(NLink *)0)?true:false);
		}

	if ( PrStType & PRINT_ALIASES )
	{
		register Entry *	ep;

		if
		(
			(ep = Lookup(rp->rg_name, AliasHash)) != (Entry *)0
			&&
			ep->MAP != NULLSTR
		)
			Fprintf(fd, english("   ALIASED to %s\n"), ep->MAP);

		if
		(
			(ep = Lookup(rp->rg_name, RegMapHash)) != (Entry *)0
			&&
			ep->MAP != NULLSTR
		)
			Fprintf(fd, english("    MAPPED to %s\n"), ep->MAP);

		if
		(
			(ep = Lookup(rp->rg_name, AdrMapHash)) != (Entry *)0
			&&
			ep->MAP != NULLSTR
		)
			Fprintf(fd, english(" FORWARDED to %s\n"), ep->MAP);

		SearchMap(rp->rg_name, AliasHash);
		SearchMap(rp->rg_name, RegMapHash);
	}

	return true;
}



#ifdef	FORCE_PRINT_ROUTES

/*
**	Force region routing via advisory links.
**
**	Assumes list is in override order (next overrides previous).
*/

static void
PrForceRoutes()
{
	register AdvRoute *	arp;

	Trace1(1, "PrForceRoutes");

	for ( arp = AdvisoryRoutes ; arp != (AdvRoute *)0 ; arp = arp->ar_next )
		pr_force_routes(arp->ar_region, arp->ar_route[0], arp->ar_route[1]);
}



/*
**	Force routes for region and all sub-regions.
*/

static void
pr_force_routes(rp, route0, route1)
	register Region *	rp;
	register Region *	route0;
	register Region *	route1;
{
	Trace4(
		2, "pr_force_routes(%s, %s, %s)",
		rp->rg_name,
		(route0==(Region *)0) ? NullStr : route0->rg_name,
		(route1==(Region *)0) ? NullStr : route1->rg_name
	);

	if ( route0 != (Region *)0 )
		rp->rg_route[0] = route0->rg_route[0];
	if ( route1 != (Region *)0 )
		rp->rg_route[1] = route1->rg_route[1];

	for ( rp = rp->rg_down ; rp != (Region *)0 ; rp = rp->rg_next )
		if ( !(rp->rg_flags & RF_LINK) )
			pr_force_routes(rp, route0, route1);
}
#endif	/* FORCE_PRINT_ROUTES */



static void
PrStComment(indent, cp1)
	char *		indent;
	register char *	cp1;
{
	register FILE *	fd = PrStFd;
	register char *	cp2;
	register char *	lcp = SiteInfoNames[(int)si_licence].si_name;
	register int	len = SiteInfoNames[(int)si_licence].si_namelength;

	Trace3(2, "PrStComment(%s, %s)", indent, cp1);

	putc('\n', fd);

	for ( ;; )
	{
		if ( (cp2 = strchr(cp1, '\n')) != NULLSTR )
			*cp2 = '\0';
		if ( cp1 != cp2 && strncmp(cp1, lcp, len) != STREQUAL )
			Fprintf(fd, "%s%s\n", indent, cp1);
		if ( (cp1 = cp2) == NULLSTR )
			break;
		*cp1++ = '\n';
	}
}



/*
**	Called from PathRoutes to print a route.
*/

void
PrStDist(this_dist, prev_dist, name)
	Ulong	this_dist;
	Ulong	prev_dist;
	char *	name;
{
	char	buf[2*10+3+1];
	char	tbuf1[TIMESTRLEN];
	char	tbuf2[TIMESTRLEN];

	if ( name == (char *)0 )
	{
		Fprintf(PrStFd, "%*s\n", 2*10+3+1, english("[UNREACHABLE]"));
		return;
	}

	if ( PrStPseudo )
		(void)strcpy(buf, "[*;*]");
	else
	if ( DistAsTime )
		(void)sprintf(buf, "[%s;%7s]",
			TimeString(this_dist, tbuf1),
			TimeString(this_dist-prev_dist, tbuf2)
		);
	else
		(void)sprintf(buf, "[%lu;%5lu]",
			(PUlong)this_dist,
			(PUlong)(this_dist-prev_dist)
		);

	Fprintf(PrStFd, "%*s <- %s", (int)(sizeof buf), buf, name);

	if ( (PrStType & (PRINT_FASTEST|PRINT_CHEAPEST)) == (PRINT_FASTEST|PRINT_CHEAPEST) )
		Fprintf(PrStFd, " (%c)\n", (PrStRoute==FASTEST)?english('F'):english('C'));
	else
		putc('\n', PrStFd);
}



/*
**	Make a sorted list of entries from a hash table.
*/

static Index
PrMakeList(listp, table, oldcount)
	Entry ***		listp;
	Entry **		table;
	Index			oldcount;
{
	register Index		count;
	register Entry **	epp;
	register Entry **	end;
	Index			val;
	int			(*funcp)();

	Trace2(1, "PrMakeList(,,%lu)", (PUlong)oldcount);

	for ( count = 0, epp = table, end = &epp[HASH_SIZE] ; epp < end ; epp++ )
		if ( *epp != (Entry *)0 )
			count += PMLcountTree(*epp);

	if ( count == 0 )
		return count;

	if ( (epp = *listp) != (Entry **)0 && oldcount < count )
	{
		free((char *)*listp);
		epp = (Entry **)0;
	}

	if ( epp == (Entry **)0 )
		*listp = epp = (Entry **)Malloc((int)count * sizeof(Entry *));

	PList = PListp = epp;

	for ( epp = table ; epp < end ; epp++ )
		if ( *epp != (Entry *)0 )
			PMLfromTree(*epp);

	if ( (PrStType & (PRINT_ROUTE|PRINT_DSORT)) == (PRINT_ROUTE|PRINT_DSORT) )
		funcp = PMLcompare;
	else
		funcp = MLcompare;

	if ( (val = count) > 1 )
		qsort((char *)PList, (int)count, sizeof(Entry *), funcp);

	return val;
}



/*
**	Compare distance (UNREACHABLE==max.)
*/

int
PMLcompare(epp1, epp2)
	char *		epp1;
	char *		epp2;
{
	Region *	r1 = (*(Entry **)epp1)->REGION;
	Region *	r2 = (*(Entry **)epp2)->REGION;
	register Ulong	d1;
	register Ulong	d2;

	if ( r1->rg_route[PrStRoute] == (Region *)0 )
		d1 = MAX_ULONG;
	else
		d1 = r1->rg_dist[PrStRoute];

	if ( r2->rg_route[PrStRoute] == (Region *)0 )
		d2 = MAX_ULONG;
	else
		d2 = r2->rg_dist[PrStRoute];

	if ( d1 < d2 )
		return -1;
	if ( d1 > d2 )
		return 1;

	return strccmp((*(Entry **)epp1)->en_name, (*(Entry **)epp2)->en_name);
}



static Index
PMLcountTree(ep)
	register Entry *	ep;
{
	register Index		count;
	register Region *	rp;

	Trace2(4, "PMLcountTree(%s)", ep->en_name);

	if ( ep->en_great != (Entry *)0 )
		count = PMLcountTree(ep->en_great);
	else
		count = 0;

	if ( ep->en_less != (Entry *)0 )
		count += PMLcountTree(ep->en_less);

	if
	(
		(rp = ep->REGION) != (Region *)0
		&&
		(rp->rg_flags & PMLmatch)
	)
		++count;

	return count;
}



static void
PMLfromTree(ep)
	register Entry *	ep;
{
	register Region *	rp;

	if ( ep->en_great != (Entry *)0 )
		PMLfromTree(ep->en_great);

	if ( ep->en_less != (Entry *)0 )
		PMLfromTree(ep->en_less);

	if ( (rp = ep->REGION) == (Region *)0 )
		return;

	if ( !(rp->rg_flags & PMLmatch) )
		return;

	*PListp++ = ep;

	Trace2(3, "PMLfromTree(%s)", ep->en_name);
}



/*
**	Find any entries whose MAP matches "map".
*/

static void
SearchMap(map, table)
	char *			map;
	Entry **		table;
{
	register Entry **	epp;
	register Entry **	end;

	Trace3(1, "SearchMap(%s, %s)", map, table==AliasHash?"Aliases":"RegMap");

	if ( (Searchmap = map) == NULLSTR )
		return;

	Searchtype = (table==AliasHash)?english("  ALIASED"):english("   MAPPED");

	for ( epp = table, end = &epp[HASH_SIZE] ; epp < end ; epp++ )
		if ( *epp != (Entry *)0 )
			SearchTree(*epp);
}



static void
SearchTree(ep)
	register Entry *	ep;
{
	if ( ep->en_great != (Entry *)0 )
		SearchTree(ep->en_great);

	if ( ep->en_less != (Entry *)0 )
		SearchTree(ep->en_less);

	if ( ep->MAP != NULLSTR && strccmp(Searchmap, ep->MAP) == STREQUAL )
		Fprintf(PrStFd, english(" %s by %s\n"), Searchtype, ep->en_name);
}
