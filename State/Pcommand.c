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
#include	"routefile.h"
#include	"statefile.h"
#include	"commands.h"


bool		BreakPseudo;

extern bool	AcceptPseudo;
extern bool	NoChange;
extern Region *	SourceRegion;
extern int	SpoolDirLen;
extern int	TokenLength;

extern char *	cmdname _FA_((CmdType));
int		cmpflg _FA_((const void *, const void *));
extern void	Map1stName _FA_((char *, Region *));
static char *	set_flags _FA_((Region *, NLink *, char *));



/*
**	Process a command.
*/

PrType
Pcommand(type, ep1, ep2, lp, cp, errp)
	CmdType			type;
	Entry *			ep1;
	Entry *			ep2;
	NLink *			lp;
	register char *		cp;
	register char **	errp;
{
	register Entry *	ep;
	register char **	cpp;
	register Region *	rp;
	register int		i;
	Ulong			n;
	PrType			retval = pr_ok;
	char			buf[TOKEN_SIZE+1];

	static char *		nonindir = english("parameter change on non-inbound link may cause routing loops");
	static char *		nonoutdir = english("parameter change on non-outbound link may cause routing loops");
	static char *		badparam = english("parameter too large");
	static char *		badregion = english("address not in region");
	static char *		cantmap = english("cannot map pre-existing address");
	static char *		conflict = english("conflict");
	static char *		illlink = english("illegal link");
	static char *		mapint = english("can't map internal type");
	static char *		mapunk = english("unknown type");
	static char *		nolink = english("no link exists");
	static char *		unkaddress = english("unknown address");

	Trace7(2,
		"process(%s,%#lx,%#lx,%#lx,%s,%.20s)",
		cmdname(type), (PUlong)ep1, (PUlong)ep2, (PUlong)lp,
		(cp==NULLSTR)?NullStr:cp,
		(*errp==NULLSTR)?NullStr:*errp
	);

	switch ( type )
	{
	case ct_add:
		if ( IsCommand && HomeRegion == (Region *)0 )
		{
			*errp = english("\"add\" command must be preceded by \"address\"");
			return pr_error;
		}
		rp = ep1->REGION;
		if ( (rp->rg_flags & (RF_NEWREGION|RF_REMOVE)) == (RF_NEWREGION|RF_REMOVE) )
		{
			*errp = conflict;
			return pr_error;
		}
		rp->rg_flags &= ~RF_REMOVE;
		rp->rg_flags |= RF_NEWREGION;
		if ( rp->rg_flags & RF_NEW )
			if ( (rp->rg_visible = rp->rg_up) == TopRegion )
				rp->rg_visible = (Region *)0;
		break;

	case ct_address:
		if ( (rp = HomeRegion) == ep1->REGION )
			break;
		if ( OldName != NULLSTR )
		{
			*errp = conflict;
			return pr_error;
		}
		if ( (OldName = HomeName) == NULLSTR && IsCommand )
			OldName = EmptyString;
		if ( rp != (Region *)0 )
		{
			BreakLinks(rp);
			do
				rp->rg_flags &= ~RF_HOME;
			while
				( (rp = rp->rg_up) != (Region *)0 );
			(void)RemoveRegion(HomeRegion);
		}
		else
			ChangeRegion = ep1->REGION;
		rp = ep1->REGION;
		if ( IsCommand && VisibleName != NULLSTR )
			SetChange(VisRegion);
		if ( HomeRegion != (Region *)0 )
			BreakLinks(rp);
		HomeRegion = rp;
		do
			rp->rg_flags |= RF_HOME;
		while
			( (rp = rp->rg_up) != (Region *)0 );
		if ( (cp = OldName) != NULLSTR && *cp != '\0' )
		{
			RemEntMap(cp, AliasHash);
			RemEntMap(cp, RegMapHash);
			OldAddress = cp;
			*errp = cp;	/* Turn old address into an alias */
			ep1 = Lookup(cp, RegMapHash);
			ep2 = Lookup(cp, AliasHash);
			(void)Pcommand(ct_xalias, ep1, ep2, (NLink *)0, cp, errp);	/* Remove old */
			HomeName = HomeRegion->rg_name;
			(void)Pcommand(ct_xalias, ep1, ep2, (NLink *)0, NULLSTR, errp);	/* Add new */
		}
		else
			HomeName = HomeRegion->rg_name;
		if ( NOADDRCOMPL )
			Map1stName(HomeName, HomeRegion);
		break;

	case ct_break:
	case ct_halfbreak:
		for ( ;; )
		{
			if ( lp != (NLink *)0 )
			{
				if ( lp->nl_flags & FL_NEWLINK )
				{
					*errp = conflict;
					return pr_error;
				}
				lp->nl_flags |= FL_REMOVE|FL_NEWLINK;
			}
			if ( type == ct_halfbreak )
				break;
			type = ct_halfbreak;
			ep = ep1;
			if ( (ep1 = ep2) == (Entry *)0 || (ep2 = ep) == (Entry *)0 )
				break;
			lp = IsLinked(ep1->REGION, ep2->REGION);
		}
		break;

	case ct_breakall:
		if ( (ep = ep1) != (Entry *)0 )
		{
			if ( ep->REGION->rg_flags & RF_NEWREGION )
			{
				*errp = conflict;
				return pr_error;
			}

			NoChange = true;
			BreakLinks(ep->REGION);
			NoChange = false;
		}
		break;

	case ct_breakpseudo:
		if ( (ep = ep1) != (Entry *)0 )
			BreakPLinks(ep->REGION);
		else
		if ( **errp == '\0' )
			BreakPseudo = true;	/* Break all pseudo links */
		break;

	case ct_caller:
		if ( (ep = ep1) == (Entry *)0 || !(ep->REGION->rg_flags & RF_LINK) )
		{
			*errp = nolink;
			return pr_warn;
		}
		if ( cp != NULLSTR && strncmp(SPOOLDIR, cp, SpoolDirLen) == STREQUAL )
			cp += SpoolDirLen;
		((Link *)ep->REGION->rg_down)->ln_caller = newstr(cp);
		break;

/*	case ct_cheaproute:	SEE ct_route */

	case ct_clear:
		if ( (ep = ep1) == (Entry *)0 )
			break;
		ep->REGION->rg_state = 0;
		break;

	case ct_comment:
		i = (int)si_comment;
do_comment:
		if ( cp == NULLSTR )
		{
			SiteInfoNames[i].si_value = cp;
			SiteInfoNames[i].si_length = 0;
		}
		else
		{
			SiteInfoNames[i].si_value = newstr(cp);
			SiteInfoNames[i].si_length = strlen(cp);
		}
		NewComment = true;
		break;

	case ct_contact:
		i = (int)si_contact;
		goto do_comment;

	case ct_cost:
		if ( lp == (NLink *)0 )
		{
			*errp = nolink;
			return pr_warn;
		}
		if ( (n = IsCommand?atol(cp):DecodeNum(cp, TokenLength)) > MAX_COST )
		{
			*errp = badparam;
			return pr_error;
		}
		if ( lp->nl_cost == n )
			break;	/* No change */
		lp->nl_cost = n;
		rp = ep1->REGION;
		if ( rp == HomeRegion && !(lp->nl_flags & FL_NOCHANGE) )
		{
			if ( lp->nl_flags & FL_NEWCOST )
			{
				*errp = conflict;
				return pr_error;
			}
			if ( IsCommand )
				SetChange(MinRegion(rp->rg_visible, lp->nl_to->rg_visible));
		}
		if ( Check_Conflict )
			lp->nl_flags |= FL_NEWCOST;
		if
		(
			rp != HomeRegion
			&&
			!(rp->rg_flags & RF_FOREIGN)
			&&
			rp->rg_state != 0
		)
		{
			*errp = nonoutdir;
			return pr_fwarn;
		}
		break;

	case ct_delay:
		if ( lp == (NLink *)0 )
		{
			*errp = nolink;
			return pr_warn;
		}
		if ( (n = IsCommand?atol(cp):DecodeNum(cp, TokenLength)) > MAX_DELAY )
		{
			*errp = badparam;
			return pr_error;
		}
		if ( lp->nl_delay == n )
			break;	/* No change */
		lp->nl_delay = n;
		rp = ep2->REGION;
		if ( rp == HomeRegion && !(lp->nl_flags & FL_NOCHANGE) )
		{
			if ( lp->nl_flags & FL_NEWDELAY )
			{
				*errp = conflict;
				return pr_error;
			}
			if ( IsCommand )
				SetChange(MinRegion(rp->rg_visible, lp->nl_to->rg_visible));
		}
		if ( Check_Conflict )
			lp->nl_flags |= FL_NEWDELAY;
		if
		(
			rp != HomeRegion
			&&
			!(rp->rg_flags & RF_FOREIGN)
			&&
			rp->rg_state != 0
		)
		{
			*errp = nonindir;
			return pr_fwarn;
		}
		break;

	case ct_edited:
		i = (int)si_edited;
		goto do_comment;

	case ct_email:
		i = (int)si_email;
		goto do_comment;

/*	case ct_fastroute:	SEE ct_route */

	case ct_filter:
		if
		(
			(ep = ep1) == (Entry *)0
			||
			!((rp = ep->REGION)->rg_flags & RF_LINK)
			||
			!(rp->rg_flags & (RF_LINKTO|RF_LINKFROM))
		)
		{
			*errp = nolink;
			return pr_warn;
		}
		if ( cp != NULLSTR && strncmp(SPOOLDIR, cp, SpoolDirLen) == STREQUAL )
			cp += SpoolDirLen;
		((Link *)ep->REGION->rg_down)->ln_filter = newstr(cp);
		if ( (rp->rg_flags & RF_LINKTO) && (lp = IsLinked(rp, HomeRegion)) != (NLink *)0 )
		{
			if ( cp == NULLSTR )
				lp->nl_flags &= ~FL_FILTERED;
			else
				lp->nl_flags |= FL_FILTERED;
		}
		if ( (rp->rg_flags & RF_LINKFROM) && (lp = IsLinked(HomeRegion, rp)) != (NLink *)0 )
		{
			if ( cp == NULLSTR )
				lp->nl_flags &= ~FL_FILTERED;
			else
				lp->nl_flags |= FL_FILTERED;
		}
		break;

	case ct_flag:
		if ( lp == (NLink *)0 )
		{
			*errp = nolink;
			return pr_warn;
		}
		if ( cp == NULLSTR )
		{
			*errp = english("missing flags");
			return pr_error;
		}
		if ( (*errp = set_flags(ep1->REGION, lp, cp)) != NULLSTR )
			return pr_error;
		break;

	case ct_forward:
		if ( cp == NULLSTR )
		{
			if ( (ep = ep1) != (Entry *)0 )
				ep->MAP = NULLSTR;
			break;
		}
		if ( (*errp)[0] == '*' )
		{
			if ( (*errp)[1] != '.' )
			{
				*errp = unkaddress;
				return pr_error;
			}
			(*errp) += 2;
			if ( (*errp)[0] != '\0' )
			{
				if ( (*errp)[1] != TYPE_SEP )
				{
					*errp = unkaddress;
					return pr_warn;
				}
				(*errp)[1] = '\0';
				if ( !InternalType(&(*errp)[0]) )
				{
					*errp = unkaddress;
					return pr_warn;
				}
				(*errp)[1] = TYPE_SEP;
			}
			i = 1;
		}
		else
			i = 0;
		if ( (ep = Lookup(*errp, RegionHash)) != (Entry *)0 && ep->REGION == HomeRegion )
		{
			*errp = conflict;
			return pr_error;
		}
		ep2 = ep;
		if
		(
			(ep = Lookup(cp, RegionHash)) == (Entry *)0
			&&
			(ep = Lookup(CanonicalName(cp, buf), RegionHash)) == (Entry *)0
		)
		{
			*errp = unkaddress;
			return pr_warn;
		}
		if ( ep == ep2 )
		{
			*errp = conflict;
			return pr_error;
		}
		cp = ep->REGION->rg_name;
		if ( (ep = ep1) == (Entry *)0 )
			ep = Enter(newstr(*errp), i ? FwdMapHash : AdrMapHash);
		ep->MAP = cp;
		break;

/*	case ct_halflink:	SEE ct_link */
/*	case ct_halfbreak:	SEE ct_break */

	case ct_ialias:
		if ( cp == NULLSTR )
		{
			if ( (ep = ep2) != (Entry *)0 )	/* AliasHash */
				ep->MAP = NULLSTR;
			if ( (ep = ep1) != (Entry *)0 )	/* RegMapHash */
				ep->MAP = NULLSTR;
			if ( (ep = Lookup(FirstName(*errp), RegMapHash)) != (Entry *)0 )
				ep->MAP = NULLSTR;
			if ( (ep = Lookup(StripTypes(*errp), RegMapHash)) != (Entry *)0 )
				ep->MAP = NULLSTR;
			break;
		}
		if
		(
			(ep = Lookup(cp, RegionHash)) == (Entry *)0
			&&
			(ep = Lookup(CanonicalName(cp, buf), RegionHash)) == (Entry *)0
		)
		{
			*errp = unkaddress;
			return pr_fwarn;
		}
		if ( (rp = ep->REGION) == HomeRegion )
		{
			*errp = english("value of import alias is home");
			return pr_fwarn;
		}
		if
		(
			(ep = Lookup(*errp, RegionHash)) != (Entry *)0
			&&
			!(ep->REGION->rg_flags & RF_REMOVE)
		)
		{
			*errp = cantmap;
			return pr_fwarn;
		}
		if ( !InRegion(HomeRegion, rp->rg_visible) )
		{
			*errp = badregion;
			return pr_warn;
		}
		cp = rp->rg_name;
		if ( rp->rg_up == VisRegion && strccmp(FirstDomain(cp), FirstDomain(*errp)) == STREQUAL )
		{
			*errp = cantmap;
			return pr_warn;
		}
		if ( (ep = ep2) != (Entry *)0 && ep->MAP != NULLSTR && strccmp(ep->MAP, cp) != STREQUAL )
		{
			*errp = cantmap;
			return pr_fwarn;
		}
		if ( ep == (Entry *)0 )
			ep = Enter(newstr(*errp), AliasHash);
		ep->MAP = cp;
		rp->rg_flags |= RF_ALIAS;
#		if	0
		if
		(
			(cp = strchr(*errp, DOMAIN_SEP)) != NULLSTR
			&&
			strccmp(++cp, HomeRegion->rg_up->rg_name) == STREQUAL
		)
		{
			if ( (ep = MapDomain(FirstName(*errp), FirstDomain(rp->rg_name))) != (Entry *)0 )
				ep->en_flags |= EF_NOWRITE;
			break;
		}
#		else	/* 0 */
		cp = strchr(*errp, DOMAIN_SEP);
#		endif	/* 0 */
		Map1stName(*errp, rp);	/* Enter first name in Region map */
		/** Enter stripped ialias in Region map **/
		if ( cp == NULLSTR )
			break;	/* Just first name -- already added */
		if ( (ep = ep1) == (Entry *)0 )
			ep = Enter(StripTypes(*errp), RegMapHash);
		ep->MAP = rp->rg_name;
		ep->en_flags |= EF_NOWRITE;
		break;

	case ct_licence:
		i = (int)si_licence;
		goto do_comment;

	case ct_halflink:
	case ct_link:
link:		if ( lp == (NLink *)0 || (lp->nl_flags & FL_NOLINK) )
		{
			if ( (ep = ep1) == (Entry *)0 || ep2 == (Entry *)0 )
			{
				*errp = unkaddress;
				return pr_error;
			}
			if ( IsCommand && !LegalLink(ep->REGION, ep2->REGION) )
			{
				*errp = illlink;
				return pr_error;
			}
			if ( lp != (NLink *)0 )
				lp->nl_flags &= ~(FL_REMOVE|FL_PSEUDO|FL_NOLINK);
			else
				lp = MakeLink(ep->REGION, ep2->REGION);
			if ( ep2->REGION == HomeRegion || (ep->REGION == HomeRegion && (ep = ep2)) )
			{
				rp = ep->REGION;
				if ( rp->rg_flags & ((ep==ep1)?RF_LINKTO:RF_LINKFROM) )
				{
					*errp = conflict;
					return pr_error;
				}
				rp->rg_flags |= ((ep==ep1)?RF_LINKTO:RF_LINKFROM);
				if ( IsCommand )
					SetChange
					(
						MinRegion(
							ep1->REGION->rg_visible,
							ep2->REGION->rg_visible
						)
					);
				if ( !(rp->rg_flags & RF_LINK) )
				{
					register Link *	rlp;

					if ( rp->rg_down != (Region *)0 )
					{
						*errp = illlink;
						return pr_error;
					}
						
					rlp = Talloc0(Link);
					rp->rg_down = (Region *)rlp;
					rp->rg_flags |= RF_LINK;
					rlp->ln_index = LinkCount++;
					rlp->ln_name = DomainToPath(rp->rg_name);

					/** Map names of links for convenience **/
					Map1stName(rp->rg_name, rp);
					if ( !(rp->rg_up->rg_flags & RF_HOME) )
					{
						ep = Enter(StripTypes(rp->rg_name), RegMapHash);
						if ( ep->MAP == NULLSTR )
						{
							ep->MAP = rp->rg_name;
							ep->en_flags |= EF_NOWRITE;
							rp->rg_flags |= RF_ALIAS;
						}
					}
				}
			}
			if ( Check_Conflict )
				lp->nl_flags |= FL_NEWLINK;
		}
		else
		{
			if ( Check_Conflict )
			{
				if ( lp->nl_flags & FL_NEWLINK )
				{
					if ( lp->nl_flags & FL_REMOVE )
					{
						*errp = conflict;
						return pr_error;
					}
					*errp = english("duplicate");
					retval = pr_warn;
				}
				else
					lp->nl_flags |= FL_NEWLINK;
			}
			lp->nl_flags &= ~(FL_REMOVE|FL_PSEUDO);
		}
		if ( cp != NULLSTR && (*errp = set_flags(ep1->REGION, lp, cp)) != NULLSTR )
			return pr_error;
		if ( type == ct_link )
		{
			type = ct_halflink;
			ep = ep1;
			if ( (ep1 = ep2) == (Entry *)0 || (ep2 = ep) == (Entry *)0 )
				break;
			lp = IsLinked(ep1->REGION, ep2->REGION);
			goto link;
		}
		break;

	case ct_linkname:
		if ( (ep = ep1) == (Entry *)0 )
		{
			*errp = unkaddress;
			return pr_warn;
		}
		rp = ep->REGION;
		if ( !(rp->rg_flags & RF_LINK) )
		{
			*errp = nolink;
			return pr_warn;
		}
		cpp = &((Link *)(rp->rg_down))->ln_name;
		if ( cp == NULLSTR )
		{
			if ( (cp = *cpp) != NULLSTR && (ep = Lookup(cp, RegMapHash)) != (Entry *)0 )
				ep->MAP = NULLSTR;
			((Link *)(rp->rg_down))->ln_flags &= ~FL_LINKNAME;
			*cpp = DomainToPath(rp->rg_name);
			break;
		}
		((Link *)(rp->rg_down))->ln_flags |= FL_LINKNAME;
		*cpp = cp = newstr(cp);

		/** Map linknames for convenience **/
		ep = Enter(cp, RegMapHash);
		if ( ep->MAP == NULLSTR )
		{
			ep->MAP = rp->rg_name;
			ep->en_flags |= EF_NOWRITE;
			rp->rg_flags |= RF_ALIAS;
		}
		break;

	case ct_location:
		i = (int)si_location;
		goto do_comment;

	case ct_map:		/* type map, typename map, or region map, depending on "cp" */
		if ( cp == NULLSTR )
		{
			if ( IntTypeName(*errp) )
			{
				*errp = mapint;
				return pr_error;
			}
			if ( (ep = ep1) != (Entry *)0 )	/* RegMapHash */
				ep->MAP = NULLSTR;
			if ( (ep = Lookup(*errp, TypMapHash)) != (Entry *)0 )
				ep->MAP = NULLSTR;
			if ( (ep = Lookup(*errp, DomMapHash)) != (Entry *)0 )
				ep->MAP = NULLSTR;
			break;
		}
#		ifndef	TYPED_MAP_REGION
		if ( strchr(*errp, TYPE_SEP) != NULLSTR )
		{
			*errp = english("cannot specify types in mapped name");
			return pr_error;
		}
#		endif	/* TYPED_MAP_REGION */
		if ( strchr(cp, TYPE_SEP) != NULLSTR || strchr(cp, DOMAIN_SEP) != NULLSTR )
		{
			/** "map name domain" or "map name region" **/
			if ( IntTypeName(*errp) )
			{
				*errp = mapint;
				return pr_error;
			}
			cp = CanonicalName(cp, buf);
			if ( MapDomain(*errp, cp) != (Entry *)0 )
				break;
			if ( (ep = Lookup(cp, RegionHash)) != (Entry *)0 )
			{
				cp = ep->en_name;
				ep->REGION->rg_flags |= RF_ALIAS;
				i = 1;
			}
#			ifdef	notdef
			else
			if ( strchr(cp, DOMAIN_SEP) == NULLSTR )
			{
				if ( NoTypes || ExpTypes )
					break;
				*errp = badregion;
				return pr_error;
			}
#			endif	/* notdef */
			else
				i = 0;
			if ( (ep = ep1) == (Entry *)0 )
				ep = Enter(newstr(*errp), RegMapHash);
			if ( ep->MAP == NULLSTR || strcmp(ep->MAP, cp) != STREQUAL )
				ep->MAP = i?cp:newstr(cp);
			break;
		}
		/** "map name type" **/
		if ( (ep = Lookup(cp, TypMapHash)) == (Entry *)0 || ep->MAP == NULLSTR )
		{
			*errp = mapunk;
			return pr_error;
		}
		cp = ep->MAP;
	case ct_maptype:	/* From statefile */
		if ( (cp = InternalType(cp)) == NULLSTR )
		{
			*errp = mapunk;
			return pr_error;
		}
		if ( (ep = Lookup(*errp, TypMapHash)) != (Entry *)0 && IntTypeName(*errp) && ep->MAP != NULLSTR )
		{
			if ( cp[0] == ep->MAP[0] )
				break;
			*errp = mapint;
			return pr_error;
		}
		if ( ep == (Entry *)0 )
			ep = Enter(newstr(*errp), TypMapHash);
		ep->MAP = cp;
		break;

	case ct_noroute:
		if ( (ep = ep1) == (Entry *)0 )
		{
			*errp = unkaddress;
			return pr_warn;
		}
		NewAdvRoutes(ep->REGION, CLEAR_ROUTE, CLEAR_ROUTE);
		break;

	case ct_ordertypes:
		if ( (*errp = ParseOrderTypes(*errp)) != NULLSTR )
			return pr_error;
		break;

	case ct_org:
		i = (int)si_org;
		goto do_comment;

	case ct_post:
		i = (int)si_post;
		goto do_comment;

	case ct_remove:
		if ( (ep = ep1) == (Entry *)0 )
			break;
		rp = ep->REGION;
		if ( rp->rg_flags & RF_NEWREGION )
		{
			*errp = conflict;
			return pr_error;
		}
		if ( rp->rg_flags & (RF_HOME|RF_LINK) || !RemoveRegion(rp) )
		{
			*errp = english("cannot remove own/linked region");
			return pr_error;
		}
		rp->rg_flags |= RF_NEWREGION;
		break;

	case ct_removemaps:
		if ( **errp != '\0' )
			cp = *errp;
		else
			cp = NULLSTR;
		RemEntMap(cp, FwdMapHash);
		RemEntMap(cp, AdrMapHash);
		RemEntMap(cp, AliasHash);
		RemEntMap(cp, DomMapHash);
		RemEntMap(cp, RegMapHash);
		RemEntMap(cp, TypMapHash);
		break;

	case ct_restrict:
		if ( lp == (NLink *)0 )
		{
			*errp = nolink;
			return pr_warn;
		}
		if ( cp == NULLSTR )
			rp = (Region *)0;
		else
		if
		(
			(ep = Lookup(cp, RegionHash)) == (Entry *)0
			&&
			(ep = Lookup(CanonicalName(cp, buf), RegionHash)) == (Entry *)0
		)
		{
			*errp = unkaddress;
			return pr_error;
		}
		else
		{
			rp = ep->REGION;
			if ( !InRegion(lp->nl_to, rp) )
			{
				*errp = badregion;
				return pr_error;
			}
		}
		if ( lp->nl_restrict == rp )
			break;	/* No change */
		lp->nl_restrict = rp;
		ep = ep2;
		if ( IsCommand )
		{
			if ( ep->REGION == HomeRegion )
			{
				if ( lp->nl_flags & FL_NEWREGION )
				{
					*errp = conflict;
					return pr_error;
				}
				lp->nl_flags |= FL_NEWREGION;
				SetChange(MinRegion(HomeRegion->rg_visible, ep1->REGION->rg_visible));
			}
			else
			if
			(
				!(ep->REGION->rg_flags & RF_FOREIGN)
				&&
				ep->REGION->rg_state != 0
			)
			{
				*errp = nonindir;
				return pr_fwarn;
			}
		}
		break;

	case ct_route:
	case ct_fastroute:
	case ct_cheaproute:
		if ( (ep = ep1) == (Entry *)0 )
		{
			*errp = unkaddress;
			return (cp==NULLSTR)?pr_warn:pr_error;
		}
		rp = ep->REGION;
		if ( cp != NULLSTR )
		{
			if
			(
				(ep = Lookup(cp, RegionHash)) == (Entry *)0
				&&
				(ep = Lookup(CanonicalName(cp, buf), RegionHash)) == (Entry *)0
			)
			{
				*errp = badregion;
				return pr_fwarn;
			}
			if ( !(ep->REGION->rg_flags & RF_LINK) )
			{
				*errp = nolink;
				return pr_error;
			}
			switch ( type )
			{
			case ct_route:		NewAdvRoutes(rp, ep->REGION, ep->REGION);	break;
			case ct_fastroute:	NewAdvRoutes(rp, ep->REGION, NULL_ROUTE);	break;
			case ct_cheaproute:	NewAdvRoutes(rp, NULL_ROUTE, ep->REGION);	break;
			default:								break;	/* gcc -Wall */
			}
		}
		else	/* Remove */
			switch ( type )
			{
			case ct_route:		NewAdvRoutes(rp, REMOVE_ROUTE, REMOVE_ROUTE);	break;
			case ct_fastroute:	NewAdvRoutes(rp, REMOVE_ROUTE, NULL_ROUTE);	break;
			case ct_cheaproute:	NewAdvRoutes(rp, NULL_ROUTE, REMOVE_ROUTE);	break;
			default:								break;	/* gcc -Wall */
			}
		break;

/*	case ct_speed:
**		if ( lp == (NLink *)0 )
**		{
**			*errp = nolink;
**			return pr_warn;
**		}
**		if ( (n = IsCommand?atol(cp):DecodeNum(cp, TokenLength)) > MAX_SPEED )
**		{
**			*errp = badparam;
**			return pr_error;
**		}
**		if ( lp->nl_speed == n )
**			break;	*//* No change *//*
**		lp->nl_speed = n;
**		rp = ep1->REGION;
**		if ( rp == HomeRegion && !(lp->nl_flags & FL_NOCHANGE) )
**		{
**			if ( lp->nl_flags & FL_NEWSPEED )
**			{
**				*errp = conflict;
**				return pr_error;
**			}
**			if ( IsCommand )
**				SetChange(MinRegion(rp->rg_visible, lp->nl_to->rg_visible));
**		}
**		if ( Check_Conflict )
**			lp->nl_flags |= FL_NEWSPEED;
**		if
**		(
**			rp != HomeRegion
**			&&
**			!(rp->rg_flags & RF_FOREIGN)
**			&&
**			rp->rg_state != 0
**		)
**		{
**			*errp = nonoutdir;
**			return pr_fwarn;
**		}
**		break;
*/
	case ct_spooler:
		if ( (ep = ep1) == (Entry *)0 || !(ep->REGION->rg_flags & RF_LINK) )
		{
			*errp = nolink;
			return pr_warn;
		}
		if ( cp != NULLSTR && strncmp(SPOOLDIR, cp, SpoolDirLen) == STREQUAL )
			cp += SpoolDirLen;
		((Link *)ep->REGION->rg_down)->ln_spooler = newstr(cp);
		break;

	case ct_systype:
		i = (int)si_systype;
		goto do_comment;

	case ct_telno:
		i = (int)si_telno;
		goto do_comment;

/*	case ct_traffic:
**		if ( lp == (NLink *)0 )
**		{
**			*errp = nolink;
**			return pr_warn;
**		}
**		if ( (n = IsCommand?atol(cp):DecodeNum(cp, TokenLength)) > MAX_TRAFFIC )
**		{
**			*errp = badparam;
**			return pr_error;
**		}
**		if ( lp->nl_traffic == n )
**			break;	*//* No change *//*
**		lp->nl_traffic = n;
**		rp = ep1->REGION;
**		if ( rp == HomeRegion && !(lp->nl_flags & FL_NOCHANGE) )
**		{
**			if ( lp->nl_flags & FL_NEWTRAFFIC )
**			{
**				*errp = conflict;
**				return pr_error;
**			}
**			if ( IsCommand )
**				SetChange(MinRegion(rp->rg_visible, lp->nl_to->rg_visible));
**		}
**		if ( Check_Conflict )
**			lp->nl_flags |= FL_NEWTRAFFIC;
**		if
**		(
**			rp != HomeRegion
**			&&
**			!(rp->rg_flags & RF_FOREIGN)
**			&&
**			rp->rg_state != 0
**		)
**		{
**			*errp = nonoutdir;
**			return pr_fwarn;
**		}
**		break;
*/
	case ct_visible:
		if ( (ep = ep1) == (Entry *)0 )
		{
			*errp = unkaddress;
			return pr_warn;
		}
		rp = ep->REGION;
		if ( cp != NULLSTR )
		{
			if
			(
				(ep = Lookup(cp, RegionHash)) == (Entry *)0
				&&
				(ep = Lookup(CanonicalName(cp, buf), RegionHash)) == (Entry *)0
			)
			{
				*errp = badregion;
				return pr_error;
			}
			if ( rp == ep->REGION || !InRegion(rp, ep->REGION) )
			{
				*errp = badregion;
				return pr_error;
			}
			if ( rp->rg_visible == ep->REGION )
				break;	/* No change */
			rp->rg_visible = ep->REGION;
		}
		else
		if ( rp->rg_visible != (Region *)0 )
			rp->rg_visible = (Region *)0;
		else
			break;	/* No change */
		if ( rp == HomeRegion )
		{
			rp = rp->rg_visible;
			if ( VisRegion == rp )
				break;
			if ( OldVisName != NULLSTR )
			{
				*errp = conflict;
				return pr_error;
			}
			if ( (OldVisName = VisibleName) == NULLSTR && IsCommand )
				OldVisName = EmptyString;
			if ( IsCommand )
			{
				if ( VisibleName == NULLSTR )
					SetChange(rp);
				else
					SetChange(MaxRegion(rp, VisRegion));
			}
			VisRegion = rp;
			VisibleName = (rp==(Region *)0) ? EmptyString : rp->rg_name;
		}
		else
		if ( rp == SourceRegion )
		{
			if ( InRegion(VisRegion, rp->rg_visible) )
				AcceptPseudo = true;
			else
				AcceptPseudo = false;
		}
		else
		if ( !(rp->rg_flags & RF_PSEUDO) )
		{
			*errp = english("visible region change on non-home region may cause routing loops");
			return pr_fwarn;
		}
		break;

	case ct_xalias:
		if ( cp != NULLSTR )
		{
			if
			(
				(ep = ep2) != (Entry *)0	/* AliasHash */
				&&
				ep->MAP == HomeName
			)
				ep->MAP = NULLSTR;
			if
			(
				(ep = ep1) != (Entry *)0	/* RegMapHash */
				&&
				ep->MAP == HomeName
			)
				ep->MAP = NULLSTR;
			if
			(
				(ep = Lookup(FirstName(*errp), RegMapHash)) != (Entry *)0
				&&
				ep->MAP == HomeName
			)
				ep->MAP = NULLSTR;
			break;
		}
		if
		(
			(
			(ep = ep1) != (Entry *)0	/* RegMapHash */
			&&
			(cp = ep->MAP) != NULLSTR
			&&
			cp != HomeName
			) || (
			(ep = ep2) != (Entry *)0	/* AliasHash */
			&&
			(cp = ep->MAP) != NULLSTR
			&&
			cp != HomeName
			) || (
			(ep = Lookup(*errp, RegionHash)) != (Entry *)0
			&&
			!(ep->REGION->rg_flags & RF_REMOVE)
			)
		)
		{
			*errp = cantmap;
			return pr_fwarn;
		}
		if ( (ep = ep2) == (Entry *)0 )
			/** Enter whole xalias in Alias table **/
			ep = Enter(newstr(*errp), AliasHash);
		ep->MAP = HomeName;
		if ( (ep = ep1) == (Entry *)0 )
			/** Enter stripped xalias in Region map **/
			ep = Enter(StripTypes(*errp), RegMapHash);
		ep->MAP = HomeName;
		ep->en_flags |= EF_NOWRITE;
		Map1stName(*errp, HomeRegion);	/* Enter first name in Region map */
		break;

	default:
		Fatal2("Pcommand: unknown command type %d", (int)type);
	}

	return retval;
}



/*
**	Compare for flags bsearch.
*/

int
#ifdef	ANSI_C
cmpflg(const void *cp, const void *flgp)
#else	/* ANSI_C */
cmpflg(cp, flgp)
	char *	cp;
	char *	flgp;
#endif	/* ANSI_C */
{
	Trace3(5, "cmpflg \"%s\" <> \"%s\"", cp, ((Flag *)flgp)->fl_name);
	return strccmp(cp, ((Flag *)flgp)->fl_name);
}



/*
**	Enter first name in Region map.
*/

void
Map1stName(fullname, rp)
	char *			fullname;
	Region *		rp;
{
	register Entry *	ep;

	ep = Enter(FirstName(fullname), RegMapHash);
	if ( ep->MAP == NULLSTR )
	{
		ep->MAP = rp->rg_name;
		ep->en_flags |= EF_NOWRITE;
		rp->rg_flags |= RF_ALIAS;
	}
}



/*
**	Decode state flags from string (only for commands).
*/

static char *
set_flags(rp, lp, cp)
	Region *	rp;
	register NLink *lp;
	register char *	cp;
{
	register Flag *	flgp;
	register char *	ep;
	Flags		old = lp->nl_flags;
	bool		set;

	Trace2(3, "set_flags(%s)", cp);

	for ( ;; )
	{
		set = true;

		switch ( *cp )
		{
		case '-':	set = false;
		case '+':	cp++;
		}

		if ( (ep = strchr(cp, FLAG_C)) != NULLSTR )
			*ep = '\0';

		if
		(
			(flgp = (Flag *)bsearch
					(
						cp,
						(char *)SortedFlags,
						(unsigned)NSFLGS,
						FLGZ,
						cmpflg
					)
			) == (Flag *)0
		)
			return english("unknown flag");

		if ( flgp->fl_value & FL_RESTRICT )
			return english("illegal flag");

		if ( set )
			lp->nl_flags |= flgp->fl_value;
		else
			lp->nl_flags &= ~flgp->fl_value;

		if ( (cp = ep) != NULLSTR )
			*cp++ = FLAG_C;
		else
			break;
	}

	if
	(
		(old & FL_HOME)
		&&
		((old ^ lp->nl_flags) & FL_ROUTING)
	)
	{
		if ( old & FL_NEWFLAGS )
			return english("conflict");
		if ( Check_Conflict )
			lp->nl_flags |= FL_NEWFLAGS;
		if ( LinkCount > 1 && !(lp->nl_flags & FL_NOCHANGE) )
			SetChange(MinRegion(rp->rg_visible, lp->nl_to->rg_visible));
	}

	return NULLSTR;
}
