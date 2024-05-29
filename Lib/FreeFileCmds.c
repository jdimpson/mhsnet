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
#include	"commandfile.h"



/*
**	Free file-manipulation commands from a command list.
*/

void
FreeFileCmds(chp)
	register CmdHead *	chp;
{
	register Cmd *		cep;
	register Cmd **		cepp;

	Trace2(2, "FreeFileCmds(%#lx)", (PUlong)chp);

	for ( cepp = &chp->ch_first ; (cep = *cepp) != (Cmd *)0 ; )
	{
		switch ( cep->cd_type )
		{
		default:
			cepp = &cep->cd_next;
			continue;

		case file_cmd:
		case start_cmd:
		case end_cmd:
		case unlink_cmd:
			;
		}

		*cepp = cep->cd_next;
		if ( --chp->ch_count == 0 )
			chp->ch_last = &chp->ch_first;

		if
		(
			(unsigned)cep->cd_type <= (unsigned)last_cmd
			&&
			CmdfTypes[(int)cep->cd_type] == string_type
		)
			free(cep->cd_value);

		cep->cd_next = CmdFreelist;
		CmdFreelist = cep;
	}
}
