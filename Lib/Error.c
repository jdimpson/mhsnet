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
**	Software error handler.
*/

#define	FILE_CONTROL
#define	RECOVER
#define	STDARGS
#define	STDIO
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"sysexits.h"




jmp_buf		ErrBuf;
ERC_t		ErrFlag;
bool		ErrIsatty;
bool		IsattyDone;

#define	Fprintf	(void)fprintf
#define	Fflush	(void)fflush



/*
**	Hard error, message on ErrorFd, and recover.
*/

/*VARARGS*/
#ifdef	ANSI_C

void
Error(char *fmt, ...)
{
	va_list	vp;

	(void)alarm(0);

	va_start(vp, fmt);
	FreeStr(&ErrString);
	ErrString = newvprintf(fmt, vp);
	va_end(vp);
	va_start(vp, fmt);
	MesgV(english("error"), fmt, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	Fflush(ErrorFd);

	errno = 0;

	switch ( ErrFlag )
	{
	case ert_here:
		longjmp(ErrBuf, EX_SOFTWARE);
	case ert_finish:
		finish(EX_SOFTWARE);
	case ert_exit:
		exit(EX_SOFTWARE);
	case ert_return:
		break;
	}
}

#else	/* ANSI_C */

void
Error(va_alist)
	va_dcl
{
	char *	fmt;
	va_list	vp;

	(void)alarm(0);

	va_start(vp);
	fmt = va_arg(vp, char *);
	FreeStr(&ErrString);
	ErrString = newvprintf(fmt, vp);
	MesgV(english("error"), fmt, vp);
	va_end(vp);

	putc('\n', ErrorFd);
	Fflush(ErrorFd);

	errno = 0;

	switch ( ErrFlag )
	{
	case ert_here:
		longjmp(ErrBuf, EX_SOFTWARE);
	case ert_finish:
		finish(EX_SOFTWARE);
	case ert_exit:
		exit(EX_SOFTWARE);
	}
}
#endif	/* ANSI_C */



/*
**	Set pointer to fd of error file, check if error file is a ``tty''.
*/

bool
ErrorTty(fdp)
	int *	fdp;
{
	if ( !IsattyDone )
	{
		ErrIsatty = (bool)isatty(fileno(ErrorFd));
		IsattyDone = true;
	}

	if ( fdp != (int *)0 )
		*fdp = fileno(ErrorFd);

	return ErrIsatty;
}



/*
**	Reset error file knowledge (eg: after fork()).
*/

void
ResetErrTty()
{
	IsattyDone = false;
}



/*
**	Prevent diversions of stderr into new files.
*/

void
StdError()
{
	ErrIsatty = true;
	IsattyDone = true;
}
