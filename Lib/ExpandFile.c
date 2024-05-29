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



#define	MAXCOLS		78	/* Maximum columns used in expanded offset output */



/*
**	Expand a string to printable characters on passed fd.
**	Each line is preceded by a tab, or, if 'n' is -ve, by -n spaces.
*/

void
ExpandFile(fd, string, n)
	register FILE *	fd;
	char *		string;
	register int	n;	/* Length of string (+ve), or indent (-ve) */
{
	register char *	s;
	register int	c;
	register int	col;
	register int	indent = 8;

	if ( (s = string) == NULLSTR || *s == '\0' )
		goto out;

	if ( n < 0 )
	{
		indent = -n;
		n = 0;
	}

	if ( n == 0 )
	{
		col = indent;
		n = strlen(string);
	}
	else
		col = 0;

	for ( ; n-- ; col++ )
	{
		if ( col > (MAXCOLS-5) && n > 0 )
		{
			putc('\\', fd);
			putc('\n', fd);
			col = 0;
		}

		if ( col == 0 )
		{
			for ( c = indent ; c >= 8 ; c -= 8 )
				putc('\t', fd);
			while ( --c >= 0 )
				putc(' ', fd);
			col = indent;
		}

		if
		(
			(c = *s++) < ' '
			||
			c >= '\177'
		)
		{
			switch ( c )
			{
			case '\n':
				col = -1;
				break;

			case '\t':
				col |= 7;
				break;

			case '\r':
				c = 'r';
				putc('\\', fd);
				col++;
				break;
#ifdef	notdef
			case '\0':
				if ( s[1] < '0' || s[1] > '9' )
				{
					c = '0';
					putc('\\', fd);
					col++;
					break;
				}
#endif	/* notdef */
			default:
				(void)fprintf(fd, "\\%03o", c&0xff);
				col += 3;
				continue;
			}
		}
		else
		if ( c == '\\' )
		{
			putc(c, fd);
			col++;
		}

		putc(c, fd);
	}

	if ( col > 0 )
out:		putc('\n', fd);

	(void)fflush(fd);

	if ( ferror(fd) )
	{
		Syserror(CouldNot, WriteStr, english("pipe"));
		clearerr(fd);
	}
}
