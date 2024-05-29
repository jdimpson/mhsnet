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
#include	"header.h"



/*
**	Write out header at end of file.
**	Assumes that "HdrFields" has been set up properly.
**	Result, if not SYSERROR, is the number of bytes in the header.
*/

int
WriteHeader(fd, end, minlength)
	int			fd;
	Ulong			end;
	int			minlength;
{
	register HdrFld *	fp;
	register char *		cp;
	register int		bytes;
	register int		i;
	register char *		hdbuf;
	Crc_t			crc;
	static char		fmt1[]	= "%lu";
	static char		fmt2[]	= "%lx";
	char *			numfmt;
	char			bufs[NHDRFIELDS][ULONG_SIZE];

	Trace4(2, "WriteHeader(%d, %lu, %d)", fd, (PUlong)end, minlength);

	if ( lseek(fd, end, 0) == (long)SYSERROR )
		if ( errno != ESPIPE || end > 0 )
			return SYSERROR;

	switch ( HdrType )
	{
	case HDR_TYPE1:	numfmt = fmt1;	break;
	case HDR_TYPE2:	numfmt = fmt2;	break;
	default:	Fatal1("WriteHeader: bad type!");
	}

	for
	(
		bytes = NHDRFIELDS, fp = HdrFields.hdr_field, i = 0 ;
		i < NHDRFIELDS ;
		i++, fp++
	)
	{
		switch ( HdrConvs[i] )
		{
		case number_field:
			if ( fp->hf_number )
			{
				cp = &bufs[i][0];
				(void)sprintf(cp, numfmt, (PUlong)fp->hf_number);
				bytes += strlen(cp);
			}
			break;

		case string_field:
			if ( (cp = fp->hf_string) != NULLSTR )
				bytes += strlen(cp);
			break;

		case byte_field:
			if ( fp->hf_byte != '\0' )
				bytes++;
		}
	}

	if ( minlength > (bytes+CRC_SIZE+HDR_LENGTH_SIZE) )
		bytes = minlength - (CRC_SIZE+HDR_LENGTH_SIZE);

	DODEBUG(if(CRC_SIZE!=2)Fatal("CRC_SIZE"));

	hdbuf = Malloc(bytes+CRC_SIZE+HDR_LENGTH_SIZE+1);

	for
	(
		cp = hdbuf, fp = HdrFields.hdr_field, i = 0;
		i < NHDRFIELDS ;
		i++, fp++
	)
	{
		switch ( HdrConvs[i] )
		{
		case number_field:
			if ( fp->hf_number )
			{
				cp = strcpyend(cp, &bufs[i][0]) + 1;	/* Add 1 for null */
				continue;
			}
			break;

		case string_field:
			if ( fp->hf_string != NULLSTR )
			{
				cp = strcpyend(cp, fp->hf_string) + 1;	/* Add 1 for null */
				continue;
			}
			break;

		case byte_field:
			if ( fp->hf_byte != '\0' )
				*cp++ = fp->hf_byte;
		}

		*cp++ = '\0';
	}

	while ( cp < &hdbuf[bytes] )
		*cp++ = '\0';

	crc = acrc((Crc_t)0, hdbuf, bytes);
	*cp++ = LOCRC(crc);
	*cp++ = HICRC(crc);
	bytes += CRC_SIZE;

	(void)strclr(cp, HDR_LENGTH_SIZE);
	(void)sprintf(cp, "%d", bytes);
	bytes += HDR_LENGTH_SIZE;

	errno = 0;
	if ( write(fd, hdbuf, bytes) != bytes )
	{
		free(hdbuf);
		return SYSERROR;
	}

	free(hdbuf);

	return HdrLength = bytes;
}
