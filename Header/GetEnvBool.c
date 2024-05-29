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
**	Look for an environment field tagged by 'name'.
**	If the field exists, return true, otherwise false.
*/

bool
GetEnvBool(name)
	char *		name;
{
	register char *	ep;
	register char *	fp;
	register char *	sp;
	register int	val;

	if ( (ep = HdrEnv) != NULLSTR )
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
			if ( sp != NULLSTR )
				*sp = ENV_RS;

			Trace2(2, "GetEnvBool(%s) ==> true", name);
			return true;
		}

		if ( (ep = sp) == NULLSTR )
			break;

		*ep++ = ENV_RS;
	}

	Trace2(2, "GetEnvBool(%s) ==> false", name);
	return false;
}
