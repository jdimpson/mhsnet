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
**	Add a command to list.
*/

char *
AddCmd(chp, type, val)
	register CmdHead *	chp;
	CmdT			type;
	CmdV			val;
{
	register Cmd *		cep;
	CmdC			conv;

	if ( (cep = CmdFreelist) != (Cmd *)0 )
		CmdFreelist = cep->cd_next;
	else
		cep = Talloc(Cmd);

	cep->cd_next = (Cmd *)0;

	if ( (unsigned)(cep->cd_type = type) > (unsigned)last_cmd )
		conv = error_type;
	else
		conv = CmdfTypes[(int)type];

	switch ( conv )
	{
	case number_type:
		cep->cd_number = val.cv_number;
		cep->cd_value = cep->cd_string;
		(void)sprintf(cep->cd_string, "%lx", (PUlong)cep->cd_number);
		Trace5(2,
			"AddCmd(%#lx, %s, 0x%s [%s])",
			(PUlong)chp,
			CmdfDescs[(int)type],
			cep->cd_value,
			CmdToString(type, cep->cd_number)
		);
		break;

	case string_type:
		cep->cd_value = newstr(val.cv_name);
		Trace4(2,
			"AddCmd(%#lx, %s, %s)",
			(PUlong)chp,
			CmdfDescs[(int)type],
			cep->cd_value
		);
		break;

	default:
		Fatal2("Bad type %d in AddCmd", (int)type);
	}

	*chp->ch_last = cep;
	chp->ch_last = &cep->cd_next;
	chp->ch_count++;

	return cep->cd_value;
}
