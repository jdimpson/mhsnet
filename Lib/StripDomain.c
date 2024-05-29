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


static char *	StripDom;
static bool	StripExpl;
static char *	StripMatch;

static char *	copydest _FA_((char *, char *));
static char *	stripdomain _FA_((char *, char *));



/*
**	Strip types, explicit parts, and final domain,
**	from destinations in passed address.
**	Basically Sun4->3 conversion.
**
**	Returns pointer to modified argument.
*/

char *
StripDomain(s, m, d, b)
	char *		s;
	char *		m;	/* Match this */
	char *		d;	/* Then strip this */
	bool		b;
{
	Trace6(2,
		"StripDomain(%s [%#lx], %s, %s, %s)",
		s,
		(PUlong)s,
		m,
		d,
		b?"strip explicit":"leave explicit"
	);

	if ( (StripMatch = m) == NULLSTR )
		StripMatch = EmptyString;
	if ( (StripDom = d) == NULLSTR )
		StripDom = EmptyString;
	StripExpl = b;
	(void)stripdomain(s, s);

	return s;
}



/*
**	Strip types, explicit parts, and final domain,
**	from destinations in passed address.
**
**	Returns pointer to end of copied string.
*/

char *
StripDEnd(c, s, m, d, b)
	char *		c;
	char *		s;
	char *		m;	/* Match this */
	char *		d;	/* Then strip this */
	bool		b;
{
	Trace7(2,
		"StripDEnd(%#lx, %s [%#lx], %s, %s, %s)",
		(PUlong)c,
		s==NULLSTR?NullStr:s,
		(PUlong)s,
		m==NULLSTR?NullStr:m,
		d==NULLSTR?NullStr:d,
		b?"strip explicit":"leave explicit"
	);

	if ( (StripMatch = m) == NULLSTR )
		StripMatch = EmptyString;
	if ( (StripDom = d) == NULLSTR )
		StripDom = EmptyString;
	StripExpl = b;
	return stripdomain(s, c);
}



/*
**	Strip a domain from end of a destination and copy into arg.
*/

static char *
stripdomain(s, r)
	char *		s;
	char *		r;
{
	register char *	cp;
	register char *	rp;
	register char *	np;
	register char *	tp;
	register char	c;

	Trace3(3, "stripdomain(%s, %#lx)", s==NULLSTR?NullStr:s, (PUlong)r);

	if ( (cp = s) == NULLSTR || (rp = r) == NULLSTR )
		return r;

	switch ( c = *cp++ )
	{
	case ATYP_MULTICAST:
nostrip:	for ( ;; )
		{
			*rp++ = c;
			if ( (np = strchr(cp, c)) != NULLSTR )
				*np = '\0';
			if ( (tp = stripdomain(cp, rp)) == rp )
				*--rp = '\0';
			else
				rp = tp;
			if ( (cp = np) != NULLSTR )
				*cp++ = c;
			else
				break;
		}
		break;

	case ATYP_EXPLICIT:
		if ( !StripExpl )
			goto nostrip;
		if ( (np = strrchr(cp, c)) != NULLSTR )
			cp = ++np;
		rp = stripdomain(cp, rp);
		break;

	case ATYP_BROADCAST:
		*rp++ = c;
		*rp++ = *cp++;
		if ( (np = strchr(cp, DOMAIN_SEP)) != NULLSTR )
			*np = '\0';
		if ( (cp = copydest(cp, rp)) == rp )
		{
			*--rp = '\0';
			*--rp = '\0';
		}
		else
			rp = cp;
		if ( np != NULLSTR && s != r )
			*np = DOMAIN_SEP;
		break;

	case ATYP_DESTINATION:
		rp = copydest(cp, rp);
		break;

	default:
		rp = copydest(--cp, rp);
	}

	Trace3(4, "stripdomain(%s) ==> %s", s, r);

	return rp;
}



/*
**	Strip types and copy string.
*/

static char *
copydest(s, r)
	char *		s;
	char *		r;
{
	register char	c;
	register char *	rp;
	register char *	sp;
	register char *	tp;

	Trace3(3, "copydest(%s, %#lx)", s==NULLSTR?NullStr:s, (PUlong)r);

	if ( (sp = s) == NULLSTR || (rp = r) == NULLSTR )
		return r;

	tp = rp;

	do
		if ( (c = *sp++) == TYPE_SEP )
			rp = tp;
		else
		if ( (*rp++ = c) == DOMAIN_SEP )
			tp = rp;
	while
		( c != '\0' );

	if ( strccmp(tp, StripDom) == STREQUAL )
		for ( sp = tp ;; )
		{
			if ( strccmp(sp, StripMatch) == STREQUAL )
			{
				if ( tp == r )
					++tp;
				rp = tp;
				*--tp = '\0';
				break;
			}

			if ( --sp < r )
				break;

			while ( sp > r && sp[-1] != DOMAIN_SEP )
				sp--;
		}

	Trace3(4, "copydest(%s) ==> %s", s, r);

	return --rp;
}
