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


static char	PATH[]	= "PATH=/bin:/usr/bin";
static char *	Envs[]	=
{
#	ifdef	APOLLO
	"NODEID",
	"SYSTYPE",
#	endif	/* APOLLO */
#	if	SYSTEM > 0
	"TZ",
#	endif	/* SYSTEM > 0 */
	NULLSTR
};
static char *	Env[(sizeof Envs)/(sizeof Envs[0])+1];



/*
**	Sanitize environment.
*/

char **
StripEnv()
{
	register char *	cp;
	register char *	ep;
	register char **cpp;
	register char **epp;

	for ( epp = Env, cpp = Envs ; (cp = *cpp++) != NULLSTR ; )
		if ( (ep = getenv(cp)) != NULLSTR && ep[0] != '\0' )
			*epp++ = concat(cp, "=", ep, NULLSTR);

	*epp++ = PATH;
	*epp = NULLSTR;

	return Env;
}
