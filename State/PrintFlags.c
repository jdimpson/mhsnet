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


static char **	FlagNames;
static char *	FlagStr;
static int	FlagLen;



/*
**	Print flags as string.
*/

int
PrintFlags(fd, flags)
	FILE *	fd;
	Flags	flags;
{
	(void)StringFlags(flags);

	if ( FlagLen > 0 )
		(void)fputs(FlagStr, fd);

	return FlagLen;
}



/*
**	Produce printable representation of Flags.
*/

char *
StringFlags(flags)
	Flags		flags;
{
	register char *	cp;
	register int	i;
	register int	j;
	register int	n;
	register int	z;

	Trace2(3, "StringFlags(%#lx)", (PUlong)flags);

	if ( FlagNames == (char **)0 )
	{
		Trace3(2, "PrintFlags(%#lx) set FlagNames [%d]", (PUlong)flags, MAXFLAGS);

		FlagNames = (char **)Malloc(sizeof(char *) * MAXFLAGS);

		for ( i = 1, j = 0, z = 0 ; j < MAXFLAGS ; i <<= 1, j++ )
		{
			FlagNames[j] = Unknown;

			for ( n = NSFLGS ; --n >= 0 ; )
				if ( SortedFlags[n].fl_value == i )
				{
					FlagNames[j] = SortedFlags[n].fl_name;
					break;
				}

			Trace3(3, "Flag[%d] = \"%s\"", j, FlagNames[j]);

			z += strlen(FlagNames[j]);
		}

		FlagStr = Malloc(z+MAXFLAGS+3);
	}

	if ( flags == 0 )
	{
		FlagStr[0] = '\0';
		FlagLen = 0;
		return FlagStr;
	}

	cp = FlagStr;
	*cp++ = '(';

	for ( n = 0, i = 1, j = 0 ; j < MAXFLAGS ; i <<= 1, j++ )
		if ( flags & i )
		{
			if ( n++ )
				*cp++ = ',';
			cp = strcpyend(cp, FlagNames[j]);
		}

	*cp++ = ')';
	*cp = '\0';

	FlagLen = cp - FlagStr;
	return FlagStr;
}
