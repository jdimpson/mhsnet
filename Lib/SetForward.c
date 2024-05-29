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
#include	"forward.h"
#include	"Passwd.h"



/*
**	Set up forwarding file with entry for handler
**	(or change existing data.)
**
**	If ``address'' is null, remove forwarding line.
*/

void
SetForward(handler, name, address, sub_addr)
	char *			handler;
	char *			name;
	char *			address;
	char *			sub_addr;
{
	register Forward *	fp;
	register char *		cp;
	register int		i;
	register int		fd;
	register int		len;
	char *			savedata;

	Trace5(1, "SetForward(%s, %s, %s, %s)", handler, name, address, sub_addr);

	if ( handler == NULLSTR || name == NULLSTR )
		Fatal1(english("Null argument to SetForward"));

	if ( (fp = GetForward(handler, name)) != (Forward *)0 )
	{
		if
		(
			address != NULLSTR
			&&
			strcmp(fp->address, address) == STREQUAL
			&&
			(
				(sub_addr == NULLSTR && fp->sub_addr == NULLSTR)
				||
				(sub_addr != NULLSTR && fp->sub_addr != NULLSTR && strcmp(fp->sub_addr, sub_addr) == STREQUAL)
			)
		)
			return;

		fp->address = address;
		fp->sub_addr = sub_addr;
	}
	else
	if ( address != NULLSTR )
	{
		fp = (Forward *)Malloc((ForwdCount+1) * sizeof(Forward));

		if ( ForwdCount )
			bcopy((char *)Forwds, (char *)(fp+1), ForwdCount * sizeof(Forward));

		FreeStr((char **)&Forwds);
		Forwds = fp;
		ForwdCount++;

		fp->handler = handler;
		fp->address = address;
		fp->sub_addr = sub_addr;
	}
	else
		return;

	for ( fp = Forwds, i = ForwdCount, len = 0 ; --i >= 0 ; fp++ )
	{
		if ( (cp = fp->address) == NULLSTR )
			continue;
		len += strlen(fp->handler);
		len += strlen(cp);
		if ( (cp = fp->sub_addr) != NULLSTR )
			len += strlen(cp);
		len += 3;
	}

	savedata = ForwdData;
	ForwdData = cp = Malloc(++len);

	for ( fp = Forwds, i = ForwdCount ; --i >= 0 ; fp++ )
	{
		if ( fp->address == NULLSTR )
			continue;
		cp = strcpyend(cp, fp->handler);
		*cp++ = '\t';
		cp = strcpyend(cp, fp->address);
		if ( fp->sub_addr != NULLSTR )
		{
			*cp++ = '\t';
			cp = strcpyend(cp, fp->sub_addr);
		}
		*cp++ = '\n';
	}

	*cp = '\0';

	if ( savedata != NULLSTR )
		free(savedata);

	if ( (len = cp - ForwdData) == 0 )
	{
		(void)unlink(ForwdFile);
		return;
	}

	(void)unlink(ForwdFile);
	fd = create(ForwdFile);

	GetNetUid();

	(void)chown(ForwdFile, NetUid, NetGid);

	if ( write(fd, ForwdData, len) != len )
		Syserror(CouldNot, WriteStr, ForwdFile);

	(void)close(fd);
}
