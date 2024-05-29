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
#include	"link.h"
#include	"route.h"
#include	"statefile.h"

#include	<setjmp.h>


bool		NoChange;

extern jmp_buf	NameErrJmp;
extern bool	NoJmp;

static void	force_fwd _FA_((Entry *));
static void	force_routes _FA_((bool, Region *, Region *, Region *));



/*
**	Coalesce regions sharing same route home.
*/

bool
CoalesceRegion(rp, r0, r1)
	register Region *	rp;
	register Region *	r0;
	register Region *	r1;
{
	register Region *	rr0;
	register Region *	rr1;
	register bool		val;

	DODEBUG(Region *	arp = rp->rg_up);

	Trace5(
		4, "CoalesceRegion(%-16.16s, %-8.8s, %-8.8s)%s",
		rp->rg_name,
		r0==(Region *)0 ? NullStr : r0->rg_name,
		r1==(Region *)0 ? NullStr : r1->rg_name,
		(rp->rg_flags & RF_FORCE) ? " FORCE" : EmptyString
	);

	val = true;

	do
	{
		if ( (rr0 = rp->rg_route[0]) == (Region *)0 )
			rr0 = r0;
		if ( (rr1 = rp->rg_route[1]) == (Region *)0 )
			rr1 = r1;

		if
		(
			(
				rp->rg_down == (Region *)0
				||
				(
					!(rp->rg_flags & RF_LINK)
					&&
					CoalesceRegion(rp->rg_down, rr0, rr1)
				)
			)
			&&
			!(rp->rg_flags & (RF_HOME|RF_FORCE))
			&&
			rr0 == r0
			&&
			rr1 == r1
		)
		{
			Trace6(
				3, "CoalesceRegion %-16.16s {%-8.8s, %-8.8s} route %8.8s|%-8.8s",
				rp->rg_name,
				r0==(Region *)0 ? NullStr : r0->rg_name,
				r1==(Region *)0 ? NullStr : r1->rg_name,
				rr0==(Region *)0 ? NullStr : rr0->rg_name,
				rr1==(Region *)0 ? NullStr : rr1->rg_name
			);

			rp->rg_route[0] = (Region *)0;
			rp->rg_route[1] = (Region *)0;
		}
		else
			val = false;
	}
	while
		( (rp = rp->rg_next) != (Region *)0 );

	Trace3(2, "CoalesceRegion %s => %s", (arp==(Region *)0)?"TOP":arp->rg_name, val?"true":"false");

	return val;
}



/*
**	Return parent region common to two.
*/

Region *
CommonRegion(rp1, rp2)
	Region *		rp1;
	Region *		rp2;
{
	register Region *	p1;
	register Region *	p2;
	
	Trace3(
		4, "CommonRegion(%s, %s)",
		(rp1==(Region *)0)?EmptyString:rp1->rg_name,
		(rp2==(Region *)0)?EmptyString:rp2->rg_name
	);

	if ( (p2 = rp2) == (Region *)0 )
		return p2;

	if ( (p1 = rp1) == (Region *)0 || p1 == p2 )
		return p1;

	while ( (p1 = p1->rg_up) != (Region *)0 )
		p1->rg_flags |= RF_MARK;

	while ( (p2 = p2->rg_up) != (Region *)0 )
		if ( p2->rg_flags & RF_MARK )
			break; 

	p1 = rp1;
	while ( (p1 = p1->rg_up) != (Region *)0 )
		p1->rg_flags &= ~RF_MARK;

	return p2;
}



/*
**	Make an entry in the Region tree.
*/

Entry *
EnterRegion(region, new)
	char *			region;
	bool			new;
{
	register Entry *	ep;
	register Region *	rp;
	register Region *	up;
	register char *		cp;
	
	Trace2(4, "EnterRegion(%s)", region);

	ep = Enter(region, RegionHash);

	if ( ep->REGION == (Region *)0 )
	{
		ep->REGION = rp = Talloc0(Region);
		rp->rg_name = region;
		rp->rg_number = AllRegCount++;
		rp->rg_llink = &rp->rg_links;

		if ( new )
			rp->rg_flags = RF_NEW;
	
		if ( (cp = strchr(region, DOMAIN_SEP)) != NULLSTR )
			up = EnterRegion(++cp, new)->REGION;
		else
			up = TopRegion;

		if ( (up == HomeRegion || (up->rg_flags & RF_LINK)) && !NoJmp )
			longjmp(NameErrJmp, ne_floor);

		rp->rg_up = up;
		rp->rg_next = up->rg_down;
		up->rg_down = rp;
	}

	return ep;
}



/*
**	Force region routing via advisory links.
**
**	Assumes list is in override order (next overrides previous).
**
**	Also, force all globally forwarded regions (*.xx.yy)
**	to have immediate sub-regions in routing tables.
*/

void
ForceRoutes()
{
	register AdvRoute *	arp;
	register Region *	rp;
	register Entry **	epp;
	register Entry **	end;

	Trace1(1, "ForceRoutes");

	for ( arp = AdvisoryRoutes ; arp != (AdvRoute *)0 ; arp = arp->ar_next )
		force_routes(true, arp->ar_region, arp->ar_route[0], arp->ar_route[1]);

	Trace1(1, "ForceForwards");

	for ( epp = FwdMapHash, end = &epp[HASH_SIZE] ; epp < end ; epp++ )
		if ( (rp = *(Region **)epp) != (Region *)0 )
			force_fwd((Entry *)rp);
}



/*
**	Walk `Fwd' tree forcing `one below forwarded' regions into routing table
**	- provided forwarded region is also a known region,
**	and routes are different.
*/

static void
force_fwd(ep)
	register Entry *	ep;
{
	register Region *	rp;
	register Region *	r0;
	register Region *	r1;
	register char *		np;
	register char *		cp;

	Trace2(2, "force_fwd(%s)", ep->en_name);

	if ( ep->en_great != (Entry *)0 )
		force_fwd(ep->en_great);
		
	if ( ep->en_less != (Entry *)0 )
		force_fwd(ep->en_less);

	if ( (cp = ep->MAP) == NULLSTR )
		return;

	if ( (np = ep->en_name)[0] == '\0' )
		rp = TopRegion;
	else
	if ( (ep = Lookup(np, RegionHash)) != (Entry *)0 )
		rp = ep->REGION;
	else
		return;

	if ( (ep = Lookup(cp, RegionHash)) == (Entry *)0 )
		return;

	r0 = ep->REGION->rg_route[0];
	r1 = ep->REGION->rg_route[1];

	for ( rp = rp->rg_down ; rp != (Region *)0 ; rp = rp->rg_next )
		if ( rp->rg_route[0] != r0 || rp->rg_route[1] != r1 )
		{
			Trace2(3, "force_fwd %s", rp->rg_name);
			rp->rg_flags |= RF_FORCE;
		}
}



/*
**	Force routes for region and all sub-regions.
*/

static void
force_routes(first, rp, route0, route1)
	bool			first;
	register Region *	rp;
	register Region *	route0;
	register Region *	route1;
{
	extern bool		Warnings;

	Trace4(
		2, "force_routes(%s, %s, %s)",
		rp->rg_name,
		(route0==NULL_ROUTE) ? NullStr : (route0==CLEAR_ROUTE) ? "clear" : route0->rg_name,
		(route1==NULL_ROUTE) ? NullStr : (route1==CLEAR_ROUTE) ? "clear" : route1->rg_name
	);

	if ( route0 != NULL_ROUTE )
	{
		if ( route0 != CLEAR_ROUTE )
		{
			if ( (route0->rg_flags & (RF_REMOVE|RF_LINK)) != RF_LINK )
			{
				Warn
				(
					english("Can't force route for region \"%s\" to \"%s\""),
					rp->rg_name,
					route0->rg_name
				);
				return;
			}

			if ( Warnings )
				Warn
				(
					english("Forcing route for region \"%s\" to \"%s\""),
					rp->rg_name,
					route0->rg_name
				);

			if ( first )
				rp->rg_flags |= RF_FORCE;
		}

		rp->rg_route[0] = route0;
	}

	if ( route1 != NULL_ROUTE )
	{
		if ( route1 != CLEAR_ROUTE  )
		{
			if ( (route1->rg_flags & (RF_REMOVE|RF_LINK)) != RF_LINK )
			{
				Warn
				(
					english("Can't force route for region \"%s\" to \"%s\""),
					rp->rg_name,
					route1->rg_name
				);
				return;
			}

			if ( first )
				rp->rg_flags |= RF_FORCE;
		}

		rp->rg_route[1] = route1;
	}

	if ( rp->rg_flags & RF_LINK )
		return;

	for ( rp = rp->rg_down ; rp != (Region *)0 ; rp = rp->rg_next )
		force_routes(false, rp, route0, route1);
}



/*
**	Return true if rp1 IN rp2
*/

bool
InRegion(rp1, rp2)
	Region *		rp1;
	Region *		rp2;
{
	register Region *	p1;
	register Region *	p2;

	if ( (p2 = rp2) == (Region *)0 )
		return true;

	if ( (p1 = rp1) == (Region *)0 )
		return false;

	while ( p1 != p2 )
		if ( (p1 = p1->rg_up) == (Region *)0 )
			return false;

	return true;
}



/*
**	Initialise top region ("*WORLD*" region).
*/

void
MakeTopRegion()
{
	register Region *	rp;
	register Entry *	ep;

	RegionHash[0] = ep = Talloc0(Entry);
	ep->en_name = EmptyString;
	ep->REGION = rp = Talloc0(Region);

	TopRegion = rp;
	rp->rg_name = EmptyString;

	AllRegCount = 1;
}



/*
**	Set advisory routes for a region (maintain order).
*/

void
NewAdvRoutes(rp, r0, r1)
	register Region *	rp;
	Region *		r0;
	Region *		r1;
{
	register AdvRoute **	arpp;
	register AdvRoute *	arp;

	for
	(
		arpp = (AdvRoute **)&AdvisoryRoutes ;
		(arp = *arpp) != (AdvRoute *)0 ;
		arpp = &arp->ar_next
	)
		if ( rp == arp->ar_region )
			goto set_routes;

	if ( r0 == REMOVE_ROUTE || r1 == REMOVE_ROUTE )
		return;

	*arpp = arp = Talloc(AdvRoute);
	arp->ar_next = (AdvRoute *)0;
	arp->ar_region = rp;
	arp->ar_route[0] = NULL_ROUTE;
	arp->ar_route[1] = NULL_ROUTE;

set_routes:
	if ( r0 != NULL_ROUTE )
	{
		if ( r0 == REMOVE_ROUTE )
			arp->ar_route[0] = NULL_ROUTE;
		else
			arp->ar_route[0] = r0;
	}

	if ( r1 != NULL_ROUTE )
	{
		if ( r1 == REMOVE_ROUTE )
			arp->ar_route[1] = NULL_ROUTE;
		else
			arp->ar_route[1] = r1;
	}

	if
	(
		arp->ar_route[0] == NULL_ROUTE
		&&
		arp->ar_route[1] == NULL_ROUTE
	)
	{
		*arpp = arp->ar_next;
		free((char *)arp);
	}
}



/*
**	Remove a region and all contained in it.
*/

bool
RemoveRegion(arp)
	register Region *	arp;
{
	register Region *	rp;

	Trace2(2, "RemoveRegion(%s)", arp->rg_name);

	if ( arp->rg_flags & (RF_HOME|RF_LINK) )
		return false;

	for ( rp = arp->rg_down ; rp != (Region *)0 ; rp = rp->rg_next )
		if ( !RemoveRegion(rp) )
			return false;

	if ( arp->rg_flags & RF_ALIAS )
	{
		RemEntMap(arp->rg_name, AliasHash);
		RemEntMap(arp->rg_name, RegMapHash);
		arp->rg_flags &= ~RF_ALIAS;
	}

	arp->rg_flags |= RF_REMOVE;
	RegionRemoved = true;

	return true;
}



/*
**	Indicate important state change, and set affected region.
*/

void
SetChange(rp)
	Region *	rp;
{

	if ( NoChange )
		return;

	Trace3(
		2,
		"SetChange(%s) [ChangeRegion=%s]",
		(rp!=(Region *)0)?rp->rg_name:NullStr,
		(ChangeRegion!=(Region *)0)?ChangeRegion->rg_name:NullStr
	);

	ChangeState = true;

	if ( ChangeRegion == (Region *)0 )
		return;

	if ( rp == (Region *)0 )
	{
		ChangeRegion = rp;
		return;
	}

	while ( rp->rg_flags & RF_REMOVE )
		if ( (rp = rp->rg_up) == (Region *)0 )
			return;

	while ( ChangeRegion->rg_flags & RF_REMOVE )
	{
		if ( (ChangeRegion = ChangeRegion->rg_up) == (Region *)0 )
		{
			ChangeRegion = rp;
			return;
		}
	}

	ChangeRegion = MaxRegion(ChangeRegion, rp);

	Trace2(
		1,
		"SetChange() ==> ChangeRegion=%s",
		(ChangeRegion!=(Region *)0)?ChangeRegion->rg_name:NullStr
	);
}
