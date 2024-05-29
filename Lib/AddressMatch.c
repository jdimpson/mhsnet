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


char *		MatchedBC;	/* Broadcast address matched */
bool		MatchRegion;	/* Ok to try matching inside region */

static bool	addr_match _FA_((char *, char *));



/*
**	Return true if ``match'' is a final destination within ``address''.
*/

bool
AddressMatch(address, match)
	register char *	address;
	register char *	match;
{
	register char *	np;
	register bool	val;

	Trace3(2, "AddressMatch(%s, %s)", address, match);

	FreeStr(&MatchedBC);

	if ( match == NULLSTR || *match == '\0' )
		return false;

	if ( address == NULLSTR || *address == '\0' )
		return false;

	switch ( *match++ )
	{
	default:
	case ATYP_DESTINATION:
	case ATYP_BROADCAST:
		--match;
		return addr_match(address, match);

	case ATYP_EXPLICIT:
		if ( (np = strrchr(match, ATYP_EXPLICIT)) != NULLSTR )
			return addr_match(address, np+1);
		return false;

	case ATYP_MULTICAST:
		while ( (np = strchr(match, ATYP_MULTICAST)) != NULLSTR )
		{
			*np = '\0';
			val = addr_match(address, match);
			*np++ = ATYP_MULTICAST;
			if ( !val )
				return false;
			match = np;
		}
		return addr_match(address, match);
	}
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

	Trace3(3, "addr_match(%s, %s)", address, match);

	switch ( *address++ )
	{
	case ATYP_BROADCAST:
		if ( *address++ != ATYP_DESTINATION )
			return false;
		if ( *address == '\0' )
		{
			MatchedBC = newstr(address - 2);
			return true;
		}
		do
		{
			if ( *match == DOMAIN_SEP )
				match++;
			if ( strccmp(match, address) == STREQUAL )
			{
				MatchedBC = newstr(address - 2);
				return true;
			}
		}
		while
			( (match = strchr(match, DOMAIN_SEP)) != NULLSTR );
		return false;

	default:
		--address;	/* Allow simple address here */
	case ATYP_DESTINATION:
		do
		{
			if ( *match == DOMAIN_SEP )
				match++;
			if ( strccmp(match, address) == STREQUAL )
				return true;
			if ( !MatchRegion )
				return false;
		}
		while
			( (match = strchr(match, DOMAIN_SEP)) != NULLSTR );
		return false;

	case ATYP_EXPLICIT:
		if ( (np = strrchr(address, ATYP_EXPLICIT)) != NULLSTR )
			return addr_match(np+1, match);
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
}
