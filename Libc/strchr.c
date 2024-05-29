/*
**	Return the ptr in sp at which the character c appears;
**	NULL if not found
**
**	SCCSID @(#)strchr.c	1.2 09/07/30
*/

char *
strchr(sp, c)
	const char *	sp;
	int		c;
{
	do
		if ( *sp == c )
			return (char *)sp;
	while
		( *sp++ );

	return (char *)0;
}
