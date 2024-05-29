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


#define	FILE_CONTROL
#define	STAT_CALL
#define	TERMIOCTL
#define	SIGNALS
#define	STDIO
#define	ERRNO

#include	"global.h"
#include	"debug.h"

#include	"call.h"
#include	"setup.h"

#include	<setjmp.h>

#if	SYSTEM >= 3

static struct termio	ttyi;

#else	/* SYSTEM >= 3 */

#if	BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c')
#ifdef	LNOMDM
int			lflag = LNOHANG | LNOMDM;
#else	/* LNOMDM */
int			lflag = LNOHANG;
#endif	/* LNOMDM */
#endif	/* BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c') */

static struct sgttyb	ttyi;

#endif	/* SYSTEM >= 3 */

char *		UUCPlockfile;	/* Used in Libc/uucplock.c if present */

static int	locktty _FA_((char *));
static int	opentty _FA_((char *, VarArgs *));
static char *	tty_close _FA_((void));
static char *	tty_err _FA_((char *, char *));
static char *	tty_open _FA_((VarArgs *));
static void	unlocktty _FA_((void));

extern jmp_buf	Alrm_jmp;

extern int	mlock _FA_((char *));
extern void	rmlock _FA_((char *));



/*
**	Control operations via simple ``tty'' circuit.
*/

char *
f_tty(dev_op, va)
	DevOp		dev_op;
	VarArgs *	va;
{
	Reason = DEVFAIL;

	switch ( dev_op )
	{
	case t_device:
		return tty_device(va);

	case t_open:
		return tty_open(va);

	case t_close:
		return tty_close();
	}

	Fatal2(english("unknown dev_op %d"), dev_op);

	return Reason;
}



/*
**	``close'' for ``tty'' type circuit.
*/

static char *
tty_close()
{
	if ( close(Fd) == SYSERROR )
		Reason = ErrnoStr(errno);
	else
		Reason = DEVOK;

	Fd = SYSERROR;
	FreeStr(&Fn);

	unlocktty();

	return Reason;
}



/*
**	``device'' operations on ``tty'' type circuit.
*/

char *
tty_device(va)
	VarArgs *	va;
{
	register char *	cp;
	static char	cantioctl[]	= english("Can't ioctl(%s) device: %s");
	static char	devneedsa[]	= english("device \"%s\" operation needs one argument");

	if ( NARGS(va) == 0 || (cp = ARG(va, 0)) == NULLSTR || cp[0] == '\0' )
	{
		Error(english("device control?"));
		return DEVFAIL;
	}

#	ifdef	O_NDELAY
	if ( strcmp(cp, "offdelay") == STREQUAL )
	{
		int	val;

		/* turn off O_NDELAY for line */
		Ondelay = 0;
		if ( fcntl(Fd, F_SETFL, val = fcntl(Fd, F_GETFL, 0) & ~O_NDELAY) == SYSERROR )
			return ErrnoStr(errno);
		Trace2(1, "fcntl flags = 0%o", val);
		return DEVOK;
	}

	if ( strcmp(cp, "ondelay") == STREQUAL )
	{
		int	val;

		/* turn on O_NDELAY for line */
		Ondelay = O_NDELAY;
		if ( fcntl(Fd, F_SETFL, val = fcntl(Fd, F_GETFL, 0) | O_NDELAY) == SYSERROR )
			return ErrnoStr(errno);
		Trace2(1, "fcntl flags = 0%o", val);
		return DEVOK;
	}
#	endif	/* O_NDELAY */

#	if	SYSTEM >= 3
	if ( strcmp(cp, "flush") == STREQUAL )
	{
		/* flush both input and output */
		if ( ioctl(Fd, TCXONC, 1) == SYSERROR )
			return tty_err(cantioctl, "TCXONC");
		if ( ioctl(Fd, TCFLSH, 2) == SYSERROR )
			return tty_err(cantioctl, "TCFLSH");
		return DEVOK;
	}
	
	if ( strcmp(cp, "local") == STREQUAL )
	{
		static struct termio	ttyi;

		if ( ioctl(Fd, TCGETA, &ttyi) == SYSERROR )
			return tty_err(cantioctl, "TCGETA");
		ttyi.c_cflag |= CLOCAL;
		if ( ioctl(Fd, TCSETA, &ttyi) == SYSERROR )
			return tty_err(cantioctl, "TCSETA");
#		if	CARR_ON_BUG == 1
		/*
		 * This fixes a bug on some SYSTEM xx.
		 * Driver sets CARR_ON on open or on modem DCD
		 * becoming high but not on above ioctl!
		 */
		(void)close(open(Fn, O_RDWR, 0));
#		endif	/* CARR_ON_BUG == 1 */
		return DEVOK;
	}

	if ( strcmp(cp, "remote") == STREQUAL )
	{
		static struct termio	ttyi;

		if ( ioctl(Fd, TCGETA, &ttyi) == SYSERROR )
			return tty_err(cantioctl, "TCGETA");
		ttyi.c_cflag &= ~CLOCAL;
		if ( ioctl(Fd, TCSETA, &ttyi) == SYSERROR )
			return tty_err(cantioctl, "TCSETA");
		return DEVOK;
	}

	if ( strcmp(cp, "break") == STREQUAL )
	{
		if ( ioctl(Fd, TCSBRK, 0) == SYSERROR )
			return tty_err(cantioctl, "TCSBRK");
		return DEVOK;
	}

	if ( strcmp(cp, "cook") == STREQUAL )
	{
		if ( ioctl(Fd, TCSETA, &ttyi) == SYSERROR )
			return tty_err(cantioctl, "TCSETA");
		Cookflag = true;
		return DEVOK;
	}

#	else	/* SYSTEM >= 3 */

	if ( strcmp(cp, "cook") == STREQUAL )
	{
		if ( ioctl(Fd, TIOCSETP, &ttyi) == SYSERROR )
			return tty_err(cantioctl, "TIOCSETP");
		Cookflag = true;
		return DEVOK;
	}
#	endif	/* SYSTEM >= 3 */

#	if	BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c')
	if ( strcmp(cp, "flush") == STREQUAL )
	{
		int	rw = 0;	/* flush both input and output */

		if ( ioctl(Fd, TIOCFLUSH, &rw) == SYSERROR )
			return tty_err(cantioctl, "TIOCFLUSH");
		return DEVOK;
	}

	if ( strcmp(cp, "local") == STREQUAL )
	{
		if ( ioctl(Fd, TIOCLBIS, &lflag) == SYSERROR )
			return tty_err(cantioctl, "TIOCLBIS");
		return DEVOK;
	}

	if ( strcmp(cp, "remote") == STREQUAL )
	{
		if ( ioctl(Fd, TIOCLBIC, &lflag) == SYSERROR )
			return tty_err(cantioctl, "TIOCLBIC");
		return DEVOK;
	}

	if ( strcmp(cp, "break") == STREQUAL )
	{
		if ( ioctl(Fd, TIOCSBRK, (char *)0) == SYSERROR )
			return tty_err(cantioctl, "TIOCSBRK");

		(void)sleep(MINSLEEP);	/* Big break */

		if ( ioctl(Fd, TIOCCBRK, (char *)0) == SYSERROR )
			return tty_err(cantioctl, "TIOCCBRK");

		return DEVOK;
	}
#	endif	/* BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c') */

	if ( strcmp(cp, "raw") == STREQUAL )
	{
		int	vmin;
		int	vtime;

		if ( (cp = ARG(va, 1)) != NULLSTR && (vmin = atoi(cp)) > 1 )
		{
			if ( (cp = ARG(va, 2)) == NULLSTR || (vtime = atoi(cp)) < 1 )
				vtime = 1;
		}
		else
		{
			vmin = 1;
			vtime = 0;
		}

		if ( SetRaw(Fd, 0, vmin, vtime, XonXoff) == SYSERROR )
			return ErrnoStr(errno);

		Cookflag = false;
		return DEVOK;
	}

	if ( strcmp(cp, "speed") == STREQUAL )
	{
		int	speed;

		if ( (cp = ARG(va, 1)) == NULLSTR )
		{
			Error(devneedsa, english("speed"));
			return DEVFAIL;
		}

		if ( (speed = do_speed(cp)) == SYSERROR )
			return ErrnoStr(errno);

#		if	SYSTEM >= 3
		ttyi.c_cflag = (ttyi.c_cflag & ~CBAUD) | speed;
#		else	/* SYSTEM >= 3 */
		ttyi.sg_ispeed = ttyi.sg_ospeed = speed;
#		endif	/* SYSTEM >= 3 */
		return DEVOK;
	}

	if ( strcmp(cp, "stty") == STREQUAL )
	{
		if ( (cp = ARG(va, 1)) == NULLSTR )
		{
			Error(devneedsa, english("stty"));
			return DEVFAIL;
		}

		return do_stty(cp);
	}

	if ( strcmp(cp, "xonxoff") == STREQUAL )
	{
		XonXoff = true;

		if ( SetRaw(Fd, 0, 1, 0, XonXoff) == 0 )
			return ErrnoStr(errno);

		return DEVOK;
	}

	Warn(english("unimplemented command \"%s\""), cp);
	return DEVFAIL;
}



static char *
tty_open(va)
	VarArgs *	va;
{
	static char *	device;
	static int	retries;
	static SigFunc *old_al_sig;

	if ( NARGS(va) == 0 || (device = ARG(va, 0)) == NULLSTR || device[0] == '\0' )
	{
		Error(english("device name?"));
		return Reason;
	}

	retries = Retries;
	old_al_sig = (SigFunc *)signal(SIGALRM, alrmcatch);

	do
	{
		if ( Finish )
			break;

		if ( setjmp(Alrm_jmp) )
		{
			Reason = TIMEOUT;
			continue;
		}

		(void)alarm(TimeOut);

		if ( (Fd = opentty(device, va)) != SYSERROR )
		{
			(void)alarm((unsigned)0);
			(void)signal(SIGALRM, old_al_sig);

			FreeStr(&Fn);
			Fn = newstr(device);

			(void)SetRaw(Fd, 0, 1, 0, false);

			VCtype = BYTESTREAM;

			return DEVOK;
		}

		Reason = ErrnoStr(errno);

		(void)alarm((unsigned)0);

		Trace3(2, CouldNot, OpenStr, device);
		Trace2(2, "Reason: %s", Reason);
	}
	while
		( !Finish && Fd == SYSERROR && --retries >= 0 && sleep(TimeOut) != SYSERROR );

	(void)signal(SIGALRM, old_al_sig);
	return Reason;
}



static int
locktty(device)
	char *		device;
{
	register char *	l;
	int		omask;

	if ( !UUCPLOCKS || UUCPlocked )
		return 0;

	if ( strncmp(device, DevNull, 5) == STREQUAL )
		l = device+5;
	else
		l = device;

	Trace2(1, "locking \"%s\"", l);

	omask = umask(022);

	if ( mlock(l) != 0 )
	{
		MesgN("open ", english("%s locked by UUCP"), (UUCPlockfile==NULLSTR)?l:UUCPlockfile);

		(void)umask(omask);

		errno = EBUSY;
		return SYSERROR;
	}

	(void)umask(omask);

	UUCPlocked = 1;

	return 0;
}



static void
unlocktty()
{
	int	save;

	if ( !UUCPlocked )
		return;

	save = errno;

	rmlock(NULLSTR);
	UUCPlocked = 0;

	errno = save;
}



/*
**	``open'' for ``tty'' type circuits.
*/

#if	SYSTEM >= 3
int
opentty(name, va)
	char *			name;
	VarArgs *		va;
{
	register int		fd;
	register int		i;
	int			cflag = CS8 | CREAD | HUPCL;
 	static struct termio	ttyj;

	XonXoff = false;
	Cookflag = false;
	Ondelay = 0;

	for ( i = NARGS(va), fd = 1 ; i >= 2 ; fd++, i-- )
	{
		char *	param = ARG(va, fd);

#		ifdef	O_NDELAY
		if ( strcmp(param, "offdelay") == STREQUAL )
			Ondelay = 0;		/* turn off O_NDELAY for line */
		else
		if ( strcmp(param, "ondelay") == STREQUAL )
			Ondelay = O_NDELAY;	/* turn on O_NDELAY for line */
		else
#		endif	/* O_NDELAY */
		if ( strcmp(param, "local") == STREQUAL )
			cflag |= CLOCAL;
		else
		if ( strcmp(param, "remote") == STREQUAL )
			cflag &= ~CLOCAL;
		else
		if ( strcmp(param, "uucplock") == STREQUAL )
			if ( locktty(name) == SYSERROR )
				return SYSERROR;
	}

	if ( (fd = open(name, Ondelay|O_RDWR)) == SYSERROR )
	{
		unlocktty();
		return fd;
	}

	if ( ioctl(fd, TCGETA, &ttyi) == SYSERROR )
	{
		unlocktty();
		(void)close(fd);
		return SYSERROR;
	}

	ttyj = ttyi;

	ttyj.c_cflag = cflag | (ttyi.c_cflag & CBAUD);
	ttyj.c_iflag |= IGNBRK|IGNPAR;
	ttyj.c_lflag |= NOFLSH;

	(void)ioctl(fd, TCSETA, &ttyj);

#	if	CARR_ON_BUG == 1
	if ( cflag & CLOCAL )
		/*
		**	This fixes a bug on some SYSTEM xx. Driver sets CARR_ON on open
		**	or on modem DCD becoming high but not on the above ioctl!
		*/
		(void)close(open(name, O_RDWR, 0));
#	endif	/* CARR_ON_BUG == 1 */

	Trace3(1, "opentty(%s) -> %d", name, fd);

	return fd;
}
#else	/* SYSTEM >= 3 */



#if	BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c')
int
opentty(name, va)
	char *		name;
	VarArgs *	va;
{
	register int	fd;
	register int	i;
	bool		ismodem = true;
	int		dflag;

	XonXoff = false;
	Cookflag = false;
	Ondelay = 0;

	for ( i = NARGS(va), fd = 1 ; i >= 2 ; fd++, i-- )
	{
		char *	param = ARG(va, fd);

#		ifdef	O_NDELAY
		if ( strcmp(param, "offdelay") == STREQUAL )
			Ondelay = 0;		/* turn off O_NDELAY for line */
		else
		if ( strcmp(param, "ondelay") == STREQUAL )
			Ondelay = O_NDELAY;	/* turn on O_NDELAY for line */
		else
#		endif	/* O_NDELAY */
		if ( strcmp(param, "local") == STREQUAL )
			ismodem = false;
		else
		if ( strcmp(param, "remote") == STREQUAL )
			ismodem = true;
		else
		if ( strcmp(param, "uucplock") == STREQUAL )
			if ( locktty(name) == SYSERROR )
				return SYSERROR;
	}

	if ( (fd = open(name, Ondelay|O_RDWR)) == SYSERROR )
	{
		unlocktty();
		return fd;
	}

	(void)ioctl(fd, TIOCEXCL, (char *)0);

#	ifdef	TIOCSCTTY
        (void)ioctl(fd, TIOCSCTTY, (char *)0);
#	endif	/* TIOCSCTTY */

	if ( ioctl(fd, TIOCGETP, &ttyi) == SYSERROR )
	{
		unlocktty();
		(void)close(fd);
		return SYSERROR;
	}

	dflag = OTTYDISC;
	(void)ioctl(fd, TIOCSETD, &dflag);

	(void)ioctl(fd, TIOCHPCL, (char *)0);

	if ( ismodem )
		(void)ioctl(fd, TIOCLBIC, &lflag);
	else
		(void)ioctl(fd, TIOCLSET, &lflag);

	DODEBUG((void)ioctl(fd, TIOCLGET, &dflag));
	Trace4(1, "opentty(%s) -> %d, lflg=0%o", name, fd, dflag);

	return fd;
}
#else	/* BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c') */



int
opentty(name, va)
	char *		name;
	VarArgs *	va;
{
	register int	fd;
	register int	i;

	XonXoff = false;
	Cookflag = false;
	Ondelay = 0;

	for ( i = NARGS(va), fd = 1 ; i >= 2 ; fd++, i-- )
	{
		char *	param = ARG(va, fd);

#		ifdef	O_NDELAY
		if ( strcmp(param, "offdelay") == STREQUAL )
			Ondelay = 0;		/* turn off O_NDELAY for line */
		else
		if ( strcmp(param, "ondelay") == STREQUAL )
			Ondelay = O_NDELAY;	/* turn on O_NDELAY for line */
		else
#		endif	/* O_NDELAY */
		if ( strcmp(param, "uucplock") == STREQUAL )
			if ( locktty(name) == SYSERROR )
				return SYSERROR;
	}

	if ( (fd = open(name, O_RDWR)) == SYSERROR )
	{
		unlocktty();
		return fd;
	}

#	ifdef	TIOCEXCL
	(void)ioctl(fd, TIOCEXCL, (char *)0);
#	endif	/* TIOCEXCL */

	if ( ioctl(fd, TIOCGETP, &ttyi) == SYSERROR )
	{
		unlocktty();
		(void)close(fd);
		return SYSERROR;
	}

	Trace3(1, "opentty(%s) -> %d", name, fd);

	return fd;
}
#endif	/* BSD4 > 1 || (BSD4 == 1 && BSD4V >= 'c') */
#endif	/* SYSTEM >= 3 */


static char *
tty_err(desc, type)
	char *		desc;
	char *		type;
{
	static char *	err_str;

	FreeStr(&err_str);
	err_str = newprintf(desc, type, ErrnoStr(errno));
	return err_str;
}
