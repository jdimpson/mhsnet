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


static char	sccsid[]	= "%W% %E%";

/*
**	Dis -- VDU page display program.
*/

#define	RECOVER
#define	SIGNALS
#define	STDIO

#ifdef	TERMCAP
#define	TERMIOCTL
#endif	/* TERMCAP */

#include	"global.h"
#include	"Args.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */

#ifdef	TERMCAP
#include	<termcap.h>
#else	/* TERMCAP */
#include	<curses.h>
#endif	/* TERMCAP */

extern	int tputs(const char *str, int affcnt, int (*putc)(int));
/* extern	int tgetnum(char *id); - defined in termcap.h */
extern	int tgetent(char *bp, const char *name);
/* extern	int tgetflag(char *id); - defined in termcap.h */


/*
**	Parameters.
*/

#define		MAXWID		500
#define		MAXLEN		200

/*
**	Parameters set from arguments.
*/

int		rcount;			/* Refresh count */
unsigned int	page_timeout;		/* Timeout between pages */
char *		Name	= sccsid;	/* Program invoked name */
bool		Nobuf;			/* Turn off output buffer */

AFuncv	getNobuf _FA_((PassVal, Pointer));	/* Turn off input buffer (?!?) */
AFuncv	getRefresh _FA_((PassVal, Pointer));	/* Get refresh count (default) */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(b, &Nobuf, 0),
	Arg_bool(u, 0, getNobuf),
	Arg_int(c, &rcount, getRefresh, "refresh", OPTARG|OPTVAL),
	Arg_int(t, &page_timeout, getInt1, "page_timeout", OPTARG|OPTVAL),
	Arg_end
};

/*
**	Screen buffer
*/

char *		Buf;

/*
**	Term Cap
*/

char		*CM, *CL;
char		*TI, *TE;
short		amflag;

#ifdef	TERMCAP
extern short	ospeed;
extern char	PC, *BC, *UP;
#endif	/* TERMCAP */

extern char *	tgoto();
extern char *	tgetstr();

/*
**	Screen macro
*/

#define		putcm(cp,p,c)		if ( *cp++ != c ) \
					{ \
						if ( Move((--cp)-p) ) \
						{ \
							putc(c,stdout); \
							Flush = 1; \
						} \
						*cp++ = c; \
					}

/*
**	Miscellaneous
*/

jmp_buf		alrmbuf;
short		Flush;
short		length;
short		Length;		/* length - 1 */
int		Traceflag;
short		width;
short		Width;		/* width - 1 */

void		dis _FA_((unsigned));
void		tcinit _FA_((void));
void		terror _FA_((char *));
void		warn _FA_((char *));

int		Move _FA_((int));
int		outc _FA_((int));

extern SigFunc	alrmcatch;



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register int	size;

	InitParams();

	DoArgs(argc, argv, Usage);

	setbuf(stdout, Nobuf?NULLSTR:(char *)SObuf);

	tcinit();

	if ( (SigFunc *)signal(SIGINT, SIG_IGN) != SIG_IGN )
		(void)signal(SIGINT, finish);

	size = length * width;
	Buf = Malloc0(size);

	Length = length - 1;
	Width = width - 1;

	dis(size);

	finish(0);
	return 0;
}



void
dis(size)
	unsigned		size;
{
	register char *		ep;
	register int		c;
	register int		line;
	/** static to avoid setjmp **/
	static char *		p;
	static char *		cp;
	static char *		lastend;
	static int		rc;

	p = Buf;
	lastend = p;
	rc = rcount;

	do
	{
		line = 0;
		cp = p;
		ep = cp+width;
		c = 0;

		if ( rcount && --rc == 0 )
		{
			tputs(CL, 1, outc);
			for (c = size; c > 0; --c, *p++ = '\0');
			p = cp;
			rc = rcount;
		}

		if ( page_timeout == 0 || setjmp(alrmbuf) == 0 )
		{
			if ( page_timeout )
			{
				signal(SIGALRM, alrmcatch);
				alarm(page_timeout);
			}
			while ( (c = getchar()) != EOF )
			{
				if ( c < ' ' )
				{
					switch ( c )
					{
					 case '\f':	if ( cp != p )
								break;
							continue;
					 case '\t':	if ( cp >= ep )
								continue;
							c = cp - &p[line*width] + 1;
							putcm(cp, p, ' ');
							while ( c++&7 )
								putcm(cp, p, ' ');
							continue;
					 case '\n':	while ( cp < ep )
								putcm(cp, p, ' ');
							if ( line < Length )
							{
								cp = &p[(++line)*width];
								ep = cp+width;
							}
							continue;
					 default:
							if ( cp < ep )
								putcm(cp, p, '?');
							continue;
					}
				}
				else
				{
					if ( cp < ep )
						putcm(cp, p, c);
					continue;
				}
				break;
			}
			if ( page_timeout )
				alarm(0);
		}

		if ( !Flush )
			continue;
		Flush = 0;

		ep = cp - 1;
		while ( cp < lastend )
			putcm(cp, p, ' ');
		lastend = ep;
		if ( (line = (ep-p)/width) < Length )
			line++;
		(void)Move(line*width);

		fflush(stdout);
	}
	while
		( c != EOF );
}



void
finish(error)
	int	error;
{
	if ( TE != NULLSTR )
		tputs(TE, 1, outc);

	(void)fflush(stdout);

	exit(error);
}



int
Move(pos)
	register int	pos;
{
	register int	x = pos%width;
	register int	y = pos/width;
	register int	i;
	static int	oy, ox = -1;

	if ( oy == y )
	{
		if ( (i = x - ox) != 1 )
		{
			if ( i <= 3 && i > 0 )
			{
				i--;
				pos -= i;
				do
					putc(Buf[pos++], stdout);
				while
					( --i > 0 );
			}
			else
				tputs(tgoto(CM, x, y), 1, outc);

			Flush = 1;
		}
	}
	else
	{
		if ( oy == (y-1) && x == 0 )
		{
			if ( ox != Width || !amflag )
				putc('\n', stdout);
		}
		else
			tputs(tgoto(CM, x, y), oy<y?y-oy:oy-y, outc);

		Flush = 1;
	}

	ox = x; oy = y;
	if ( y==Length && x==Width && amflag )
		return 0;
	return 1;
}



void
alrmcatch(sig)
	int	sig;
{
	longjmp(alrmbuf, 1);
}



/*ARGSUSED*/
AFuncv
getNobuf(val, arg)
	PassVal		val;
	Pointer		arg;
{
	setbuf(stdin, NULL);
	return ACCEPTARG;
}



/*ARGSUSED*/
AFuncv
getRefresh(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.l == 0 )
		rcount = 10;	/* Default */

	return ACCEPTARG;
}



void
tcinit()
{
	register char *		cp;
#	ifdef	TERMCAP
#	if	SYSTEM < 3
	struct sgttyb		gb;
#	else	/* SYSTEM < 3 */
	static struct termio	gb;
#	endif	/* SYSTEM < 3 */
#	endif	/* TERMCAP */
	char			bp[1024];
	static char		buf[100];
	char			*area = buf;

	if ( (cp = getenv("TERM")) == (char *)0 || cp[0] == '\0' )
		terror("no \"TERM\" environment variable");
#	ifdef TERMCAP
	if ( tgetent(bp, cp) != 1 )
		terror("no \"termcap\" entry");
#	else	/* TERMCAP */
	setterm(cp);
#	ifdef	CURSES_TAB_BUG
	system("stty tab3");	/* Cures some weird bug in setterm() that disables tabs */
#	endif	/* CURSES_TAB_BUG */
#	endif	/* TERMCAP */
	if ( (CL = tgetstr("cl", &area)) == NULLSTR )
		terror("not VDU");
	if ( (CM = tgetstr("cm", &area)) == NULLSTR )
		terror("no cursor addressing");
#	ifdef	TERMCAP
	UP = tgetstr("up", &area);
	BC = tgetstr("bc", &area);
#	endif	/* TERMCAP */
	TI = tgetstr("ti", &area);
	TE = tgetstr("te", &area);
	if ( tgetflag("am") == 1 )
		amflag++;
	if ( (cp = getenv("COLUMNS")) != NULLSTR && cp[0] != '\0' )
		width = atoi(cp);
	else
	if ( (cp = getenv("WIDTH")) != NULLSTR && cp[0] != '\0' )
		width = atoi(cp);
	else
		width = tgetnum("co");
	if ( width > MAXWID )
	{
		width = MAXWID;
		warn("width truncated");
	}
	if ( (cp = getenv("LINES")) != NULLSTR && cp[0] != '\0' )
		length = atoi(cp);
	else
		length = tgetnum("li");
	if ( length > MAXLEN )
	{
		length = MAXLEN;
		warn("length truncated");
	}

#	ifdef	TERMCAP
	if ( (cp = tgetstr("pc", &area)) != NULLSTR )
		PC = *cp;
#	if	SYSTEM < 3
	(void)gtty(1, &gb);
	ospeed = gb.sg_ospeed;
#	else	/* SYSTEM < 3 */
	(void)ioctl(1, TCGETA, &gb);
	ospeed = gb.c_cflag & CBAUD;
#	endif	/* SYSTEM < 3 */
#	endif	/* TERMCAP */

	if ( TI != NULLSTR )
		tputs(TI, 1, outc);

	tputs(CL, 1, outc);
}



int
#ifdef	ANSI_C
outc(int c)
#else	/* ANSI_C */
outc(c)
	int	c;
#endif	/* ANSI_C */
{
	return putc(c, stdout);
}



void
warn(s)
	char *	s;
{
	fprintf(stderr, "Warning: %s\n", s);
	sleep(2);
}



void
terror(s)
	char *	s;
{
	fprintf(stderr, "Terminal capability error - %s\n", s);
	exit(1);
}
