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
#include	"sysexits.h"

#include	"call.h"



bool
match(s, pp)
	char *	s;
	Pat *	pp;
{
	if ( Monitorflag >= 5 && s[0] != '\n' && s[1] != '\0' )
		MesgN
		(
			english("match"),
			english("\"%s\" =?= \"%s\""),
			ExpandString(s, -1),
			pp->pattern
		);

	if ( strcmp(s, UNDEFINED) == STREQUAL )
	{
		if ( strcmp(pp->pattern, UNDEFINED) == STREQUAL )
			return true;

		return false;
	}

	if
	(
		pp->comp_pat == NULLSTR
		&&
		(pp->comp_pat = regcmp(pp->pattern, NULLSTR)) == NULLSTR
	)
	{
		Error(english("cannot compile pattern \"%s\""), pp->pattern);
		finish(EX_DATAERR);
	}

	if ( regex(pp->comp_pat, s, NULLSTR) != NULLSTR )
		return true;

	return false;
}
