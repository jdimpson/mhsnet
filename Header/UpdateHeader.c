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
**	Update the statistics in the global header.
**	If 'TimeToDie' field is non-zero, decrement it.
**	Return false if message has timed out, otherwise true.
*/

bool
UpdateHeader(tt, home)
	Ulong		tt;	/* Travel time for last link */
	char *		home;	/* Our address */
{
	char *		or;
	char		ttstr[TIME_SIZE+1];

	static char	hc_route[]	= { ROUTE_RS, '\0' };
	static char	hc_time[]	= { ROUTE_US, '\0' };
	static char	format[]	= "%lu";

	Trace3(2, "UpdateHeader(%lu, %s)", (PUlong)tt, home);

	/*
	**	Add "home" to "route".
	*/

	(void)sprintf(ttstr, format, (PUlong)tt);

	if ( (or = HdrRoute) != NULLSTR )
	{
		HdrRoute = concat(or, hc_route, home, hc_time, ttstr, NULLSTR);
		if ( FreeHdrRoute )
			free(or);
	}
	else
		HdrRoute = concat(hc_route, home, hc_time, ttstr, NULLSTR);

	FreeHdrRoute = true;

	/*
	**	Update "travel time" and "time-to-die".
	*/

	HdrTt += tt;

	if ( HdrTtd == 0 )
		return true;

	if ( tt >= HdrTtd )
	{
		HdrTtd = 0;
		return false;		/* This messsage has expired */
	}

	HdrTtd -= tt;

	return true;
}
