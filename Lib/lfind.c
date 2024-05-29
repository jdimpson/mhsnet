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
**	Linear search algorithm, generalized from Knuth (6.1) Algorithm S.
*/

char *
lfind(key, base, nelp, width, compar)
	char *		key;		/* Key to be located */
	register char *	base;		/* Beginning of table */
	int *		nelp;		/* Pointer (NB) to number of elements */
	register int	width;		/* Width of an element */
#	ifndef	ANSI_C
	int		(*compar)();	/* Comparison function */
#	else	/* !ANSI_C */
	int		(*compar)(const char *, const char *);
#	endif	/* !ANSI_C */
{
	register char *	u;		/* Last element in table */
	int		nel = *nelp;

	if ( --nel < 0 )
		return (char *)0;

	u = base + nel*width;

	while ( u >= base )
	{
		if ( (*compar)(key, base) == 0 )
			return base;

		base += width;
	}

	return (char *)0;
}
