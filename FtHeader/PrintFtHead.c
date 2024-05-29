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
#include	"ftheader.h"


#define	INDENT	13

static char *	convert _FA_((char *, char, char, char, char));
static char *	quote _FA_((char *));



/*
**	Print fields from message FTP header.
*/

void
PrintFtHeader(fd, indent, env)
	FILE *			fd;
	int			indent;
	bool			env;	/* `true' if output should be as environment variables */
{
	register char **	fp;
	register char *		cp;
	register int		i;
	char			cc1 = ' ';
	char			cc2 = '\n';

	for
	(
		fp = FthFields.fth_start, i = 0 ;
		i < NFTHFIELDS ;
		i++, fp++
	)
	{
		if ( env )
			(void)fprintf(fd, "FTP_%s='", FthDescs[i]);
		else
			(void)fprintf(fd, "%*s ", INDENT+indent, FthDescs[i]);
		cp = *fp;
		switch ( (FthReason)i )
		{
		case fth_type:
			(void)fprintf(fd, "%c", TYPEOF_FTH(*cp));
			if ( (*cp) & FTH_DATACRC )
				(void)fprintf(fd, " CRC16");
			if ( (*cp) & FTH_DATA32CRC )
				(void)fprintf(fd, " CRC32");
			if ( env )
				putc('\'', fd);
			putc('\n', fd);
			continue;
		case fth_files:
			if ( env ) { cc1 = ' '; cc2 = ';'; }
			cp = convert(cp, FTH_FDSEP, cc1, FTH_FSEP, cc2);
			break;
		case fth_to:
			if ( env ) { cc1 = '@'; cc2 = ' '; }
			cp = convert(cp, FTH_UDEST, cc1, FTH_UDSEP, cc2);
			break;
		default:
			break;
		}

		if ( env )
			(void)fprintf(fd, "%s'\n", quote(cp));
		else
			ExpandFile(fd, cp, -(INDENT+indent+1));
	}
}



static char *
#if	ANSI_C
convert(char *cp, char in1, char out1, char in2, char out2)
#else	/* ANSI_C */
convert(cp, in1, out1, in2, out2)
	register char *	cp;
	char		in1, out1, in2, out2;
#endif	/* ANSI_C */
{
	register char	c;
	register char *	op;
	register int	len;

	static char *	keep_cp;
	static int	keep_len;

	if ( cp == NULLSTR )
		return NULLSTR;

	if ( (len = strlen(cp)) == 0 )
		len = 64;		/* Something reasonable */
	else
		len = (len | 7) + 1;	/* Round up */

	if ( len > keep_len )
	{
		FreeStr(&keep_cp);
		keep_cp = Malloc(len);
		keep_len = len;
	}

	op = keep_cp;

	if ( *cp++ != in2 )
		--cp;

	do
		if ( (c = *cp++) == in1 )
			c = out1;
		else
		if ( c == in2 )
			c = out2;
	while
		( (*op++ = c) != '\0' );

	while ( op[-2] == '\n' )
	{
		op[-2] = '\0';
		op--;
	}

	return keep_cp;
}



static char *
quote(cmd)
	register char *	cmd;
{
	register int	c;
	register char *	np;
	char *		str;

	if ( strchr(cmd, '\'') == NULLSTR )
		return cmd;

	str = np = Malloc(strlen(cmd)*4+1);

	while ( (c = *cmd++) != '\0' )
	{
		/* Turn "'" into "'\''" */
		if ( c == '\'' )
		{
			*np++ = '\'';
			*np++ = '\\';
			*np++ = '\'';
		}
		*np++ = c;
	}

	*np++ = '\0';
	return str;
}
