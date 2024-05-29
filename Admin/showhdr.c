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


static char	sccsid[]	= "@(#)showhdr.c	1.16 05/12/16";

/*
**	Print out headers from message files.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"header.h"
#include	"ftheader.h"
#include	"link.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sub_proto.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Arguments.
*/

bool	DataOut;		/* Copy data part of file to stdout */
bool	IgnErrors;		/* Ignore header errors */
char *	Name	= sccsid;	/* Program invoked name */
bool	RouteMsg;		/* Make commandsfile for message */
bool	ShellEnv;		/* Print headers as shell variable assignments */
bool	SizeNonZero;		/* Exist status `true' if data size > 0 */
int	Traceflag;		/* Global tracing control */
bool	Truncate;		/* Truncate headers */

AFuncv	getFile _FA_((PassVal, Pointer, char *));
AFuncv	getLink _FA_((PassVal, Pointer, char *));	/* Get link for commandsfile */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(a, &ShellEnv, 0),
	Arg_bool(d, &DataOut, 0),
	Arg_bool(e, &IgnErrors, 0),
	Arg_bool(r, &RouteMsg, 0),
	Arg_bool(s, &SizeNonZero, 0),
	Arg_bool(t, &Truncate, 0),
	Arg_string(L, 0, getLink, "link", OPTARG),
	Arg_int(T, &Traceflag, getInt1, "level", OPTARG|OPTVAL),
	Arg_noflag(0, getFile, "message-file", MANY),
	Arg_end
};

/*
**	Miscellaneous.
*/

int	ExitVal	= EX_OK;
char *	LinkAddress;
char *	Headerfile;		/* File containing message header */
Ulong	MsgLength;		/* Length of Headerfile */

#define	Fprintf		(void)fprintf

void	RouteMessage _FA_((int));



int
main(argc, argv)
	register int	argc;
	register char *	argv[];
{
	Passwd		me;

	InitParams();
	GetNetUid();
	GetInvoker(&me);

	if ( !(me.P_flags & P_SU) )
	{
		Warn(english("No permission."));
		exit(EX_NOPERM);
	}

	DoArgs(argc, argv, Usage);

	exit(ExitVal);
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
**	Process a message file.
*/

AFuncv
getFile(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	register char *	cp;
	int		hfd;
	bool		verbose;
	HdrReason	reason;
	FthReason	fthreason;
	static char *	sof = "%s in \"%s\" <<EOF\n";
	static char *	eof = "EOF%s\n\n";

	if ( RouteMsg && Truncate )
		return (AFuncv)english("route/truncate conflict");

	if ( (Headerfile = val.p) == NULLSTR || Headerfile[0] == '\0' )
		return REJECTARG;

	if ( !DataOut && !RouteMsg && !ShellEnv && !Truncate )
		verbose = true;
	else
		verbose = false;

	if ( verbose )
		Fprintf(stdout, sof, english("Message header"), Headerfile);

	if ( IgnErrors )
		InitFtHeader();

	if ( (hfd = open(Headerfile, O_READ)) == SYSERROR )
		ErrString = newstr(english(" (Can't open file.)"));
	else
	if ( (reason = ReadHeader(hfd)) != hr_ok && !IgnErrors )
	{
		if ( verbose && (cp = ReadErrFile(Headerfile, false)) != NULLSTR )
		{
			ExpandFile(stdout, cp, ErrSize);
			free(cp);
		}

		ErrString = concat(english(" (Header "), HeaderReason(reason), english(" error.)"), NULLSTR);

		(void)close(hfd);
	}
	else
	{
		MsgLength = DataLength + HdrLength;

		if ( !DataOut && !RouteMsg && !Truncate )
			PrintHeader(stdout, 0, ShellEnv);

		ErrString = newstr(EmptyString);

		if ( HdrSubpt == FTP && DataLength > 0 )
		{
			if ( verbose )
				Fprintf(stdout, english("\n\tFTP message header:\n"));

			if ( (fthreason = ReadFtHeader(hfd, DataLength)) != fth_ok && !IgnErrors )
			{
				if ( verbose && (cp = ReadErrFile(Headerfile, false)) != NULLSTR )
				{
					ExpandFile(stdout, cp, ErrSize);
					free(cp);
				}

				ErrString = concat(english(" (FTP header "), FTHREASON(fthreason), english(" error.)"), NULLSTR);
			}
			else
			{
				if ( !DataOut && !RouteMsg && !Truncate )
					PrintFtHeader(stdout, 8, ShellEnv);
				else
					DataLength = FtDataLength;

				ErrString = newstr(EmptyString);
			}
		}

		if ( DataLength > 0 )
		{
			if ( Truncate )
			{
#				if	BSD4 >= 2 || SYSTEM >= 5 && SYSVREL >= 4
				truncate(Headerfile, (off_t)DataLength);
#				else	/* BSD4 >= 2 || SYSTEM >= 5 && SYSVREL >= 4 */
				Error(english("Header truncation not supported"));
#				endif	/* BSD4 >= 2 || SYSTEM >= 5 && SYSVREL >= 4 */
			}
			else
			if ( DataOut && strlen(ErrString) == 0 )
			{
				(void)lseek(hfd, (long)0, 0);
				CopyFdToFd(hfd, Headerfile, fileno(stdout), "stdout", DataLength);
			}

			ExitVal = EX_OK;
		}
		else
		if ( SizeNonZero )
			ExitVal = EX_DATAERR;

		if ( RouteMsg )
			RouteMessage(hfd);

		(void)close(hfd);
	}

	if ( ShellEnv )
	{
		if ( ErrString[0] != '\0' )
			Fprintf(stdout, "ERROR='%s'\n", ErrString);
	}
	else
	if ( verbose )
		Fprintf(stdout, eof, ErrString);
	else
	if ( (int)strlen(ErrString) > 0 )
		Fprintf(stderr, "%s\n", ErrString);

	return ACCEPTARG;
}



/*
**	Find address of link for commandsfile.
*/

/*ARGSUSED*/
AFuncv
getLink(val, arg, str)
	PassVal		val;
	Pointer		arg;
	char *		str;
{
	Addr *		ap;
	Link		link;

	ap = StringToAddr(val.p, NO_STA_MAP);

	if ( FindLink(TypedName(ap), &link) )
	{
		FreeAddr(&ap);
		LinkAddress = newstr(link.ln_rname);
		return ACCEPTARG;
	}

	FreeAddr(&ap);

	return (AFuncv)english("Unknown link");
}



/*
**	Create a commandsfile for message, and spool for router.
*/

void
RouteMessage(fd)
	register int	fd;
{
	register CmdV	cmdv;
	char		qual;
	char *		cmds_route_file;
	char *		cmds_temp_file;
	Ulong		ttime;
	CmdHead		commands;
	char		template[UNIQUE_NAME_LENGTH+1];
	struct stat	dstat;

	while ( fstat(fd, &dstat) == SYSERROR )
		Syserror(CouldNot, StatStr, Headerfile);

	if ( (ttime = Time - dstat.st_mtime) > Time )
		ttime = 0;

	SetUser(NetUid, NetGid);
	if ( geteuid() != NetUid )
		Error(english("No permission."));;

	(void)sprintf(template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	cmds_route_file = concat(SPOOLDIR, ROUTINGDIR, template, NULLSTR);
	cmds_temp_file = concat(SPOOLDIR, WORKDIR, template, NULLSTR);

	/*
	**	Make a link to message.
	*/

	(void)UniqueName(cmds_temp_file, CHOOSE_QUALITY, OPTNAME, MsgLength, Time);
	make_link(Headerfile, cmds_temp_file);

	/*
	**	Make commands for linked message.
	*/

	InitCmds(&commands);

	(void)AddCmd(&commands, file_cmd, (cmdv.cv_name = cmds_temp_file, cmdv));
	(void)AddCmd(&commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&commands, end_cmd, (cmdv.cv_number = MsgLength, cmdv));
	(void)AddCmd(&commands, unlink_cmd, (cmdv.cv_name = cmds_temp_file, cmdv));
	if ( LinkAddress != NULLSTR )
		(void)AddCmd(&commands, linkname_cmd, (cmdv.cv_name = LinkAddress, cmdv));
	(void)AddCmd(&commands, traveltime_cmd, (cmdv.cv_number = ttime, cmdv));

	/*
	**	Make a command file for linked message in a temporary directory.
	*/

	fd = create(UniqueName(cmds_temp_file, CHOOSE_QUALITY, OPTNAME, MsgLength, Time));

	if ( !WriteCmds(&commands, fd, cmds_temp_file) )
		Fatal3(CouldNot, english("write commands to"), cmds_temp_file);

	(void)close(fd);

	FreeCmds(&commands);

	/*
	**	Move commands into routing directory.
	*/

#	if	PRIORITY_ROUTING == 1
	if ( (qual = HdrQuality) == STATE_QUALITY )
		MsgLength = 0;	/* State messages */
#	else	/* PRIORITY_ROUTING == 1 */
	qual = STATE_QUALITY;
	MsgLength = 0;
#	endif	/* PRIORITY_ROUTING == 1 */

	move(cmds_temp_file, UniqueName(cmds_route_file, qual, NOOPTNAME, MsgLength, Time));
}
