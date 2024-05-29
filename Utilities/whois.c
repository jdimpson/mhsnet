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


static char	sccsid[]	= "%W% %E%";

/*
**	Interrogate remote people list.
**
**	Any oddities are for backward compatiblity with Sun3.
*/

#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"exec.h"
#include	"handlers.h"
#include	"Passwd.h"
#include	"sysexits.h"


/*
**	CONFIGURATION.
*/

#define	MINPATTERN	2	/* Anything smaller is too general */

/*
**	Parameters set from arguments.
*/

char *	Destination;		/* Destination address */
char *	Name	= sccsid;	/* Program invoked name */
char *	Pattern;		/* Query */
bool	ReportID;		/* Report ID of query */
int	Traceflag;		/* Global tracing control */

AFuncv	getDest _FA_((PassVal, Pointer));	/* Check destination */
AFuncv	getSender _FA_((PassVal, Pointer));	/* Change sender */
AFuncv	getPat _FA_((PassVal, Pointer));	/* Check pattern */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(q, &ReportID, 0),
	Arg_string(S, 0, getSender, english("sender name"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_noflag(&Destination, getDest, english("pattern@address>|<address"), 0),
	Arg_noflag(0, getPat, english("pattern"), OPTARG),
#if	0
	Arg_minus(0, getFil_),	/* Later -- read query off stdin */
#endif	/* 0 */
	Arg_end
};

/*
**	Miscellaneous.
*/

Passwd	Me;			/* Who I am */
char *	ParentName;		/* Execute changes `Name' in child */

void	sendmsg _FA_((void));
void	set_uid _FA_((VarArgs *));



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	InitParams();

	GetNetUid();
	GetInvoker(&Me);

	DoArgs(argc, argv, Usage);

	if ( Pattern == NULLSTR )
	{
		(void)fprintf(stderr, "Pattern?\n");
		exit(EX_USAGE);
	}

	ParentName = Name;

	sendmsg();

	exit(EX_OK);
	return 0;
}



/*
**	Called from the errors routines to cleanup
*/

void
finish(error)
	int	error;
{
	(void)exit(error);
}



/*
**	Destination address.
**
**	Impose some administrative policy here.
*/
/*ARGSUSED*/
AFuncv
getDest(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	cp;
	AFuncv		retval;

	if ( (cp = strrchr(val.p, '@')) != NULLSTR )
	{
		*cp++ = '\0';
		*(char **)arg = cp;
		retval = getPat(val, (Pointer)0);
	}
	else
	{
		cp = val.p;
		retval = ACCEPTARG;
	}

	if ( strchr(cp, ATYP_BROADCAST) != NULLSTR )
		return (AFuncv)english("Broadcast disallowed.");	/* Asshole! */
		
	return retval;
}



/*
**	Query pattern.
**
**	Impose some administrative policy here.
*/
/*ARGSUSED*/
AFuncv
getPat(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( Pattern != NULLSTR )
		return (AFuncv)english("pattern already set");

	if ( (int)strlen(val.p) < MINPATTERN )
		return (AFuncv)english("pattern too small");

	Pattern = val.p;

	return ACCEPTARG;
}



/*
**	Change sender name.
*/
/*ARGSUSED*/
AFuncv
getSender(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.p[0] == '\0' )
		return ACCEPTARG;

	if
	(
		!(Me.P_flags & P_SU)
		&&
		strcmp(val.p, Me.P_name) != STREQUAL
	)
	{
		NoArgsUsage = true;
		return (AFuncv)english("no permission");
	}

	Me.P_name = val.p;

	return ACCEPTARG;
}



/*
**	Invoke `sendmsg' to transmit request to server host.
*/

void
sendmsg()
{
	register FILE * ofd;
	char *		errs;
	char		tbuf[NUMERICARGLEN];
	ExBuf		exbuf;

	FIRSTARG(&exbuf.ex_cmd) = concat(SPOOLDIR, LIBDIR, "sendmsg", NULLSTR);
	if ( ReportID )
		NEXTARG(&exbuf.ex_cmd) = "-q";
	NEXTARG(&exbuf.ex_cmd) = concat("-A", WHOISHANDLER, NULLSTR);
	if ( Traceflag )
		NEXTARG(&exbuf.ex_cmd) = NumericArg(tbuf, 'T', (Ulong)Traceflag);
	NEXTARG(&exbuf.ex_cmd) = concat("-E", Pattern, ";", Me.P_name, NULLSTR);
	NEXTARG(&exbuf.ex_cmd) = "-D";
	NEXTARG(&exbuf.ex_cmd) = Destination;

	ofd = (FILE *)Execute(&exbuf, set_uid, ex_pipe, SYSERROR);

	/*
	**	To be used by WHOISHANDLER in case of remote failure.
	*/

	(void)fprintf(ofd, "%s", Me.P_name);

	/*
	**	Clean up.
	*/

	(void)fflush(ofd);
	if ( ferror(ofd) )
		(void)SysWarn(CouldNot, WriteStr, "pipe");

	if ( (errs = ExClose(&exbuf, ofd)) != NULLSTR )
	{
		if ( ErrIsatty )
			exit(EX_DATAERR);	/* Any errors will be on <stderr> */
		Error(english("message insertion error:-\n%s"), CleanError(errs));
	}
}



/*
**	Change uid, and reset invoked name for errors.
*/

void
set_uid(vap)
	VarArgs *	vap;
{
	ARG(vap, 0) = ParentName;
	(void)setuid(geteuid());
}
