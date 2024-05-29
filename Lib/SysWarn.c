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
**	System error handler.
*/

#define	RECOVER
#define	STDARGS
#define	STDIO
#define	ERRNO

#if __sun__
#define va_copy(to, from)       ((to) = (from))
#endif

#include	"global.h"
#include	"debug.h"
#include	"exec.h"
#include	"sysexits.h"



#define		MAXZERO		3
static int	ZeroErrnoCount;	/* Count incidences of errno==0 */
static char	SysErrStr[]	= english("system error");
static char	SysWrnStr[]	= english("system warning");

#define	Fprintf	(void)fprintf
#define	Fflush	(void)fflush



/*
**	Write error record to ErrorFd, and ErrorLog.
**
**	Return true if recoverable resource error, else false.
*/

static bool
sys_common(type, en, fmt, vp)
	char *		type;
	int		en;
	char *		fmt;
	va_list		vp;
{
	bool		val;
	static char	errs[]	= ": %s";

	FreeStr(&ErrString);
	if ( en != ENOMEM )
	{
		va_list	vq;
		va_copy(vq, vp);
		ErrString = newvprintf(fmt, vq);
		va_end(vq);
	}

	if ( (val = SysRetry(en)) == true && en == 0 )
		return true;

	MesgV(type, fmt, vp);

	Fprintf(ErrorFd, errs, ErrnoStr(en));
	(void)fputc('\n', ErrorFd);
	Fflush(ErrorFd);

	return val;
}



/*
**	Report system error, terminate if non-recoverable, else return.
*/

/*VARARGS*/
void
#ifdef	ANSI_C
Syserror(char *fmt, ...)
{
#else	/* ANSI_C */
Syserror(va_alist)
	va_dcl
{
	char *	fmt;
#endif	/* ANSI_C */
	va_list	vp;
	int	en;

	if ( (en = errno) == EINTR )
		return;

#	ifdef	ANSI_C
	va_start(vp, fmt);
#	else	/* ANSI_C */
	va_start(vp);
	fmt = va_arg(vp, char *);
#	endif	/* ANSI_C */

	if ( sys_common(SysErrStr, en, fmt, vp) )
	{
		va_end(vp);
		errno = en;
		return;
	}

	va_end(vp);

	if ( ErrFlag != ert_exit )
		finish(EX_OSERR);

	if ( ExInChild )
		_exit(EX_OSERR);

	exit(EX_OSERR);
}



/*
**	Test if error is retry-able, and if so, sleep, return true.
*/

bool
SysRetry(en)
	int	en;
{
	switch ( errno = en )
	{
	default:
		ZeroErrnoCount = 0;
		return false;

	case 0:
		if ( ++ZeroErrnoCount > MAXZERO )
			return false;
		(void)sleep(10);
		break;

	case EINTR:
		ZeroErrnoCount = 0;
		return true;

	case ENFILE:
	case ENOSPC:
	case EAGAIN:

#	ifdef	EALLOCFAIL
	case EALLOCFAIL:
#	endif	/* EALLOCFAIL */

#	ifdef	ENOBUFS
	case ENOBUFS:
#	endif	/* ENOBUFS */

#	ifdef	ENOLCK
	case ENOLCK:
#	endif	/* ENOLCK */

		ZeroErrnoCount = 0;
		(void)sleep(20);
		break;
	}

	errno = en;
	return true;
}



/*
**	Report system error, return true if recoverable, else false.
*/

/*VARARGS*/
bool
#ifdef	ANSI_C
SysWarn(char *fmt, ...)
{
#else	/* ANSI_C */
SysWarn(va_alist)
	va_dcl
{
	char *	fmt;
#endif	/* ANSI_C */
	va_list	vp;
	int	en;
	bool	val;

	if ( (en = errno) == EINTR )
		return true;

#	ifdef	ANSI_C
	va_start(vp, fmt);
#	else	/* ANSI_C */
	va_start(vp);
	fmt = va_arg(vp, char *);
#	endif	/* ANSI_C */

	val = sys_common(SysWrnStr, en, fmt, vp);

	va_end(vp);

	errno = en;

	return val;
}
