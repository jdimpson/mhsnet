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
#include	"sysexits.h"

#include	"setup.h"
#include	"shell.h"



/*
**	Accept `XTI' connection -- routine for shell.
*/

char *
a_xti(tp)
	VCtype *	tp;
{
	register int	i;
	char		source[MAX_LINE_LEN];

	for ( i = 0 ; i < MAX_LINE_LEN ; )
	{
		char	c;

		if ( read(0, &c, 1) <= 0 )
			finish(EX_IOERR);

		if ( c == 0 )
			break;

		source[i++] = c;
	}

	source[i] = '\0';

	return newstr(source);
}
