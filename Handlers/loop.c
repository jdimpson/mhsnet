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


static char	sccsid[]	= "@(#)loop.c	1.14 05/12/16";

/*
**	Handle looping messages.
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
#include	"handlers.h"
#include	"header.h"
#include	"Passwd.h"
#include	"sysexits.h"


/*
**	Arguments.
*/

char *	Commandsfile;		/* Looping commands file */
char *	Destination;		/* Destination address for message */
char *	HomeAddress;		/* Name of this node */
char *	Invoker;		/* Program that detected loop */
char *	Name	= sccsid;	/* Program invoked name */
char *	LinkAddress;		/* Message was travelling via this link */
int	Traceflag;		/* Global tracing control */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_string(A, &Destination, 0, english("address"), 0),
	Arg_string(C, &Commandsfile, 0, english("commands"), 0),
	Arg_string(H, &HomeAddress, 0, english("home"), 0),
	Arg_string(I, &Invoker, 0, english("invoker"), OPTARG),
	Arg_string(L, &LinkAddress, 0, english("link"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_end
};

/*
**	Miscellaneous.
*/

char *	ExDestAddress;		/* Typeless Destination */
char *	Headerfile;		/* Last file named by commands */
char	NetRerouter[]	= "netreroute";

#define	Fprintf		(void)fprintf

void	loop _FA_((FILE *));



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	char *	errs;

	InitParams();

	DoArgs(argc, argv, Usage);

	GetNetUid();

	ExDestAddress = StripTypes(Destination);
	ExHomeAddress = StripTypes(HomeAddress);
	if ( LinkAddress != NULLSTR )
		ExLinkAddress = StripTypes(LinkAddress);

	if ( (errs = MailNCC(loop, NCC_ADMIN)) != NULLSTR )
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
	if ( type == file_cmd )
		Headerfile = newstr(val.cv_name);

	return true;
}



/*
**	Make up mail body about looping message from commands file.
*/

void
loop(fd)
	register FILE *	fd;
{
	register char *	errs;
	int		hfd;
	HdrReason	reason;
	static char *	sof = english("%s in \"%s\" << EOF\n");
	static char *	eof = english("EOF >> \"%s\"%s\n\n");

	Fprintf(fd, english("Subject: Network message needs re-routing\n"));

	putc('\n', fd);

	if ( Invoker != NULLSTR )
		Fprintf(fd, english("Requested by:\t\t\t\"%s\"\n"), Invoker);
	else
		Invoker = "program";

	if ( LinkAddress != NULLSTR )
		Fprintf(fd, english("Message routed via link:\t\"%s\"\n"), ExLinkAddress);

	putc('\n', fd);

	if ( LinkAddress != NULLSTR )
	{
		Fprintf
		(
			fd,
			english("The following message is looping via \"%s\".\n\nPlease choose another link and run:\n  %s %s <new_link>\n"),
			ExLinkAddress,
			NetRerouter,
			Destination
		);

		Fprintf
		(
			fd,
			english("or determine its route by:\n  netmap -lY %s\n"),
			Destination
		);

		Fprintf
		(
			fd,
			english("and then choose an adjacent site on the route and run:\n  %s %s <adjacent_site>\n\n"),
			NetRerouter,
			Destination
		);

		Fprintf
		(
			fd,
			english("If you manage to fix the routing on \"%s\"\nthen <new_link> may also be \"%s\".\n\n"),
			ExLinkAddress,
			ExLinkAddress
		);

		Fprintf
		(
			fd,
			english("If the address can't be reached, return the message to its source using:\n  %s -alyD\"%s\" -R\"%s\"\n\n"),
			STOP,
			Destination,
			english("Unreachable address.")
		);
	}
	else
	{
		Fprintf
		(
			fd,
			english("The following message has an unrecognised address:\n\t%s\n\n"),
			ExDestAddress
		);

		Fprintf
		(
			fd,
			english("Please choose a suitable site where the address will be understood,\nand run:\n  %s \"%s\" <site>\n\n"),
			NetRerouter,
			Destination
		);

		Fprintf
		(
			fd,
			english("Alternatively, return the message to its source using:\n  %s -alyD\"%s\" -R\"%s: \\\"%s\\\"\"\n\n"),
			STOP,
			Destination,
			english("Unrecognised address"),
			ExDestAddress
		);
	}

	if ( !ReadCmds(Commandsfile, getcommand) )
		Fprintf(fd, english("Commandsfile garbled!\n"));

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
