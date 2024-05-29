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
#include	"ftheader.h"


/*
**	Initialise FTP header details
**	(necessary only if ReadFtHeader() error is to be ignored).
*/

void
InitFtHeader()
{
	register char **	fp;
	register char *		cp;
	register int		i;

	FthType[0] = '0';

	for
	(
		fp = FthFields.fth_start, i = 0 ;
		i < NFTHFIELDS ;
		i++, fp++
	)
	{
		switch ( (FthReason)i )
		{
		case fth_type:
			cp = "0";
			break;
		case fth_files:
			{	static char	null_files[] = {FTH_FDSEP, FTH_FSEP, '\0'};
				cp = null_files;
			}
			break;
		case fth_to:
			{	static char	null_to[] = {FTH_UDEST, FTH_UDSEP, '\0'};
				cp = null_to;
			}
			break;
		default:
			cp = EmptyString;
		}

		*fp = cp;
	}
}
