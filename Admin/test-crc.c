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


static char	sccsid[]	= "@(#)test-crc.c	1.17 05/12/16";

/*
**	Run crc code over known string and value to test accuracy,
**	Then iterate over average length packet to test speed.
*/

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

int	CputHz;				/* Kernel clock-ticks, set by `-c<n>' or `calibrate_cput()' */
char *	Name		= sccsid;	/* Program invoked name */
int	Traceflag;
bool	Verbose;

/*
**	Arguments.
*/

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(v, &Verbose, 0),
	Arg_int(t, &CputHz, 0, "HZ", OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_end
};

/*
**	Miscellaneous
*/

#define	CALLUSECS	1	/* Assume average usecs/call overhead */
#define	BYTETIME	0.05	/* Assume average usecs/byte crc time is this fraction of calloverhead */

Uchar *	String	= (Uchar *)"this is a sample string for testing.";
int	StrLen;			/* This will be its length */
Ulong	Val16	= 054616;	/* And this is its known CRC-16 */
Ulong	Val32	= 0x8ddeb8ad;	/* And this is its known CRC-32 */

char		BigStr[8192+CRC32_SIZE];	/* Large enough to access every word of table */
char		OverHead[CRC32_SIZE];

void		calibrate_cput _FA_((void));
int		check16 _FA_((int));
int		check32 _FA_((int));
double		cput _FA_((void));
int		test _FA_((bool(*)(), Ulong(*)(), bool(*)(), Ulong, int(*)(), char*, int));

#define	Printf	(void)printf



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	int	retval;
	Uchar *	cp;

	InitParams();

	DoArgs(argc, argv, Usage);

	calibrate_cput();

	StrLen = strlen((char *)String);
	cp = (Uchar *)malloc(StrLen+CRC32_SIZE);
	String = (Uchar *)strcpy((char *)cp, (char *)String);	/* New copy to avoid "readonly" strings */

	cp = &String[StrLen];
	String[StrLen] = 0xff;
	if ( (int)(String[StrLen]) != 0xff || (int)(BYTE(*cp)) != 0xff )
		Printf
		(
			"WARNING: c-compiler doesn't handle (unsigned char) properly!\n"
		);

	retval = test(crc, (Ulong(*)())acrc, tcrc, Val16, check16, "16", CRC_SIZE);

	if ( Verbose )
	Printf("\n");

	retval += test((bool(*)())crc32, (Ulong(*)())acrc32, tcrc32, Val32, check32, "32", CRC32_SIZE);

	exit(retval);
	return 0;
}



void
finish(error)
	int	error;
{
	exit(error);
}



int
test(Acrc, acrc, tcrc, Val, check, type, size)
	bool		(*Acrc)();
	Ulong		(*acrc)();
	bool		(*tcrc)();
	Ulong		Val;
	int		(*check)();
	char *		type;
	int		size;
{
	register bool	(*crc)()	= Acrc;
	register char *	cp;
	register long	i;
	register Ulong	val;
	int		retval;
	double		ohtime, calctime;
	double		count, calcount;
	static char	goodcrc[]	= "OK";
	static char	badcrc[]	= "WRONG!";
	static char	fmt[]		= "%s%s routine returns %lu %s";
	DODEBUG(Time_t	start_ms);

	retval = 0;
	i = StrLen;
	String[i] = 0xff;	/* Make incorrect */

	val = (Ulong)(*crc)((char *)String, (int)i);

	if ( size == 4 )
		val = 1;
	else
	if ( val == 0 )
		retval = 1;

	if ( Verbose || retval )
	Printf
	(
		fmt,
		"1st call of crc", type,
		(PUlong)val, val ? goodcrc : badcrc
	);

	retval += (*check)(i);

	(void)fflush(stdout);

	String[i] = Val & 0xff;
	String[i+1] = (Val >> 8) & 0xff;
	if ( size == 4 )
	{
		String[i+2] = (Val >> 16) & 0xff;
		String[i+3] = (Val >> 24) & 0xff;
	}

	val = (Ulong)(*crc)((char *)String, (int)i);

	if ( size == 4 )
		val = 0;
	else
	if ( val )
		retval = 1;

	if ( Verbose || retval )
	Printf
	(
		fmt,
		"2nd call of crc", type,
		(PUlong)val, val ? badcrc : goodcrc
	);

	retval += (*check)(i);

	(void)fflush(stdout);

	String[i] = Val & 0xff;
	String[i+1] = (Val >> 8) & 0xff;
	if ( size == 4 )
	{
		String[i+2] = (Val >> 16) & 0xff;
		String[i+3] = (Val >> 24) & 0xff;
	}

	if ( (val = (Ulong)(*tcrc)((char *)String, (int)i)) )
		retval = 1;

	if ( Verbose || retval )
	Printf
	(
		fmt,
		"tcrc", type,
		(PUlong)val, val ? badcrc : goodcrc
	);

	retval += (*check)(i);

	(void)fflush(stdout);

	if ( size == CRC_SIZE )
		val = (Ulong)(*acrc)((Crc_t)0, (char *)String, (int)i);
	else
		val = (Ulong)(*acrc)((Crc32_t)0, (char *)String, (int)i);

	if ( val == Val )
	{
		if ( Verbose )
		Printf("acrc%s routine calculated CRC-%s correctly.\n", type, type);
	}
	else
	{
		Printf
		(
			"acrc%s routine returned BAD value for calculated CRC-%s = 0x%lx (0%lo),\nshould be 0x%lx (0%lo).\n",
			type, type,
			(PUlong)val, (PUlong)val,
			(PUlong)Val, (PUlong)Val
		);

		retval = 1;
	}

	if ( retval == 0 && !Verbose )
		Printf("crc%s ok. ", type);

	(void)fflush(stdout);

	(void)SRand(357);

	for ( cp = BigStr ; cp < &BigStr[sizeof BigStr] ; )
		*cp++ = Rand()>>8;

	/** Calculate overhead **/

	DODEBUG(if(Traceflag) start_ms = millisecs());

	count = 1000000/CALLUSECS;
	calctime = 0;
	calcount = 0;

	do
	{
		count = count*2;
		i = count / 8;
		calcount += i*8;

		(void)cput();

		do
		{
			(void)(*crc)(OverHead, 0);
			(void)(*crc)(OverHead, 0);
			(void)(*crc)(OverHead, 0);
			(void)(*crc)(OverHead, 0);
			(void)(*crc)(OverHead, 0);
			(void)(*crc)(OverHead, 0);
			(void)(*crc)(OverHead, 0);
			(void)(*crc)(OverHead, 0);
		}
		while
			( --i );

		calctime += cput();
	}
	while
		( calctime < 10000000 );

	ohtime = calctime / calcount;

	if ( Verbose )
	{
		Trace3(1, "%.0f calls in %.3f real secs.", count, (millisecs()-start_ms)/1000.0);

		Printf
		(
			"crc%s routine overhead is %.3g microsecs per call.\n",
			type,
			ohtime
		);
	}
	else
		Printf("%5.3g microsecs/call", ohtime);

	(void)fflush(stdout);

	/** Calculate time /byte **/

	DODEBUG(if(Traceflag) start_ms = millisecs());

	count = 1000000/(ohtime*BYTETIME*(sizeof BigStr - CRC32_SIZE));
	calctime = 0;
	calcount = 0;

	do
	{
		count = count*2;
		i = count / 8;
		calcount += i*8;

		(void)cput();

		do
		{
			(void)(*crc)(BigStr, sizeof BigStr - CRC32_SIZE);
			(void)(*crc)(BigStr, sizeof BigStr - CRC32_SIZE);
			(void)(*crc)(BigStr, sizeof BigStr - CRC32_SIZE);
			(void)(*crc)(BigStr, sizeof BigStr - CRC32_SIZE);
			(void)(*crc)(BigStr, sizeof BigStr - CRC32_SIZE);
			(void)(*crc)(BigStr, sizeof BigStr - CRC32_SIZE);
			(void)(*crc)(BigStr, sizeof BigStr - CRC32_SIZE);
			(void)(*crc)(BigStr, sizeof BigStr - CRC32_SIZE);
		}
		while
			( --i );

		calctime += cput();
	}
	while
		( calctime < 10000000 );

	calctime = (calctime-(ohtime*count))/((sizeof BigStr - CRC32_SIZE)*count);

	if ( Verbose )
	{
		Trace3(1, "%.0f bytes in %.3f real secs.", (sizeof BigStr - CRC32_SIZE)*count, (millisecs()-start_ms)/1000.0);

		Printf
		(
			"crc%s calculation takes %.3g microsecs per byte.\n",
			type,
			calctime
		);
	}
	else
		Printf("; %5.3g microsecs/byte.\n", calctime);

	return retval;
}


int
check16(i)
	int		i;
{
	register Ulong	val;

	val = (String[i] & 0xff) | ((String[i+1] & 0xff) << 8);

	if ( val == Val16 )
	{
		if ( Verbose )
		Printf(" and calculated CRC-16 correctly.\n");
		return 0;
	}

	Printf
	(
		"\nBAD value for calculated CRC-16 = 0x%lx (0%lo), should be 0x%lx (0%lo).\n",
		(PUlong)val, (PUlong)val,
		(PUlong)Val16, (PUlong)Val16
	);

	return 1;
}


int
check32(i)
	int		i;
{
	register Ulong	val;

	val = (String[i] & 0xff)
		| ((String[i+1] & 0xff) << 8)
		| ((String[i+2] & 0xff) << 16)
		| ((String[i+3] & 0xff) << 24);

	if ( val == Val32 )
	{
		if ( Verbose )
		Printf(" and calculated CRC-32 correctly.\n");
		return 0;
	}

	Printf
	(
		"\nBAD value for calculated CRC-32 = 0x%lx (0%lo), should be 0x%lx (0%lo).\n",
		(PUlong)val, (PUlong)val,
		(PUlong)Val32, (PUlong)Val32
	);

	return 1;
}



#include	<sys/types.h>
#include	<sys/times.h>

void
calibrate_cput()
{
	register Ulong	st, et;
	register Uint	delay = 22;

	if ( CputHz != 0 )
	{
		if ( Verbose )
		{
			Printf("%s. Using %d ticks/sec.\n", Version, CputHz);
			(void)fflush(stdout);
		}
		return;
	}

	if ( Verbose )
	{
		Printf("%s\nCalibrating HZ ... ", Version);
		(void)fflush(stdout);
	}

	st = (Ulong)ticks();
	(void)sleep(delay);
	et = (Ulong)ticks();
	if ( et < st )
	{
		st = et;
		(void)sleep(delay);
		et = (long)ticks();
	}
	CputHz = (et-st+((Ulong)delay/2))/(Ulong)delay;

	DODEBUG(if(Traceflag) Printf("\n"));
	Trace5(1, "start=%lu stop=%lu (=%lu) ticks, delay=%d secs", (PUlong)st, (PUlong)et, (PUlong)(et-st), delay);
	Printf("%d ticks/sec.\n", CputHz);
	(void)fflush(stdout);
}


/*
**	Return elapsed time in microseconds
*/

double
cput()
{
	register double		nt;
	static struct tms	otb;
	struct tms		tb;

	(void)times(&tb);
	nt = tb.tms_utime - otb.tms_utime;
	otb.tms_utime = tb.tms_utime;
	nt = (nt*1000000.0)/CputHz;	/* microseconds */
	if ( nt > 100.0 ) Trace2(1, "cput() => %.0f CPU microsecs.", nt);
	return nt;
}
