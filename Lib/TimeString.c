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


static bool	zero;		/* TRUE if value printed is zero */

static char *	ptn _FA_((Time_t, char *, int));



/*
**	Return time in form "nnnXnnY" where X&Y can be any of 'd','h','m','s'.
*/

char *
TimeString(time, string)
	register Time_t	time;	/* Seconds */
	register char *	string;	/* Pointer to buffer of length TIMESTRLEN */
{
	register int	trac;
	register int	sepc;
	register int	divn;

	sepc = 'm';
	trac = 's';
	divn = 60;

	if ( time >= (60*60) )	/* More than 60 mins. */
	{
		trac = sepc;
		sepc = 'h';
		time /= 60;	/* Minutes */

		if ( time >= (60*24*7) )	/* More than 1 week */
		{
			trac = sepc;
			sepc = 'd';
			divn = 24;
			time /= 60;		/* Hours */

			if ( time >= (24*1000) )	/* More than 1000 days */
				trac = 0;
		}
	}

	zero = true;

	if ( trac == 0 )
	{
		time /= divn;
		(void)ptn(time, string, 6);
		string[6] = sepc;
	}
	else
	{
		(void)ptn(time/divn, string, 3);
		string[3] = zero ? ' ' : sepc;
		(void)ptn(time%divn, &string[4], 2);
		string[6] = zero ? '0' : trac;
	}

	string[7] = '\0';

	for ( trac = 0 ; string[trac] == ' ' ; trac++ );

	return &string[trac];
}



/*
**	Put a decimal number into string with leading spaces.
*/

static char *
ptn(t, s, n)
	register Time_t	t;
	register char *	s;
	register int	n;
{
	if ( --n )
		s = ptn(t/10, s, n);

	if ( (t %= 10) || !zero )
	{
		zero = false;
		*s++ = t + '0';
	}
	else
		*s++ = ' ';

	return s;
}
