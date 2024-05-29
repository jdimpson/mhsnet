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


#define	FILE_CONTROL
#define	STAT_CALL

#include	"global.h"
#include	"debug.h"
#include	"link.h"
#include	"route.h"
#include	"routefile.h"
#include	"spool.h"


#ifdef	ALIGN
#undef	ALIGN
#endif
#define		ALIGN		sizeof(int)
#define		MASK		(ALIGN-1)


static char *	TmpFile;
static char	Template[UNIQUE_NAME_LENGTH+1];

extern Time_t	Time;

static Ulong	Sum _FA_((Uchar *, int, Ulong));



/*
**	Size of routing tables file.
*/

Ulong
RouteSize()
{
	struct stat	statb;

	if ( RouteFile == NULLSTR )
		RouteFile = concat(SPOOLDIR, STATEDIR, ROUTEFILE, NULLSTR);

	if ( stat(RouteFile, &statb) == SYSERROR )
		return (Ulong)0;

	return (Ulong)statb.st_size;
}



/*
**	Alternate name for routing tables file.
*/

void
SetSRoute(newfile)
	char *	newfile;
{
	RouteFile = newfile;

	Trace2(1, "SetRoute(%s)", RouteFile);
}



/*
**	Sum bytes over range.
*/

static Ulong
Sum(data, count, sum)
	register Uchar *data;
	int		count;
	register Ulong	sum;
{
	register int	i;
	register int	j;

	Trace4(2, "Sum(%#lx, %d, %#lx)", (PUlong)data, count, (PUlong)sum);

	if ( (i = (count+7) >> 3) > 0 || count > 0 )
	{
		if ( (j = (count & 7)) )
			data -= 8 - j;

		switch ( j )
		{
		default:
		case 0:	do {	sum += data[0];
		case 7:		sum += data[1];
		case 6:		sum += data[2];
		case 5:		sum += data[3];
		case 4:		sum += data[4];
		case 3:		sum += data[5];
		case 2:		sum += data[6];
		case 1:		sum += data[7];
				data += 8;
			} while ( --i );
		}
	}

	Trace2(1, "Sum() => %#lx", (PUlong)sum);

	return sum;
}



/*
**	Write the routing table to the routefile.
*/

Ulong
WriteRoute()
{
	register int	fd;
	register char *	cp;
	register int	ss;
	Ulong		sumcheck;

	Trace1(1, "WriteRoute()");

#	if	DEBUG
	if ( sizeof(Index) & MASK )
		Error(english("routing tables index size %d not aligned on %d"), sizeof(Index), MASK);
#	endif	/* DEBUG */

	if ( RouteBase == NULLSTR )
		MakeRoute();

	if ( RouteFile == NULLSTR )
		RouteFile = concat(SPOOLDIR, STATEDIR, ROUTEFILE, NULLSTR);

	if ( TmpFile == NULLSTR )
	{
		(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);

		if ( (cp = strrchr(RouteFile, '/')) != NULLSTR )
		{
			*cp = '\0';
			TmpFile = concat(RouteFile, Slash, Template, NULLSTR);
			*cp = '/';
		}
		else
			TmpFile = newstr(Template);
	}

	(void)UniqueName(TmpFile, CHOOSE_QUALITY, OPTNAME, (Ulong)(Route_size+Str_size), Time);

	fd = create(TmpFile);

	Trace4(1, "WriteRoute() tables %lu, strings %lu, on %d", (PUlong)Route_size, (PUlong)Str_size, fd);

	while ( (Index)write(fd, RouteBase, (int)Route_size) != Route_size )
	{
		Syserror(CouldNot, WriteStr, TmpFile);
		(void)lseek(fd, (long)0, 0);
	}

	sumcheck = Sum((Uchar *)RouteBase, (int)Route_size, (Ulong)0);
	ss = Str_size;
	sumcheck = Sum((Uchar *)Strings, ss, sumcheck);
	bcopy((char *)&sumcheck, Strings+ss, sizeof(sumcheck));

	ss += sizeof(sumcheck);

	while ( (Index)write(fd, Strings, ss) != ss )
	{
		Syserror(CouldNot, WriteStr, TmpFile);
		(void)lseek(fd, (long)Route_size, 0);
	}

	(void)close(fd);

	(void)chmod(TmpFile, 0644);
	move(TmpFile, RouteFile);

	return (Ulong)(Route_size+ss);
}
