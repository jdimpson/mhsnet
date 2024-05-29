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
**	Argument parsing.
**
**	Returns number of arguments processed.
**	Errors call (*usagerr)(usagestring).
*/

#include	"global.h"
#include	"Args.h"
#include	"debug.h"


bool		ArgsIgnFlag;		/* All succeeding flags are strings */

#define	QUERYARGS	"-?"
#define	QUERYVERSION	"-#"

static char *	QueryArgs	= QUERYARGS;
static char *	QueryVersion	= QUERYVERSION;

typedef enum
{
	accept1, accept2, acceptbool, reject, error
}
		Result;

static AFuncv	FuncVal;
static Result	process_arg _FA_((Args *, char *, int, char **));
static char *	argusage _FA_((char *, Args *, int, char *));

char *		ArgTypes[MAXTYPE+1] =
{
	EmptyString,
	EmptyString,			/* BOOL */
	english("string"),		/* STRING */
	english("char"),		/* CHAR */
	english("integer"),		/* INT */
	english("float"),		/* FLOAT */
	english("integer"),		/* LONG */
	english("octal_number"),	/* OCT */
	english("hex_number"),		/* HEX */
	english("number"),		/* NUMBER */
	english("float"),		/* DOUBLE */
	english("integer"),		/* SHORT */
};




int
EvalArgs(argc, argv, usage, usagerr)
	int		argc;
	char *		argv[];
	Args *		usage;
	Funcp		usagerr;
{
	register Args *	up;
	register char *	argv0;
	register int	count	= 0;
	register int	flag	= -1;
	register int	nfcount	= 0;
	char *		val;
	int		errs	= 0;
	bool		ign_no_match = false;
	bool		ign_dups = false;
	char		buf[512];

	if ( AVersion == NULLSTR )
	{
		if ( (AVersion = Name) == NULLSTR )
			AVersion = EmptyString;
		else
		if ( strncmp(AVersion, "@(#)", 4) == STREQUAL )
			AVersion += 4;
	}

	Trace5(1,
		"EvalArgs(%d, \"%s\", 0x%lx, 0x%lx)",
		argc,
		argv[0],
		(PUlong)usage,
		(PUlong)usagerr
	);

	for ( up = usage ; up->type ; up++ )
		if ( up->opt & SPECIAL )
		{
			if ( up->type == IGNNOMATCH )
			{
				ign_no_match = true;
				Trace1(2, "ign_no_match");
			}
			if ( up->type == IGNDUPS )
			{
				ign_dups = true;
				Trace1(2, "ign_dups");
			}
		}

	for ( argv0 = argv[0] ; argc ; )
	{
		Trace2(3, "\"%s\"", argv0);

		if ( flag < 0 )
			flag = argv0[0] == '-';

		val = argv0;

		for ( up = usage ; up->type ; up++ )
			if ( up->posn == count && !(up->opt & (SPECIAL|REJECT|NFPOS)) )
				goto found;

		DODEBUG(if(ArgsIgnFlag)Trace1(2, "ArgsIgnFlag"));

		if ( flag && !ArgsIgnFlag )
		{
			Uchar	c = ((Uchar *)argv0)[1];

			Trace2(4, "\tflag '%c'", c?c:'-');

			for ( up = usage ; up->type ; up++ )
			{
				Trace2(9, "\tflagchar '%o'", up->flagchar);

				if
				(
					(
						up->flagchar == c
						||
						(up->opt & ANYFLAG)
						||
						(up->flagchar == NULLFLAG && argv0[0] == '-')
					)
					&&
					!(up->opt & (SPECIAL|REJECT))
					&&
					(
						!(up->opt & FOUND)
						||
						(up->opt & MANY)
						||
						ign_dups
					)
				)
				{
					if ( c && up->flagchar != NULLFLAG )
						val = (up->type==BOOL)?&argv0[1]:&argv0[2];
					goto found;
				}
			}
		}
		else
		{
			Trace3(4, "\tnon-flag \"%s\" nfc %d", argv0, nfcount);

			for ( up = usage ; up->type ; up++ )
				if
				(
					up->posn == nfcount
					&&
					(up->opt & (NFPOS|REJECT)) == NFPOS
				)
					goto found;

			for ( up = usage ; up->type ; up++ )
				if
				(
					(up->flagchar == NONFLAG || up->flagchar == NULLFLAG)
					&&
					!(up->opt & (SPECIAL|REJECT))
					&&
					(
						!(up->opt & FOUND)
						||
						(up->opt & MANY)
					)
				)
					goto found;
		}

		if
		(
			ign_no_match
			&&
			strcmp(argv0, QueryArgs) != STREQUAL
			&&
			strcmp(argv0, QueryVersion) != STREQUAL
		)
		{
			Trace1(2, "\tignored nomatch");
			goto out;
		}

		up = (Args *)0;
err:
		Trace1(2, "\terror");
		if ( (val = argusage(buf, up, count, argv0)) != NULLSTR )
			(void)(*usagerr)(val);
		errs++;
		goto out;

found:
		if ( (up->opt & (FOUND|MANY)) == FOUND && !ign_dups )
		{
			up = (Args *)0;
			goto err;
		}

		Trace2(3, "\trule %d", up-usage);

		switch ( process_arg(up, val, argc, argv) )
		{
		case accept2:	argv++;
				argc--;
		case accept1:	break;

		case acceptbool:argv0++;
				if ( up->flagchar == MINFLAG || argv0[1] == '\0' )
					break;
				up->opt |= FOUND;
				continue;

		case reject:	up->opt |= REJECT;
				continue;

		default:	goto err;
		}

		up->opt |= FOUND;
out:
		for ( up = usage ; up->type ; up++ )
			up->opt &= ~REJECT;

		if ( --argc <= 0 )
			break;

		argv0 = *++argv;
		count++;
		if ( !flag )
			nfcount++;
		flag = -1;
	}

	if ( errs )
		return -errs;

	for ( up = usage ; up->type ; up++ )
		if ( (up->opt & (SPECIAL|OPTARG|FOUND)) == 0 )
			return -1;

	return count;
}

static Result
process_arg(up, val, argc, argv)
	register Args *		up;
	register char *		val;
	int			argc;
	char *			argv[];
{
	register Pointer	vp = up->argp;
	register Result		retval = accept1;
	register int		zero = 0;
	PassVal			r;
	int			type = up->type;
	int			maybezero = 0;

	Trace5(2, "process_arg(up=%lx, val=%s, argc=%d, argv[0]=%s)", (PUlong)up, val, argc, argv[0]);

	if ( type != BOOL )
	{
		if ( up->opt & OPTVAL )
		{
/*			if ( val[0] == '\0' && argc > 1 && argv[1][0] != '-' )
**			{
**				val = argv[1];
**				retval = accept2;
**			}
*/			maybezero = (val[0] == '0' || val[0] == '\0');
		}
		else
		{
			while ( val[0] == '\0' && up->flagchar > SPECIALFLAGS )
			{
				if ( argc <= 1 || retval == accept2 || argv[1][0] == '-' )
					return error;
				val = argv[1];
				retval = accept2;
			}
			maybezero = val[0] == '0';
		}
	}

	switch ( type )
	{
	case BOOL:	r.b = true;
			if ( vp != (Pointer)0 )
				*(bool *)vp = r.b;
			retval = acceptbool;
			break;

	case STRING:	r.p = val;
			if ( vp != (Pointer)0 )
				*(char **)vp = r.p;
			break;

	case CHAR:	r.c = val[0];
			if ( vp != (Pointer)0 )
				*(char *)vp = r.c;
			break;

_int_:	case LONG:	if ( (r.l = atol(val)) == 0 )
				zero = 1;
_numb_:			if ( vp != (Pointer)0 )
				*(long *)vp = r.l;
_zero_:			if ( zero )
			{
				if ( !maybezero )
					return error;
				if ( (up->opt & OPTVAL) && val[0] != '\0' && val[0] != '0' )
					retval = acceptbool;
			}
			break;

	case FLOAT:	if ( (r.f = atof(val)) == 0 )
				zero = 1;
			if ( vp != (Pointer)0 )
				*(float *)vp = r.f;
			goto _zero_;

	case DOUBLE:	if ( (r.d = atof(val)) == 0 )
				zero = 1;
			if ( vp != (Pointer)0 )
				*(double *)vp = r.d;
			goto _zero_;

_hex_:	case HEX:	if ( (r.l = xtol(val)) == 0 )
				zero = 1;
			goto _numb_;

	case INT:	if ( (r.l = atoi(val)) == 0 )
				zero = 1;
			if ( vp != (Pointer)0 )
				*(int *)vp = r.l;
			goto _zero_;

	case SHORT:	if ( (r.s = atol(val)) == 0 )
				zero = 1;
			if ( vp != (Pointer)0 )
				*(short *)vp = r.s;
			goto _zero_;

	case NUMBER:	if ( !maybezero || val[0] == '\0' )
				goto _int_;
			if ( ((++val)[0] | 040) == 'x' )
			{
				++val;
				goto _hex_;
			}
	case OCT:	if ( (r.l = otol(val)) == 0 )
				zero = 1;
			goto _numb_;

	default:	return error;
	}

	if ( up->func != (AFuncp)0 )
	{
		FuncVal = (*up->func)(r, vp, val);

		if ( FuncVal == REJECTARG )
			return reject;
		else
		if ( FuncVal != ACCEPTARG )
			return error;
	}

	return retval;
}

static char *
argusage(buf, up, argn, argp)
	char *		buf;
	register Args *	up;
	int		argn;
	char *		argp;
{
	register char *	cp;
	register char *	np;
	static char	rd[] = english("thstndrdthththththth");

	Trace3(1, "argusage(%d, %s)", argn, argp);

	if ( up == (Args *)0 )
	{
		if ( strcmp(argp, QueryArgs) == STREQUAL )
		{
			ExplainArgs = true;
			return NULLSTR;
		}

		if ( strcmp(argp, QueryVersion) == STREQUAL )
		{
			ExplVersion = true;
			return NULLSTR;
		}

		if ( argn > 3 && argn < 21 )
			cp = rd;
		else
			cp = &rd[(argn%10)*2];

		(void)sprintf(buf, english("\"%s\" unexpected in %d%.2s argument"), argp, argn, cp);
		return buf;
	}

	if ( FuncVal > ARGERROR )
	{
		if ( up->posn != OPTPOS || up->flagchar == NONFLAG || up->flagchar == MINFLAG )
			(void)sprintf(buf, english("argument %d %s \"%s\""), argn, FuncVal, argp);
		else
			(void)sprintf(buf, english("flag '%c' %s"), up->flagchar, FuncVal);
		return buf;
	}

	if ( up->type == 0 || up->type > MAXTYPE )
		cp = "[BAD TYPE]";
	else
	if ( (cp = up->descr) == NULLSTR )
		cp = ArgTypes[up->type];

	if ( up->opt & OPTVAL )
		np = english("an optional");
	else
		np = strchr(english("aeiou"), cp[0]) != NULLSTR ? english("an") : english("a");

	if ( up->posn != OPTPOS )
		(void)sprintf(buf, english("argument %d must be %s <%s>"), argn, np, cp);
	else
	if ( up->flagchar == NONFLAG )
		(void)sprintf(buf, english("expected %s <%s> at argument %d"), np, cp, argn);
	else
	if ( up->flagchar == MINFLAG )
		(void)sprintf
		(
			buf,
			english("expected \"-%s%s%s\" at argument %d"),
			up->opt&OPTVAL?"[":EmptyString,
			cp,
			up->opt&OPTVAL?"]":EmptyString,
			argn
		);
	else
		(void)sprintf(buf, english("flag '%c' needs %s <%s>"), up->flagchar, np, cp);

	return buf;
}
