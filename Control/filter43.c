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


static char	sccsid[]	= "@(#)filter43.c	1.18 05/12/16";

/*
**	Filter for links using the SUNIII compatible daemons.
*/

#define	FILE_CONTROL
#define	RECOVER
#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"ftheader.h"
#include	"handlers.h"
#include	"header.h"
#include	"link.h"
#include	"params.h"
#include	"routefile.h"
#include	"spool.h"
#include	"statefile.h"
#include	"sub_proto.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */

#if	SUN3 == 1
#include	"sun3.h"

/*
**	Parameters set from arguments.
*/

char *	HomeAddress;		/* Name of this node */
bool	Input;			/* Message coming in -- convert 3->4 */
char *	LinkAddress;		/* Message arrived from / travelling to this node */
char *	LinkName;		/* Directory for LinkAddress */
Ulong	LinkTime;		/* Travel time over inbound link */
Ulong	MesgTime;		/* Start time of message */
char *	Name	= sccsid;	/* Program invoked name */
bool	Output;			/* Message going out -- convert 4->3 */
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
CmdHead	PartCmds;		/* File commands describing composite from multipart message */

/*
**	Results
*/

typedef enum
{
	ok_res, err_res
}
	Result;

/*
**	Miscellaneous.
*/

int	ABORT;			/* Terminate with prejudice */
bool	AckIn;			/* Positive acknowledgement requested */
bool	CRCIn;			/* Data CRC present */
char *	EnvFlags;		/* Accumulated ENV_FLTRFLAGS from header */
int	ERROR;			/* Terminate with error */
Ulong	FtpHdrEnd;		/* End of data in file containing FTP header */
char *	FtpHdrFile;		/* Possibly separate file for FTP header */
char	Fth_error[]	= english("TYP%c file transfer protocol header \"%s\" error");
Handler*HndlrP;			/* Message handler details */
char *	Idp;			/* Pointer into OutFile name for quality byte */
Ulong	MesgLength;		/* Total length of message */
char *	MsgHdrFile	= english("No message header file!");
Ulong	MsgHdrSize;		/* Size of data in file containing header */
bool	NoOpt;			/* Unoptimised message transmission */
Ulong	OrigDataLength;		/* Real ``DataLength'' */
int	OutFd;			/* Descriptor for OutFile */
char *	OutFile;		/* File containing (modified) data from message */
char	PostMaster[]	= english("Postmaster");
bool	Returned;		/* True if invoked for returned message */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"ABORT",		(char **)&ABORT,		PINT},
	{"ERROR",		(char **)&ERROR,		PINT},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

#define	Fprintf		(void)fprintf

void	filterftp _FA_((int));
void	filterother _FA_((void));
bool	getcommand _FA_((CmdT, CmdV));
void	s3cnvftp _FA_((void));
void	s3cnvhdr _FA_((Ulong));
bool	s3getenv_bool _FA_((char *));
void	s4cleanup _FA_((void));
void	s4cnvftp _FA_((void));
void	s4cnvother _FA_((void));

#endif	/* SUN3 == 1 */



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
#	if	SUN3 == 1
	register int	fd;
	HdrReason	hdr_reason;
	static char	hdr_error[]	= english("Message protocol header \"%s\" error");

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

	InitCmds(&Commands);
	InitCmds(&DataCmds);
	InitCmds(&PartCmds);

	if ( !ReadFdCmds(stdin, getcommand) )
		Error(CouldNot, ReadStr, "commands from stdin");

	while ( (fd = open(MsgHdrFile, O_RDWR)) == SYSERROR )
		Syserror(CouldNot, OpenStr, MsgHdrFile);

	if ( (hdr_reason = ReadHeader(fd)) != hr_ok )
		Error(hdr_error, HeaderReason(hdr_reason));

	Trace4(1, "Message from %s to %s for %s", HdrSource, HdrDest, HdrHandler);

	DataLength = OrigDataLength = MesgLength - HdrLength;

	if ( Output )
	{
		if ( HdrType == HDR_TYPE1 )
		{
			Warn(english("TYP1 header passed on output"));
			exit(EX_OK);
		}
	}
	else
		if ( HdrType == HDR_TYPE2 )
		{
			Warn(english("TYP2 header passed on input"));
			exit(EX_OK);
		}

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	if ( Input )
		OutFile = LINKMSGSINNAME;
	else
		OutFile = LINKSPOOLNAME;
	OutFile = concat(SPOOLDIR, LinkName, Slash, OutFile, Slash, Template, NULLSTR);
	Idp = strrchr(OutFile, '/') + 1;

	(void)UniqueName(OutFile, CHOOSE_QUALITY, OPTNAME, MesgLength, MesgTime);
	OutFd = create(OutFile);

	switch ( HdrSubpt )
	{
	case FTP:	filterftp(fd);	break;
	default:	filterother();	break;
	}

	(void)close(fd);

#	if	FSYNC_2
	while ( fsync(OutFd) == SYSERROR )
		Syserror(CouldNot, "fsync", OutFile);
#	endif	/* FSYNC_2 */
	(void)close(OutFd);

	if ( !WriteCmds(&Commands, fileno(stdout), "stdout") )
		exit(EX_SOFTWARE);

	s4cleanup();

	finish(EX_OK);

#	else	/* SUN3 == 1 */

	exit(EX_UNAVAILABLE);

#	endif	/* SUN3 == 1 */
	return 0;
}
#if	SUN3 == 1



/*
**	Fix FTP header, call filterother().
*/

void
filterftp(fd)
	int		fd;
{
	FthReason	fth_reason;

	if ( Output )
	{
		if ( HdrFlags & HDR_RETURNED )
			Returned = true;
	}
	else
	{
		if ( s3getenv_bool(ENV3_RETURNED) )
			Returned = true;
	}

	if ( MsgHdrSize == HdrLength )
	{
		while ( (fd = open(FtpHdrFile, O_READ)) == SYSERROR )
			Syserror(CouldNot, ReadStr, FtpHdrFile);
	}
	else
		FtpHdrEnd = MsgHdrSize - HdrLength;

	if
	(
		(fth_reason = ReadFtHeader(fd, (long)FtpHdrEnd)) != fth_ok
		&&
		!Returned
		&&
		(Output || fth_reason != fth_type)
	)
	{
		if ( Input )
		{
			Recover(ert_return);
			Error(Fth_error, '1', FTHREASON(fth_reason));
			exit(EX_RETMESG);
			return;
		}

		Error(Fth_error, '2', FTHREASON(fth_reason));
		return;	/* EX_SOFTWARE */
	}

	DataLength -= FthLength;

	if ( GetFthTo(false) == 0 )
	{
		static char	NullFthFDesc[]	= { FTH_FDSEP, FTH_FDSEP, FTH_FDSEP, '\0' };

		if ( !Returned )
		{
			Recover(ert_return);
			Error(english("addressing error - no users in list \"%s\""), UQFthTo);
			exit(EX_RETMESG);
			return;
		}

		FthUsers = Talloc0(FthUlist);
		FthUsers->u_name = "ERROR";
		NFthUsers = 1;

		if ( FthFrom == NULLSTR )
			FthFrom = PostMaster;

		if ( FthFdescs == NULLSTR )
			FthFdescs = NullFthFDesc;
	}

	Trace2(1, "userlist: %s", UQFthTo);

	if ( Output )
		s4cnvftp();
	else
		s3cnvftp();
}



/*
**	Fix header, return status.
*/

void
filterother()
{
	if ( Output )
		s4cnvother();
	else
		s3cnvhdr((Ulong)0);
}



/*
**	Called from the errors routines to cleanup
*/

void
finish(error)
	int	error;
{
	if ( ABORT )
		abort();

	if ( error == EX_OK )
		error = ERROR;
	else
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
**	Routines to convert Sun3 message to Sun4.
*/

/*
**	Sun3 magic.
*/

char	Sun3EQ[]	= { Sun3ERs, Sun3EUs, '\0' };
char	GlobalDest[]	= GLOBALDEST;


char	*s3addexplicit(), *s3cnvaddress(), *s3cnvexpl(), *s3cnvnames(), *s3cnvroute(),
	*s3bangaddress(), *s3get_env(), *s3make_env();



/*
**	Convert and then prefix "tail" with pre-converted "head".
*/

char *
s3bangaddress(np, head, tail, types)
	register char *	np;
	char *		head;
	register char *	tail;
	bool		types;
{
	Trace4(1, "s3bangaddress(%s, %s, %d)", head, tail, types);

	if ( head[0] != ATYP_EXPLICIT )
		*np++ = ATYP_EXPLICIT;

	np = strcpyend(np, head);
	*np++ = ATYP_EXPLICIT;

	return s3cnvnames(np, tail, types);
}



/*
**	Convert complex Sun3 address into Sun4 typed address.
*/

char *
s3cnvaddress(address, types)
	char *		address;
	bool		types;
{
	register char *	cp;
	register char *	ep;
	register char *	np;
	register char *	tp;
	register int	m;

	static char	sepset[] = { '!', ',', '\0'};
	static char	illset[] = { '*', ',', '\0'};

	Trace3(1, "s3cnvaddress(%s, %ctypes)", address, types?'+':'-');

	if ( (cp = address) == NULLSTR || *cp == '\0' )
		return cp;

	{
		register int	n;

		for ( np = cp, m = 1, n = 1 ; (np = strpbrk(++np, sepset)) != NULLSTR ; )
			if ( *np == '!' )
				m++;				/* Separate multicasts */
			else
				n++;				/* Multicast size */

		address = np = Malloc(n*m*180 + strlen(cp));	/* 180 == 10 domains * .1=15 */
	}

	switch ( *cp++ )
	{
	case '!':
		if ( (ep = strrchr(cp, '!')) == NULLSTR )
			Error(english("Malformed Sun3 address \"%s\""), --cp);
		*ep++ = '\0';
		if ( strpbrk(cp, illset) != NULLSTR )
		{
			*--ep = '!';
			Recover(ert_return);
			Error(english("Gateway spooler can't handle complex explicit address \"%s\""), cp);
			exit(EX_RETMESG);
			return cp;
		}
		cp = s3cnvexpl(m, cp, types);
		if
		(
			*ep == ','
			||
			(strchr(ep, ',') != NULLSTR && ep--)	/* This is malformed -- but seems to occur */
		)
		{
			do
			{
				if ( (tp = strchr(++ep, ',')) != NULLSTR )
					*tp = '\0';
				*np++ = ATYP_MULTICAST;
				np = s3bangaddress(np, cp, ep, types);
			}
			while
				( (ep = tp) != NULLSTR );
			free(cp);
			break;
		}
		(void)s3bangaddress(np, cp, ep, types);
		free(cp);
		break;

	case ',':
		do
		{
			if ( (ep = strchr(cp, ',')) != NULLSTR )
				*ep++ = '\0';
			*np++ = ATYP_MULTICAST;
			np = s3cnvnames(np, cp, types);
		}
		while
			( (cp = ep) != NULLSTR );
		break;

	default:
		np = s3cnvnames(np, --cp, types);
	}

	cp = newstr(address);
	free(address);

	Trace2(2, "convaddress ==> %s", cp);

	return cp;
}



/*
**	Make environment. (ENV_ strings -> HDR_FLAGS.)
**	Magic strings from Sun3/Include/header.h
*/

void
s3cnvenv()
{
	register char *	ep;
	register char *	cp;

	ep = EmptyString;
	HdrFlags = 0;

	if ( s3getenv_bool(ENV3_ACK) )
		HdrFlags |= HDR_ACK;
	if ( s3getenv_bool(ENV3_ENQALL) )
	{
		ep = s3make_env(ep, ENV_FLAGS, STATE_ENQ_ALL);
		HdrFlags |= HDR_ENQ;
	}
	else
	if ( s3getenv_bool(ENV3_ENQ) )
		HdrFlags |= HDR_ENQ;
	if ( s3getenv_bool(ENV3_NO_AUTOCALL) )
		HdrFlags |= HDR_NO_AUTOCALL;
	if ( NoOpt )
		HdrFlags |= HDR_NOOPT;
	if ( s3getenv_bool(ENV3_NORET) )
		HdrFlags |= HDR_NORET;
	if ( Returned )
		HdrFlags |= HDR_RETURNED;
	if ( s3getenv_bool(ENV3_REV_CHARGE) )
		HdrFlags |= HDR_REV_CHARGE;
	if ( AckIn )
		HdrFlags |= HDR_RQA;

	if ( (cp = s3get_env(ENV3_DESTINATION)) != NULLSTR )
	{
		ep = s3make_env(ep, ENV_DEST, cp);
		free(cp);
	}
	if ( (cp = s3get_env(ENV3_DUP)) != NULLSTR )
	{
		ep = s3make_env(ep, ENV_DUP, cp);
		free(cp);
	}
	if ( (cp = s3get_env(ENV3_ERR1)) != NULLSTR )
	{
		ep = s3make_env(ep, ENV_ERR, cp);
		free(cp);
	}
	if ( (cp = s3get_env(ENV3_HANDLER_FLAGS)) != NULLSTR )
	{
		ep = s3make_env(ep, ENV_FLAGS, cp);
		free(cp);
	}
	if ( (cp = s3get_env(ENV3_NOTNODE)) != NULLSTR )
	{
		ep = s3make_env(ep, ENV_NOTREGIONS, s3cnvaddress(cp, true));
		free(cp);
	}
	if ( (cp = s3get_env(ENV3_ROUTE)) != NULLSTR && (cp = s3cnvroute(cp)) != NULLSTR )
	{
		ep = s3make_env(ep, ENV_ROUTE, cp);
		free(cp);
	}
	if ( (cp = s3get_env(ENV3_TRUNC)) != NULLSTR )
	{
		ep = s3make_env(ep, ENV_TRUNC, cp);
		free(cp);
	}

	HdrEnv = ep;
}



/*
**	Convert explicit Sun3 address into Sun4 typed address.
*/

char *
s3cnvexpl(count, address, types)
	int		count;
	char *		address;
	bool		types;
{
	register char *	cp;
	register char *	np;
	register char *	tp;

	Trace4(1, "s3cnvexpl(%d, %s, %d)", count, address, types);

	if ( (cp = address) == NULLSTR || *cp == '\0' )
		return cp;

	address = np = Malloc(count*180 + strlen(cp));

	if ( count <= 0 )
		(void)s3cnvnames(np, cp, types);
	else
	do
	{
		if ( (tp = strchr(cp, '!')) != NULLSTR )
			*tp++ = '\0';
		*np++ = ATYP_EXPLICIT;
		np = s3cnvnames(np, cp, types);
	}
	while
		( (cp = tp) != NULLSTR );

	cp = newstr(address);
	free(address);

	Trace2(2, "convexpl ==> %s", cp);

	return cp;
}



/*
**	Convert Sun3 FTP header to Sun4.
*/

void
s3cnvftp()
{
	register FthUlist *	up;
	register char *		cp;

	/*
	**	Convert addresses.
	**	Magic numbers from Sun3/Include/ftheader.h
	*/

	if ( (FthType[0] & (1<<1)) && !Returned )
	{
		if ( !CheckData(&DataCmds, (Ulong)0, DataLength, &DataCrc) )
		{
			Recover(ert_return);
			Error(Fth_error, '1', FTHREASON(fth_baddatacrc));
			exit(EX_RETMESG);
			return;
		}

		CRCIn = true;
	}

	if ( FthType[0] & (1<<0) )
		AckIn = true;

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
	{
		if ( up->u_name == NULLSTR )
			continue;

		if ( (cp = up->u_dest) != NULLSTR )
			up->u_dest = s3cnvaddress(cp, false);
	}

	/*
	**	Write new FTP header.
	*/

	FthType[0] = FTH_TYPE;
	if ( CRCIn )
		FthType[0] |= FTH_DATACRC;

	SetFthTo();

	while ( WriteFtHeader(OutFd, (long)0) == SYSERROR )
		Syserror(CouldNot, WriteStr, OutFile);

	/*
	**	Convert header.
	*/

	s3cnvhdr((Ulong)FthLength);
}



/*
**	Change Sun3 header -> Sun4.
*/

void
s3cnvhdr(hdrstart)
	Ulong		hdrstart;
{
	register CmdV	cmdv;

	if ( s3getenv_bool(ENV3_RETURNED) )
		Returned = true;

	if ( (HndlrP = GetHandler(HdrHandler)) == (Handler *)0 )
	{
		static Handler	defhandler;

		defhandler.quality = LOWEST_QUALITY;
		defhandler.order = ALWAYS_ORDER;
		HndlrP = &defhandler;
	}

	if ( HndlrP->order == NEVER_ORDER )
		NoOpt = OPTNAME;
	else
	if ( HndlrP->order == ALWAYS_ORDER )
		NoOpt = NOOPTNAME;
	else
		NoOpt = s3getenv_bool("NOOPT");

	if ( (HdrQuality = Idp[0]) < HndlrP->quality || NoOpt )
		HdrQuality = HndlrP->quality;

	if ( (HdrID = s3get_env(ENV3_ID)) == NULLSTR )	/* From previous incarnation in Sun4 */
		HdrID = Idp;

	/*
	**	Convert environment (HdrFlags & HdrEnv).
	*/

	s3cnvenv();

	/*
	**	Convert HdrRoute.
	*/

	HdrRoute = s3cnvroute(HdrRoute);

	/*
	**	Convert addresses.
	*/

	HdrDest = s3cnvaddress(HdrDest, true);

	if ( HdrDest[0] == '\0' )
		Error(english("null destination"));

	HdrSource = s3cnvaddress(HdrSource, true);

	/*
	**	Write new message header.
	*/

	HdrType = HDR_TYPE2;

	while ( WriteHeader(OutFd, (long)hdrstart, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, OutFile);

	MesgLength = DataLength + HdrLength + hdrstart;

	/*
	**	Describe OutFile.
	*/

	LinkCmds(&DataCmds, &Commands, (Ulong)0, DataLength, newstr(OutFile), Time);

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = OutFile, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = (Ulong)0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = HdrLength + hdrstart, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = OutFile, cmdv));
}



/*
**	Convert simple Sun3 address into Sun4 typed address.
*/

char *
s3cnvnames(to, address, types)
	char *		to;
	char *		address;
	bool		types;
{
	register char *	cp;
	register char *	np;
	register int	n;
	Addr *		ap;

	Trace4(1, "s3cnvnames(%#lx, %s, %d)", (PUlong)to, address, types);

	if ( (cp = address) == NULLSTR || *cp == '\0' )
		return to;

	if ( cp[0] == '*' )
	{
		if ( cp[1] == '\0' || (cp[1] == '.' && cp[2] == '*' && cp[3] == '\0') )
			cp = GlobalDest;	/* Catch "*" and "*.*" */
	}
	else
	{
		Link		link;

		static char	au[]	= ".au";
		static char	oz[]	= ".oz";
		static char	node[]	= "9=";

		/** Sun3 has a nasty habit of truncating the obvious **/

		n = strlen(cp);

		if ( strccmp(&cp[n-3], oz) == STREQUAL )
			np = concat(node, cp, au, NULLSTR);
		else
			np = concat(node, cp, NULLSTR);

		/** The above is risky, so check it out... **/

		ap = StringToAddr(np, NO_STA_MAP);
		if ( FindAddr(ap, &link, FASTEST) != wh_none && !(link.ln_flags & FL_FORWARD) )
		{
			cp = np;
			goto ok;
		}
		FreeAddr(&ap);
		free(np);
	}

	cp = newstr(cp);
	ap = StringToAddr(cp, NO_STA_MAP);
ok:
	to = strcpyend(to, types ? TypedAddress(ap) : UnTypAddress(ap));

	FreeAddr(&ap);
	free(cp);

	return to;
}



/*
**	Convert Sun3 route to Sun4.
*/

char *
s3cnvroute(route)
	char *		route;
{
	register char *	cp;
	register char *	rp;
	register char *	np;
	register char *	tp;
	register int	len;

	if ( (cp = route) == NULLSTR || *cp++ != Sun3RS )
		return NULLSTR;

	for ( rp = cp, len = 1 ; (rp = strchr(++rp, Sun3RS)) != NULLSTR ; len++ );

	len *= 180;			/* For name expansion */
	len += strlen(cp);		/* For times and separators */
	route = rp = Malloc(len);	/* HOPE THIS IS BIG ENOUGH! */

	for ( ; *cp != '\0' ; )
	{
		if ( (np = strchr(cp, Sun3RS)) != NULLSTR )
			*np++ = '\0';

		if ( (tp = strchr(cp, Sun3RT)) != NULLSTR )
			*tp++ = '\0';
		else
			tp = "0";	/* Malformed, but who cares? */

		*rp++ = ROUTE_RS;
		(void)s3cnvnames(rp, cp, true);
		rp = strcpyend(rp, rp+1);	/* Skip ATYP_DESTINATION */
		*rp++ = ROUTE_US;
		rp = strcpyend(rp, tp);

		if ( (cp = np) == NULLSTR )
			break;
	}

	return route;
}



/*
**	Special Sun3 GetEnv.
*/

char *
s3get_env(name)
	char *		name;
{
	register char *	ep;
	register char *	fp;
	register char *	sp;
	register int	val;

	if ( (ep = HdrEnv) == NULLSTR || *ep == '\0' )
		return NULLSTR;

	for ( ;; )
	{
		if ( (sp = strchr(ep, Sun3ERs)) != NULLSTR )
			*sp = '\0';
		
		if ( (fp = strchr(ep, Sun3EUs)) != NULLSTR )
			val = strncmp(name, ep, fp-ep);
		else
			val = strcmp(name, ep);
		
		if ( val == STREQUAL )
		{
			if ( fp != NULLSTR && sp != NULLSTR )
			{
				ep = Malloc((sp-fp)+1);
				(void)strcpy(ep, fp+1);
			}
			else
			if ( fp != NULLSTR )
			{
				ep = Malloc(strlen(fp));
				(void)strcpy(ep, fp+1);
			}
			else
			{
				ep = Malloc(1);
				*ep = '\0';
			}

			if ( sp != NULLSTR )
				*sp = Sun3ERs;
			
			UnQuoteChars(ep, Sun3EQ);

			return ep;
		}

		if ( (ep = sp) == NULLSTR )
			break;

		*ep++ = Sun3ERs;
	}

	return NULLSTR;
}



/*
**	Special Sun3 GetEnvBool.
*/

bool
s3getenv_bool(name)
	char *		name;
{
	register char *	ep;
	register char *	fp;
	register char *	sp;
	register int	val;

	if ( (ep = HdrEnv) == NULLSTR || *ep == '\0' )
		return false;

	for ( ;; )
	{
		if ( (sp = strchr(ep, Sun3ERs)) != NULLSTR )
			*sp = '\0';
		
		if ( (fp = strchr(ep, Sun3EUs)) != NULLSTR )
			val = strncmp(name, ep, fp-ep);
		else
			val = strcmp(name, ep);
		
		if ( val == STREQUAL )
		{
			if ( sp != NULLSTR )
				*sp = Sun3ERs;

			return true;
		}

		if ( (ep = sp) == NULLSTR )
			break;

		*ep++ = Sun3ERs;
	}

	return false;
}



/*
**	Special Sun4 MakeEnv
*/

char *
s3make_env(env, name, val)
	char *	env;
	char *	name;
	char *	val;
{
	static char	ers[]	= { ENV_RS, '\0' };
	static char	eus[]	= { ENV_US, '\0' };

	QuoteChars(val, EnvRestricted);
	return concat(env, ers, name, eus, val, NULLSTR);
}



/*
**	Pass Sun4 message to Sun3.
*/

/*
**	Sun3 magic.
*/

char	Sun3ERsa[]	= { Sun3ERs, '\0' };
char	Sun3EUsa[]	= { Sun3EUs, '\0' };
char	Sun3RQa[]	= { Sun3RS, Sun3RT, '\0' };

/*
**	Miscellaneous
*/

char *	HRp;			/* Pointer into converted route */
bool	Noprefix;		/* TRUE if ``Prefix'' invalid */
char *	Prefix;			/* Explicit part from HdrDest */


char *	s4make_env();
bool	s4cnvroute();
char *	s4cnvaddress();
void	s4cnvhdr();



/*
**	Do unlink commands for message parts.
*/

void
s4cleanup()
{
	register Cmd *	cmdp;

	for ( cmdp = PartCmds.ch_first ; cmdp != (Cmd *)0 ; cmdp = cmdp->cd_next )
		if ( cmdp->cd_type == unlink_cmd )
			(void)unlink(cmdp->cd_value);
}



/*
**	Convert Sun4 address into Sun3 address, stripping explicit where inconvenient.
*/

char *
s4cnvaddress(xp, address)
	register char *	xp;		/* Converted address */
	char *		address;	/* Original address */
{
	register char *	ap = address;
	register char *	np;

	switch ( *ap++ )
	{
	case ATYP_MULTICAST:
		do
		{
			*xp++ = ',';
			if ( (np = strchr(ap, ATYP_MULTICAST)) != NULLSTR )
				*np++ = '\0';
			if ( Prefix != NULLSTR && strchr(ap, ATYP_EXPLICIT) != NULLSTR )
				Noprefix = true;
			xp = s4cnvaddress(xp, ap);
		}
		while
			( (ap = np) != NULLSTR );
		break;

	case ATYP_EXPLICIT:
		if ( (np = strrchr(ap, ATYP_EXPLICIT)) == NULLSTR )
			np = ap;	/* Malformed! */
		else
		{
			*np++ = '\0';
			if ( *ap == ATYP_DESTINATION )
				ap++;
			if ( !Noprefix && strccmp(ap, LinkAddress) != STREQUAL )
			{
				if ( Prefix == NULLSTR )
					Prefix = address;
				else
				if ( strccmp(Prefix, address) != STREQUAL )
					Noprefix = true;
			}
		}
		xp = StripDEnd(xp, np, OzAu, Au, false);
		break;

	default:
		xp = StripDEnd(xp, --ap, OzAu, Au, false);
		break;
	}

	return xp;
}



/*
**	Convert Sun4->Sun3 environment.
**	(HDR_FLAGS -> ENV_ strings.)
**	Magic strings from Sun3/Include/header.h
*/

void
s4cnvenv()
{
	register char *	cp;
	register char *	ep;

	cp = s4make_env(EmptyString, ENV3_ID, HdrID);	/* Saved in case message crosses back into Sun4 */

	if ( (ep = GetEnv(ENV_FLAGS)) != NULLSTR )
	{
		if ( strcmp(ep, STATE_ENQ_ALL) == STREQUAL )
		{
			cp = concat(cp, Sun3ERsa, ENV3_ENQALL, NULLSTR);
			HdrFlags &= ~HDR_ENQ;
		}
		else
			cp = s4make_env(cp, ENV3_HANDLER_FLAGS, ep);
	}

	if ( HdrFlags & HDR_ACK )
		cp = concat(cp, Sun3ERsa, ENV3_ACK, NULLSTR);
	if ( HdrFlags & HDR_ENQ )
		cp = concat(cp, Sun3ERsa, ENV3_ENQ, NULLSTR);
	if ( HdrFlags & HDR_NO_AUTOCALL )
		cp = concat(cp, Sun3ERsa, ENV3_NO_AUTOCALL, NULLSTR);
	if
	(
		NoOpt
		&&
		strcmp(HdrHandler, MAILHANDLER) != STREQUAL
		&&
		strcmp(HdrHandler, STATEHANDLER) != STREQUAL
	)
		cp = concat(cp, Sun3ERsa, ENV3_NOOPT, NULLSTR);
	if ( HdrFlags & HDR_NORET )
		cp = concat(cp, Sun3ERsa, ENV3_NORET, NULLSTR);
	if ( Returned )
		cp = concat(cp, Sun3ERsa, ENV3_RETURNED, NULLSTR);
	if ( HdrFlags & HDR_REV_CHARGE )
		cp = concat(cp, Sun3ERsa, ENV3_REV_CHARGE, NULLSTR);
	if ( HdrSubpt == STATE_PROTO && OrigDataLength > 0 )
		cp = concat(cp, Sun3ERsa, ENV3_STATE, NULLSTR);

	if ( (ep = GetEnv(ENV_DEST)) != NULLSTR )
		cp = s4make_env(cp, ENV3_DESTINATION, ep);
	if ( (ep = GetEnv(ENV_DUP)) != NULLSTR )
		cp = s4make_env(cp, ENV3_DUP, ep);
	if ( (ep = GetEnv(ENV_ERR)) != NULLSTR )
		cp = s4make_env(cp, ENV3_ERR1, ep);
	if ( (ep = GetEnv(ENV_ROUTE)) != NULLSTR )
	{
		register char *	np;
		register char *	sp;

		HRp = np = newstr(ep);
		*np = '\0';					/* Might be no route */
		if ( (sp = GetEnv(ENV_SOURCE)) == NULLSTR )
			sp = HdrDest;
		(void)ExRoute(sp, ep, s4cnvroute);
		cp = s4make_env(cp, ENV3_ROUTE, np);
	}
	if ( (ep = GetEnv(ENV_TRUNC)) != NULLSTR )
		cp = s4make_env(cp, ENV3_TRUNC, ep);

	HdrEnv = cp;
}



/*
**	Check FTP message and pass on if all correct.
*/

void
s4cnvftp()
{
	register FthUlist *	up;
	register char *		cp;

	if ( Returned )
		goto returned;

	if ( FthType[0] & FTH_DATACRC )
	{
		if ( !CheckData(&DataCmds, (Ulong)0, DataLength, &DataCrc) )
		{
			Recover(ert_return);
			Error(Fth_error, '2', FTHREASON(fth_baddatacrc));
			finish(EX_RETMESG);
			return;
		}

		CRCIn = true;
	}

	if ( HdrPart[0] != '\0' )
	{
		if
		(
			!AllParts
			(
				&DataCmds,
				(Ulong)0,
				DataLength,
				MesgLength,
				&PartCmds,
				newstr(OutFile),
				StringCRC32(LinkAddress)
			)
		)
		{
			finish(EX_DROPMESG);
			return;
		}

		LinkCmds(&PartCmds, &Commands, (Ulong)0, PartsLength, newstr(OutFile), Time);

		if ( CRCIn )
		{
			/*
			**	Re-create CRC for whole message
			*/

			DataCrc = 0;
			(void)CheckData(&PartCmds, (Ulong)0, PartsLength, &DataCrc);
		}
	}
	else
returned:	LinkCmds(&DataCmds, &Commands, (Ulong)0, DataLength, newstr(OutFile), Time);

	/*
	**	Convert addresses. (Remove any trailing Au domain.)
	*/

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
	{
		if ( up->u_name == NULLSTR )
			continue;

		if ( (cp = up->u_dest) != NULLSTR )
			up->u_dest = StripDomain(cp, OzAu, Au, true);
	}

	/*
	**	Write new FTP header.
	**	Magic numbers from Sun3/Include/ftheader.h
	*/

	SetFthTo();
	
	FthType[0] = (1<<2)|0100;
	if ( HdrFlags & HDR_RQA )
		FthType[0] |= (1<<0);
	if ( CRCIn )
		FthType[0] |= (1<<1);

	while ( WriteFtHeader(OutFd, (long)0) == SYSERROR )
		Syserror(CouldNot, WriteStr, OutFile);

	DataLength = FthLength;

	s4cnvhdr();
}



/*
**	Convert Sun4->Sun3 header.
*/

void
s4cnvhdr()
{
	register CmdV	cmdv;
	register char *	cp;

	/*
	**	Convert environment.
	*/

	s4cnvenv();

	/*
	**	Convert HdrRoute.
	*/

	HRp = cp = newstr(HdrRoute);	/* Used by s4cnvroute() */
	*cp = '\0';			/* Might be no route */
	(void)ExRoute(HdrSource, HdrRoute, s4cnvroute);
	HdrRoute = cp;

	/*
	**	Other header fields.
	*/

	HdrType = HDR_TYPE1;
	HdrID = NULLSTR;
	HdrPart = NULLSTR;
	HdrQuality = '\0';
	HdrFlags = 0;

	/*
	**	Convert addresses. (Remove any trailing Au domain.)
	*/

	HdrSource = StripDomain(HdrSource, OzAu, Au, false);

	/*
	**	Convert Sun4 destination into a single Sun3 destination.
	**	(Might have to throw away explicit routing.)
	*/

	Noprefix = false;
	cp = newstr(HdrDest);		/* Must get shorter */

	Noprefix = false;
	Prefix = NULLSTR;
	(void)s4cnvaddress(cp, HdrDest);

	if ( cp[0] == '\0' )
	{
		finish(EX_DROPMESG);	/* HdrDest == Au */
		return;
	}

	if ( !Noprefix && Prefix != NULLSTR )
		HdrDest = concat(StripDomain(Prefix, OzAu, Au, false), "!", cp, NULLSTR);
	else
		HdrDest = cp;

	/*
	**	Write new message header.
	*/

	while ( WriteHeader(OutFd, (long)DataLength, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, OutFile);

	MesgLength = DataLength + HdrLength;

	/*
	**	Describe OutFile.
	*/

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = OutFile, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = (Ulong)0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = MesgLength, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = OutFile, cmdv));
}



/*
**	Check state (or other) message and pass on if all correct.
*/

void
s4cnvother()
{
	if ( HdrFlags & HDR_RETURNED )
		Returned = true;

	if
	(
		!Returned
		&&
		HdrPart[0] != '\0'
		&&
		!AllParts(
				&DataCmds,
				(Ulong)0,
				DataLength,
				MesgLength,
				&PartCmds,
				newstr(OutFile),
				StringCRC32(LinkAddress)
			 )
	)
	{
		exit(EX_DROPMESG);
		return;
	}

	if ( !Returned && HdrPart[0] != '\0' )
		LinkCmds(&PartCmds, &Commands, (Ulong)0, PartsLength, newstr(OutFile), Time);
	else
		LinkCmds(&DataCmds, &Commands, (Ulong)0, DataLength, newstr(OutFile), Time);

	DataLength = (Ulong)0;

	s4cnvhdr();
}



/*
**	Convert Sun4 route to Sun3 route.
*/
/*ARGSUSED*/
bool
s4cnvroute(from, tt, to)
	char *		from;
	Ulong		tt;
	char *		to;
{
	register char *	cp;
	char		numb[12];

	if ( (cp = strchr(to, DOMAIN_SEP)) != NULLSTR )
		*cp = '\0';
	if ( (cp = strrchr(to, TYPE_SEP)) != NULLSTR )
		to = ++cp;

	(void)sprintf(numb, "%lu", (PUlong)tt);
	*HRp++ = Sun3RS;
	HRp = strcpyend(HRp, to);
	*HRp++ = Sun3RT;
	HRp = strcpyend(HRp, numb);

	return true;
}



/*
**	Special MakeEnv for Sun3 headers.
*/

char *
s4make_env(env, name, val)
	char *	env;
	char *	name;
	char *	val;
{
	QuoteChars(val, Sun3RQa);
	return concat(env, Sun3ERsa, name, Sun3EUsa, val, NULLSTR);
}
#endif	/* SUN3 == 1 */
