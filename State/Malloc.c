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
**	Safe, fast "malloc" for statefile.
*/

#include	<sys/types.h>
#define	_TYPES_

#include	"global.h"
#include	"debug.h"


#ifdef	SLOW_MALLOC

static char *	Last;
static char *	Next;
static char *	End;
static unsigned	Left;

#define		BRKINCR		8192

#ifndef	ALIGN
#define		ALIGN		sizeof(char *)
#endif
#define		MASK		(ALIGN-1)

#ifdef	MALLOC_SIZES
#include	<stdio.h>
static Ulong	Malloc_sizes[32];
#endif	/* MALLOC_SIZES */

#undef	free



char *
Malloc(asize)
	unsigned		asize;
{
	register char *		cp;
	register unsigned	incr;
	register unsigned	size = asize;

	DODEBUG(if(size==0)Fatal("Malloc(0)"));

#	ifdef	MALLOC_SIZES
	register int	i;

	for ( i = 0, incr = 4 ; incr < size ; incr <<= 1, i++ );
	Malloc_sizes[i]++;
#	endif	/* MALLOC_SIZES */

	size += MASK;
	size &= ~MASK;

	if ( Left < size )
	{
		incr = (size-Left) | (unsigned)(BRKINCR-1);
		incr++;

		Trace5(4, "Malloc(%u) => sbrk(%u) [size=%u, Left=%u]", asize, incr, size, Left);

		while ( (cp = (char *)sbrk(incr)) == (char *)-1 )
			Syserror("no memory for malloc(%u)", asize);

		Trace4(2, "Malloc(%u) => sbrk(%u) => %#lx", asize, incr, (PUlong)cp);

		if ( cp == End )
			Left += incr;
		else
		{
			if ( End != NULLSTR )
				Trace3(2, "Malloc() => non-contiguous (%#lx<>%#lx)", (PUlong)End, (PUlong)cp);

			Left = incr;
			Next = cp;
		}

		End = cp + incr;
	}

	cp = Next;
	Last = cp;
	Next += size;
	Left -= size;

	Trace3(5, "Malloc(%u) ==> %#lx", size, (PUlong)cp);

	return cp;
}



char *
Malloc0(size)
	unsigned	size;
{
	return strclr(Malloc(size), size);
}



/*
**	Routines used by UNIX library functions.
*/



#ifdef	ANSI_C
void *
calloc(size_t nelem, size_t size)
{
	return (void *)Malloc0((unsigned)(nelem*size));
}
#else	/* ANSI_C */
Pointer
calloc(nelem, size)
	unsigned	nelem;
	unsigned	size;
{
	return Malloc0(nelem*size);
}
#endif	/* ANSI_C */



void
Free(p)
	char *	p;
{
	Trace2(5, "free(%#lx)", (PUlong)p);

	if ( p == NULLSTR )
	{
		if ( Traceflag )
			Fatal("free(0)");

		return;
	}
	else
	if ( p != Last )
		return;

	Left += Next - Last;
	Next = Last;
	Last = NULLSTR;
}



void
#ifdef	ANSI_C
free(void *p)
{
	Free((char *)p);
}
#else	/* ANSI_C */
free(p)
	char *	p;
{
	Free(p);
}
#endif	/* ANSI_C */



#ifdef	ANSI_C
void *
malloc(size_t size)
{
	return (void *)Malloc((unsigned)size);
}
#else	/* ANSI_C */
Pointer
malloc(size)
	unsigned	size;
{
	return Malloc(size);
}
#endif	/* ANSI_C */



#ifdef	ANSI_C
void *
realloc(void *old, size_t size)
{
	register void *	new;

	Free((char *)old);
	if ( (new = (void *)Malloc((unsigned)size)) != old )
		bcopy(old, new, size);
	return new;
}
#else	/* ANSI_C */
Pointer
realloc(old, size)
	char *		old;
	unsigned	size;
{
	register char *	new;

	Free(old);
	if ( (new = Malloc(size)) != old )
		bcopy(old, new, size);
	return new;
}
#endif	/* ANSI_C */



#ifdef	MALLOC_SIZES
void
Print_MS(fd)
	FILE *	fd;
{
	register int	i;
	register Ulong	s, j, k;

	(void)fprintf(fd, "Malloc sizes histogram:\n");

	for ( k = 0, j = 4, i = 0 ; i < 32 ; i++, k = j, j <<= 1 )
		if ( (s = Malloc_sizes[i]) != 0 )
			(void)fprintf(fd, "%10ld-%-10ld%12ld\n", (PUlong)k, (PUlong)j, (PUlong)s);
}
#endif	/* MALLOC_SIZES */



#else	/* SLOW_MALLOC == 1 */
static int _dummy_Malloc(){}
#endif	/* SLOW_MALLOC == 1 */
