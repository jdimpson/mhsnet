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


#define	TIME

#include	"global.h"


#define	SIX_MONTHS	(60*60*24*7*4*6)

extern Time_t	Time;			/* Global - current date */



/*
**	Return date in form "mmm dd hh:mm",
**	or, if date older than 6 months, in form "mmm dd  yyyy".
*/

char *
DateString(adate, string)
	Time_t		adate;
	register char *	string;		/* Pointer to buffer of length DATESTRLEN */
{
	register char *	s;
	register long	t;
	time_t		date = adate;

	s = ctime(&date);

	(void)strncpy(string, s+4, 7);

	if ( (t = Time - adate) > SIX_MONTHS || t < 0 )
		(void)strncpy(string+7, s+19, 5);
	else
		(void)strncpy(string+7, s+11, 5);

	string[12] = '\0';

	return string;
}
