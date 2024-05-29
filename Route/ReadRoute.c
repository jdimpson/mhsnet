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
#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"link.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"routefile.h"


#if	SYSVREL >= 4
#undef	SYSVREL
#define	SYSVREL	0	/* mmap(2) is slower than read(2) */
#endif	/* SYSVREL >= 4 */

#if	SYSVREL >= 4
#include	<sys/mman.h>
#endif	/* SYSVREL >= 4 */


int		MinTypC;
int		MaxTypC;

static Time_t	RouteRead;	/* Time routefile last read */
static Ulong	RouteSize;	/* Size of last routefile */

static Ulong	Sum _FA_((Uchar *, int, Ulong));



/*
**	Read new routefile if changed.
*/

bool
CheckRoute(check)
	bool		check;
{
	struct stat	statb;

	Trace4(1,
		"CheckRoute(%scheck) base=%#lx time=%#lx",
		check?EmptyString:"no",
		(PUlong)RouteBase,
		(PUlong)RouteRead
	);

	if
	(
		RouteBase == NULLSTR
		||
		stat(RouteFile, &statb) == SYSERROR
		||
		statb.st_mtime > RouteRead
		||
		statb.st_size != RouteSize
	)
		return ReadRoute(check);

	return false;
}



/*
**	Read routing tables into memory.
*/

bool
ReadRoute(check)
	bool		check;
{
	register int	fd;
	register int	count;
	register Index	newsize;
	Ulong		sumcheck;
	struct stat	statb;

	if ( RouteFile == NULLSTR )
		RouteFile = concat(SPOOLDIR, STATEDIR, ROUTEFILE, NULLSTR);

	Trace3(1, "ReadRoute(%scheck) \"%s\"", check?EmptyString:"no", RouteFile);

	for ( count = 3 ; ; count-- )
	{
		while ( (fd = open(RouteFile, O_READ)) == SYSERROR )
		{
			if ( SysRetry(errno) )
				continue;

			if ( count )
				goto out;

			Syserror(CouldNot, OpenStr, RouteFile);
		}

		while ( fstat(fd, &statb) == SYSERROR )
			Syserror(CouldNot, StatStr, RouteFile);

		if ( (RouteSize = newsize = statb.st_size) > 0 )
			break;

		(void)close(fd);

		if ( count == 0 )
			return false;
out:
		(void)sleep(10);
	}

	RouteRead = statb.st_mtime;	/* Remember modify time */

#	if	SYSVREL >= 4

	if ( RouteBase != NULLSTR )
		while ( munmap(RouteBase, Route_size) == SYSERROR )
			Syserror(CouldNot, "munmap", RouteFile);

	for ( ;; )
	{
		RouteBase = (char *)mmap((caddr_t)0, (int)newsize, PROT_READ, MAP_SHARED, fd, (off_t)0);
		if ( RouteBase != (char *)-1 )
			break;
		Syserror(CouldNot, "mmap", RouteFile);
	}

	Route_size = newsize;

#	else	/* SYSVREL >= 4 */

	if ( RouteBase == NULLSTR || newsize > Route_size )
	{
		if ( RouteBase != NULLSTR )
			free(RouteBase);
		RouteBase = Malloc((int)newsize);
		Route_size = newsize;
	}

	while ( read(fd, RouteBase, (int)newsize) != newsize )
	{
		Syserror(CouldNot, ReadStr, RouteFile);
		(void)lseek(fd, (long)0, 0);
	}

#	endif	/* SYSVREL >= 4 */

	(void)close(fd);

	if ( check )
	{
		count = newsize - sizeof(sumcheck);
		bcopy(RouteBase+count, (char *)&sumcheck, sizeof(sumcheck));

		if ( Sum((Uchar *)RouteBase, count, (Ulong)0) != sumcheck )
		{
			Error(english("routing tables in \"%s\" corrupted"), RouteFile);
			return false;
		}
	}

	MaxTypes = MAXTYPES;
	TypeCount = TYPE_COUNT;
	LinkCount = LINK_COUNT;
	TypeEntries = TYPE_ENTRIES;
	RegionCount = REGION_COUNT;
	FwdMapCount = FWDMAP_COUNT;
	AdrMapCount = ADRMAP_COUNT;
	RegMapCount = REGMAP_COUNT;
	TypMapCount = TYPMAP_COUNT;
	MaxLinkLength = MAXLINKLENGTH;
	MaxStrLength = MAXSTRLENGTH;

	Trace3(1, "LinkCount %lu, RegionCount %lu", (PUlong)LinkCount, (PUlong)RegionCount);

	Trace5(
		2,
		"maxtypes %lu, typecount %lu, maxlinklen %lu, maxstrlen %lu",
		(PUlong)MaxTypes,
		(PUlong)TypeCount,
		(PUlong)MaxLinkLength,
		(PUlong)MaxStrLength
	);

	fd = ((LinkCount * RegionCount) + 7) / 8;

	TypeNameVector = (Index *)&RouteBase[ROUTE_HEADER_SIZE];
	TypeVector = (TypeTab *)&TypeNameVector[MaxTypes];
	TypeTables = (TypeEnt *)&TypeVector[TypeCount];
	LinkVector = (RLink *)&TypeTables[TypeEntries];
	RegionVector = (RegionEnt *)&LinkVector[LinkCount];
	NameVector = (MapEnt *)&RegionVector[RegionCount];
	FwdMapVector = (MapEnt *)&NameVector[RegionCount];
	AdrMapVector = (MapEnt *)&FwdMapVector[FwdMapCount];
	RegMapVector = (MapEnt *)&AdrMapVector[AdrMapCount];
	TypMapVector = (MapEnt *)&RegMapVector[RegMapCount];
	RegionTable = (char *)&TypMapVector[TypMapCount];

	Strings = &RegionTable[fd];
	HomeName = &Strings[HOME_INDEX];
	VisibleName = &Strings[VIS_INDEX];
#	if	CHECK_LICENCE
	LicenceNumber = &Strings[SERIAL_INDEX];
#	endif	/* CHECK_LICENCE */

	MinTypC = TypeVector->tp_type[0];
	MaxTypC = MinTypC + MaxTypes-1;

	Trace3(2, "MinTypC '%c', MaxTypC '%c'", MinTypC, MaxTypC);

	return true;
}



/*
**	Alternate name for routing tables file.
*/

void
SetRoute(newfile)
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
