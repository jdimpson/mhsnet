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
#include	"commandfile.h"
#include	"debug.h"



/*
**	Write commands from list at end of file.
*/

bool
WriteCmds(chp, fd, name)
	CmdHead *	chp;
	int		fd;
	char *		name;
{
	register Cmd *	cep;
	register char *	cp;
	register int	size;
	register char *	commands;
	long		posn;

	Trace4(1, "WriteCmds(%#lx, %d, %s)", (PUlong)chp, fd, name);

	for ( size = 0, cep = chp->ch_first ; cep != (Cmd *)0 ; cep = cep->cd_next )
	{
		if ( cep->cd_type == null_cmd )
			continue;

		size += 2;

		if ( (cp = cep->cd_value) != NULLSTR )
			size += strlen(cp);
	}

	if ( size == 0 )
	{
		Warn(english("Empty commands file \"%s\""), name);
		return false;
	}

	commands = cp = Malloc(size);

	for ( cep = chp->ch_first ; cep != (Cmd *)0 ; cep = cep->cd_next )
	{
		if ( cep->cd_type == null_cmd )
			continue;

#		if	SYSTEM > 0
		*cp++ = cep->cd_type;	/* (Cast removed because some SYSTEM V CCs barf!) */
#		else	/* SYSTEM > 0 */
		*cp++ = (char)cep->cd_type;
#		endif	/* SYSTEM > 0 */

		if ( cep->cd_value != NULLSTR )
			cp = strcpyend(cp, cep->cd_value) + 1;
		else
			*cp++ = '\0';
	}

	size = cp - commands;

	if ( (posn = lseek(fd, (long)0, 2)) == SYSERROR && errno != ESPIPE )
	{
		(void)SysWarn(CouldNot, SeekStr, name);
		free(commands);
		return false;
	}

	while ( write(fd, commands, size) != size )
	{
		if ( !SysWarn(CouldNot, WriteStr, name) )
		{
			free(commands);
			return false;
		}
		(void)lseek(fd, posn, 0);
	}

	free(commands);

	return true;
}
