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
#define	ERRNO

#include	"global.h"
#include	"commandfile.h"
#include	"debug.h"



/*
**	Copy data in files named in cmds to ofd starting at obase up to ofend.
*/

bool
CollectData(cmds, obase, ofend, ofd, ofile)
	CmdHead *	cmds;
	Ulong		obase;
	Ulong		ofend;
	int		ofd;
	char *		ofile;
{
	register int	r;
	register int	w;
	register char *	cp;
	register Cmd *	cep;
	long		posn;
	Ulong		ibase;
	Ulong		ifend;
	int		ifd;
	char *		ifile;
	Ulong		ilength;
	Ulong		oposn;
	bool		pipe = false;
	char		buf[FILEBUFFERSIZE];

	Trace6(1, "CollectData(%#lx, %lu, %lu, %d, %s)", (PUlong)cmds, (PUlong)obase, (PUlong)ofend, ofd, ofile);

	if ( ofend <= obase )
		return true;

	while ( (posn = lseek(ofd, (long)0, 1)) == SYSERROR )
	{
		if ( errno == ESPIPE )
		{
			pipe = true;
			posn = 0;
			break;
		}

		if ( !SysWarn(CouldNot, SeekStr, ofile) )
			return false;
	}

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

			Trace4(2, "collect file \"%s\" %lu->%lu", ifile, (PUlong)ibase, (PUlong)ifend);

			while ( (ifd = open(ifile, O_READ)) == SYSERROR )
				if ( !SysWarn(CouldNot, OpenStr, ifile) )
					return false;

			if ( ibase != 0 )
				(void)lseek(ifd, (long)ibase, 0);

			while ( ibase < ifend )
			{
				Ulong	l;

				if ( (l = ifend - ibase) > sizeof buf )
					r = sizeof buf;
				else
					r = l;

				while ( (r = read(ifd, buf, r)) <= 0 )
					if ( r == 0 || !SysWarn(CouldNot, ReadStr, ifile) )
						return false;

				ibase += r;

				cp = buf;

				while ( (w = write(ofd, cp, r)) != r )
				{
					if ( w == SYSERROR )
					{
						if ( !SysWarn(CouldNot, WriteStr, ofile) )
							return false;
						if ( !pipe )
							(void)lseek(ofd, posn, 0);
					}
					else
					{
						cp += w;
						r -= w;
						posn += w;
					}
				}

				posn += r;
			}

			(void)close(ifd);
		}

		oposn += ilength;
	}

	return true;
}
