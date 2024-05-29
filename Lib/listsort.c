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
#include	"debug.h"


typedef struct ListEl *	ListP;

struct ListEl
{
	ListP	next;
};



/*
**	Sort a list in place using merge sort.
*/

void
#ifdef	ANSI_C
listsort(char **list, int (*compar)(char *, char *))
#else	/* ANSI_C */
listsort(list, compar)
	ListP *		list;
	int		(*compar)();	/* Same as compar in qsort(3) */
#endif	/* ANSI_C */
{
	register ListP *mt;
	register ListP	tmp;
	ListP		a;
	register ListP	b;
	register int	j;
	register ListP	t;
	ListP		sorted[32];	/* (2**33)-1 items */

	if ( (t = *(ListP *)list) == (ListP)0 )
		return;

	for ( j = 0 ; j < 32 ; j++ )
		sorted[j] = (ListP)0;

	while ( (a = t) != (ListP)0 && (b = a->next) != (ListP)0 )
	{
		t = b->next;

		if ( (*compar)((char *)a, (char *)b) > 0 )
		{
			b->next = a;
			a->next = (ListP)0;
			a = b;
		}
		else
			b->next = (ListP)0;

		for ( j = 0 ; ; j++ )
		{
			if ( (b = sorted[j]) == (ListP)0 )
			{
				sorted[j] = a;
				break;
			}

			/** Merge equal length lists a and b **/

			sorted[j] = (ListP)0;

			for ( mt = &a ;; )
			{
				if ( (*compar)((char *)*mt, (char *)b) > 0 )
				{
					tmp = b->next;
					b->next = *mt;
					*mt = b;

					if ( (b = tmp) == (ListP)0 )
						break;
				}

				mt = &(*mt)->next;

				if ( *mt == (ListP)0 )
				{
					*mt = b;
					break;
				}
			}
		}
	}

	for ( j = 0 ; a == (ListP)0 ; j++ )
		a = sorted[j];

	for ( ; j < 32 ; j++ )
	{
		if ( (b = sorted[j]) != (ListP)0 )
		{
			for ( mt = &a ;; )
			{
				if ( (*compar)((char *)*mt, (char *)b) > 0 )
				{
					tmp = b->next;
					b->next = *mt;
					*mt = b;

					if ( (b = tmp) == (ListP)0 )
						break;
				}

				mt = &(*mt)->next;

				if ( *mt == (ListP)0 )
				{
					*mt = b;
					break;
				}
			}
		}
	}

#	ifdef	ANSI_C
	*list = (void *)a;
#	else	/* ANSI_C */
	*list = a;
#	endif	/* ANSI_C */
}
