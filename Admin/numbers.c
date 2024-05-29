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


static char	sccsid[]	= "@(#)numbers.c	1.6 05/12/16";

/*
**	numbers -- output numbers in range.
*/

#define	SIGNALS
#define	STDIO

#include	"global.h"
#include	"debug.h"
#include	"Args.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters set from arguments.
*/

char *	Name		= sccsid;	/* Program invoked name */
int	Number0;
int	Number1;
int	Step		= 1;
int	Traceflag;

AFuncv	getRange _FA_((PassVal, Pointer, char *));

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_macro(NONFLAG, &Number0, getRange, "start", INT, 0),
	Arg_macro(NONFLAG, &Number1, 0, "stop", INT, OPTARG),
	Arg_macro(NONFLAG, &Step, 0, "step", INT, OPTARG),
	Arg_end
};

/*
**	Miscellaneous
*/



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register int	i;
	register int	j;

	InitParams();

	DoArgs(argc, argv, Usage);

	if ( (j = Number1) == 0 )
		j = MAX_INT;

	for ( i = Number0 ; i <= j ; i += Step )
		printf("%d\n", i);

	exit(0);
	return 0;
}



/*
**	Detect ranges.
*/

AFuncv
getRange(value, argp, string)
	PassVal			value;
	Pointer			argp;
	char *			string;
{
	register char *		cp;

	/** [0-9]+`-'[0-9]+ ? **/

	if ( (cp = strchr(string, '-')) != NULLSTR )
	{
		*cp++ = '\0';
		Number1 = atol(cp);
	}

	return ACCEPTARG;
}



void
finish(error)
	int	error;
{
	exit(error);
}
