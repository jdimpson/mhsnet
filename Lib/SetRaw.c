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


#define	TERMIOCTL

#include	"global.h"
#include	"debug.h"

#if	QNX
#undef	SYSVREL
#define	SYSVREL	3
#endif	/* QNX */

#if	SYSVREL >= 4
#include	<sys/stropts.h>
#endif	/* SYSVREL >= 4 */

#if	V8
extern int	tty_ld;
extern int	ntty_ld;
#endif	/* V8 */



/*
**	Set RAW mode on `tty' link.
**	Ignores non-`tty' devices, returning SYSERROR.
*/
/*ARGSUSED*/
int
SetRaw(fd, speed, minchars, mintime, xon_xoff)
	int	fd;
	int	speed;
	int	minchars;
	int	mintime;
	bool	xon_xoff;
{
#	if	SYSTEM < 3
	struct sgttyb	params;

	Trace3(1, "SetRaw speed=%d, xon_xoff=%d", speed, xon_xoff);

	if ( gtty(fd, &params) == SYSERROR )
	{
		speed = SYSERROR;
		goto out;
	}

	if ( speed )
		params.sg_ispeed = params.sg_ospeed = speed;
	else
		speed = params.sg_ispeed;

	if ( xon_xoff )
	{
#		ifdef	TIOCLSET
		static int	lmode;
#		endif	/* TIOCLSET */

#		if	V8 == 1
		if
		(
			(minchars = ioctl(fd, FIOLOOKLD, (struct sgttyb *)0)) != tty_ld
			&&
			minchars != ntty_ld
		)
			(void)ioctl(fd, FIOPUSHLD, (struct sgttyb *)&tty_ld);
#		endif	/* V8 == 1 */

#		ifdef	TIOCLSET
		lmode = LDECCTQ
#			ifdef	LLITOUT
			|LLITOUT
#			endif
#			ifdef	LPASS8
			|LPASS8
#			endif
#			ifdef	LNOFLSH
			|LNOFLSH
#			endif
			;
		(void)ioctl(fd, TIOCLSET, &lmode);
#		endif	/* TIOCLSET */

		params.sg_flags = EVENP|ODDP
#				ifdef	CBREAK
				  |CBREAK
#				endif	/* CBREAK */
#				ifdef	TANDEM
				  |TANDEM
#				endif	/* TANDEM */
				  ;
	}
	else
	{
		params.sg_flags = RAW;

#		if	V8 == 1
		if
		(
			(minchars = ioctl(fd, FIOLOOKLD, (struct sgttyb *)0)) == tty_ld
			||
			minchars == ntty_ld
		)
			(void)ioctl(fd, FIOPOPLD, (struct sgttyb *)0);
#		endif	/* V8 == 1 */
	}

	if ( stty(fd, &params) == SYSERROR )
		Syserror("SetRaw \"stty\"");

#	else	/* SYSTEM < 3 */
#	if	SYSVREL >= 4
	char	lookbuf[FMNAMESZ+1];
#	endif	/* SYSVREL >= 4 */
	/*
	**	Some SYSTEM xx don't do system calls to stack very well
	**	(part of this comment has been censored)
	**	and hence the need for the following ``static''.
	*/
	static struct termio	params;

	Trace5(1, "SetRaw speed=%d, minch=%d, mintim=%d, xon_xoff=%d", speed, minchars, mintime, xon_xoff);

	if ( ioctl(fd, TCGETA, &params) == SYSERROR )
	{
		speed = SYSERROR;
#		if	SYSVREL >= 4
		goto out_sys4;
#		else	/* SYSVREL >= 4 */
		goto out;
#		endif	/* SYSVREL >= 4 */
	}

	params.c_iflag &= ~(ISTRIP|INLCR|ICRNL|IGNCR|IXON|IXOFF|INPCK|BRKINT|PARMRK
#	ifdef	IUCLC
			|IUCLC
#	endif	/* IUCLC */
#	ifdef	IXANY
			|IXANY
#	endif	/* IXANY */
			);
	params.c_iflag |= IGNBRK|IGNPAR;
	if ( xon_xoff )
		params.c_iflag |= IXON|IXOFF;

	params.c_oflag &= ~OPOST;

	if ( speed == 0 )
		speed = params.c_cflag & CBAUD;

	params.c_cflag &= ~(PARENB|CBAUD);
	params.c_cflag |= (speed&CBAUD)|CREAD|CS8;

	params.c_lflag = 0;

	params.c_cc[VMIN] = minchars == 0 ? 1 : minchars;
	params.c_cc[VTIME] = mintime;

	if ( ioctl(fd, TCSETA, &params) == SYSERROR )
		Syserror("SetRaw(TCSETAF)");

#	if	SYSVREL >= 4
out_sys4:
	while ( ioctl(fd, I_LOOK, lookbuf) != SYSERROR )
	{
		Trace2(2, "found streams module %s", lookbuf);
		if ( strcmp(lookbuf, "ttcompat") != 0 && strcmp(lookbuf, "ldterm") != 0 )
			break;
		Trace2(1, "popping streams module %s", lookbuf);
		if ( ioctl(fd, I_POP, 0) == SYSERROR )
		{
			(void)SysWarn("pop stream module %s", lookbuf);
			break;
		}
	}
#	endif	/* SYSVREL >= 4 */
#	endif	/* SYSTEM < 3 */

out:	Trace2(1, "SetRaw returns %d", speed);

	return speed;
}
