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
**	Print out file time as decimal [or date] number.
*/

char		Usage[]		= "Usage: %s [-[c|d|t]] [file|number]\n";
char *		Name		= sccsid;	/* Program invoked name */


#include	<stdlib.h>
#include	<string.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<time.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<sys/stat.h>

#ifndef	O_READ
#define	O_READ	0
#endif

int		Converted;
int		DateFmt;
int		Fd;
char *		File;
int		TimeFmt;
int		TouchFmt;

#define		NULLSTR		(char *)0
#define		SYSERROR	(-1)

extern char
		*asctime(),
		*ctime(),
		*strcpy(),
		*strrchr();

extern struct tm
		*localtime();

void
		catch(),
		error(),
		finish(),
		usage(),
		warn(),
		date();

extern int	errno;



int
main(argc, argv)
	int		argc;
	register char *	argv[];
{
	register int	c;

	if ( (Name = strrchr(*argv, '/')) != NULLSTR )
		Name++;
	else
		Name = *argv;

	while ( --argc > 0 )
	{
		if ( **++argv == '-' )
		{
			while ( (c = *++*argv) )
			{
				switch ( c )
				{
				case 'c':
					TimeFmt = 1;
					continue;

				case 'd':
					DateFmt = 1;
					continue;

				case 't':
					TouchFmt = 1;
					continue;

				default:
					usage();
					exit(1);
				}
			}
		}
		else
		{
			if ( File != NULLSTR )
			{
				usage();
				exit(1);
			}

			if ( (Fd = open(*argv, O_READ)) == SYSERROR )
			{
				time_t	l;

				if ( (l = (time_t)atol(*argv)) > 0 )
				{
					(void)fprintf(stdout, "%s", ctime(&l));
					Converted = 1;
				}
				else
					error(errno, "cannot open file '%s'", *argv);
			}
			else
				File = *argv;
		}
	}

	if ( (TouchFmt + DateFmt + TimeFmt) > 1 )
	{
		usage();
		exit(1);
	}

	if ( File == NULLSTR )
	{
		if ( Converted )
			exit(0);

		Fd = 0;
		File = "stdin";
	}

	date();

	exit(0);
	return 0;
}

void
date()
{
	struct stat	statb;

	if ( fstat(Fd, &statb) == SYSERROR )
	{
		error(errno, "Can't fstat \"%s\"", File);
		return;
	}

	if ( TouchFmt || DateFmt || TimeFmt )
	{
		register struct tm *	tmp;

		tmp = localtime(&statb.st_mtime);

		if ( tmp->tm_year >= 100 )
			tmp->tm_year -= 100;	/* Take care of anno domini 2000 */

		if ( TouchFmt )
			(void)fprintf
			(
				stdout,
				"%02d%02d%02d%02d%02d\n",
				tmp->tm_mon + 1,
				tmp->tm_mday,
				tmp->tm_hour,
				tmp->tm_min,
				tmp->tm_year
			);
		else
		if ( DateFmt )
			(void)fprintf
			(
				stdout,
				"%02d%02d%02d%02d%02d%02d\n",
				tmp->tm_year,
				tmp->tm_mon + 1,
				tmp->tm_mday,
				tmp->tm_hour,
				tmp->tm_min,
				tmp->tm_sec
			);
		else
			(void)fprintf
			(
				stdout,
				"%s",
				asctime(tmp)
			);
	}
	else
		(void)fprintf(stdout, "%ld\n", (unsigned long)statb.st_mtime);
}

void
finish()
{
	(void)fflush(stdout);
}

void
_errp(mesg, s, a1, a2)
	char *	mesg;
	char *	s;
	char *	a1;
	char *	a2;
{
	(void)fflush(stdout);
	(void)fprintf(stderr, "%s: %s -- ", Name, mesg);
	(void)fprintf(stderr, s, a1, a2);
}

/*VARARGS1*/
void
warn(s, a1, a2)
	char *	s;
	char *	a1;
	char *	a2;
{
	_errp("warning", s, a1, a2);
	putc('\n', stderr);
}

/*VARARGS2*/
void
error(en, s, a1)
	register int	en;
	char *		s;
	char *		a1;
{
	_errp("error", s, a1, NULLSTR);
	if ( en )
		(void)fprintf(stderr, ": %s\n", strerror(en));
	else
		putc('\n', stderr);
	finish();
	exit(2);
}

void
usage()
{
	(void)fprintf(stderr, Usage, Name);
}

void
catch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	finish();
	exit(0);
}
