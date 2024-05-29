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
#include	"header.h"



static char *	Header;		/* Malloced area for header */


/*
**	Read in a message header from file.
**	Returns "hr_ok" if successful, otherwise a failure reason.
*/

HdrReason
ReadHeader(fd)
	int			fd;
{
	register HdrFld *	fp;
	register char *		cp;
	register int		len;
	register int		i;
	Ulong			l;
	long			(*numfuncp)();
	char			length[HDR_LENGTH_SIZE+1];

	Trace2(1, "ReadHeader(%d)", fd);

	if
	(
		(DataLength = lseek(fd, (long)-HDR_LENGTH_SIZE, 2)) == SYSERROR
		||
		read(fd, length, HDR_LENGTH_SIZE) != HDR_LENGTH_SIZE
	)
		return hr_badread;

	length[HDR_LENGTH_SIZE] = '\0';

	if ( (l = (Ulong)atol(length)) > DataLength || l < MIN_HDR_LEN )
		return hr_badlen;

	len = (int)l;

	if ( len > HdrStrLength )
	{
		if ( Header != NULLSTR )
			free(Header);

		i = (len | 7) + 1;	/* Round up */
		Header = Malloc(i);
		HdrStrLength = i;
	}

	HdrLength = len + HDR_LENGTH_SIZE;

	if
	(
		(DataLength = (Ulong)lseek(fd, (long)-HdrLength, 2)) == SYSERROR
		||
		read(fd, Header, len) != len
	)
		return hr_badread;

	Trace3(2, "ReadHeader() hdrlength %d datalength %lu", HdrLength, (PUlong)DataLength);

	len -= CRC_SIZE;

	if ( tcrc(Header, len) )
		return hr_badcrc;

	switch ( Header[0] )
	{
	case HDR_TYPE1:	numfuncp = atol;	break;
	case HDR_TYPE2:	numfuncp = xtol;	break;
	default:	return ht_type;
	}

	for
	(
		fp = HdrFields.hdr_field, cp = Header, i = 0 ;
		i < NHDRFIELDS ;
		i++, fp++
	)
	{
		switch ( HdrConvs[i] )
		{
		case number_field:
			fp->hf_number = (Ulong)(*numfuncp)(cp);
			break;

		case string_field:
			if ( fp->hf_free )
				free(fp->hf_string);
			fp->hf_string = cp;
			break;

		case byte_field:
			fp->hf_byte = *cp;
		}

		fp->hf_free = false;

		while ( len-- && *cp++ != '\0' )
			;

		if ( len < 0 )
		{
			/*
			**	Check for TYP1 header.
			*/

			register HdrReason	hr;

			if ( (hr = (HdrReason)i) != ht_id || Header[0] != HDR_TYPE1 )
				return hr;

			/*
			**	Clear TYP2 fields from ID on.
			*/

			for ( ; i < NHDRFIELDS ; i++, fp++ )
			{
				switch ( HdrConvs[i] )
				{
				case number_field:
					fp->hf_number = 0;
					break;

				case string_field:
					if ( fp->hf_free )
						free(fp->hf_string);
					fp->hf_string = EmptyString;
					break;

				case byte_field:
					fp->hf_byte = '\0';
				}

				fp->hf_free = false;
			}
		}
	}

	return hr_ok;
}
