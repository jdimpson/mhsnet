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
#include	"address.h"
#include	"link.h"
#include	"routefile.h"


bool		NeedRemap;	/* Explicit address needs remapping */
static bool	Removed;	/* Address component removed */

extern int	MinTypC;
extern int	MaxTypC;

static What	matchtype _FA_((TypeTab *, Dname *, int));
static MapEnt *	SearchMapTab _FA_((MapEnt *, Index, char *));
static RegionEnt *SearchRegions _FA_((char *));
static TypeEnt *SearchTypeTab _FA_((TypeEnt *, Index, char *));
static void	SetLink _FA_((Link *, RegionEnt *, int));



/*
**	Return true if address contains a home destination.
*/

bool
AddrAtHome(app, remove, follow)
	Addr **		app;
	bool		remove;
	bool		follow;
{
	register Addr *	ap;
	register bool	val;

	Trace4(2, "AddrAtHome(%s, %d, %d)", ADDRNAME(*app), remove, follow);

	Removed = false;

	if ( (ap = *app) == (Addr *)0 )
		return false;

	switch ( ADDRTYPE(ap) )
	{
	case ATYP_DESTINATION:
		if ( FindDest(ADDRDEST(ap), (Link *)0, FASTEST) == wh_home )
		{
			if ( remove )
			{
				RemoveAddr(app);
				Removed = true;
			}

			Trace4(1, "AddrAtHome(%s, %d, %d) ==> TRUE", ADDRNAME(*app), remove, follow);
			return true;
		}

		return false;

	case ATYP_BROADCAST:
		return AddrAtHome(&ap->ad_down, false, follow);

	case ATYP_EXPLICIT:
		while ( ap->ad_next != (Addr *)0 )
		{
			if ( follow && (ap->ad_flags & AD_FORWARDED) )
				follow = false;

			if
			(
				ap->ad_down != (Addr *)0
				&&
				!follow
				&&
				!AddrAtHome(&ap->ad_down, remove, false)
			)
				return false;

			if ( Removed )
				NeedRemap = true;

			ap = ap->ad_next;
		}

		return AddrAtHome(&ap->ad_down, remove, follow);

	case ATYP_MULTICAST:
		val = false;
		do
			if ( AddrAtHome(&ap->ad_down, remove, follow) )
			{
				if ( !remove )
					return true;
				val = true;
			}
		while
			( (ap = ap->ad_next) != (Addr *)0 );

		return val;
	}

	return false;
}



/*
**	Return true if address contains broadcast.
*/

bool
AddrIsBC(aa)
	Addr *		aa;
{
	register Addr *	ap;

	Trace2(2, "AddrIsBC(%s)", ADDRNAME(aa));

	if ( (ap = aa) == (Addr *)0 )
		return false;

	switch ( ADDRTYPE(ap) )
	{
	case ATYP_DESTINATION:
		break;

	case ATYP_BROADCAST:
		Trace2(1, "AddrIsBC(%s) ==> TRUE", ADDRNAME(ap));
		return true;

	case ATYP_EXPLICIT:
		do
		{
			if ( AddrIsBC(ap->ad_down) )
				return true;
			if ( !AddrAtHome(&ap->ad_down, false, false) )
				break;
		}
		while
			( (ap = ap->ad_next) != (Addr *)0 );
		break;

	case ATYP_MULTICAST:
		do
			if ( AddrIsBC(ap->ad_down) )
				return true;
		while
			( (ap = ap->ad_next) != (Addr *)0 );
	}

	return false;
}



/*
**	Convert internal type to exportable type name.
*/

char *
ExportType(t)
	int	t;
{
	t -= '0';
	DODEBUG(if(t<0||t>=MaxTypes)Fatal("ExportType(%d)",t));
	Trace3(5, "ExportType(%c) ==> %s", t+'0', &Strings[TypeNameVector[t]]);
	return &Strings[TypeNameVector[t]];
}



/*
**	Find (Addr *) in routing tables, if it represents a single destination.
*/

What
FindAddr(aa, lp, route)
	Addr *		aa;
	Link *		lp;
	int		route;
{
	register Addr *	ap;

	Trace4(2, "FindAddr(%s, %#lx, %d)", ADDRNAME(aa), (PUlong)lp, route);

	if ( (ap = aa) == (Addr *)0 )
		return wh_none;

	if ( EXPLTYPE(ap) )
	{
		while ( ap->ad_down == (Addr *)0 || AddrAtHome(&ap->ad_down, false, false) )
			if ( (ap = ap->ad_next) == (Addr *)0 )
				return wh_none;
		ap = ap->ad_down;
	}

	if ( !DESTTYPE(ap) )
		return wh_none;

	return FindDest(ADDRDEST(ap), lp, route);
}



/*
**	Find (Dest *) in routing tables.
*/

What
FindDest(dp, lp, route)
	Dest *			dp;
	Link *			lp;
	int			route;
{
	register char *		tp;
	register char *		np;
	register RegionEnt *	rep;
	register TypeEnt *	tep;
	register TypeTab *	ttp;
	register MapEnt *	mep;
	register int		i;
	register What		result;

	Trace4(3, "FindDest(%s, %#lx, %d)", DESTNAME(dp), (PUlong)lp, route);

	if ( RouteBase == NULLSTR && !ReadRoute(NOSUMCHECK) )
		return wh_noroute;

	result = wh_none;

	/*
	**	Try for region match.
	*/

	for
	(
		np = dp->dt_name - 1, tp = dp->dt_tname - 1 ;
		np != NULLSTR ;
		np = strchr(np, DOMAIN_SEP), tp = strchr(tp, DOMAIN_SEP)
	)
		if
		(
			(rep = SearchRegions(++tp)) != (RegionEnt *)0
			||
			(
				(mep = SearchMapTab(NameVector, RegionCount, ++np)) != (MapEnt *)0
				&&
				(rep = &RegionVector[mep->me_index]) != (RegionEnt *)0
			)
		)
		{
			switch ( result = (What)rep->re_what )
			{
			case wh_next:	result = wh_home;
			case wh_home:	if ( tp != dp->dt_tname )
						break;
					SetLink(lp, rep, route);
					return result;

			case wh_lixt:	result = wh_link;
			case wh_link:	SetLink(lp, rep, route);
					return result;
			default:	break;		/* for gcc -Wall */
			}

			break;
		}

	if ( result != wh_none || dp->dt_count <= 1 )
		return wh_none;

	/*
	**	Try forwarding.
	*/

	if
	(
		RegionVector->re_what == (What_t)wh_lixt
		&&
		Strings[RegionVector->re_name] == '\0'	/* *WORLD* */
	)
	{
		Trace2(4, "FindDest(%s) forwarded to WORLD", DESTNAME(dp));

		if ( lp != (Link *)0 )
		{
			SetLink(lp, RegionVector, route);
			lp->ln_flags |= FL_FORWARD;
		}
		return wh_link;
	}

	for ( i = TypeCount, ttp = TypeVector ; --i >= 0 ; ttp++ )
		if
		(
			(tep = SearchTypeTab(&TypeTables[ttp->tp_index], ttp->tp_count, &Strings[ttp->tp_name])) != (TypeEnt *)0
			&&
			tep->te_what == (What_t)wh_lixt
		)
		{
			Trace3(
				4,
				"FindDest(%s) forwarded to %s",
				DESTNAME(dp),
				&Strings[LinkVector[RegionVector[tep->te_index].re_route[route]].rl_rname]
			);

			if ( lp != (Link *)0 )
			{
				SetLink(lp, &RegionVector[tep->te_index], route);
				lp->ln_flags |= FL_FORWARD;
			}
			return wh_link;
		}

	return wh_none;
}



/*
**	Find a real link in sorted LinkVector, given typed name.
*/

bool
FindLink(rname, lp)
	char *			rname;	/* Typed name */
	Link *			lp;	/* Result */
{
	register char *		cp1;	/* String comparison temporary */
	register char *		cp2;	/* String comparison temporary */
	register Index		n;	/* Number of entries in partition */
	register RLink *	f;	/* First entry in partition */
	register RLink *	l;	/* Last entry in partition */
	register RLink *	m;	/* Approximate middle entry in partition */

	if ( lp != (Link *)0 )
	{
		(void)strclr((char *)lp, sizeof(Link));
		lp->ln_index = NON_INDEX;
	}

	if ( RouteBase == NULLSTR && !ReadRoute(NOSUMCHECK) )
		return false;

	Trace4(2, "FindLink(%s, %#lx) [%lu links]", rname, (PUlong)lp, (PUlong)LinkCount);

	if ( rname[0] == ATYP_DESTINATION )
		rname++;

	if ( (n = LinkCount) == 0 )
		return false;
	f = LinkVector;
	l = &f[--n];

	while ( l >= f )
	{
		m = &f[n/2];

		Trace3(5, "FindLink[%lu] => \"%s\"", (PUlong)n, &Strings[m->rl_rname]);

		for ( cp1 = rname, cp2 = &Strings[m->rl_rname] ; (n = *cp1++) ; )
			if ( (n ^= *cp2++) && n != 040 )
				break;

		if ( n == 0 && *cp2++ == '\0' )
		{
			GetLink(lp, (Index)(m - LinkVector));
			return true;
		}

		if ( (*--cp1|040) < (*--cp2|040) )
			l = m - 1;
		else
			f = m + 1;

		n = l - f;
	}

	return false;
}



/*
**	Lookup name in region forwarding table and return match.
*/

char *
FwdDest(name)
	char *			name;
{
	register MapEnt *	mep;
	register char *		cp;

	Trace2(4, "FwdDest(%s)", name);

	if ( (RouteBase == NULLSTR && !ReadRoute(NOSUMCHECK)) || FwdMapCount == 0 )
		return NULLSTR;

	for
	(
		cp = name-1 ;
		cp != NULLSTR ;
		cp = strchr(cp, DOMAIN_SEP)
	)
	{
		cp++;

		if ( (mep = SearchMapTab(FwdMapVector, FwdMapCount, cp)) != (MapEnt *)0 )
		{
			Trace4(3, "FwdDest(%s) (on \"%s\") ==> \"%s\"", name, cp, &Strings[mep->me_index]);
			return &Strings[mep->me_index];
		}

		if ( SearchMapTab(NameVector, RegionCount, cp) != (MapEnt *)0 )
			return NULLSTR;
	}

	if ( Strings[FwdMapVector->me_name] == '\0' )	/* *WORLD* */
		return &Strings[FwdMapVector->me_index];

	return NULLSTR;
}



/*
**	Set up Link from LinkVector given link index.
*/

void
GetLink(alp, index)
	Link *			alp;
	Index			index;
{
	register Link *		lp;
	register RLink *	rlp;
	register Index		i;

	if ( (lp = alp) == (Link *)0 )
		return;

	rlp = &LinkVector[index];

	lp->ln_flags = rlp->rl_flags;
	lp->ln_index = index;
	lp->ln_name = &Strings[rlp->rl_name];
	lp->ln_regindex = NON_INDEX;
	lp->ln_region = NULLSTR;
	lp->ln_rname = &Strings[rlp->rl_rname];
	lp->ln_delay = rlp->rl_delay;

	if ( (i = rlp->rl_caller) )
		lp->ln_caller = &Strings[i];
	else
		lp->ln_caller = NULLSTR;
	if ( (i = rlp->rl_filter) )
		lp->ln_filter = &Strings[i];
	else
		lp->ln_filter = NULLSTR;
	if ( (i = rlp->rl_spooler) )
		lp->ln_spooler = &Strings[i];
	else
		lp->ln_spooler = NULLSTR;

	Trace3(3, "GetLink(%s, %lu)", lp->ln_name, (PUlong)index);
}



/*
**	Lookup name in address mapping table and return match.
*/

char *
MapDest(name)
	char *			name;
{
	register MapEnt *	mep;

	Trace2(4, "MapDest(%s)", name);

	if ( (RouteBase == NULLSTR && !ReadRoute(NOSUMCHECK)) || AdrMapCount == 0 )
		return NULLSTR;

	if ( (mep = SearchMapTab(AdrMapVector, AdrMapCount, name)) != (MapEnt *)0 )
	{
		Trace3(3, "MapDest(%s) ==> \"%s\"", name, &Strings[mep->me_index]);
		return &Strings[mep->me_index];
	}

	return NULLSTR;
}



/*
**	Lookup name in region mapping table and return match.
*/

char *
MapRegion(name)
	char *			name;
{
	register MapEnt *	mep;

	Trace2(4, "MapRegion(%s)", name);

	if ( (RouteBase == NULLSTR && !ReadRoute(NOSUMCHECK)) || RegMapCount == 0 )
		return NULLSTR;

	if ( (mep = SearchMapTab(RegMapVector, RegMapCount, name)) != (MapEnt *)0 )
	{
		Trace3(3, "MapRegion(%s) ==> \"%s\"", name, &Strings[mep->me_index]);
		return &Strings[mep->me_index];
	}

	return NULLSTR;
}



/*
**	Lookup type in type mapping table and return match.
*/

char *
MapType(type)
	char *			type;
{
	register MapEnt *	mep;

	Trace2(4, "MapType(%s)", type);

	if ( (RouteBase == NULLSTR && !ReadRoute(NOSUMCHECK)) || TypMapCount == 0 )
		return NULLSTR;

	if ( (mep = SearchMapTab(TypMapVector, TypMapCount, type)) != (MapEnt *)0 )
	{
		Trace3(3, "MapType(%s) ==> \"%s\"", type, &Strings[mep->me_index]);
		return &Strings[mep->me_index];
	}

	return NULLSTR;
}



/*
**	Search type table for name (and do any mapping).
**
*/

static What
matchtype(tp, np, left)
	register TypeTab *	tp;
	register Dname *	np;
	int			left;
{
	register TypeEnt *	tep;
	register int		loop_count = 0;

	Trace5(
		4,
		"matchtype(%s=%s, %c=%s)",
		tp->tp_type,
		&Strings[tp->tp_name],
		(np->dn_type=='\0')?'?':np->dn_type,
		np->dn_name
	);

	while ( (tep = SearchTypeTab(&TypeTables[tp->tp_index], tp->tp_count, np->dn_name)) != (TypeEnt *)0 )
	{
		if ( np->dn_type != tep->te_type && np->dn_type != '\0' )
			return wh_none;

		if ( tep->te_type > (MaxTypC - left) )
			return wh_none;

		if ( tep->te_what == (What_t)wh_map )
		{
			if ( ++loop_count > tp->tp_count )
			{
				Error(english("Type %d: mapping loop for name \"%s\""), tp->tp_type, np->dn_name);
				return wh_noroute;
			}
			Trace3(2, "\"%s\" MAPPED -> \"%s\"", np->dn_name, &Strings[tep->te_index]);
			np->dn_name = &Strings[tep->te_index];
			continue;
		}

		np->dn_type = tep->te_type;
		Trace3(4, "patch type %c=%s", np->dn_type, np->dn_name);
		return (What)tep->te_what;
	}

	if
	(
		(np->dn_type == tp->tp_type[0] || np->dn_type == '\0')
		&&
		strccmp(&Strings[tp->tp_name], np->dn_name) == STREQUAL
	)
	{
		np->dn_type = tp->tp_type[0];
		Trace3(4, "patch type %c=%s", np->dn_type, np->dn_name);
		return wh_next;
	}

	return wh_none;
}



/*
**	Fill missing types from routing tables.
*/

void
MatchTypes(dp)
	register Dest *		dp;
{
	register int		j;
	register TypeTab *	tp;
	register Dname *	np;
	register int		i;
	register TypeTab *	ptp;
	register Dname *	pnp;
	register int		last_type;

	Trace2(3, "MatchTypes(%s)", DESTNAME(dp));

	if ( RouteBase == NULLSTR && !ReadRoute(NOSUMCHECK) )
		return;

	ptp = (TypeTab *)0;
	pnp = (Dname *)0;

	i = dp->dt_count;
	np = &dp->dt_names[i-1];
	last_type = np->dn_type;

	if ( NOADDRCOMPL && last_type == '\0' )
		np->dn_type = last_type = MinTypC;

	for
	(
		j = TypeCount, tp = TypeVector ;
		--i >= 0 ;
		tp++, --np
	)
	{
		/** Linear search of ordered types **/

		for ( ; --j >= 0 ; tp++ )
		{
			Trace3(5, "match type '%c' > '%s'", (np->dn_type=='\0')?'?':np->dn_type, tp->tp_type);

			/** Binary search of sorted names **/

			if ( np->dn_type == '\0' || np->dn_type == tp->tp_type[0] )
			switch ( matchtype(tp, np, i) )
			{
			default:				return;
			case wh_home:
			case wh_link:	ptp = tp; pnp = np;	goto out;
			case wh_lixt:
			case wh_next:	ptp = tp; pnp = np;	goto loop;
			case wh_none:				break;
			}
		}

		break;
loop:;	}

out:	if
	(
		(tp = ptp) == (TypeTab *)0
		||
		(NOADDRCOMPL && last_type == MinTypC)
	)
		return;

	np = pnp;

	j = ++np - dp->dt_names;

	if ( (i = dp->dt_count) < MAXPARTS )
		i = MAXPARTS;

	while ( ++j <= i && --tp >= TypeVector )
	{
		Trace3(4, "patch type %s=%s", tp->tp_type, &Strings[tp->tp_name]);
		np->dn_type = tp->tp_type[0];
		np->dn_name = &Strings[tp->tp_name];
		np->dn_tname = NULLSTR;
		np++;
	}

	dp->dt_count = --j;
}



/*
**	Use binary search to find name in map table.
*/

static MapEnt *
#ifdef	ANSI_C
SearchMapTab(MapEnt *fe, Index count, char *name)
#else	/* ANSI_C */
SearchMapTab(fe, count, name)
	MapEnt *		fe;	/* First entry in table */
	Index			count;	/* Number of entries in table */
	char *			name;	/* Name to be matched */
#endif	/* ANSI_C */
{
	register char *		cp1;	/* String comparison temporary */
	register char *		cp2;	/* String comparison temporary */
	register Index		n;	/* Number of entries in partition */
	register MapEnt *	f;	/* First entry in partition */
	register MapEnt *	l;	/* Last entry in partition */
	register MapEnt *	m;	/* Approximate middle entry in partition */

	Trace4(4, "SearchMapTab(%s, %lu, %s)", count?&Strings[fe->me_name]:NullStr, (PUlong)count, name);

	if ( (n = count) == 0 )
		return (MapEnt *)0;
	f = fe;
	l = &f[--n];

	while ( l >= f )
	{
		m = &f[n/2];

		Trace3(5, "SearchMapTab[%lu] => \"%s\"", (PUlong)n, &Strings[m->me_name]);

		for ( cp1 = name, cp2 = &Strings[m->me_name] ; (n = *cp1++) ; )
			if ( (n ^= *cp2++) && n != 040 )
				break;

		if ( n == 0 && *cp2++ == '\0' )
			return m;

		if ( (*--cp1|040) < (*--cp2|040) )
			l = m - 1;
		else
			f = m + 1;

		n = l - f;
	}

	return (MapEnt *)0;
}



/*
**	Find name in names table.
*/

char *
SearchNames(name)
	char *			name;	/* Name to be matched */
{
	register MapEnt *	mep;

	Trace2(4, "SearchNames(%s)", name);

	if ( (mep = SearchMapTab(NameVector, RegionCount, name)) == (MapEnt *)0 )
		return NULLSTR;

	return &Strings[RegionVector[mep->me_index].re_name];
}



/*
**	Use binary search to find name in regions table.
*/

static RegionEnt *
SearchRegions(name)
	char *			name;	/* Name to be matched */
{
	register char *		cp1;	/* String comparison temporary */
	register char *		cp2;	/* String comparison temporary */
	register Index		n;	/* Number of entries in partition */
	register RegionEnt *	f;	/* First entry in partition */
	register RegionEnt *	l;	/* Last entry in partition */
	register RegionEnt *	m;	/* Approximate middle entry in partition */

	Trace2(4, "SearchRegions(%s)", name);

	if ( (n = RegionCount) == 0 )
		return (RegionEnt *)0;
	f = RegionVector;
	l = &f[--n];

	while ( l >= f )
	{
		m = &f[n/2];

		Trace3(5, "SearchRegions[%lu] => \"%s\"", (PUlong)n, &Strings[m->re_name]);

		for ( cp1 = name, cp2 = &Strings[m->re_name] ; (n = *cp1++) ; )
			if ( (n ^= *cp2++) && n != 040 )
				break;

		if ( n == 0 && *cp2++ == '\0' )
			return m;

		if ( (*--cp1|040) < (*--cp2|040) )
			l = m - 1;
		else
			f = m + 1;

		n = l - f;
	}

	return (RegionEnt *)0;
}



/*
**	Use binary search to find name in type table.
*/

static TypeEnt *
#ifdef	ANSI_C
SearchTypeTab(TypeEnt *fe, Index count, char *name)
#else	/* ANSI_C */
SearchTypeTab(fe, count, name)
	TypeEnt *		fe;	/* First entry in table */
	Index			count;	/* Number of entries in table */
	char *			name;	/* Name to be matched */
#endif	/* ANSI_C */
{
	register char *		cp1;	/* String comparison temporary */
	register char *		cp2;	/* String comparison temporary */
	register Index		n;	/* Number of entries in partition */
	register TypeEnt *	f;	/* First entry in partition */
	register TypeEnt *	l;	/* Last entry in partition */
	register TypeEnt *	m;	/* Approximate middle entry in partition */

	Trace4(4, "SearchTypeTab(%s, %lu, %s)", count?&Strings[fe->te_name]:NullStr, (PUlong)count, name);

	if ( (n = count) == 0 )
		return (TypeEnt *)0;
	f = fe;
	l = &f[--n];

	while ( l >= f )
	{
		m = &f[n/2];

		Trace3(5, "SearchTypeTab[%lu] => \"%s\"", (PUlong)n, &Strings[m->te_name]);

		for ( cp1 = name, cp2 = &Strings[m->te_name] ; (n = *cp1++) ; )
			if ( (n ^= *cp2++) && n != 040 )
				break;

		if ( n == 0 && *cp2++ == '\0' )
			return m;

		if ( (*--cp1|040) < (*--cp2|040) )
			l = m - 1;
		else
			f = m + 1;

		n = l - f;
	}

	return (TypeEnt *)0;
}



/*
**	Setup Link from LinkVector given region index.
*/

static void
SetLink(alp, rep, route)
	Link *			alp;
	register RegionEnt *	rep;
	int			route;
{
	register RLink *	rlp;
	register Link *		lp;
	register Index		i;

	Trace3(4, "SetLink(%#lx, %s)", (PUlong)alp, &Strings[rep->re_name]);

	if ( (lp = alp) == (Link *)0 )
		return;

	if ( rep->re_what == (What_t)wh_home || rep->re_what == (What_t)wh_next )
	{
		lp->ln_flags = FL_HOME | rep->re_flags;
		lp->ln_index = NON_INDEX;
		lp->ln_name = NULLSTR;
		lp->ln_rname = NULLSTR;
		lp->ln_regindex = rep - RegionVector;
		lp->ln_region = &Strings[rep->re_name];

		lp->ln_caller = NULLSTR;
		lp->ln_filter = NULLSTR;
		lp->ln_spooler = NULLSTR;
		lp->ln_delay = (Ulong)0;

		Trace2(3, "SetLink(%s) --> HOME", lp->ln_region);
		return;
	}

	if ( (i = rep->re_route[route]) == NON_INDEX )
		Fatal1("SetLink called with non-link");

	rlp = &LinkVector[i];

	lp->ln_flags = rlp->rl_flags | rep->re_flags;
	lp->ln_index = i;
	lp->ln_name = &Strings[rlp->rl_name];
	lp->ln_regindex = rep - RegionVector;
	lp->ln_region = &Strings[rep->re_name];
	lp->ln_rname = &Strings[rlp->rl_rname];
	lp->ln_delay = rlp->rl_delay;

	if ( (i = rlp->rl_caller) )
		lp->ln_caller = &Strings[i];
	else
		lp->ln_caller = NULLSTR;
	if ( (i = rlp->rl_filter) )
		lp->ln_filter = &Strings[i];
	else
		lp->ln_filter = NULLSTR;
	if ( (i = rlp->rl_spooler) )
		lp->ln_spooler = &Strings[i];
	else
		lp->ln_spooler = NULLSTR;

	Trace5(3, "SetLink(%s) --> %s \"%s\" [%d]", lp->ln_region, lp->ln_rname, lp->ln_name, lp->ln_index);
}
