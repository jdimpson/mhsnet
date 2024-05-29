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

#define	HDR_FLAGS_DATA
#include	"header.h"


#define	INDENT	13

static char *	convert _FA_((char *, char, char, char, char));
static void	printflags _FA_((FILE *, char *, HFtype, int, bool));
static char *	quote _FA_((char *));



/*
**	Print fields from message header.
*/

void
PrintHeader(fd, indent, env)
	FILE *			fd;
	int			indent;
	bool			env;	/* `true' if output should be as environment variables */
{
	register HdrFld *	fp;
	register char *		cp;
	register int		i;
	char			cc1;
	char			cc2;
	union {
		char	ns[ULONG_SIZE];
		char	ts[TIMESTRLEN];
	}			buf;

	if ( env )
	{
		cc1 = '=';
		cc2 = ';';
	}
	else
	{
		cc1 = ' ';
		cc2 = '\n';
	}

	cp = NullStr;

	for
	(
		fp = HdrFields.hdr_field, i = 0 ;
		i < NHDRFIELDS ;
		i++, fp++
	)
	{
		switch ( HdrConvs[i] )
		{
		case number_field:
			switch ( (HdrReason)i )
			{
			case ht_flags:
				printflags(fd, HdrDescs[i], (HFtype)fp->hf_number, indent, env);
				continue;
			case ht_tt:
			case ht_ttd:
				if ( !env )
				{
					cp = TimeString(fp->hf_number, buf.ts);
					break;
				}
			default:
				cp = buf.ns;
				(void)sprintf(cp, "%lu", (PUlong)fp->hf_number);
				break;
			}
			break;

		case byte_field:
			cp = buf.ns;
			cp[0] = fp->hf_byte;
			cp[1] = '\0';
			break;

		case string_field:
			switch ( (HdrReason)i )
			{
			case ht_env:
				cp = convert(fp->hf_string, ENV_US, cc1, ENV_RS, cc2);
				break;
			case ht_route:
				cp = convert(fp->hf_string, ROUTE_US, cc1, ROUTE_RS, cc2);
				break;
			default:
				cp = fp->hf_string;
				break;
			}
			if ( env )
				break;
			(void)fprintf(fd, "%*s ", INDENT+indent, HdrDescs[i]);
			ExpandFile(fd, cp, -(INDENT+indent+1));
			continue;
		}

		if ( env )
			(void)fprintf(fd, "%s='%s'\n", HdrDescs[i], quote(cp));
		else
			(void)fprintf(fd, "%*s %s\n", INDENT+indent, HdrDescs[i], cp);
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



static void
#ifdef	ANSI_C
printflags(FILE *fd, char *desc, HFtype flags, int indent, bool env)
#else	/* ANSI_C */
printflags(fd, desc, flags, indent, env)
	FILE *			fd;
	char *			desc;
	register HFtype		flags;
	int			indent;
	register bool		env;
#endif	/* ANSI_C */
{
	register char **	cpp;
	register int		bit;
	register bool		first = true;
	register char		space;

	if ( env )
	{
		(void)fprintf(fd, "%s='", desc);
		space = ' ';
	}
	else
	{
		(void)fprintf(fd, "%*s 0x%lx ", INDENT+indent, desc, (PUlong)flags);
		space = '|';
	}

	for ( bit = 1, cpp = HdrFlagsDescs ; *cpp != NULLSTR ; cpp++, bit <<= 1 )
		if ( bit & flags )
		{
			if ( first )
			{
				first = false;
				if ( !env )
					(void)putc('[', fd);
			}
			else
				(void)putc(space, fd);

			(void)fprintf(fd, "%s", *cpp);
		}

	if ( env )
		(void)putc('\'', fd);
	else
	if ( !first )
		(void)putc(']', fd);

	(void)putc('\n', fd);
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
