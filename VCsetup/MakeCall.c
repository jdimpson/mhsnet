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


#define	STDIO

#include	"global.h"
#include	"debug.h"
#include	"call.h"


bool		CallErr	= false;


/*
**	Switch input to another file.
*/

Op *
MakeCall(fname)
	char *		fname;
{
	register char *	cp;
	Op *		o;
	FILE *		f;
	CallFrame	save;

	Trace2(1, "MakeCall(%s)", fname);

	if ( calldepth++ > MAXCALL )
	{
		warning(english("call nesting too deep - "), fname);
		return (Op *)0;
	}

	for ( cp = newstr(fname) ; (f = fopen(cp, "r")) == NULL ; )
	{
		if ( cp[0] == '/' )
		{
			free(cp);
			(void)SysWarn(CouldNot, OpenStr, fname);
			return (Op *)0;
		}

		free(cp);

		if ( strchr(fname, '/') != NULLSTR )
			cp = concat(SPOOLDIR, fname, NULLSTR);
		else
			cp = concat(SPOOLDIR, CALLDIR, fname, NULLSTR);
	}

	save = CallScript;

	CallScript.fd = f;
	CallScript.name = cp;
	CallScript.lineno = 1;
	CallScript.head = (Op *)0;

	(void)yyparse();

	(void)fclose(f);

	if ( CallErr )
	{
		CallErr = false;
		o = (Op *)0;
	}
	else
		o = CallScript.head;

	CallScript = save;

	free(cp);

	calldepth--;

	return o;
}
