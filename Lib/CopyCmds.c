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
#include	"commandfile.h"
#include	"debug.h"



/*
**	Copy commands from one header to another.
*/

void
CopyCmds(from, to)
	register CmdHead *	from;
	register CmdHead *	to;
{
	register Cmd *		cep;
	register CmdT		type;
	register CmdV		val;
	CmdC			conv;

	Trace3(1, "CopyCmds(%#lx, %#lx)", (PUlong)from, (PUlong)to);

	for ( cep = from->ch_first; cep != (Cmd *)0 ; cep = cep->cd_next )
	{
		if ( (unsigned)(type = cep->cd_type) > (unsigned)last_cmd )
			conv = error_type;
		else
			conv = CmdfTypes[(int)type];

		switch ( conv )
		{
		case number_type:
			val.cv_number = cep->cd_number;
			break;

		case string_type:
			val.cv_name = cep->cd_value;
			break;

		default:
			Fatal2("Bad type %d in CopyCmds", (int)type);
		}

		(void)AddCmd(to, type, val);
	}
}
