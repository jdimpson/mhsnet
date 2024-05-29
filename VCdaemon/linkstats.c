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


static char	sccsid[]	= "@(#)linkstats.c	1.33 05/12/16";

/*
**	Print out statistics for each link based on contents of status files.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	SIGNALS
#define	STDIO
#define	TIME
#define	ERRNO

#include	"global.h"

#undef	VCDAEMON_STATS
#define	VCDAEMON_STATS	1

#undef	PROTO_STATS
#define	PROTO_STATS	1

#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"link.h"
#include	"lockfile.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sysexits.h"

#include	"driver.h"
#include	"packet.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"pktstats.h"
#include	"status.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */



/*
**	Version of statusfile should be:
*/

char	ChanVersion[]	= CHANVERSION;

/*
**	Arguments
*/

bool	Continuous;			/* Keep scanning status files */
char *	Name		= sccsid;	/* Program invoked name */
bool	NoHeaders;			/* No garbage */
bool	Rates;				/* Show byte rates on short listing */
int	SleepTime;			/* Delay for continuous mode scans */
bool	Totals;				/* Show channel totals */
int	Traceflag;			/* Trace level */
bool	Verbose;			/* Tell everything */
bool	Warnings;			/* Complain */

AFuncv	getLink _FA_((PassVal, Pointer));
AFuncv	getSleep _FA_((PassVal, Pointer));

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_int(c, &SleepTime, getSleep, english("seconds"), OPTARG|OPTVAL),
	Arg_bool(h, &NoHeaders, 0),
	Arg_bool(r, &Rates, 0),
	Arg_bool(t, &Totals, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_bool(w, &Warnings, 0),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_noflag(0, getLink, english("linkname>|<address"), OPTARG|MANY),
	Arg_igndups,
	Arg_end
};

/*
**	List for found links.
*/

typedef struct Lnk	Lnk;

struct Lnk
{
	Lnk *	lnk_next;		/* List */
	char *	lnk_name;		/* Link name */
	char *	lnk_lock;		/* Link lockfile */
	char *	lnk_stati[2];		/* Reader/writer status file names */
	int	lnk_count;		/* Run-on count */
	int	lnk_lockpid;		/* Lockfile value */
	char	lnk_locknode[NODE_NAME_SIZE+1];
	int	lnk_errp;		/* Error indicator */
	bool	lnk_lockactive;		/* Lockfile has active pid */
	bool	lnk_state;		/* True if ``link up'' */
	Time_t	lnk_locktime;		/* Lockfile mtime */
	Time_t	lnk_time[2];		/* Time last processed */
	bool	lnk_new[2];		/* True if data changed */
	bool	lnk_up[2];		/* True if this dir. up */
	long	lnk_upt[2];		/* Up time */
	long	lnk_elt[2];		/* Elapsed time */
	long	lnk_raw[2];		/* Last raw bytes throughput */
	long	lnk_avg[2];		/* Last data bytes throughput */
	long	lnk_pkts[2];		/* Last packets throughput */
	long	lnk_data[2];		/* Last data count */
	long	lnk_rdata[2];		/* Last raw data count */
	long	lnk_tpkts[2];		/* Last packets count */
	Ulong	lnk_errs;		/* All errors */
};

Lnk *	LinkList;			/* Head of linked list */
Lnk **	Link_Vec;			/* Vector of sorted links */
int	NLinks;				/* No. of links found */

VCstatus Stati[2];			/* Status files data */

#define	IN	0			/* Array indices */
#define	OUT	1

/*
**	Array to match channel size ranges.
*/

char *	ChanDescs[NCHANNELS] =
{
	english("high"),
	english("medium"),
	english("low"),
	english("background")
};

char	ChanHdr[]		= english("  Channels:   Priority       State    Message     Size      Bytes  Messages\n");

/*
**	Miscellaneous info.
*/

Addr *	Destinations;			/* For resolving complex address arguments */
bool	Empty[2];			/* True if all channels empty */
bool	First	= true;			/* For elapsed stats. */
Link	LinkD;				/* Info. from FindAddr(), GetLink() */

#define	LINK_NAME_SIZE	14		/* Restrict bytes in a name to this */

#define	Fprintf		(void)fprintf

/*
**	Routines
*/

bool	checkpid _FA_((int));
int	compare _FA_((const void *, const void *));
void	errf _FA_((Addr *, Addr *));
bool	Find_Links _FA_((void));
bool	linkdown _FA_((Lnk *));
bool	linkf _FA_((Addr *, Addr *, Link *));
void	LinkStats _FA_((Lnk *));
void	ListLink _FA_((char *, char *));
void	LongStats _FA_((Lnk *));
void	parameters _FA_((Lnk *));
void	printchans _FA_((int, char *));
void	printtotal _FA_((int, int, char *));
Time_t	settimes _FA_((Lnk *, long *));
void	ShortHeader _FA_((void));
void	ShortStats _FA_((Lnk *));
void	sort_links _FA_((void));
void	throughputs _FA_((Lnk *));



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	InitParams();

	while ( chdir(SPOOLDIR) == SYSERROR )
		Syserror(CouldNot, "chdir", SPOOLDIR);

	DoArgs(argc, argv, Usage);

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

	if ( NLinks == 0 && !Find_Links() )
	{
bad:		Error(english("no active network links found!"));
		exit(EX_OSFILE);
	}

	if ( Verbose )
		Rates = false;

	sort_links();

	setbuf(stdout, SObuf);

	for ( ;; )
	{
		register int	i;

		Time = time((time_t *)0);

		if ( !Verbose && !NoHeaders )
			ShortHeader();

		for ( i = 0 ; i < NLinks ; i++ )
			LinkStats(Link_Vec[i]);

		if ( !Continuous )
			break;

		putc('\f', stdout);
		(void)fflush(stdout);
		(void)sleep(SleepTime);
	}

	(void)fflush(stdout);

	exit(EX_OK);
}



/*
**	Find change in a throughput statistic.
*/

long
change(astat, statp, chngp, elt, new)
	Ulong		astat;
	register long *	statp;
	long *		chngp;
	register long	elt;
	bool		new;
{
	register long	result;
	register long	stat;
	register long	olds;

	if ( (stat = astat) <= 0 || ((olds = *statp) <= 0 && Continuous) )
		result = 0;
	else
	if
	(
		elt <= 0
		||
		(result = (stat - olds + elt-1) / elt) < 0
		||
		(result == 0 && !new)
	)
		result = *chngp;
	else
		*chngp = result;

	*statp = stat;

	return result;
}



/*
**	Scan channels for state.
*/

char *
checkchans()
{
	register Chan *	chnp;
	register int	i;
	register int	j;
	register char *	val;

	val = "idle";

	for ( j = 0 ; j < 2 ; j++ )
	{
		if ( Stati[IN].st_flags & ((j == OUT) ? CHLMQERCVD : CHRMQERCVD) )
			Empty[j] = true;
		else
			Empty[j] = false;

		for ( chnp = &Stati[j].st_channels[0], i = 0 ; i < NCHANNELS ; i++, chnp++ )
		{
			switch ( chnp->ch_state )
			{
			case CH_ENDED:
			case CH_IDLE:		continue;
			default:		break;		/* gcc -Wall */
			}

			val = english("active");
			Empty[j] = false;
			break;
		}
	}

	return val;
}



/*
**	Check that ``lock'' is active.
*/

bool
checklock(lp)
	Lnk *		lp;
{
	int		fd;
	struct stat	statb;
	LockFile	lockdata;

	DODEBUG(char	timeb[DATETIMESTRLEN]);

	Trace2(1, "checklock(%s)", lp->lnk_lock);

	if
	(
		(fd = open(lp->lnk_lock, O_READ)) == SYSERROR
		||
		read(fd, (char *)&lockdata, sizeof lockdata) != sizeof lockdata
	)
	{
		if ( fd != SYSERROR )
			(void)close(fd);

		lp->lnk_lockpid = 0;
		lp->lnk_locktime = 0;
		lp->lnk_lockactive = false;

		return false;
	}

	(void)fstat(fd, &statb);

	(void)close(fd);

	lp->lnk_lockpid = atoi(lockdata.lck_pid);
	lp->lnk_locktime = statb.st_mtime;

	Trace3(1, "pid=%d, time=%s", lp->lnk_lockpid, DateTimeStr((Time_t)statb.st_mtime, timeb));

	if ( strcmp(NodeName(), lockdata.lck_node) != STREQUAL )
	{
		lp->lnk_lockactive= true;
		(void)strcpy(lp->lnk_locknode, lockdata.lck_node);
	}
	else
	{
		if
		(
			lp->lnk_locktime > Stati[0].st_start
			||
			(
				lp->lnk_lockpid != Stati[0].st_pid
				&&
				lp->lnk_lockpid != Stati[1].st_pid
			)
		)
			lp->lnk_lockactive = checkpid(lp->lnk_lockpid);

		lp->lnk_locknode[0] = '\0';
	}

	Trace2(1, "lockactive=%s", lp->lnk_lockactive?"true":"false");

	return lp->lnk_lockactive;
}



/*
**	Check that ``pid'' is active.
*/

bool
checkpid(pid)
	int	pid;
{
	Trace2(2, "checkpid(%d)", pid);

	if ( pid == 0 )
		return false;

	if
	(
		kill(pid, SIG0) != SYSERROR
		||
		errno == EPERM
	)
		return true;

	return false;
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
Find_Links()
{
	register int	i;
	bool		found = false;

	Trace1(1, "Find_Links()");

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
**	Cleanup for error routines
*/

void
finish(error)
	int	error;
{
	(void)exit(error);
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
		name = StripTypes(LinkD.ln_rname);
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

	FreeAddr(&ap);
	free(cp);

	ListLink(link, name);

	return ACCEPTARG;
}



/*
**	Set continuous operation with optional sleep time.
*/

/*ARGSUSED*/
AFuncv
getSleep(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.l == 0 )
		SleepTime = 4;

	Continuous = true;
	First = false;
	return ACCEPTARG;
}



/*
**	Check channel flags.
*/

bool
linkdown(lp)
	register Lnk *	lp;
{
	register int	i;
	register bool	up;

	DODEBUG(char	timeb[DATETIMESTRLEN]);

	Trace3(2, "linkdown(%s) state %s", lp->lnk_name, lp->lnk_state?"up":"down");

	i = OUT;
	{
		up = (Stati[i].st_flags & CHLINKDOWN)?false:true;

		Trace4(
			3,
			"linkdown() dir %d CHLINK%s lastpkt %s",
			i,
			up?"UP":"DOWN",
			DateTimeStr(Stati[i].st_lastpkt, timeb)
		);
	}

	lp->lnk_state = up;

	return up?false:true;
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

	sp = concat(link->ln_name, Slash, READERSTATUS, NULLSTR);

	if
	(
		stat(sp, &statb) != SYSERROR
		&&
		(statb.st_mode&S_IFMT) == S_IFREG
	)
	{
		ListLink(link->ln_name, StripTypes(link->ln_rname));
		val = true;
	}
	else
	{
		if ( Warnings )
		{
			char *	fs;

			if ( link->ln_flags & FL_FOREIGN )
				fs = english("a foreign");
			else
				fs = english("an inactive");

			Warn(english("%s is %s link"), link->ln_name, fs);
		}
		val = false;
	}

	free(sp);

	return val;
}



/*
**	Read stats from status files in directory.
*/

void
LinkStats(lp)
	Lnk *		lp;
{
	register int	i;
	register int	fd;
	int		n;
	int		count;
	bool		newstats;

	Trace2(1, "link stats from \"%s\"", lp->lnk_name);

	if ( Continuous && Verbose && NLinks == 1 )
		newstats = false;
	else
		newstats = true;

	for ( i = 0 ; i < 2 ; i++ )
	{
		lp->lnk_new[i] = false;

		for
		(
			count = 0
			;
			(fd = open(lp->lnk_stati[i], O_READ)) == SYSERROR
			||
			(n = read(fd, (char *)&Stati[i], sizeof(VCstatus))) < MIN_STATUS_SIZE(Stati[0])
			||
			strcmp(Stati[i].st_version, ChanVersion) != STREQUAL
			;
			count++
		)
		{
			if ( fd != SYSERROR )
			{
				(void)close(fd);

				if ( n == 0 && count < 3 )
				{
					(void)sleep(MINSLEEP);
					continue;
				}

				if ( NLinks == 1 && (n > 0 || !Continuous) )
				{
					Warn
					(
						"%s: %s", lp->lnk_name,
						(n==0)?english("zero length"):english("status file version mis-match")
					);

					Trace6(1,
						"read size = %d, MIN_STATUS_SIZE = %d\n\t&st_version=0%lo, st_version=%s, ChanVersion=%s",
						n, MIN_STATUS_SIZE(Stati[0]),
						(long)Stati[i].st_version-(long)&Stati[i],
						Stati[i].st_version, ChanVersion
					)
					return;
				}
			}
			else
			if ( NLinks == 1 && !Continuous )
			{
				if ( Warnings )
					(void)SysWarn(CouldNot, OpenStr, lp->lnk_stati[i]);
				return;
			}

			(void)strclr((char *)&Stati[i], sizeof(VCstatus));

			goto next;
		}

		(void)close(fd);

		if ( lp->lnk_time[i] < Stati[i].st_now )
		{
			newstats = true;
			lp->lnk_new[i] = true;
		}

		if ( lp->lnk_time[i] == 0 )
		{
			if ( (lp->lnk_time[i] = Stati[i].st_start) == 0 )
				lp->lnk_time[i] = 1;
/*			(void)checklock(lp);	*/
		}
next:;
	}

	Trace2(2, "newstats=%s", newstats?"true":"false");

	i = lp->lnk_lockpid;

	if
	(
		(!checklock(lp) || lp->lnk_locktime <= Stati[0].st_start)
		&&
		i == lp->lnk_lockpid
		&&
		!newstats
		&&
		--lp->lnk_count < 0
	)
	{
		lp->lnk_count = 0;
		return;
	}

	lp->lnk_count = 1;

	if ( Verbose )
		LongStats(lp);
	else
		ShortStats(lp);
}



/*
**	Queue a network link directory.
*/

void
ListLink(link, name)
	char *		link;
	char *		name;
{
	register Lnk *	lp;
	register char *	rs = concat(link, Slash, READERSTATUS, NULLSTR);

	Trace3(1, "ListLink(%s, %s)", link, name);

	for ( lp = LinkList ; lp != (Lnk *)0 ; lp = lp->lnk_next )
		if ( strcmp(lp->lnk_stati[IN], rs) == STREQUAL )
		{
			free(rs);
			return;
		}

	lp = Talloc0(Lnk);

	lp->lnk_name = name;
	lp->lnk_lock = concat(link, Slash, LINKCMDSNAME, Slash, LOCKFILE, NULLSTR);
	lp->lnk_stati[IN] = rs;
	lp->lnk_stati[OUT] = concat(link, Slash, WRITERSTATUS, NULLSTR);

	lp->lnk_errp = 3;
	lp->lnk_errs = MAX_ULONG;

	lp->lnk_next = LinkList;
	LinkList = lp;

	NLinks++;
}



/*
**	Verbose statistics for a link.
*/

void
LongStats(lp)
	register Lnk *	lp;
{
	register char *	cp;
	long		upt;
	Time_t		now;
	char		tbuf[TIMESTRLEN];

	now = settimes(lp, &upt);

	cp = checkchans();

	if
	(
		(lp->lnk_locknode[0] || checkpid(Stati[0].st_pid) || checkpid(Stati[1].st_pid))
		||
		(
			lp->lnk_lockactive
			&&
			(cp = english("calling"))
			&&
			(now = Time)
			&&
			(upt = now - lp->lnk_locktime)
			&&
			(Stati[0].st_pid = lp->lnk_lockpid)
		)
	)
	{
		Fprintf
		(
			stdout,
			english("%s daemon (%s%s%d/%d) %s%s. Time: %.8s, up: %s.\n"),
			lp->lnk_name,
			lp->lnk_locknode,
			(lp->lnk_locknode[0]=='\0')?EmptyString:" ",
			Stati[0].st_pid,
			Stati[1].st_pid,
			cp,
			linkdown(lp)?" link down":EmptyString,
			ctime(&now)+11,
			TimeString((Time_t)upt, tbuf)
		);
	}
	else
	if ( lp->lnk_upt[0] != 0 )
	{
		char	dbuf[DATESTRLEN];

		Fprintf
		(
			stdout,
			english("%s daemon inactive since %s, was up for %s.\n"),
			lp->lnk_name,
			DateString(Stati[0].st_now, dbuf),
			TimeString((Time_t)lp->lnk_upt[0], tbuf)
		);
	}
	else
	{
		if ( NLinks == 1 )
			Fprintf(stdout, english("%s daemon inactive.\n"), lp->lnk_name);
		return;
	}

	putc('\n', stdout);
	throughputs(lp);

	putc('\n', stdout);
	parameters(lp);

	Fprintf(stdout, "\n%s", ChanHdr);

	printchans(IN, english("Receive:"));
	if ( Totals )
		printtotal(IN, IN, NULLSTR);
	printchans(OUT, english("Transmit:"));
	if ( Totals )
	{
		printtotal(OUT, OUT, NULLSTR);
		printtotal(IN, OUT, "Total");
	}

	if ( NLinks > 1 )
		putc('\n', stdout);
}



/*
**	Print VC parameters.
*/

void
parameters(lp)
	Lnk *		lp;
{
	register Chan *	chnp;
	register int	i;
	register int	j;
	register Ulong	errs;
	register Ulong	berrs;

	for ( berrs = 0, errs = 0, i = 0 ; i < 2 ; i++ )
	{
		berrs += Stati[i].st_pktstats[PS_SKIPBYTE];

		errs += Stati[i].st_pktstats[PS_BADSIZE];
		errs += Stati[i].st_pktstats[PS_BADDCRC];
		errs += Stati[i].st_pktstats[PS_BADTYPE];

		for ( chnp = &Stati[i].st_channels[0], j = 0 ; j < NCHANNELS ; j++, chnp++ )
			errs += (i==IN)?chnp->ch_statsin(rch_datanak):chnp->ch_statsout(wch_datanak);
	}

	if ( (berrs+errs) > lp->lnk_errs )
		lp->lnk_errp = i = 0;
	else
	if ( (i = ++lp->lnk_errp) > 3 )
		lp->lnk_errp = i = 3;

	lp->lnk_errs = berrs + errs;

	if ( Stati[OUT].st_pktdatasize == Stati[IN].st_pktdatasize )
		Fprintf
		(
			stdout,
			english("  Protocol: d_size %ld, "),
			Stati[OUT].st_pktdatasize
		);
	else
		Fprintf
		(
			stdout,
			english("  Protocol: osize %ld, isize %ld, "),
			Stati[OUT].st_pktdatasize,
			Stati[IN].st_pktdatasize
		);

	if ( Stati[OUT].st_flags & CHCOOKEDVC )
		Fprintf
		(
			stdout,
			english("cook %c, "),
			((Stati[OUT].st_flags&CHCOOKTYP) >> 8) + '0'
		);

	if ( Stati[OUT].st_flags & CHDATACRC16 )
		Fprintf
		(
			stdout,
			english("data CRC16, ")
		);

	if ( Stati[OUT].st_flags & CHDATACRC32 )
		Fprintf
		(
			stdout,
			english("data CRC32, ")
		);

	if ( Stati[OUT].st_flags & CHTURNAROUND )
		Fprintf
		(
			stdout,
			english("1/2dplx%s%s, "),
			(Stati[OUT].st_flags&CHINONLY)?english(" in"):EmptyString,
			(Stati[OUT].st_flags&CHOUTONLY)?english(" out"):EmptyString
		);
	else
	if ( Stati[OUT].st_flags & (CHINONLY|CHOUTONLY) )
		Fprintf
		(
			stdout,
			english("%s only, "),
			(Stati[OUT].st_flags&CHINONLY)?english("in"):english("out")
		);

	Fprintf
	(
		stdout,
		english("skip %lu, errs %lu.%s\n"),
		(PUlong)berrs,
		(PUlong)errs,
		&("***"[i])
	);
}



/*
**	Print channel states for one direction.
*/

void
printchans(dir, type)
	int		dir;
	char *		type;
{
	register Chan *	chnp;
	register char *	cp;
	register int	i;
	register Ulong	size;
	register float	addr;

	for ( chnp = &Stati[dir].st_channels[0], i = 0 ; i < NCHANNELS ; i++, chnp++ )
	{
		Fprintf
		(
			stdout,
			"  %9s%11s",
			(i==0)?type:EmptyString,
			ChanDescs[i]
		);

		if ( Empty[dir] )
			cp = english("empty");
		else
		switch ( chnp->ch_state )
		{
		default:
		case CH_ENDED:
		case CH_IDLE:		cp = english("idle");		break;
		case CH_STARTING:	cp = english("starting");	break;
		case CH_ACTIVE:		cp = english("active");		break;
		case CH_ENDING:		cp = english("terminating");	break;
		}

		if ( chnp->ch_flags & CH_LOWSPACE )
		{
			cp = english("LOW SPACE");
			chnp->ch_state = CH_STARTING;
		}

		switch ( chnp->ch_state )
		{
		case CH_STARTING:
		case CH_ACTIVE:
		case CH_ENDING:
			if ( (size = chnp->ch_msgsize) > 0 )
			{
				if ( dir == OUT || chnp->ch_msgaddr < chnp->ch_goodaddr )
					addr = chnp->ch_msgaddr;
				else
					addr = chnp->ch_goodaddr;

				Fprintf
				(
					stdout,
					"%12s%8d%%%11lu",
					cp,
					(int)(addr * 100.0 / (float)size),
					(PUlong)size
				);
				break;
			}

		default:
			Fprintf(stdout, "%12s%20s", cp, EmptyString);
		}

		Fprintf
		(
			stdout,
			"%11lu%10lu\n",
			(PUlong)((dir==IN)?chnp->ch_statsin(rch_data):chnp->ch_statsout(wch_data)),
			(PUlong)((dir==IN)?chnp->ch_statsin(rch_messages):chnp->ch_statsout(wch_messages))
		);
	}
}



/*
**	Print channel totals for both directions.
*/

void
printtotal(start, end, descr)
	int		start;
	int		end;
	char *		descr;
{
	register Chan *	chnp;
	register int	i;
	register int	dir;
	register Ulong	byts = 0;
	register Ulong	msgs = 0;

	for ( dir = start ; dir <= end ; dir++ )
		for ( chnp = &Stati[dir].st_channels[0], i = 0 ; i < NCHANNELS ; i++, chnp++ )
		{
			byts += (dir==IN)?chnp->ch_statsin(rch_data):chnp->ch_statsout(wch_data);
			msgs += (dir==IN)?chnp->ch_statsin(rch_messages):chnp->ch_statsout(wch_messages);
		}

	if ( descr != NULLSTR )
		Fprintf(stdout, "%10s:%*lu%10lu\n", descr, 54, (PUlong)byts, (PUlong)msgs);
	else
		Fprintf(stdout, "%22s%*lu%10lu\n", english("*all*"), 43, (PUlong)byts, (PUlong)msgs);
}



/*
**	Set times from Statii, return ``now''.
*/

Time_t
settimes(lp, up)
	register Lnk *	lp;
	long *		up;
{
	register int	i;
	register long	l;
	register long	upt;
	Time_t		now;

	for ( upt = 0, now = 0, i = 0 ; i < 2 ; i++ )
	{
		if ( Stati[i].st_start == 0 )
		{
			lp->lnk_upt[i] = 0;
			lp->lnk_elt[i] = 0;
		}
		else
		{
			if ( (l = Stati[i].st_now - Stati[i].st_start) <= 0 )
				l = 1;
			if ( (lp->lnk_upt[i] = l) > upt )
				upt = l;
			if ( (lp->lnk_elt[i] = Stati[i].st_now - lp->lnk_time[i]) < 0 )
				lp->lnk_elt[i] = 0;
		}

		if ( (lp->lnk_time[i] = Stati[i].st_now) > now )
			now = Stati[i].st_now;
	}

	*up = upt;
	return now;
}



/*
**	Header for ``short'' listing (one line per link.)
*/

void
ShortHeader()
{
	register int	i;
	register int	j;
	register char *	cp;

	j = LINK_NAME_SIZE+25;

	if ( Rates )
	{
		cp = english("Receive Transmit");
		j -= strlen(cp) + 2;
		Fprintf(stdout, "%*s%s", j, EmptyString, cp);
		j = 2;
	}

	cp = english("Receive Channels");
	i = strlen(cp);
	Fprintf(stdout, "%*s%s", j+((5*NCHANNELS)-i+1)/2, EmptyString, cp);
	cp = english("Transmit Channels");
	Fprintf(stdout, "%*s%s\n", ((2*5*NCHANNELS)-i-(int)strlen(cp)+2)/2, EmptyString, cp);

	Fprintf(stdout, "%-*s", LINK_NAME_SIZE+4, english("Link"));

	if ( Rates )
		Fprintf(stdout, "%19s", english("(bytes/second)"));
	else
		Fprintf(stdout, "%10s %8s", english("Bytes"), english("Messages"));

	for ( i = 0 ; i < 2 ; i++ )
	{
		putc(' ', stdout);

		for ( j = 0 ; j < NCHANNELS ; j++ )
			Fprintf(stdout, " %3d ", j);
	}

	putc('\n', stdout);
}



/*
**	One line to describe state of a link.
*/

void
ShortStats(lp)
	Lnk *		lp;
{
	register Chan *	chnp;
	register char	c;
	register int	i;
	register int	j;
	register char	ls;
	long		upt;

	if ( Rates )
		(void)settimes(lp, &upt);

	if ( lp->lnk_locknode[0] || checkpid(Stati[0].st_pid) || checkpid(Stati[1].st_pid) )
	{
		c = checkchans()[0] & ~040;	/* Yes, well ... */
		ls = linkdown(lp)?'D':'U';
	}
	else
	if ( lp->lnk_lockactive )
	{
		c = 'C';
		ls = '*';
	}
	else
	{
		c = '*';
		ls = '*';
	}

	Fprintf
	(
		stdout,
		"%-*.*s %c%c",
		LINK_NAME_SIZE, LINK_NAME_SIZE, lp->lnk_name,
		c,
		ls
	);

	if ( Rates )
		throughputs(lp);
	else
		Fprintf
		(
			stdout,
			" %10lu %8lu",
			(PUlong)(Stati[0].st_dmnstats[DS_DATA] + Stati[1].st_dmnstats[DS_DATA]),
			(PUlong)(Stati[0].st_dmnstats[DS_MESSAGES] + Stati[1].st_dmnstats[DS_MESSAGES])
		);

	for ( i = 0 ; i < 2 ; i++ )
	{
		putc(' ', stdout);
 
		for ( chnp = &Stati[i].st_channels[0], j = 0 ; j < NCHANNELS ; j++, chnp++ )
		{
			switch ( chnp->ch_state )
			{
			case CH_STARTING:	c = '+';	break;
			default:		c = ' ';	break;
			case CH_ENDING:		c = '~';	break;
			}

			switch ( chnp->ch_state )
			{
			case CH_STARTING:
			case CH_ACTIVE:
			case CH_ENDING:
				if ( chnp->ch_msgsize > 0 )
				{
					Fprintf
					(
						stdout,
						" %3d%%",
						(int)((float)chnp->ch_msgaddr * 100.0 / (float)chnp->ch_msgsize)
					);
					break;
				}

			default:
				Fprintf(stdout, "   %c ", c);
			}
		}
	}

	putc('\n', stdout);
}



/*
**	Sort the links into alphabetic order.
*/

void
sort_links()
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



/*
**	Display VC throughputs.
*/

void
throughputs(lp)
	register Lnk *	lp;
{
	register int	i;
	register long	l;
	register long	raw;
	register long	avg;
	register long	ovl;
	register long	max;
	register long	elt;
	bool		new;

	long		tmax = 0;
	long		tavg = 0;
	long		tovl = 0;
	long		traw = 0;
	long		tpkt = 0;
	long		tdata = 0;
	long		tmsgs = 0;

	static char	hdr[]	= "  %9s: %7s %7s %7s %7s %7s %13s %8s\n";
	static char	fmt[]	= "  %9s: %7lu %7lu %7lu %7lu %5lu.%lu %13lu %8lu\n";

	if ( !Rates )
	Fprintf
	(
		stdout, hdr,
		english("Bytes/sec"),
		english("average"),
		english("current"),
		english("overall"),
		english("raw"),
		english("packets"),
		english("Totals: bytes"),
		english("messages")
	);

	for ( i = 0 ; i < 2 ; i++ )
	{
		elt = lp->lnk_elt[i];
		new = lp->lnk_new[i];

		avg = change(Stati[i].st_dmnstats[DS_DATA], &lp->lnk_data[i], &lp->lnk_avg[i], elt, new);

		if ( (l = lp->lnk_upt[i]) <= 0 )
			ovl = 0;
		else
			ovl = (Stati[i].st_dmnstats[DS_DATA] + l-1) / l;

		if ( (raw = Stati[i].st_act_time) > 0 )
			max = (Stati[i].st_act_bytes + raw/2) / raw;
		else
			max = 0;

		if ( avg == 0 || !Continuous )
			avg = Stati[i].st_datarate;

		raw = change(Stati[i].st_dmnstats[DS_VCDATA], &lp->lnk_rdata[i], &lp->lnk_raw[i], elt, new);

		l = change
		    (
			(Stati[i].st_pktstats[PS_RPKTS] + Stati[i].st_pktstats[PS_XPKTS]) * 10,
			&lp->lnk_tpkts[i],
			&lp->lnk_pkts[i],
			elt,
			new
		    );

		if ( Rates )
			Fprintf(stdout, " %9lu", (PUlong)(First?max:avg));
		else
		Fprintf
		(
			stdout, fmt,
			(i==IN) ? english("Input") : english("Output"),
			(PUlong)max,
			(PUlong)avg,
			(PUlong)ovl,
			(PUlong)raw,
			(PUlong)(l/10), (PUlong)(l%10),
			(PUlong)Stati[i].st_dmnstats[DS_DATA],
			(PUlong)Stati[i].st_dmnstats[DS_MESSAGES]
		);

		tmax += max;
		tavg += avg;
		tovl += ovl;
		traw += raw;
		tpkt += l;
		tdata += Stati[i].st_dmnstats[DS_DATA];
		tmsgs += Stati[i].st_dmnstats[DS_MESSAGES];
	}

	if ( !Rates )
	Fprintf
	(
		stdout, fmt,
		english("Total"),
		tmax,
		tavg,
		tovl,
		traw,
		tpkt/10, tpkt%10,
		tdata,
		tmsgs
	);
}
