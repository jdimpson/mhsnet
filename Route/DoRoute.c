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
#include	"address.h"
#include	"debug.h"
#include	"header.h"
#include	"link.h"
#include	"routefile.h"


/*
**	Declare structures for making multicast disjoint sets.
*/

typedef struct
{
	Addr *	ls_addr;
	Link	ls_link;
}
		LinkSet;

static LinkSet *LinkSets;		/* One for each possible link */

/*
**	Miscellaneous.
*/

static Link	DestLink;
static Addr *	ErrDest;
static DR_ep	Errfuncp;
static Index	LastLinkCount;
static Link *	Linkp;
static Addr *	Source;
static int	Type;

static bool	broadcast _FA_((Addr *, Addr **));
static bool	route _FA_((Addr **, Addr **));



/*
**	Applies routing algorithm to each element of address in 'dest',
**	and for each outgoing link found builds a new destination address
**	and calls '*funcp' with the destination, source and link details
**	as arguments.
**
**	If any address error is encountered, '*errfuncp' is called with
**	the source address and offending destination address as arguments.
**
**	Returns true if '*funcp' was called, otherwise false.
**
**	Assumes `HdrRoute' is valid, and searches it to avoid broadcast
**	loops, and to choose alternate routes to avoid revisiting links.
*/

bool
DoRoute(source, destp, type, link, funcp, errfuncp)
	Addr *	source;		/* The source address */
	Addr **	destp;		/* The destination address */
	int	type;		/* FASTEST or CHEAPEST */
	Link *	link;		/* The incoming link address */
	DR_fp	funcp;		/* Function to send message on link */
	DR_ep	errfuncp;	/* Address error function */
{
	register LinkSet *	lsp;
	register int		i;
	bool			val;

	Trace5(
		1,
		"DoRoute source \"%s\", dest \"%s\", %s, link \"%s\"",
		UnTypAddress(source),
		UnTypAddress(*destp),
		(type==FASTEST)?"FAST":(type==CHEAPEST)?"CHEAP":"????",
		(link!=(Link *)0)?link->ln_rname:NullStr
	);

	if ( RouteBase == NULLSTR && !ReadRoute(NOSUMCHECK) )
	{
		/*
		**	No route file!
		*/

		Error(english("no routing tables"));

		return false;
	}

	/*
	**	Need to make disjoint sets of nodes with common
	**	shortest paths from here.
	*/

	if ( (lsp = LinkSets) != (LinkSet *)0 && LastLinkCount < LinkCount )
	{
		free((char *)lsp);
		lsp = (LinkSet *)0;
	}

	if ( lsp == (LinkSet *)0 && LinkCount > 0 )
	{
		LinkSets = lsp = (LinkSet *)Malloc((int)LinkCount * sizeof(LinkSet));
		LastLinkCount = LinkCount;
	}

	for ( i = 0 ; i < LinkCount ; i++, lsp++ )
	{
		lsp->ls_addr = (Addr *)0;
		lsp->ls_link.ln_name = NULLSTR;
	}

	/*
	**	Parse the address into link sets.
	*/

	Errfuncp = errfuncp;
	Linkp = link;
	Source = source;
	Type = type;

	if ( !route(destp, destp) )
		(*Errfuncp)(Source, ErrDest);

	/*
	**	Call function for each outgoing link.
	*/

	val = false;

	for ( lsp = LinkSets, i = 0 ; i < LinkCount ; i++, lsp++ )
	{
		if ( lsp->ls_addr == (Addr *)0 )
			continue;

		Trace3(
			2,
			"Multi-cast subset \"%s\" on link \"%s\"",
			UnTypAddress(lsp->ls_addr),
			lsp->ls_link.ln_name
		);

		(*funcp)(Source, lsp->ls_addr, &lsp->ls_link);

		FreeAddr(&lsp->ls_addr);

		val = true;
	}

	return val;
}



/*
**	Parse the address.
**	Add each element into appropriate linkset.
*/

static bool
route(destp, move_addr)
	Addr **			destp;
	Addr **			move_addr;
{
	register Addr *		ap;
	register LinkSet *	lsp;
	register int		type;
	register int		count;

	Trace3(
		2, "route(%s, %s)",
		UnTypAddress(*destp),
		(move_addr==(Addr **)0)?NullStr:UnTypAddress(*move_addr)
	);

	if ( (ap = *destp) == (Addr *)0 )
		return true;

	while ( ap->ad_down == (Addr *)0 )
		if ( (ap = ap->ad_next) == (Addr *)0 )
			return true;

	switch ( ADDRTYPE(ap) )
	{
	case ATYP_DESTINATION:
		for ( type = Type, count = 1 ; ; count++ )
		{
			if ( FindDest(ADDRDEST(ap), &DestLink, type) != wh_link )
			{
				ErrDest = ap;
				return false;
			}

			if
			(
				count == NROUTES
				||
				DestLink.ln_region == DestLink.ln_rname
				||
				!InRoute(DestLink.ln_rname, 0)
			)
				break;

			/** Try alternate route **/

			if ( type == FASTEST )
				type = CHEAPEST;
			else
				type = FASTEST;
		}

		lsp = &LinkSets[DestLink.ln_index];

		if ( lsp->ls_link.ln_name == NULLSTR )
			lsp->ls_link = DestLink;

		if ( move_addr != (Addr **)0 )
			AddAddr(&lsp->ls_addr, ExtractAddr(move_addr));

		return true;

	case ATYP_EXPLICIT:
		return route(&ap->ad_down, destp);

	case ATYP_MULTICAST:
		do
			if ( !route(&ap->ad_down, &ap->ad_down) )
				(*Errfuncp)(Source, ErrDest);
		while
			( (ap = ap->ad_next) != (Addr *)0 );
		
		return true;

	case ATYP_BROADCAST:
		return broadcast(ap, move_addr);
	}

	ErrDest = ap;
	return false;
}



/*
**	Hot Potato Forwarding Algorithm:
**
**	Message data-base has attested uniqueness of this message,
**	so propagate the message on all those links through which
**	the destination region is reachable,
**	and which it hasn't already visited.
*/

static bool
broadcast(ap, move_addr)
	register Addr *		ap;
	Addr **			move_addr;
{
	register int		region;
	register Index		i;
	register LinkSet *	lsp;
	register int		arrival_link;

	Trace4(
		2, "broadcast(%s, %s) arrived from %s",
		UnTypAddress(ap),
		(move_addr==(Addr **)0)?NullStr:UnTypAddress(*move_addr),
		(Linkp!=(Link *)0)?Linkp->ln_rname:NullStr
	);

	/*
	**	Identify arrival link.
	*/

	if ( Linkp != (Link *)0 )
		arrival_link = Linkp->ln_index;
	else
		arrival_link = NON_INDEX;

	/*
	**	Identify broadcast region.
	*/

	DestLink.ln_flags = 0;

	if ( !route(&ap->ad_down, (Addr **)0) && !(DestLink.ln_flags & FL_HOME) )
	{
		ErrDest = ap;
		return false;	/* Unknown region */
	}

	if ( DestLink.ln_flags & FL_FORWARD )
	{
		/*
		**	Destination region is being forwarded along this link.
		*/

		Trace2(
			2,
			"Forwarded along link \"%s\"",
			DestLink.ln_rname
		);

		if ( DestLink.ln_index == arrival_link )
		{
			/*
			**	Departure link same as arrival link, so drop it.
			*/

			RemoveAddr(move_addr);
			return true;
		}

		return route(&ap->ad_down, move_addr);
	}

	region = DestLink.ln_regindex * LinkCount;	/* Index to tables for destination */

	/*
	**	Propagate along all links through which the destination is reachable.
	*/

	for ( lsp = LinkSets, i = 0 ; i < LinkCount ; i++, region++, lsp++ )
	{
		if ( i == arrival_link )
			continue;

		if ( RegionTable[region/8] & (1<<(region%8)) )
		{
			if ( lsp->ls_link.ln_name == NULLSTR )
				(void)GetLink(&lsp->ls_link, i);

			/*
			**	If it has already been there, don't bother.
			*/

			if ( InRoute(lsp->ls_link.ln_rname, 0) )
				continue;

			Trace2(
				2,
				"Broadcast link \"%s\"",
				lsp->ls_link.ln_rname
			);

			AddAddr(&lsp->ls_addr, (move_addr==(Addr **)0) ? (Addr *)0 : CloneAddr(*move_addr));
		}
	}

	return true;
}
