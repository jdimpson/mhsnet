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
**	Test result of fstat on non-empty pipe,
**	and produce #define for "site.h".
*/

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<unistd.h>


#define	BufSize			100

char	Buf[BufSize];
char	Ofmt[]			= "#define	PIPE_FSTAT_SIZE	%d\n";
int	Pfd[2];
#define	READ	0
#define	WRITE	1

#define	english(A)		A

char	CouldNot[]		= english("Could not %s \"%s\"");
char	EmptyString[]		= "";
char	ForkStr[]		= english("fork");
char	FstatStr[]		= english("fstat");
char	OpenStr[]		= english("open");
char	PipeStr[]		= english("pipe");

void	syserror();
#define	SYSERROR	(-1)
#define	MINSLEEP	2


int
main()
{
	struct stat	fstatb;

	while ( pipe(Pfd) == SYSERROR )
		syserror(CouldNot, OpenStr, PipeStr);

	for ( ;; )
	{
		switch ( fork() )
		{
		case 0:		(void)write(Pfd[WRITE], Buf, BufSize);		exit(0);
		case SYSERROR:	syserror(CouldNot, ForkStr, EmptyString);	continue;
		}

		break;
	}

	(void)sleep(MINSLEEP);

	while ( fstat(Pfd[READ], &fstatb) == SYSERROR )
		syserror(CouldNot, FstatStr, PipeStr);

	(void)printf(Ofmt, (fstatb.st_size == BufSize)?1:0);

	exit(0);
	return 0;
}

void
syserror(f, a, b)
	char		*f, *a, *b;
{
	extern int	errno;

	(void)fprintf(stderr, f, a, b);

	switch ( errno )
	{
	case ENFILE:
	case ENOSPC:
	case EAGAIN:
		(void)sleep(MINSLEEP);
		return;
	}

	(void)printf(Ofmt, 0);
	exit(1);
}
