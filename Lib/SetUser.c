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


#define	PASSWD_USED

#include	"global.h"
#include	"debug.h"



/*
**	Routine to cause invoker to become a different user.
*/

void
SetUser(uid, gid)
	int		uid;
	int		gid;
{
	static char	CouldNotD[]	= english("Could not set%s [%d]");
#	if	AUSAS == 1
#	if	LIMITS == 1
	struct pwent	pe;

	if ( geteuid() != 0 )
		return;

	pe.pw_uid = uid;

	errno = 0;	/* Clear old error */

	while ( getpwlog(&pe, NULLSTR, 0) == PWERROR )
	{
		if ( errno != 0 )
		{
			Syserror("Can't access passwd file");
			errno = 0;
		}
		else
		{
			pwclose();
			Error("no such user");
			return;
		}
	}

	pwclose();

#	if	V8 == 1
	if ( setlimits(&pe.pw_limits) == SYSERROR )
		(void)SysWarn("setlimits");
#	else	/* V8 == 1 */
	if ( limits(&pe.pw_limits, L_SETLIM) == SYSERROR )
		(void)SysWarn("limits");
#	endif	/* V8 == 1 */

#	endif	/* LIMITS == 1 */
#	endif	/* AUSAS == 1 */

	Trace3(2, "SetUser(%d, %d)", uid, gid);

#	if	SYSTEM == 5 && SYSVREL <= 3
	if ( getuid() == 0 && geteuid() != 0 )
		(void)setuid(0);
#	endif	/* SYSTEM == 5 && SYSVREL <= 3 */

	while ( setgid(gid) == SYSERROR && geteuid() == 0 )
		if ( !SysWarn(CouldNotD, "gid", gid) )
			break;

	while ( setuid(uid) == SYSERROR && geteuid() == 0 )
		Syserror(CouldNotD, "uid", uid);
}
