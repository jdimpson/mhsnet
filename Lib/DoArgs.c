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
**	Default argument parsing.
**
**	Errors print on stderr and "finish(EX_USAGE)".
*/

#define	STDIO

#include	"global.h"
#include	"Args.h"
#include	"sysexits.h"

bool		NoArgsUsage;


/*
**	Call EvalArgs with default program name and error processing
*/

void
DoArgs(argc, argv, args)
	int	argc;
	char *	argv[];
	Args *	args;
{
	if ( EvalArgs(argc, argv, args, argerror) < 0 )
		usagerror(NoArgsUsage?NULLSTR:ArgsUsage(args));
}

/*ARGSUSED*/
AFuncv
getName(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	register char **cpp;

	if ( (cpp = (char **)arg) == (char **)0 )
		cpp = &Name;

	if ( (*cpp = strrchr(val.p, '/')) != NULLSTR )
		(*cpp)++;
	else
	{
		if ( val.p[0] == '-' )
			++val.p;
		*cpp = val.p;
	}

	return ACCEPTARG;
}

AFuncv
getInt1(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	register int *	ip;

	if ( (ip = (int *)arg) == (int *)0 )
		return (AFuncv)english("missing variable address");

	if ( (*ip = val.l) == 0 && str[0] != '0' )
		*ip = 1;

	return ACCEPTARG;
}

void
usagerror(s)
	char *	s;
{
	if ( s != NULLSTR )
		(void)fprintf(stderr, "%s\n", s);
	finish(EX_USAGE);
}

int
argerror(s)
	char *	s;
{
	(void)fprintf(stderr, "%s: argument error: %s\n", Name, s);
	return 0;
}
