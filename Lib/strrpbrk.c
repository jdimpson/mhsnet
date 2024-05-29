
/*
**	Return ptr to last occurance of any character from `brkset'
**	in the character string `string'; NULLSTR if none exists.
**
**	SCCSID @(#)strrpbrk.c	1.2 88/09/10
*/

#define	NULLSTR	(char *)0

char *
strrpbrk(string, brkset)
	register char *	string;
	register char *	brkset;
{
	register char *	p;
	register char *	l;
	register char	c;
	register char	m;

	if ( string == NULLSTR || brkset == NULLSTR || *brkset == '\0' )
		return NULLSTR;

	l = NULLSTR;

	while ( (m = *string++) != '\0' )
	{
		for ( p = brkset ; (c = *p++) != '\0' ; )
			if ( c == m )
				break;

		if ( c != '\0' )
			l = string-1;
	}

	return l;
}
