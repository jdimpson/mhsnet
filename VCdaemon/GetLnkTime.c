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

#include	"global.h"
#include	"debug.h"
#include	"spool.h"

#include	"driver.h"
#include	"packet.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"pktstats.h"
#include	"status.h"


/*
**	Version of statusfile should be:
*/

static char	ChanVersion[]	= CHANVERSION;



/*
**	Find link reader status file, and pass back last packet read time.
*/

Time_t
GetLnkTime(dir)
	char *		dir;
{
	register int	i;
	register int	fd;
	char *		statusname;
	VCstatus	status;
	DODEBUG(char	tbuf[DATETIMESTRLEN]);

	Trace2(1, "GetLnkTime(%s)", dir);

	statusname = concat(dir, Slash, READERSTATUS, NULLSTR);

	i = MIN_STATUS_SIZE(status);

	if
	(
		(fd = open(statusname, O_READ)) == SYSERROR
		||
		read(fd, (char *)&status, i) != i
		||
		strcmp(status.st_version, ChanVersion) != STREQUAL
	)
		status.st_lastpkt = (Time_t)0;

	if ( fd != SYSERROR )
		(void)close(fd);

	Trace3(2, "GetLnkTime read \"%s\", time %s", statusname, DateTimeStr(status.st_lastpkt, tbuf));

	free(statusname);

	return status.st_lastpkt;
}
