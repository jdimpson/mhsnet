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
**	Manipulate statistics.
*/

#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"handlers.h"
#include	"params.h"
#include	"stats.h"
#include	"sysexits.h"

#define	HDR_FLAGS_DATA
#include	"header.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters set from arguments.
*/

bool	DescHandler;	/* Show handlers by description */
char *	Destination;	/* Select destination */
char *	DestAddress;	/* Destination variations */
bool	InBound;	/* Select inbound records */
char *	Name = sccsid;
bool	OutBound;	/* Select outbound records */
bool	ShowDate;	/* Print record date */
bool	ShowDelay;	/* Print last link delay */
bool	ShowDest;	/* Print message destination */
bool	ShowFlags;	/* Print message flags */
bool	ShowHandler;	/* Print message handler */
bool	ShowHeader;	/* Show header */
bool	ShowID;		/* Print message IDs */
bool	ShowLink;	/* Print last link name */
bool	ShowSize;	/* Print message size */
bool	ShowSource;	/* Print message source */
bool	ShowTravel;	/* Print message travel-time */
char *	Source;		/* Select source */
char *	SourceAddress;	/* Source variations */
char *	StatsFile;	/* Statistics records file */
int	Traceflag;
bool	Verbose;	/* Show full addresses */

#define	SET_DEFAULTS	!(ShowDate||ShowDelay||ShowDest||ShowFlags||ShowHandler||ShowID||ShowLink||ShowSize||ShowSource||ShowTravel)

/*
**	Arguments.
*/

AFuncv	getAll _FA_((PassVal, Pointer));	/* All record options */
AFuncv	getDest _FA_((PassVal, Pointer));	/* Select by destination */
AFuncv	getFile _FA_((PassVal, Pointer));	/* Alternate statistics file */
AFuncv	getHandler _FA_((PassVal, Pointer));	/* Select by handler */
AFuncv	getSource _FA_((PassVal, Pointer));	/* Select by source */
AFuncv	getType _FA_((PassVal, Pointer));	/* Address type printed */

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, 0, getAll),
	Arg_bool(d, &ShowDest, 0),
	Arg_bool(f, &ShowFlags, 0),
	Arg_bool(h, &ShowHandler, 0),
	Arg_bool(i, &ShowID, 0),
	Arg_bool(j, &ShowTravel, 0),
	Arg_bool(k, &ShowDelay, 0),
	Arg_bool(l, &ShowLink, 0),
	Arg_bool(n, &DescHandler, 0),
	Arg_bool(p, &ShowHeader, 0),
	Arg_bool(r, &InBound, 0),
	Arg_bool(s, &ShowSource, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_bool(w, &OutBound, 0),
	Arg_bool(x, &ShowDate, 0),
	Arg_bool(z, &ShowSize, 0),
	Arg_string(t, 0, getType, english("External>|<Internal"), OPTARG|OPTVAL),
	Arg_string(A, &HdrHandler, getHandler, english("handler"), OPTARG),
	Arg_string(D, &Destination, getDest, english("destination"), OPTARG),
	Arg_string(F, &StatsFile, getFile, english("stats-file>|<-"), OPTARG|OPTVAL),
	Arg_string(S, &Source, getSource, english("source"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_end
};

/*
**	Statistic record.
*/

typedef struct
{
	char *	delay;
	char *	dest;
	char *	flags;
	char *	handler;
	char *	id;
	char *	link;
	char *	size;
	char *	source;	
	char *	time;
	char *	tt;
	char *	type;
}
	Stat;

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"FILESERVERLOG",	&FILESERVERLOG,			PSPOOL},
};

/*
**	Miscellaneous definitions.
*/

char *	(*AdrsFuncP) _FA_((Addr *))		= UnTypAddress;

void	print_flags _FA_((Stat *));
void	process _FA_((FILE *));
bool	select_stats _FA_((Stat *));
char *	stripexpl _FA_((char *));

#define	Fprintf	(void)fprintf

DODEBUG(extern int malloc_debug(int));




/*
**	Argument processing.
*/

int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	FILE *	fd;

	DODEBUG((void)malloc_debug(MALLOC_DEBUG));

	InitParams();

	GetParams("fileserver", Params, sizeof Params);

	StatsFile = concat(SPOOLDIR, STATSDIR, STATSFILE, NULLSTR);

	DoArgs(argc, argv, Usage);

	if ( SET_DEFAULTS )
	{
		if ( StatsFile == FILESERVERLOG )
		{
			ShowDate = true;
			ShowDest = true;
			ShowFlags = true;
			ShowHandler = true;
			DescHandler = false;
			InBound = false;
			OutBound = true;
		}
		else
		{
			ShowDate = true;
			ShowDest = true;
			ShowSource = true;
			ShowHandler = true;
		}
	}

	if ( DescHandler )
		ShowHandler = true;

	if ( !InBound && !OutBound )
	{
		InBound = true;
		OutBound = true;
	}

	if ( strcmp(StatsFile, "-") == STREQUAL )
		fd = stdin;
	else
	while ( (fd = fopen(StatsFile, "r")) == NULL )
		Syserror(CouldNot, ReadStr, StatsFile);

	setbuf(stdout, SObuf);

	process(fd);

	(void)fflush(stdout);

	exit(EX_OK);
	return 0;
}



/*
**	Error clean up.
*/

void
finish(error)
	int	error;
{
	exit(error);
}



/*
**	Turn on all record options.
*/

/*ARGSUSED*/
AFuncv
getAll(val, arg)
	PassVal		val;
	Pointer		arg;
{
	ShowDate = true;
	ShowDelay = true;
	ShowDest = true;
	ShowFlags = true;
	ShowHandler = true;
	ShowID = true;
	ShowLink = true;
	ShowSize = true;
	ShowSource = true;
	ShowTravel = true;

	return ACCEPTARG;
}



/*
**	Get optional destination selection.
*/

/*ARGSUSED*/
AFuncv
getDest(val, arg)
	PassVal	val;
	Pointer	arg;
{
	Addr *	ap = StringToAddr(newstr(val.p), NO_STA_MAP);

	DestAddress = newstr(TypedAddress(ap));
	FreeAddr(&ap);

	return ACCEPTARG;
}



/*
**	Get optional alternate statistics file.
*/

/*ARGSUSED*/
AFuncv
getFile(val, arg)
	PassVal	val;
	Pointer	arg;
{
	if ( val.p == NULLSTR || val.p[0] == '\0' )
	{
		AdrsFuncP = TypedAddress;
		StatsFile = FILESERVERLOG;
	}

	return ACCEPTARG;
}



/*
**	Set different handler.
*/

/*ARGSUSED*/
AFuncv
getHandler(val, arg)
	PassVal			val;
	Pointer			arg;
{
	register Handler *	hp;

	if
	(
		(hp = GetHandler(val.p)) == (Handler *)0
		&&
		(hp = GetDHandler(val.p)) == (Handler *)0
	)
		return (AFuncv)english("unknown handler");

	HdrHandler = hp->handler;

	return ACCEPTARG;
}



/*
**	Get optional source selection.
*/

/*ARGSUSED*/
AFuncv
getSource(val, arg)
	PassVal	val;
	Pointer	arg;
{
	Addr *	ap = StringToAddr(newstr(val.p), NO_STA_MAP);

	SourceAddress = newstr(TypedAddress(ap));
	FreeAddr(&ap);

	return ACCEPTARG;
}



/*
**	Get optional type for addresses.
*/

/*ARGSUSED*/
AFuncv
getType(val, arg)
	PassVal		val;
	Pointer		arg;
{
	switch ( val.p[0] | 040 )
	{
	case 040:	AdrsFuncP = UnTypAddress;	break;
	case 'e':
	case 'x':	AdrsFuncP = ExpTypAddress;	break;
	case 'i':	AdrsFuncP = TypedAddress;	break;
	default:	return (AFuncv)english("unrecognised types option");
	}

	return ACCEPTARG;
}



/*
**	Read next statistics record.
*/

Stat *
getrec(fd)
	register FILE *		fd;
{
	register int		c;
	register char *		cp;
	register char *		rp;
	register StMesg_t	state;

	static Stat		stat;
	static char		buf[1024];

flush:
	cp = EmptyString;
	stat.delay = cp;
	stat.dest = cp;
	stat.flags = cp;
	stat.handler = cp;
	stat.id = cp;
	stat.link = cp;
	stat.size = cp;
	stat.source = cp;
	stat.time = cp;
	stat.tt = cp;

	rp = cp = buf;
	state = (StMesg_t)-1;

	while ( (c = getc(fd)) != EOF )
		switch ( c )
		{
		default:
			*cp++ = c;
			if ( cp >= &buf[sizeof buf] )
				--cp;
			continue;

		case ST_SEP:
		case ST_RE_SEP:
			*cp++ = '\0';
			if ( state == (StMesg_t)-1 )
				stat.type = rp;
			else
			if ( stat.type[0] == ST_F_S_LOG )
			{
				switch ( (FSlog_t)state )
				{
				case fl_dest:	stat.source = stat.dest = rp;	break;
				case fl_id:	stat.id = rp;		break;
				case fl_names:	stat.flags = rp;	break;
				case fl_service:stat.handler = rp;	break;
				case fl_size:	stat.size = rp;		break;
				case fl_time:	stat.time = rp;		break;
				}
				if ( (state == (StMesg_t)fl_last || c == ST_SEP) && select_stats(&stat) )
					return &stat;
			}
			else
			{
				switch ( state )
				{
				case sm_delay:	stat.delay = rp;	break;
				case sm_dest:	stat.dest = rp;		break;
				case sm_flags:	stat.flags = rp;	break;
				case sm_handler:stat.handler = rp;	break;
				case sm_id:	stat.id = rp;		break;
				case sm_link:	stat.link = rp;		break;
				case sm_size:	stat.size = rp;		break;
				case sm_source:	stat.source = rp;	break;
				case sm_time:	stat.time = rp;		break;
				case sm_tt:	stat.tt = rp;		break;
				}
				if ( (state == sm_last || c == ST_SEP) && select_stats(&stat) )
					return &stat;
			}
			if ( c == ST_SEP )
				goto flush;
			state = (StMesg_t)(1+(int)state);
			rp = cp;
			continue;
		}

	return (Stat *)0;
}



/*
**	Print flags.
*/

void
print_flags(sp)
	Stat *			sp;
{
	register char **	cpp;
	register int		bit;
	register HFtype		flags = (HFtype)atol(sp->flags);
	register bool		first = true;

	for ( bit = 1, cpp = HdrFlagsDescs ; *cpp != NULLSTR ; cpp++, bit <<= 1 )
		if ( bit & flags )
		{
			if ( first )
			{
				first = false;
				(void)putc(' ', stdout);
				(void)putc('[', stdout);
			}
			else
				(void)putc('|', stdout);

			Fprintf(stdout, "%s", *cpp);
		}

	if ( !first )
		(void)putc(']', stdout);
}



/*
**	Stats processing.
*/

void
process(fd)
	FILE *			fd;
{
	register Stat *		sp;
	register Handler *	hp;

	char		dateb[DATETIMESTRLEN];
	char		timeb[TIMESTRLEN];

	static char	fmtdt[] = "%s";
	static char	fmtdr[] =   " %s";
	static char	fmtsc[] =      " %-23.23s";
	static char	fmtscV[] =     " %-23s";
#	if	0
	static char	fmtds[] =		" %-23.23s";
	static char	fmtdsV[] =		" %-23s";
#	else	/* 0 */
#	define		fmtds			fmtsc
#	define		fmtdsV			fmtscV
#	endif	/* 0 */
	static char	fmthn[] =			 " %-11s";
	static char	fmtID[] =			 " %-14s";
	static char	fmttt[] =			       " %7s";
	static char	fmtsz[] =			       " %10s";
#	if	0
	static char	fmtlk[] =				    " %-23.23s";
	static char	fmtlkV[] =				    " %-23s";
#	else	/* 0 */
#	define		fmtlk					    fmtsc
#	define		fmtlkV					    fmtscV
#	endif	/* 0 */
	static char	fmtdl[] =					    " %7s";

	if ( ShowHeader )
	{
		if ( ShowDate )
		{
			(void)sprintf(dateb, "%*s", DATETIMESTRLEN-1, EmptyString);
			Fprintf(stdout, fmtdt, dateb);
		}
		if ( InBound && OutBound )
			Fprintf(stdout, fmtdr, "   ");
		if ( ShowSource )
			Fprintf(stdout, fmtsc, english("Source"));
		if ( ShowDest )
			Fprintf(stdout, fmtds, english("Destination"));
		if ( ShowHandler )
			Fprintf(stdout, fmthn, (StatsFile == FILESERVERLOG)?english("Service"):english("Handler"));
		if ( ShowID )
			Fprintf(stdout, fmtID, english("ID"));
		if ( ShowTravel )
			Fprintf(stdout, fmttt, english(" Travel"));
		if ( ShowSize )
			Fprintf(stdout, fmtsz, english("Size"));
		if ( ShowLink )
			Fprintf(stdout, fmtlk, english("Link"));
		if ( ShowDelay )
			Fprintf(stdout, fmtdl, english("  Delay"));
		if ( ShowFlags )
			Fprintf(stdout, " %s", (StatsFile == FILESERVERLOG)?english("Files"):english("Flags"));
		putc('\n', stdout);
	}

	while ( (sp = getrec(fd)) != (Stat *)0 )
	{
		if ( DescHandler && (hp = GetHandler(sp->handler)) != (Handler *)0 )
			sp->handler = hp->descrip;

		if ( ShowDate )
			Fprintf(stdout, fmtdt, DateTimeStr((Time_t)atol(sp->time), dateb));
		if ( InBound && OutBound )
			Fprintf(stdout, fmtdr, (*sp->type==ST_INMESG)?english("IN "):english("OUT"));
		if ( ShowSource )
		{
			if ( Verbose )
				Fprintf(stdout, fmtscV, sp->source);
			else
				Fprintf(stdout, fmtsc, sp->source);
		}
		if ( ShowDest )
		{
			if ( Verbose )
				Fprintf(stdout, fmtdsV, sp->dest);
			else
				Fprintf(stdout, fmtds, stripexpl(sp->dest));
		}
		if ( ShowHandler )
			Fprintf(stdout, fmthn, sp->handler);
		if ( ShowID )
			Fprintf(stdout, fmtID, sp->id);
		if ( ShowTravel )
			Fprintf(stdout, fmttt, TimeString((Time_t)atol(sp->tt), timeb));
		if ( ShowSize )
			Fprintf(stdout, fmtsz, sp->size);
		if ( ShowLink )
		{
			if ( Verbose )
				Fprintf(stdout, fmtlkV, sp->link);
			else
				Fprintf(stdout, fmtlk, sp->link);
		}
		if ( ShowDelay )
			Fprintf(stdout, fmtdl, TimeString((Time_t)atol(sp->delay), timeb));
		if ( ShowFlags )
		{
			if ( StatsFile == FILESERVERLOG )
				Fprintf(stdout, " %s", sp->flags);
			else
				print_flags(sp);
		}

		putc('\n', stdout);
	}
}



/*
**	Select stats record.
*/

bool
select_stats(sp)
	register Stat *	sp;
{
	Addr *		ap;
	static char *	fs1;
	static char *	fs2;

	if ( *sp->type == ST_INMESG )
	{
		if ( !InBound )
			return false;
	}
	else
		if ( !OutBound )
			return false;

	if
	(
		HdrHandler != NULLSTR
		&&
		strcmp(HdrHandler, sp->handler) != STREQUAL
	)
		return false;

	if
	(
		Source != NULLSTR
		&&
		strccmp(Source, sp->source) != STREQUAL
		&&
		!AddressMatch(SourceAddress, sp->source)
	)
		return false;

	if
	(
		Destination != NULLSTR
		&&
		strccmp(Destination, sp->dest) != STREQUAL
		&&
		!AddressMatch(DestAddress, sp->dest)
	)
		return false;

	/*
	**	Fix up addresses.
	*/

	if ( ShowSource )
	{
		if ( AdrsFuncP == UnTypAddress )
			(void)StripCopyEnd(sp->source, sp->source);
		else
		if ( AdrsFuncP == ExpTypAddress )
		{
			ap = StringToAddr(sp->source, NO_STA_MAP);
			FreeStr(&fs1);
			sp->source = fs1 = ExpTypAddress(ap);
			FreeAddr(&ap);
		}
	}

	if ( ShowDest )
	{
		if ( AdrsFuncP == UnTypAddress )
			(void)StripCopyEnd(sp->dest, sp->dest);
		else
		if ( AdrsFuncP == ExpTypAddress )
		{
			ap = StringToAddr(sp->dest, NO_STA_MAP);
			FreeStr(&fs2);
			sp->dest = fs2 = ExpTypAddress(ap);
			FreeAddr(&ap);
		}
	}

	if ( ShowLink )
	{
		if ( AdrsFuncP == UnTypAddress )
			(void)StripCopyEnd(sp->link, sp->link);
		else
		if ( AdrsFuncP == ExpTypAddress )
		{
			ap = StringToAddr(sp->link, NO_STA_MAP);
			FreeStr(&fs2);
			sp->link = fs2 = ExpTypAddress(ap);
			FreeAddr(&ap);
		}
	}

	return true;
}



/*
**	Strip any explicit parts from `addr'.
*/

char *
stripexpl(addr)
	register char *	addr;
{
	register char *	cp;
	register char *	np;
	register char *	xp;

	switch ( *addr++ )
	{
	default:
		return --addr;

	case ATYP_DESTINATION:
		return addr;

	case ATYP_EXPLICIT:
		if ( (cp = strrchr(addr, ATYP_EXPLICIT)) != NULLSTR )
			return stripexpl(++cp);
		return addr;

	case ATYP_MULTICAST:
		cp = xp = addr-1;
		for ( ;; )
		{
			if ( (np = strchr(addr, ATYP_MULTICAST)) != NULLSTR )
				*np++ = '\0';
			*cp++ = ATYP_MULTICAST;
			cp = strcpyend(cp, stripexpl(addr));
			if ( (addr = np) == NULLSTR )
				break;
		}
		return xp;
	}
}
