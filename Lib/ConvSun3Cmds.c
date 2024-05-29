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
**	Read in a Sun3 command file and change commands into Sun4 ones.
**
**	Returns range of data described.
*/

Ulong
ConvSun3Cmds(file, cmdh)
	char *		file;
	CmdHead *	cmdh;
{
	register char *	cp;
	register char *	ep;
	register int	conv;
	CmdV		name;
	CmdV		base;
	CmdV		end;
	Ulong		range;
	char *		commands;
	Ulong		length = 0;

	Trace3(1, "ConvSun3Cmds(%s, %#lx)", file, (PUlong)cmdh);

	if ( (cp = ReadFile(file)) == NULLSTR )
	{
		Syserror(CouldNot, ReadStr, file);
		return length;
	}

	commands = cp;

	ep = &cp[RdFileSize];
	conv = 0;

	do
	{
		switch ( conv )
		{
		case 0:
			name.cv_name = cp;
			conv++;
			break;

		case 1:
			base.cv_number = atol(cp);
			conv++;
			break;

		case 2:
			if ( (range = atol(cp)) == 0 )
				(void)AddCmd(cmdh, unlink_cmd, name);
			else
			{
				length += range;
				end.cv_number = base.cv_number + range;
				(void)AddCmd(cmdh, file_cmd, name);
				(void)AddCmd(cmdh, start_cmd, base);
				(void)AddCmd(cmdh, end_cmd, end);
			}
			conv = 0;
			break;
		}
	}
	while
		( (cp += (strlen(cp)+1)) < ep );

	free(commands);

	if ( cp != ep || conv != 0 )
	{
		Error(english("bad Sun3 command file \"%s\""), file);
		return (Ulong)0;
	}

	return length;
}
