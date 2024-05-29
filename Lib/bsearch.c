

#ifndef __APPLE__

/*
**	Binary search algorithm, generalized from Knuth (6.2.1) Algorithm B.
*/

char *
bsearch(key, base, nel, width, compar)
	char *		key;		/* Key to be located */
	register char *	base;		/* Beginning of table */
	register int	nel;		/* Number of elements */
	register int	width;		/* Width of an element */
#	ifndef	ANSI_C
	int		(*compar)();	/* Comparison function */
#	else	/* !ANSI_C */
	int		(*compar)(const char *, const char *);
#	endif	/* !ANSI_C */
{
	register char *	u;		/* Last element in table */
	register char *	i;		/* Approximate middle element */
	register int	res;		/* Result of comparison */        

	if ( --nel < 0 )
		return (char *)0;

	u = base + nel*width;

	while ( u >= base )
	{
		nel /= 2;
		i = base + width*nel;

		if ( (res = (*compar)(key, i)) == 0 )
			return i;
		else
		if ( res < 0 )
			u = i - width;
		else
			base = i + width;

		nel = (u-base) / width;
	}

	return (char *)0;
}

#else	/* __APPLE__ */
dummy_bsearch(){}
#endif	/* __APPLE__ */
