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



/*
**	Check whether messages are being forwarded.
**
**	Return Forward structure, or null.
*/

Forward *
Forwarded(handler, name, source_address)
	char *			handler;
	char *			name;		/* Forced to lowercase */
	char *			source_address;
{
	register Forward *	fp;
	register char *		cp;

	Trace4(1, "Forwarded(%s, %s, %s)", handler, name, source_address);

	if ( (cp = name) == NULLSTR || cp[0] == '\0' )
		return (Forward *)0;

	cp = ToLower(cp, strlen(cp));
	fp = GetForward(handler, cp);
	free(cp);

/*	if ( fp == (Forward *)0 )
**		return (Forward *)0;
**
**	if ( AddressMatch(fp->address, source_address) )
**		return (Forward *)0;
*/
	return fp;
}
