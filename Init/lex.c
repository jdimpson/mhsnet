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
#include	"Netinit.h"

#include	<ctype.h>


extern FILE *	fin;

static int	c;

int		lineno;
int		lexlen;
char *		lexval;

static int	backslash(void);



int
lex()
{
	char	sbuf[MAX_LINE_LEN];
	char *	p;
	int	val;

RESCAN:
	/* skip white space and comments */
	while ( (c = getc(fin)) == ' ' || c == '\t' )
		;

	if ( c == '#' )
		while ( (c != '\n') && (c != EOF) )
			c = getc(fin);

	while ( c == '\n' )
	{
		lineno++;
		if ( (c = getc(fin)) == ' ' || c == '\t' )
			goto RESCAN;
		else
		if ( c != '\n' )
		{
			(void)ungetc(c, fin);
			return END;
		}
	}
	if ( c == EOF )
		return 0;

	p = sbuf;

	if ( c != '"' )
	{
		/* check for name */
		do
		{
			if ( p >= sbuf + sizeof(sbuf) - 1 )
				warning("name too long");
			else
				*p++ = c;
		}
		while
			(
				(c = getc(fin)) != EOF
				&&
				c != ' '
				&&
				c != '\t'
				&&
				c != '\n'
			);
		(void)ungetc(c, fin);
		val = NAME;
	}
	else
	{
		/* check for string */
		while ( (c = getc(fin)) != '"' )
		{
			if ( c == '\n' || c == EOF )
				warning("missing quote");
			else
			if ( p >= sbuf + sizeof(sbuf) - 1 )
				warning("string too long");
			else
				*p++ = backslash();
		}
		val = NSTRING;
	}

	*p = '\0';

	FreeStr(&lexval);

	lexlen = p - sbuf;
	lexval = Malloc(lexlen+1);
	(void)strcpy(lexval, sbuf);

	return val;
}



static int
backslash()	/* get next char with \'s interpreted */
{
	char *		p;
	static char	transtab[] = "b\bf\fn\nr\rt\t";

	if ( c != '\\' )
		return c;
	c = getc(fin);
	if ( islower(c) && (p = strchr(transtab, c)) != NULLSTR )
		return p[1];
	return c;
}



void
warning(s)
	char *	s;
{
	Error("%s in %s near line %d", s, InitFile, lineno);

	while ( c != '\n' && c != EOF )
		c = getc(fin);	/* flush input line */
	if ( c == '\n' )
		lineno++;
}
