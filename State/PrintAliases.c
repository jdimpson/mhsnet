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
#include	"routefile.h"


#define	Fprintf	(void)fprintf



/*
**	Print out Aliases from contents of sorted alias map.
*/

void
PrintAliases(fd)
	FILE *			fd;
{
	register Entry **	epp;
	register int		i;
	register int		j;
	register int		maxlen;

	Trace2(1, "PrintAliases(%d)", fileno(fd));

	if ( AliasList == (Entry **)0 )
		AliasCount = MakeList(&AliasList, AliasHash, AliasCount);

	for ( epp = AliasList, i = AliasCount, maxlen = 0 ; --i >= 0 ; epp++ )
		if ( (j = strlen((*epp)->en_name)) > maxlen )
			maxlen = j;

	Fprintf(fd, "Aliases:-\n");

	for ( epp = AliasList, i = AliasCount ; --i >= 0 ; epp++ )
		Fprintf
		(
			fd,
			"%*s ==> %s\n",
			maxlen,
			(*epp)->en_name,
			(*epp)->MAP
		);

	putc('\n', fd);
}
