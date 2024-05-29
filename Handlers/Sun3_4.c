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


static char	sccsid[]	= "@(#)Sun3_4.c	1.14 05/12/16";

/*
**	Pass Sun3 message to Sun4.
**
**	Invoked as a ``link spooler''.
**
**	MUST BE SETUID --> ROOT if SUN3UID != SUN4UID.
*/

#define	FILE_CONTROL

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
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"statefile.h"
#include	"sub_proto.h"
#include	"sysexits.h"

char *	Name	= sccsid;	/* Program invoked name */

#ifdef	SUN3SPOOLDIR

#include	"sun3.h"

/*
**	Parameters set from arguments (from Sun3 routing program).
*/

char *	ComFile;		/* Sun3 message descriptor for in-place files */
char *	Destination;		/* Sun3 destination address */
char *	Environment;		/* Sun3 environment string */
char *	HomeAddress;		/* Sun3 name of this node */
char *	LinkAddress;		/* Message destined for this link (Sun3 name) */
char *	Message;		/* Sun3 spooled message */
char *	Source;			/* Sun3 source address */
int	Traceflag;		/* Global tracing control */
Ulong	TravelTime;		/* Travel time over last link */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_string(a, &Destination, 0, english("destination"), 0),
	Arg_string(c, &ComFile, 0, english("commandsfile"), OPTARG),
	Arg_string(e, &Environment, 0, english("environment"), OPTARG),
	Arg_string(h, &HomeAddress, 0, english("home"), 0),
	Arg_string(l, &LinkAddress, 0, english("link"), 0),
	Arg_string(s, &Source, 0, english("source"), 0),
	Arg_long(t, &TravelTime, 0, english("travel-time"), 0),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_noflag(&Message, 0, english("message"), 0),
	Arg_end
};

/*
**	Sun3 magic.
*/

char	Sun3EQ[]	= { Sun3ERs, Sun3EUs, '\0' };
char	Sun3RQ[]	= { Sun3RS, Sun3RT, '\0' };

/*
**	Miscellaneous
*/

bool	AckIn;			/* Delivery acknowledgement requested */
Ulong	ComLength;		/* Length of data in Sun3 commands for any directly transmitted files */
CmdHead	Commands;		/* Describing FTP part of message when spooling */
char *	CmdsFile;		/* Spooled commands for router */
char *	CmdsTemp;		/* PENDING commands for router */
bool	CRCIn;			/* Data CRC present */
char *	ExplAddress;		/* Address to be prefixed to current conversion */
char	GlobalDest[]	= GLOBALDEST;
Handler*HndlrP;			/* Message handler details */
char *	Idp;			/* Pointer to id part of temp file name created by UniqueName() */
char *	LastRtNode;		/* Inbound link for message */
int	LenExplAddr;		/* Length of string in ExplAddress */
Ulong	MesgLength;		/* Length of whole message */
bool	NoOpt;			/* Unoptimised message transmission */
bool	Returned;		/* True if message has RETURNED flag */
int	RetVal		= EX_OK;
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	WorkFile;		/* Template for spooling files in WORKDIR */


bool	getenv_bool();
char	*addexplicit(), *convaddress(), *convexpl(), *convnames(), *convroute(),
	*bangaddress(), *get_env(), *make_env();
int	pass_setup();
void	checkCRC(), passftp(), passother(), qcommands();

#endif	/* SUN3SPOOLDIR */



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
#	ifdef	SUN3SPOOLDIR
	register int	fd;
	HdrReason	hdr_reason;
	static char	hdr_error[]	= english("Message protocol header \"%s\" error");

	InitParams();

	DoArgs(argc, argv, Usage);

	GetNetUid();

	InitCmds(&Commands);

	if ( ComFile != NULLSTR )
		ComLength = ConvSun3Cmds(ComFile, &Commands);

	while ( (fd = open(Message, O_READ)) == SYSERROR )
		Syserror(CouldNot, OpenStr, Message);

	if ( (hdr_reason = ReadHeader(fd)) != hr_ok )
		Error(hdr_error, HeaderReason(hdr_reason));

	MesgLength = DataLength + HdrLength;

	if ( Environment != NULLSTR )
		HdrEnv = Environment;

	if ( getenv_bool(ENV3_RETURNED) )
		Returned = true;

	if ( (HndlrP = GetHandler(HdrHandler)) == (Handler *)0 )
	{
		static Handler	defhandler;

		defhandler.quality = LOWEST_QUALITY;
		defhandler.order = ALWAYS_ORDER;
		HndlrP = &defhandler;
	}

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	WorkFile = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	Idp = WorkFile + strlen(SPOOLDIR) + strlen(WORKDIR);

	if ( (HdrID = get_env(ENV3_ID)) == NULLSTR )	/* From previous incarnation in Sun4 */
		HdrID = Idp;

	if ( !getenv_bool(NOTSUN4) )
	switch ( HdrSubpt )
	{
	case STATE_PROTO:
		passother(true);
		break;

	case FTP:
		passftp(fd);
		break;

	default:
		passother(false);
		break;
	}

	(void)close(fd);

	exit(RetVal);
#	else	/* SUN3SPOOLDIR */
	exit(EX_UNAVAILABLE);
#	endif	/* SUN3SPOOLDIR */
	return 0;
}

#ifdef	SUN3SPOOLDIR



/*
**	Prefix each address part with explicit route.
*/

char *
addexplicit(s, x)
	char *		s;
	char *		x;
{
	register char *	cp;

	Trace3(1, "addexplicit(%s, %s)", s, x);

	ExplAddress = convnames(x, true);
	LenExplAddr = strlen(ExplAddress);

	cp = convaddress(s, true);

	FreeStr(&ExplAddress);

	Trace2(1, "addexplicit ==> %s", cp);

	return cp;
}



/*
**	Convert and then prefix "tail" with pre-converted "head".
*/

char *
bangaddress(np, head, tail, types)
	register char *	np;
	char *		head;
	register char *	tail;
	bool		types;
{
	register char *	cp;

	Trace4(1, "bangaddress(%s, %s, %d)", head, tail, types);

	if ( head[0] == ATYP_EXPLICIT )
		head++;

	if ( ExplAddress != NULLSTR && strnccmp(ExplAddress, head, LenExplAddr) != STREQUAL )
	{
		*np++ = ATYP_EXPLICIT;
		np = strcpyend(np, ExplAddress);
	}

	*np++ = ATYP_EXPLICIT;
	np = strcpyend(np, head);

	*np++ = ATYP_EXPLICIT;
	cp = convnames(tail, types);
	np = strcpyend(np, cp);
	free(cp);

	return np;
}



/*
**	Check data CRC for FTP message.
*/

void
checkCRC(fth_error)
	char *		fth_error;
{
	register CmdV	cmdv;
	CmdHead		cmdh;

	InitCmds(&cmdh);
	CopyCmds(&Commands, &cmdh);
	(void)AddCmd(&cmdh, file_cmd, (cmdv.cv_name = WorkFile, cmdv));
	(void)AddCmd(&cmdh, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&cmdh, end_cmd, (cmdv.cv_number = FtDataLength, cmdv));

	if ( !CheckData(&cmdh, (Ulong)0, ComLength+FtDataLength, &DataCrc) )
		Error(fth_error, FTHREASON(fth_baddatacrc));

	FreeCmds(&cmdh);
}



/*
**	Convert complex Sun3 address into Sun4 typed address.
*/

char *
convaddress(address, types)
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

	Trace3(1, "convaddress(%s, %ctypes)", address, types?'+':'-');

	if ( (cp = address) == NULLSTR || *cp == '\0' )
		return cp;

	{
		register int	n;

		if ( ExplAddress != NULLSTR )
			m = 2;
		else
			m = 1;

		for ( np = cp, n = 1 ; (np = strpbrk(++np, sepset)) != NULLSTR ; )
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
			Error(english("Gateway spooler can't handle complex explicit address \"%s\""), cp);
			return cp;
		}
		cp = convexpl(m, cp, types);
		if
		(
			*ep == ','
			||
			strchr(ep, ',') != NULLSTR && ep--	/* This is malformed -- but seems to occur */
		)
		{
			do
			{
				if ( (tp = strchr(++ep, ',')) != NULLSTR )
					*tp = '\0';
				*np++ = ATYP_MULTICAST;
				np = bangaddress(np, cp, ep, types);
			}
			while
				( (ep = tp) != NULLSTR );
			free(cp);
			break;
		}
		(void)bangaddress(np, cp, ep, types);
		free(cp);
		break;

	case ',':
		do
		{
			if ( (ep = strchr(cp, ',')) != NULLSTR )
				*ep++ = '\0';
			*np++ = ATYP_MULTICAST;
			cp = convnames(cp, types);
			if ( ExplAddress != NULLSTR && strccmp(ExplAddress, cp) != STREQUAL )
			{
				*np++ = ATYP_EXPLICIT;
				np = strcpyend(np, ExplAddress);
				*np++ = ATYP_EXPLICIT;
			}
			np = strcpyend(np, cp);
			free(cp);
		}
		while
			( (cp = ep) != NULLSTR );
		break;

	default:
		cp = convnames(--cp, types);
		if ( ExplAddress != NULLSTR && strccmp(ExplAddress, cp) != STREQUAL )
		{
			*np++ = ATYP_EXPLICIT;
			np = strcpyend(np, ExplAddress);
			*np++ = ATYP_EXPLICIT;
		}
		(void)strcpy(np, cp);
		free(cp);
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
convenv()
{
	register char *	ep;
	register char *	cp;

	ep = EmptyString;
	HdrFlags = 0;

	if ( getenv_bool(ENV3_ACK) )
		HdrFlags |= HDR_ACK;
	if ( getenv_bool(ENV3_ENQALL) )
	{
		ep = make_env(ep, ENV_FLAGS, STATE_ENQ_ALL);
		HdrFlags |= HDR_ENQ;
	}
	else
	if ( getenv_bool(ENV3_ENQ) )
		HdrFlags |= HDR_ENQ;
	if ( getenv_bool(ENV3_NO_AUTOCALL) )
		HdrFlags |= HDR_NO_AUTOCALL;
	if ( NoOpt )
		HdrFlags |= HDR_NOOPT;
	if ( getenv_bool(ENV3_NORET) )
		HdrFlags |= HDR_NORET;
	if ( Returned )
		HdrFlags |= HDR_RETURNED;
	if ( getenv_bool(ENV3_REV_CHARGE) )
		HdrFlags |= HDR_REV_CHARGE;
	if ( AckIn )
		HdrFlags |= HDR_RQA;

	if ( (cp = get_env(ENV3_DESTINATION)) != NULLSTR )
	{
		ep = make_env(ep, ENV_DEST, cp);
		free(cp);
	}
	if ( (cp = get_env(ENV3_DUP)) != NULLSTR )
	{
		ep = make_env(ep, ENV_DUP, cp);
		free(cp);
	}
	if ( (cp = get_env(ENV3_ERR1)) != NULLSTR )
	{
		ep = make_env(ep, ENV_ERR, cp);
		free(cp);
	}
	if ( (cp = get_env(ENV3_HANDLER_FLAGS)) != NULLSTR )
	{
		ep = make_env(ep, ENV_FLAGS, cp);
		free(cp);
	}
	if ( (cp = get_env(ENV3_NOTNODE)) != NULLSTR )
	{
		ep = make_env(ep, ENV_NOTREGIONS, convaddress(cp, true));
		free(cp);
	}
	if ( (cp = get_env(ENV3_ROUTE)) != NULLSTR && (cp = convroute(cp)) != NULLSTR )
	{
		ep = make_env(ep, ENV_ROUTE, cp);
		free(cp);
	}
	if ( (cp = get_env(ENV3_TRUNC)) != NULLSTR )
	{
		ep = make_env(ep, ENV_TRUNC, cp);
		free(cp);
	}

	HdrEnv = ep;
}



/*
**	Convert explicit Sun3 address into Sun4 typed address.
*/

char *
convexpl(count, address, types)
	int		count;
	char *		address;
	bool		types;
{
	register char *	cp;
	register char *	np;
	register char *	tp;

	Trace4(1, "convexpl(%d, %s, %d)", count, address, types);

	if ( (cp = address) == NULLSTR || *cp == '\0' )
		return cp;

	if ( ExplAddress != NULLSTR )
		count--;

	if ( count <= 0 )
		return convnames(cp, types);

	address = np = Malloc(count*180 + strlen(cp));

	do
	{
		if ( (tp = strchr(cp, '!')) != NULLSTR )
			*tp++ = '\0';
		*np++ = ATYP_EXPLICIT;
		np = strcpyend(np, convnames(cp, types));
	}
	while
		( (cp = tp) != NULLSTR );

	cp = newstr(address);
	free(address);

	Trace2(2, "convexpl ==> %s", cp);

	return cp;
}



/*
**	Convert simple Sun3 address into Sun4 typed address.
*/

char *
convnames(address, types)
	char *		address;
	bool		types;
{
	register char *	cp;
	register char *	np;
	register int	n;
	Addr *		ap;

	Trace3(1, "convnames(%s, %d)", address, types);

	if ( (cp = address) == NULLSTR || *cp == '\0' )
		return cp;

	if ( cp[0] == '*' )
	{
		if ( (cp[1] == '\0' || cp[1] == '.' && cp[2] == '*' && cp[3] == '\0') )
			cp = GlobalDest;	/* Catch "*" and "*.*" */
	}
	else
	{
		np = concat("9=", cp, NULLSTR);	/* This is risky, so check it out... */
		ap = StringToAddr(np, NO_STA_MAP);
		if ( FindAddr(ap, (Link *)0, FASTEST) != wh_none )
		{
			cp = np;
			goto ok;
		}
		FreeAddr(&ap);
		free(np);
	}

	cp = newstr(cp);
	ap = StringToAddr(cp, NO_STA_MAP);
	if ( !DESTTYPE(ap) || FindAddr(ap, (Link *)0, FASTEST) != wh_none )
		goto ok;
	n = strlen(address);
	if ( strccmp(&address[n-3], ".oz") == STREQUAL )
		goto ok;
	FreeAddr(&ap);
	np = concat(cp, ".oz.au", NULLSTR);	/* Sun3 has a nasty habit of truncating the obvious */
	ap = StringToAddr(np, NO_STA_MAP);
	free(cp);
	cp = np;

ok:
	address = newstr(types ? TypedAddress(ap) : UnTypAddress(ap));

	FreeAddr(&ap);
	free(cp);

	return address;
}



/*
**	Convert route.
**
**	Last route entry is last link.
*/

char *
convroute(route)
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
	route = rp = Malloc(len);	/* HOPE THIS IS BIG ENOUGH */

	for ( ; *cp != '\0' ; )
	{
		if ( (np = strchr(cp, Sun3RS)) != NULLSTR )
			*np++ = '\0';

		if ( (tp = strchr(cp, Sun3RT)) != NULLSTR )
			*tp++ = '\0';
		else
			tp = "0";	/* Malformed, but who cares? */

		FreeStr(&LastRtNode);
		LastRtNode = cp = convnames(cp, true);
		cp++;			/* Skip ATYP_DESTINATION */

		*rp++ = ROUTE_RS;
		rp = strcpyend(rp, cp);
		*rp++ = ROUTE_US;
		rp = strcpyend(rp, tp);

		if ( (cp = np) == NULLSTR )
			break;
	}

	return route;
}



/*
**	Called from the errors routines to cleanup.
*/

void
finish(error)
	int	error;
{
	if ( RetVal != EX_OK )
		error = RetVal;

	if ( CmdsFile != NULLSTR )
		(void)unlink(CmdsFile);

	if ( CmdsTemp != NULLSTR )
		(void)unlink(CmdsTemp);

	if ( WorkFile != NULLSTR )
		(void)unlink(WorkFile);

	(void)exit(error);
}



/*
**	Special Sun3 GetEnv.
*/

char *
get_env(name)
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
getenv_bool(name)
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
make_env(env, name, val)
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
**	Make temp file for Sun4 message, and copy in data part.
*/

int
pass_setup(datalength)
	Ulong		datalength;
{
	register int	fd;
	register CmdV	cmdv;
	CmdHead		cmds;

	fd = create(UniqueName(WorkFile, CHOOSE_QUALITY, OPTNAME, MesgLength, Time));
	(void)chown(WorkFile, NetUid, NetGid);

	if ( HndlrP->order == NEVER_ORDER )
		NoOpt = OPTNAME;
	else
	if ( HndlrP->order == ALWAYS_ORDER )
		NoOpt = NOOPTNAME;
	else
		NoOpt = getenv_bool("NOOPT");

	if ( (HdrQuality = Idp[0]) < HndlrP->quality || NoOpt )
		HdrQuality = HndlrP->quality;

	/*
	**	Copy data from Sun3 message to new Sun4 message file.
	*/

	if ( datalength > 0 )
	{
		InitCmds(&cmds);

		(void)AddCmd(&cmds, file_cmd, (cmdv.cv_name = Message, cmdv));
		(void)AddCmd(&cmds, start_cmd, (cmdv.cv_number = (Ulong)0, cmdv));
		(void)AddCmd(&cmds, end_cmd, (cmdv.cv_number = datalength, cmdv));

		if ( !CollectData(&cmds, (Ulong)0, datalength, fd, WorkFile) )
			finish(EX_OSERR);
	}

	return fd;
}



/*
**	Pass FTP message to Sun4 router via a commandfile and copy of Message.
*/

void
passftp(hfd)
	int			hfd;
{
	register FthUlist *	up;
	register char *		cp;
	register int		fd;

	FthReason	fth_reason;
	static char	fth_error[]	= english("File transfer protocol header \"%s\" error");

	if ( (fth_reason = ReadFtHeader(hfd, (long)DataLength)) != fth_ok && fth_reason != fth_type )
		Error(fth_error, FTHREASON(fth_reason));

	/*
	**	Copy data from Sun3 message to new Sun4 message file.
	*/

	fd = pass_setup(FtDataLength);

	/*
	**	Convert addresses.
	**	Magic numbers from Sun3/Include/ftheader.h
	*/

	if ( (FthType[0] & (1<<1)) && !Returned )
	{
		checkCRC(fth_error);
		CRCIn = true;
	}

	if ( FthType[0] & (1<<0) )
		AckIn = true;

	if ( GetFthTo(false) == 0 )
	{
		RetVal = EX_DATAERR;
		Error(english("addressing error - no users in list \"%s\""), UQFthTo);
		return;	/* No users! */
	}

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
	{
		if ( up->u_name == NULLSTR )
			continue;

		if ( (cp = up->u_dest) != NULLSTR )
			up->u_dest = convaddress(cp, false);
	}

	/*
	**	Write new FTP header.
	*/

	FthType[0] = FTH_TYPE;
	if ( CRCIn )
		FthType[0] |= FTH_DATACRC;

	SetFthTo();

	while ( WriteFtHeader(fd, (long)FtDataLength) == SYSERROR )
		Syserror(CouldNot, WriteStr, WorkFile);

	/*
	**	Pass to Sun4 router via commandsfile.
	*/

	qcommands(fd, FtDataLength+FthLength, false);
}



/*
**	Pass non-FTP message to Sun4 router via a commandfile and copy of Message.
*/

void
passother(state)
	bool		state;
{
	qcommands(pass_setup(DataLength), DataLength, state);
}



/*
**	Pass new message in Workfile to Sun4 router via commandsfile.
*/

void
qcommands(fd, datalength, state)
	int		fd;
	Ulong		datalength;
	bool		state;
{
	register char *	cp;
	register CmdV	cmdv;

	/*
	**	Convert environment (HdrFlags & HdrEnv).
	*/

	convenv();

	/*
	**	Convert HdrRoute.
	*/

	HdrRoute = convroute(HdrRoute);

	/*
	**	Convert addresses.
	*/

	HdrDest = addexplicit(Destination, LinkAddress);

	if ( HdrDest[0] == '\0' )
		return;

	HdrSource = convnames(Source, true);

	/*
	**	Write new message header.
	*/

	HdrType = HDR_TYPE2;

	if ( HdrQuality == CHOOSE_QUALITY )
		HdrQuality = Idp[0];

	while ( WriteHeader(fd, (long)datalength, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, WorkFile);

	(void)close(fd);

	MesgLength = datalength + HdrLength;

	/*
	**	Make commands file.
	*/

	(void)unlink(Message);			/* Remove Sun3 message */

	SetUser(NetUid, NetGid);		/* Become Sun4 */

	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = WorkFile, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = MesgLength, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = WorkFile, cmdv));

	MesgLength += ComLength;

	if ( cmdv.cv_number = TravelTime )
		(void)AddCmd(&Commands, traveltime_cmd, cmdv);

	if ( (cmdv.cv_name = LastRtNode) != NULLSTR && FindLink(LastRtNode, (Link *)0) )
		(void)AddCmd(&Commands, linkname_cmd, cmdv);

	cp = concat(SPOOLDIR, ROUTINGDIR, NULLSTR);
	CmdsFile = concat(cp, Template, NULLSTR);
	CmdsTemp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	fd = create(UniqueName(CmdsTemp, CHOOSE_QUALITY, OPTNAME, MesgLength, Time));
	(void)WriteCmds(&Commands, fd, CmdsTemp);
	(void)close(fd);

	/*
	**	Queue commands for router, and nudge it.
	*/

#	if	PRIORITY_ROUTING == 1
	move(CmdsTemp, UniqueName(CmdsFile, HdrQuality, NoOpt, state?(Ulong)0:MesgLength, Time));
#	else	/* PRIORITY_ROUTING == 1 */
	move(CmdsTemp, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time));
#	endif	/* PRIORITY_ROUTING == 1 */

	(void)DaemonActive(cp, true);
}

#endif	/* SUN3SPOOLDIR */
