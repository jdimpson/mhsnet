/*
**	Return string representing <errno>.
**
**	SCCSID	@(#)strerror.c	1.4 05/12/16
*/

#ifdef	NO_STRERROR
#include	"global.h"


char *
strerror(en)
	int		en;
{
	static char	errs[6+10+1];
	extern char *	sys_errlist[];
	extern int	sys_nerr;

	if ( en < sys_nerr && en > 0 )
		return sys_errlist[en];

	(void)sprintf(errs, english("Error %d"), en);
	return errs;
}
#else	/* NO_STRERROR */
void _dummy_strerror(){}
#endif	/* NO_STRERROR */
