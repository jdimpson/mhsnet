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
**	Remote "cooked" read for handler.
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
extern char	magic_chars[];
extern char	rep_char;
void		Cread _FA_((char *, int));



void
RCread(data, size, alrm_on, alrm_off)
	char *	data;
	int	size;
	bool	alrm_on;
	bool	alrm_off;
{
#	if	DEBUG == 2
	static int	syncs;

	if ( size > 1 )
	{
		if ( syncs )
		{
			Trace2(2, "%d*RCread 1", syncs);
			syncs = 0;
		}
		Trace6(
			2,
			"RCread %d on=%d off=%d scan=%d time=%lu",
			size,
			alrm_on, alrm_off,
			ScanRate,
			(PUlong)time((time_t *)0)
		);
	}
	else
	if ( size == 1 )
		syncs++;
#	endif	/* DEBUG == 2 */

	if ( alrm_on || (alrm_off && !Alarm) )
		IOALRMON(ScanRate);

	if ( Wbufcount )
		(void)RWflush(1);

	if ( size > 0 )
		Cread(data, size);

	if ( alrm_off )
	{
		IOALRMOFF();
		NNstate.inpkts++;
	}

	Trace2(3, "RCread \"%s\"", ExpandString(data, size));
}


void
Cread(b, n)
	char *		b;
	int		n;
{
	register int	c;
	register char *	bp;
	register char * cp;
	register int	repcnt;
	register int	i;
	register int	j;
	register char * p;
	register enum
	{
		Norm, Special, Repcnt
	}
			state;
	static char	cbuf[sizeof(Packet)];
	static char *	cbp = (char *)0;
	static int	lastj = 0;
	static int	svrepcnt = 0;
	static char	svrepchr;

	DODEBUG(char	sbuf[sizeof(Packet)*2]);
	DODEBUG(char *	sbp = sbuf);

	Trace2(3, "Cread %d", n);

	i = n;
	cp = b;
	state = Norm;

	if ((repcnt = svrepcnt) > 0)
	{
		c = svrepchr;
		Trace3(3, "Cread had %d * 0x%x", repcnt, c);
		while (repcnt > 0 && i > 0)
		{
			*cp++ = c;
			i--;
			repcnt--;
		}
		svrepcnt = repcnt;
		if (i <= 0) {
			DODEBUG(if((i=sbp-sbuf)>0)Trace2(2,"Cread got \"%s\"",ExpandString(sbuf,i)));
			if (repcnt > 0)
				Trace3(3, "Cread left %d * 0x%x", repcnt, c);
			Trace2(3, "Cread \"%s\"", ExpandString(b, n));
			return;
		}
	}

	while ( i > 0 && !Finish )
	{
		if ( cbp == (char *)0 )
		{
			bp = (CookVC != (CookT *)0) ? cp : cbuf;

			if ( (j = read(0, bp, i)) == 0 )
			{
				pause();	/* Await timeout */
				continue;
			}
			else
			if ( j == SYSERROR )
			{
#				if	DEBUG >= 1
				if ( errno == EINTR )
					continue;
#				endif	/* DEBUG */
				longjmp(IOerrorbuf, 1);
				break;
			}
			StVCdata(STINCHAN, j);
		}
		else
		{
			bp = cbp;
			j = lastj;
			Trace2(3, "Cread had \"%s\"", ExpandString(bp, j));
		}

		cbp = (char *) 0;

		if ( CookVC != (CookT *)0 )
		{
			c = (*CookVC->uncook)(cp, j);
			cp += c;
			i -= c;
		}
		else
		while ( --j >= 0 )
		{
			c = *bp++ & 0x7F;
			DODEBUG(*sbp++ = c);

			if ( c < ' ' || c > '~' )
				continue;

			if ( repcnt <= 0 )
				repcnt = 1;

			switch ( state )
			{
			case Norm:
				if ( c == rep_char )
				{
					state = Repcnt;
					continue;
				}

				if ( (p = strchr(magic_chars, c)) )
				{
					*cp = (p - magic_chars) << 6;
					state = Special;
					continue;
				}
				break;

			case Special:
				c &= 077;
				c |= *cp;
				state = Norm;
				break;

			case Repcnt:
				repcnt = c & 077;
				state = Norm;
				continue;
			}

			while ( repcnt > 0 && i > 0 )
			{
				*cp++ = c;
				i--;
				repcnt--;
			}

			if ( i <= 0 && (j > 0 || repcnt > 0) )
			{
				if (j > 0)
				{
					cbp = bp;
					lastj = j;
					Trace2(3, "Cread left \"%s\"", ExpandString(bp, j));
				}
				if (repcnt > 0)
				{
					svrepcnt = repcnt;
					svrepchr = c;
					Trace3(3, "Cread left %d * 0x%x", repcnt, c);
				}
				DODEBUG(if((i=sbp-sbuf)>0)Trace2(2,"Cread got \"%s\"",ExpandString(sbuf,i)));
				Trace2(3, "Cread \"%s\"", ExpandString(b, n));
				return;
			}
		}
	}

	DODEBUG(if((i=sbp-sbuf)>0)Trace2(2,"Cread got \"%s\"",ExpandString(sbuf,i)));
	Trace2(3, "Cread \"%s\"", ExpandString(b, n));
}
