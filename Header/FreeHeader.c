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
#include	"header.h"
#include	"debug.h"



/*
**	Free any "Malloc"ed fields in header structure.
*/

void
FreeHeader()
{
	register int		i;
	register HdrFld *	hfp;

	Trace1(1, "FreeHeader()");

	for ( hfp = HdrFields.hdr_field, i = 0 ; i < NHDRFIELDS ; i++, hfp++ )
		if ( hfp->hf_free )
		{
			Trace3(3,
				"FreeHeader field %s pointer %#lx",
				HdrDescs[i],
				(PUlong)hfp->hf_string
			);
			if ( HdrConvs[i] == string_field )
				FreeStr(&hfp->hf_string);
			hfp->hf_free = false;
		}
}
