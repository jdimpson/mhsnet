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
#define	TIMES	/* To get types.h */

#include	"global.h"
#include	"debug.h"



/*
**	Return date in form "yy/mm/dd hh:mm:ss".
*/

char *
DateTimeStr(adate, string)
	Time_t			adate;
	char *			string;	/* Pointer to buffer of length DATETIMESTRLEN */
{
	register struct tm *	tmp;
	time_t			date = adate;

	tmp = localtime(&date);

	if ( tmp->tm_year >= 100 )
		tmp->tm_year -= 100;	/* Take care of anno domini 2000 */

	(void)sprintf
	(
		string,
		"%02d/%02d/%02d %02d:%02d:%02d",
		tmp->tm_year, tmp->tm_mon+1, tmp->tm_mday,
		tmp->tm_hour, tmp->tm_min, tmp->tm_sec
	);

	return string;
}
