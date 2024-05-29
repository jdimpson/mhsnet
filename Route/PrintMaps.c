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
**	Print mapping tables in readable format.
*/

#define STDIO

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"routefile.h"


static int	StringLength;
#define		MINSTRLENGTH	10

static bool	PFMap _FA_((FILE *, char *, MapEnt *, Index));
static bool	PMap _FA_((FILE *, char *, MapEnt *, Index));

#define	Fprintf	(void)fprintf



/*ARGSUSED*/
void
PrintMaps(fd, verbose)
	register FILE *		fd;
	bool			verbose;
{
	register TypeEnt *	tep;
	register TypeTab *	ttp;
	register Index		j;
	register Index		i;
	register char *		cp;

	Trace2(1, "PrintMaps(%d)", fileno(fd));

	if ( RouteBase == NULLSTR )
		if ( !ReadRoute(SUMCHECK) )
			return;

	if ( verbose )
	{
		Fprintf(fd, "Ordered types:-\n\t");
		for ( i = 0 ; i < MaxTypes ; i++ )
		{
			if ( i )
				putc(';', fd);
			Fprintf(fd, "%s", &Strings[TypeNameVector[i]]);
		}
		Fprintf(fd, "\n\n");
	}

	if ( (StringLength = MaxLinkLength) < MaxStrLength )
		if ( (StringLength = MaxStrLength) < MINSTRLENGTH )
			StringLength = MINSTRLENGTH;

	StringLength += 2;

	if ( PFMap(fd, "Region Forwarding Table", FwdMapVector, FwdMapCount) )
		putc('\n', fd);
	if ( PMap(fd, "Address Forwarding Table", AdrMapVector, AdrMapCount) )
		putc('\n', fd);
	if ( PMap(fd, "Address Mapping Table", RegMapVector, RegMapCount) )
		putc('\n', fd);
	if ( verbose && PMap(fd, "Type Name Table", TypMapVector, TypMapCount) )
		putc('\n', fd);

	Fprintf(fd, "Region Vector:-\n\n");

	for ( ttp = TypeVector, i = TypeCount ; i-- ; ttp++ )
	{
		Fprintf
		(
			fd,
			"%8s=%s\n",
			ttp->tp_type,
			&Strings[ttp->tp_name]
		);

		for ( tep = &TypeTables[ttp->tp_index], j = ttp->tp_count ; j-- ; tep++ )
		{
			switch ( tep->te_what )
			{
			case wh_map:
				Fprintf
				(
					fd,
					"%*s ==> %s\n",
					StringLength,
					&Strings[tep->te_name],
					&Strings[tep->te_index]
				);
				continue;

			default:	cp = "* ??? *";		break;

			case wh_none:	cp = "* N/A *";		break;

			case wh_next:
			case wh_home:	cp = "* HOME *";	break;

			case wh_lixt:
			case wh_link:
				cp = &Strings[LinkVector[RegionVector[tep->te_index].re_route[0]].rl_rname];
			}

			Fprintf
			(
				fd,
				"%*s --> %s\n",
				StringLength,
				&Strings[tep->te_name],
				cp
			);
		}
	}
}



static bool
PFMap(fd, desc, mep, i)
	register FILE *		fd;
	char *			desc;
	register MapEnt *	mep;
	register Index		i;
{
	register char *		cp;
	register int		l;

	if ( i == 0 )
		return false;

	Fprintf(fd, "%s:-\n", desc);

	for ( ; i-- ; mep++ )
	{
		cp = &Strings[mep->me_name];
		l = strlen(cp);
		Fprintf
		(
			fd,
			"%*s%s ==> %s\n",
			StringLength-l,
			"*.",
			cp,
			&Strings[mep->me_index]
		);
	}

	return true;
}


static bool
PMap(fd, desc, mep, i)
	register FILE *		fd;
	char *			desc;
	register MapEnt *	mep;
	register Index		i;
{
	if ( i == 0 )
		return false;

	Fprintf(fd, "%s:-\n", desc);

	for ( ; i-- ; mep++ )
		Fprintf
		(
			fd,
			"%*s ==> %s\n",
			StringLength,
			&Strings[mep->me_name],
			&Strings[mep->me_index]
		);

	return true;
}
