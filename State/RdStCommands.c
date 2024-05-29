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


#define	FILE_CONTROL
#define	STAT_CALL
#define	STDIO

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"statefile.h"


static char *	CommandsFile;


/*
**	Open commands file and read it.
*/

bool
RdStCommands(warn)
	bool		warn;
{
	register FILE *	fd;
	bool		retval = false;
	struct stat	statb;

	if ( CommandsFile == NULLSTR )
		CommandsFile = concat(SPOOLDIR, STATEDIR, COMMANDSFILE, NULLSTR);

	Trace3(1, "RdStCommands(%s) <== %s", warn?"warn":EmptyString, CommandsFile);
	
	if ( (fd = fopen(CommandsFile, "r")) != NULL )
	{
		retval = Rcommands(fd, warn);
		(void)fclose(fd);
	}
	else
	if ( warn || stat(CommandsFile, &statb) != SYSERROR )
		(void)SysWarn(CouldNot, ReadStr, CommandsFile);

	return retval;
}



/*
**	Change name of commands file.
*/

void
SetCommands(newfile)
	char *		newfile;
{
	CommandsFile = newfile;

	Trace2(1, "SetCommands(%s)", CommandsFile);
}
