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
#include	"call.h" 
#include	"grammar.h"


static struct key
{
	char *	name;
	int	kval;
}
	keywords[] = 
{
	{"call",	CALL},
	{"close",	CLOSE},
	{"crc",		CRC},
	{"daemon",	DAEMON},
	{"device",	DEVICE},
	{"execdaemon",	EXECD},
	{"expect",	EXPECT},
	{"fail",	FAIL},
	{"forkcommand",	FORKC},
	{"forkdaemon",	FORKD},
	{"match",	MATCH},
	{"mode",	MODE},
	{"monitor",	MONITOR},
	{"next",	NEXT},
	{"open",	OPEN},
	{"read",	READ},
	{"readchar",	READCHAR},
	{"retry",	RETRY},
	{"return",	RETURN},
	{"set",		SET},
	{"shell",	SHELL},
	{"shift",	SHIFT},
	{"sleep",	SLEEP},
	{"slowwrite",	SLOWWRITE},
	{"test",	TEST},
	{"timeout",	TIME_OUT},
	{"trace",	TRACE},
	{"write",	WRITE},
	{NULLSTR,	0},
};

#define	Nkeywords	((sizeof keywords)/(sizeof (struct key)) - 1)

static void	InsStr _FA_((char *, char *));



/*
**	Function to install keywords in the symbol table.
*/

void
InitInterp()
{
	register int	i;

	for ( i = 0 ; keywords[i].name ; i++ )
		(void)install(keywords[i].name, keywords[i].kval);

	InsStr("DEVFAIL", DEVFAIL);
	InsStr("DEVOK", DEVOK);
	InsStr("EOF", EOFSTR);
	InsStr("HOMENAME", HomeName);
	InsStr("INPUT", input);
	InsStr("LINKDIR", LinkDir);
	InsStr("SPOOLDIR", SPOOLDIR);
	InsStr("TERMINATE", TERMINATE);
	InsStr("TIMEOUT", TIMEOUT);
	InsStr("UNDEFINED", UNDEFINED);
	InsStr("VERSION", Version);

	RESULTsym = InstallId("RESULT", UNDEFINED);
	DELIMsym = InstallId("DELIMCHARS", delim);
}



/*
**	Install command line definition.
*/

Symbol *
InstallId(s, v)
	char *			s;
	char *			v;
{
	register Symbol *	sym;

	sym = install(s, ID);
	sym->val.str = v;
	sym->vtype = STRING;

	return sym;
}



/*
**	Install string definition.
*/

static void
InsStr(s, v)
	char *			s;
	char *			v;
{
	register Symbol *	sym;

	sym = install(s, STRING);
	sym->val.str = v;
	sym->vtype = STRING;
}



#if	DEBUG >= 2
int
#if	ANSI_C
keycmp(const void *p1, const void *p2)
#else	/* ANSI_C */
keycmp(p1, p2)
	char *	p1;
	char *	p2;
#endif	/* ANSI_C */
{
	return ((struct key *)p1)->kval - ((struct key *)p2)->kval;
}



char *
command_name(code)
	int		code;
{
	struct key *	k;
	struct key	val;
	static bool	sorted;
	static char	num[ULONG_SIZE+1];

	if ( !sorted )
	{
		qsort
		(
			(char *)keywords,
			(unsigned)Nkeywords,
			sizeof(struct key),
			keycmp
		);

		sorted = true;
	}

	val.kval = code;

	k = (struct key *)bsearch
			    (
				(char *)&val,
				(char *)keywords,
				(unsigned)Nkeywords,
				sizeof(struct key),
				keycmp
			    );

	if ( k != (struct key *)0 )
		return k->name;

	(void)sprintf(num, "[%d]", code);
	return num;
}
#endif	/* DEBUG */
