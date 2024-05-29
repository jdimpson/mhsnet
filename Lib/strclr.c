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
**	Clear a string 's' of length 'n'.
*/

/*ARGSUSED*/
char *	
strclr(s, n)
	char *		s;
	register int	n;
{
#	if	vax
	asm("	movc5	$0,(ap),$0,8(ap),*4(ap)");		/* Why piss around? */
#	else	/* vax */

	/*
	**	If you can do this faster, in assembler,
	**	then you're a better man than I am, Gunga Din.
	*/

	if ( ((long)n|(long)s) & (sizeof(long)-1) )		/* True for 1% of calls */
	{
		register char *	cp;
		register int	i;

		cp = s;

		if ( (i = (n+7) >> 3) == 0 && n == 0 )
			return cp;

		switch ( n & 7 )
		{
		default:
		case 0:	do {	*cp++ = '\0';
		case 7:		*cp++ = '\0';
		case 6:		*cp++ = '\0';
		case 5:		*cp++ = '\0';
		case 4:		*cp++ = '\0';
		case 3:		*cp++ = '\0';
		case 2:		*cp++ = '\0';
		case 1:		*cp++ = '\0';
			} while ( --i > 0 );
		}
	}
	else
	if ( n /= sizeof(long) )			/* This is 99% of cases */
	{
		register long *	lp;
		register int	i;

		lp = (long *)s;

		i = (n+7) >> 3;

		switch ( n & 7 )
		{
		default:
		case 0:	do {	*lp++ = 0;
		case 7:		*lp++ = 0;
		case 6:		*lp++ = 0;
		case 5:		*lp++ = 0;
		case 4:		*lp++ = 0;
		case 3:		*lp++ = 0;
		case 2:		*lp++ = 0;
		case 1:		*lp++ = 0;
			} while ( --i > 0 );
		}
	}
#	endif	/* vax */

	return s;
}
