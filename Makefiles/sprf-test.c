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
**	Find system library peculiarities.
*/

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>

#ifndef	Rand
#define	Rand	rand
#define	SRand	srand
#endif

#ifndef	NEED_BCOPY
int	need_bcopy	= 0;
#endif	/* NEED_BCOPY */
int	printf_nulls	= 0;
int	sprf_size	= 0;

char *	format		= "%s%s\t%d\n";
char *	define		= "#define\t";

#	ifndef	NEED_BCOPY
void	bcopy_test();
#	endif	/* NEED_BCOPY */

int
main()
{
	register char *	p;
	char		s[8];
	unsigned long	ul;

	for ( p = s ; p < &s[sizeof s] ; )
		*p++ = '\377';

	if ( (int)sprintf(s, "01%c34", '\0') == 5 )
		sprf_size = 1;

	for ( p = s ; p < &s[sizeof s] ; )
		*p++ = '\377';

	ul = sprintf(s, "01%c34", '\0');
	if ( (char *)ul == s )
		sprf_size = 0;

	if ( s[2] == '\0' && s[3] == '3' )
		printf_nulls = 1;

#	ifndef	NEED_BCOPY
	bcopy_test();

	(void)printf(format, define, "NEED_BCOPY", need_bcopy);
#	endif	/* NEED_BCOPY */
	(void)printf(format, define, "PRINTF_NULLS", printf_nulls);
	(void)printf(format, define, "SPRF_SIZE", sprf_size);

	exit(0);
	return 0;
}

#ifndef	NEED_BCOPY

#define	LONGZ	sizeof(long)
#define	LONGZ2	(2*LONGZ)
#define	LONGZ4	(4*LONGZ)

void
bcopy_test()
{
	register int	i;
	register int	j;
	register int	k;
	register int	l;

	char	orig[128+LONGZ4];
	char	from[128+LONGZ4];
	char	to[128+LONGZ4];

	SRand(357);

	for ( i = 0 ; i < 128 ; i++ )
		for ( j = 0 ; j < LONGZ ; j++ )
			for ( k = 0 ; k < LONGZ ; k++ )
			{
				for ( l = i+LONGZ2 ; --l >= 0 ; )
				{
					orig[l] = (Rand()>>12)|040;
					to[l] = 0;
				}
				for ( l = LONGZ2 ; --l >= 0 ; )
				{
					from[i+LONGZ2+l] = 0;
					to[i+LONGZ2+l] = 0;
				}
				(void)strncpy(from, orig, i+LONGZ2);
				bcopy(from+j+k, from+j, i);
				if ( strncmp(orig+j+k, from+j, i) )
				{
					need_bcopy = 1;
					return;
				}
				(void)strncpy(from, orig, i+LONGZ2);
				bcopy(from+j, to+k, i);
				if ( strncmp(orig+j, to+k, i) )
				{
					need_bcopy = 1;
					return;
				}
			}
}
#endif	/* NEED_BCOPY */
