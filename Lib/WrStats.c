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
#include	"debug.h"
#include	"stats.h"


static char *	StatsFile;
static FILE *	StatsFd;



/*
**	Close statistics file.
*/

bool
CloseStats()
{
	if ( StatsFd == NULL )
		return true;

	if ( fclose(StatsFd) == EOF )
	{
		StatsFd = NULL;
		return false;
	}

	StatsFd = NULL;
	return true;
}



/*
**	Close statistics file.
*/

bool
FlushStats()
{
	if ( StatsFd == NULL )
		return true;

	if ( fflush(StatsFd) == EOF )
		return false;

	return true;
}



/*
**	Write out a statistics record.
*/

bool
WrStats(id, funcp)
	int		id;
	bool		(*funcp)();	/* Caller supplied function to write a field */
{
	register int	i;

	if ( StatsFile == NULLSTR )
		StatsFile = concat(SPOOLDIR, STATSDIR, STATSFILE, NULLSTR);

	if
	(
		StatsFd == NULL
		&&
		(
			access(StatsFile, 0) == SYSERROR
			||
			(StatsFd = fopen(StatsFile, "a")) == NULL
		)
	)
		return false;

/*	(void)fseek(StatsFd, 0L, 2);	*/

	putc(id, StatsFd);

	i = 0;

	do
		putc(ST_RE_SEP, StatsFd);
	while
		( (*funcp)(StatsFd, i++) );

	putc(ST_SEP, StatsFd);

	return true;
}
