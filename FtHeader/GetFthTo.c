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
#include	"debug.h"
#include	"ftheader.h"

#if	SUN3 == 1
#include	"sun3.h"
#endif	/* SUN3 == 1 */



/*
**	Extract users from ``FthTo'' list.
**
**	``User-friendly'' version of ``FthTo'' set up in ``UQFthTo''.
**
**	If ``checkathome'', exclude users whose addresses aren't ``AtHome''.
**
**	Returns number of users found.
*/

int
GetFthTo(checkathome)
	bool			checkathome;
{
	register char *		s;
	register char *		u;
	register FthUlist *	up;
	register FthUlist **	endup = &FthUsers;
	register char *		dest;
	register char *		a;
	register char *		d;
	register char *		qp;
	bool			exclude;

	Trace3(
		1,
		"GetFthTo(%d) <== %s",
		checkathome,
		(FthTo==NULLSTR)?NullStr:ExpandString(FthTo, -1)
	);

	FreeStr(&UQFthTo);

	NFthUsers = 0;

	if ( (s = FthTo) == NULLSTR )
	{
		UQFthTo = qp = Malloc(1);
		*qp = '\0';
	}
	else
	for ( UQFthTo = qp = Malloc(strlen(FthTo)*2+1) ;; )
	{
		if ( (a = strchr(s, FTH_UDSEP)) != NULLSTR )
			*a = '\0';

		exclude = false;

		if ( (d = strchr(s, FTH_UDEST)) != NULLSTR )
		{
			dest = d+1;
			if ( checkathome && !StringAtHome(dest) )
				exclude = true;
			*d = '\0';
		}
		else
			dest = NULLSTR;

		/*
		**	Build legible string.
		*/

		if ( qp != UQFthTo )
			*qp++ = ' ';
		u = strcpyend(qp, s);
		UnQuoteChars(qp, FthToRestricted);

		if ( dest != NULLSTR )
		{
			*u++ = '@';
#			if	SUN3 == 1
			if ( Sun3 )
				qp = StripDEnd(u, dest, OzAu, Au, false);
			else
#			endif	/* SUN3 == 1 */
				qp = StripDEnd(u, dest, NULLSTR, NULLSTR, false);
		}
		else
			qp = u;

		/*
		**	Extract users into list.
		*/

		if ( !exclude )
		for ( ;; )
		{
			if ( (u = strchr(s, FTH_USEP)) != NULLSTR )
				*u = '\0';

			NFthUsers++;

			if ( (up = FthUFreeList) == (FthUlist *)0 )
				up = Talloc(FthUlist);
			else
				FthUFreeList = up->u_next;

			*endup = up;
			endup = &up->u_next;

			up->u_dir = NULLSTR;
			up->u_uid = -1;

			if ( (up->u_dest = dest) != NULLSTR )
				up->u_dest = newstr(dest);
			up->u_name = newstr(s);
			UnQuoteChars(up->u_name, FthToRestricted);

			if ( (s = u) != NULLSTR )
				*s++ = FTH_USEP;
			else
				break;
		}

		if ( d != NULLSTR )
			*d = FTH_UDEST;

		if ( (s = a) != NULLSTR )
			*s++ = FTH_UDSEP;
		else
			break;
	}

	*endup = (FthUlist *)0;

	Trace2(1, "GetFthTo() ==> %s", UQFthTo);

	return NFthUsers;
}
