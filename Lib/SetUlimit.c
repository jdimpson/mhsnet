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


/*
**	AT&T Systems usually have a default file size limit which
**	is totally inappropriate. Set it larger here.
**
**	The magic number refers to BSIZE units, which varies with
**	file system type -- and is therefore an imprecise number.
*/

#if	SYSVREL >= 4
#define	MINULIMIT	((1L << 16) - 1)	/* Set by system configuration for setuid programs(!) */
#else	/* SYSVREL >= 4 */
#define	MINULIMIT	((1L << 19) - 1)	/* Somewhere between 200Mb and 4Gb */
#endif	/* SYSVREL >= 4 */

long	Ulimit		= ((1L << 21) - 1);	/* Somewhere around 1Gb */


/*
**	Set SYSTEM V ulimit.
*/

void
SetUlimit()
{
#	if	SYSTEM > 0 && QNX == 0
	long		ul;
	extern long	ulimit();

	if ( ulimit(2, Ulimit) == SYSERROR && (ul = ulimit(1, (long)0)) < MINULIMIT )
		Syserror(english("could not raise \"ulimit\" from %ld to %ld"), ul, Ulimit);
#	endif	/* SYSTEM > 0 && QNX == 0 */
}
