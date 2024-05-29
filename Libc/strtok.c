/*
**	uses strpbrk and strspn to break string into tokens on
**	sequentially subsequent calls. Returns NULLSTR at end.
**	`subsequent' calls are calls with first argument NULL.
**
**	SCCSID	@(#)strtok.c	1.2 96/01/10
*/

#include	"global.h"


char *
strtok(string, sepset)
	char *		string;
#	if	ANSI_C
	const char *	sepset;
#	else	/* ANSI_C */
	char *		sepset;
#	endif	/* ANSI_C */
{
	register char *	p;
	register char *	q;

	static char *	savept;

	if ( (q = string) == NULLSTR && (q = savept) == NULLSTR )
		return NULLSTR;

	q += strspn(q, sepset);

	if ( *q == '\0' )
	{
		savept = NULLSTR;
		return NULLSTR;
	}

	if ( (p = strpbrk(q, sepset)) == NULLSTR )
		savept = NULLSTR;
	else
	{
		*p++ = '\0';
		savept = p;
	}

	return q;
}
