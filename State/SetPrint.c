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
**	Control the output of the Print routines.
*/

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"


/*
**	Structure for remembering restricted print items.
*/

typedef struct PrintEl	PE;
typedef struct PrintEl
{
	PE *	p_next;
	char *	p_name;
}
		PrintEl;

static PE *	PrintList;
static Region *	MVmatch;

extern char	_ERROR_[];
extern bool	PrintInRegion;
extern bool	PrintVisible;
extern bool	NoJmp;

static void	MarkSubTree _FA_((Region *));
static void	MatchVisible _FA_((Region *));
static void	MVtree _FA_((Entry *));


void
PrintOnly(name)
	char *		name;
{
	register PE *	pp;

	Trace2(2, "PrintOnly(%s)", name);

	pp = Talloc(PrintEl);

	pp->p_next = PrintList;
	PrintList = pp;

	pp->p_name = name;
}



RFlags
SetPrint()
{
	register PE *	pp;
	register Entry *ep;
	register char *	cp;
	RFlags		found = (RFlags)0;
	char		buf[TOKEN_SIZE+1];

	Trace3(1, "SetPrint(), TopRegion=%#lx, PrintList=%#lx", (PUlong)TopRegion, (PUlong)PrintList);

	if ( TopRegion == (Region *)0 )
		return found;

	if ( (pp = PrintList) == (PE *)0 )
	{
		if ( PrintVisible )
			MatchVisible(TopRegion);
		else
			MarkSubTree(TopRegion);		/* Match all regions */
		TopRegion->rg_flags &= ~RF_PRINT;
		return RF_PRINT;
	}

	NoJmp = true;

	do
	{
		if ( strccmp(pp->p_name, "world") == STREQUAL )
			ep = RegionHash[0];
		else
		if
		(
			(
				(cp = CanonicalName(pp->p_name, buf)) == NULLSTR
				||
				cp == _ERROR_
				||
				(ep = Lookup(cp, RegionHash)) == (Entry *)0
			)
			&&
			(ep = Lookup(pp->p_name, RegionHash)) == (Entry *)0
		)
			continue;

		if ( PrintVisible )
		{
			MatchVisible(ep->REGION);
			found = RF_PRINT;
		}
		else
		if ( PrintInRegion )
		{
			MarkSubTree(ep->REGION);
			found = RF_PRINT;
		}
		else
		{
			ep->REGION->rg_flags |= RF_PRINT;
			found = RF_PRINT;
		}
	}
	while
		( (pp = pp->p_next) != (PE *)0 );

	NoJmp = false;

	return found;
}



static void
MarkSubTree(rp)
	register Region *	rp;
{
	Trace2(2, "MarkSubTree(%s)", rp->rg_name);

	rp->rg_flags |= RF_PRINT;

	if ( rp->rg_flags & RF_LINK )
		return;

	for ( rp = rp->rg_down ; rp != (Region *)0 ; rp = rp->rg_next )
		MarkSubTree(rp);
}



/*
**	Mark any regions whose visible region matches "rp".
*/

static void
MatchVisible(rp)
	Region *		rp;
{
	register Entry **	epp;
	register Entry **	end;

	Trace2(1, "MatchVisible(%s)", rp->rg_name);

	if ( rp->rg_name[0] == '\0' )
		MVmatch = (Region *)0;
	else
		MVmatch = rp;

	for ( epp = RegionHash, end = &epp[HASH_SIZE] ; epp < end ; epp++ )
		if ( *epp != (Entry *)0 )
			MVtree(*epp);
}



static void
MVtree(ep)
	register Entry *	ep;
{
	if ( ep->en_great != (Entry *)0 )
		MVtree(ep->en_great);

	if ( ep->en_less != (Entry *)0 )
		MVtree(ep->en_less);

	if ( ep->REGION->rg_visible == MVmatch )
	{
		Trace2(2, "MVtree(%s) match", ep->en_name);
		ep->REGION->rg_flags |= RF_PRINT;
	}
}
