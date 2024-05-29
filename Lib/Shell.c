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
**	Run shell on passed args.
**
**	Return false if shell program fails.
**
**	Shell program is either in current directory, or in CALLDIR.
*/

#define	SIGNALS
#define	STAT_CALL
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"exec.h"
#include	"Passwd.h"
#include	"spool.h"


#if	BSD4 >= 2
#include	<sys/time.h>
#include	<sys/resource.h>
#endif	/* BSD4 >= 2 */


void	ShellSetChild _FA_((VarArgs *));



bool
Shell(cp, link, reason, ofd)
	char *		cp;
	char *		link;
	char *		reason;
	int		ofd;
{
	char *		cp1;
	char *		cp2;
	char *		errs;
	bool		retval;
	ExBuf		va;
	struct stat	statb;

	Trace5(1, "Shell(%s, %s, %s, %d)", (cp==NULLSTR)?NullStr:cp, link, reason, ofd);

	if ( cp == NULLSTR || *cp == '\0' )
		return true;

	if ( strchr(cp, '/') == NULLSTR )
	{
		cp1 = concat(".", Slash, cp, NULLSTR);
		cp2 = concat(SPOOLDIR, CALLDIR, cp, NULLSTR);
	}
	else
	{
		cp1 = newstr(cp);
		cp2 = NULLSTR;
	}

	while ( stat(cp1, &statb) == SYSERROR )
		if ( errno == ENOENT || !SysWarn(CouldNot, StatStr, cp1) )
		{
			free(cp1);

			if ( (cp1 = cp2) == NULLSTR )
				return true;
			
			cp2 = NULLSTR;
		}

	FreeStr(&cp2);

	if ( !(statb.st_mode & 0110) )
	{
		free(cp1);
		return true;	/* No execute permission */
	}

	if ( statb.st_uid != NetUid || statb.st_gid != NetGid || (statb.st_mode & 02) )
	{
		Warn(english("no permission to run \"%s\""), cp1);
		free(cp1);
		return true;
	}

	FIRSTARG(&va.ex_cmd) = SHELL;
	NEXTARG(&va.ex_cmd) = "-c";
	cp2 = concat(cp1, " ", link, " \"", reason, "\" ", Name, NULLSTR);
	NEXTARG(&va.ex_cmd) = cp2;

	if ( (errs = Execute(&va, ShellSetChild, ex_exec, ofd)) != NULLSTR )
	{
		if ( (cp = errs)[0] == '\0' )
			cp = english("shell error");
		Warn(StringFmt, cp);
		free(errs);
		retval = false;
	}
	else
		retval = true;

	free(cp2);
	free(cp1);

	return retval;
}



/*
**	Ignore SIGHUP, return to nice(0),
**	set uid/gid, and make fd0 == fd1.
*/

void
ShellSetChild(args)
	VarArgs *	args;
{
	int		uid;
	int		gid;

	(void)signal(SIGHUP, SIG_IGN);

	(void)close(0);
	(void)dup(1);	/* == ofd */

#	if	BSD4 >= 2
	(void)setpriority(PRIO_PROCESS, 0, 0);
#	else	/* BSD4 >= 2 */
#	if	SYSTEM > 0
#	ifndef	QNX
	(void)nice(-nice(0));
#	endif	/* QNX */
#	else	/* SYSTEM > 0 */
	if ( geteuid() == 0 )
	{
		(void)nice(-40);
		(void)nice(20);
	}
#	endif	/* SYSTEM > 0 */
#	endif	/* BSD4 >= 2 */

	gid = getgid();

	if ( (uid = getuid()) == 0 )
	{
		gid = NetGid;
		uid = NetUid;
	}

	SetUser(uid, gid);
}
