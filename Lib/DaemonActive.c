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
#include	"debug.h"
#include	"spool.h"



/*
**	Return true if daemon for given link is active (and wake it up).
*/

bool
DaemonActive(dir, wake)
	char *		dir;
	bool		wake;
{
	register bool	val;
	char *		lockfile;

#	ifdef	mips
	SMdate(dir, Time);	/* MIPS directory mtime bug fix. */
#	endif	/* mips */

	Trace3(1, "DaemonActive(%s, %s)", dir, wake?"wakeup":"nowake");

	lockfile = concat(dir, LOCKFILE, NULLSTR);

	val = (SetLock(lockfile, 0, wake) ? false : true);

	free(lockfile);

	Trace2(1, "DaemonActive => %s", val ? "true" : "false");

	return val;
}
