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
#include	"link.h"
#include	"route.h"


#define	Fprintf	(void)fprintf

static int	Maxlen;

static void	pr_force_routes _FA_((FILE *, Region *, Region *, Region *));



/*
**	Print out forced route list.
*/

void
PrintForced(fd)
	FILE *			fd;
{
	register AdvRoute *	arp;
	register int		i;

	Trace2(1, "PrintForced(%d)", fileno(fd));

	for ( arp = AdvisoryRoutes, Maxlen = 0 ; arp != (AdvRoute *)0 ; arp = arp->ar_next )
	{
		if ( arp->ar_route[0] == NULL_ROUTE && arp->ar_route[1] == NULL_ROUTE )
			continue;
		if ( (i = strlen(arp->ar_region->rg_name)) > Maxlen )
			Maxlen = i;
	}

	if ( Maxlen == 0 )
		return;

	Fprintf(fd, english("Forced routes:-\n"));

	for ( arp = AdvisoryRoutes ; arp != (AdvRoute *)0 ; arp = arp->ar_next )
		pr_force_routes(fd, arp->ar_region, arp->ar_route[0], arp->ar_route[1]);

	putc('\n', fd);
}



/*
**	Force routes for region and all sub-regions.
*/

static void
pr_force_routes(fd, rp, route0, route1)
	FILE *			fd;
	register Region *	rp;
	register Region *	route0;
	register Region *	route1;
{
	char *			fast;
	static char		fmt[] = "%*s ==> %s%s\n";
	static char		nor[] = english("NOROUTE");

	if ( route0 != route1 )
		fast = " (F)";
	else
		fast = EmptyString;

	if ( route0 != NULL_ROUTE )
		Fprintf(fd, fmt, Maxlen, rp->rg_name, (route0==CLEAR_ROUTE)?nor:route0->rg_name, fast);

	if ( route0 == route1 )
		return;

	if ( route1 != NULL_ROUTE )
		Fprintf(fd, fmt, Maxlen, rp->rg_name, (route1==CLEAR_ROUTE)?nor:route1->rg_name, " (C)");
}
