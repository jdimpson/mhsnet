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


#include	<time.h>

/*
**	Produce time string in format suitable for RFC822 mail headers:-
*/
static char	buf[]	= "ddd, dd mmm yy hh:mm:ss GMT\n";
/*			   0123 56 890 23 56 89 12 456 7 8
**			   0         1         2           [29]
*/

char *
rfc822gmtime(timep)
	long *			timep;
{
	register char *		s;

	extern char *		asctime();
	extern struct tm *	gmtime();
	extern char *		strncpy();

	s = asctime(gmtime(timep));

	buf[0] = s[0];
	buf[1] = s[1];
	buf[2] = s[2];

	buf[5] = s[8];
	buf[6] = s[9];

	buf[8] = s[4];
	buf[9] = s[5];
	buf[10] = s[6];

	buf[12] = s[22];
	buf[13] = s[23];

	(void)strncpy(&buf[15], &s[11], 8);

	return buf;
}
