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



/*
**	Remove an environment field tagged by 'name' and 'value',
**	(or a field matched by 'name' if it is an environment field).
**
**	If the field exists, it will be removed from the environment,
**	and DelEnv will return true, otherwise false.
*/
/*VARARGS1*/
bool
DelEnv(name, value)
	register char *	name;
	char *		value;
{
	register char *	ep;
	register char *	np;
	register int	len;
	register bool	val;
	register bool	freename;

	if ( (ep = HdrEnv) == NULLSTR )
		return false;

	if ( name[0] != ENV_RS )
	{
		name = MakeEnv(name, value, NULLSTR);
		freename = true;
	}
	else
		freename = false;

	len = strlen(name);
	val = false;

	while ( (np = StringMatch(ep, name)) != NULLSTR )
	{
		(void)strcpy(np, np+len);
		ep = np;
		val = true;
	}

	Trace3(2, "DelEnv(%s) ==> %s", name+1, val?"true":"false");

	if ( freename )
		free(name);

	return val;
}
