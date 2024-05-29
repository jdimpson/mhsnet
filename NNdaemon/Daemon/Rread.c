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
**	Remote read for handler
*/

#define	TERMIOCTL
#define	TIME
#define	ERRNO

#include	"global.h"
#include	"debug.h"

#include	<setjmp.h>

#include	"Channel.h"

#include	"daemon.h"
#include	"Stream.h"
#include	"driver.h"


extern jmp_buf	IOerrorbuf;

#ifdef	VCBUFSIZE
static char	PktBuf[VCBUFSIZE];
static int	PktSize;
static char *	PktPtr;
#endif	/* VCBUFSIZE */



void
Rread(data, size, alrm_on, alrm_off)
	register char *	data;
	register int	size;
	bool		alrm_on;
	bool		alrm_off;
{
	register int	n;
#	if	DEBUG >= 1
	char *		d = data;
	int		s = size;
	static int	syncs;
#	endif	/* DEBUG >= 1 */

	if ( alrm_on || (alrm_off && !Alarm) )
		IOALRMON(ScanRate);

	if ( Wbufcount )
		(void)RWflush(1);

#	if	DEBUG == 2
	if ( size > 1 )
	{
		if ( syncs )
		{
			Trace2(2, "%d*Rread 1", syncs);
			syncs = 0;
		}

		Trace6(
			2,
			"Rread %d  on=%d, off=%d, scan=%d, time=%lu",
			size,
			alrm_on,
			alrm_off,
			ScanRate,
			(PUlong)time((time_t *)0)
		);
	}
	else
	if ( size == 1 )
		syncs++;
#	endif	/* DEBUG == 2 */

#	ifdef	VCBUFSIZE
	while ( size > 0 && !Finish )
	{
		if ( (n = PktSize) == 0 )
		{
			if ( (n = read(0, PktBuf, sizeof PktBuf)) == 0 )
			{
#				ifdef	APOLLO
read0:
#				endif	/* APOLLO */
				Trace1(1, "Rread ZERO");
				pause();	/* Await timeout */
				continue;
			}
			else
			if ( n < 0 )
			{
#				ifdef	APOLLO
				if ( errno == EIO )
					goto read0;
#				endif	/* APOLLO */
#				if	DEBUG >= 1
				if ( errno == EINTR )
					continue;
#				endif	/* DEBUG */
				longjmp(IOerrorbuf, 1);
				break;
			}

			PktSize = n;
			PktPtr = PktBuf;
			StVCdata(STINCHAN, n);
		}

		if ( n > size )
			n = size;

		bcopy(PktPtr, data, n);

		PktPtr += n;
		PktSize -= n;

		data += n;
		size -= n;
	}

#	else	/* VCBUFSIZE */

	if ( size > 0 )
	{
		while ( (n = read(0, data, size)) != size && !Finish )
		{
			if ( n >= 0 )
			{
			 	if ( n == 0 )
				{
#					ifdef	APOLLO
read0:
#					endif	/* APOLLO */
					Trace1(1, "Rread ZERO");
					pause();	/* Await timeout */
					continue;
				}
				else
				{
					if ( PprotoT == PT_DATAGRAM )
					{
						DODEBUG(s = n);
						break;
					}

					Trace2(2, "Rread part=%d", n);

					data += n;
					size -= n;
					StVCdata(STINCHAN, n);
				}
			}
			else
			{
#				ifdef	APOLLO
				if ( errno == EIO )
					goto read0;
#				endif	/* APOLLO */
#				if	DEBUG >= 1
				if ( errno == EINTR )
					continue;
#				endif	/* DEBUG */
				longjmp(IOerrorbuf, 1);
				break;
			}
		}

		StVCdata(STINCHAN, n);
	}

#	endif	/* VCBUFSIZE */

	if ( alrm_off )
	{
		IOALRMOFF();
		NNstate.inpkts++;
	}

	Trace2(3, "Rread \"%s\"", ExpandString(d, s));
}
