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
#define	STAT_CALL
#define	STDARGS

#include	"global.h"
#include	"debug.h"


typedef bool		(*bFuncp) _FA_((char *, char *));



/*
**	First argument is a pointer to a boolean function to perform
**	a match of the second argument with a list element.
**
**	Return TRUE if second argument matches an element in "list".
**
**	"list" may be any sequence of words separated by '\n' or '\t',
**	in any number of arguments terminated by a NULLSTR.
**
**	If "list" is a full path name, and the file can be read,
**	then the list will be read from the file.
*/

/*VARARGS*/
bool
#ifdef	ANSI_C
InList(bool(*funcp)(char *, char *), char *match, char *list, ...)
{
#else	/* ANSI_C */
InList(va_alist)
	va_dcl
{
	bFuncp		funcp;
	char *		match;
	char *		list;
#endif	/* ANSI_C */
	va_list		vp;
	register char *	cp;
	register int	count;
	char *		data;
	static char	delim[] = "\t\n";

#ifdef	ANSI_C
	va_start(vp, list);

	if ( funcp == (bool(*)(char *, char *))0 )
		return false;

	if ( match == NULLSTR )
		return false;

	if ( list == NULLSTR )
		return false;
#else	/* ANSI_C */
	va_start(vp);

	if ( (funcp = va_arg(vp, bFuncp)) == (bFuncp)0 )
		return false;

	if ( (match = va_arg(vp, char *)) == NULLSTR )
		return false;

	if ( (list = va_arg(vp, char *)) == NULLSTR )
		return false;
#endif	/* ANSI_C */

	Trace3(2, "InList(%s, %s, ...)", match, list);

	for ( count = 0, cp = list ; cp != NULLSTR ; cp = va_arg(vp, char *), count++ )
	{
		data = newstr(cp);

		cp = strtok(data, delim);
		do
			if ( (*funcp)(cp, match) )
			{
				free(data);
				return true;
			}
		while
			( (cp = strtok(NULLSTR, delim)) != NULLSTR );

		free(data);
	}

	va_end(vp);

	if ( count == 1 && (list[0] == '/' || strchr(list, '/') != NULLSTR ))
	{
		Trace2(2, "InList trying file \"%s\"", list);

		if ( list[0] != '/' )
			cp = concat(SPOOLDIR, list, NULLSTR);
		else
			cp = newstr(list);

		if ( (data = ReadFile(cp)) != NULLSTR )
		{
			free(cp);

			cp = strtok(data, delim);
			do
				if ( (*funcp)(cp, match) )
				{
					free(data);
					return true;
				}
			while
				( (cp = strtok(NULLSTR, delim)) != NULLSTR );

			free(data);
		}
		else
			free(cp);
	}

	Trace1(2, "InList no match");

	return false;
}
