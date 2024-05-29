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

#include	<setjmp.h>


/*
**	Default types for domains.
*/

typedef struct
{
	char *	dt_name;	/* Name of type */
	char *	dt_type;	/* Internal representation */
}
		DfltType;

static DfltType	DfltTypes[] =
{
	{"C",		"0"},	/* Mandatory */	/* Country */
	{"A",		"1"},	/* Mandatory */	/* Administrative management domain */
	{"P",		"2"},	/* Mandatory */	/* Private management domain */
	{"O",		"3"},	/* Mandatory */	/* Organisation */
	{"OU1",		"4"},			/* Organisational units... */
	{"OU2",		"5"},
	{"OU3",		"6"},
	{"OU4",		"7"},
	{"OU5",		"8"},
	{"N",		"9"}	/* Mandatory */	/* Node */
};

#define	NTYPES		((sizeof DfltTypes)/sizeof(DfltType))
#define	MINORDERTYPES	5

char		_ERROR_[]	= "_ERROR_";
int		ExpTypLens;		/* E{strlen(OrderTypes[i])-1} */
char *		OrderTypes[NTYPES];	/* Types exported */
jmp_buf		NameErrJmp;
bool		NoJmp;


char *		ExpCopy _FA_((char *, char *));
int		dm_comp _FA_((const void *, const void *));
int		tv_comp _FA_((const void *, const void *));



/*
**	Walk hash table tree placing domain maps into type tree.
*/

static void
BuildDomainMaps(ep)
	register Entry *	ep;
{
	register int		i;
	register TypeVec *	tvp;
	register TypeEl *	tep;
	register char *		cp;

	if ( ep->en_great != (Entry *)0 )
		BuildDomainMaps(ep->en_great);
		
	if ( ep->en_less != (Entry *)0 )
		BuildDomainMaps(ep->en_less);

	if ( (cp = ep->MAP) == NULLSTR )
		return;

	Trace3(2, "BuildDomainMaps(%s) => %s", ep->en_name, cp);
		
	for ( i = TypeCount, tvp = TypeTree ; --i >= 0 ; tvp++ )
		if ( tvp->tv_type[0] == cp[0] )
		{
			tep = Talloc(TypeEl);
			tep->tl_next = tvp->tv_list;
			tvp->tv_list = tep;
			tvp->tv_count++;
			MapEntries++;
			tep->tl_name = ep->en_name;
			tep->TMAP = cp+2;	/* Skip TYPE_SEP */
			tep->tl_what = wh_map;
			return;
		}

	Trace2(2, "BuildDomainMaps(%s) NOT MAPPED", ep->en_name);
}



/*
**	Walk `Fwd' tree placing `one below HOME' regions into type tree.
**
**	Select: `*.xx.our.region => link'.
*/

static void
BuildFwdVec(ep)
	register Entry *	ep;
{
	register int		i;
	register TypeVec *	tvp;
	register TypeEl *	tep;
	register char *		cp;
	register char *		dp;
	register char *		np;

	if ( ep->en_great != (Entry *)0 )
		BuildFwdVec(ep->en_great);
		
	if ( ep->en_less != (Entry *)0 )
		BuildFwdVec(ep->en_less);

	if ( (cp = ep->MAP) == NULLSTR )
		return;

	if ( (np = ep->en_name)[0] == '\0' )
		return;

	if ( Lookup(np, RegionHash) != (Entry *)0 )
		return;

	if
	(
		(dp = strchr(np, DOMAIN_SEP)) != NULLSTR
		&&
		(
			(ep = Lookup(++dp, RegionHash)) == (Entry *)0
			||
			!(ep->REGION->rg_flags & RF_HOME)
		)
	)
		return;

	if ( (ep = Lookup(cp, RegionHash)) == (Entry *)0 )
		return;

	Trace3(2, "BuildFwdVec(%s) => %s", np, cp);

	for ( i = TypeCount, tvp = TypeTree ; --i >= 0 ; tvp++ )
		if ( tvp->tv_type[0] == np[0] )
		{
			tep = Talloc(TypeEl);
			tep->tl_next = tvp->tv_list;
			tvp->tv_list = tep;
			tvp->tv_count++;
			MapEntries++;
			tep->tl_name = FirstName(np);
			tep->TREGION = ep->REGION;
			tep->tl_what = wh_none;		/* Signal to MakeTypeVector() */
			return;
		}

	Trace2(2, "BuildFwdVec(%s) NOT MAPPED", np);
}



/*
**	Make a tree to hold all types in home address.
*/

void
BuildTypeTree()
{
	register int		i;
	register Region *	rp;
	register TypeVec *	tvp;
	register Entry **	epp;
	register Entry **	end;

	/*
	**	Find number of levels in hierarchy.
	*/

	for ( i = 0, rp = HomeRegion ; rp != TopRegion ; rp = rp->rg_up )
		i++;

	TypeCount = i;
	MapEntries = 0;

	/*
	**	Build tree trunk for our hierarchy.
	*/

	Trace2(1, "BuildTypeTree() => %lu types", (PUlong)TypeCount);

	TypeTree = tvp = (TypeVec *)Malloc(i * sizeof(TypeVec));

	for ( rp = HomeRegion ; rp != TopRegion ; rp = rp->rg_up, tvp++ )
	{
		tvp->tv_list = (TypeEl *)0;
		tvp->tv_sorted = (TypeEl *)0;
		tvp->tv_count = 0;
		tvp->tv_type[0] = rp->rg_name[0];
		tvp->tv_type[1] = '\0';
		tvp->tv_name = NULLSTR;
	}

	/*
	**	Add branches for domain maps for all types in hierarchy.
	*/

	for ( epp = DomMapHash, end = &epp[HASH_SIZE] ; epp < end ; epp++ )
		if ( (rp = *(Region **)epp) != (Region *)0 )
			BuildDomainMaps((Entry *)rp);

	/*
	**	Add branches for forwarded regions in hierarchy.
	*/

	for ( epp = FwdMapHash, end = &epp[HASH_SIZE] ; epp < end ; epp++ )
		if ( (rp = *(Region **)epp) != (Region *)0 )
			BuildFwdVec((Entry *)rp);
}



/*
**	Check and map types to canonical types for region names.
**
**	Returns new string if ``buf'' == NULLSTR.
*/

char *
CanonicalName(name, buf)
	char *			name;
	char *			buf;
{
	register char *		cp;
	register char *		np;
	register int		c;
	register char *		mp;
	register char *		eb;
	register Entry *	ep;
	register int		len;
	register int		last_type;
	register bool		sort;
	register char **	dp;
	register bool		all_space;
	static char *		domains[NTYPES];
	static char		nbuf[TOKEN_SIZE+1];

	Trace3(3, "CanonicalName(%s, %#lx)", name, (PUlong)buf);
	DODEBUG(mp = np = EmptyString);

	if ( (cp = name) == NULLSTR )
	{
		c = (int)ne_null;
jump:
		DODEBUG(*np = '\0');
		Trace4(1, "CanonicalName ERR %s: mp=%s, cp=%s", IsCommand?" cmd":EmptyString, mp, cp)

		if ( !NoJmp )
			longjmp(NameErrJmp, c);

		return _ERROR_;
	}

	while ( (c = *(Uchar *)cp++) )
		if ( !LEGAL_C(c) )
		{
			c = (int)ne_illc;
			goto jump;
		}

	if ( (len = cp - name) <= 1 )
	{
		c = (int)ne_null;
		goto jump;
	}

	sort = false;
	dp = domains;

	cp = name;

	if ( IsCommand )
	{
		np = nbuf;
		len = TOKEN_SIZE+1;
	}
	else
	if ( (np = buf) == NULLSTR )
	{
		if ( ExpTypes )
			len += ExpTypLens;
		np = Malloc(len);
	}
	else
		len = TOKEN_SIZE+1;
	if ( ExpTypes )
		len -= ExpTypLens;
	eb = &np[len];

	name = mp = np;

	all_space = true;

	for ( last_type = 0x7f ;; )
	{
		switch ( *np++ = *cp++ )
		{
		case TYPE_SEP:
			if ( c )
			{
				c = (int)ne_illc;
				goto jump;
			}
			c = *mp++;
			if ( mp == np )
			{
				c = (int)ne_notype;
				goto jump;
			}
			if ( IsCommand )
			{
				*--np = '\0';
				if ( (ep = Lookup(--mp, TypMapHash)) != (Entry *)0 && ep->MAP != NULLSTR )
				{
					if ( dp >= &domains[NTYPES] )
					{
						c = (int)ne_illtype;
						goto jump;
					}
					if ( (c = ep->MAP[0]) >= last_type )
						sort = true;
					*dp++ = mp;
					*mp++ = c;
					*mp++ = TYPE_SEP;
					np = mp;
					last_type = c;
					continue;
				}
				if ( dp >= &domains[NTYPES] )
				{
					c = (int)ne_illtype;
					goto jump;
				}
				*dp++ = mp;
				*np++ = TYPE_SEP;
				++mp;
			}
			if
			(
				++mp != np
				||
				c < DfltTypes[0].dt_type[0]
				||
				c > DfltTypes[NTYPES-1].dt_type[0]
			)
			{
				c = (int)ne_unktype;
				goto jump;
			}
			if ( c >= last_type )
			{
				if ( !IsCommand )
				{
					c = (int)ne_illtype;
					goto jump;
				}
				sort = true;
			}
			last_type = c;
			continue;

		case '\0':
			cp = NULLSTR;
		case DOMAIN_SEP:
			if ( ++mp == np || all_space )
			{
				c = (int)ne_noname;
				goto jump;
			}
			all_space = true;
			if ( c == 0 )
			{
				if ( !IsCommand )
				{
					c = (int)ne_notype;
					goto jump;
				}
				*--np = '\0';
				if 
				(
					(
						(ep = Lookup(--mp, DomMapHash)) == (Entry *)0
						&&
						(ep = Lookup(mp, RegMapHash)) == (Entry *)0
					)
					||
					ep->MAP == NULLSTR
					||
					strchr(ep->MAP, TYPE_SEP) == NULLSTR
				)
				{
					c = (int)ne_notype;
					goto jump;
				}
				if ( (int)strlen(ep->MAP) >= (len-(int)(mp-name)) )
				{
					c = (int)ne_toolong;
					goto jump;
				}
				if
				(
					dp >= &domains[NTYPES]
					||
					strchr(ep->MAP, DOMAIN_SEP) != NULLSTR
				)
				{
					c = (int)ne_illtype;
					goto jump;
				}
				*dp++ = mp;
				if ( ep->MAP[0] >= last_type )
					sort = true;
				last_type = ep->MAP[0];
				mp = strcpyend(mp, ep->MAP);
				if ( cp == NULLSTR )
					break;
				*mp++ = DOMAIN_SEP;
				np = mp;
			}
			else
			if ( cp == NULLSTR )
				break;
			else
				mp = np;
			c = 0;
			continue;

		default:
			all_space = false;
		case ' ':
			if ( np >= eb )
			{
				c = (int)ne_toolong;
				goto jump;
			}
			continue;
		}

		break;
	}

	if ( IsCommand )
	{
		if ( (cp = buf) == NULLSTR )
		{
			c = np - name;
			if ( ExpTypes )
				c += ExpTypLens;
			name = Malloc(c);
		}
		else
			name = cp;

		if ( sort )
		{
			c = dp - domains;
			while ( --dp > domains )
				(*dp)[-1] = '\0';
			qsort((char *)domains, c, sizeof(char *), dm_comp);
			for ( cp = name ; --c >= 0 ; )
			{
				np = *dp++;
				if ( ExpTypes )
				{
					cp = strcpyend(cp, OrderTypes[np[0]-DfltTypes[0].dt_type[0]]);
					np += 2;
				}
				else
				if ( NoTypes )
					np += 2;	/* Skip internal type */
				cp = strcpyend(cp, np);
				*cp++ = DOMAIN_SEP;
			}
			*--cp = '\0';
		}
		else
		{
			if ( NoTypes )
				(void)StripCopy(name, nbuf);
			else
			if ( ExpTypes )
				(void)ExpCopy(name, nbuf);
			else
				(void)strcpy(name, nbuf);
		}
	}
	else
	if ( NoTypes )
		(void)StripCopy(name, name);
	else
	if ( ExpTypes )
		(void)strcpy(name, ExpCopy(nbuf, name));

	Trace2(3, "CanonicalName() ==> \"%s\"", name);

	return name;
}



/*
**	Compare two typed domains.
*/

int
#ifdef	ANSI_C
dm_comp(const void *cpp1, const void *cpp2)
#else	/* ANSI_C */
dm_comp(cpp1, cpp2)
	char *	cpp1;
	char *	cpp2;
#endif	/* ANSI_C */
{
	if ( **((char **)cpp1) > **((char **)cpp2) )
		return -1;
	if ( **((char **)cpp1) < **((char **)cpp2) )
		return 1;
	longjmp(NameErrJmp, (int)ne_illtype);	/* No return */
	return 0;
}



/*
**	Expand types from "string" to new copy of string in "copy".
*/

char *
ExpCopy(copy, string)
	char *		copy;
	char *		string;
{
	register int	c;
	register char *	sp;
	register char *	cp;
	register char *	tp;

	Trace3(5, "ExpCopy(%#lx, %s)", (PUlong)copy, string);

	sp = string;
	cp = tp = copy;

	do
	{
		if ( (c = *sp++) == TYPE_SEP )
			cp = strcpyend(tp, OrderTypes[tp[0]-DfltTypes[0].dt_type[0]]);

		if ( (*cp++ = c) == DOMAIN_SEP )
			tp = cp;
	}
	while
		( c != '\0' );

	Trace3(4, "ExpCopy(%s) ==> %s", string, copy);

	return copy;
}



/*
**	Return first domain in string.
*/

char *
FirstDomain(acp)
	char *		acp;
{
	register char *	cp;
	register char *	cp1;

	if ( (cp = acp) == NULLSTR )
		return newstr(EmptyString);

	if ( (cp1 = strchr(cp, DOMAIN_SEP)) == NULLSTR )
		return newstr(cp);

	*cp1 = '\0';
	cp = newstr(cp);
	*cp1 = DOMAIN_SEP;

	return cp;
}



/*
**	Return name of first domain in string.
*/

char *
FirstName(acp)
	char *		acp;
{
	register char *	cp;
	register char *	cp1;
	register char *	cp2;

	if ( (cp = acp) == NULLSTR )
		return newstr(EmptyString);

	if ( (cp2 = strchr(cp, DOMAIN_SEP)) == NULLSTR )
	{
		if ( (cp1 = strchr(cp, TYPE_SEP)) == NULLSTR )
			return newstr(cp);

		return newstr(++cp1);
	}

	*cp2 = '\0';

	if ( (cp1 = strchr(cp, TYPE_SEP)) == NULLSTR )
		cp1 = cp;
	else
		cp1++;

	cp = newstr(cp1);

	*cp2 = DOMAIN_SEP;

	return cp;
}



/*
**	Initialise type map and exported type names with defaults.
*/

void
InitTypeMap()
{
	register DfltType *	tp;
	register Entry *	ep;
	register char **	cpp;
	register int		i;

	Trace1(1, "InitTypeMap");

	ExpTypLens = 0;

	for ( cpp = OrderTypes, tp = DfltTypes, i = NTYPES ; --i >= 0 ; tp++ )
	{
		ep = Enter(tp->dt_name, TypMapHash);
		ep->MAP = tp->dt_type;
		*cpp++ = tp->dt_name;
		ExpTypLens += strlen(tp->dt_name)-1;
	}

	MaxTypes = NTYPES;
}



/*
**	Check if passed string is an internal type.
*/

char *
InternalType(cp)
	register char *		cp;
{
	register DfltType *	tp;
	register int		i;

	Trace2(3, "InternalType(%s)", cp);

	if ( cp[1] == '\0' )
	for ( tp = DfltTypes, i = NTYPES ; --i >= 0 ; tp++ )
		if ( cp[0] == tp->dt_type[0] )
			return tp->dt_type;

	return NULLSTR;
}



/*
**	Check if passed string is an internal type name.
*/

bool
IntTypeName(cp)
	register char *		cp;
{
	register DfltType *	tp;
	register int		i;

	for ( tp = DfltTypes, i = NTYPES ; --i >= 0 ; tp++ )
		if ( strccmp(cp, tp->dt_name) == STREQUAL )
			return true;

	return false;
}



/*
**	Sort regions and mappings into TypeTree.
*/

Index
MakeTypeTree(tvp, arp)
	TypeVec *		tvp;
	Region *		arp;
{
	register int		i;
	register Region *	rp;
	register TypeEl *	tep;
	register TypeEl *	tlp;
	register char *		cp;
	register Entry *	ep;
	register int		j;
	Index			count;

	if ( arp != TopRegion )
		count = MakeTypeTree(tvp+1, arp->rg_up);
	else
		count = 0;

	Trace2(2, "MakeTypeTree(%s)", arp->rg_name);

	i = tvp->tv_count;

	for ( rp = arp->rg_down ; rp != (Region *)0 ; rp = rp->rg_next )
		if ( rp->rg_route[FASTEST] != (Region *)0 )
			i++;

	if ( i > 0 )
		tvp->tv_sorted = tep = (TypeEl *)Malloc(i * sizeof(TypeEl));
	else
		tvp->tv_sorted = tep = (TypeEl *)0;

	for ( tlp = tvp->tv_list ; tlp != (TypeEl *)0 ; tlp = tlp->tl_next )
		*tep++ = *tlp;

	for ( rp = arp->rg_down ; rp != (Region *)0 ; rp = rp->rg_next )
	{
		cp = FirstName(rp->rg_name);

		if ( rp->rg_flags & RF_HOME )
			tvp->tv_name = cp;

		if ( rp->rg_route[FASTEST] == (Region *)0 )
			continue;

		if
		(
			(ep = Lookup(cp, DomMapHash)) != (Entry *)0
			&&
			ep->MAP != NULLSTR
			&&
			ep->MAP[0] == tvp->tv_type[0]
		)
		{
			Warn(english("map (%s => %s) discarded, region \"%s\""), cp, ep->MAP, rp->rg_name);

			for ( tlp = tvp->tv_sorted, j = tvp->tv_count ; --j >= 0 ; tlp++ )
				if ( strccmp(tlp->tl_name, cp) == STREQUAL )
				{
					tlp->tl_name = cp;
					tlp->TREGION = rp;
					tlp->tl_what = wh_noroute;
					i--;
					break;
				}

			DODEBUG(if(j<0)Fatal("duplicate map \"%s\" not found", cp));
			continue;
		}

		tep->tl_name = cp;

		tep->TREGION = rp;
		tep->tl_what = wh_noroute;

		tep++;
	}

	if ( (tvp->tv_count = i) > 1 )
		qsort((char *)tvp->tv_sorted, i, sizeof(TypeEl), tv_comp);

	return count + i;
}



/*
**	If map is for a legal domain, add to domain map hash.
**
**	EG: MapDomain("australia", "0=au")
*/

Entry *
MapDomain(map, domain)
	char *		map;
	char *		domain;
{
	register char *	cp;
	register Entry *ep;

	Trace3(3, "MapDomain(%s, %s)", map, domain);

	if ( (cp = map) == NULLSTR || *cp == '\0' || strchr(cp, DOMAIN_SEP) != NULLSTR )
		return (Entry *)0;

	if ( strchr(domain, DOMAIN_SEP) != NULLSTR )
		return (Entry *)0;

	if ( (cp = strchr(domain, TYPE_SEP)) == NULLSTR )
		return (Entry *)0;

	if ( strccmp(map, cp+1) == STREQUAL )
		return (Entry *)0;

	*cp = '\0';

	if ( InternalType(domain) == NULLSTR )
	{
		*cp = TYPE_SEP;
		return (Entry *)0;
	}

	*cp = TYPE_SEP;

	if ( (ep = Lookup(map, DomMapHash)) == (Entry *)0 )
	{
		ep = Enter(newstr(map), DomMapHash);
		ep->MAP = newstr(domain);
	}
	else
	if ( ep->MAP == NULLSTR )
		ep->MAP = newstr(domain);
	else
	if ( strcmp(ep->MAP, domain) != STREQUAL )
		return (Entry *)0;

	Trace3(2, "MapDomain(%s, %s)", map, domain);

	return ep;
}



/*
**	Parse argument to "ordertypes" command.
**
**	minimum is:-
**		"<type0>;<type1>;<type2>;<type4>...;<node>"
*/

char *
ParseOrderTypes(o)
	char *		o;
{
	register int	c;
	register char *	lp;
	register char *	cp;
	register char *	np;
	register Entry *ep;
	register int	count;
	char		type[2];

	static char	toofew[]	= english("too few types");
	static char	illtyp[]	= english("illegal type");

	if ( (np = o) == NULLSTR || *np == '\0' )
		return toofew;

	Trace2(1, "ParseOrderTypes(%s)", np);

	type[0] = DfltTypes[0].dt_type[0];
	type[1] = '\0';

	for ( count = 0, np = o ; (cp = np) != NULLSTR ; type[0]++ )
	{
		if ( ++count > NTYPES )
			return english("too many types");

		if ( (np = strchr(cp, ORDER_C)) == NULLSTR )
		{
			if ( count < MINORDERTYPES )
				return toofew;
			count = NTYPES;
			type[0] = DfltTypes[NTYPES-1].dt_type[0];
		}
		else
			*np++ = '\0';

		if ( *cp == '\0' )
			return illtyp;

		for ( lp = cp ; (c = *lp++) ; )
			if ( !LEGAL_C(c) )
				longjmp(NameErrJmp, (int)ne_illc);

		if ( InternalType(cp) != NULLSTR )
			return illtyp;

		if ( (ep = Lookup(cp, TypMapHash)) == (Entry *)0 )
			ep = Enter(newstr(cp), TypMapHash);

		ExpTypLens -= strlen(OrderTypes[count-1])-1;
		ExpTypLens += strlen(ep->en_name)-1;

		OrderTypes[count-1] = ep->en_name;

		if ( ep->MAP == NULLSTR )
			ep->MAP = newstr(type);
		else
		if ( ep->MAP[0] != type[0] )
			return illtyp;
	}

	return NULLSTR;
}



/*
**	Strip types from "string" to new copy of string in "copy".
*/

char *
StripCopy(copy, string)
	char *		copy;
	char *		string;
{
	register int	c;
	register char *	sp;
	register char *	cp;
	register char *	tp;

	Trace3(5, "StripCopy(%#lx, %s)", (PUlong)copy, string);

	sp = string;
	cp = tp = copy;

	do
		if ( (c = *sp++) == TYPE_SEP )
			cp = tp;
		else
		if ( (*cp++ = c) == DOMAIN_SEP )
			tp = cp;
	while
		( c != '\0' );

	Trace3(4, "StripCopy(%s) ==> %s", (copy==string)?EmptyString:string, copy);

	return copy;
}



/*
**	Sort types by domain name.
*/

int
#ifdef	ANSI_C
tv_comp(const void *epp1, const void *epp2)
#else	/* ANSI_C */
tv_comp(epp1, epp2)
	char *		epp1;
	char *		epp2;
#endif	/* ANSI_C */
{
	register int	val;

	val = strccmp(((TypeEl *)epp1)->tl_name, ((TypeEl *)epp2)->tl_name);

	if ( val == STREQUAL )
		Warn(english("Identical names in type tree \"%s\""), ((TypeEl *)epp1)->tl_name);

	return val;
}



/*
**	Make OrderTypes string for statefile.
*/

int
WriteOrderTypes(buf)
	char *		buf;
{
	register char *	cp;
	register int	i;

	for ( cp = buf, i = 0 ; i < NTYPES ; i++ )
	{
		cp = strcpyend(cp, OrderTypes[i]);
		*cp++ = ORDER_C;
	}

	*--cp = '\0';

	return cp - buf;
}
