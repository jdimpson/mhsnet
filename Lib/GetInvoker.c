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
#define	NEED_HZ		/* To pull in <sys/param.h> */
#define	PASSWD_USED
#define	SIGNALS

#include	"global.h"
#include	"debug.h"
#include	"Passwd.h"

#if	BSD4 >= 2
#include	<sys/time.h>
#include	<sys/resource.h>
#endif	/* BSD4 >= 2 */

#ifdef	PWHIGHUID
#undef	NGROUPS	/* AUSAM -- wrong sort! */
#endif	/* PWHIGHUID */



/*
**	Get uid and fill a structure with details from the 'passwd' file.
**
**	Also, observe all safety requirements.
*/

void
GetInvoker(pp)
	register Passwd *pp;
{
	register int	uid;
	register char *	cp;
#	ifdef	NGROUPS
	register int	ngroups;
	gid_t		groups[NGROUPS+1];
#	endif	/* NGROUPS */

	Trace2(1, "GetInvoker(%#lx)", (PUlong)pp);

	GetNetUid();

	if
	(
		(
			(uid = getuid()) != 0
			||
			(
				((cp = getenv("LNAME")) == NULLSTR || cp[0] == '\0' )
				&&
				((cp = getenv("LOGNAME")) == NULLSTR || cp[0] == '\0' )
				&&
				((cp = getenv("USER")) == NULLSTR || cp[0] == '\0' )
			)
			||
			!GetUid(pp, cp)
		)
		&&
		!GetUser(pp, uid)
	)
		Error(pp->P_error);

	GetPrivs(pp);

	if ( uid == NetUid )
	{
		pp->P_flags = P_ALL;	/* Prevent any ``privsfile'' silliness */
		pp->P_region = NULLSTR;
	}
	else
	if ( !(pp->P_flags & P_SU) )
	{
		if ( uid == 0 || pp->P_gid == NetGid )
			pp->P_flags |= P_SU|P_BROADCAST|P_OTHERHANDLERS|P_CANSEND|P_CANEXEC;
#		ifdef	NGROUPS
		else
		if ( NetGid > 0 && (ngroups = getgroups(NGROUPS+1, groups)) > 0 )
			while ( --ngroups >= 0 )
				if ( groups[ngroups] == NetGid )
				{
					pp->P_flags |= P_SU|P_BROADCAST|P_OTHERHANDLERS|P_CANSEND|P_CANEXEC;
					break;
				}
#		endif	/* NGROUPS */
	}

	Trace3(1, "GetInvoker ==> %s%s", pp->P_name, (pp->P_flags & P_SU)?" [SU]":EmptyString);

	/*
	**	Ignore stopping signals:
	*/

#	ifdef	SIGTTOU
	(void)signal(SIGTTOU, SIG_IGN);
#	endif
#	ifdef	SIGTTIN
	(void)signal(SIGTTIN, SIG_IGN);
#	endif
#	ifdef	SIGTSTP
	(void)signal(SIGTSTP, SIG_IGN);
#	endif

	/*
	**	Don't let them nice us off:
	*/

#	if	BSD4 >= 2
	(void)setpriority(PRIO_PROCESS, 0, 0);
#	else	/* BSD4 >= 2 */
#	if	SYSTEM > 0
#	ifndef	QNX
	(void)nice(0-nice(0));
#	endif	/* QNX */
#	else	/* SYSTEM > 0 */
	if ( geteuid() == 0 )	/* Otherwise doesn't matter! */
	{
		(void)nice(-40);
		(void)nice(20);
	}
#	endif	/* SYSTEM > 0 */
#	endif	/* BSD4 >= 2 */
}
