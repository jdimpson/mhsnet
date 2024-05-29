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
**	Accept `tty' connection -- routines for shell.
*/

#define	TERMIOCTL
#define	STAT_CALL

#include	"global.h"
#include	"debug.h"
#include	"Passwd.h"
#include	"sysexits.h"

#include	"setup.h"
#include	"shell.h"


#if	SYSTEM >= 3
/*
**	`Tty' type device speeds.
*/

#ifndef	B19200
#ifdef	EXTA
#define		B19200		EXTA
#endif	/* EXTA */
#endif	/* !B19200 */

#ifndef	B38400
#ifdef	EXTB
#define		B38400		EXTB
#endif	/* EXTB */
#endif	/* !B38400 */

typedef struct
{
	short	s_vmin;
	short	s_val;
}
		Speed;

static Speed	Speeds[] =
{
	{16,	B2400},
	{32,	B4800},
	{64,	B9600},
#	ifdef	B19200
	{64,	B19200},
#	endif
#	ifdef	B38400
	{64,	B38400},
#	endif
};

#define		SPDZ		(sizeof(Speed))
#define 	NSPDS		((sizeof Speeds)/SPDZ)
#endif	/* SYSTEM >= 3 */

static int	VCSpeed;
static bool	XonXoff;

static int	compare _FA_((const void *, const void *));
static char *	s_tty _FA_((bool, VCtype *));



/*
**	Condition `tty' connection.
*/

char *
a_tty(tp)
	VCtype *	tp;
{
	return s_tty(false, tp);
}



/*
**	Condition XON/XOFF `tty' connection.
*/

char *
a_ttyx(tp)
	VCtype *	tp;
{
	return s_tty(true, tp);
}



/*
**	Set VC specific params: VMIN VTIME
*/

void
a_ttyparams(params)
	char *	params;
{
#	if	SYSTEM >= 3
	char *	cp;
	int	n;
	int	vmin;
	int	vtime;
	VarArgs	va;
	Speed *	spdp;
	Speed	speed;

	NARGS(&va) = 0;

	if ( (n = SplitArg(&va, params)) == 0 )
	{
		size_t	nspds = NSPDS;

		speed.s_val = VCSpeed;

		if
		(
			(spdp = (Speed *)lfind((char *)&speed, (char *)Speeds, &nspds, SPDZ, compare))
			==
			(Speed *)0
		)
			return;		/* Default already set */

		vmin = spdp->s_vmin;
		vtime = 1;
	}
	else
	{
		if ( (cp = ARG(&va, 0)) == NULLSTR || (vmin = atoi(cp)) <= 1 )
			return;		/* Default already set */

		if ( n == 1 || (cp = ARG(&va, 1)) == NULLSTR || (vtime = atoi(cp)) < 1 )
			vtime = 1;
	}

	if ( SetRaw(1, 0, vmin, vtime, XonXoff) == SYSERROR )
		SysWarn(CouldNot, "ioctl", english("virtual circuit"));
	else
		Trace3(1, "Circuit read params: VMIN/VTIME = %d/%d", vmin, vtime);
#	endif	/* SYSTEM >= 3 */
}



#if	SYSTEM >= 3
static int
compare(p1, p2)
	const void *	p1;
	const void *	p2;
{
	return ((Speed *)p2)->s_val - ((Speed *)p1)->s_val;
}
#endif	/* SYSTEM >= 3 */



/*
**	Condition `tty' connection (common code.)
*/

static char *
s_tty(xonxoff, tp)
	bool		xonxoff;
	VCtype *	tp;
{
	Passwd		pw;
	char *		tty;
	extern char *	ttyname();
	extern char *	getlogin();

	VCSpeed = SetRaw(1, 0, 1, 0, xonxoff);
	XonXoff = xonxoff;

	if ( (tty = ttyname(1)) != NULLSTR )
	{
		(void)chmod(tty, 0600);	/* Turn off "write" permission */
		if ( strncmp(tty, DevNull, 5) == STREQUAL )
			tty += 5;
		tp->VCname = newstr(tty);
	}

	if ( (pw.P_name = getlogin()) == NULLSTR && !GetUser(&pw, getuid()) )
	{
		Error(english("passwd file error \"%s\" for uid %d"), pw.P_error, pw.P_uid);
		finish(EX_NOUSER);
	}

	return newstr(pw.P_name);
}
