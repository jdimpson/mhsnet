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
#include	"routefile.h"


static ExpType	SetType;

extern int	MinTypC;
extern int	MaxTypC;

/*
**	Structure caches.
*/

static Addr *	addrfreelist;
static Dest *	destfreelist;

/*
**	Routines for handling address groups.
*/

static char *	addrintostr _FA_((Addr *, char *));
static int	addrstrlen _FA_((Addr *));
static char *	destintostr _FA_((Dest *, char *));
static int	deststrlen _FA_((Dest *));
static void	freeaddr _FA_((Addr *));
static Addr *	newaddr _FA_((void));
static void	sort_types _FA_((Dest *));



/*
**	Add address group to previous group.
*/

void
AddAddr(app, np)
	Addr **		app;
	Addr *		np;
{
	register Addr *	ap;
	register Addr *	p1;
	register Addr *	p2;
	register int	count;

	Trace3(2, "AddAddr(%s, %s)", ADDRNAME(*app), ADDRNAME(np));

	if ( (p2 = *app) == (Addr *)0 )
	{
		if ( (p1 = np) != (Addr *)0 )
		{
			p1->ad_up = (Addr *)app;
			*app = p1;
		}
		return;
	}

	if ( (p1 = np) == (Addr *)0 )
	{
		p2->ad_up = (Addr *)app;
		return;
	}

	DODEBUG(
		if ( p1->ad_down == (Addr *)0 && p1->ad_next == (Addr *)0 )
			Fatal1("AddAddr free'd Addr");
	);

	for ( count = 0 ;; )
	{
		if ( !MLTITYPE(p1) )
		{
			ap = newaddr();
			ap->ad_down = p1;
			p1->ad_up = ap;
			p1 = ap;
			ADDRTYPE(p1) = ATYP_MULTICAST;
		}

		Trace3(3, "AddAddr(%s, %s)", ADDRNAME(p1), ADDRNAME(p2));

		ap = p1;

		if ( count++ )
			break;

		p1 = p2;
		p2 = ap;
	}

	while ( ap->ad_next != (Addr *)0 )
		ap = ap->ad_next;

	ap->ad_next = p2;
	p2->ad_prev = ap;

	p1->ad_up = (Addr *)app;
	*app = p1;

	FreeStr(&(*app)->ad_typed);
	FreeStr(&(*app)->ad_untyped);
}



/*
**	Make a string representing address group.
*/

char *
AddrToString(aa, type)
	Addr *		aa;
	ExpType		type;
{
	register Addr *	ap;
	register char *	cp;
	register char **cpp;
	register int	len;
	char *		maptyped;

	Trace3(3, "AddrToString(%s, %s)", ADDRNAME(aa), (type==maptype)?"+TYPE":(type==intype)?"+type":"-type");

	if ( (ap = aa) == (Addr *)0 )
		return (type==maptype) ? newstr(EmptyString) : EmptyString;

	switch ( SetType = type )
	{
	case notype:
		cpp = &ap->ad_untyped;	/* Kept */
		break;
	case intype:
		cpp = &ap->ad_typed;	/* Kept */
		break;
	default:			/* To quieten gcc -Wall */
	case maptype:
		maptyped = NULLSTR;
		cpp = &maptyped;	/* Exported */
		break;
	}

	if ( (cp = *cpp) == NULLSTR )
	{
		if ( (len = addrstrlen(ap)) == 0 )
		{
			if ( type != maptype )
				return EmptyString;
			else
				return newstr(EmptyString);
		}

		*cpp = cp = Malloc(len+1);

		addrintostr(ap, cp)[0] = '\0';
	}

	Trace2(3, "AddrToString ==> %s", cp);

	return cp;
}



static char *
addrintostr(aa, cp)
	Addr *		aa;
	register char *	cp;
{
	register Addr *	ap;
	register Addr *	np;
	register bool	type_flag;

	Trace3(4, "addrintostr(%s, %#lx)", ADDRNAME(aa), (PUlong)cp);

	if ( (ap = aa) == (Addr *)0 )
		return cp;

	if ( DESTTYPE(ap) || BCSTTYPE(ap) )
		type_flag = true;
	else
	{
		type_flag = false;

		while ( ap->ad_down == (Addr *)0 )
			if ( (ap = ap->ad_next) == (Addr *)0 )
				return cp;

		for ( np = ap->ad_next ; np != (Addr *)0 ; np = np->ad_next )
			if ( np->ad_down != (Addr *)0 )
			{
				type_flag = true;
				break;
			}
	}

	do
	{
		while ( ap->ad_down == (Addr *)0 )
			if ( (ap = ap->ad_next) == (Addr *)0 )
				return cp;

		if ( type_flag )
			*cp++ = ADDRTYPE(ap);

		if ( !DESTTYPE(ap) )
			cp = addrintostr(ap->ad_down, cp);
		else
		if ( SetType == notype )
			cp = strcpyend(cp, ADDRDEST(ap)->dt_name);
		else
		if ( SetType == intype )
			cp = strcpyend(cp, ADDRDEST(ap)->dt_tname);
		else
			cp = destintostr(ADDRDEST(ap), cp);
	}
	while
		( (ap = ap->ad_next) != (Addr *)0 );

	return cp;
}



static int
addrstrlen(aa)
	Addr *		aa;
{
	register Addr *	ap;
	register Addr *	np;
	register int	len = 0;
	register int	type_flag;

	Trace2(6, "addrstrlen(%s)", ADDRNAME(aa));

	if ( (ap = aa) != (Addr *)0 )
	{
		if ( DESTTYPE(ap) || BCSTTYPE(ap) )
			type_flag = 1;
		else
			type_flag = 0;

		do
		{
			while ( ap->ad_down == (Addr *)0 )
				if ( (ap = ap->ad_next) == (Addr *)0 )
					goto out;

			if ( (np = ap->ad_next) != (Addr *)0 )
				len += type_flag = 1;

			if ( !DESTTYPE(ap) )
				len += addrstrlen(ap->ad_down);
			else
			if ( SetType == notype )
				len += strlen(ADDRDEST(ap)->dt_name);
			else
			if ( SetType == intype )
				len += strlen(ADDRDEST(ap)->dt_tname);
			else
				len += deststrlen(ADDRDEST(ap));
		}
		while
			( (ap = np) != (Addr *)0 );
	}
	else
out:		type_flag = 0;

	Trace3(5, "addrstrlen(%s) ==> %d", ADDRNAME(aa), len + type_flag);

	return len + type_flag;
}



/*
**	Return canonical version of string.
*/

char *
CanonString(a)
	char *	a;
{
	char *	vp;
	char *	cp = newstr(a);
	Addr *	ap = StringToAddr(cp, NO_STA_MAP);

	vp = newstr(TypedAddress(ap));
	FreeAddr(&ap);
	free(cp);
	return vp;
}



/*
**	Clone an address group (but don't copy destinations.)
*/

Addr *
CloneAddr(aa)
	Addr *		aa;
{
	register Addr *	ap;
	register Addr *	nap;

	Trace2(3, "CloneAddr(%s)", ADDRNAME(aa));

	if ( (ap = aa) == (Addr *)0 )
		return (Addr *)0;

	while ( ap->ad_down == (Addr *)0 )
		if ( (ap = ap->ad_next) == (Addr *)0 )
			return (Addr *)0;

	nap = newaddr();

	ADDRTYPE(nap) = ADDRTYPE(ap);

	if ( DESTTYPE(ap) )
	{
		nap->ad_down = ap->ad_down;
		nap->ad_flags |= AD_CLONED;
	}
	else
	if ( (nap->ad_down = CloneAddr(ap->ad_down)) != (Addr *)0 )
		nap->ad_down->ad_up = nap;

	if ( (ap = ap->ad_next) == (Addr *)0 )
		return nap;

	if ( (nap->ad_next = CloneAddr(ap)) != (Addr *)0 )
		nap->ad_next->ad_prev = nap;

	return nap;
}



static char *
destintostr(dp, cp)
	Dest *		dp;
	register char *	cp;
{
	register Dname *np;
	register int	i;
	register int	t;

	Trace3(4, "destintostr(%s, %#lx)", DESTNAME(dp), (PUlong)cp);

	if ( dp == (Dest *)0 || (i = dp->dt_count) == 0 )
	{
		*cp = '\0';
		return cp;
	}

	for ( np = dp->dt_names ; --i >= 0 ; np++ )
	{
		if ( SetType != notype )
		{
			if ( SetType == intype )
			{
				if ( (t = np->dn_type) != '\0' )
				{
					*cp++ = t;
					*cp++ = TYPE_SEP;
				}
			}
			else
			if ( np->dn_tname != NULLSTR )
			{
				cp = strcpyend(cp, np->dn_tname);
				*cp++ = TYPE_SEP;
			}
		}

		cp = strcpyend(cp, np->dn_name);

		*cp++ = DOMAIN_SEP;
	}

	*--cp = '\0';

	return cp;
}



static int
deststrlen(dp)
	Dest *		dp;
{
	register Dname *np;
	register int	i;
	register int	len;
	register int	t;

	Trace2(6, "deststrlen(%s)", DESTNAME(dp));

	if ( dp == (Dest *)0 || (i = dp->dt_count) == 0 )
		len = 0;
	else
	{
		len = -1;

		for ( np = dp->dt_names ; --i >= 0 ; np++ )
		{
			if ( SetType != notype )
			{
				if ( SetType == intype )
				{
					if ( np->dn_type != '\0' )
						len += 2;
				}
				else
				if
				(
					np->dn_tname != NULLSTR
					||
					(
						(t = np->dn_type) != '\0'
						&&
						(np->dn_tname = ExportType(t)) != NULLSTR
					)
				)
					len += strlen(np->dn_tname) + 1;
			}

			len += strlen(np->dn_name) + 1;
		}
	}

	Trace3(5, "deststrlen(%s) ==> %d", DESTNAME(dp), len);

	return len;
}



/*
**	Return exported typed address.
**	Returns a new string.
*/

char *
ExpTypAddress(aa)
	Addr *		aa;
{
	register Addr *	ap;
	register char *	cp;

	if ( (ap = aa) == (Addr *)0 )
		return newstr(EmptyString);

	cp = AddrToString(ap, maptype);

	return cp;
}



/*
**	Return exported typed name if address is a single destination, or empty string.
**	Returns a new string.
*/

char *
ExpTypName(aa)
	Addr *		aa;
{
	register Addr *	ap;
	register char *	cp;

	if ( (ap = aa) == (Addr *)0 || !DESTTYPE(ap) )
		return newstr(EmptyString);

	cp = AddrToString(ap, maptype);

	return strcpy(cp, cp+1);
}



/*
**	Extract an address group.
*/

Addr *
ExtractAddr(aap)
	Addr **		aap;
{
	register Addr *	ap;
	register Addr *	np;

	Trace2(3, "ExtractAddr(%s)", ADDRNAME(*aap));

	if ( (ap = *aap) == (Addr *)0 )
		return ap;

	if ( (np = ap->ad_prev) != (Addr *)0 )
	{
		np->ad_next = ap->ad_next;
		ap->ad_next = (Addr *)0;
		ap->ad_prev = (Addr *)0;
	}
	else
	if ( (np = ap->ad_up) != (Addr *)0 )
	{
		np->ad_down = (Addr *)0;
		ap->ad_up = (Addr *)0;
	}
	else
		*aap = (Addr *)0;

	return ap;
}



/*
**	Return first domain's untyped name if address is a single destination, or EmptyString.
*/

char *
FirstUnTypedName(aa)
	Addr *		aa;
{
	register Addr *	ap;

	if ( (ap = aa) == (Addr *)0 || !DESTTYPE(ap) )
		return EmptyString;

	return ADDRDEST(ap)->dt_names->dn_name;
}



/*
**	Free an address group.
*/

void
FreeAddr(app)
	Addr **		app;
{
	register Addr *	ap;
	register Addr *	np;

	if ( app == (Addr **)0 || (ap = *app) == (Addr *)0 )
		return;

	Trace2(4, "FreeAddr(%s)", ADDRNAME(ap));

	*app = (Addr *)0;

	do
	{
		np = ap->ad_next;

		if ( !DESTTYPE(ap) )
			FreeAddr(&ap->ad_down);
		else
		if ( !(ap->ad_flags & AD_CLONED) )
			FreeDest(ADDRDESTP(ap));

		if ( ap->ad_string != NULLSTR )
			free(ap->ad_string);

		if ( ap->ad_typed != NULLSTR )
			free(ap->ad_typed);

		if ( ap->ad_untyped != NULLSTR )
			free(ap->ad_untyped);

		freeaddr(ap);
	}
	while
		( (ap = np) != (Addr *)0 );
}



/*
**	Free an Addr struct.
*/

static void
freeaddr(ap)
	Addr *	ap;
{
	ap->ad_up = addrfreelist;
	addrfreelist = ap;
}



/*
**	Free a destination.
*/

void
FreeDest(dpp)
	Dest **		dpp;
{
	register Dest *	dp;

	if ( (dp = *dpp) == (Dest *)0 )
		return;

	Trace2(4, "FreeDest(%s)", DESTNAME(dp));

	*dpp = (Dest *)0;

	if ( dp->dt_tname != NULLSTR )
		free(dp->dt_tname);

	if ( dp->dt_name != NULLSTR )
		free(dp->dt_name);

	if ( dp->dt_string != NULLSTR )
		free(dp->dt_string);

	if ( dp->dt_names != dp->dt_parts )
		free((char *)dp->dt_names);

	SET_NEXTDEST(dp, destfreelist);
	destfreelist = dp;
}



static Addr *
newaddr()
{
	register Addr *	ap;

	if ( (ap = addrfreelist) != (Addr *)0 )
	{
		addrfreelist = ap->ad_up;
		(void)strclr((char *)ap, sizeof(Addr));
		return ap;
	}

	return Talloc0(Addr);
}



/*
**	Remove an address group.
*/

void
RemoveAddr(aap)
	Addr **	aap;
{
	Addr *	ap;

	if (aap == (Addr **)0 )
	{
		Trace2(3, "RemoveAddr(%s)", NullStr);
		return;
	}

	Trace2(3, "RemoveAddr(%s)", ADDRNAME(*aap));

	ap = ExtractAddr(aap);

	FreeAddr(&ap);
}



/*
**	Bubble sort on usually sorted types.
*/

static void
sort_types(dp)
	Dest *		dp;
{
	register Dname *np1;
	register Dname *np2;
	register int	i;
	register bool	resort;

	Trace2(4, "sort_types(%s)", DESTNAME(dp));

	do
	{
		resort = false;

		for
		(
			np1 = dp->dt_names, np2 = np1+1, i = dp->dt_count ;
			--i > 0 ;
			np1 = np2++
		)
			if ( np1->dn_type < np2->dn_type && np1->dn_type != '\0' )
			{
				Dname	temp;

				temp = *np1;
				*np1 = *np2;
				*np2 = temp;

				resort = true;

				Trace3(4, "swap %s and %s", np2->dn_name, np1->dn_name);
			}
	}
	while
		( resort );
}



/*
**	Split (typed) destination string into a Dest (modifies argument.)
*/

Dest *
SplitDest(dest)
	register char *	dest;
{
	register char *	cp;
	register Dname *np;
	register Dest *	dp;
	register int	i;
	register char *	tp;

	Trace2(3, "SplitDest(%s)", dest);

	if ( (dp = destfreelist) != (Dest *)0 )
		destfreelist = NEXTDEST(dp);
	else
		dp = Talloc(Dest);

	if ( RouteBase == NULLSTR && !ReadRoute(NOSUMCHECK) )
	{
		(void)strclr((char *)dp, sizeof(Dest));
		return dp;
	}

	if ( (cp = strrchr(dest, DOMAIN_SEP)) != NULLSTR && cp[1] == '\0' )
		*cp = '\0';	/* Ignore trailing `.' */

	DODEBUG(dp->dt_name = dest);

	/*
	**	Try direct mapping.
	*/

	if
	(
		strchr(dest, TYPE_SEP) == NULLSTR
		&&
		(
			(cp = SearchNames(dest)) != NULLSTR
			||
			(
				(cp = MapRegion(dest)) != NULLSTR
				&&
				(tp = strrchr(cp, DOMAIN_SEP)) != NULLSTR
				&&
				tp[2] == TYPE_SEP
				&&
				tp[1] == MinTypC
			)
		)
	)
	{
		dest = newstr(cp);
		dp->dt_string = dest;
	}
	else
		dp->dt_string = NULLSTR;

	/*
	**	Break out domains.
	*/

remap:
	if ( (cp = dest) == NULLSTR )
		dest = cp = EmptyString;

	for ( i = 1 ; (cp = strchr(cp, DOMAIN_SEP)) != NULLSTR ; i++ )
		cp++;

	if ( (dp->dt_count = i) > MAXPARTS )
		dp->dt_names = np = (Dname *)Malloc(i * sizeof(Dname));
	else
		dp->dt_names = np = dp->dt_parts;

	for ( ; (cp = dest) != NULLSTR ; np++ )
	{
		if ( (dest = strchr(cp, DOMAIN_SEP)) != NULLSTR )
			*dest++ = '\0';

		Trace2(5, "domain \"%s\"", cp);

		np->dn_tname = cp;

		if ( (cp = strchr(cp, TYPE_SEP)) != NULLSTR )
		{
			*cp++ = '\0';

			np->dn_name = cp;

			if ( dp->dt_string != NULLSTR )
			{
				np->dn_type = np->dn_tname[0];
				np->dn_tname = NULLSTR;
				Trace3(4, "domain \"%c=%s\"", np->dn_type, cp);
				continue;
			}

			if ( cp[0] == '\0' || (cp[0] == ' ' && cp[1] == '\0') )
			{
				np--;	/* Ignore null name */
				dp->dt_count--;
				Trace2(4, "domain %s", NullStr);
				continue;
			}

			if
			(
				(cp - np->dn_tname) == 2
				&&
				(i = np->dn_tname[0]) <= MaxTypC
				&&
				i >= MinTypC
			)
			{
				np->dn_type = i;
				np->dn_tname = NULLSTR;
			}
			else
			if ( (cp = MapType(np->dn_tname)) != NULLSTR )
			{
				np->dn_type = cp[0];
				np->dn_tname = NULLSTR;
			}
			else
				np->dn_type = '\0';
				
			Trace3(
				4,
				"domain \"%c=%s\"",
				np->dn_type=='\0'?'?':np->dn_type,
				np->dn_name
			);
			continue;
		}

		np->dn_name = np->dn_tname;
		np->dn_type = '\0';
		np->dn_tname = NULLSTR;

		Trace2(4, "domain \"?=%s\"", np->dn_name);
	}

	/*
	**	Sort and fill missing types.
	*/

	sort_types(dp);
	MatchTypes(dp);	/* Fill missing types */
	sort_types(dp);

	/*
	**	Make up canonical strings.
	*/

	SetType = notype;
	dp->dt_name = cp = Malloc(deststrlen(dp)+1);
	DODEBUG((void)strcpy(cp, dp->dt_names->dn_name));
	(void)destintostr(dp, cp);

	SetType = intype;
	dp->dt_tname = tp = Malloc(deststrlen(dp)+1);
	(void)destintostr(dp, tp);

	Trace2(3, "SplitDest ==> %s", tp);

	/*
	**	Match any region mapping.
	**
	**	(No recursion!)
	*/

	if ( dp->dt_string == NULLSTR && (dest = MapRegion(cp)) != NULLSTR )
	{
domap:		free(dp->dt_tname);
		free(dp->dt_name);
		DODEBUG(dp->dt_name = dp->dt_names->dn_name);

		dp->dt_count = 0;
		dp->dt_string = newstr(dest);

		if ( dp->dt_names != dp->dt_parts )
			free((char *)dp->dt_names);

		dest = dp->dt_string;
		goto remap;
	}

	/*
	**	If only typed part matches visible region, try mapping prefix only.
	*/

	while ( dp->dt_string == NULLSTR )		/* No recursion! */
	{
		tp = dp->dt_tname;

		if ( (cp = strchr(tp, TYPE_SEP)) == NULLSTR )
			break;

		if ( --cp == tp )
			break;

		if ( strccmp(cp, VisibleName) != STREQUAL )
			break;

		*--cp = '\0';	/* DOMAIN_SEP */

		if
		(
			(dest = MapRegion(tp)) != NULLSTR
			&&
			(tp = strchr(dest, VisibleName[0])) != NULLSTR
			&&
			strcmp(tp, VisibleName) == STREQUAL
		)
			goto domap;

		*cp = DOMAIN_SEP;
		break;
		/* NOTREACHED */
	}

	return dp;
}



/*
**	Return true if string contains a home destination.
*/

bool
StringAtHome(s)
	char *		s;
{
	register char *	cp;
	register bool	val;
	Addr *		ap;

	Trace2(2, "StringAtHome(%s)", s);

	if ( (cp = s) == NULLSTR || *cp == '\0' )
		return false;

	ap = StringToAddr(cp = newstr(cp), false);

	val = AddrAtHome(&ap, false, true);

	FreeAddr(&ap);
	free(cp);

	Trace2(2, "StringAtHome => %s", val?"TRUE":"FALSE");

	return val;
}



/*
**	Convert string into address group (modifies argument.)
*/

Addr *
StringToAddr(s, noremap)
	char *		s;
	bool		noremap;
{
	register char *	cp;
	register Addr *	ap;
	register char *	np;
	register int	c;
	register char *	tp;
	Addr *		aa;
	bool		forwarded;

	/** NB: last two chars of atypes assumed below **/

	static char	atypes[] = {ATYP_MULTICAST, ATYP_EXPLICIT, '\0'};

	Trace3(3, "StringToAddr(%s, %sremap)", (s==NULLSTR)?NullStr:s, noremap?"no":EmptyString);

	if ( (cp = s) == NULLSTR || *cp == '\0' )
		return (Addr *)0;

	ap = newaddr();

	for ( tp = atypes ; (c = *tp++) != '\0' ; )
	{
		if ( *cp++ == c )
			goto outfix;
		if ( (np = strchr(cp--, c)) != NULLSTR )
			goto infix;
	}

	switch ( c = *cp++ )
	{
	default:
		--cp;
		c = ATYP_DESTINATION;
	case ATYP_DESTINATION:
		SET_ADDRDEST(ap, SplitDest(cp));
		if ( noremap )
			break;		/* No recursion */
		cp = ADDRDEST(ap)->dt_name;
		tp = ADDRDEST(ap)->dt_tname;
		if
		(
			(np = MapDest(tp)) == NULLSTR
			&&
			(np = MapDest(cp)) == NULLSTR
			&&
			(np = FwdDest(cp)) == NULLSTR
		)
			break;

		if ( strccmp(np, HomeName) == STREQUAL )
		{
			forwarded = false;
			Trace3(2, "MAPPED %s to %s", tp, np);
			cp = newstr(np);
		}
		else
		{
			forwarded = true;
			Trace3(2, "FORWARD %s via %s", tp, np);
			cp = concat(np, &atypes[1], tp, NULLSTR);
		}
		ADDRTYPE(ap) = c;
		aa = ap;
		FreeAddr(&aa);
		if ( (ap = StringToAddr(cp, true)) != (Addr *)0 )
			ap->ad_string = cp;
		else
			free(cp);
		if ( forwarded )
			ap->ad_flags |= AD_FORWARDED;
		return ap;

	case ATYP_MULTICAST:
	case ATYP_EXPLICIT:
outfix:		while ( *cp == c )
			cp++;
		if ( (np = strchr(cp, c)) != NULLSTR )
		{
infix:			if ( (ap->ad_next = StringToAddr(np, noremap)) != (Addr *)0 )
				ap->ad_next->ad_prev = ap;
			*np = '\0';
		}
		if ( (ap->ad_down = StringToAddr(cp, noremap)) == (Addr *)0 )
		{
			register Addr *	aa = ap;

			ap = ap->ad_next;
			freeaddr(aa);
			return ap;
		}
		if ( c == ATYP_EXPLICIT && EXPLTYPE(ap->ad_down) )
		{
			register Addr *	aa = ap;

			for ( ap = ap->ad_down ; ap->ad_next != (Addr *)0 ; ap = ap->ad_next );
			ap->ad_next = aa->ad_next;
			ap = aa->ad_down;
			freeaddr(aa);
			return ap;
		}
		ap->ad_down->ad_up = ap;
		break;

	case ATYP_BROADCAST:
		if ( *cp != ATYP_DESTINATION )		/* Must have *. */
		{
			freeaddr(ap);
			return (Addr *)0;
		}
		if ( (ap->ad_down = StringToAddr(cp, true)) == (Addr *)0 )
		{
			register Addr *	aa = ap;

			ap = ap->ad_next;
			freeaddr(aa);
			return ap;
		}
		ap->ad_down->ad_up = ap;
	}

	ADDRTYPE(ap) = c;

	return ap;
}



/*
**	Return typed address.
*/

char *
TypedAddress(aa)
	Addr *		aa;
{
	register Addr *	ap;
	register char *	cp;

	if ( (ap = aa) == (Addr *)0 )
		return NULLSTR;

	if ( (cp = ap->ad_typed) == NULLSTR )
		cp = AddrToString(ap, intype);

	return cp;
}



/*
**	Return typed name if address is a single destination, or EmptyString.
*/

char *
TypedName(aa)
	Addr *		aa;
{
	register Addr *	ap;
	register char *	cp;

	if ( (ap = aa) == (Addr *)0 || !DESTTYPE(ap) )
		return EmptyString;

	if ( (cp = ap->ad_typed) == NULLSTR )
		cp = AddrToString(ap, intype);

	return ++cp;
}



/*
**	Return untyped address.
*/

char *
UnTypAddress(aa)
	Addr *		aa;
{
	register Addr *	ap;
	register char *	cp;

	if ( (ap = aa) == (Addr *)0 )
		return NULLSTR;

	if ( (cp = ap->ad_untyped) == NULLSTR )
		cp = AddrToString(ap, notype);

	return cp;
}



/*
**	Return untyped name if address is a single destination, or EmptyString.
*/

char *
UnTypName(aa)
	Addr *		aa;
{
	register Addr *	ap;

	if ( (ap = aa) == (Addr *)0 || !DESTTYPE(ap) )
		return EmptyString;

	return ADDRDEST(ap)->dt_name;
}
