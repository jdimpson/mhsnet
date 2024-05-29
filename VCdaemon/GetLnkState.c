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
#define	SIGNALS
#define	STDIO
#define	ERRNO

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


static char *	checkchans _FA_((VCstatus *));
static bool	checkpid _FA_((int));



/*
**	Find link status, and pass back strings and pid.
*/

bool
GetLnkState(dir, d_state, l_state, d_pid, check)
	char *		dir;
	char **		d_state;
	char **		l_state;
	int *		d_pid;
	bool		check;
{
	register int	i;
	register int	fd;
	char *		statusname;
	VCstatus	status;

	Trace3(1, "GetLnkState(%s,,,,%scheck)", dir, check?EmptyString:"no");

	statusname = concat(dir, WRITERSTATUS, NULLSTR);

	i = MIN_STATUS_SIZE(status);

	if
	(
		(fd = open(statusname, O_READ)) == SYSERROR
		||
		read(fd, (char *)&status, i) != i
		||
		strcmp(status.st_version, ChanVersion) != STREQUAL
	)
	{
		(void)strclr((char *)&status, i);
		Trace2(2, "GetLnkState can't read \"%s\"", statusname);
	}
	else
		Trace2(2, "GetLnkState read \"%s\"", statusname);

	if ( fd != SYSERROR )
		(void)close(fd);

	free(statusname);

	if ( !check || checkpid(status.st_pid) )
	{
		*d_pid = status.st_pid;
		*d_state = checkchans(&status);
		if
		(
			(status.st_flags & CHLINKDOWN)
/*			&&
**			status.st_lastpkt != 0
**			&&
**			status.st_lastpkt < (Time-LINK_DOWN_TIME)
*/		)
			*l_state = english("down");
		else
			*l_state = english("up");
		return true;
	}

	*d_pid = 0;
	*d_state = english("inactive");
	*l_state = english("down");

	return false;
}



/*
**	Scan channels for state.
*/

static char *
checkchans(sp)
	VCstatus *	sp;
{
	register Chan *	chnp;
	register int	i;

	Trace1(2, "checkchans()");

	for ( chnp = &sp->st_channels[0], i = 0 ; i < NCHANNELS ; i++, chnp++ )
	{
		switch ( chnp->ch_state )
		{
		case CH_ENDED:
		case CH_IDLE:		continue;
		default:		return english("active");
		}
	}

	return english("idle");
}



/*
**	Check that ``pid'' is active.
*/

static bool
checkpid(pid)
	int	pid;
{
	Trace2(2, "checkpid(%d)", pid);

	if
	(
		pid
		&&
		(
			kill(pid, SIG0) != SYSERROR
			||
			errno == EPERM
		)
	)
		return true;

	return false;
}
