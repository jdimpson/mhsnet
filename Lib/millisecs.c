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


#define	TIMES
#define	NEED_HZ

#include	"global.h"

#if	V8 == 1
#include	<sys/timeb.h>
#endif	/* V8 == 1 */
#if	SYSVREL >= 4 || BSD4 >= 2 || linux == 1
#include	<sys/time.h>
#endif	/* SYSVREL >= 4 || BSD4 >= 2 || linux == 1 */



/*
**	Fetch system time in milliseconds, if possible.
*/

Time_t
millisecs()
{
#	if	SYSVREL >= 4 || BSD4 >= 2 || linux == 1
	struct timeval	tv;

	(void)gettimeofday(&tv, (struct timezone *)0);

	return (Time_t)(((float)(tv.tv_sec%1000000)*(float)1000000.0 + (float)tv.tv_usec) / (float)1000.0);

#	else	/* SYSVREL >= 4 || BSD4 >= 2 || linux == 1 */

#	if	SYSTEM > 0
	struct tms	stimes;

	return (Time_t)((float)(times(&stimes)%100000000) * ((float)1000.0 / (float)HZ));

#	else	/* SYSTEM > 0 */

#	if	V8 == 1
	struct timeb	tv;

	(void)ftime(&tv);

	return (Time_t)((float)(tv.time%1000000)*(float)1000 + (float)tv.millitm);

#	else	/* V8 == 1 */

	return (Time_t)((time((long *)0)%1000000) * 1000.0);

#	endif	/* V8 == 1 */
#	endif	/* SYSTEM > 0 */
#	endif	/* SYSVREL >= 4 || BSD4 >= 2 */
}
