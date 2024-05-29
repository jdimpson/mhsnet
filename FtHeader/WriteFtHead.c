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


#define	ERRNO

#include	"global.h"
#include	"debug.h"
#include	"ftheader.h"



/*
**	Write out file transfer header at end of data.
**	Assumes that "FthFields" has been set up properly.
**	Result, if not SYSERROR, is the number of bytes in the header.
*/

int
WriteFtHeader(fd, start)
	int			fd;
	Ulong			start;
{
	register char **	fp;
	register int		n;
	register char *		cp;
	register int		bytes;
	register char *		hdbuf;
	Crc_t			crc;

	Trace3(2, "WriteFtHeader(%d, %lu)", fd, (PUlong)start);

	if ( lseek(fd, start, 0) == (long)SYSERROR )
		if ( errno != ESPIPE || start > 0 )
			return SYSERROR;

	FthFields.fth_start[(int)fth_type] = FthType;

	for
	(
		fp = FthFields.fth_start, bytes = NFTHFIELDS, n = 0 ;
		n < NFTHFIELDS ;
		n++, fp++
	)
		if ( (cp = *fp) != NULLSTR )
			bytes += strlen(cp);

	hdbuf = Malloc(bytes+2*CRC_SIZE+FTH_LENGTH_SIZE+1);

	for
	(
		fp = FthFields.fth_start, cp = hdbuf, n = 0 ;
		n < NFTHFIELDS ;
		n++, fp++
	)
		if ( *fp != NULLSTR )
			cp = strcpyend(cp, *fp) + 1;	/* Add 1 for null */
		else
			*cp++ = '\0';

	DODEBUG(if(CRC_SIZE!=2)Fatal("CRC_SIZE"));

	*cp++ = LOCRC(DataCrc);
	*cp++ = HICRC(DataCrc);
	bytes += CRC_SIZE;

	crc = acrc((Crc_t)0, hdbuf, bytes);

	*cp++ = LOCRC(crc);
	*cp++ = HICRC(crc);
	bytes += CRC_SIZE;

	(void)strclr(cp, FTH_LENGTH_SIZE);
	(void)sprintf(cp, "%d", bytes);
	bytes += FTH_LENGTH_SIZE;

	errno = 0;
	if ( write(fd, hdbuf, bytes) != bytes )
	{
		free(hdbuf);
		return SYSERROR;
	}

	free(hdbuf);

	return FthLength = bytes;
}
