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


static char	sccsid[]	= "@(#)randnum.c	1.4 05/12/16";

/*
**	Random -- produce random number in given range, default 1-1000.
*/

#define	STDIO

#include	"global.h"
#include	"Args.h"

/*
**	Parameters.
*/

#define		LORANGE		0
#define		HIRANGE		1000

/*
**	Parameters set from arguments.
*/

int	Bucket		= 1;		/* How many items per test bucket */
int	HiRange		= HIRANGE;
int	LoRange		= LORANGE;
char *	Name		= sccsid;	/* Program invoked name */
bool	RetRand;			/* Return result as exit code */
bool	TestRand;			/* Test random number generator */
int	Traceflag;

AFuncv	getSeed _FA_((PassVal, Pointer, char *));

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(x, &RetRand, 0),
	Arg_bool(t, &TestRand, 0),
	Arg_int(C, &Bucket, getInt1, "count", OPTARG|OPTVAL),
	Arg_int(S, 0, getSeed, "seed", OPTARG|OPTVAL),
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_macro(NONFLAG, &HiRange, 0, "hi-range [1000]", INT, OPTARG),
	Arg_macro(NONFLAG, &LoRange, 0, "lo-range [0]", INT, OPTARG),
	Arg_end
};

/*
**	Miscellaneous
*/

void	randrand _FA_((void));
void	testrand _FA_((void));




main(argc, argv)
	int		argc;
	char *		argv[];
{
	randrand();

	InitParams();

	DoArgs(argc, argv, Usage);

	if ( LoRange >= HiRange )
		Error(english("range error: %d >= %d"), LoRange, HiRange);

	if ( TestRand )
		testrand();
	else
	if ( RetRand )
		exit(dorand()&0177);
	else
		printf("%d\n", dorand());

	exit(0);
}



int
dorand()
{
	return ((Rand()>>12) % (HiRange+1-LoRange)) + LoRange;
}



void
finish(error)
	int	error;
{
	exit(error);
}



/*ARGSUSED*/
AFuncv
getSeed(val, arg)
	PassVal		val;
	Pointer		arg;
{
	int		seed;

	if ( (seed = val.l) == 0 )
		seed = 1;	/* Default */

	(void)SRand(seed);

	return ACCEPTARG;
}



void
randrand()
{
	int		seed;

	if ( (seed = (time((long *)0) << 15) + getpid()) < 0 )
		seed = -seed;

	(void)SRand(seed);
}



void
testrand()
{
	register int	i;
	register int	n;
	register int	m;
	register int	col;
	register long *	a;

	n = HiRange;

	a = (long *)Malloc0((n + 1) * sizeof(long));

	n *= Bucket;
	do
		a[dorand()]++;
	while
		( --n > 0 );

	for ( i = HiRange, n = 0 ; i ; n++ )
		i /= 10;

	for ( i = Bucket, m = 0 ; i ; m++ )
		i /= 10;
	m++;

	for ( i = LoRange, col = 0 ; i <= HiRange ; i++ )
	{
		if ( col == 0 )
			printf("%*d:", n, i);

		printf(" %*d", m, a[i]);

		if ( ++col == 10 )
		{
			putchar('\n');
			col = 0;
		}
	}

	if ( col )
		putchar('\n');
}
