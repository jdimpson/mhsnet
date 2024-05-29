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


#define	STDIO

#include	"global.h"
#include	"commandfile.h"
#include	"debug.h"



static bool	next_cmd _FA_((FILE *, char *));

/*
**	Read commands from `fd' and pass them back via function.
**
**	Returns TRUE if no errors.
*/

bool
ReadFdCmds(fd, funcp)
	FILE *		fd;
#	if	ANSI_C
	bool		(*funcp)(CmdT, CmdV);
#	else	/* ANSI_C */
	bool		(*funcp)();
#	endif	/* ANSI_C */
{
	register char *	cp;
	CmdV		val;
	CmdT		type;
	CmdC		conv;

	Trace3(2, "ReadFdCmds(%#lx, %#lx)", (PUlong)fd, (PUlong)funcp);

	cp = Malloc(MAXCOMZ);

	while ( next_cmd(fd, cp) )
	{
		if ( (unsigned)(type = (CmdT)*cp) > (unsigned)last_cmd )
			conv = error_type;
		else
			conv = CmdfTypes[(int)type];

		switch ( conv )
		{
		case number_type:
			val.cv_number = xtol(cp+1);
			Trace4(3, "ReadFdCmd %s: %s [%s]", CmdfDescs[(int)type], cp+1, CmdToString(type, val.cv_number));
			break;

		case string_type:
			val.cv_name = cp+1;
			Trace3(3, "ReadFdCmd %s: %s", CmdfDescs[(int)type], cp+1);
			break;

		default:
			free(cp);
			return false;
		}

		if ( !(*funcp)(type, val) )
		{
			free(cp);
			return false;
		}
	}

	free(cp);

	if ( !feof(fd) )
		return false;

	return true;
}



static bool
next_cmd(fd, buf)
	register FILE *	fd;
	register char *	buf;
{
	register char *	cp;
	register int	c;

	cp = buf;
	buf += MAXCOMZ;

	while ( (c = getc(fd)) != EOF )
		if ( cp < buf && (*cp++ = c) == '\0' )
			return true;

	return false;
}
