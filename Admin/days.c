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


static char	sccsid[]	= "@(#)days.c	1.5 05/12/16";

/*
**	elapsed -- output elapsed between two dates.
*/

#define	STDIO
#define	TIME

#include	"global.h"
#include	"debug.h"
#include	"Args.h"
#include	"sysexits.h"

/*
**	Parameters set from arguments.
*/

char *	Date1;
char *	Date2;
bool	Hours;
bool	Minutes;
char *	Name		= sccsid;	/* Program invoked name */
bool	Seconds;
int	Traceflag;

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(h, &Hours, 0),
	Arg_bool(m, &Minutes, 0),
	Arg_bool(s, &Seconds, 0),
	Arg_noflag(&Date1, 0, "dd/mm/yy", 0),
	Arg_noflag(&Date2, 0, "dd/mm/yy", 0),
	Arg_end
};

/*
**	Miscellaneous
*/

#define	SECSTODAY	(60*60*24)
#define	SECSTOHOUR	(60*60)
#define	SECSTOMIN	(60)



void
main(argc, argv)
	int		argc;
	char *		argv[];
{
	double		elapsed;
	long		stime;
	double		conv;
	struct tm	tm1;
	struct tm	tm2;
	static char	fmt[] = "%d/%m/%y";

	InitParams();

	DoArgs(argc, argv, Usage);

	if ( Seconds )
		conv = 1;
	else
	if ( Minutes )
		conv = SECSTOMIN;
	else
	if ( Hours )
		conv = SECSTOHOUR;
	else
		conv = SECSTODAY;

	stime = time((long *)0);
	tm1 = *localtime(&stime);
	tm2 = tm1;

	/*
	**	strptime(3) has a bug that won't convert dates more than 90 years old.
	*/

	(void)strptime(Date1, fmt, &tm1);
	(void)strptime(Date2, fmt, &tm2);

	elapsed = ((double)timelocal(&tm1) - (double)timelocal(&tm2)) / conv;

	if ( elapsed < 0 )
		elapsed = -elapsed;

	printf("%.0f\n", elapsed);

	exit(0);
}



void
finish(error)
	int	error;
{
	exit(error);
}
