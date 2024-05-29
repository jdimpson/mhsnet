/*
**	Return the number of characters in the maximum leading segment
**	of string which consists solely of characters from charset.
**
**	SCCSID @(#)strspn.c	1.5 05/12/16
*/

#ifndef	STRING_H
int
strspn(string, charset)
	const char *	string;
	const char *	charset;
{
	char *	p;
	char *	q;

	for ( q = string ; *q != '\0' ; ++q )
	{
		for ( p = charset ; *p != '\0' && *p != *q ; ++p )
			;
		if ( *p == '\0' )
			break;
	}

	return q - string;
}
#else	/* STRING_H */
void _dummy_strspn(){}
#endif	/* STRING_H */
