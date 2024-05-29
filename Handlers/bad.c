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


static char	sccsid[]	= "@(#)bad.c	1.11 05/12/16";

/*
**	Handle bad messages.
*/

#define	FILE_CONTROL
#define	STDIO

#include	"global.h"
#include	"Args.h"
#include	"address.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"expand.h"
#include	"header.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"sysexits.h"


/*
**	Arguments.
*/

char *	Commandsfile;		/* Bad commands file */
char *	HomeAddress;		/* Name of this node */
char *	Invoker;		/* Program that detected error */
char *	Name	= sccsid;	/* Program invoked name */
char *	LinkAddress;		/* Message had arrived from this link */
char *	SourceAddress;		/* Message had arrived from this source */
int	Traceflag;		/* Global tracing control */

AFuncv	getStr _FA_((PassVal, Pointer));	/* Save copy of string */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_string(C, &Commandsfile, 0, english("commands"), 0),
	Arg_string(E, &ErrString, getStr, english("error"), OPTARG),
	Arg_string(H, &HomeAddress, 0, english("home"), 0),
	Arg_string(I, &Invoker, 0, english("invoker"), OPTARG),
	Arg_string(L, &LinkAddress, 0, english("link"), OPTARG),
	Arg_string(S, &SourceAddress, 0, english("source"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_end
};

/*
**	Miscellaneous.
*/

char *	Datafile;		/* First file named by commands */
char *	Headerfile;		/* Last file named by commands */
FILE *	MailFd;			/* File descriptor of pipe to mail program */

#define	Fprintf		(void)fprintf

void	bad _FA_((FILE *));



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	char *	errs;

	InitParams();

	DoArgs(argc, argv, Usage);

	GetNetUid();

	ExHomeAddress = StripTypes(HomeAddress);
	ExSourceAddress = ExHomeAddress;	/* Source of mail is here */
	if ( LinkAddress != NULLSTR )
		ExLinkAddress = StripTypes(LinkAddress);

	if ( (errs = MailNCC(bad, NCC_ADMIN)) != NULLSTR )
	{
		/*
		**	Not much one can do about this, except report the evidence.
		*/

		Error(StringFmt, errs);
		free(errs);
	}

	exit(EX_OK);
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
**	Process a command from commands file.
*/

bool
getcommand(type, val)
	CmdT		type;
	CmdV		val;
{
	register char *	cp;
	register CmdC	conv;

	if ( (unsigned)type > (unsigned)last_cmd )
		conv = error_type;
	else
		conv = CmdfTypes[(int)type];

	switch ( conv )
	{
	case number_type:
		cp = CmdToString(type, val.cv_number);
		break;

	case string_type:
		cp = val.cv_name;
		break;

	default:
		Fprintf(MailFd, english("\tunrecognised command type [%d]\n"), (int)type);
		return true;
	}

	switch ( type )
	{
	case bounce_cmd:
		Fprintf(MailFd, "\t%-*s ", MAX_CMD_NAMES_LENGTH, CmdfDescs[(int)type]);
		ExpandFile(MailFd, cp, -(8+MAX_CMD_NAMES_LENGTH+1));
		break;

	case file_cmd:
		Headerfile = newstr(cp);
		if ( Datafile == NULLSTR )
			Datafile = Headerfile;
	default:
		Fprintf(MailFd, "\t%-*s %s\n", MAX_CMD_NAMES_LENGTH, CmdfDescs[(int)type], cp);
		break;
	}

	return true;
}



/*
**	Copy new string to target.
*/

/*ARGSUSED*/
AFuncv
getStr(val, arg)
	PassVal		val;
	Pointer		arg;
{
	*(char **)arg = newstr(val.p);
	return ACCEPTARG;
}



/*
**	Make up mail body about bad message/commands file.
*/

void
bad(fd)
	register FILE *	fd;
{
	register char *	errs;
	int		hfd;
	HdrReason	reason;
	static char *	sof = english("%s from \"%s\" << EOF\n");
	static char *	eof = english("EOF >> \"%s\"%s\n\n");

	MailFd = fd;

	Fprintf(fd, english("Subject: Network problem needs attention.\n"));

	putc('\n', fd);

	if ( Invoker != NULLSTR )
		Fprintf(fd, english("Requested by:\t\t\t\"%s\"\n"), Invoker);
	else
		Invoker = "program";

	putc('\n', fd);

	if ( SourceAddress != NULLSTR )
		Fprintf(fd, english("Message came from:\t\t\"%s\"\n"), StripTypes(SourceAddress));

	if ( LinkAddress != NULLSTR )
		Fprintf(fd, english("Message received/transmitted on link:\t\"%s\"\n"), ExLinkAddress);

	putc('\n', fd);

	Fprintf
	(
		fd,
		english("Look at the details that follow, fix the problem if possible,\nand then run \"%s%s%s\" to re-route the message.\n"),
		SPOOLDIR,
		LIBDIR,
		CHECKBAD
	);

	putc('\n', fd);

	if ( ErrString != NULLSTR )
	{
		static char	desc1[] = english("Concise error message");
		static char	desc2[] = english("Error message");

		errs = StripErrString(ErrString);
		if ( strcmp(errs, ErrString) != STREQUAL )
		{
			char *	ep;

			Fprintf(fd, sof, desc1, Invoker);
			ep = CleanError(errs);
			if ( (ErrSize = strlen(ep)) > 0 )
				ExpandFile(fd, ep, ErrSize);
			Fprintf(fd, eof, Invoker, EmptyString);
		}
		free(errs);

		Fprintf(fd, sof, desc2, Invoker);
		if ( (ErrSize = strlen(ErrString)) > 0 )
			ExpandFile(fd, ErrString, ErrSize);
		Fprintf(fd, eof, Invoker, EmptyString);
	}

	Fprintf(fd, sof, english("Commands"), Commandsfile);

	if ( ReadCmds(Commandsfile, getcommand) )
		ErrString = newstr(EmptyString);
	else
		ErrString = newstr(english(" (Commands garbled!)"));

	Fprintf(fd, eof, Commandsfile, ErrString);

	if ( Datafile != NULLSTR )
	{
		Fprintf(fd, sof, english("Start of message"), Datafile);

		if ( (errs = ReadErrFile(Datafile, true)) != NULLSTR )
		{
			ExpandFile(fd, errs, ErrSize);
			free(errs);
		}

		Fprintf(fd, eof, Datafile, ErrString);
	}

	if ( Headerfile != NULLSTR )
	{
		Fprintf(fd, sof, english("Message header"), Headerfile);

		if ( (hfd = open(Headerfile, O_READ)) == SYSERROR )
			ErrString = newstr(english(" (Can't open file.)"));
		else
		if ( (reason = ReadHeader(hfd)) != hr_ok )
		{
			if ( (errs = ReadErrFile(Headerfile, false)) != NULLSTR )
			{
				ExpandFile(fd, errs, ErrSize);
				free(errs);
			}

			ErrString = concat(english(" (Header "), HeaderReason(reason), english(" error.)"), NULLSTR);
			(void)close(hfd);
		}
		else
		{
			PrintHeader(fd, 0, false);
			ErrString = newstr(EmptyString);
			(void)close(hfd);
		}

		Fprintf(fd, eof, Headerfile, ErrString);
	}
}
