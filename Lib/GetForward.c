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
#undef	Extern
#define	Extern
#define	ExternU
#include	"forward.h"


static char *	GotForwds;

int		GFcomp _FA_((const void *, const void *));



/*
**	Read in and sort a forwarding description file if necessary,
**	then attempt to match name against one of the handlers;
**	return a Forward structure pointer if successful, else null.
*/

Forward *
GetForward(handler, name)
	char *		handler;
	char *		name;
{
	static char *	space	= " \t";
	Forward		key;

	Trace3(1, "GetForward(%s, %s)", handler, name);

	if ( handler == NULLSTR || name == NULLSTR )
		Fatal1(english("Null argument to GetForward"));

	if ( name[0] == '.' )
		return (Forward *)0;	/* Names may not start with a leading `.' */

	if ( GotForwds == NULLSTR || strcmp(GotForwds, name) != STREQUAL )
	{
		register char *		cp;
		register char *		np;
		register Forward *	fp;

		FreeStr(&GotForwds);
		GotForwds = newstr(name);

		FreeStr(&ForwdFile);
		ForwdFile = concat(SPOOLDIR, FORWARDINGDIR, name, NULLSTR);

		ForwdCount = 0;
		FreeStr((char **)&Forwds);

		FreeStr(&ForwdData);
		if ( (ForwdData = cp = ReadFile(ForwdFile)) == NULLSTR )
			return (Forward *)0;

		cp[RdFileSize++] = '\n';	/* Ensure terminating '\n' */
		cp[RdFileSize] = '\0';

		while ( (cp = strchr(cp, '\n')) != NULLSTR )
		{
			ForwdCount++;
			while ( *++cp == '\n' );
		}

		if ( ForwdCount == 0 )
			return (Forward *)0;	/* Malformed? */

		Forwds = fp = (Forward *)Malloc(ForwdCount * sizeof(Forward));

		cp = ForwdData;

		while ( (np = strchr(cp, '\n')) != NULLSTR )
		{
			*np = '\0';

			fp->handler = strtok(cp, space);
			fp->address = strtok(NULLSTR, space);
			fp->sub_addr = strtok(NULLSTR, space);
			fp++;

			cp = np;
			while ( *++cp == '\n' );
		}

		if ( ForwdCount > 1 )
			qsort((char *)Forwds, ForwdCount, sizeof(Forward), GFcomp);
	}

	if ( ForwdCount > 0 )
	{
		key.handler = handler;

		return (Forward *)bsearch((char *)&key, (char *)Forwds, ForwdCount, sizeof(Forward), GFcomp);
	}

	return (Forward *)0;
}



int
#ifdef	ANSI_C
GFcomp(const void *hp1, const void *hp2)
#else	/* ANSI_C */
GFcomp(hp1, hp2)
	char *	hp1;
	char *	hp2;
#endif	/* ANSI_C */
{
	return strcmp(((Forward *)hp1)->handler, ((Forward *)hp2)->handler);
}
