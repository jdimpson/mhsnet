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


static char	sccsid[]	= "@(#)Netinit.c	1.26 05/12/16";

/*
**	Run programs, restarting them as necessary, until SIGTERM.
*/

#define	RECOVER
#define	SIGNALS
#define	STDIO
#define	ERRNO

#include "global.h"
#include "Args.h"
#include "debug.h"
#include "params.h"
#include "Passwd.h"
#include "spool.h"
#include "sysexits.h"

#include "Netinit.h"
#include "net.h"



/*
**	Variables set from program arguments
*/

char *	InitFile;				/* Name of init file */
char *	LogName;				/* Log file */
char *	Name		= sccsid;		/* Program name */
bool	NoFork;					/* Don't fork off parent */
int	Traceflag;				/* Debug trace level */

AFuncv	getStr _FA_((PassVal, Pointer));	/* Save copy of string */

/*
**	Must save strings because using "NameProg()"
*/

Args	Usage[] =
{
        Arg_0(0, getName),
	Arg_bool(f, &NoFork, 0),
	Arg_string(L, &LogName, getStr, english("log_file_name"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("trace_level"), OPTARG|OPTVAL),
	Arg_noflag(&InitFile, getStr, english("initfile"), OPTARG),
	Arg_end
};

/*
**	Miscellaneous.
*/

int	JmpSet;
char *	LockName;
bool	LockSet;
int	netsignal;
int	NveTimeDelay	= NVETIMEDELAY;
int	ProgsActive;
jmp_buf	SetJmp;

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"NVETIMECHANGE",	(char **)&NVETIMECHANGE,	PINT},
	{"NVETIMEDELAY",	(char **)&NveTimeDelay,		PINT},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

DODEBUG(extern int malloc_debug(int));



int
main(argc, argv, envp)
	int	argc;
	char *	argv[];
	char *	envp[];
{
	DODEBUG((void)malloc_debug(MALLOC_DEBUG));

	SetNameProg(argc, argv, envp);

	InitParams();

	DoArgs(argc, argv, Usage);

	Name = newstr(Name);	/* Save copy */

	GetParams(Name, Params, sizeof Params);

	Pid = Detach(NoFork, 0, true, true);

	if ( geteuid() != NetUid )
		Error(english("No permission."));

	if ( getegid() != NetGid && NETGROUPNAME != NULLSTR )
		Error(english("Effective gid != %s."), NETGROUPNAME);

	while ( chdir(SPOOLDIR) == SYSERROR )
		Syserror(CouldNot, "chdir", SPOOLDIR);

	LockName = concat(LIBDIR, LOCKFILE, NULLSTR);

	if ( !SetLock(LockName, Pid, false) )
	{
		Warn(english("Netinit (%d) already running on %s."), LockPid, LockNode);
		finish(EX_UNAVAILABLE);
	}

	LockSet = true;

	if ( LogName == NULLSTR )
		LogName = concat(LIBDIR, LOGFILE, NULLSTR);

	StderrLog(LogName);
	(void)fclose(stdin);
	(void)fclose(stdout);

	StartTime = Time;
	MesgN(english("STARTED"), "Vn=\"%s\" [%d]", Version, Pid);

	initargs = concat(LIBDIR, NETINITARGSFILE, NULLSTR);

	if ( InitFile == NULLSTR )
		InitFile = concat(LIBDIR, INITFILE, NULLSTR);

	parse();

	if ( Procs == NULL )
	{
		Error(english("initfile \"%s\" is empty."), InitFile);
		finish(EX_USAGE);
	}

	(void)signal(SIGTERM, termin);
	(void)signal(SIGWAKEUP, NetSig);

	RunProgs();

	finish(EX_OK);
	return 0;
}



void
finish(error)
	int	error;
{
	if ( !ExInChild && LockSet )
	{
		netshutdown();

		(void)unlink(LockName);

		MesgN(error?english("ERROR"):english("FINISHED"), "Vn=\"%s\" [%d]", Version, Pid);

		DODEBUG(CpuStats(stderr, Time-StartTime));

		if ( !error && NoFork && getppid() == 1 && !setjmp(SetJmp) )
		{
			JmpSet = 1;
			(void)signal(SIGTERM, NetSig);
			for ( ;; )
				pause();
		}
	}

	if ( error )
		(void)sleep(10);	/* Depress bad invokation loops */

	exit(error);
}



/*
**	Copy new string to target.
*/

/*ARGSUSED*/
AFuncv
getStr(val, arg)
	PassVal		val;
	Pointer		arg;
{
	*(char **)arg = newstr(val.p);
	return ACCEPTARG;
}



void
NetSig(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	netsignal++;
	if ( JmpSet )
		longjmp(SetJmp, 1);
}



void
termin(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);

	finish(EX_OK);
}
