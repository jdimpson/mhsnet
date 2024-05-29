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

#undef	Extern
#define	Extern
#define	ExternU
#include	"Passwd.h"



/*
**	Given a site address,
**	fill a structure with details from the `_call/passwd' file.
*/

bool
GetPassword(pp, name)
	Passwd *	pp;
	char *		name;
{
	register char *	cp1;
	register char *	cp2;
	register char *	cp;
	register char *	np;

	Trace2(1, "GetPassword(%s)", name);

	pp->P_name = name;
	pp->P_crypt[0] = '\0';

	if ( PasswdFile == NULLSTR )
		PasswdFile = concat(SPOOLDIR, CALLDIR, "passwd", NULLSTR);

	if ( (cp = PasswdData) == NULLSTR )
	{
		if ( (cp = ReadFile(PasswdFile)) == NULLSTR )
			return false;

		PasswdData = cp;
		cp[RdFileSize++] = '\n';	/* Ensure terminating '\n' */
		cp[RdFileSize] = '\0';

		PasswdLength = RdFileSize;
	}

	for ( ; (np = strchr(cp, '\n')) != NULLSTR ; *np++ = '\n', cp = np )
	{
		*np = '\0';

		Trace2(4, "match line {%s} ", cp);

		if ( cp[0] == '#' || (cp1 = strchr(cp, ':')) == NULLSTR )
			continue;

		*cp1 = '\0';

		if ( !AddressMatch(cp, name) )
		{
			*cp1 = ':';
			continue;
		}

		if ( MatchedBC != NULLSTR )
			pp->P_name = newstr(MatchedBC);

		*cp1++ = ':';

		if ( (cp2 = strchr(cp1, ':')) != NULLSTR )
		{
			*cp2 = '\0';
			(void)strncpy(pp->P_crypt, cp1, sizeof(pp->P_crypt));
			*cp2++ = ':';
			PasswdTail = cp2;
		}
		else
			PasswdTail = np;

		*np++ = '\n';

		PasswdEnd = np;
		PasswdPoint = cp1;
		PasswdStart = cp;

		return true;
	}

	return false;
}
