/*
**	Return the ptr in sp at which the character c last appears;
**	NULL if not found.
**
**	SCCSID @(#)strrchr.c	1.2 09/07/30
*/

char *
strrchr(sp, c)
	const char *	sp;
	int		c;
{
	register char *	r;

	r = (char *)0;

	do
		if ( *sp == c )
			r = (char *)sp;
	while
		( *sp++ );

	return r;
}
