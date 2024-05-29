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
**	Definitions for using:
**		(char *)Execute(ExBuf *, vFuncp, ExType, int)
**	and:
**		(char *)ExClose(ExBuf *, FILE *)
**
**	Passed function is called from within child with one arg:-
**		a pointer to execve() argument vector.
*/

typedef struct
{
	VarArgs		ex_cmd;		/* Commands for exec() */
	SigFunc *	ex_sig;		/* Old signal value */
	int		ex_efd;		/* Error file descriptor */
	int		ex_pid;		/* Pid of command */
}
		ExBuf;

typedef enum
{
	ex_exec,			/* Don't make pipe, and "wait()" */
	ex_pipe,			/* Make pipe, and return write end */
	ex_pipo,			/* Make pipe, output on `ofd', and return write end */
	ex_nofork,			/* Don't fork, just exec */
	ex_nowait			/* Don't make pipe, and don't "wait()" */
}
		ExType;

Extern bool	ExInChild;		/* True after a fork for child */

#ifdef	ANSI_C
typedef void	(*Ex_fp)(VarArgs *);

Extern void	(*TakeUnkChild)(int, int);

extern char *	Execute(ExBuf *, Ex_fp, ExType, int);
extern char *	ExClose(ExBuf *, FILE *);
extern void	ExShell(char *, VarArgs *);
#else	/* ANSI_C */
#define	Ex_fp	vFuncp

Extern vFuncp	TakeUnkChild;		/* Optional function called from ExClose() */

extern char *	Execute();		/* Returns (FILE *) for ex_pip[eo] */
extern char *	ExClose();		/* Does "wait()" after ex_pip[eo] */
extern void	ExShell();		/* Passes arg vector to SHELL */
#endif	/* ANSI_C */
