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
**	Extract an environment field tagged by 'name'.
**	If the field exists, a pointer will be returned to reallocated field,
**	even if the field is empty; otherwise NULLSTR will be returned.
*/

char *
GetEnv(name)
	char *		name;
{
	register char *	ep;
	register char *	fp;
	register char *	sp;
	register int	val;

	DODEBUG(static char	tfmt[] = "GetEnv(%s) ==> %s");

	if ( (ep = HdrEnv) == NULLSTR )
		return NULLSTR;

	for ( ;; )
	{
		if ( (sp = strchr(ep, ENV_RS)) != NULLSTR )
			*sp = '\0';
		
		if ( (fp = strchr(ep, ENV_US)) != NULLSTR )
			val = strncmp(name, ep, fp-ep);
		else
			val = strcmp(name, ep);
		
		if ( val == STREQUAL )
		{
			if ( fp != NULLSTR )
			{
				if ( sp != NULLSTR )
					ep = newnstr(fp+1, (sp-fp)-1);
				else
					ep = newstr(fp+1);
			}
			else
			{
				ep = Malloc(1);
				*ep = '\0';
			}

			if ( sp != NULLSTR )
				*sp = ENV_RS;
			
			UnQuoteChars(ep, EnvRestricted);

			Trace3(2, tfmt, name, ep);
			return ep;
		}

		if ( (ep = sp) == NULLSTR )
			break;

		*ep++ = ENV_RS;
	}

	Trace3(2, tfmt, name, NullStr);
	return NULLSTR;
}
