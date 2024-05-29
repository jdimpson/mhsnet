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


#include	"global.h"
#include	"debug.h"
#include	"ftheader.h"



/*
**	Read in a file transfer header from file.
**
**	Returns "fth_ok" if successful, otherwise a failure reason.
*/

FthReason
ReadFtHeader(fd, end)
	int			fd;
	Ulong			end;
{
	register char **	fp;
	register char *		cp;
	register int		len;
	register int		i;
	register Ulong		l;
	char			length[FTH_LENGTH_SIZE+1];

	Trace3(1, "ReadFtHeader(%d, %lu)", fd, (PUlong)end);

	if
	(
		lseek(fd, (long)end-FTH_LENGTH_SIZE, 0) == SYSERROR
		||
		read(fd, length, FTH_LENGTH_SIZE) != FTH_LENGTH_SIZE
	)
		return fth_badread;

	length[FTH_LENGTH_SIZE] = '\0';

	if ( (l = (Ulong)atol(length)) > end || l < MIN_FTH_LENGTH )
		return fth_badlen;

	if ( (len = (int)l) > FthStrLength )
	{
		if ( FtHeader != NULLSTR )
			free(FtHeader);

		i = (len | 7) + 1;	/* Round up */
		FtHeader = Malloc(i);
		FthStrLength = i;
	}

	FthLength = len + FTH_LENGTH_SIZE;

	if
	(
		(FtDataLength = lseek(fd, (long)end-FthLength, 0)) == SYSERROR
		||
		read(fd, FtHeader, len) != len
	)
		return fth_badread;

	Trace3(2, "ReadFtHeader() hdrlength %d datalength %lu", FthLength, (PUlong)FtDataLength);

	len -= CRC_SIZE;

	if ( tcrc(FtHeader, len) )
		return fth_badcrc;

	FthType[0] = BYTE(FtHeader[0]);
	FthType[1] = '\0';

	for
	(
		fp = FthFields.fth_start, cp = FtHeader, i = 0 ;
		i < NFTHFIELDS ;
		i++, fp++
	)
	{
		Trace4(4, "field %d: len=%2d \"%s\"", i, len, cp);

		*fp = cp;

		while ( len-- && *cp++ != '\0' )
			;

		if ( len < 0 )
			return (FthReason)i;
	}

	cp = &FtHeader[l-(2*CRC_SIZE)];

	DataCrc = BYTE(cp[0]) | (BYTE(cp[1])<<8);

	if ( TYPEOF_FTH(FthType[0]) != FTH_TYPE )
		return fth_type;

	return fth_ok;
}
