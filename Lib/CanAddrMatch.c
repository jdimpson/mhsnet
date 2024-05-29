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



/*
**	Call ``AddressMatch()'' with canonical address arguments.
*/

bool
CanAddrMatch(address, match)
	register char *	address;
	register char *	match;
{
	register char *	acp;
	register char *	mcp;
	register bool	val;
	Addr *		ap;
	Addr *		mp;

	Trace3(2, "CanAddrMatch(%s, %s)", address, match);

	if ( match == NULLSTR || *match == '\0' )
		return false;

	if ( address == NULLSTR || *address == '\0' )
		return false;

	if ( strccmp(address, match) == STREQUAL )
		return true;

	acp = newstr(address);
	mcp = newstr(match);

	ap = StringToAddr(acp, true);
	mp = StringToAddr(mcp, true);
	val = AddressMatch(UnTypAddress(ap), UnTypAddress(mp));
	FreeAddr(&mp);
	FreeAddr(&ap);

	free(mcp);
	free(acp);

	return val;
}
