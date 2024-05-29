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
**	Free and initialise a command list.
*/

void
FreeCmds(chp)
	register CmdHead *	chp;
{
	register Cmd *		cep;

	Trace2(2, "FreeCmds(%#lx)", (PUlong)chp);

	while ( (cep = chp->ch_first) != (Cmd *)0 )
	{
		chp->ch_first = cep->cd_next;

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

	InitCmds(chp);
}



/*
**	Initialise a command list.
*/

void
InitCmds(chp)
	register CmdHead *	chp;
{
	Trace2(2, "InitCmds(%#lx)", (PUlong)chp);

	chp->ch_first = (Cmd *)0;
	chp->ch_last = &chp->ch_first;
	chp->ch_count = 0;
}
