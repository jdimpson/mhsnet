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
**	Extract inbound active message filenames from READERSTATUS,
**	and pass them back via passed function.
**
**	Returns true if all ok, else false.
*/

bool
GetInStatusFiles(dir, funcp)
	char *		dir;
	vFuncp		funcp;
{
	register Chan *	chnp;
	register int	i;
	register char *	fn;
	register int	shortlen;
	int		wholelen;
	int		fd;
	int		n;
	int		count;
	char *		statusname;
	VCstatus	status;

	Trace3(1, "GetInStatusFiles(%s, %#lx)", dir, (PUlong)funcp);

	statusname = concat(dir, READERSTATUS, NULLSTR);

	i = MIN_STATUS_SIZE(status);

	for
	(
		count = 0
		;
		(fd = open(statusname, O_READ)) == SYSERROR
		||
		(n = read(fd, (char *)&status, i)) != i
		||
		strcmp(status.st_version, ChanVersion) != STREQUAL
		;
		count++
	)
	{
		if ( fd != SYSERROR )
		{
			(void)close(fd);

			if ( n == 0 && count < 3 )
			{
				(void)sleep(MINSLEEP);
				continue;
			}

			Warn
			(
				"\"%s\": %s", statusname,
				(n==0)?english("zero length"):english("status file version mis-match")
			);
		}
		else
			(void)SysWarn(CouldNot, OpenStr, statusname);

		free(statusname);
		return false;
	}

	(void)close(fd);

	Trace2(2, "GetInStatusFiles read \"%s\"", statusname);

	free(statusname);

	dir = concat(dir, LINKMSGSINNAME, Slash, NULLSTR);
	shortlen = strlen(dir);
	wholelen = shortlen + MSGIDLENGTH;
	fn = strcpy(Malloc(wholelen + 1), dir);
	fn[wholelen] = '\0';

	for ( chnp = status.st_channels, i = 0 ; i < NCHANNELS ; i++, chnp++ )
		if ( chnp->ch_msgid[0] != '\0' )
		{
			(void)strncpy(&fn[shortlen], chnp->ch_msgid, MSGIDLENGTH);
			(*funcp)(fn);
		}

	free(fn);
	free(dir);

	return true;
}
