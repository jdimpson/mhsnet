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
**	Fetch system time in ticks, if possible.
*/

#ifndef	GLBOLT
#define	TIMES
#define	NEED_HZ

#include	"global.h"

#if	BSD4 >= 2
#include	<sys/time.h>
#endif



Time_t
ticks()
{
#	if	SYSTEM > 0
	struct tms	stimes;

	return (Time_t)times(&stimes);

#	else	/* SYSTEM > 0 */

#	if	BSD4 >= 2
	struct timeval	tv;

	(void)gettimeofday(&tv, (struct timezone *)0);

	return (Time_t)((((float)(tv.tv_sec%1000000)*1000000.0 + (float)tv.tv_usec) * (float)HZ) / 1000000.0);

#	else	/* BSD4 >= 2 */

	return time((long *)0) * HZ;

#	endif	/* BSD4 >= 2 */
#	endif	/* SYSTEM > 0 */
}

#else	/* GLBOLT */

long
ticks()
{
	extern long	glbolt();

	return glbolt();
}
#endif	/* GLBOLT */
