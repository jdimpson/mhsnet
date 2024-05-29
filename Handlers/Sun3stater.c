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


static char	sccsid[]	= "@(#)Sun3stater.c	1.9 05/12/16";
char *	Name	= sccsid;	/* Program invoked name */

/*
**	Pass Sun3 state message to Sun4, and then invoke SUN3STATER.
**	This must be done so that Sun3 state messages get delivered
**	locally, as well as being passed to the links via Sun3_4.
**
**	Invoked as a Sun3 `handler'.
**
**	MUST BE SETUID --> ROOT if SUN3USERNAME != NETUSERNAME.
*/

#define	RECOVER
#define	FILE_CONTROL

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"commandfile.h"
#include	"debug.h"
#include	"exec.h"
#include	"header.h"
#include	"link.h"
#include	"params.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"statefile.h"
#include	"sub_proto.h"
#include	"sysexits.h"

#ifdef	SUN3STATER

#include	"sun3.h"

/*
**	Parameters set from arguments (from Sun3 routing program).
*/

bool	Broadcast;		/* If message destination is broadcast */
char *	ComFile;		/* Sun3 message descriptor -- shouldn't happen */
char *	Destination;		/* Sun3 destination address */
char *	Environment;		/* Message header environment */
char *	Handler;		/* Non-standard domain handler */
char *	HomeAddress;		/* Sun3 name of this node */
Ulong	Length;			/* Length of data in message */
char *	LinkAddress;		/* Message arrived via this link (Sun3 name) */
char *	Message;		/* Sun3 spooled message */
char *	Source;			/* Sun3 source address */
int	Traceflag;		/* Global tracing control */
Ulong	TravelTime;		/* Total travel time including last link */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(B, &Broadcast, 0),
	Arg_string(a, &Destination, 0, english("destination"), OPTARG),
	Arg_string(b, &Handler, 0, english("handler"), OPTARG),
	Arg_string(c, &ComFile, 0, english("commandsfile"), OPTARG),
	Arg_long(d, &Length, 0, english("datalength"), 0),
	Arg_string(e, &Environment, 0, english("environment"), OPTVAL),
	Arg_string(h, &HomeAddress, 0, english("home"), 0),
	Arg_string(l, &LinkAddress, 0, english("link"), OPTVAL),
	Arg_string(s, &Source, 0, english("source"), 0),
	Arg_long(t, &TravelTime, 0, english("travel-time"), OPTVAL),
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

bool	ChUid;			/* True if NETUSERNAME != SUN3USERNAME */
CmdHead	Commands;		/* Describing data part of message when spooling */
char *	CmdsFile;		/* Spooled commands for router */
char *	CmdsTemp;		/* PENDING commands for router */
bool	CRCIn;			/* Data CRC present */
char	GlobalDest[]	= GLOBALDEST;
Ulong	LinkTravelTime;		/* Travel time over last link */
Ulong	MesgLength;		/* Length of whole message */
char *	OldDest;		/* Save HdrDest for environment */
char	Template[UNIQUE_NAME_LENGTH+1];	/* Last component of a command file name */
char *	WorkFile;		/* Template for spooling files in WORKDIR */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"SUN3LIBDIR",		&SUN3LIBDIR,			PDIR},
	{"SUN3SPOOLDIR",	&SUN3SPOOLDIR,			PDIR},
	{"SUN3STATEP",		&SUN3STATEP,			PSTRING},
	{"SUN3STATER",		&SUN3STATER,			PSTRING},
	{"SUN3USERNAME",	&SUN3USERNAME,			PSTRING},
	{"SUN3WORKDIR",		&SUN3WORKDIR,			PDIR},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};


bool	checkaddress _FA_((void));
char *	convaddress _FA_((char *));
void	convroute _FA_((void));
void	do_exec _FA_((VarArgs *));
bool	getenv_bool _FA_((char *));
char *	get_env _FA_((char *));
void	passon _FA_((void));
void	passstate _FA_((void));

#endif	/* SUN3STATER */



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
#	ifdef	SUN3STATER
	register int	fd;
	register char *	cp;
	HdrReason	hdr_reason;
	static char	hdr_error[]	= english("Message protocol header \"%s\" error");

	InitParams();

	DoArgs(argc, argv, Usage);

	GetParams(SUN3PARAMS, Params, sizeof Params);

	if ( ComFile != NULLSTR )
		Error(english("Commandsfile for state message!"));

	if ( strcmp(SUN3USERNAME, NETUSERNAME) != STREQUAL )
	{
		ChUid = true;
		GetNetUid();
	}

	InitCmds(&Commands);

	while ( (fd = open(Message, O_READ)) == SYSERROR )
		Syserror(CouldNot, OpenStr, Message);

	if ( (hdr_reason = ReadHeader(fd)) != hr_ok )
		Error(hdr_error, HeaderReason(hdr_reason));

	(void)close(fd);

	MesgLength = DataLength + HdrLength;
	LinkTravelTime = TravelTime - HdrTt;

	(void)sprintf(Template, "%*s", UNIQUE_NAME_LENGTH, EmptyString);
	WorkFile = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);

	if ( (cp = get_env("ID")) != NULLSTR )
		HdrID = cp;
	else
		HdrID = WorkFile + strlen(SPOOLDIR) + strlen(WORKDIR);

	if
	(
		!getenv_bool("NOTSUN4")
		&&
		checkaddress()
		&&
		!RecoverV(ert_here)
	)
		passstate();

	FreeStr(&WorkFile);
	Recover(ert_finish);

	passon();	/* No return */

#	endif	/* SUN3STATER */

	exit(EX_UNAVAILABLE);
	return 0;
}

#ifdef	SUN3STATER



/*
**	Check address includes home, and if so change to exactly home.
*/

bool
checkaddress()
{
	Addr *		ap;
	char *		cp;

	if ( !StringAtHome(HdrDest) )
		return false;

	ap = StringToAddr(cp = newstr(HomeName), true);

	OldDest = HdrDest;
	HdrDest = newstr(TypedAddress(ap));

	FreeAddr(&ap);
	free(cp);
	return true;
}



/*
**	Convert Sun3 address into Sun4 typed address.
*/

char *
convaddress(address)
	char *		address;
{
	register char *	cp;
	Addr *		ap;

	Trace2(1, "convaddress(%s)", address);

	if ( (cp = address) == NULLSTR || *cp == '\0' )
		return cp;

	if ( strchr(cp, ATYP_EXPLICIT) != NULLSTR && strchr(cp, ATYP_MULTICAST) != NULLSTR )
		Error("Gateway handler %s can't handle complex address \"%s\"", Name, cp);

	if ( cp[0] == '*' && (cp[1] == '\0' || cp[1] == '.' && cp[2] == '*' && cp[3] == '\0') )
		cp = GlobalDest;		/* Catch "*" & "*.*" */

	cp = newstr(cp);
	ap = StringToAddr(cp, true);
	address = newstr(TypedAddress(ap));
	FreeAddr(&ap);
	free(cp);

	Trace2(1, "convaddress ==> %s", address);

	return address;
}



/*
**	Convert route.
*/

void
convroute()
{
	register char *	cp;
	register char *	rp;
	register char *	np;
	register char *	tp;

	if ( (cp = HdrRoute) == NULLSTR || *cp++ != Sun3RS )
	{
		HdrRoute = NULLSTR;
		return;
	}

	HdrRoute = rp = Malloc(strlen(cp)*5 + 3);

	for ( ;; )
	{
		if ( (np = strchr(cp, Sun3RS)) != NULLSTR )
			*np++ = '\0';

		if ( (tp = strchr(cp, Sun3RT)) != NULLSTR )
			*tp++ = '\0';
		else
			tp = "0";	/* Malformed, but who cares? */

		cp = convaddress(cp) + 1;

		*rp++ = ROUTE_RS;
		rp = strcpyend(rp, cp);
		*rp++ = ROUTE_US;
		rp = strcpyend(rp, tp);

		if ( (cp = np) == NULLSTR )
			break;
	}
}



/*
**	Function called from Execute()
**	-- use real pathname instead of ARG(0).
*/

void
do_exec(vap)
	VarArgs *	vap;
{
	char *		prog = concat(SUN3SPOOLDIR, SUN3LIBDIR, SUN3STATER, NULLSTR);

	for ( ;; )
	{
		(void)execve(prog, &ARG(vap, 0), StripEnv());
		Syserror(CouldNot, "execve", prog);
	}
}



/*
**	Called from the errors routines to cleanup
*/

void
finish(error)
	int	error;
{
	if ( !ExInChild )
	{
		if ( WorkFile != NULLSTR )
			(void)unlink(WorkFile);

		if ( CmdsTemp != NULLSTR )
			(void)unlink(CmdsTemp);
	}

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
**	Become real Sun3 stater.
*/

void
passon()
{
	ExBuf	eb;
	Passwd	sun3user;
	char	nbuf1[NUMERICARGLEN];
	char	nbuf2[NUMERICARGLEN];

	if ( ChUid && GetUid(&sun3user, SUN3USERNAME) )
		SetUser(sun3user.P_uid, sun3user.P_gid);

	FIRSTARG(&eb.ex_cmd) = Name;	/* `real' name supplied by do_exec() */
	if ( Broadcast )
		NEXTARG(&eb.ex_cmd) = "-B";
	if ( Destination != NULLSTR )
		NEXTARG(&eb.ex_cmd) = concat("-a", Destination, NULLSTR);
	if ( Handler != NULLSTR )
		NEXTARG(&eb.ex_cmd) = concat("-b", Handler, NULLSTR);
	NEXTARG(&eb.ex_cmd) = NumericArg(nbuf1, 'd', Length);
	NEXTARG(&eb.ex_cmd) = concat("-e", Environment, NULLSTR);
	NEXTARG(&eb.ex_cmd) = concat("-h", HomeAddress, NULLSTR);
	NEXTARG(&eb.ex_cmd) = concat("-l", LinkAddress, NULLSTR);
	NEXTARG(&eb.ex_cmd) = concat("-s", Source, NULLSTR);
	NEXTARG(&eb.ex_cmd) = NumericArg(nbuf2, 't', TravelTime);
	NEXTARG(&eb.ex_cmd) = Message;

	(void)Execute(&eb, do_exec, ex_nofork, SYSERROR);	/* No return */
}



/*
**	Pass state message to Sun4 router via a commandfile and copy of Message.
*/

void
passstate()
{
	register char *		cp;
	register char *		ep;
	register int		fd;
	register CmdV		cmdv;

	/*
	**	Copy data from Sun3 message to new Sun4 message file.
	*/

	fd = create(UniqueName(WorkFile, CHOOSE_QUALITY, OPTNAME, MesgLength, Time));
	if ( ChUid )
		(void)chown(WorkFile, NetUid, NetGid);

	if ( DataLength > 0 )
	{
		(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = Message, cmdv));
		(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = (Ulong)0, cmdv));
		(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = DataLength, cmdv));

		(void)CollectData(&Commands, (Ulong)0, DataLength, fd, WorkFile);
	}

	/*
	**	Make environment. (ENV_ strings -> HDR_FLAGS.)
	**	Magic strings from Sun3/Include/header.h
	*/

	HdrFlags = HDR_NOOPT;
	if ( getenv_bool("ACK") )
		HdrFlags |= HDR_ACK;
	if ( getenv_bool("ENQ") )
		HdrFlags |= HDR_ENQ;
	if ( getenv_bool("NOCALL") )
		HdrFlags |= HDR_NO_AUTOCALL;
	if ( getenv_bool("NORET") )
		HdrFlags |= HDR_NORET;
	if ( getenv_bool("R_C") )
		HdrFlags |= HDR_REV_CHARGE;
	if ( getenv_bool("RET") )
		HdrFlags |= HDR_RETURNED;

	cp = EmptyString;
	cp = make_env(cp, ENV_FLAGS, "NOTSUN3");	/* Don't pass this back */

	if ( (ep = get_env("DEST")) != NULLSTR )
	{
		cp = make_env(cp, ENV_DEST, ep);
		free(ep);
	}
	else
		cp = make_env(cp, ENV_DEST, OldDest);
	if ( (ep = get_env("DUP")) != NULLSTR )
	{
		cp = make_env(cp, ENV_DUP, ep);
		free(ep);
	}
	if ( (ep = get_env("ENQALL")) != NULLSTR )
	{
		cp = make_env(cp, ENV_FLAGS, STATE_ENQ_ALL);
		free(ep);
	}
	if ( (ep = get_env("ERR1")) != NULLSTR )
	{
		cp = make_env(cp, ENV_ERR, ep);
		free(ep);
	}
	if ( (ep = get_env("NOTNODE")) != NULLSTR )
	{
		cp = make_env(cp, ENV_NOTREGIONS, convaddress(ep));
		free(ep);
	}
	if ( (ep = get_env("ROUTE")) != NULLSTR )
	{
		cp = make_env(cp, ENV_ROUTE, ep);
		free(ep);
	}
	if ( (ep = get_env("TRUNC")) != NULLSTR )
	{
		cp = make_env(cp, ENV_TRUNC, ep);
		free(ep);
	}

	HdrEnv = cp;

	/*
	**	Convert HdrRoute.
	*/

	convroute();

	/*
	**	Convert addresses (HdrDest set by checkaddress().)
	*/

	HdrSource = convaddress(Source);

	/*
	**	Write new message header.
	*/

	HdrType = HDR_TYPE2;
	HdrQuality = STATE_QUALITY;

	while ( WriteHeader(fd, (long)DataLength, 0) == SYSERROR )
		Syserror(CouldNot, WriteStr, WorkFile);

	(void)close(fd);

	MesgLength = DataLength + HdrLength;

	/*
	**	Pass to Sun4 router via commandsfile.
	*/

	FreeCmds(&Commands);
	(void)AddCmd(&Commands, file_cmd, (cmdv.cv_name = WorkFile, cmdv));
	(void)AddCmd(&Commands, start_cmd, (cmdv.cv_number = 0, cmdv));
	(void)AddCmd(&Commands, end_cmd, (cmdv.cv_number = MesgLength, cmdv));
	(void)AddCmd(&Commands, unlink_cmd, (cmdv.cv_name = WorkFile, cmdv));

	if ( cmdv.cv_number = LinkTravelTime )
		(void)AddCmd(&Commands, traveltime_cmd, cmdv);

	if ( (cp = LinkAddress) != NULLSTR )
		cp = convaddress(cp) + 1;
	if ( (cmdv.cv_name = cp) != NULLSTR && FindLink(cp, (Link *)0) )
		(void)AddCmd(&Commands, linkname_cmd, cmdv);

	CmdsTemp = concat(SPOOLDIR, WORKDIR, Template, NULLSTR);
	cp = concat(SPOOLDIR, ROUTINGDIR, NULLSTR);
	CmdsFile = concat(cp, Template, NULLSTR);

	fd = create(UniqueName(CmdsTemp, CHOOSE_QUALITY, OPTNAME, MesgLength, Time));
	if ( ChUid )
		(void)chown(CmdsTemp, NetUid, NetGid);

	(void)WriteCmds(&Commands, fd, CmdsTemp);
	(void)close(fd);

	/*
	**	Queue commands for router, and nudge it.
	*/

	move(CmdsTemp, UniqueName(CmdsFile, STATE_QUALITY, NOOPTNAME, (Ulong)0, Time));

	(void)DaemonActive(cp, true);

	free(cp);
	FreeStr(&CmdsTemp);
}

#endif	/* SUN3STATER */
