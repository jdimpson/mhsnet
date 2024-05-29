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
**	Free up 'FthTo' list.
*/

#include	"global.h"
#include	"ftheader.h"



void
FthToFree()
{
	register FthUl_p	up;

	NFthUsers = 0;

	while ( (up = FthUsers) != (FthUlist *)0 )
	{
		FthUsers = up->u_next;
		if ( up->u_name != NULLSTR )
			free(up->u_name);
		if ( up->u_dest != NULLSTR )
			free(up->u_dest);
		up->u_next = FthUFreeList;
		FthUFreeList = up;
	}
}
