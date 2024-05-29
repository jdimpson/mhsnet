/*
**	Provide `varargs' version of `fprintf' and `vsprintf' missing from some systems,
**	assumes `_doprnt()' in libc.a takes a `va_list' type 2nd arg.
**
**		If not, then this won't work either.
**		If yes, then could instead "#define vfprintf(A,B,C) _doprnt(B,C,A)" in site.h
**
**	SCCSID	@(#)vfprintf.c	1.7 05/12/16
*/

#define	STDARGS
#define	STDIO

#include	"global.h"

#if	VPRINTF == 0

extern int	_doprnt _FA_((char *, va_list *, FILE *));



int
vfprintf(stream, fmt, args)
	FILE *		stream;
	char *		fmt;
	va_list *	args;
{
	return _doprnt(fmt, args, stream);
}



int
vsprintf(buf, fmt, args)
	char *		buf;
	char *		fmt;
	va_list *	args;
{
	FILE		siop;

	siop._cnt = siop._bufsiz = MAX_INT;
	siop._base = siop._ptr = (unsigned char *)buf;
	siop._flag = _IOWRT;
#	ifdef	_NFILE
	siop._file = _NFILE;
#	else	/* _NFILE */
	siop._file = 0;
#	endif	/* _NFILE */

	(void)_doprnt(fmt, args, &siop);

	*siop._ptr = '\0';
}

#else	/* VPRINTF == 0 */
void
_dummy_vfprintf(){}
#endif	/* VPRINTF == 0 */
