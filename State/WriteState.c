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
**	Write new version of State file from contents of sorted Region list.
*/

#define	STDIO

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"


static State_t	StateType;
static Crc_t	StCRC;
static FILE *	WrStateFd;

static void	WrFlags _FA_((char, Ulong));
static void	WrNumber _FA_((char, Ulong));
static void	WrOrdTyp _FA_((void));
static void	WrRegion _FA_((Region *));
static void	WrString _FA_((char, char *));
static void	WrToken _FA_((char));



void
Write_State(fd, type)
	FILE *			fd;
	State_t			type;
{
	register Entry **	epp;
	register Entry *	ep;
	register int		i;
	register Region *	rp;
	register AdvRoute *	arp;

	Trace3(1, "Write_State(%d, %s)", (int)fileno(fd), type==local_state?"local":"export");

	DODEBUG(CHECKTOKEN());

	if ( HomeRegion == TopRegion )
		return;

	WrStateFd = fd;
	StateType = type;
	StCRC = 0;

	/*
	**	Write types, home address and visible region.
	*/

	if ( StateType == local_state )
		WrOrdTyp();
	else
	if ( StateType == export_state )
		WrToken(C_ORDERTYPES);	/* Indicate all region info. enclosed */

	WrString(C_ADDRESS, HomeName);

	if ( OldAddress != NULLSTR )
		WrString(C_OLDADDRESS, OldAddress);

	WrString(C_COMMENT, HomeComment);

	/*
	**	Write regions and links (in reverse order).
	*/

	if ( StateType == export_state && !RegionsCoalesced )
	{
		(void)CoalesceRegion(TopRegion, (Region *)0, (Region *)0);
		RegionsCoalesced = true;
	}

	WrRegion(HomeRegion);

	RegionCount = i = MakeList(&RegionList, RegionHash, RegionCount);

	for ( epp = &RegionList[i] ; --i >= 0 ; )
		if
		(
			(rp = (*--epp)->REGION) != HomeRegion
			&&
			rp != TopRegion
			&&
			(
				StateType != site_state
				||
				(rp->rg_flags & (RF_LINKFROM|RF_LINKTO))
			)
		)
			WrRegion(rp);

	/*
	**	Write mappings.
	*/

	if ( StateType == local_state )
	{
		if ( FwdMapList == (Entry **)0 )
			FwdMapCount = MakeList(&FwdMapList, FwdMapHash, FwdMapCount);

		for ( epp = FwdMapList, i = FwdMapCount ; --i >= 0 ; epp++ )
		{
			if ( (ep = *epp)->MAP == NULLSTR || (ep->en_flags & EF_NOWRITE) )
				continue;
			WrString(C_FORWARD, ep->en_name);
			WrString(C_MAPVAL, ep->MAP);
		}

		if ( AdrMapList == (Entry **)0 )
			AdrMapCount = MakeList(&AdrMapList, AdrMapHash, AdrMapCount);

		for ( epp = AdrMapList, i = AdrMapCount ; --i >= 0 ; epp++ )
		{
			if ( (ep = *epp)->MAP == NULLSTR || (ep->en_flags & EF_NOWRITE) )
				continue;
			WrString(C_MAPADR, ep->en_name);
			WrString(C_MAPVAL, ep->MAP);
		}

		if ( RegMapList == (Entry **)0 )
			RegMapCount = MakeList(&RegMapList, RegMapHash, RegMapCount);

		for ( epp = RegMapList, i = RegMapCount ; --i >= 0 ; epp++ )
		{
			if ( (ep = *epp)->MAP == NULLSTR || (ep->en_flags & EF_NOWRITE) )
				continue;
			WrString(C_MAPREG, ep->en_name);
			WrString(C_MAPVAL, ep->MAP);
		}

		if ( DomMapList == (Entry **)0 )
			i = MakeList(&DomMapList, DomMapHash, (Index)0);

		for ( epp = DomMapList ; --i >= 0 ; epp++ )
		{
			if ( (ep = *epp)->MAP == NULLSTR || (ep->en_flags & EF_NOWRITE) )
				continue;
			WrString(C_MAPREG, ep->en_name);
			WrString(C_MAPVAL, ep->MAP);
		}

		if ( TypMapList == (Entry **)0 )
			TypMapCount = MakeList(&TypMapList, TypMapHash, TypMapCount);

		for ( epp = TypMapList, i = TypMapCount ; --i >= 0 ; epp++ )
		{
			if ( (ep = *epp)->MAP == NULLSTR || (ep->en_flags & EF_NOWRITE) || IntTypeName(ep->en_name) )
				continue;
			WrString(C_MAPTYP, ep->en_name);
			WrString(C_MAPVAL, ep->MAP);
		}

		for ( arp = AdvisoryRoutes ; arp != (AdvRoute *)0 ; arp = arp->ar_next )
		{
			bool	wr_region;

			if ( arp->ar_region->rg_route[FASTEST] == (Region *)0 )
				wr_region = true;
			else
				wr_region = false;

			if
			(
				(rp = arp->ar_route[FASTEST]) != NULL_ROUTE
				&&
				rp != CLEAR_ROUTE
				&&
				(rp->rg_flags & (RF_REMOVE|RF_LINK)) == RF_LINK
			)
			{
				if ( wr_region )
				{
					WrRegion(arp->ar_region);	/* Not written above */
					wr_region = false;
				}
				WrString(C_FASTROUTE, arp->ar_region->rg_name);
				WrString(C_MAPVAL, rp->rg_name);
			}
			if
			(
				(rp = arp->ar_route[CHEAPEST]) != NULL_ROUTE
				&&
				rp != CLEAR_ROUTE
				&&
				(rp->rg_flags & (RF_REMOVE|RF_LINK)) == RF_LINK
			)
			{
				if ( wr_region )
					WrRegion(arp->ar_region);	/* Not written above */
				WrString(C_CHEAPROUTE, arp->ar_region->rg_name);
				WrString(C_MAPVAL, rp->rg_name);
			}
		}
	}

	if ( AliasList == (Entry **)0 )
		AliasCount = MakeList(&AliasList, AliasHash, AliasCount);

	for ( epp = AliasList, i = AliasCount ; --i >= 0 ; epp++ )
	{
		if ( (ep = *epp)->MAP == NULLSTR || (ep->en_flags & EF_NOWRITE) )
			continue;
		if ( ep->MAP == HomeName )
			WrString(C_XALIAS, ep->en_name);
		else
		if ( StateType == local_state )
		{
			WrString(C_IALIAS, ep->en_name);
			WrString(C_MAPVAL, ep->MAP);
		}
	}

	/*
	**	Write CRC.
	*/

	i = 0;
	Buf[i++] = C_EOF|TOKEN_C;
	Buf[i++] = LOCRC(StCRC);
	Buf[i++] = HICRC(StCRC);

	(void)fwrite(Buf, i, 1, WrStateFd);

	putc('\n', WrStateFd);
	(void)fwrite(Version, strlen(Version), 1, WrStateFd);
	putc('\n', WrStateFd);

	(void)fflush(WrStateFd);

	if ( ferror(WrStateFd) )
		Error("Bad statefile write");
}



static void
WrRegion(rp)
	register Region *	rp;
{
	register NLink *	nlp;

	Trace2(2, "WrRegion(%s)", rp->rg_name);

	if ( StateType != local_state && (rp->rg_flags & RF_PSEUDO) )
		return;

	WrString(C_REGION, rp->rg_name);

	if ( rp->rg_visible != (Region *)0 )
		WrString(C_VISIBLE, rp->rg_visible->rg_name);
	else
	if ( StateType != local_state && rp->rg_links != (NLink *)0 )
		WrToken(C_VISIBLE);

	if ( StateType == local_state )
	{
		WrNumber(C_DATE, rp->rg_state);
		WrFlags(C_RFLAGS, (Ulong)(rp->rg_flags & ~RF_INTERNAL));

		if ( rp->rg_flags & RF_LINK )
		{
			register Link *	lp = (Link *)rp->rg_down;

			WrString(C_SPOOLER, lp->ln_spooler);
			WrString(C_CALLER, lp->ln_caller);
			WrString(C_FILTER, lp->ln_filter);

			if ( lp->ln_flags & FL_LINKNAME )
				WrString(C_LINKNAME, lp->ln_name);
		}
	}

	for ( nlp = rp->rg_links ; nlp != (NLink *)0 ; nlp = nlp->nl_next )
	{
		register Ulong		f;
		register Region *	lrp;

		if ( nlp->nl_to->rg_route[FASTEST] == (Region *)0 )
			continue;

		if ( StateType != local_state && (nlp->nl_flags & (FL_PSEUDO|FL_NOLINK)) )
			continue;

		if ( StateType == site_state && !(nlp->nl_flags & FL_HOME) )
			continue;

		WrString(C_LINK, nlp->nl_to->rg_name);

		if ( (f = (nlp->nl_flags & ((StateType==local_state)?(~FL_INTERNAL):(~FL_NOEXPORT)))) != 0 )
			WrFlags(C_LFLAGS, f);
		else
		if ( StateType != local_state && rp == HomeRegion )
			WrToken(C_LFLAGS);

		if ( (lrp = nlp->nl_restrict) != (Region *)0 )
			WrString(C_RESTRICT, lrp->rg_name);
		else
		if ( StateType != local_state && nlp->nl_to == HomeRegion )
			WrToken(C_RESTRICT);

		if ( (f = nlp->nl_cost) != 0 )
			WrNumber(C_COST, f);
		else
		if ( StateType != local_state && rp == HomeRegion )
			WrToken(C_COST);

		if ( (f = nlp->nl_delay) != 0 )
			WrNumber(C_DELAY, f);
		else
		if ( StateType != local_state && nlp->nl_to == HomeRegion )
			WrToken(C_DELAY);

/*		if ( (f = nlp->nl_speed) != 0 )
**			WrNumber(C_SPEED, f);
**		else
**		if ( StateType != local_state && rp == HomeRegion )
**			WrToken(C_SPEED);
**
**		if ( (f = nlp->nl_traffic) != 0 )
**			WrNumber(C_TRAFFIC, f);
**		else
**		if ( StateType != local_state && nlp->nl_to == HomeRegion )
**			WrToken(C_TRAFFIC);
*/	}
}



static void
#if	ANSI_C
WrFlags(char id, Ulong flags)
#else	/* ANSI_C */
WrFlags(id, flags)
	char		id;
	Ulong		flags;
#endif	/* ANSI_C */
{
	register long	s;
	register long	f;
	register char *	cp;
	register int	c;

	if ( (f = flags) == 0 )
		return;

	cp = Buf;
	*cp++ = id|TOKEN_C;

	for ( s = 1, c = 'A' ; f ; s <<= 1, c++ )
		if ( s & f )
		{
			*cp++ = c;
			f &= ~s;
		}

	StCRC = acrc(StCRC, Buf, cp-Buf);

	(void)fwrite(Buf, cp-Buf, 1, WrStateFd);
}



static void
#if	ANSI_C
WrNumber(char c, Ulong number)
#else	/* ANSI_C */
WrNumber(c, number)
	char		c;
	Ulong		number;
#endif	/* ANSI_C */
{
	register int	n;

	if ( number == 0 )
		return;

	Buf[0] = c|TOKEN_C;
	n = EncodeNum(&Buf[1], number, -1) + 1;

	StCRC = acrc(StCRC, Buf, n);

	(void)fwrite(Buf, n, 1, WrStateFd);
}



static void
WrOrdTyp()
{
	register int	len;

	Buf[0] = C_ORDERTYPES|TOKEN_C;
	len = WriteOrderTypes(&Buf[1]) + 1;

	StCRC = acrc(StCRC, Buf, len);

	(void)fwrite(Buf, len, 1, WrStateFd);
}



static void
#if	ANSI_C
WrString(char c, char *string)
#else	/* ANSI_C */
WrString(c, string)
	char		c;
	char *		string;
#endif	/* ANSI_C */
{
	register char *	s;
	register char *	cp;
	register char *	ep;

	if ( (s = string) == NULLSTR || (*s == '\0' && c != C_MAPVAL) )
		return;

	cp = Buf;
	ep = BufEnd;
	*cp++ = c|TOKEN_C;
	while ( (*cp++ = *s++) )
		if ( cp > ep )
			Fatal3("WrString(%d, %s)", (int)c, string);
	cp--;

	StCRC = acrc(StCRC, Buf, cp-Buf);

	(void)fwrite(Buf, cp-Buf, 1, WrStateFd);
}



static void
#if	ANSI_C
WrToken(char c)
#else	/* ANSI_C */
WrToken(c)
	char		c;
#endif	/* ANSI_C */
{
	Buf[0] = c|TOKEN_C;

	StCRC = acrc(StCRC, Buf, 1);

	putc(Buf[0], WrStateFd);
}
