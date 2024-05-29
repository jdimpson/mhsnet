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
#include	"debug.h"
#include	"header.h"



/*
**	Extract destinations from route,
**	and pass them back in from/time/to tuples.
**
**	Returns false if error or !*funcp.
*/

bool
ExRoute(source, route, funcp)
	register char *	source;		/* Initialise with HdrSource */
	register char *	route;		/* Initialise with HdrRoute */
	bool		(*funcp) _FA_((char *, Ulong, char *));
{
	char		ne;
	register char *	tp	= NULLSTR;
	register char *	nr	= NULLSTR;
	register char *	se	= &ne;
	register Ulong	tt;

	Trace4(1, "ExRoute(%s, %s, %#lx)", source==NULLSTR?NullStr:source, route==NULLSTR?NullStr:ExpandString(route, -1), (PUlong)funcp);

	if ( route != NULLSTR && *route++ == ROUTE_RS )
	for ( ;; )
	{
		if ( (nr = strchr(route, ROUTE_RS)) != NULLSTR )
			*nr = '\0';	/* End of time, next route */

		if ( (tp = strchr(route, ROUTE_US)) == NULLSTR )
			break;		/* Malformed route */

		*tp = '\0';		/* End of ``route'', start of time */

		tt = (Ulong)atol(tp+1);	/* Extract travel-time to ``route'' */

		Trace4(2, "(*funcp)(%s, %lu, %s)", source==NULLSTR?NullStr:source, (PUlong)tt, route);

		if ( !(*funcp)(source, tt, route) )
			break;

		/** Shift along **/

		*se = ROUTE_US;
		source = route;

		if ( (route = nr) == NULLSTR )
		{
			*tp = ROUTE_US;
			return true;
		}

		se = tp;
		*route++ = ROUTE_RS;
	}

	*se = ROUTE_US;
	if ( tp != NULLSTR )
		*tp = ROUTE_US;
	if ( nr != NULLSTR )
		*nr = ROUTE_RS;

	return false;
}
