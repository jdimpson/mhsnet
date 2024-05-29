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


#if	AUSAS == 1
static bool	pw_policy _FA_((int (*)(uid_t), struct pwent *, Passwd *));
#else	/* AUSAS == 1 */
static bool	pw_policy _FA_((struct passwd *, Passwd *));
#endif	/* AUSAS == 1 */



/*
**	Free up strings allocated in `pw_policy()'.
*/

void
FreeUid(pp)
	register Passwd *	pp;
{
	FreeStr(&pp->P_name);
	FreeStr(&pp->P_rname);
	FreeStr(&pp->P_dir);
	FreeStr(&pp->P_shell);
}



/*
**	Given a user name, find uid and
**	fill a structure with details from the 'passwd' file.
*/

bool
GetUid(pp, name)
	register Passwd *pp;
	char *		name;
{
#	if	AUSAS == 1
	struct pwent	pe;
	extern int	getpwuid();

	Trace3(2, "GetUid(%#lx, %s)", (PUlong)pp, name);

	pp->P_name = name;
	pe.pw_strings[LNAME] = name;

	return pw_policy(getpwuid, &pe, pp);

#	else	/* AUSAS == 1 */

	register struct passwd *pw;
	extern struct passwd *	getpwnam();

	Trace3(2, "GetUid(%#lx, %s)", (PUlong)pp, name);

	pp->P_name = name;

	errno = 0;

	if ( (pw = getpwnam(name)) == (struct passwd *)0 )
	{
		endpwent();
		pp->P_error = english("no such user");
		return false;
	}

	endpwent();

	return pw_policy(pw, pp);
#	endif	/* AUSAS == 1 */
}



/*
**	Given a uid, find user name and
**	fill a structure with details from the 'passwd' file.
*/

bool
GetUser(pp, uid)
	register Passwd *pp;
	int		uid;
{
#	if	AUSAS == 1
	struct pwent	pe;
	extern int	getpwlog();

	Trace3(2, "GetUser(%#lx, %d)", (PUlong)pp, uid);

	pp->P_uid = uid;
	pe.pw_limits.l_uid = uid;

	return pw_policy(getpwlog, &pe, pp);

#	else	/* AUSAS == 1 */

	register struct passwd *pw;
	extern struct passwd *	getpwuid _FA_((uid_t));

	Trace3(2, "GetUser(%#lx, %d)", (PUlong)pp, uid);

	pp->P_uid = uid;

	errno = 0;

	if ( (pw = getpwuid(uid)) == (struct passwd *)0 )
	{
		endpwent();
		pp->P_error = english("no such user");
		return false;
	}

	endpwent();

	return pw_policy(pw, pp);
#	endif	/* AUSAS == 1 */
}



/*
**	Convert passwd data into canonical form in "*pp".
**
**	Sets default network priviledges.
*/

#if	AUSAS == 1

static bool
pw_policy(funcp, pe, pp)
	int			(*funcp)();
	register struct pwent *	pe;
	register Passwd *	pp;
{
	register char *	cp1;
	register char *	cp2;
	register int	len;
	char		namebuf[SSIZ];

	errno = 0;	/* Clear old error */

	while ( (*funcp)(pe, namebuf, sizeof namebuf) == PWERROR )
	{
		if ( errno != 0 )
		{
			Syserror(english("Can't access passwd file"));
			errno = 0;
		}
		else
		{
			pwclose();
			pp->P_error = english("no such user");
			return false;
		}
	}

	pwclose();

	pp->P_uid = pe->pw_limits.l_uid;
	pp->P_gid = pe->pw_gid;

	pp->P_name = newstr(pe->pw_strings[LNAME]);
	(void)strncpy(pp->P_crypt, pe->pw_passwd, sizeof(pp->P_crypt));

	len = 0;
	if ( (cp1 = pe->pw_strings[FIRSTNAME]) != NULLSTR )
		len += strlen(cp1);
	else
		cp1 = EmptyString;
	if ( (cp2 = pe->pw_strings[LASTNAME]) != NULLSTR )
		len += strlen(cp2);
	else
		cp2 = EmptyString;
	pp->P_rname = Malloc(len + 2);
	cp1 = strcpyend(pp->P_rname, cp1);
	*cp1++ = ' ';
	(void)strcpyend(cp1, cp2);

	pp->P_dir = newstr(pe->pw_strings[DIRPATH]);

	pp->P_shell = (((cp1 = pe->pw_strings[SHELLPATH]) != NULLSTR && *cp1 != '\0')
			? newstr(cp1)
			: NULLSTR);

	pp->P_flags = P_EXPLICIT|P_MULTICAST;

	if ( pe->pw_xflags & USENET )
		pp->P_flags |= P_CANSEND|P_CANRECV;

#	ifdef	NOLOGIN
	if ( (pe->pw_limits.l_flags & (ASYNCKILL|NOLOGIN)) == 0 )
		pp->P_flags |= P_CANEXEC;
#	endif	/* NOLOGIN */
			
	if
	(
		pp->P_uid == 0
		||
		strcmp(pp->P_name, NETUSERNAME) == STREQUAL
		||
		(SERVERUSER != NULLSTR && strcmp(pp->P_name, SERVERUSER) == STREQUAL)
#		ifdef	NETPRIV
		||
		(pe->pw_xflags & NETPRIV)
#		endif	/* NETPRIV */
	)
		pp->P_flags |= P_SU|P_WORLD|P_BROADCAST|P_OTHERHANDLERS|P_CANSEND|P_CANRECV|P_CANEXEC;

	pp->P_region = NULLSTR;

	return true;
}

#else	/* AUSAS == 1 */

static bool
pw_policy(pw, pp)
	register struct passwd *pw;
	register Passwd *	pp;
{
	register char *		cp;

	pp->P_uid = pw->pw_uid;
	pp->P_gid = pw->pw_gid;

	pp->P_name = newstr(pw->pw_name);
	pp->P_rname = newstr(pw->pw_gecos);
	if ( (cp = strchr(pp->P_rname, ',')) != NULLSTR )
		*cp = '\0';	/* Always ignore after first comma */

	pp->P_dir = newstr(pw->pw_dir);
	pp->P_shell = newstr(pw->pw_shell);

	(void)strncpy(pp->P_crypt, pw->pw_passwd, sizeof(pp->P_crypt));

	pp->P_flags = P_CANSEND|P_CANRECV|P_CANEXEC|P_MULTICAST|P_EXPLICIT;

	if
	(
		pp->P_uid == 0
		||
		strcmp(pp->P_name, NETUSERNAME) == STREQUAL
		||
		(SERVERUSER != NULLSTR && strcmp(pp->P_name, SERVERUSER) == STREQUAL)
	)
		pp->P_flags |= P_SU|P_WORLD|P_BROADCAST|P_OTHERHANDLERS;

	pp->P_region = NULLSTR;

	return true;
}

#endif	/* AUSAS == 1 */
