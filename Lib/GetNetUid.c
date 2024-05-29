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
#include	"Passwd.h"

#if	AUSAS == 0
#include	<grp.h>

extern struct group *	getgrnam _FA_((const char *));
#endif	/* AUSAS == 0 */


Passwd		NetPasswd;
static bool	NetUidSet;



/*
**	Setup network user passwd details.
*/

void
GetNetUid()
{
	Trace3(1, "GetNetUid(), euid=%d, egid=%d", geteuid(), getegid());

	if ( NetUidSet )
		return;

	if ( !GetUid(&NetPasswd, NETUSERNAME) )
		Error(english("Network id \"%s\" -- %s."), NETUSERNAME, NetPasswd.P_error);

#	if	AUSAS == 0
	if ( NETGROUPNAME != NULLSTR )
	{
		register struct group * gp;

		if ( (gp = getgrnam(NETGROUPNAME)) == (struct group *)0 )
			Error(english("Network group id \"%s\" does not exist!"), NETGROUPNAME);

		endgrent();

		NetGid = gp->gr_gid;
	}
#	endif	/* AUSAS == 0 */

	NetUidSet = true;

	Trace3(1, "GetNetUid() ==> netuid=%d, netgid=%d", NetUid, NetGid);
}
