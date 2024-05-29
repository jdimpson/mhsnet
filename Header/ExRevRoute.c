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


#define	STDIO

#include	"global.h"
#include	"debug.h"
#include	"header.h"



/*
**	Extract destinations from route,
**	and pass them back in from/time/to tuples in reverse order.
**
**	Returns false if error or !*funcp.
*/

bool
ExRevRoute(source, route, dest, funcp, fd)
	char *		source;		/* Initialise with HdrSource */
	char *		route;		/* Initialise with HdrRoute */
	char *		dest;		/* Initialise with HdrDest */
	bool		(*funcp) _FA_((char *, Ulong, char *, bool, FILE *));
	FILE *		fd;
{
	char		ec;
	register char *	ep	= &ec;
	register char *	node;
	register char *	nr;
	register char *	tp;
	register Ulong	tt	= 0;

	Trace6(1, "ExRevRoute(%s, %s, %s, %#lx, %#lx)", source, route, dest, (PUlong)funcp, (PUlong)fd);

	if ( route == NULLSTR || *route != ROUTE_RS )
		return false;

	dest = newstr(dest);

	while ( (nr = strrchr(route, ROUTE_RS)) != NULLSTR )
	{
		*ep = ROUTE_RS;

		if ( (tp = strchr(nr++, ROUTE_US)) == NULLSTR )
			break;		/* Malformed route */

		node = newnstr(nr, tp-nr);

		Trace5(2, "(*funcp)(%s, %lu, %s, %d)", node, (PUlong)tt, dest, false);
		if ( !(*funcp)(node, tt, dest, false, fd) )
			break;

		tt = (Ulong)atol(tp+1);	/* Extract travel-time to `node' */

		/** Shift back **/

		free(dest);
		dest = node;

		if ( (ep = --nr) == route )
		{
			Trace5(2, "(*funcp)(%s, %lu, %s, %d)", source, (PUlong)tt, dest, true);
			if ( !(*funcp)(source, tt, dest, true, fd) )
				break;

			free(dest);
			return true;
		}

		*ep = '\0';
	}

	*ep = ROUTE_RS;
	free(dest);
	return false;
}
