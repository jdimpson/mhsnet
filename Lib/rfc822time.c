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


#include	<stdio.h>
#include	<string.h>
#include	<time.h>



/*
**	Produce time string in format suitable for RFC822 mail headers:-
*/
static char	buf[]	= "ddd, dd mmm 19yy hh:mm:ss +hhmm\n";
/*			   012  56 890 2345 78 01 34 67890 1 2
**			   0         1         2         3    [32]
*/

char *
rfc822time(timep)
	time_t *		timep;
{
	register char *		s;
	register struct tm *	tp;
	register int		day;
	char			sign;

	static char		zone[8];
	struct tm		gmt;

#	if 0
	extern char *		ctime();
	extern struct tm *	gmtime();
	extern struct tm *	localtime();
	extern char *		strcpy();
	extern char *		strncpy();
#	endif

	s = ctime(timep);

	buf[0] = s[0];
	buf[1] = s[1];
	buf[2] = s[2];

	if ( (buf[5] = s[8]) == ' ' )
		buf[5] = '0';
	buf[6] = s[9];

	buf[8] = s[4];
	buf[9] = s[5];
	buf[10] = s[6];

	buf[12] = s[20];
	buf[13] = s[21];
	buf[14] = s[22];
	buf[15] = s[23];

	(void)strncpy(&buf[17], &s[11], 8);

	if ( zone[0] == '\0' )
	{
		gmt = *gmtime(timep);
		tp = localtime(timep);

		if ( gmt.tm_year == tp->tm_year )
		{
			if ( gmt.tm_yday == tp->tm_yday )
				day = 0;
			else
			if ( gmt.tm_yday < tp->tm_yday )
				day = 1;
			else
				day = -1;
		}
		else
		if ( gmt.tm_year < tp->tm_year )
			day = 1;
		else
			day = -1;

		day = (tp->tm_hour + day * 24) * 60 + tp->tm_min;
		day -= gmt.tm_hour * 60 + gmt.tm_min;

		if ( day < 0 )
		{
			day = -day;
			sign = '-';
		}
		else
			sign = '+';

		(void)sprintf(zone, "%c%2.2d%2.2d\n", sign, day/60, day%60);
	}

	(void)strcpy(&buf[26], zone);

	return buf;
}
