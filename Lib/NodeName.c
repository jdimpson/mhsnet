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


/*
**	Routine to obtain local CPU ``node'' name.
*/

#define	FILE_CONTROL
#define	STAT_CALL

#include	"global.h"
#include	"debug.h"
#include	"lockfile.h"


static char	Node_Name[NODE_NAME_SIZE+1];


#if	SYSTEM > 0

#include	<sys/utsname.h>

char *
NodeName()
{
	register int	n;
	struct utsname	SysNames;

	if ( Node_Name[0] == '\0' )
	{
		if ( uname(&SysNames) != SYSERROR )
			(void)strncpy(Node_Name, SysNames.nodename, sizeof Node_Name-1);

		if ( (n = strlen(Node_Name)) > NODE_NAME_SIZE || n == 0 )
		{
			Warn(english("kernel nodename too %s"), n?english("long"):english("short"));

			if ( n )
				Node_Name[NODE_NAME_SIZE] = '\0';
			else
				strcpy(Node_Name, Unknown);
		}

		Trace2(1, "NodeName() ==> %s", Node_Name);
	}

	return Node_Name;
}

#else	/* SYSTEM > 0 */



#if	BSD4 >= 2

char *
NodeName()
{
	register int	n;

	if ( Node_Name[0] == '\0' )
	{
		(void)gethostname(Node_Name, sizeof(Node_Name)-1);

		if ( (n = strlen(Node_Name)) > NODE_NAME_SIZE || n == 0 )
		{
			Warn(english("kernel nodename too %s"), n?english("long"):english("short"));

			if ( n )
				Node_Name[NODE_NAME_SIZE] = '\0';
			else
				strcpy(Node_Name, Unknown);
		}

		Trace2(1, "NodeName() ==> %s", Node_Name);
	}

	return Node_Name;
}

#else	/* BSD4 >= 2 */


char *
NodeName()
{
	register int	n;
	register char *	nodename;

	if ( Node_Name[0] == '\0' )
	{
		if ( (nodename = ReadFile(NODENAMEFILE)) == NULLSTR )
		{
			(void)SysWarn
			(
				CouldNot, ReadStr,
				NODENAMEFILE==NULLSTR?"<NODENAMEFILE>":NODENAMEFILE
			);

			(void)strcpy(Node_Name, Unknown);
		}
		else
		if ( (n = strlen(nodename)) > NODE_NAME_SIZE || n == 0 )
		{
			Warn(english("kernel nodename too %s"), n?english("long"):english("short"));

			if ( n )
			{
				nodename[NODE_NAME_SIZE] = '\0';
				(void)strcpy(Node_Name, nodename);
			}
			else
				(void)strcpy(Node_Name, Unknown);

			free(nodename);
		}
		else
		{
			(void)strncpy(Node_Name, nodename, n);

			free(nodename);
		}

		Trace2(1, "NodeName() ==> %s", Node_Name);
	}

	return Node_Name;
}

#endif	/* BSD4 >= 2 */
#endif	/* SYSTEM > 0 */
