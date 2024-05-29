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

#ifdef	TERMCAP
#include	<termcap.h>
#else	/* TERMCAP */
#include	<curses.h>
#endif	/* TERMCAP */

extern	int tputs(const char *str, int affcnt, int (*putc)(int));
/* extern	int tgetnum(char *id);	- now defined in termcap.h */
extern	int tgetent(char *bp, const char *name);
/* extern	int tgetflag(char *id);	- now defined in termcap.h */

#include        <signal.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>

#define M_SIZE	200
#define C_SIZE	200
#define NUMMENU	20

typedef struct Menu
{
	char	str[M_SIZE];
	char	command[C_SIZE];
} Menu;

Menu	menu[20];
char *	head[20];
char	buf[80];

FILE *	m;
char *	term;
int	num;		/* number of menu elements */
int	numhead;	/* number of header lines */
char *	CM;
char *	CL;

#ifdef TERMCAP
extern char	PC, *BC, *UP;
#endif /* TERMCAP */

extern char *	tgoto();
extern char *	tgetstr();
extern int	outc();

char	tcapbuf[100];
char *	area = tcapbuf;

void		drawmenu(void);
void		finish(int);
void		ignsig(int);
char *		Malloc(unsigned);

int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	int	i;
	char *	sh;
	char	buf[1000];

	if ( argc != 2 )
	{
		fprintf(stderr, "usage: %s menu-file\n", argv[0]);
		exit(1);
	}
	
	if ( (term = getenv("TERM")) == (char *)0 || term[0] == '\0' )
	{
		fprintf(stderr, "%s: can't get terminal type\n", argv[0]);
		exit(1);
	}

#	ifdef TERMCAP
	if ( tgetent(buf, term) != 1 )
	{
		fprintf(stderr, "no \"termcap\" entry");
		exit(1);
	}
#	else	/* TERMCAP */
	setterm(term);
#	ifdef	CURSES_TAB_BUG
	system("stty tab3");	/* Cures some weird bug in setterm() that disables tabs */
#	endif	/* CURSES_TAB_BUG */
#	endif	/* TERMCAP */

	CM = tgetstr("cm", &area);
	CL = tgetstr("cl", &area);

	if ( (m = fopen(argv[1], "r")) == (FILE *)0 )
	{
		fprintf(stderr, "%s: cannot open %s\n", argv[0], argv[1]);
		exit(1);
	}

	signal(SIGINT, finish);

	for ( i = 0 ; i < sizeof head ; i++ )
	{
		char *	cp;
		int	l;

		if ( fgets(buf, sizeof buf, m) == NULL )
		{
			fprintf(stderr, "%s: EOF while reading header\n", argv[0]);
			exit(1);
		}
		l = strlen(buf);
		buf[l-1] = '\0';	/* Remove '\n' */
		cp = Malloc(l);
		(void)strcpy(cp, buf);
		head[i] = cp;
		if ( cp[0] == '&' )
			break;
	}
	if ( (numhead = i) == sizeof head )
	{
		fprintf(stderr, "%s: too many header lines\n", argv[0]);
		exit(1);
	}
	for ( i = 0 ; i < NUMMENU ; i++ )
	{
		if ( fgets(menu[i].str, M_SIZE, m) == NULL )
			break;
		if ( fgets(menu[i].command, C_SIZE, m) == NULL )
		{
			fprintf(stderr, "unexpected eof\n");
			exit(1);
		}
		menu[i].str[strlen(menu[i].str)-1] = '\0';
		menu[i].command[strlen(menu[i].command)-1] = '\0';
	}

	num = i;

	if ( (sh = getenv("SHELL")) == (char *)0 || sh[0] == '\0' )
		sh = "/bin/sh";

	for ( ;; )
	{
		drawmenu();
		tputs(tgoto(CM, 1, 22), 1, outc);
		fprintf(stdout, "Function (1-%d or `q'=%d) ", num, num);
		(void)fgets(buf, sizeof buf, stdin);

		if ( (i = atoi(buf)) == 0 && (buf[0]|040) == 'q' )
			i = num;

		if ( (i <= num) && (i > 0) )
		{
			ignsig(SIGINT);
			execl(sh, "sh", "-c", menu[i-1].command, (char *)0);
			perror(sh);
			finish(1);
			return 0;
		}
		else
		{
			tputs(tgoto(CM, 1, 23), 1, outc);
			printf("The number must be in the range 1 to %d\n", num);
			sleep(5); /* time to see the message */
		}
	}
}

void
drawmenu()
{
	int	i;

	tputs(CL, 1, outc);
	for ( i = 0 ; i < numhead ; i++ )
		puts(head[i]);
	for ( i = 0 ; i < num ; i++ )
		printf("\t%d. %s\n\n", i+1, menu[i].str);
}

char *
Malloc(size)
	unsigned size;
{
	char *	result;

	if ( (result = (char *)malloc(size)) == (char *)0 )
	{
		fprintf(stderr, "cannot malloc\n");
		exit(1);
	}

	return result;
}

void
finish(sig)
	int	sig;
{
	(void)outc('\n');
	_exit(sig);	
}

int
outc(c)
	int	c;
{
	return fputc(c, stdout);
}

void
ignsig(sig)
	int	sig;
{
	(void)signal(sig, ignsig);
}
