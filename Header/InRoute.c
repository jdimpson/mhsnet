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
#include	"address.h"
#include	"header.h"



/*
**	Return number of occurences of "region" in "HdrRoute".
*	(`maxadj' occurences of same region are treated as one.)
*/

int
InRoute(region, maxadj)
	char *		region;
	int		maxadj;	/* Maximum adjacent occurences of same region */
{
	register char *	p;
	register char *	e;
	register int	count;
	register int	matched_prev;

	Trace2(3, "InRoute(%s)", region);

	if ( (p = HdrRoute) == NULLSTR || region == NULLSTR )
		return 0;

	Trace2(3, "InRoute [%s]", ExpandString(HdrRoute, -1));

	for
	(
		count = 0, matched_prev = 0 ;
		*p++ == ROUTE_RS ;
	)
	{
		if ( (e = strchr(p, ROUTE_US)) == NULLSTR )
			break;
		else
			*e = '\0';

		if ( !AddressMatch(p, region) )
			matched_prev = 0;
		else
		if ( matched_prev == 0 || ++matched_prev > maxadj )
		{
			count++;
			matched_prev = 1;
		}

		*e = ROUTE_US;

		if ( (p = strchr(p, ROUTE_RS)) == NULLSTR )
			break;
	}

	Trace2(2, "InRoute found %d", count);

	return count;
}
