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
#include	"ftheader.h"



/*
**	Make FthTo list.
**
**	It may take the following form:
**
**	user[<FTH_USEP>user...][<FTH_UDEST>nodelist][<FTH_UDSEP>...]
*/

void
SetFthTo()
{
	register FthUlist *	up;
	register FthUlist *	nup;
	register char *		cp;
	register int		length;

	if ( NFthUsers == 0 )
		Error("Need at least one destination user");

	for ( up = FthUsers, length = 0 ; up != (FthUlist *)0 ; up = up->u_next )
	{
		if ( up->u_name == NULLSTR )
			continue;

		length += strlen(up->u_name);

		if ( up->u_dest != NULLSTR )
			length += strlen(up->u_dest);
	}

	FthTo = cp = Malloc(length + NFthUsers*3);

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = nup )
	{
		register char *	ocp = cp;

		nup = up->u_next;

		if ( up->u_name == NULLSTR )
			continue;

		cp = strcpyend(cp, up->u_name);

		QuoteChars(ocp, FthToRestricted);

		if
		(
			nup != (FthUlist *)0
			&&
			(
				(up->u_dest == NULLSTR && nup->u_dest == NULLSTR)
				||
				(up->u_dest != NULLSTR && nup->u_dest != NULLSTR
				&&
				strcmp(nup->u_dest, up->u_dest) == STREQUAL)
			)
		)
		{
			*cp++ = FTH_USEP;
		}
		else
		{
			if ( up->u_dest != NULLSTR )
			{
				*cp++ = FTH_UDEST;
				cp = strcpyend(cp, up->u_dest);
			}

			if ( nup != (FthUlist *)0 )
				*cp++ = FTH_UDSEP;
		}
	}
}
