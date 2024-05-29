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
**	Print routing tables in readable format.
*/

#define STDIO

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"routefile.h"


#define	Fprintf			(void)fprintf

#define	LINK_NAME_LENGTH	MaxLinkLength		/* From routefile */
#define	LINK_TNAME_LENGTH	MaxLinkLength		/* From routefile */
#define	MIN_NAME_LENGTH		10
#define	REGION_NAME_LENGTH	MaxStrLength		/* From routefile */
#define	VIA_LENGTH		12



void
PrintRoute(fd, verbose)
	register FILE *	fd;
	bool		verbose;
{
	register int	i;
	register int	j;
	register int	k;
	register char *	cp;
	register int	len;

	Trace3(1, "PrintRoute(%d, %s)", fileno(fd), verbose?"verbose":EmptyString);

	if ( RouteBase == NULLSTR )
		if ( !ReadRoute(SUMCHECK) )
			return;

	if ( (len = REGION_NAME_LENGTH) < MIN_NAME_LENGTH )
		len = MIN_NAME_LENGTH;

	Fprintf
	(
		fd,
		"Routing tables for    %*s\nvisible within region %*s\n\n\t%lu link%s, %lu region%s\n",
		len, verbose?HomeName:StripTypes(HomeName),
		len, (VisibleName[0] == '\0') ? "* WORLD *" : verbose?VisibleName:StripTypes(VisibleName),
		(PUlong)LinkCount,
		LinkCount==1?EmptyString:"s",
		(PUlong)RegionCount,
		RegionCount==1?EmptyString:"s"
	);

	if ( (len = LINK_TNAME_LENGTH+1) < MIN_NAME_LENGTH )
		len = MIN_NAME_LENGTH;

	if ( verbose )
		cp = "\n No. %-*s %-*s (Flags)    Spooler & Caller | Filter\n";
	else
		cp = "\n No. %-*s %-*s\n";

	i = (int)fprintf(fd, cp, len, "Link", (int)LINK_NAME_LENGTH, "Directory") - 2;

#	if	SPRF_SIZE != 1
	i = 79;
#	endif	/* SPRF_SIZE != 1 */

	while ( --i >= 0 )
		putc('_', fd);

	for ( i = 0 ; i < LinkCount ; i++ )
	{
		register RLink *	rlp = &LinkVector[i];

		cp = &Strings[rlp->rl_rname];

		if ( !verbose )
			cp = StripTypes(cp);

		Fprintf
		(
			fd,
			"\n%3d: %-*s %-*s ",
			i+1,
			len, cp,
			(int)LINK_NAME_LENGTH, &Strings[rlp->rl_name]
		);

		if ( !verbose )
		{
			free(cp);
			continue;
		}

		if ( (j = rlp->rl_flags) )
		{
			j = PrintFlags(fd, (Flags)j);
			while ( j++ < 10 )
				putc(' ', fd);
		}
		else
			Fprintf(fd, "          ");
		if ( (j = rlp->rl_spooler) )
			Fprintf(fd, " \"%s\"", &Strings[j]);
		if ( (j = rlp->rl_caller) )
			Fprintf(fd, " & \"%s\"", &Strings[j]);
		if ( (j = rlp->rl_filter) )
			Fprintf(fd, " | \"%s\"", &Strings[j]);
	}

	if ( (len = REGION_NAME_LENGTH) < MIN_NAME_LENGTH )
		len = MIN_NAME_LENGTH;

	i = (int)fprintf
	    (
		fd,
		"\n\n%-*s %-*s%s\n",
		len,
		"Region",
		VIA_LENGTH,
		" Fast Cheap",
		verbose?"  Destination reachable link numbers":EmptyString
	    ) - 3;

#	if	SPRF_SIZE != 1
	i = 79;
#	endif	/* SPRF_SIZE != 1 */

	while ( --i >= 0 )
		putc('_', fd);

	for ( i = 0 ; i < RegionCount ; i++ )
	{
		register RegionEnt *	rep;
		char			number[VIA_LENGTH+1];
		static char		format[] = " %2d";
		static char		space[] = "   ";

		if ( verbose )
		{
			rep = &RegionVector[i];
			cp = &Strings[rep->re_name];
		}
		else
		{
			rep = &RegionVector[NameVector[i].me_index];
			cp = &Strings[NameVector[i].me_name];
		}

		if ( *cp == '\0' )
			cp = "* WORLD *";
		Fprintf(fd, "\n%-*s ", len, cp);

		switch ( (What)rep->re_what )
		{
		default:	cp = "[???] ";	break;
		case wh_none:	cp = "[N/A] ";	break;

		case wh_next:
		case wh_home:	cp = "[HOME] ";	break;

		case wh_lixt:
		case wh_link:
			(void)sprintf
			      (
				number, "%5lu%c%5lu%c",
				(PUlong)rep->re_route[0]+1,
				(rep->re_flags&FL_DUPROUTE)?'*':' ',
				(PUlong)rep->re_route[1]+1,
				(rep->re_route[0]==rep->re_route[1])?' ':'*'
			      );
			cp = number;
		}

		Fprintf(fd, "%*s  ", VIA_LENGTH, cp);

		if ( !verbose )
			continue;

		for ( k = 0, j = i*LinkCount ; k < LinkCount ; k++, j++ )
			if ( RegionTable[j/8] & (1<<(j%8)) )
				Fprintf(fd, format, k+1);
			else
				Fprintf(fd, "%s", space);
	}

	putc('\n', fd);
}
