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
**	Extract files info to FthFiles from FthFdescs list.
**
**	Returns "fth_ok" if successful, otherwise a failure reason.
*/

FthReason
GetFthFiles()
{
	register char *		cp;
	register int		i;
	register FthFD_p	flp;
	register FthFD_p *	endflp = &FthFiles;
	register char *		ep;
	char *			p[4];

	if ( (cp = FthFdescs) == NULLSTR || *cp == '\0' )
	{
		NFthFiles = 1;
		
		if ( (flp = FthFFreeList) == (FthFD_p)0 )
			flp = Talloc(FthFDesc);
		else
			FthFFreeList = flp->f_next;

		*endflp = flp;
		endflp = &flp->f_next;

		flp->f_name = "<?>";
		flp->f_alias = flp->f_name;
		flp->f_length = 0;
		flp->f_time = 0;
		flp->f_mode = 0;
		return fth_ok;
	}

	NFthFiles = 0;

	do
	{
		if ( (ep = strchr(cp, FTH_FSEP)) != NULLSTR )
			*ep++ = '\0';
		
		p[0] = cp;

		for ( i = 1 ; i < 4 ; i++ )
			if ( (cp = strchr(cp, FTH_FDSEP)) != NULLSTR )
			{
				*cp++ = '\0';
				p[i] = cp;
			}
			else
				return fth_files;
		
		NFthFiles++;

		if ( (flp = FthFFreeList) == (FthFD_p)0 )
			flp = Talloc(FthFDesc);
		else
			FthFFreeList = flp->f_next;

		*endflp = flp;
		endflp = &flp->f_next;

		flp->f_name = newstr(p[0]);
		flp->f_alias = flp->f_name;
		UnQuoteChars(flp->f_name, FthFdRestricted);
		flp->f_length = atol(p[1]);
		flp->f_time = atol(p[2]);
		flp->f_mode = otol(p[3]);
	}
	while
		( (cp = ep) != NULLSTR );

	*endflp = (FthFD_p)0;

	return fth_ok;
}
