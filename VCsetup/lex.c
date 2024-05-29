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
#include	"grammar.h"

#include	<ctype.h>


static int	c;
static int	backslash(int);



int
yylex()
{
	Symbol *	s;
	char		sbuf[MAX_LINE_LEN];
	char *		p;
	int		n;

RESCAN:
	/* skip white space */
	while ( (c = getc(CallScript.fd)) == ' ' || c == '\t' )
		;

	if ( c == '\n' )
	{
		CallScript.lineno++;
		goto RESCAN;
	}

	if ( c == EOF )
		return 0;

	/* check for comment */
	if ( c == '/' )
	{
		if ( (c = getc(CallScript.fd)) != '*' )
		{
			ungetc(c, CallScript.fd);
			return '/';
		}

		while ( (c = getc(CallScript.fd)) != EOF )
		{
			while ( c == '*' )
				if ( (c = getc(CallScript.fd)) == '/' )
					goto RESCAN;
			if ( c == '\n')
				CallScript.lineno++;
		}

		return 0;
	}

	/* check for number */
	if ( isdigit(c) )
	{
		ungetc(c, CallScript.fd);
		(void)fscanf(CallScript.fd, "%d", &n);

		yylval.ysym = install(EmptyString, NUMBER);
		yylval.ysym->val.num = n;
		yylval.ysym->vtype = NUMBER;

		return NUMBER;
	}

	/* check for identifier */
	if ( isalpha(c) || c == '_' )
	{
		p = sbuf;
		do
		{
			if ( p >= sbuf + sizeof(sbuf) -1 )
			{
				*p = '\0';
				warning(english("name too long"), sbuf);
			}
			*p++=c;
		} while
		  (
			(c = getc(CallScript.fd)) != EOF
			&&
			(isalnum(c) || c == '_')
		  );
		ungetc(c, CallScript.fd);
		*p = '\0';
		s = install(sbuf, ID);
		yylval.ysym = lexsym = s;
		return s->type;
	}

	/* check for string */
	if ( c != '"' )
		return c;

	for ( p = sbuf ; (c = getc(CallScript.fd)) != '"' ; p++ )
	{
		if ( c == '\n' || c == EOF )
			warning(english("missing quote"), EmptyString);
		if ( p >= sbuf + sizeof(sbuf) - 1 )
		{
			*p = '\0';
			warning(english("string too long"), sbuf);
		}
		if ( c == '\\' )
			*p = backslash(c);
		else
			*p = c;
	}
	*p = 0;

	yylval.ysym = install(EmptyString, STRING);
	yylval.ysym->val.str = newstr(sbuf);
	yylval.ysym->vtype = STRING;

	return STRING;
}



static int
backslash(c)	/* get next char with \'s interpreted */
	register int	c;
{
	register char *	p;
	register int	i;
	register int	n;
	static char 	transtab[] = "b\bf\fn\nr\rt\t";

	c = getc(CallScript.fd);

	if ( islower(c) && (p = strchr(transtab, c)) != NULLSTR )
		return p[1];

	if ( c > '7' || c < '0' )
		return c;

	n = c - '0';

	for ( i = 0 ; i < 2 ; i++ )
	{
		c = getc(CallScript.fd);
		if ( c > '7' || c < '0' )
		{
			ungetc(c, CallScript.fd);
			break;
		}
		n = (n << 3) + c - '0';
	}

	return n;
}



void
yyerror(s)	/* report compile-time error */
	char *s;
{
	warning(s, EmptyString);
}



void
warning(s, t)
	char	*s;
	char	*t;
{
	MesgN(english("FAIL "), english("%s %s in %s near line %d"), s, t, CallScript.name, CallScript.lineno);

	while ( c != '\n' && c != EOF )
		c = getc(CallScript.fd);	/* flush input line */

	if ( c == '\n' )
		CallScript.lineno++;

	CallErr = true;
}
