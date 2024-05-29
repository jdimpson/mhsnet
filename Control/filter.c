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


static char	sccsid[]	= "@(#)filter.c	1.18 05/12/16";

/*
**	Filter for links.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	STDIO

#include	"global.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"ftheader.h"
#include	"header.h"
#include	"params.h"
#include	"spool.h"
#include	"sub_proto.h"
#include	"sysexits.h"

/*
**	Parameters set from arguments.
*/

char *	HomeAddress;		/* Name of this node */
bool	Input;			/* Message coming in */
char *	LinkAddress;		/* Message arrived from this node */
char *	LinkName;		/* Directory for LinkAddress */
Ulong	LinkTime;		/* Travel time over inbound link */
Ulong	MesgTime;		/* Start time of message */
char *	Name	= sccsid;	/* Program invoked name */
bool	Output;			/* Message going out */
bool	Rerouted;		/* Message is being re-routed */
int	Traceflag;		/* Global tracing control */
bool	UseLink;		/* Message is being forced to use this link */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(i, &Input, 0),
	Arg_bool(o, &Output, 0),
	Arg_bool(r, &Rerouted, 0),
	Arg_bool(u, &UseLink, 0),
	Arg_string(H, &HomeAddress, 0, "home-address", 0),
	Arg_string(L, &LinkAddress, 0, "link", 0),
	Arg_long(M, &MesgTime, 0, "start-time", 0),
	Arg_string(N, &LinkName, 0, "linkdir", 0),
	Arg_int(T, &Traceflag, getInt1, "level", OPTARG|OPTVAL),
	Arg_long(X, &LinkTime, 0, "link-time", OPTARG),
	Arg_ignnomatch,
	Arg_end
};

/*
**	Parameters set from commands.
*/

CmdHead	Commands;		/* Miscellaneous commands from stdin to be passed through */
CmdHead	DataCmds;		/* File commands describing message to be filtered */

/*
**	Miscellaneous.
*/

char *	EnvFlags;		/* Accumulated ENV_FLTRFLAGS from header */
ExBuf	ExArgs;			/* Used to pass args to Execute() */
char *	ExString;		/* "stderr" from filter program */
char *	FilterProg;		/* Name of filter control program */
Ulong	FtpHdrEnd;		/* End of data in file containing FTP header */
char *	FtpHdrFile;		/* Possibly separate file for FTP header */
CmdV	HdrEnd;			/* Value of initial header "end_cmd" */
Cmd **	HdrEndCmd;		/* Command containing header "end_cmd" */
Ulong	HdrStart;		/* Start of header in MsgHdrFile */
char *	LogName;		/* Name of log file */
Ulong	MesgLength;		/* Total length of message */
char *	MsgHdrFile	= english("No message header file!");
Ulong	MsgHdrSize;		/* Size of data in file containing header */
int	OutFd;			/* Descriptor for OutFile */
char *	OutFile;		/* File containing (modified) data from message */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"FILTERPROG",		&FilterProg,			PSPOOL},
	{"LOGFILE",		&LogName,			PSPOOL},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

/*
**	Allowable returns from FILTERPROG.
*/

#define	CHNGBIT			0010	/* Data modified */
#define	PASSBIT			0020	/* No local delivery */
#define	FLAGBIT			0040	/* Flag on "stderr" */
#define	DROPBIT			0100	/* Drop message */
#define	RETNBIT			0200	/* Return message */

#define	CHANGEMESG			/* Changed data */ \
			(CHNGBIT)
#define	PASSMESG			/* No local delivery */ \
			(PASSBIT)
#define	PASSCHANGEMESG			/* No local delivery, data changed */ \
			(PASSBIT|CHNGBIT)
#define	FLAGMESG			/* Flag on stderr */ \
			(FLAGBIT)
#define	FLAGCHANGEMESG			/* Flag on stderr, data changed */ \
			(FLAGBIT|CHNGBIT)
#define	FLAGPASSMESG			/* No local delivery, flag on stderr */ \
			(FLAGBIT|PASSBIT)
#define	FLAGPASSCHANGEMESG		/* No local delivery, flag on stderr, data changed */ \
			(FLAGBIT|PASSBIT|CHNGBIT)
#define	DROPMESG			/* Forget message */ \
			(DROPBIT)
#define	RETMESG				/* Return, reason on stderr */ \
			(RETNBIT)

#define	OKMESG			0	/* Did nothing */
#define	NOFILTER		-1	/* Special case */

#define	Fprintf		(void)fprintf

bool	copy _FA_((Ulong));
bool	copyin _FA_((void));
bool	copyout _FA_((void));
int	filterftp _FA_((int));
int	filterother _FA_((void));
bool	getcommand _FA_((CmdT, CmdV));
void	writelog _FA_((void));
void	passflag _FA_((int, int));



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register int	fd;
	register int	res;
	HdrReason	hdr_reason;
	static char	hdr_error[]	= english("Message protocol header \"%s\" error");

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

	InitCmds(&Commands);
	InitCmds(&DataCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, ReadStr, "commands from stdin");

	while ( (fd = open(MsgHdrFile, O_RDWR)) == SYSERROR )
		Syserror(CouldNot, OpenStr, MsgHdrFile);

	if ( (hdr_reason = ReadHeader(fd)) != hr_ok )
		Error(hdr_error, HeaderReason(hdr_reason));

	Trace4(1, "Message from %s to %s for %s", HdrSource, HdrDest, HdrHandler);

	HdrStart = DataLength;
	DataLength = MesgLength - HdrLength;

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	OutFile = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	(void)UniqueName(OutFile, CHOOSE_QUALITY, OPTNAME, MesgLength, MesgTime);
	OutFd = create(OutFile);

	switch ( HdrSubpt )
	{
	case FTP:	res = filterftp(fd);	break;
	default:	res = filterother();	break;
	}

	if ( res != 0177 && (res & FLAGBIT) )
		passflag(fd, res);

	(void)close(fd);

	switch ( res )
	{
	case NOFILTER:		if ( !copyout() )
					exit(EX_SOFTWARE);
				writelog();
				break;
	case CHANGEMESG:
	case FLAGCHANGEMESG:
	case PASSCHANGEMESG:
	case FLAGPASSCHANGEMESG:
				if ( !copyin() )
					exit(EX_SOFTWARE);
				break;

	default:		(void)unlink(OutFile);
	}

#	if	FSYNC_2
	while ( fsync(OutFd) == SYSERROR )
		Syserror(CouldNot, "fsync", OutFile);
#	endif	/* FSYNC_2 */

	(void)close(OutFd);

	switch ( res )
	{
	case NOFILTER:		break;
	case DROPMESG:		exit(EX_DROPMESG);
				break;
	case RETMESG:		Fprintf(stderr, "%s\n", ExString);
				exit(EX_RETMESG);
				break;
	case FLAGPASSCHANGEMESG:
	case FLAGPASSMESG:
	case PASSCHANGEMESG:
	case PASSMESG:		exit(EX_EXMESG);
				break;
	case FLAGCHANGEMESG:
	case FLAGMESG:
	case CHANGEMESG:
	case OKMESG:		break;

	default:		if ( ExString != NULLSTR && ExString != EmptyString )
					Fprintf(stderr, "%s\n", ExString);
				exit(EX_SOFTWARE);
	}

	exit(EX_OK);
	return 0;
}



static char *
#if	ANSI_C
convert(char *cp, char in1, char in2)
#else	/* ANSI_C */
convert(cp, in1, in2)
	register char *	cp;
	char		in1, in2;
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
		len = 0x40;			/* Something reasonable */
	else
		len = ((len+1) | 0x3f) + 1;	/* Round up */

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
			c = '&';
		else
		if ( c == in2 )
			c = ';';
	while
		( (*op++ = c) != '\0' );

	if ( op[-2] != ';' )
	{
		op[-1] = ';';
		op[0] = '\0';
	}
	
	UnQuoteChars(keep_cp, EnvRestricted);
	return keep_cp;
}



/*
**	Describe data in OutFile.
*/

bool
copy(size)
	Ulong		size;
{
	register CmdV	cmdv;

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = OutFile, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = (Ulong)0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = size, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = OutFile, cmdv));

	return WriteCmds(&Commands, fileno(stdout), "stdout");
}



/*
**	Changed data is in OutFile, need to add back headers.
*/

bool
copyin()
{
	struct stat	statb;

	if ( !CollectData(&DataCmds, DataLength, MesgLength, OutFd, OutFile) )
		return false;

	if ( stat(OutFile, &statb) == SYSERROR )
	{
		Syserror(CouldNot, StatStr, OutFile);
		return false;
	}

	return copy((Ulong)statb.st_size);
}



/*
**	Copy data in message to one file.
*/

bool
copyout()
{
	if ( !CollectData(&DataCmds, (Ulong)0, MesgLength, OutFd, OutFile) )
		return false;

	return copy(MesgLength);
}



/*
**	Describe data files for use by filterprog
*/

void
describe_data(vap)
	VarArgs *	vap;
{
	register Cmd *	cep;
	register char *	cp;
	char *		arg;
	int		len;
	Ulong		oposn;
	Ulong		ibase;
	Ulong		ifend;
	Ulong		ilength;
	char *		ifile;

	for ( len = 0, oposn = 0, cep = DataCmds.ch_first ; cep != (Cmd *)0 && oposn < DataLength ; cep = cep->cd_next )
	{
		switch ( cep->cd_type )
		{
		default:
			continue;

		case file_cmd:
			len += strlen(cep->cd_value) + ULONG_SIZE*2 + 3;
			continue;

		case start_cmd:
			ibase = cep->cd_number;
			continue;

		case end_cmd:
			ifend = cep->cd_number;
		}

		oposn += ifend - ibase;
	}

	if ( len == 0 )
		return;

	len += 3;	/* "-D\0" */
	arg = cp = Malloc(len);
	cp = strcpyend(cp, "-D");

	for ( oposn = 0, cep = DataCmds.ch_first ; cep != (Cmd *)0 && oposn < DataLength ; cep = cep->cd_next )
	{
		switch ( cep->cd_type )
		{
		default:
			continue;

		case file_cmd:
			ifile = cep->cd_value;
			continue;

		case start_cmd:
			ibase = cep->cd_number;
			continue;

		case end_cmd:
			ifend = cep->cd_number;
		}

		ilength = ifend - ibase;

		if ( (oposn + ilength) > DataLength )
			ilength = DataLength - oposn;

#		if	SPRF_SIZE != 1
		(void)sprintf(cp, "%s@%ld&%ld;", ifile, (PUlong)ibase, (PUlong)ilength);
		cp += strlen(cp);
#		else	/* SPRF_SIZE != 1 */
		cp += (int)sprintf(cp, "%s@%ld&%ld;", ifile, (PUlong)ibase, (PUlong)ilength);
#		endif	/* SPRF_SIZE != 1 */

		oposn += ilength;
	}

	NEXTARG(vap) = arg;
}



/*
**	Extract from/to details from FTP header.
*/

int
filterftp(fd)
	int		fd;
{
	FthReason	fth_reason;

	if ( MsgHdrSize == HdrLength )
	{
		while ( (fd = open(FtpHdrFile, O_READ)) == SYSERROR )
			Syserror(CouldNot, ReadStr, FtpHdrFile);
	}
	else
		FtpHdrEnd = MsgHdrSize - HdrLength;

	if ( (fth_reason = ReadFtHeader(fd, FtpHdrEnd)) != fth_ok )
		Error(english("File transfer protocol header \"%s\" error"), FTHREASON(fth_reason));

	DataLength -= FthLength;

	if ( HdrFlags & HDR_RETURNED )
	{
		UQFthTo = FthFrom;
		FthFrom = english("RETURNED");
	}
	else
	if ( GetFthTo(false) == 0 )
	{
		Error(english("addressing error - no users in list \"%s\""), UQFthTo);
		return EX_DATAERR;	/* No users! */
	}

	Trace2(1, "userlist: %s", UQFthTo);

	return filterother();
}



/*
**	Pass data to FILTERPROG, return status.
*/

int
filterother()
{
	FILE *		fd;
	register char *	errs;
	char		size[NUMERICARGLEN];

	Trace2(1, "filterother() \"%s\"", FilterProg);

	ExString = EmptyString;

	if ( FilterProg == NULLSTR )
		FilterProg = concat(SPOOLDIR, LIBDIR, Name, ".sh", NULLSTR);

	if ( access(FilterProg, 1) == SYSERROR )
		return NOFILTER;

	FIRSTARG(&ExArgs.ex_cmd) = FilterProg;
	if ( Input )
		NEXTARG(&ExArgs.ex_cmd) = "-i";
	else
		NEXTARG(&ExArgs.ex_cmd) = "-o";

	describe_data(&ExArgs.ex_cmd);	/* Add "-D[<filename>@<start>&<length>;]..." */

	if ( (errs = HdrEnv) != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = concat("-E", convert(errs, ENV_US, ENV_RS), NULLSTR);

	if ( (errs = GetEnv(ENV_FLTRFLAGS)) != NULLSTR )
	{
		char * 	cp = EmptyString;

		do
			cp = concat(cp, errs, ";", NULLSTR);
		while
			( (errs = GetEnvNext(ENV_FLTRFLAGS, errs)) != NULLSTR );

		EnvFlags = cp;

		NEXTARG(&ExArgs.ex_cmd) = concat("-F", cp, NULLSTR);
	}

	if ( (errs = HdrRoute) != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = concat("-R", convert(errs, ROUTE_US, ROUTE_RS), NULLSTR);

	NEXTARG(&ExArgs.ex_cmd) = HomeAddress;
	NEXTARG(&ExArgs.ex_cmd) = LinkAddress;
	NEXTARG(&ExArgs.ex_cmd) = LinkName;

	(void)sprintf(size, "%lu", (PUlong)DataLength);
	NEXTARG(&ExArgs.ex_cmd) = size;

	NEXTARG(&ExArgs.ex_cmd) = HdrHandler;
	if ( HdrPart[0] != '\0' )
		NEXTARG(&ExArgs.ex_cmd) = concat(HdrID, Slash, HdrPart, NULLSTR);
	else
		NEXTARG(&ExArgs.ex_cmd) = HdrID;
	NEXTARG(&ExArgs.ex_cmd) = HdrSource;
	NEXTARG(&ExArgs.ex_cmd) = HdrDest;
	if ( FthFrom != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = FthFrom;
	if ( UQFthTo != NULLSTR )
		NEXTARG(&ExArgs.ex_cmd) = UQFthTo;

	fd = (FILE *)Execute(&ExArgs, NULLVFUNCP, ex_pipo, OutFd);

	(void)CollectData(&DataCmds, (Ulong)0, DataLength, fileno(fd), PipeStr);

	if ( (errs = ExClose(&ExArgs, fd)) != NULLSTR )
	{
		ExString = StripErrString(errs);
		free(errs);
	}

	Trace3(1, "\"%s\" returns %d", FilterProg, ExStatus);

	return ExStatus;
}



/*
**	Called from the errors routines to cleanup
*/

void
finish(error)
	int	error;
{
	if ( OutFile != NULLSTR )
		(void)unlink(OutFile);
	(void)exit(error);
}



/*
**	Process a command from commands file.
*/

bool
getcommand(type, val)
	CmdT	type;
	CmdV	val;
{
	static Ulong	start;
	static Ulong	fend;

	switch ( type )
	{
	case end_cmd:
		fend = val.cv_number;
		MesgLength += val.cv_number - start;
		MsgHdrSize = val.cv_number - start;
		HdrEnd = val;
		HdrEndCmd = DataCmds.ch_last;
		(void)AddCmd(&DataCmds, type, val);
		break;

	case file_cmd:
		FtpHdrEnd = fend;
		FtpHdrFile = MsgHdrFile;
		MsgHdrFile = AddCmd(&DataCmds, type, val);
		break;

	case start_cmd:
		start = val.cv_number;
		(void)AddCmd(&DataCmds, type, val);
		break;

	default:
		(void)AddCmd(&Commands, type, val);
	}

	return true;
}



/*
**	Write record in STATSDIR/<link>.filterlog.
*/

void
writelog()
{
	register FILE *	fd;
	register char *	cp;
	char *		link;
	union
	{
		char	dt[DATETIMESTRLEN];
		char	t[TIMESTRLEN];
	}
			buf;
	struct stat	statb;

	static char	fmt1[]	= "%6s\t%*s";
	static char	fmt2[]	= "  %-18s %-16s";
	static char	fmt3[]	= " %*s %10lu";
	static char	hdr3[]	= " %*s %10s";
	static char	fmt4[]	= "  %-30s %-30s";
	static char	fmt5[]	= "  %-10s %s\n";

	link = StripTypes(LinkAddress);
	if ( (cp = strchr(link, '.')) != NULLSTR )
		*cp = '\0';

	if ( LogName == NULLSTR )
		LogName = concat(SPOOLDIR, STATSDIR, link, ".fltr.log", NULLSTR);

#	if	TRUNCDIRNAME == 1
	if ( (cp = strrchr(LogName, '/')) == NULLSTR )
		cp = LogName;
	else
		cp++;
	if ( strlen(cp) > 14 )
		cp[14] = '\0';
#	endif	/* TRUNCDIRNAME == 1 */

	if ( (fd = fopen(LogName, "a")) == NULL )
		fd = fdopen(create(LogName), "a");

	if ( fstat(fileno(fd), &statb) != SYSERROR && statb.st_size == 0 )
	{
		Fprintf(fd, fmt1, "IN/out", DATETIMESTRLEN-1, "Date     Time    ");
		Fprintf(fd, fmt2, "Message ID/Part", "Handler");
		Fprintf(fd, hdr3, TIMESTRLEN-1, "TravelT", "Size");
		Fprintf(fd, fmt4, "Source", "Destination");
		Fprintf(fd, fmt5, "From", "To");
	}

	cp = Input?"IN  <-":"out ->";

	Fprintf(fd, fmt1, cp, DATETIMESTRLEN-1, DateTimeStr(Time, buf.dt));
	if ( HdrPart[0] != '\0' )
		Fprintf(fd, fmt2, concat(HdrID, Slash, HdrPart, NULLSTR), HdrHandler);
	else
		Fprintf(fd, fmt2, HdrID, HdrHandler);
	Fprintf(fd, fmt3, TIMESTRLEN-1, TimeString(HdrTt+LinkTime, buf.t), (PUlong)MesgLength);
	Fprintf(fd, fmt4, StripTypes(HdrSource), StripTypes(HdrDest));
	if ( UQFthTo != NULLSTR )
		Fprintf(fd, fmt5, FthFrom, UQFthTo);
	else
		putc('\n', fd);

	(void)fclose(fd);
}



/*
**	Pass back new flag for header.
*/

void
passflag(fd, res)
	int		fd;
	int		res;
{
	register CmdV	cmdv;
	register int	len;
	register bool	delete;

	Trace3(1, "passflag(%d) \"%s\"", fd, ExString);

	if ( (len = strlen(ExString)) == 0 )
		return;

	if ( ExString[len-1] == '\n' )
		ExString[--len] = '\0';

	if ( len == 0 )
		return;

	if ( strncmp(ExString, "DELETE ", 7) == STREQUAL )
	{
		ExString += 7;

		if ( EnvFlags == NULLSTR || StringMatch(EnvFlags, ExString) == NULLSTR )
			return;

		delete = true;
	}
	else
	{
		if ( strncmp(ExString, "ADD ", 4) == STREQUAL )
			ExString += 4;

		if ( EnvFlags != NULLSTR && StringMatch(EnvFlags, ExString) != NULLSTR )
			return;

		delete = false;
	}

	ExString = MakeEnv(ENV_FLTRFLAGS, ExString, NULLSTR);

	if ( Input )	/* Must use "env" command */
	{
		(void)AddCmd(&Commands, delete?delenv_cmd:env_cmd, (cmdv.cv_name = ExString, cmdv));
	}
	else		/* Have to re-write header */
	{
		if ( delete )
		{
			if ( !DelEnv(ExString, NULLSTR) )
				return;
		}
		else
			HdrEnv = concat(HdrEnv, ExString, NULLSTR);

		while ( (cmdv.cv_number = WriteHeader(fd, HdrStart, HdrLength)) == SYSERROR )
			Syserror(CouldNot, WriteStr, MsgHdrFile);

		if ( !ChangeCmd(HdrEndCmd, end_cmd, HdrEnd, cmdv) )
			Fatal1("ChangeCmd failed");

		MesgLength += cmdv.cv_number - HdrEnd.cv_number;
	}

	if ( res & CHNGBIT )
		return;

	/** Write all commands back to stdout **/

	(void)WriteCmds(&DataCmds, fileno(stdout), "stdout");
	(void)WriteCmds(&Commands, fileno(stdout), "stdout");
}
