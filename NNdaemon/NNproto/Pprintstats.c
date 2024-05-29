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


/*
**	Print out protocol statistics
*/

#define	STDIO

#include	"global.h"

#include	"Pstats.h"

#define	Fprintf	(void)fprintf



void
Pprintstats(fd)
	FILE *	fd;
{
	register int	i;
	register int	first = 1;

#	if	PSTATISTICS == 1
	for ( i = 0 ; i < PS_NSTATS ; i++ )
	{
		if ( Pstats[i].ss_count )
		{
			if ( first )
			{
				first = 0;
				Fprintf(fd, "Protocol statistics:\n");
			}

#			if	PSTATSDESC == 1
			Fprintf(fd, "%9lu %s\n", (PUlong)Pstats[i].ss_count, Pstats[i].ss_descp);
#			else	/* PSTATSDESC */
			Fprintf(fd, "%9lu [%d]\n", (PUlong)Pstats[i].ss_count, i);
#			endif	/* PSTATSDESC */
		}
	}
#	else	/* PSTATISTICS */
#	ifdef	lint
	i = (int)fd; first = i; i = first;
#	endif	/* lint */
#	endif	/* PSTATISTICS */
}
