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


static char	sccsid[]	= "@(#)fetchfile.c	1.11 09/07/30";

/*
**	Fetch files from remote file-server.
**
**	Any oddities are for backward compatiblity with Sun3.
*/

#define	MAXFILES	(MAXVARARGS-4)

#define	FILE_CONTROL
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
**	Parameters set from arguments.
*/

bool	CRC;			/* Perform CRC on returned data */
char *	Destination;		/* Destination address */
bool	List;			/* Return file names, rather than data */
char *	Name	= sccsid;	/* Program invoked name */
bool	NoErrors;		/* Don't return message on error detection */
bool	NoMail;			/* Don't signal file delivery */
bool	Recurse;		/* Allow directory recursion with ``List'' */
bool	ReportID;		/* Report ID generated on stdout */
int	Traceflag;		/* Global tracing control */
bool	Verbose;		/* Return more data with ``List'' */

VarArgs Fargs;			/* List of names of files to return */

AFuncv	getDest _FA_((PassVal, Pointer));	/* Check destination */
AFuncv	getFil_ _FA_((PassVal, Pointer));	/* File names from stdin */
AFuncv	getFile _FA_((PassVal, Pointer));	/* File name */
AFuncv	getSender _FA_((PassVal, Pointer));	/* Change sender */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(c, &CRC, 0),
	Arg_bool(e, &NoErrors, 0),
	Arg_bool(l, &List, 0),
	Arg_bool(q, &ReportID, 0),
	Arg_bool(m, &NoMail, 0),
	Arg_bool(r, &Recurse, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_string(S, 0, getSender, english("requester name"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_noflag(&Destination, getDest, english("source[,source...]"), 0),
#if	0
	Arg_minus(0, getFil_),	/* Later -- read list of file names off stdin */
#endif	/* 0 */
	Arg_noflag(0, getFile, english("file|dir"), MANY),
	Arg_end
};

/*
**	Miscellaneous.
*/

Passwd	Me;			/* Who I am */
char *	ParentName;		/* Execute changes ``Name'' in child */

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

	ParentName = Name;

	if ( Verbose )
		List = true;

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
	if ( strchr(val.p, ATYP_BROADCAST) != NULLSTR )
		return (AFuncv)english("Broadcast disallowed.");	/* Asshole! */

	return ACCEPTARG;
}



/*
**	Add a file name into appropriate list, and extract details.
*/
/*ARGSUSED*/
AFuncv
getFile(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( NARGS(&Fargs) >= MAXFILES )
		return (AFuncv)english("Too many files to fetch.");

	NEXTARG(&Fargs) = val.p;

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
**	Output string + '\0'.
*/

void
putstr(fd, str)
	register FILE *	fd;
	register char *	str;
{
	do
		putc(*str, fd);
	while
		( *str++ );
}



/*
**	Invoke ``sendmsg'' to transmit request to server host.
*/

void
sendmsg()
{
	register int	i;
	register FILE * ofd;
	char *		errs;
	char		tbuf[NUMERICARGLEN];
	ExBuf		exbuf;

	FIRSTARG(&exbuf.ex_cmd) = concat(SPOOLDIR, LIBDIR, "sendmsg", NULLSTR);
	if ( ReportID )
		NEXTARG(&exbuf.ex_cmd) = "-q";
	if ( NoErrors )
		NEXTARG(&exbuf.ex_cmd) = "-r";
	NEXTARG(&exbuf.ex_cmd) = concat("-A", FSVRHANDLER, NULLSTR);
	if ( Traceflag )
		NEXTARG(&exbuf.ex_cmd) = NumericArg(tbuf, 'T', (Ulong)Traceflag);
	NEXTARG(&exbuf.ex_cmd) = "-D";
	NEXTARG(&exbuf.ex_cmd) = Destination;

	ofd = (FILE *)Execute(&exbuf, set_uid, ex_pipe, SYSERROR);

	/*
	**	Format request as body of message.
	*/

	putstr(ofd, Me.P_name);

	if ( List )			/* These should all have been ``options'' in SUN3 version */
	{
		if ( Verbose )
			putstr(ofd, "ListVerbose");	/* Silly! -- SUN3 should have had a 'V' option */
		else
			putstr(ofd, "List");
	}
	else
		putstr(ofd, "SendFile");

	if ( CRC || NoMail || Recurse || ReportID )
	{
		putc('-', ofd);		/* Needed 'cos SUN3 passes whole option to sendfile(1)! */
		if ( CRC )
			putc('C', ofd);
		if ( ReportID )
			putc('I', ofd);
		if ( Recurse )
			putc('R', ofd);
		if ( NoMail )
			putc('S', ofd);
	}
	putc('\0', ofd);		/* End of options */

	putc('\0', ofd);		/* File name marker (also superfluous!) */

	for ( i = 0 ; i < NARGS(&Fargs) ; i++ )
		putstr(ofd, ARG(&Fargs, i));

	putc('\0', ofd);		/* What was wrong with EOF, you might ask? */

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
