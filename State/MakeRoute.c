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
**	Make up route table in memory, and write it to RouteFile.
*/

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"


static Index	MaxRouteStrLength;
static int	Str_count;
static char **	Str_vec;
static Entry *	StringsHash[HASH_SIZE];

static void	MakeLinkVector _FA_((void));
static void	MakeMapTables _FA_((void));
static void	MakeNameVector _FA_((void));
static void	MakeRegionVector _FA_((void));
static void	MakeTNameVector _FA_((void));
static void	MakeTypeVector _FA_((void));
int		name_list_cmp _FA_((const void *, const void *));
static Index	routestr _FA_((char *));
static void	SortNameVector _FA_((void));

extern bool	StripList;



/*
**	Create routing tables in memory.
*/

void
MakeRoute()
{
	register int	bytes;

	Trace1(1, "MakeRoute()");

	/*
	**	Coalesce regions with same routes (FASTEST set in CheckState()).
	*/

	Paths(HomeRegion, (RFlags)0, CHEAPEST);

	ForceRoutes();
	(void)CoalesceRegion(TopRegion, (Region *)0, (Region *)0);
	RegionsCoalesced = true;
	
	/*
	**	Make sorted list of region names,
	**	and set rg_index for each region.
	*/

	RegionCount = MakeList(&RegionList, RegionHash, RegionCount);

	/*
	**	Make tree of types in TypeTree.
	*/

	BuildTypeTree();

	TypeEntries = MakeTypeTree(TypeTree, HomeRegion->rg_up);
	
	/*
	**	Make sorted list of mapped addresses.
	*/

	StripList = true;
	FwdMapCount = MakeList(&FwdMapList, FwdMapHash, FwdMapCount);
	StripList = false;

	if ( AdrMapList == (Entry **)0 )
		AdrMapCount = MakeList(&AdrMapList, AdrMapHash, AdrMapCount);
	if ( RegMapList == (Entry **)0 )
		RegMapCount = MakeList(&RegMapList, RegMapHash, RegMapCount);
	if ( TypMapList == (Entry **)0 )
		TypMapCount = MakeList(&TypMapList, TypMapHash, TypMapCount);

	/*
	**	Allocate enough memory to hold entire routing file - Strings.
	*/

	if ( RouteBase != NULLSTR )
		free(RouteBase);

	bytes = ((RegionCount * LinkCount) + 7) / 8;

	Route_size = ROUTE_HEADER_SIZE
			+ sizeof(Index) * MaxTypes
			+ sizeof(TypeTab) * TypeCount
			+ sizeof(TypeEnt) * TypeEntries
			+ sizeof(RLink) * LinkCount
			+ sizeof(RegionEnt) * RegionCount
			+ sizeof(MapEnt) * (RegionCount+FwdMapCount+AdrMapCount+RegMapCount+TypMapCount)
			+ sizeof(char) * bytes
			;

	RouteBase = Malloc((int)Route_size);

	/*
	**	Set routing table counts.
	*/

	MAXTYPES = MaxTypes;
	TYPE_COUNT = TypeCount;
	LINK_COUNT = LinkCount;
	TYPE_ENTRIES = TypeEntries;
	REGION_COUNT = RegionCount;
	FWDMAP_COUNT = FwdMapCount;
	ADRMAP_COUNT = AdrMapCount;
	REGMAP_COUNT = RegMapCount;
	TYPMAP_COUNT = TypMapCount;

	/*
	**	Set pointers to routing tables.
	*/

	TypeNameVector = (Index *)&RouteBase[ROUTE_HEADER_SIZE];
	TypeVector = (TypeTab *)&TypeNameVector[MaxTypes];
	TypeTables = (TypeEnt *)&TypeVector[TypeCount];
	LinkVector = (RLink *)&TypeTables[TypeEntries];
	RegionVector = (RegionEnt *)&LinkVector[LinkCount];
	NameVector = (MapEnt *)&RegionVector[RegionCount];
	FwdMapVector = (MapEnt *)&NameVector[RegionCount];
	AdrMapVector = (MapEnt *)&FwdMapVector[FwdMapCount];
	RegMapVector = (MapEnt *)&AdrMapVector[AdrMapCount];
	TypMapVector = (MapEnt *)&RegMapVector[RegMapCount];
	RegionTable = (char *)&TypMapVector[TypMapCount];

	/*
	**	Zero Tables
	*/

	if ( bytes )
		(void)strclr(RegionTable, bytes);

	/*
	**	Allocate enough memory to contain:-
	**	pointers to name and handlers for each link,
	**	pointers to name and addresses for each mapping,
	**	pointers to type and addresses for each region.
	*/

	Str_vec = (char **)Malloc
			  (
				sizeof(char *) *
				(int)(
					5 * LinkCount +
					2 * (FwdMapCount+AdrMapCount+RegMapCount+TypMapCount+TypeCount) +
					2 * RegionCount +
					MaxTypes +
					TypeEntries +
					MapEntries + 3
				     )
			  );

	Str_size = 1;
	Str_count = 0;

	/*
	**	Build tables.
	*/

	MakeLinkVector();
	MAXLINKLENGTH = MaxLinkLength = MaxRouteStrLength;

	MakeRegionVector();
	MAXSTRLENGTH = MaxStrLength = MaxRouteStrLength;

	HOME_INDEX = routestr(HomeName);
	VIS_INDEX = routestr(VisibleName);
#	if	CHECK_LICENCE
	SERIAL_INDEX = routestr(LicenceNumber);
#	else	/* CHECK_LICENCE */
	SERIAL_INDEX = NON_INDEX;
#	endif	/* CHECK_LICENCE */

	MakeNameVector();
	MakeTNameVector();
	MakeTypeVector();
	MakeMapTables();

	/*
	**	Make strings contiguous.
	*/

	{
		register char *	cp;
		register Index	i;

		Strings = cp = Malloc((int)Str_size+sizeof(Ulong));

		*cp = '\0';	/* First is empty string, last is sumcheck */

		for ( i = 0 ; i < Str_count ; i++ )
			cp = strcpyend(++cp, Str_vec[i]);
	}

	free((char *)Str_vec);

	SortNameVector();
}



/*
**	Set up link entry table.
*/

static void
MakeLinkVector()
{
	register RLink *	rlp;
	register Link *		lnp;
	register NLink *	nlp;
	register Index		savelength;

	Trace1(1, "MakeLinkVector()");

	for
	(
		nlp = HomeRegion->rg_links, rlp = LinkVector ;
		nlp != (NLink *)0 ;
		nlp = nlp->nl_next, rlp++
	)
	{
		if ( !(nlp->nl_to->rg_flags & RF_LINK) )
			continue;

		Trace2(2, "link %s", nlp->nl_to->rg_name);

		lnp = (Link *)(nlp->nl_to->rg_down);

		rlp->rl_rname = routestr(nlp->nl_to->rg_name);

		savelength = MaxRouteStrLength;

		rlp->rl_name = routestr(lnp->ln_name);

		rlp->rl_spooler = routestr(lnp->ln_spooler);
		rlp->rl_caller = routestr(lnp->ln_caller);
		rlp->rl_filter = routestr(lnp->ln_filter);

		MaxRouteStrLength = savelength;

		rlp->rl_flags = nlp->nl_flags & ~FL_INTERNAL;
		rlp->rl_delay = nlp->nl_delay;
	}
}



/*
**	Make a MapTable.
*/

static void
MakeMapTable(epp, mep, i)
	register Entry **	epp;
	register MapEnt *	mep;
	register Index		i;
{
	Trace4(2,
		"MakeMapTable(%s, %#lx, %lu)",
		epp==FwdMapList?"fwdmap":epp==AdrMapList?"adrmap":epp==RegMapList?"regmap":"typmap",
		(PUlong)mep,
		(PUlong)i
	);

	for ( ; i-- != 0 ; epp++, mep++ )
	{
		Trace3(3, "%s ==> %s", (*epp)->en_name, (*epp)->MAP);

		mep->me_name = routestr((*epp)->en_name);
		mep->me_index = routestr((*epp)->MAP);
	}
}



/*
**	Make MapTables.
*/

static void
MakeMapTables()
{
	Trace1(1, "MakeMapTables()");

	MakeMapTable(FwdMapList, FwdMapVector, FwdMapCount);
	MakeMapTable(AdrMapList, AdrMapVector, AdrMapCount);
	MakeMapTable(RegMapList, RegMapVector, RegMapCount);
	MakeMapTable(TypMapList, TypMapVector, TypMapCount);
}



/*
**	Set up NameVector from RegionList.
*/

static void
MakeNameVector()
{
	register Index		i;
	register MapEnt *	mep;
	register Entry **	epp;

	Trace1(1, "MakeNameVector()");

	for
	(
		epp = RegionList, mep = NameVector, i = RegionCount ;
		i-- != 0 ;
		epp++, mep++
	)
	{
		mep->me_name = routestr(StripTypes((*epp)->en_name));
		mep->me_index = (*epp)->REGION->rg_index;
	}
}



/*
**	Set up RegionVector and link Tables.
*/

static void
MakeRegionVector()
{
	register Index		l;
	register Index		k;
	register Index		j;
	register Index		i;
	register RegionEnt *	rep;
	register Region *	rp;
	register Entry **	epp;

	Trace1(1, "MakeRegionVector()");

	for
	(
		epp = RegionList, rep = RegionVector, i = RegionCount ;
		i-- != 0 ;
		epp++, rep++
	)
	{
		Trace1(3, (*epp)->en_name);

		rep->re_name = routestr((*epp)->en_name);

		if ( (rp = (*epp)->REGION) == HomeRegion )
		{
			rep->re_what = (What_t)wh_home;
			rep->re_route[0] = NON_INDEX;
			rep->re_route[1] = NON_INDEX;
		}
		else
		if ( rp->rg_route[0] == (Region *)0 || rp->rg_route[1] == (Region *)0 )
		{
			rep->re_what = (What_t)wh_none;
			rep->re_route[0] = NON_INDEX;
			rep->re_route[1] = NON_INDEX;
		}
		else
		if ( rp->rg_route[0] == HomeRegion )
		{
			rep->re_what = (What_t)wh_next;
			rep->re_route[0] = NON_INDEX;
			rep->re_route[1] = NON_INDEX;
		}
		else
		{
			if ( rp->rg_flags & RF_HOME )
				rep->re_what = (What_t)wh_lixt;
			else
				rep->re_what = (What_t)wh_link;

			rep->re_route[0] = ((Link *)(rp->rg_route[0]->rg_down))->ln_index;
			rep->re_route[1] = ((Link *)(rp->rg_route[1]->rg_down))->ln_index;
		}

		if ( rp->rg_flags & RF_DUPROUTE )
			rep->re_flags = FL_DUPROUTE;
		else
			rep->re_flags = 0;

		/*
		**	Set region reachable table.
		*/

		k = rp->rg_number * LinkCount;
		l = rp->rg_index * LinkCount;

		for ( j = LinkCount ; j-- != 0 ; k++, l++ )
			if ( AllRegTable[k/8] & (1 << (k%8)) )
				RegionTable[l/8] |= 1 << (l%8);
	}
}



/*
**	Set up TypeNameVector.
*/

static void
MakeTNameVector()
{
	register char **cpp;
	register Index *tnp;
	register int	i;

	Trace1(1, "MakeTNameVector()");

	for ( tnp = TypeNameVector, cpp = OrderTypes, i = MaxTypes ; --i >= 0 ; )
		*tnp++ = routestr(*cpp++);
}



/*
**	Set up TypeVector and TypeTables.
*/

static void
MakeTypeVector()
{
	register Region *	rp;
	register TypeEl *	tlp;
	register TypeVec *	tvp;
	register TypeTab *	ttp;
	register TypeEnt *	tep;
	register Index		j;
	register Index		i;

	Trace1(1, "MakeTypeVector()");

	for
	(
		ttp = TypeVector, tep = TypeTables, i = TypeCount ;
		i-- != 0 ;
		ttp++
	)
	{
		tvp = &TypeTree[i];

		Trace3(2, "%s=%s", tvp->tv_type, tvp->tv_name);

		ttp->tp_count = j = tvp->tv_count;
		ttp->tp_index = tep - TypeTables;
		ttp->tp_name = routestr(tvp->tv_name);
		ttp->tp_type[0] = tvp->tv_type[0];
		ttp->tp_type[1] = '\0';

		for ( tlp = tvp->tv_sorted ; j-- != 0 ; tep++, tlp++ )
		{
			Trace1(3, tlp->tl_name);

			tep->te_name = routestr(tlp->tl_name);

#			ifdef	ENUM_NOT_INT
			if ( (tep->te_what = tlp->tl_what) == (What_t)wh_map )
#			else	/* ENUM_NOT_INT */
			if ( (tep->te_what = (What_t)tlp->tl_what) == (What_t)wh_map )
#			endif	/* ENUM_NOT_INT */
			{
				tep->te_type = tvp->tv_type[0];
				tep->te_index = routestr(tlp->TMAP);
			}
			else
			{
				rp = tlp->TREGION;
				tep->te_index = rp->rg_index;	/* In RegionTable */

				if ( tlp->tl_what == wh_none )
					tep->te_type = tvp->tv_type[0];
				else
					tep->te_type = rp->rg_name[0];

				if ( rp == HomeRegion )
					tep->te_what = (What_t)wh_home;
				else
				if ( rp->rg_route[0] == (Region *)0 || rp->rg_route[1] == (Region *)0 )
					tep->te_what = (What_t)wh_none;
				else
				if ( rp->rg_route[0] == HomeRegion )
					tep->te_what = (What_t)wh_next;
				else
				if ( rp->rg_flags & RF_HOME )
					tep->te_what = (What_t)wh_lixt;
				else
					tep->te_what = (What_t)wh_link;
			}
		}

		free((char *)tvp->tv_sorted);
	}
}



int
#if	ANSI_C
name_list_cmp(const void *nvp1, const void *nvp2)
#else	/* ANSI_C */
name_list_cmp(nvp1, nvp2)
	char *		nvp1;
	char *		nvp2;
#endif	/* ANSI_C */
{
	register int	val;

	val = strccmp
		(
			&Strings[(int)((MapEnt *)nvp1)->me_name],
			&Strings[(int)((MapEnt *)nvp2)->me_name]
		);

	if ( val == STREQUAL )
		Warn
		(
			english("Identical names in typeless region vector \"%s\""),
			&Strings[(int)((MapEnt *)nvp1)->me_name]
		);

	return val;
}



/*
**	Add new string to Strings, and return Index.
*/

static Index
routestr(acp)
	char *		acp;
{
	register char *	cp;
	register Entry *ep;
	register Index	len;

	if ( ((cp = acp) == NULLSTR) || *cp == '\0' )
		return 0;

	if ( (ep = EnterC(cp, StringsHash))->INDEX == 0 )
	{
		Str_vec[Str_count++] = cp;
		ep->INDEX = Str_size;
		if ( (len = strlen(cp)) > MaxRouteStrLength )
			MaxRouteStrLength = len;
		Str_size += len+1;
	}

	return ep->INDEX;
}



/*
**	Sort NameVector from Strings.
*/

static void
SortNameVector()
{
	Trace1(1, "SortNameVector()");

	qsort((char *)NameVector, (int)RegionCount, sizeof(MapEnt), name_list_cmp);
}
