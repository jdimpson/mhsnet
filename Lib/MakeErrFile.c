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


/*
**	Create a file for error output (use "KeepErrFile" if set),
**	unless "stderr" is already assigned to a tty.
*/

#define	FILE_CONTROL
#define	STAT_CALL

#include	"global.h"
#include	"debug.h"
#include	"spool.h"

char *		KeepErrFile;



void
MakeErrFile(fdp)
	int *		fdp;
{
	register char *	errfile;
	char		template[UNIQUE_NAME_LENGTH+1];

	Trace3(1, "MakeErrFile(%#lx), KeepErrFile=%s", (PUlong)fdp, (KeepErrFile==NULLSTR)?NullStr:KeepErrFile);

	if ( ErrorTty(fdp) )
		return;

	if ( (errfile = KeepErrFile) == NULLSTR )
	{
		(void)sprintf(template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
		errfile = UniqueName(concat(TMPDIR, template, NULLSTR), CHOOSE_QUALITY, OPTNAME, (Ulong)0, Time);
	}

	*fdp = createn(errfile);	/* Might return SYSERROR if TMPDIR unwriteable! */

	Trace3(2, "MakeErrFile() ==> \"%s\" (%d)", errfile, *fdp);

	if ( KeepErrFile == NULLSTR && Traceflag == 0 )
	{
		(void)unlink(errfile);
		free(errfile);
	}
	else
		(void)chmod(errfile, 0640);
}
