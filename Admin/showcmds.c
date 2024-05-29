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
**	Print out command files.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	STDIO

#include	"global.h"
#include	"Args.h"
#include	"debug.h"
#include	"header.h"
#include	"ftheader.h"
#include	"sub_proto.h"
#include	"sysexits.h"

#include	"commandfile.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Arguments.
*/

bool	DataOut;		/* Copy data part of file to stdout */
bool	IgnErrors;		/* Ignore header errors */
bool	MsgOut;			/* Copy whole message to stdout */
char *	Name	= sccsid;	/* Program invoked name */
bool	ShellEnv;		/* Print headers as shell variable assignments */
int	Traceflag;		/* Global tracing control */

AFuncv	getFile _FA_((PassVal));

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(a, &ShellEnv, 0),
	Arg_bool(d, &DataOut, 0),
	Arg_bool(e, &IgnErrors, 0),
	Arg_bool(m, &MsgOut, 0),
	Arg_int(T, &Traceflag, getInt1, "level", OPTARG|OPTVAL),
	Arg_noflag(0, getFile, "commands-file", MANY),
	Arg_end
};

/*
**	Miscellaneous.
*/

CmdHead	DataCmds;		/* Data commands from command file */
Ulong	MesgLength;		/* Length of data in current message */

struct
{
	char *	name;
	Ulong	end;
}
	FileStack[2];		/* Remember last 2 files in message */

#define	Ftheaderfile	FileStack[0].name
#define	Ftheaderend	FileStack[0].end
#define	Headerfile	FileStack[1].name
#define	Headerend	FileStack[1].end

#define	Fprintf		(void)fprintf

char *	quote _FA_((char *));



int
main(argc, argv)
	register int	argc;
	register char *	argv[];
{
	InitParams();

	DoArgs(argc, argv, Usage);

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
	register char *	cp;
	register CmdC	conv;
	static Ulong	filestart;
	static Ulong	fileend;

	switch ( type )
	{
	default:
		if ( (unsigned)type > (unsigned)last_cmd )
			conv = error_type;
		else
			conv = CmdfTypes[(int)type];
		break;

	case end_cmd:
		fileend = val.cv_number;
		FileStack[1].end = fileend;
		MesgLength += fileend - filestart;
		(void)AddCmd(&DataCmds, type, val);
		conv = CmdfTypes[(int)type];
		break;

	case file_cmd:
		FileStack[0] = FileStack[1];
		FileStack[1].name = AddCmd(&DataCmds, type, val);
		conv = CmdfTypes[(int)type];
		break;

	case start_cmd:
		filestart = val.cv_number;
		(void)AddCmd(&DataCmds, type, val);
		conv = CmdfTypes[(int)type];
		break;
	}

	if ( DataOut )
	{
		if ( conv == error_type )
			return false;
		else
			return true;
	}

	switch ( conv )
	{
	case number_type:
		cp = CmdToString(type, val.cv_number);
		break;

	case string_type:
		cp = val.cv_name;
		break;

	default:
		if ( ShellEnv )
			Fprintf(stdout, "ERROR='unrecognised command type [%d]'\n", (int)type);
		else
			Fprintf(stdout, "\tunrecognised command type [%d]\n", (int)type);
		return true;
	}

	if ( ShellEnv )
		Fprintf(stdout, "%s='%s'\n", CmdfDescs[(int)type], quote(cp));
	else
		Fprintf(stdout, "\t%-*s%s\n", MAX_CMD_NAMES_LENGTH+1, CmdfDescs[(int)type], cp);

	return true;
}



/*
**	Process a commands file.
*/

AFuncv
getFile(val)
	PassVal		val;
{
	register char *	cp;
	int		hfd;
	HdrReason	reason;
	FthReason	fthreason;
	bool		fthprinted = false;
	char		dtbuf[DATETIMESTRLEN];
	struct stat	statb;

	static char *	sof = "%s in \"%s\" <<EOF\n";
	static char *	eof = "EOF%s\n\n";

	if ( MsgOut )
	{
		DataOut = true;
		ShellEnv = false;
	}
	else
	if ( DataOut )
		ShellEnv = false;

	if ( stat(val.p, &statb) != SYSERROR )
		RdFileTime = statb.st_mtime;

	if ( !DataOut )
	{
		if ( !ShellEnv )
		{
			Fprintf(stdout, "%s", DateTimeStr(RdFileTime, dtbuf));
			Fprintf(stdout, sof, english(" Commands"), val.p);
		}
		else
			Fprintf(stdout, "commands_file='%s'\n", quote(val.p));
	}

	FreeCmds(&DataCmds);
	Headerfile = NULLSTR;
	MesgLength = 0;

	if ( ReadCmds(val.p, getcommand) )
		ErrString = newstr(EmptyString);
	else
	if ( RdFileTime	== 0 )
		ErrString = newstr(english(" (Can't open file.)"));
	else
		ErrString = newstr(english(" (Commands garbled!)"));

	if ( ShellEnv )
	{
		if ( ErrString[0] != '\0' )
			Fprintf(stdout, "ERROR='%s'\n", quote(ErrString));
	}
	else
	if ( !DataOut )
		Fprintf(stdout, eof, ErrString);

	if ( Headerfile != NULLSTR )
	{
		if ( !DataOut && !ShellEnv )
			Fprintf(stdout, sof, english("Message header"), Headerfile);

		if ( IgnErrors )
			InitFtHeader();

		if ( (hfd = open(Headerfile, O_READ)) == SYSERROR )
			ErrString = newstr(english(" (Can't open file.)"));
		else
		if ( (reason = ReadHeader(hfd)) != hr_ok && !IgnErrors )
		{
			if ( !DataOut && !ShellEnv && (cp = ReadErrFile(Headerfile, false)) != NULLSTR )
			{
				ExpandFile(stdout, cp, ErrSize);
				free(cp);
			}

			ErrString = concat(english(" (Header "), HeaderReason(reason), english(" error.)"), NULLSTR);

			(void)close(hfd);
		}
		else
		{
			if ( !DataOut )
				PrintHeader(stdout, 0, ShellEnv);

			MesgLength -= HdrLength;

			ErrString = newstr(EmptyString);

			if ( HdrSubpt == FTP && DataLength > 0 )
			{
				if ( !DataOut && !ShellEnv )
					Fprintf(stdout, english("\n\tFTP message header:\n"));

				if ( (fthreason = ReadFtHeader(hfd, DataLength)) != fth_ok && !IgnErrors )
				{
					if ( !DataOut && !ShellEnv && (cp = ReadErrFile(Headerfile, false)) != NULLSTR )
					{
						ExpandFile(stdout, cp, ErrSize);
						free(cp);
					}

					ErrString = concat(english(" (FTP header "), FTHREASON(fthreason), english(" error.)"), NULLSTR);
				}
				else
				{
					if ( !DataOut )
						PrintFtHeader(stdout, 8, ShellEnv);

					MesgLength -= FthLength;

					ErrString = newstr(EmptyString);
				}

				fthprinted = true;
			}

			(void)close(hfd);
		}
	}

	if ( ShellEnv )
	{
		if ( ErrString[0] != '\0' )
			Fprintf(stdout, "ERROR='%s'\n", quote(ErrString));
	}
	else
	if ( !DataOut )
		Fprintf(stdout, eof, ErrString);
	else
	if ( strlen(ErrString) != 0 )
		Fprintf(stderr, "%s\n", ErrString);

	if ( !fthprinted && HdrSubpt == FTP && Ftheaderfile != NULLSTR && strlen(ErrString) == 0 )
	{
		if ( !DataOut && !ShellEnv )
			Fprintf(stdout, sof, english("FTP message header"), Ftheaderfile);

		if ( (hfd = open(Ftheaderfile, O_READ)) == SYSERROR )
			ErrString = newstr(english(" (Can't open file.)"));
		else
		if ( (fthreason = ReadFtHeader(hfd, Ftheaderend)) != fth_ok && !IgnErrors )
		{
			if ( !DataOut && !ShellEnv && (cp = ReadErrFile(Ftheaderfile, false)) != NULLSTR )
			{
				ExpandFile(stdout, cp, ErrSize);
				free(cp);
			}

			ErrString = concat(english(" (FTP header "), FTHREASON(fthreason), english(" error.)"), NULLSTR);

			(void)close(hfd);
		}
		else
		{
			if ( !DataOut )
				PrintFtHeader(stdout, 0, ShellEnv);

			MesgLength -= FthLength;

			ErrString = newstr(EmptyString);

			(void)close(hfd);
		}

		if ( ShellEnv )
		{
			if ( ErrString[0] != '\0' )
				Fprintf(stdout, "ERROR='%s'\n", quote(ErrString));
		}
		else
		if ( !DataOut )
			Fprintf(stdout, eof, ErrString);
		else
		if ( strlen(ErrString) != 0 )
			Fprintf(stderr, "%s\n", ErrString);
	}

	if ( DataOut && strlen(ErrString) == 0 )
		(void)CollectData(&DataCmds, (Ulong)0, MsgOut?MAX_ULONG:MesgLength, fileno(stdout), "stdout");

	return ACCEPTARG;
}



char *
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
