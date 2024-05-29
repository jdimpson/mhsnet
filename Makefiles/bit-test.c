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
**	Find bit sizes for bytes, shorts, ints and longs.
*/

#include	<stdlib.h>
#include	<stdio.h>
#include	<unistd.h>

char *		define		= "#define\t";
char *		formatd		= "%s%s%s\t%d\n";
char *		formatx		= "%s%s%s\t%s0x%lx%c\n";

#if	ANSI_C
void	printdefs(char *, char *, char *, unsigned long, unsigned long, int, int);
#else	/* ANSI_C */
void	printdefs();
#endif	/* ANSI_C */

#include	<time.h>

char		max_char;
unsigned char	max_uchar;
short		max_short;
unsigned short	max_ushort;
int		max_int;
unsigned int	max_uint;
long		max_long;
unsigned long	max_ulong;
time_t		max_time_t;
time_t		max_utime_t;
size_t		max_size_t;
size_t		max_usize_t;
ssize_t		max_ssize_t;
ssize_t		max_ussize_t;



int
main()
{
	register int	n;

	unsigned char	bitc;
	unsigned short	bits;
	unsigned int	biti;
	unsigned long	bitl;
	time_t		timet;
	size_t		sizet;
	ssize_t		ssizet;

	for ( n = 0, bitc = 1 ; bitc != 0 ; bitc <<= 1, n++ )
		max_uchar = bitc;

	max_char = ~max_uchar;
	max_uchar += max_char;

	printdefs("CHAR", "", "(Uchar)", (unsigned long)max_char, (unsigned long)max_uchar, n, ' ');

	for ( n = 0, bits = 1 ; bits != 0 ; bits <<= 1, n++ )
		max_ushort = bits;

	max_short = ~max_ushort;
	max_ushort += max_short;

	printdefs("SHORT", "", "(Ushort)", (unsigned long)max_short, (unsigned long)max_ushort, n, ' ');

	for ( n = 0, biti = 1 ; biti != 0 ; biti <<= 1, n++ )
		max_uint = biti;

	biti = n;
	max_int = ~max_uint;
	max_uint += max_int;

	printdefs("INT", "", "(Uint)", (unsigned long)max_int, (unsigned long)max_uint, n, ' ');

	for ( n = 0, bitl = 1 ; bitl != 0 ; bitl <<= 1, n++ )
		max_ulong = bitl;

	bitl = n;
	max_long = ~max_ulong;
	max_ulong += max_long;

	printdefs("LONG", "", "(Ulong)", (unsigned long)max_long, (unsigned long)max_ulong, n, 'L');

#	if	LONG_LONG
	{
	unsigned long long	bitll;
	long long		max_llong;
	unsigned long long	max_ullong;
#	ifdef	__osf__
	static char *		formatx	= "%s%s0x%lxL\n";
#	else	/* __osf__ */
#	ifdef	__mips
	static char *		formatx	= "%s%s0x%llxLL\n";
#	else	/* __mips */
	static char *		formatx	= "%s%s0x%llxLL\n";	/* MODIFY THIS? */
#	endif	/* __mips */
#	endif	/* __osf__ */

	for ( n = 0, bitll = 1 ; bitll != 0 ; bitll <<= 1, n++ )
		max_ullong = bitll;

	max_llong = ~max_ullong;
	max_ullong += max_llong;

	(void)printf(formatx, define, "MAX_LLONG\t(Llong)", max_llong);
	(void)printf(formatx, define, "MAX_ULLONG\t(Ullong)", max_ullong);
	(void)printf(formatd, define, "BITS_U", "LLONG", n);
	}
#	else	/* LONG_LONG */

	printdefs("LLONG", "(Llong)", "(Ullong)", (unsigned long)max_long, (unsigned long)max_ulong, n, 'L');

#	endif	/* LONG_LONG */

	for ( n = 0, timet = 1 ; timet != 0 ; timet <<= 1, n++ )
		max_utime_t = timet;

	max_time_t = ~max_utime_t;
	max_utime_t += max_time_t;

	printdefs("time_t", "(time_t)", "(time_t)", (unsigned long)max_time_t, (unsigned long)max_utime_t, n, 'L');

	for ( n = 0, sizet = 1 ; sizet != 0 ; sizet <<= 1, n++ )
		max_usize_t = sizet;

	max_size_t = ~max_usize_t;
	max_usize_t += max_size_t;

	printdefs("size_t", "(size_t)", "(size_t)", (unsigned long)max_size_t, (unsigned long)max_usize_t, n, 'L');

	for ( n = 0, ssizet = 1 ; ssizet != 0 ; ssizet <<= 1, n++ )
		max_ussize_t = ssizet;

	max_ssize_t = ~max_ussize_t;
	max_ussize_t += max_ssize_t;

	printdefs("ssize_t", "(ssize_t)", "(ssize_t)", (unsigned long)max_ssize_t, (unsigned long)max_ussize_t, n, 'L');

	exit(0);
	return 0;
}


void
printdefs(name, cast, unsigned_cast, val, unsigned_val, bits, c)
	char *		name;
	char *		cast;
	char *		unsigned_cast;
	unsigned long	val;
	unsigned long	unsigned_val;
	int		bits;
	int		c;
{
	(void)printf(formatx, define, "MAX_", name, cast, val, c);
	(void)printf(formatx, define, "MAX_U", name, unsigned_cast, unsigned_val, c);
	(void)printf(formatd, define, "BITS_U", name, bits);
}
