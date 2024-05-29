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


static char	sccsid[]	= "@(#)linkstats.c	1.17 05/12/16";

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

#undef	Extern
#define	Extern
#define	ExternU
#include	"Stream.h"
#include	"exec.h"		/* For ExInChild */



/*
**	Arguments.
*/

bool	Continuous;			/* Keep scanning status files */
char *	Name		= sccsid;	/* Program invoked name */
bool	NoHeaders;			/* No garbage */
int	SleepTime;			/* Delay for continuous mode scans */
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
	Arg_bool(v, &Verbose, 0),
	Arg_bool(w, &Warnings, 0),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_noflag(0, getLink, english("linkname|address"), OPTARG|MANY),
	Arg_igndups,
	Arg_end
};

/*
**	List for found links.
*/

typedef struct LLel *	LLl_p;

typedef struct LLel
{
	LLl_p	lql_next;
	char *	lql_link;
	char *	lql_name;
}
	LLel;

LLl_p	LinkList;			/* Head of linked list */
LLl_p *	LinkSVec;			/* Vector of sorted links */
int	NLinks;				/* No. of links found */

/*
**	Array to match channel size ranges.
*/

char *	StreamSizes[NSTREAMS] =
{
	english("high"),
	english("medium"),
	english("low")
};

/*
**	Miscellaneous info.
*/

Addr *	Destinations;			/* For resolving complex address arguments */
Link	LinkD;				/* Info. from FindAddr(), GetLink() */

#define	Fprintf		(void)fprintf
#define	INDENT(A)	Fprintf(stdout,"%*s",A*2,EmptyString)
#ifdef	NODE_NAME_SIZE
#undef	NODE_NAME_SIZE
#endif	/* NODE_NAME_SIZE */
#define	NODE_NAME_SIZE	14

/*
**	Routines
*/

int	compare _FA_((const void *, const void *));
void	errf _FA_((Addr *, Addr *));
bool	Find_links _FA_((void));
bool	linkf _FA_((Addr *, Addr *, Link *));
void	LinkStats _FA_((LLl_p));
void	ListLink _FA_((char *, char *));
void	LongStats _FA_((char *));
void	ShortHeader _FA_((void));
void	ShortStats _FA_((char *));
void	sort_links _FA_((void));



int
main(argc, argv)
	int		argc;
	register char *	argv[];
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

	if ( NLinks == 0 && !Find_links() )
	{
bad:		Error(english("no network links found!"));
		exit(EX_OSFILE);
	}

	sort_links();

	setbuf(stdout, SObuf);

	for ( ;; )
	{
		register int	i;

		if ( !Verbose )
			ShortHeader();

		for ( i = 0 ; i < NLinks ; i++ )
			LinkStats(LinkSVec[i]);

		if ( !Continuous )
			break;

		putc('\f', stdout);
		(void)fflush(stdout);
		(void)sleep(SleepTime);
	}

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
	return strcmp((*(LLl_p *)lpp1)->lql_link, (*(LLl_p *)lpp2)->lql_link);
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
		*(long *)arg = 4;

	Continuous = true;
	return ACCEPTARG;
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

	sp = concat(link->ln_name, Slash, "status", NULLSTR);

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
			Warn(english("%s is an inactive or foreign link"), link->ln_name);
		val = false;
	}

	free(sp);

	return val;
}



/*
**	Print stats from state file in directory.
*/

void
LinkStats(lp)
	LLl_p		lp;
{
	register int	i;
	register int	j;
	static Time_t	lasttime;
	char *		statefile = concat(lp->lql_link, Slash, "status", NULLSTR);

	Trace2(1, "link stats from \"%s\"", statefile);

	for ( j = 0 ;; )
	if
	(
		(i = open(statefile, O_READ)) == SYSERROR
		||
		read(i, (char *)&NNstate, sizeof NNstate) != sizeof NNstate
	)
	{
		if ( i != SYSERROR )
			(void)close(i);

		if ( i == SYSERROR || ++j > 3 )
		{
			(void)strclr((char *)&NNstate, sizeof NNstate);
			break;
		}

		(void)sleep(1);
	}
	else
	{
		if ( strcmp(NNstate.version, StreamSCCSID) != STREQUAL )
		{
			if ( NLinks == 1 )
				Warn("\"status\" file version mismatch");
			else
				(void)strclr((char *)&NNstate, sizeof NNstate);
		}

		(void)close(i);
		break;
	}

	free(statefile);

	if ( Continuous && Verbose && NLinks == 1 )
	{
		if ( NNstate.thistime == lasttime )
			return;
		
		lasttime = NNstate.thistime;
	}

	if ( Verbose )
		LongStats(lp->lql_name);
	else
		ShortStats(lp->lql_name);
}



void
LongStats(link)
	char *		link;
{
	register Str_p	strp;
	register char *	cp;
	register int	i;
	register int	j;
	register Ulong	t;
	char		tbuf[TIMESTRLEN];

	if  ( NNstate.starttime == 0 )
	{
		t = 0;
		j = 0;
	}
	else
	{
		if ( (t = NNstate.thistime - NNstate.starttime) == 0 )
			t = 1;
		j = NNstate.thistime - NNstate.lasttime;
	}

	if
	(
		NNstate.procid
		&&
		(
			kill(NNstate.procid, SIG0) != SYSERROR
			||
			errno == EPERM
		)
	)
	{
		register char *	ls = NULLSTR;
		static char *	down = "down";

		switch ( NNstate.procstate )
		{
		default:
		case PROC_IDLE:		cp = "idle";	break;
		case PROC_RUNNING:	cp = "active";	break;
		case PROC_ERROR:	cp = "waiting for error to be fixed";
					ls = down;
					break;
		case PROC_OPENING:	cp = "waiting for open on device";
					ls = down;
					break;
		case PROC_CALLING:	cp = "waiting for connection";
					ls = down;
		}

		if ( ls == NULLSTR )
			switch ( NNstate.linkstate )
			{
			default:
			case DLINK_DOWN:	ls = down;	break;
			case DLINK_UP:		ls = "up";
			}

		Fprintf
		(
			stdout,
			"%s daemon (%d) %s, link %s. Time: %.8s, up: %s.\n",
			link,
			NNstate.procid,
			cp,
			ls,
			ctime(&NNstate.thistime)+11,
			TimeString(t, tbuf)
		);
	}
	else
	if ( t != 0 )
	{
		Fprintf
		(
			stdout,
			"%s daemon inactive since %.15s, was up for %s.\n",
			link,
			ctime(&NNstate.thistime)+4,
			TimeString(t, tbuf)
		);
	}
	else
	{
		if ( NLinks == 1 )
			Fprintf(stdout, "%s daemon inactive.\n", link);
		return;
	}

	{
		register Ulong	avg;

		NNstate.allbytes += inByteCount + outByteCount;

		avg = NNstate.activetime ? NNstate.allbytes/NNstate.activetime : (Ulong)0;

		Fprintf
		(
			stdout,
			"%s  (bytes/sec.)%7lu%9lu%9lu%9lu%9lu%10lu\n",
			"\n  throughputs:  assumed  maximum  average  overall  receive  transmit\n",
			(PUlong)NNstate.speed,
			(PUlong)(NNstate.maxspeed > avg ? NNstate.maxspeed : avg),
			(PUlong)avg,
			(PUlong)(t ? NNstate.allbytes/t : 0),
			(PUlong)(j ? inByteCount/j : 0),
			(PUlong)(j ? outByteCount/j : 0)
		);
	}

	Fprintf
	(
		stdout,
		"\n  protocol parameters: buffers %d, data size %3d, receive timeout %2d.\n",
		NNstate.nbufs,
		NNstate.packetsize,
		NNstate.recvtimo
	);

	Fprintf
	(
		stdout,
		"%s  %25lu%13lu%10lu%9lu\n",
		"\n  active totals: packets in  packets out     bytes   messages\n",
		(PUlong)NNstate.inpkts,
		(PUlong)NNstate.outpkts,
		(PUlong)NNstate.allbytes,
		(PUlong)NNstate.allmessages
	);

	Fprintf
	(
		stdout,
		"\n  streams:     range           state    message    size     bytes  messages\n"
	);

	for ( strp = NNstate.streams[0], i = 0 ; i < 2 ; i++ )
	{
		if ( i )
			putc('\n', stdout);

		for ( j = 0 ; j < NSTREAMS ; j++, strp++ )
		{
			Fprintf
			(
				stdout,
				"%10s%10s     ",
				j ? EmptyString : i==INSTREAMS?"receive ":"transmit",
				StreamSizes[j]
			);

			switch ( strp->str_state )
			{
			default:
			case STR_ENDED:
			case STR_IDLE:		cp = "idle";		break;
			case STR_WAIT:		cp = "waiting";		break;
			case STR_START:		cp = "starting";	break;
			case STR_ACTIVE:	cp = "active";		break;
			case STR_ENDING:	cp = "terminating";	break;
			case STR_EMPTY:		cp = "empty";		break;
			case STR_ERROR:		cp = "error";		break;
			case STR_INACTIVE:	cp = "inactive";	break;
			}

			switch ( strp->str_state )
			{
			case STR_WAIT:
			case STR_START:
			case STR_ACTIVE:
			case STR_ENDING:
				if ( strp->str_size > 0 )
				{
					Fprintf
					(
						stdout,
						"%11s%8d%%%10lu",
						cp,
						(int)(strp->str_posn * 100 / strp->str_size),
						(PUlong)strp->str_size
					);
					break;
				}

			default:
				Fprintf(stdout, "%11s%19s", cp, EmptyString);
			}

			Fprintf
			(
				stdout,
				"%10lu%10lu\n",
				(PUlong)strp->str_bytes,
				(PUlong)strp->str_messages
			);
		}
	}

	if ( NLinks > 1 )
		putc('\n', stdout);
}



void
ShortStats(link)
	char *		link;
{
	register Str_p	strp;
	register char	c;
	register int	i;
	register int	j;
	register char	ls = ' ';

	if
	(
		NNstate.procid
		&&
		(
			kill(NNstate.procid, SIG0) != SYSERROR
			||
			errno == EPERM
		)
	)
	{

		switch ( NNstate.procstate )
		{
		default:
		case PROC_IDLE:		c = 'I';	break;
		case PROC_RUNNING:	c = 'A';	break;
		case PROC_ERROR:	c = 'E';
					ls = 'D';
					break;
		case PROC_OPENING:	c = 'O';
					ls = 'D';
					break;
		case PROC_CALLING:	c = 'C';
					ls = 'D';
					break;
		}

		if ( ls == ' ' )
			switch ( NNstate.linkstate )
			{
			default:
			case DLINK_DOWN:	ls = 'D';	break;
			case DLINK_UP:		ls = 'U';;
			}

	}
	else
	{
		c = '*';
		ls = '*';
	}

	Fprintf
	(
		stdout,
		"%-*.*s %c%c %10lu %8lu",
		NODE_NAME_SIZE, NODE_NAME_SIZE, link,
		c,
		ls,
		(PUlong)NNstate.allbytes,
		(PUlong)NNstate.allmessages
	);

	for ( strp = NNstate.streams[0], i = 0 ; i < 2 ; i++ )
	{
		putc(' ', stdout);

		for ( j = 0 ; j < NSTREAMS ; j++, strp++ )
		{
			switch ( strp->str_state )
			{
			default:
			case STR_ACTIVE:
			case STR_ENDED:
			case STR_IDLE:		c = ' ';	break;

			case STR_EMPTY:		c = '_';	break;
			case STR_ENDING:	c = '~';	break;
			case STR_ERROR:		c = 'E';	break;
			case STR_INACTIVE:	c = '.';	break;
			case STR_START:		c = '+';	break;
			case STR_WAIT:		c = ',';	break;
			}

			switch ( strp->str_state )
			{
			case STR_WAIT:
			case STR_START:
			case STR_ACTIVE:
			case STR_ENDING:
				if ( strp->str_size > 0 )
				{
					Fprintf
					(
						stdout,
						" %c%3d%%",
						c,
						(int)(strp->str_posn * 100 / strp->str_size)
					);
					break;
				}

			default:
				Fprintf(stdout, "    %c ", c);
			}
		}
	}

	putc('\n', stdout);
}



void
ShortHeader()
{
	register int	i;
	register int	j;
	register char	*cp;

	cp = "Receive Channels";
	i = strlen(cp);
	Fprintf(stdout, "%*s%s", NODE_NAME_SIZE+24+((6*NSTREAMS)-i)/2, EmptyString, cp);
	cp = "Transmit Channels";
	Fprintf(stdout, "%*s%s\n", ((2*6*NSTREAMS)-i-(int)strlen(cp)+1)/2, EmptyString, cp);

	Fprintf
	(
		stdout,
		"%-*s    %10s %8s",
		NODE_NAME_SIZE, "Link",
		"Bytes",
		"Messages"
	);

	for ( i = 0 ; i < 2 ; i++ )
	{
		putc(' ', stdout);

		for ( j = 0 ; j < NSTREAMS ; j++ )
			Fprintf(stdout, "  %3d ", j);
	}

	putc('\n', stdout);
	putc('\n', stdout);
}



/*
**	Queue a network link directory.
*/

void
ListLink(link, name)
	char *		link;
	char *		name;
{
	register LLl_p	hllp;

	hllp = Talloc(LLel);

	hllp->lql_link = link;
	hllp->lql_name = name;

	hllp->lql_next = LinkList;
	LinkList = hllp;

	NLinks++;

	Trace2(1, "list link \"%s\"", hllp->lql_link);
}



/*
**	Search routing table for links.
*/

bool
Find_links()
{
	register int	i;
	bool		found = false;

	Trace1(1, "Find_links()");

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
**	Sort the links into alphabetic order
*/

void
sort_links()
{
	register LLl_p	lp;
	register LLl_p *lpp;

	LinkSVec = lpp = (LLl_p *)Malloc(sizeof(LLl_p) * NLinks);

	for ( lp = LinkList ; lp != (LLl_p)0 ; lp = lp->lql_next )
		*lpp++ = lp;

	DODEBUG(if((lpp-LinkSVec)!=NLinks)Fatal1("bad NLinks"));

	Trace2(1, "found %d links", NLinks);

	if ( NLinks > 1 )
		qsort((char *)LinkSVec, NLinks, sizeof(LLl_p), compare);
}
