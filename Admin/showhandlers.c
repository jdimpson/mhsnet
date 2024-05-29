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


static char	sccsid[]	= "@(#)showhandlers.c	1.10 05/12/16";

/*
**	Show handlers.
*/

#define	STDIO

#include	"global.h"
#include	"Args.h"
#include	"handlers.h"
#include	"spool.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters set from arguments.
*/

char *	HndlrsFile;
char *	Name	= sccsid;
int	Traceflag;

AFuncv	getHandler _FA_((PassVal, Pointer));	/* Particular handler */

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_string(A, 0, getHandler, english("handler"), OPTARG),
/*	Arg_string(H, &HndlrsFile, 0, "handlers file", OPTARG),	*/
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_noflag(0, getHandler, english("handler"), OPTARG|MANY),
	Arg_end
};

/*
**	Miscellaneous
*/

bool	HndlrArg;		/* Specific handler */

#define	NAMELEN	12		/* Max. length of a handler name */
#define	DESCLEN	12		/* Max. length of a handler description */

#define	Printf	(void)printf

void	show_all _FA_((void));
void	show_hndlr _FA_((Handler *));



/*
**	Argument processing.
*/

int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	InitParams();

	setbuf(stdout, SObuf);

	DoArgs(argc, argv, Usage);

	if ( !HndlrArg )
		show_all();

	finish(EX_OK);
	return 0;
}



void
finish(error)
	int	error;
{
	exit(error);
}



/*
**	Particular handler.
*/
/*ARGSUSED*/
AFuncv
getHandler(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Handler *	hp;

	if ( val.p[0] == '\0' )
		return ACCEPTARG;

	if
	(
		(hp = GetHandler(val.p)) == (Handler *)0
		&&
		(hp = GetDHandler(val.p)) == (Handler *)0
	)
	{
		NoArgsUsage = true;
		return (AFuncv)english("unknown handler");
	}

	show_hndlr(hp);

	HndlrArg = true;

	return ACCEPTARG;
}



/*
**	Show all handlers.
*/

void
show_all()
{
	register Handler *	hp;
	register int		i;

	/** Build list of all handlers. **/

	GetHandler(EmptyString);

	/** Show each. **/

	for ( hp = Handlers, i = HandlCount ; --i >= 0 ; hp++ )
		show_hndlr(hp);;
}



/*
**	Show details for a handler.
*/

void
show_hndlr(hp)
	register Handler *	hp;
{
	char			name[NAMELEN+1];

	(void)sprintf(name, "%.*s:", NAMELEN-1, hp->handler);
	Printf("%-*s", NAMELEN, name);

	Printf("%*s", DESCLEN, hp->descrip);

	Printf(" proto:%c", hp->proto_type);
	Printf(" rstrct:%c", hp->restricted);
	if ( hp->quality == CHOOSE_QUALITY )
		Printf(" qual:*");
	else
		Printf(" qual:%c", hp->quality);
	Printf(" ord:%c", hp->order);
	Printf(" nice:%-2d", hp->nice);

	if ( hp->router )
	{
		Printf(" (subrouter");
		if ( hp->duration )
			Printf("%c%d)", DURATION_SEP, hp->duration);
		else
			Printf(")");
	}

	if ( hp->list != NULLSTR )
		Printf(" addr:\"%s\"", hp->list);

	if ( hp->timeout != 0 )
		Printf(" timeout:%d", hp->timeout);

	putchar('\n');
}
