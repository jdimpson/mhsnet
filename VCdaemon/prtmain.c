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


static char	sccsid[]	= "@(#)prtmain.c	1.18 05/12/16";

/*
**	Virtual circuit daemon status printer.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	STDIO

#include	"global.h"

#undef	VCDAEMON_STATS
#define	VCDAEMON_STATS	1

#undef	PROTO_STATS
#define	PROTO_STATS	1

#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"link.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"driver.h"
#include	"exec.h"	/* For _ExInChild */
#include	"packet.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"pktstats.h"
#include	"status.h"


/*
**	Arguments.
*/

bool	AllData;		/* All data from channels */
char *	Name	= sccsid;	/* Program invoked name */
bool	NoFormat;		/* Don't pretty-print data */
int	Traceflag;
bool	Warnings;		/* Whinge */

AFuncv	getLink _FA_((PassVal, Pointer));

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, &AllData, 0),
	Arg_bool(f, &NoFormat, 0),
	Arg_bool(w, &Warnings, 0),
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_noflag(0, getLink, "linkname>|<address", OPTARG|MANY),
	Arg_end
};

/*
**	Version of statusfile should be:
*/

char	ChanVersion[]	= CHANVERSION;

/*
**	List for found links.
*/

typedef struct Lnk	Lnk;

struct Lnk
{
	Lnk *	lnk_next;		/* List */
	char *	lnk_rname;		/* Link typed name */
	char *	lnk_name;		/* Link name */
	char *	lnk_dir;		/* Link directory */
};

Lnk *	LinkList;			/* Head of linked list */
Lnk **	Link_Vec;			/* Vector of sorted links */
int	NLinks;				/* No. of links found */

/*
**	Miscellaneous info.
*/

Addr *	Destinations;			/* For resolving complex address arguments */
bool	GotLink;			/* Specific link */
Link	LinkD;				/* Info. from FindAddr(), GetLink() */

extern void	prtchndata _FA_((FILE *, bool, char *));

int	compare _FA_((const void *, const void *));
void	dolink _FA_((Lnk *));
void	errf _FA_((Addr *, Addr *));
bool	findlinks _FA_((void));
bool	linkf _FA_((Addr *, Addr *, Link *));
void	listlink _FA_((char *, char *));
void	sortlinks _FA_((void));

#define	Fprintf	(void)fprintf



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	register Lnk **	lpp;
	register int	i;

	InitParams();

	while ( chdir(SPOOLDIR) == SYSERROR )
		Syserror(CouldNot, "chdir", SPOOLDIR);

	DoArgs(argc, argv, Usage);

	if ( NoFormat )
		AllData = true;

	if ( Destinations != (Addr *)0 )
	{
		Addr *	source = StringToAddr(newstr(HomeName), false);
		Passwd	nullpw;

		nullpw.P_name = Name;
		nullpw.P_flags = P_ALL;
		nullpw.P_region = NULLSTR;

		(void)CheckAddr(&Destinations, &nullpw, Error, true);
		(void)DoRoute(source, &Destinations, FASTEST, (Link *)0, (DR_fp)linkf, errf);
		FreeAddr(&source);
		FreeAddr(&Destinations);

		if ( NLinks == 0 )
			goto bad;
	}

	if ( NLinks == 0 && !findlinks() )
	{
bad:		Error(english("no active network links found!"));
		exit(EX_OSFILE);
	}

	sortlinks();

	setbuf(stdout, SObuf);

	for ( lpp = Link_Vec, i = NLinks ; --i >= 0 ; lpp++ )
		dolink(*lpp);

	(void)fflush(stdout);

	exit(EX_OK);
	return 0;
}



/*
**	Alpha compare.
*/

int
#ifdef	ANSI_C
compare(const void *lpp1, const void *lpp2)
#else	/* ANSI_C */
compare(lpp1, lpp2)
	char *	lpp1;
	char *	lpp2;
#endif	/* ANSI_C */
{
	return strccmp((*(Lnk **)lpp1)->lnk_name, (*(Lnk **)lpp2)->lnk_name);
}



/*
**	Unknown address error.
*/
/*ARGSUSED*/
void
errf(source, dest)
	Addr *	source;
	Addr *	dest;
{
	if ( AddrAtHome(&dest, false, false) )
		Error(english("This IS \"%s\"!"), UnTypAddress(dest));
	else
		Error(english("Link for \"%s\" unknown."), UnTypAddress(dest));
}



/*
**	Search routing table for links.
*/

bool
findlinks()
{
	register int	i;
	bool		found = false;

	Trace1(1, "findlinks()");

	(void)CheckRoute(NOSUMCHECK);

	for ( i = LinkCount ; --i >= 0 ; )
	{
		GetLink(&LinkD, (Index)i);

		Trace2(2, "found \"%s\"", LinkD.ln_name);

		if ( linkf((Addr *)0, (Addr *)0, &LinkD) )
			found = true;
	}

	return found;
}



/*
**	Cleanup on error termination.
*/

void
finish(error)
	int	error;
{
	exit(error);
}



/*
**	Find name of explicit link or save address for later resolution.
*/

/*ARGSUSED*/
AFuncv
getLink(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	cp;
	register char *	name;
	register char *	link	= val.p;
	Addr *		ap;
	struct stat	statb;

	if ( (name = strrchr(link, '/')) != NULLSTR )
		name++;
	else
		name = link;

	ap = StringToAddr(cp = newstr(name), NO_STA_MAP);

	if ( FindLink(TypedName(ap), &LinkD) )
	{
		name = LinkD.ln_rname;
		link = LinkD.ln_name;
	}
	else
	if
	(
		stat(link, &statb) == SYSERROR
		||
		(statb.st_mode&S_IFMT) != S_IFDIR
	)
	{
		AddAddr(&Destinations, ap);
		return ACCEPTARG;
	}
	else
		name = newstr(TypedName(ap));

	FreeAddr(&ap);
	free(cp);

	listlink(link, name);

	GotLink = true;

	return ACCEPTARG;
}



/*
**	Print out stats from link.
*/

void
dolink(lp)
	Lnk *		lp;
{
	register int	i;
	register int	fd;
	char *		linkdir;
	char *		statusnames[2];
	char *		dir;
	char		tbuf1[DATETIMESTRLEN];
	char		tbuf2[DATETIMESTRLEN];
	char		tbuf3[DATETIMESTRLEN];

	extern char	cfmt[];
	extern char	dfmt[];

	linkdir = concat(SPOOLDIR, lp->lnk_dir, "/", NULLSTR);
	statusnames[0] = concat(linkdir, READERSTATUS, NULLSTR);
	statusnames[1] = concat(linkdir, WRITERSTATUS, NULLSTR);

	for ( i = 0 ; i < 2 ; i++ )
	{
		if
		(
			(fd = open(statusnames[i], O_READ)) == SYSERROR
			||
			Read(fd, (char *)&Status, sizeof Status, statusnames[i]) != sizeof Status
			||
			strcmp(Status.st_version, ChanVersion) != STREQUAL
		)
		{
			if ( fd != SYSERROR )
			{
				(void)close(fd);
				Warn(english("Link \"%s\": status file version mis-match"), lp->lnk_name);
			}
			else
				(void)SysWarn(CouldNot, OpenStr, statusnames[i]);

			continue;
		}

		if ( !NoFormat )
		{
			if ( i == 0 )
				Fprintf(stdout, "\nLink %s:-\n\tInput statistics\n\n", lp->lnk_name);
			else
				Fprintf(stdout, "\n\tOutput statistics\n\n");
		}
		else
		{
			dir = i?"out":"in";
			if ( NLinks > 1 || !GotLink )
				dir = concat(lp->lnk_name, " ", dir, NULLSTR);
			else
				dir = newstr(dir);
		}

		if ( NoFormat )
		{
			Fprintf(stdout, cfmt, dir, english("started"), StartTime);
			Fprintf(stdout, cfmt, dir, english("current"), CTime);
			Fprintf(stdout, cfmt, dir, english("last_pkt"), PacketTime);
			Fprintf(stdout, cfmt, dir, english("thru_byt"), ActiveBytes);
			Fprintf(stdout, cfmt, dir, english("thru_sec"), ActiveTime);
			Fprintf(stdout, cfmt, dir, english("thru_put"), ActiveBytes/ActiveTime);
		}
		else
		Fprintf
		(
			stdout, "\tStarted:\t%s\n\tNow:\t\t%s.\n\tLast packet:\t%s\n\tThroughput:\t%lu bytes, %lu secs. ==> %lu b/s.\n",
			DateTimeStr(StartTime, tbuf1),
			DateTimeStr(CTime, tbuf2),
			DateTimeStr(PacketTime, tbuf3),
			(PUlong)ActiveBytes,
			(PUlong)ActiveTime,
			(PUlong)(ActiveBytes/ActiveTime)
		);

		if ( NoFormat )
			Fprintf(stdout, dfmt, dir, "link_flags");
		else
			Fprintf(stdout, "\tLink:\t\t");
		Fprintf(stdout, (Status.st_flags&CHLINKDOWN)?"down":"up");
		if ( Status.st_flags & CHLINKSTART ) Fprintf(stdout, " starting");
		if ( Status.st_flags & CHCOOKEDVC )
			Fprintf(stdout, " cooked %d", (Status.st_flags & CHCOOKTYP)>>8);
		if ( Status.st_flags & CHDATACRC16 ) Fprintf(stdout, " crc16");
		if ( Status.st_flags & CHDATACRC32 ) Fprintf(stdout, " crc32");
		if ( Status.st_flags & CHTURNAROUND ) Fprintf(stdout, " turn");
		if ( Status.st_flags & CHOUTONLY ) Fprintf(stdout, " out");
		if ( Status.st_flags & CHINONLY ) Fprintf(stdout, " in");
		if ( Status.st_flags & CHRMQERCVD ) Fprintf(stdout, " RMQE");
		if ( Status.st_flags & CHLMQERCVD ) Fprintf(stdout, " LMQE");

		putc('\n', stdout);

		prtdmnstats(stdout, dir);
		prtpktstats(stdout, dir);
		prtchndata(stdout, (bool)i, dir);
		prtchnstats(stdout, (bool)i, dir);

		if ( NoFormat )
			free(dir);

		(void)close(fd);
	}

	free(statusnames[1]);
	free(statusnames[0]);
	free(linkdir);
}



/*
**	List link from routing function.
*/
/*ARGSUSED*/
bool
linkf(source, dest, link)
	Addr *		source;
	Addr *		dest;
	register Link *	link;
{
	register char *	sp;
	register bool	val;
	struct stat	statb;

	Trace4(1, "linkf(%s, %s, %s)", UnTypAddress(source), UnTypAddress(dest), link->ln_rname);

	sp = concat(link->ln_name, Slash, READERSTATUS, NULLSTR);

	if
	(
		stat(sp, &statb) != SYSERROR
		&&
		(statb.st_mode&S_IFMT) == S_IFREG
	)
	{
		listlink(link->ln_name, link->ln_rname);
		val = true;
	}
	else
	{
		if ( Warnings )
			Warn(english("%s is an inactive or foreign link"), link->ln_name);
		val = false;
	}

	free(sp);

	return val;
}



/*
**	Queue a network link directory.
*/

void
listlink(link, name)
	char *		link;
	char *		name;
{
	register Lnk *	lp;

	Trace3(1, "listlink(%s, %s)", link, name);

	for ( lp = LinkList ; lp != (Lnk *)0 ; lp = lp->lnk_next )
		if ( strcmp(lp->lnk_rname, name) == STREQUAL )
			return;

	lp = Talloc(Lnk);

	lp->lnk_rname = name;
	lp->lnk_name = StripTypes(name);
	lp->lnk_dir = link;

	lp->lnk_next = LinkList;
	LinkList = lp;

	NLinks++;
}



/*
**	Sort the links into alphabetic order.
*/

void
sortlinks()
{
	register Lnk *	lp;
	register Lnk **	lpp;

	Link_Vec = lpp = (Lnk **)Malloc(sizeof(Lnk *) * NLinks);

	for ( lp = LinkList ; lp != (Lnk *)0 ; lp = lp->lnk_next )
		*lpp++ = lp;

	DODEBUG(if((lpp-Link_Vec)!=NLinks)Fatal1("bad NLinks"));

	Trace2(1, "sort %d links", NLinks);

	if ( NLinks > 1 )
		qsort((char *)Link_Vec, NLinks, sizeof(Lnk *), compare);
}
