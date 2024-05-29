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


extern char *	HomeName;


static char	GroupSep[]	= ";\t\n";
static char	AddrSep[]	= "@:";



/*
**	Extract message address and user list from a non-canonical FTP-type address.
**
**	Syntax is:- user[,user...]<@|:|.>address[tab|new-line|<;> ...]
**
**	Modifies argument.
**
**	Return Addr list, or null.
*/

Addr *
AddrFromUser(address)
	register char *		address;
{
	register char *		ep;
	register char *		cp;
	register char *		dp;
	register FthUlist *	up;
	register FthUlist **	lastuser;
	register Addr *		nap;
	Addr *			ap	= (Addr *)0;

	Trace2(1,"AddrFromUser(%s)", address);

	lastuser = &FthUsers;

	while ( *lastuser != (FthUlist *)0 )
		lastuser = &(*lastuser)->u_next;

	while ( address != NULLSTR )
	{
		if ( (ep = strpbrk(address, GroupSep)) != NULLSTR )
		{
			*ep++ = '\0';
			ep += strspn(ep, GroupSep);
		}

		if
		(
			(cp = strrpbrk(address, AddrSep)) != NULLSTR
			||
			(cp = strchr(address, '.')) != NULLSTR
		)
		{
			*cp++ = '\0';
			AddAddr(&ap, nap = StringToAddr(cp, NO_STA_MAP));
			dp = UnTypAddress(nap);
		}
		else
		if ( HomeName != NULLSTR )
		{
			cp = newstr(HomeName);
			AddAddr(&ap, nap = StringToAddr(cp, NO_STA_MAP));
			dp = UnTypAddress(nap);
			free(cp);
		}
		else
		{
			FreeAddr(&ap);
			return (Addr *)0;
		}

		do
		{
			if ( (cp = strchr(address, ',')) != NULLSTR )
				*cp++ = '\0';

			if ( address[0] == '\0' )
			{
				FreeAddr(&ap);
				return (Addr *)0;
			}

			up = Talloc(FthUlist);
			up->u_next = (FthUlist *)0;
			*lastuser = up;
			lastuser = &up->u_next;
			NFthUsers++;

			up->u_dest = dp;
			up->u_name = address;
		}
		while
			( (address = cp) != NULLSTR );

		address = ep;
	}

	return ap;
}
