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


#define	SMDATE

#include	"global.h"
#if	UTIMES
#include	<sys/time.h>
#endif	/* UTIMES */



/*
**	Set modify date of a file.
*/

void
SMdate(path, mtime)
	char *	path;
	Time_t	mtime;
{
#	if	UTIMES
	struct timeval	times[2];

	times[0].tv_sec = mtime;
	times[0].tv_usec = 0;
	times[1].tv_sec = mtime;
	times[1].tv_usec = 0;

	(void)utimes(path, times);	/* This is for systems with broken "utime(2)" */

#	else	/* UTIMES */

	struct 
	{
		time_t	ut_atime;
		time_t	ut_mtime;
	}
		times;

	times.ut_atime = mtime;
	times.ut_mtime = mtime;

	(void)utime(path, &times);
#	endif	/* UTIMES */
}
