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

#undef	free



/*
**	Safe "malloc()".
*/

char *
Malloc(size)
	unsigned	size;
{
	register char *	cp;

	DODEBUG(if(size==0)Fatal("Malloc(0)"));

	while ( (cp = (char *)malloc(size)) == NULLSTR )
		Syserror("no memory for malloc(%u)", size);

	Trace3(5, "malloc(%d) ==> %#lx", size, (PUlong)cp);

	return cp;
}



/*
**	Safe "calloc()".
*/

char *
Malloc0(size)
	unsigned	size;
{
	return strclr(Malloc(size), size);
}



/*
**	Trace "free()".
*/

void
Free(cp)
	char *	cp;
{
	Trace2(5, "free(%#lx)", (PUlong)cp);

	if ( cp == NULLSTR )
		Fatal("free(0)");

	free(cp);
}
