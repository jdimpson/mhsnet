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


static bool	process_cmds _FA_((char *, bool (*)(CmdT, CmdV)));



/*
**	Read in a command file and pass back commands via function.
**
**	Returns TRUE if no errors.
*/

bool
ReadCmds(file, funcp)
	char *	file;
#	if	ANSI_C
	bool	(*funcp)(CmdT, CmdV);
#	else	/* ANSI_C */
	bool	(*funcp)();
#	endif	/* ANSI_C */
{
	char *	cp;

	Trace3(2, "ReadCmds(%s, %#lx)", file, (PUlong)funcp);

	while ( (cp = ReadFile(file)) == NULLSTR )
		if ( !SysRetry(errno) )
			return false;

	return process_cmds(cp, funcp);
}



bool
ReadCmdsFd(fd, funcp)
	int	fd;
#	if	ANSI_C
	bool	(*funcp)(CmdT, CmdV);
#	else	/* ANSI_C */
	bool	(*funcp)();
#	endif	/* ANSI_C */
{
	char *	cp;

	Trace3(2, "ReadCmdsFd(%d, %#lx)", fd, (PUlong)funcp);

	while ( (cp = ReadFd(fd)) == NULLSTR )
		if ( !SysRetry(errno) )
			return false;

	return process_cmds(cp, funcp);
}



static bool
process_cmds(commands, funcp)
	char *		commands;
#	if	ANSI_C
	bool		(*funcp)(CmdT, CmdV);
#	else	/* ANSI_C */
	bool		(*funcp)();
#	endif	/* ANSI_C */
{
	register char *	cp;
	register char *	end;
	CmdV		val;
	CmdT		type;
	CmdC		conv;

	cp = commands;

	end = &cp[RdFileSize];

	do
	{
		if ( (unsigned)(type = (CmdT)*cp++) > (unsigned)last_cmd )
			conv = error_type;
		else
			conv = CmdfTypes[(int)type];

		switch ( conv )
		{
		case number_type:
			val.cv_number = xtol(cp);
			Trace4(3, "ReadCmd %s: %s [%s]", CmdfDescs[(int)type], cp, CmdToString(type, val.cv_number));
			break;

		case string_type:
			val.cv_name = cp;
			Trace3(3, "ReadCmd %s: %s", CmdfDescs[(int)type], cp);
			break;

		default:
			Trace3(3, "ReadCmd ERROR unrecognised command type [%d] string \"%s\"", type, ExpandString(cp, -1));
			free(commands);
			return false;
		}

		if ( !(*funcp)(type, val) )
		{
			free(commands);
			return false;
		}
	}
	while
		( (cp += (strlen(cp)+1)) < end );

	free(commands);

	if ( cp != end )
		return false;

	return true;
}
