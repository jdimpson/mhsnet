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
**	Routines to de/code cooked virtual circuits.
*/

#include	"global.h"
#include	"debug.h"
#include	"driver.h"
#include	"packet.h"


/*
**	Cooking character arrays, != 0 means cook with indicated char.
**
**	The four groups of characters are indicated by the bottom 2 bits
**	of the `escape' chars '$', '%', '&', and '\''.
*/

static char	cook_A[256] =	/* All non-printing chars are escaped */
{
/*000*/	 '$',  '$',  '$',  '$',  '$',  '$',  '$',  '$',
/*010*/	 '$',    0,    0,  '$',  '$',  '$',  '$',  '$',	/* '\t', '\n' */
/*020*/	 '$',  '$',  '$',  '$',  '$',  '$',  '$',  '$',
/*030*/	 '$',  '$',  '$',  '$',  '$',  '$',  '$',  '$',
/*040*/	   0,    0,    0,    0,  '$',  '$',  '$',  '$',	/* ' ' - # */
/*050*/	   0,    0,    0,    0,    0,    0,    0,    0,	/* ( - / */
/*060*/	   0,    0,    0,    0,    0,    0,    0,    0,	/* 0 - 7 */
/*070*/	   0,    0,    0,    0,    0,    0,    0,    0,	/* 8 - ? */
/*100*/	   0,    0,    0,    0,    0,    0,    0,    0,	/* @ - G */
/*110*/	   0,    0,    0,    0,    0,    0,    0,    0,	/* H - O */
/*120*/	   0,    0,    0,    0,    0,    0,    0,    0,	/* P - W */
/*130*/	   0,    0,    0,    0,    0,    0,    0,    0,	/* X - _ */
/*140*/	   0,    0,    0,    0,    0,    0,    0,    0,	/* ` - g */
/*150*/	   0,    0,    0,    0,    0,    0,    0,    0,	/* h - o */
/*160*/	   0,    0,    0,    0,    0,    0,    0,    0, /* p - w */
/*170*/	   0,    0,    0,    0,    0,    0,    0,  '%',	/* x - ~ */
/*200*/	 '&',  '&',  '&',  '&',  '&',  '&',  '&',  '&',
/*210*/	 '&',  '&',  '&',  '&',  '&',  '&',  '&',  '&',
/*220*/	 '&',  '&',  '&',  '&',  '&',  '&',  '&',  '&',
/*230*/	 '&',  '&',  '&',  '&',  '&',  '&',  '&',  '&',
/*240*/	 '&',  '&',  '&',  '&',  '&',  '&',  '&',  '&',
/*250*/	 '&',  '&',  '&',  '&',  '&',  '&',  '&',  '&',
/*260*/	 '&',  '&',  '&',  '&',  '&',  '&',  '&',  '&',
/*270*/	 '&',  '&',  '&',  '&',  '&',  '&',  '&',  '&',
/*300*/	'\'', '\'', '\'', '\'', '\'', '\'', '\'', '\'',
/*310*/	'\'', '\'', '\'', '\'', '\'', '\'', '\'', '\'',
/*320*/	'\'', '\'', '\'', '\'', '\'', '\'', '\'', '\'',
/*330*/	'\'', '\'', '\'', '\'', '\'', '\'', '\'', '\'',
/*340*/	'\'', '\'', '\'', '\'', '\'', '\'', '\'', '\'',
/*350*/	'\'', '\'', '\'', '\'', '\'', '\'', '\'', '\'',
/*360*/	'\'', '\'', '\'', '\'', '\'', '\'', '\'', '\'',
/*370*/	'\'', '\'', '\'', '\'', '\'', '\'', '\'', '\'',
};

#define	OFFSET	050	/* Offset cooked char by this (need 64 contiguous zeros in `cook_A') */


static long	convert _FA_((char *));



/*
**	ASCII cooking.
**
**	Cooking requires a buffer.
*/

int
CookA(data, size, buffer)
	char *		data;
	register int	size;
	char *		buffer;		/* Should be at least (size*2 + (size*2)/78 + 2) */
{
	register int	c;
	register Uchar *cp;
	register char *	bp;
	register int	x;
	register int	llen;

	TraceData("CookA", size, data);

	llen = 0;
	bp = buffer;
	cp = (Uchar *)data;

	while ( size-- > 0 )
	{
		if ( (x = cook_A[(c = *cp++)]) )
		{
			*bp++ = x;
			*bp++ = (c & 077) + OFFSET;
			llen++;
		}
		else
			*bp++ = c;

		if ( ++llen > 78 )
		{
			*bp++ = '\r';
			llen = 0;
		}
	}

	if ( llen )
		*bp++ = '\r';

	return bp - buffer;
}



/*
**	ASCII un-cooking.
**
**	Uncooking done in place.
*/

int
UncookA(data, size)
	char *		data;
	register int	size;
{
	register int	c;
	register char *	cp;
	register char *	bp;
	static int	last_esc;

	bp = cp = data;

	if ( (c = last_esc) && --size >= 0 )
	{
		last_esc = 0;
		goto restart;
	}

	while ( --size >= 0 )
	{
		c = *cp++ & 0177;

		if ( cook_A[c] == 0 )
		{
			*bp++ = c;
			continue;
		}

		if ( c < '$' || c > '\'' )
			continue;	/* Discard extra char */

		if ( --size < 0 )
		{
			last_esc = c;
			break;
		}
restart:
		*bp++ = ((*cp++ & 0177) - OFFSET) | ((c & 3) << 6);
	}

	TraceData("UncookA", bp - data, data);

	return bp - data;
}



/*
**	XON/XOFF cooking.
**
**	Cooking requires a buffer.
*/

int
CookX(data, size, buffer)
	char *		data;
	register int	size;
	char *		buffer;		/* Should be at least (size*2 + 2) */
{
	register int	c;
	register Uchar *cp;
	register char *	bp;
	register int	x;

	TraceData("CookX", size, data);

	bp = buffer;
	cp = (Uchar *)data;

	while ( size-- > 0 )
	{
		if ( (x = cook_A[(c = *cp++)]) )
		{
			*bp++ = x;
			*bp++ = (c & 077) + OFFSET;
		}
		else
			*bp++ = c;
	}

	return bp - buffer;
}



/*
**	XON/XOFF un-cooking.
**
**	Uncooking done in place.
*/

int
UncookX(data, size)
	char *		data;
	register int	size;
{
	register int	c;
	register char *	cp;
	register char *	bp;
	static int	last_esc;

	bp = cp = data;

	if ( (c = last_esc) && --size >= 0 )
	{
		last_esc = 0;
		goto restart;
	}

	while ( --size >= 0 )
	{
		c = *cp++ & 0377;

		if ( cook_A[c] == 0 )
		{
			*bp++ = c;
			continue;
		}

		if ( c < '$' || c > '\'' )
			continue;	/* Discard extra char */

		if ( --size < 0 )
		{
			last_esc = c;
			break;
		}
restart:
		*bp++ = ((*cp++ & 0377) - OFFSET) | ((c & 3) << 6);
	}

	TraceData("UncookX", bp - data, data);

	return bp - data;
}



/*
**	Set cook_A[] array with selected chars.
*/

char *
SetCookX(ckp, s)
	CookT *		ckp;	/* Pointer to CookT array */
	register char *	s;	/* String of form nnn[,nnn...] */
{
	register char *	cp;
	register int	n;
	register int	x;

	Trace2(1, "SetCookX(%s)", s);

	(void)strclr(cook_A, 256);		/* Clear out defaults */
	ckp->cooked = 0;

	while ( s != NULLSTR && *s != '\0' )
	{
		if ( (cp = strchr(s, ',')) != NULLSTR )
			*cp = '\0';		/* Peal off next */

		if ( (n = convert(s)) > 255 || (n < (OFFSET+64) && n >= (OFFSET-4)) )
			return english("each char must be in range 0->35,104->255");

		for ( ;; )
		{
			x = "$%&'"[n>>6];	/* Select escape char */
			cook_A[n] = x;		/* Escape char */
			ckp->cooked++;		/* Count escapes */

			Trace4(3, "escape %03o with '%c%c'", n, x, (n&077)+OFFSET);

			if ( cook_A[x] )
				break;

			n = x;			/* Escape escape char */
		}

		if ( (s = cp) == NULLSTR )
			break;

		*s++ = ',';
	}

	return NULLSTR;
}



/*
**	Set default escapes for CookX().
*/

void
DefCookX(ckp)
	CookT *	ckp;
{
	char *	def;

	def = newstr("03,021,023,034,0203,0221,0223,0234");	/* In case strings unwritable */
	(void)SetCookX(ckp, def);				/* Modifies `def' */
	free(def);
}



/*
**	Convert number to integer.
*/

static long
convert(s)
	register char *	s;
{
	if ( *s == '0' )
	{
		if ( (*++s|040) == 'x' )
			return xtol(++s);

		return otol(s);
	}

	return atol(s);
}



/*
**	Cooking type array.
**
**	Add one entry for each cook/uncook set.
*/

CookT	Cookers[]	=
{
	{"A", CookA, UncookA, 163},
	{"X", CookX, UncookX, 0, SetCookX, DefCookX}
};

int	NCookers	= sizeof(Cookers)/sizeof(CookT);
