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
**	Optional pipe(), fork(), execve() sequence with stdout selected.
*/

#define	FILE_CONTROL
#define	RECOVER
#define	SIGNALS
#define	STDIO
#define	ERRNO
#define	WAIT_CALL

#include	"global.h"
#include	"debug.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"
#include	"sysexits.h"


#define	TOCHILD		1
#define	CHILD0		0


static void	Exit _FA_((ExBuf *, char *, int));


char *
Execute(buf, setup, type, ofd)
	register ExBuf *buf;
#	ifdef	ANSI_C
	Ex_fp		setup;
#	else	/* ANSI_C */
	void		(*setup)();
#	endif	/* ANSI_C */
	ExType		type;
	int		ofd;
{
	register int	i;
	register char *	prog;
	int		p[2];

	prog = ARG(&buf->ex_cmd, 0);	/* Allow "setup" to change invoked name */

	Trace5(1,
		"Execute(%s, %#lx, %s, %d)",
		prog,
		(PUlong)setup,
		(type==ex_exec)?"exec"
			:(type==ex_nowait)?"nowait"
			:(type==ex_nofork)?"nofork"
			:(type==ex_pipe)?"pipe"
			:"pipo",
		ofd
	);

	DODEBUG(if(NARGS(&buf->ex_cmd)>MAXVARARGS)Fatal("MAXVARARGS"));

	ARG(&buf->ex_cmd, NARGS(&buf->ex_cmd)) = NULLSTR;

	switch ( type )
	{
	case ex_nofork:
		buf->ex_efd = fileno(ErrorFd);
		type = ex_exec;
		goto skip_fork;

	case ex_pipe: case ex_pipo:
		while ( pipe(p) == SYSERROR )
			Syserror(CouldNot, PipeStr, prog);

	default:
		break;
	}

	MakeErrFile(&buf->ex_efd);

	for ( ;; )
	{
		switch ( buf->ex_pid = fork() )
		{
		case SYSERROR:
			if ( errno == ENOMEM )
			{
				SysWarn(CouldNot, ForkStr, prog);
				(void)sleep(20);
			}
			else
				Syserror(CouldNot, ForkStr, prog);
			continue;

		case 0:
			ExInChild = true;
			Recover(ert_exit);
skip_fork:
			DODEBUG(Name = prog; if(Traceflag&&!ErrorTty((int*)0))EchoArgs(NARGS(&buf->ex_cmd)-1, &ARG(&buf->ex_cmd, 1)));

			while ( buf->ex_efd < 2 )
				if ( (buf->ex_efd = dup(buf->ex_efd)) == SYSERROR )
					Exit(buf, Dup, 2);

			if ( ofd != SYSERROR && ofd < 1 )
				if ( (ofd = dup(ofd)) == SYSERROR )
					Exit(buf, Dup, 1);

			if
			(
				(type == ex_exec || type == ex_nowait)
				&&
				(p[CHILD0] = open(DevNull, O_READ)) == SYSERROR
			)
				Exit(buf, DevNull, 0);

			if ( p[CHILD0] != 0 )
			{
				(void)close(0);
				if ( dup(p[CHILD0]) != 0 )
					Exit(buf, Dup, 0);
			}

			if ( ofd != SYSERROR )
			{
				if ( ofd != 1 )
				{
					(void)close(1);
					if ( dup(ofd) != 1 )
						Exit(buf, Dup, 1);
				}
			}
			else
			{
				(void)close(1);
				if ( dup(buf->ex_efd) != 1 )
					Exit(buf, Dup, 1);
			}

			if ( buf->ex_efd != 2 )
			{
				(void)close(2);
				if ( dup(buf->ex_efd) != 2 )
					Exit(buf, Dup, 2);
			}

			for ( i = 3 ; close(i) != SYSERROR || i < 9 ; i++ );

			if ( setup != NULLVFUNCP )
				(*setup)(&buf->ex_cmd);

			for ( ;; )
			{
				(void)execve(prog, &ARG(&buf->ex_cmd, 0), StripEnv());
				if ( errno == ENOEXEC )
					ExShell(prog, &buf->ex_cmd);
				Syserror(CouldNot, "execve", prog);
			}
		}

		if ( type == ex_exec )
			return ExClose(buf, (FILE *)0);

		if ( type == ex_nowait )
			return NULLSTR;

		(void)close(p[CHILD0]);

		buf->ex_sig = (vFuncp)signal(SIGPIPE, SIG_IGN);

		return (char *)fdopen(p[TOCHILD], "w");
	}
}



char *
ExClose(buf, fd)
	register ExBuf *buf;
	FILE *		fd;
{
	register int	i;
	int		status;
#	if	DEBUG >= 1
	char *		errfile;
#	endif	/* DEBUG >= 1 */

	Trace3(2, "ExClose(%s, %d)", ARG(&buf->ex_cmd, 0), (fd==0)?-1:fileno(fd));

	if ( fd != (FILE *)0 )
	{
		(void)fclose(fd);
		(void)signal(SIGPIPE, buf->ex_sig);
	}

	while ( (i = wait(&status)) != buf->ex_pid )
	{
		if ( i == SYSERROR )
		{
			Syserror("Lost child");
			return NULLSTR;
		}

		if ( TakeUnkChild != (void (*)())0 )
			(*TakeUnkChild)(i, status);
	}

	ExStatus = ((status >> 8) & 0xff) | ((status & 0xff) << 8);

	if ( ErrorTty((int *)0) && (status & 0x80) == 0 )
		return status?newstr(NULLSTR):NULLSTR;

	if ( status )
		return GetErrFile(&buf->ex_cmd, status, buf->ex_efd);

#	if	DEBUG >= 1
	if
	(
		Traceflag > 0
		&&
		(errfile = GetErrFile(&buf->ex_cmd, 0, buf->ex_efd)) != NULLSTR
	)
	{
		Trace(1, ExpandString(errfile, -1));
		free(errfile);
	}
	else
#	endif	/* DEBUG >= 1 */

	if ( buf->ex_efd != fileno(ErrorFd) )
		(void)close(buf->ex_efd);

	return NULLSTR;
}



static void
Exit(buf, reason, fd)
	ExBuf *	buf;
	char *	reason;
	int	fd;
{
	char	errs[256];

	(void)sprintf
	(
		errs,
		"%s: system error setting up FD %d: \"%s\" - %s\n",
		ARG(&buf->ex_cmd, 0),
		fd,
		reason,
		ErrnoStr(errno)
	);

	(void)write(buf->ex_efd, errs, strlen(errs));	/* `stderr' not yet set up */

	_exit(fd + EX_FDERR);
}



void
ExShell(prog, vap)
	char *		prog;
	VarArgs *	vap;
{
	register char **cpp;
	register char **npp;
	char *		newargs[MAXVARARGS+2];

	npp = newargs;
	cpp = &ARG(vap, 1);

	*npp++ = SHELL;
	*npp++ = prog;	/* Might have been changed by setup() */

	while ( (*npp++ = *cpp++) != NULLSTR );

	(void)execve(SHELL, newargs, StripEnv());
}
