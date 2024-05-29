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


#include	"global.h"


#if	NEED_BCOPY

/*
**	Copy 'count' bytes from 'from' to 'to'.
*/

void
bcopy(from, to, count)
#	if	ANSI_C
	const char *	from;
#	else	/* ANSI_C */
	char *		from;
#	endif	/* ANSI_C */
	char *		to;
	register int	count;
{
#	ifndef	vax

	if ( ((int)from|(int)to) & (sizeof(long)-1) )
	{
		register char *		cp1;
#		if	ANSI_C
		register const char *	cp2;
#		else	/* ANSI_C */
		register char *		cp2;
#		endif	/* ANSI_C */
		register int		i;

		cp1 = to;
		cp2 = from;

		if ( (i = (count+7) >> 3) == 0 && count == 0 )
			return;

		switch ( count & 7 )
		{
		default:
		case 0:	do {	*cp1++ = *cp2++;
		case 7:		*cp1++ = *cp2++;
		case 6:		*cp1++ = *cp2++;
		case 5:		*cp1++ = *cp2++;
		case 4:		*cp1++ = *cp2++;
		case 3:		*cp1++ = *cp2++;
		case 2:		*cp1++ = *cp2++;
		case 1:		*cp1++ = *cp2++;
			} while ( --i > 0 );
		}

		return;
	}

	{
		register long *	lp1;
#		if	ANSI_C
		register const long *	lp2;
#		else	/* ANSI_C */
		register long *		lp2;
#		endif	/* ANSI_C */
		register int	i;

		if ( i = count / sizeof(long) )
		{
			register int	n;

			lp1 = (long *)to;
#			if	ANSI_C
			lp2 = (const long *)from;
#			else	/* ANSI_C */
			lp2 = (long *)from;
#			endif	/* ANSI_C */

			n = (i+7) >> 3;

			switch ( i & 7 )
			{
			default:
			case 0:	do {	*lp1++ = *lp2++;
			case 7:		*lp1++ = *lp2++;
			case 6:		*lp1++ = *lp2++;
			case 5:		*lp1++ = *lp2++;
			case 4:		*lp1++ = *lp2++;
			case 3:		*lp1++ = *lp2++;
			case 2:		*lp1++ = *lp2++;
			case 1:		*lp1++ = *lp2++;
				} while ( --n > 0 );
			}
		}

		if ( i = count & (sizeof(long)-1) )
		{
			if ( count >= sizeof(long) )
			{
				to = (char *)lp1;
#				if	ANSI_C
				from = (const char *)lp2;
#				else	/* ANSI_C */
				from = (char *)lp2;
#				endif	/* ANSI_C */
			}

			do
				*to++ = *from++;
			while
				( --i > 0 );
		}
	}

#	else	/* vax */

		asm("movc3	12(ap), *4(ap), *8(ap)");
#		ifdef	lint
			from = &to[count];
#		endif	/* lint */

#	endif	/* vax */
}

#else	/* NEED_BCOPY */
void
dummy_bcopy(){}
#endif	/* NEED_BCOPY */
