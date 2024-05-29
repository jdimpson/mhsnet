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

#include	"global.h"
#include	"commandfile.h"
#include	"debug.h"



/*
**	Check data CRC in files named in cmds starting at obase up to ofend.
*/

bool
CheckData(cmds, obase, ofend, datacrcp)
	CmdHead *	cmds;
	Ulong		obase;
	Ulong		ofend;
	Crc_t *		datacrcp;
{
	register int	r;
	register Cmd *	cep;
	register Crc_t	rcrc	= 0;
	register Ulong	oposn;
	register Ulong	ibase	= 0;
	register Ulong	ifend;
	int		ifd;
	char *		ifile;
	Ulong		ilength;
	bool		checkcrc;
	char		buf[FILEBUFFERSIZE];

	Trace5(1, "CheckData(%#lx, %lu, %lu, %#lx)", (PUlong)cmds, (PUlong)obase, (PUlong)ofend, datacrcp==(Crc_t *)0?(PUlong)0:(PUlong)*datacrcp);

	if ( datacrcp == (Crc_t *)0 )
		checkcrc = false;
	else
		checkcrc = true;

	if ( ofend > obase )
	for ( oposn = 0, cep = cmds->ch_first ; cep != (Cmd *)0 && oposn < ofend ; cep = cep->cd_next )
	{
		switch ( cep->cd_type )
		{
		default:
			continue;

		case file_cmd:
			ifile = cep->cd_value;
			continue;

		case start_cmd:
			ibase = cep->cd_number;
			continue;

		case end_cmd:
			ifend = cep->cd_number;
		}

		ilength = ifend - ibase;

		if ( (oposn + ilength) > obase )
		{
			if ( obase > oposn )
			{
				ilength -= obase - oposn;
				ibase += obase - oposn;
				oposn = obase;
			}

			if ( (oposn + ilength) > ofend )
			{
				ifend -= (oposn + ilength) - ofend;
				ilength = ofend - oposn;
			}

			Trace4(2, "check file \"%s\" %lu->%lu", ifile, (PUlong)ibase, (PUlong)ifend);

			while ( (ifd = open(ifile, O_READ)) == SYSERROR )
				if ( !SysWarn(CouldNot, OpenStr, ifile) )
					return false;

			if ( ibase != 0 )
				(void)lseek(ifd, (long)ibase, 0);

			if ( checkcrc )
			while ( ibase < ifend )
			{
				Ulong	l;

				if ( (l = ifend - ibase) > sizeof buf )
					r = sizeof buf;
				else
					r = l;

				while ( (r = read(ifd, buf, r)) <= 0 )
					if ( r == 0 || !SysWarn(CouldNot, ReadStr, ifile) )
					{
						(void)close(ifd);
						return false;
					}

				ibase += r;

				rcrc = acrc(rcrc, buf, r);
			}

			(void)close(ifd);
		}

		oposn += ilength;
	}

	if ( !checkcrc )
		return true;

	if ( *datacrcp != rcrc )
	{
		*datacrcp = rcrc;
		return false;
	}

	return true;
}
