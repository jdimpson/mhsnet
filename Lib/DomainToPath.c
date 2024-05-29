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

static char *	nextdom _FA_((char *, char *));



/*
**	Change a domain name to a path name.
*/

char *
DomainToPath(s)
	char *		s;
{
	register char *	cp;
	register char *	np;
	char *		p;

	Trace2(2, "DomainToPath(%s)", s);

	if ( (cp = s) == NULLSTR )
		return NULLSTR;

	while ( *cp == DOMAIN_SEP )
		cp++;

	if ( *cp == '\0' )
		return NULLSTR;

	p = np = Malloc(strlen(cp)+1);

	np = nextdom(np, cp);
	*--np = '\0';

	Trace2(2, "DomainToPath() ==> \"%s\"", p);

	return p;
}



static char *
nextdom(np, s)
	register char *	np;
	register char *	s;
{
	register int	c;
	register char *	cp;
#	if	TRUNCDIRNAME == 1
	register char *	mp;
#	endif	/* TRUNCDIRNAME == 1 */

	if ( (cp = strchr(s, DOMAIN_SEP)) != NULLSTR )
	{
		*cp++ = '\0';
		np = nextdom(np, cp);
	}

#	if	TRUNCDIRNAME == 1
	mp = np + 14;
#	endif	/* TRUNCDIRNAME == 1 */

	while ( (c = *s++) != 0 )
	{
		if ( c <= 'Z' && c >= 'A' )
			c |= 040;
		*np++ = c;
	}

#	if	TRUNCDIRNAME == 1
	if ( np > mp )
		np = mp;
#	endif	/* TRUNCDIRNAME == 1 */
	*np++ = '/';

	if ( cp != NULLSTR )
		*--cp = DOMAIN_SEP;

	return np;
}
