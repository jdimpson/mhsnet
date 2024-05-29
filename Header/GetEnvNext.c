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
#include	"header.h"



/*
**	Find an environment field tagged by 'name' and 'value',
**	and then extract next environment field tagged by 'name'.
**
**	WARNING: if any name=value pair is repeated,
**	then GetEnvNext() will never return NULLSTR.
**
**	If the field exists, a pointer will be returned to reallocated field,
**	even if the field is empty; otherwise NULLSTR will be returned.
*/

char *
GetEnvNext(name, value)
	char *		name;
	char *		value;
{
	register char *	ep;
	register char *	fp;
	register char *	sp;
	register char *	vp;
	register bool	next;

	DODEBUG(static char	tfmt[] = "GetEnvNext(%s, %s) ==> %s");

	if ( (ep = HdrEnv) == NULLSTR || (vp = value) == NULLSTR || vp[0] == '\0' )
		return NULLSTR;

	vp = newstr(vp);
	QuoteChars(vp, EnvRestricted);

	for ( next = false ;; )
	{
		if ( (sp = strchr(ep, ENV_RS)) != NULLSTR )
			*sp = '\0';
		
		if
		(
			(fp = strchr(ep, ENV_US)) != NULLSTR
			&&
			strncmp(name, ep, fp-ep) == STREQUAL
			&&
			(next || strcmp(vp, fp+1) == STREQUAL)
		)
		{
			if ( !next )
				next = true;
			else
			{
				free(vp);

				if ( sp != NULLSTR )
					ep = newnstr(fp+1, (sp-fp)-1);
				else
					ep = newstr(fp+1);

				if ( sp != NULLSTR )
					*sp = ENV_RS;
				
				UnQuoteChars(ep, EnvRestricted);

				Trace4(2, tfmt, name, value, ep);
				return ep;
			}
		}

		if ( (ep = sp) == NULLSTR )
			break;

		*ep++ = ENV_RS;
	}

	free(vp);

	Trace4(2, tfmt, name, value, NullStr);
	return NULLSTR;
}
