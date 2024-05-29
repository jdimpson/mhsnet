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

#define	CMD_NAMES_DATA
#define	CMD_FLAGS_DATA

#include	"commandfile.h"



/*
**	Make up string with value of a ``number_type'' command.
*/

char *
CmdToString(type, number)
	CmdT			type;
	Ulong			number;
{
	register char *		cp;
	register char **	cpp;
	register Ulong		bit;
	register Ulong		flags;
	register bool		first = true;
	static char		buf[128];

	switch ( type )
	{
	default:
		(void)sprintf(buf, "%lu", (PUlong)number);
		return buf;

	case timeout_cmd:
	case traveltime_cmd:
		return TimeString(number, buf);

	case flags_cmd:
		break;
	}

	for ( flags = number, cp = buf, bit = 1, cpp = CmdFlgDescs ; *cpp != NULLSTR ; cpp++, bit <<= 1 )
		if ( bit & flags )
		{
			if ( first )
				first = false;
			else
				*cp++ = '|';

			cp = strcpyend(cp, *cpp);
		}

	*cp++ = '\0';

	return buf;
}
