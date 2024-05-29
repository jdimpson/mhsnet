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


static char	sccsid[]	= "@(#)timeout.c	1.17 05/12/16";

/*
**	timeout -- run command with timeout.
*/

#define	RECOVER
#define	SIGNALS
#define	STDIO
#define	TIME
#define	ERRNO
#define	WAIT_CALL

#include	"global.h"
#include	"debug.h"
#include	"Args.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters.
*/

/*
**	Parameters set from arguments.
*/

char *	Name		= sccsid;	/* Program invoked name */
int	Timeout;			/* Kill prog after this */
int	Traceflag;
bool	Warnings;			/* Show what timedout */

AFuncv	getArgs _FA_((PassVal, Pointer, char *));

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(w, &Warnings, 0),
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_macro(NONFLAG, &Timeout, getInt1, "timeout", INT, 0),
	Arg_noflag(0, getArgs, "cmd", 0),
	Arg_any(0, getArgs, "arg", OPTARG|MANY),
	Arg_end
};

/*
**	Miscellaneous
*/

char *	CmdArgs;			/* Concatenated command+args */
bool	JmpSet;
jmp_buf	SetJmp;

extern SigFunc	catchalarm;

char *	quote _FA_((char *));
int	run _FA_((void));



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	InitParams();

	DoArgs(argc, argv, Usage);

	exit(run());
	return 0;
}



/*
**	ALARM calls.
*/

void
catchalarm(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	if ( JmpSet )
		longjmp(SetJmp, 1);
}



void
finish(error)
	int	error;
{
	exit(error);
}



/*ARGSUSED*/
AFuncv
getArgs(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	ArgsIgnFlag = true;	/* No more flags */

	if ( CmdArgs == NULLSTR )
		CmdArgs = "exec";

	CmdArgs = concat(CmdArgs, " ", val.p, NULLSTR);

	return ACCEPTARG;
}



char *
quote(cmd)
	register char *	cmd;
{
	register int	c;
	register char *	np;
	char *		str;

	if ( strchr(cmd, '\'') == NULLSTR )
		return cmd;

	str = np = Malloc(strlen(cmd)*4+1);

	while ( (c = *cmd++) != '\0' )
	{
		/* Turn "'" into "'\''" */
		if ( c == '\'' )
		{
			*np++ = '\'';
			*np++ = '\\';
			*np++ = '\'';
		}
		*np++ = c;
	}

	*np++ = '\0';
	return str;
}



int
run()
{
	int		pid;
	int		child;
	int		status;
	int		retval;
	char *		args[4];
	static int	jumped;
	static char	rep_fmt[]	= "%s -c '%s'";
	extern char **	environ;

	if ( strpbrk(CmdArgs, ";\n") != NULLSTR )
		CmdArgs += 5;	/* Skip "exec " */

	switch ( pid = fork() )
	{
	case SYSERROR:
		Syserror(CouldNot, ForkStr, CmdArgs);
		return EX_TEMPFAIL;

	case 0:
#		if	PGRP == 1
#		if	BSD4 >= 2
#		if	BSDI >= 2 || __NetBSD__ == 1 || __APPLE__ == 1
		if ( (pid = getpid()) != getpgrp() )
#		else	/* BSDI >= 2 */
		if ( (pid = getpid()) != getpgrp(0) )
#		endif	/* BSDI >= 2 */
			setpgrp(0, pid);
#		else	/* BSD4 >= 2 */
		if ( (pid = getpid()) != getpgrp() )
#		if	BSD4PGRP == 1
			setpgrp(0, pid);
#		else	/* BSD4PGRP == 1 */
			setpgrp();
#		endif	/* BSD4PGRP == 1 */
#		endif	/* BSD4 >= 2 */
#		endif	/* PGRP == 1 */
		args[0] = SHELL;
		args[1] = "-c";
		args[2] = CmdArgs;
		args[3] = NULLSTR;
		(void)execve(args[0], args, environ);
		Syserror(CouldNot, "execve", args[0]);
		break;

	default:
		Trace3(1, rep_fmt, SHELL, quote(CmdArgs));
	}

	jumped = 0;

	if ( setjmp(SetJmp) )
	{
#		if	PGRP == 1
		(void)kill(-pid, jumped?SIGKILL:SIGTERM);
#		else	/* PGRP == 1 */
		(void)kill(pid, jumped?SIGKILL:SIGTERM);
#		endif	/* PGRP == 1 */
		jumped++;
		Timeout = 2;
	}

	if ( jumped < 2 )
	{
		JmpSet = true;

		(void)signal(SIGTERM, catchalarm);
		(void)signal(SIGALRM, catchalarm);
		(void)alarm(Timeout);

		while ( (child = wait(&status)) != SYSERROR && child != pid );

		retval = status >> 8;
	}

	JmpSet = false;

	(void)alarm((unsigned)0);

	if ( jumped )
	{
		retval = EX_NOINPUT;

		if ( Warnings )
			MesgN(english("timed out"), rep_fmt, SHELL, quote(CmdArgs));
	}

	return retval;
}
