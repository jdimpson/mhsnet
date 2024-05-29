/*
**	Return ptr to first occurance of any character from `brkset'
**	in the character string `string'; NULLSTR if none exists.
**
**	SCCSID @(#)strpbrk.c	1.2 09/07/30
*/

#define	NULLSTR	(char *)0

char *
strpbrk(string, brkset)
	const char *	string;
	const char *	brkset;
{
	const char *	p;

	if ( string == NULLSTR || brkset == NULLSTR || *brkset == '\0' )
		return NULLSTR;

	while ( *string )
	{
		for ( p = brkset ; *p != '\0' && *p != *string ; ++p )
			;

		if ( *p != '\0' )
			return (char *)string;

		string++;
	}

	return NULLSTR;
}
