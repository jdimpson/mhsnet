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
#include	"ftheader.h"



/*
**	Search 'FthTo' list for named user, return 'true' if present.
*/

bool
InFthTo(user)
	char *		user;
{
	register char *	s = FthTo;
	register char *	u;
	register char *	b;
	register char *	a;
	register bool	found = false;

	do
	{
		if ( (a = strchr(s, FTH_UDSEP)) != NULLSTR )
			*a = '\0';
		
		if ( (b = strchr(s, FTH_UDEST)) != NULLSTR )
		{
			*b = '\0';
			if ( !StringAtHome(b+1) )
				goto cont;
		}

		/*
		**	Search users at this host.
		*/

		do
		{
			if ( (u = strchr(s, FTH_USEP)) != NULLSTR )
				*u = '\0';
			
			if ( strcmp(user, s) == STREQUAL )
				found = true;

			if ( (s = u) != NULLSTR )
				*s++ = FTH_USEP;
			else
				break;
		}
		while
			( !found );

cont:
		if ( b != NULLSTR )
			*b = FTH_UDEST;

		if ( (s = a) != NULLSTR )
			*s++ = FTH_UDSEP;
		else
			break;
	}
	while
		( !found );

	return found;
}
