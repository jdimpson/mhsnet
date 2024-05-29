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
#include	"Passwd.h"
#include	"routefile.h"


extern char	Legal_c[];

static CA_ep	Errfunc;
static bool	Find;
static char *	Restrict;
static Flags	Uflags;
static char *	Uname;

int		CAcompare _FA_((const void *, const void *));
static char *	check_addr _FA_((Addr **));
static bool	addr_match _FA_((char *, char *));



/*
**	Check addresses are unique, and meet restrictions.
**
**	Return typed address string.
*/

char *
CheckAddr(aap, user, errfunc, find)
	Addr **			aap;
	Passwd *		user;
	CA_ep			errfunc;
	bool			find;
{
	char *			val;
	
	Trace6(
		1,
		"CheckAddr(%s, [%#x, %s], %#lx, %d)",
		(*aap==(Addr *)0)?NullStr:UnTypAddress(*aap),
		(int)user->P_flags,
		user->P_region==NULLSTR?NullStr:user->P_region,
		(PUlong)errfunc,
		(int)find
	);

	Uflags = user->P_flags;
	Uname = user->P_name;
	if ( (Restrict = user->P_region) != NULLSTR )
		Restrict = CanonString(Restrict);
	Errfunc = errfunc;
	Find = find;

	val = check_addr(aap);

	FreeStr(&Restrict);

	return val;
}



static char *
check_addr(aap)
	register Addr **	aap;
{
	register Addr *		ap;
	register Addr **	app;
	register int		i;
	register char *		s;
	register char *		cp;
	register int		count;
	static char		tdisallstr[] = english("%s addresses disallowed for user \"%s\".");
	static char		disallstr[] = english("\"%s\" -- address disallowed for user \"%s\".");
	static char		addrunkstr[] = english("\"%s\" -- address unknown.");
	static char		illcstr[] = english("illegal char \\%03o in \"%s\".");
	DODEBUG(static char	remaddr[] = "CheckAddr removed address");

	if ( (ap = *aap) == (Addr *)0 )
		return EmptyString;

	Trace2(2, "check_addr(%s)", UnTypAddress(ap));

	switch ( ADDRTYPE(ap) )
	{
	case ATYP_BROADCAST:	i = P_BROADCAST;	s = "Broadcast";	break;
	case ATYP_EXPLICIT:	i = P_EXPLICIT;		s = "Explicit";		break;
	case ATYP_MULTICAST:	i = P_MULTICAST;	s = "Multicast";	break;
	default:		i = 0;			s = NullStr;		break;
	}

	if ( i && !(i & Uflags) )
	{
		(*Errfunc)(tdisallstr, s, Uname);
rem_out:	RemoveAddr(aap);
		Trace1(2, remaddr);
		return EmptyString;
	}

	if ( DESTTYPE(ap) )
	{
		if ( Find && FindAddr(ap, (Link *)0, FASTEST) == wh_none )
		{
			(*Errfunc)(addrunkstr, UnTypAddress(ap));
			goto rem_out;
		}

		s = TypedAddress(ap);

		for ( cp = s ; (i = *cp++) ; )
			if ( !Legal_c[i] )
			{
				cp = UnTypAddress(ap);
				(*Errfunc)(illcstr, i, ExpandString(cp, -1));
				goto rem_out;
			}

		if
		(
			(s[1] == '\0' && !(Uflags & P_WORLD))
			||
			(Restrict != NULLSTR && !addr_match(Restrict, s))
		)
		{
			(*Errfunc)(disallstr, UnTypAddress(ap), Uname);
			goto rem_out;
		}

		return s;
	}

	count = 0;
	do
	{
		while ( ap->ad_down == (Addr *)0 )
			if ( (ap = ap->ad_next) == (Addr *)0 )
				goto next1;

		(void)check_addr(&ap->ad_down);

		if ( ap->ad_down != (Addr *)0 )
			count++;

		if ( EXPLTYPE(*aap) )
			Find = false;
	}
	while
		( (ap = ap->ad_next) != (Addr *)0 );

next1:
	if ( count == 0 )
		goto rem_out;

	if ( (i = count) > 1 )
	{
		app = (Addr **)Malloc(i * sizeof(Addr *));

		ap = *aap;
		do
		{
			while ( ap->ad_down == (Addr *)0 )
				if ( (ap = ap->ad_next) == (Addr *)0 )
					goto next2;
			app[--i] = ap;
		}
		while
			( (ap = ap->ad_next) != (Addr *)0 );
next2:
		if ( (i = count) > 2 && !EXPLTYPE(*aap) )
			qsort((char *)app, i, sizeof(Addr *), CAcompare);

		while ( --i > 0 )
			if ( strccmp(app[i-1]->ad_down->ad_typed, app[i]->ad_down->ad_typed) == STREQUAL )
			{
				/*
				**	Remove unnecessary address.
				*/

				ap = app[i];

				Trace2(1, "check_addr remove dup address \"%s\"", ap->ad_down->ad_typed);

				if ( !DESTTYPE(ap) )
					FreeAddr(&ap->ad_down);
				else
					FreeDest(ADDRDESTP(ap));

				FreeStr(&ap->ad_typed);
				FreeStr(&ap->ad_untyped);

				FreeStr(&(*aap)->ad_typed);
				FreeStr(&(*aap)->ad_untyped);
			}

		free((char *)app);
	}

	Trace2(2, "check_addr returns at \"%s\"", TypedAddress(*aap));

	return TypedAddress(*aap);
}



/*
**	Compare two addresses.
*/

int
#ifdef	ANSI_C
CAcompare(const void *dp1, const void *dp2)
#else	/* ANSI_C */
CAcompare(dp1, dp2)
	char *	dp1;
	char *	dp2;
#endif	/* ANSI_C */
{
	return strccmp((*(Addr **)dp2)->ad_down->ad_typed, (*(Addr **)dp1)->ad_down->ad_typed);
}



/*
**	Return true if ``match'' is a final destination within ``address''.
*/

static bool
addr_match(address, match)
	register char *	address;
	register char *	match;
{
	register char *	np;
	register bool	val;

	Trace3(2, "addr_match(%s, %s)", address, match);

	switch ( *address++ )
	{
	case ATYP_BROADCAST:
		if ( *address++ != ATYP_DESTINATION )
			return false;
		if ( *address == '\0' )
			return true;
	case ATYP_DESTINATION:
		do
		{
			if ( *match == DOMAIN_SEP )
				match++;
			if ( strccmp(match, address) == STREQUAL )
				return true;
		}
		while
			( (match = strchr(match, DOMAIN_SEP)) != NULLSTR );
		return false;

	case ATYP_MULTICAST:
		while ( (np = strchr(address, ATYP_MULTICAST)) != NULLSTR )
		{
			*np = '\0';
			val = addr_match(address, match);
			*np++ = ATYP_MULTICAST;
			if ( val )
				return true;
			address = np;
		}
		return addr_match(address, match);
	}

	return false;
}
