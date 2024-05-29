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
**	Change the value of a command.
*/

bool
ChangeCmd(cmdpp, type, oldval, newval)
	Cmd **		cmdpp;
	CmdT		type;
	CmdV		oldval;
	CmdV		newval;
{
	register Cmd *	cmdp;
	CmdC		conv;

	Trace5(1,
		"ChangeCmd(%#lx, %s, %#lx, %#lx)",
		(PUlong)cmdpp,
		CmdfDescs[(int)type],
		(PUlong)oldval.cv_number,
		(PUlong)newval.cv_number
	);

	if ( cmdpp == (Cmd **)0 || (cmdp = *cmdpp) == (Cmd *)0 || cmdp->cd_type != type )
		return false;

	if ( (unsigned)type > (unsigned)last_cmd )
		conv = error_type;
	else
		conv = CmdfTypes[(int)type];

	switch ( conv )
	{
	case number_type:
		if ( cmdp->cd_number != oldval.cv_number )
			return false;
		cmdp->cd_number = newval.cv_number;
		cmdp->cd_value = cmdp->cd_string;
		(void)sprintf(cmdp->cd_string, "%lx", (PUlong)cmdp->cd_number);
		Trace5(1,
			"ChangeCmd(%s, %#lx, %s [%s])",
			CmdfDescs[(int)type],
			(PUlong)oldval.cv_number,
			cmdp->cd_value,
			CmdToString(type, cmdp->cd_number)
		);
		break;

	case string_type:
		if ( strcmp(cmdp->cd_value, oldval.cv_name) != STREQUAL )
			return false;
		free(cmdp->cd_value);
		cmdp->cd_value = newstr(newval.cv_name);
		Trace4(1, "ChangeCmd(%s, %s, %s)", CmdfDescs[(int)type], oldval.cv_name, cmdp->cd_value);
		break;

	default:
		Fatal2("Bad type %d in ChangeCmd", (int)type);
	}

	return true;
}
