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
#include	"ftheader.h"



/*
**	Concatenate info from FthFiles list into FthFdesc field.
*/

int
SetFthFiles()
{
	register int		n;
	register char *		cp;
	register FthFD_p	flp;
	register int		count;

	for ( flp = FthFiles, n = 0 ; flp != (FthFD_p)0 ; flp = flp->f_next )
		n += strlen(flp->f_alias) + ULONG_SIZE + TIME_SIZE+1 + MODE_SIZE + 4;

	if ( n == 0 )
	{
		FthFdescs = NULLSTR;
		return 0;
	}

	FthFdescs = cp = Malloc(n);

	for ( count = 1, flp = FthFiles ; ; count++ )
	{
		register char *	ocp = cp;
		char		length[ULONG_SIZE];

		cp = strcpyend(cp, flp->f_alias);
		QuoteChars(ocp, FthFdRestricted);
		*cp++ = FTH_FDSEP;

		(void)sprintf(length, "%lu", (PUlong)flp->f_length);
		cp = strcpyend(cp, length);
		*cp++ = FTH_FDSEP;

		(void)sprintf(length, "%lu", (PUlong)flp->f_time);
		cp = strcpyend(cp, length);
		*cp++ = FTH_FDSEP;

		(void)sprintf(length, "%3o", flp->f_mode & FTH_MODES);
		cp = strcpyend(cp, length);

		if ( (flp = flp->f_next) != (FthFD_p)0 )
			*cp++ = FTH_FSEP;
		else
			break;
	}

	return count;
}
