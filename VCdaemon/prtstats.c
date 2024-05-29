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


/*
**	Print statistics.
*/

#define	STDIO

#include	"global.h"

#undef	VCDAEMON_STATS
#define	VCDAEMON_STATS	1

#undef	PROTO_STATS
#define	PROTO_STATS	1

#include	"debug.h"

#define	CHNSTATSDATA
#define	DMNSTATSDATA
#define	PKTSTATSDATA

#include	"driver.h"
#include	"packet.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"pktstats.h"
#include	"status.h"


char *	ChnFlags[] =
{
	"MSGFILE",
	"RDUP",
	"EOF",
	"IGNBUF",
	"WARNED",
	"LOW SPACE"
};

#define	NFLAGS	((sizeof ChnFlags)/(sizeof ChnFlags[0]))

extern bool	AllData;
extern bool	NoFormat;

char	nfmt[]	= "%-3s %d %-32.32s\t%10lu\n";
char	cfmt[]	= "%-3s _ %-32.32s\t%10lu\n";
char	xfmt[]	= "%-3s %d %-32.32s\t%10lx\n";
char	sfmt[]	= "%-3s %d %-32.32s\t%s\n";
char	rfmt[]	= "%-3s %d %-32.32s\t";
char	dfmt[]	= "%-3s _ %-32.32s\t";

#define	Fprintf	(void)fprintf

static void	prtstats _FA_((FILE *, char *, Ulong *, char **, int, int));



/*
**	Channel data.
*/

void
prtchndata(fd, out, dir)
	FILE *		fd;
	bool		out;
	char *		dir;
{
	register Chan *	chnp;
	register int	i;
	register char *	cp;
	register Uint	j;
	register int	k;
	char		tbuf[DATETIMESTRLEN];

	static char	desc[]	= english("%s data for channel %d:-\n");

	if ( !NoFormat )
		dir = out?english("Output"):english(" Input");

	for ( chnp = Channels, i = 0 ; i < NCHANNELS ; i++, chnp++ )
	{
		if ( chnp->ch_state == CH_ENDED || chnp->ch_msgid[0] == '\0' )
		{
			if ( !AllData )
				continue;
			if ( chnp->ch_msgid[0] == '\0' )
				chnp->ch_msgid[0] = '?';
		}

		if ( !NoFormat )
			Fprintf(fd, desc, dir, i);

		if ( (k = chnp->ch_number) != i )
			Fprintf(fd, english("\tbad channel number (%d) for channel %d\n"), k, i);

		switch ( chnp->ch_state )
		{
		default:		cp = "**UNK**";		break;
		case CH_ACTIVE:		cp = "ACTIVE";		break;
		case CH_ENDED:		cp = "ENDED";		break;
		case CH_ENDING:		cp = "ENDING";		break;
		case CH_IDLE:		cp = "IDLE";		break;
		case CH_STARTING:	cp = "STARTING";	break;
		}

		if ( NoFormat )
		{
			Fprintf(fd, sfmt, dir, i, english("chn_state"), cp);
			Fprintf(fd, rfmt, dir, i, english("chn_flags"));
		}
		else
			Fprintf(fd, english("\tstate %s, flags:"), cp);

		for ( j = 1, k = 0 ; k < NFLAGS ; k++, j <<= 1 )
			if ( chnp->ch_flags & j )
			{
				Fprintf(fd, " %s", ChnFlags[k]);
				chnp->ch_flags &= ~j;
			}
		if ( (j = chnp->ch_flags) )
			Fprintf(fd, english("unknown flag(s) %#x"), j);
		Fprintf(fd, "\n");

		if ( NoFormat )
		{
			Fprintf(fd, sfmt, dir, i, english("ID"), chnp->ch_msgid);
			Fprintf(fd, nfmt, dir, i, english("size"), (PUlong)chnp->ch_msgsize);
			Fprintf(fd, nfmt, dir, i, english("addr"), (PUlong)chnp->ch_msgaddr);
			Fprintf(fd, nfmt, dir, i, english("time"), (PUlong)chnp->ch_msgtime);
		}
		else
		Fprintf
		(
			fd, english("\tmessage ID \"%s\", size %lu, addr %lu\n\ttime %s\n"),
			chnp->ch_msgid, (PUlong)chnp->ch_msgsize, (PUlong)chnp->ch_msgaddr,
			DateTimeStr(chnp->ch_msgtime, tbuf)
		);

		if ( out )
			if ( NoFormat )
			{
				Fprintf(fd, xfmt, dir, i, english("parts"), (PUlong)chnp->ch_partlist);
				Fprintf(fd, xfmt, dir, i, english("current"), (PUlong)chnp->ch_current);
				Fprintf(fd, nfmt, dir, i, english("count"), (PUlong)chnp->ch_count);
				Fprintf(fd, nfmt, dir, i, english("cntrltime"), (PUlong)chnp->ch_cntrltime);
			}
			else
			Fprintf
			(
				fd, english("\tparts %#lx, current %#lx, count %d, cntrltime %s\n"),
				(PUlong)chnp->ch_partlist, (PUlong)chnp->ch_current, chnp->ch_count,
				DateTimeStr(chnp->ch_cntrltime, tbuf)
			);
		else
			if ( NoFormat )
			{
				Fprintf(fd, xfmt, dir, i, english("gaplist"), (PUlong)chnp->ch_gaplist);
				Fprintf(fd, nfmt, dir, i, english("eomcount"), (PUlong)chnp->ch_eomcount);
				Fprintf(fd, nfmt, dir, i, english("goodaddr"), (PUlong)chnp->ch_goodaddr);
				Fprintf(fd, nfmt, dir, i, english("naktime"), (PUlong)chnp->ch_chkpttime);
			}
			else
			Fprintf
			(
				fd, english("\tgaplist %#lx, eomcount %d, goodaddr %lu, naktime %s\n"),
				(PUlong)chnp->ch_gaplist, chnp->ch_eomcount, (PUlong)chnp->ch_goodaddr,
				DateTimeStr(chnp->ch_chkpttime, tbuf)
			);

		if ( NoFormat )
		{
			Fprintf(fd, nfmt, dir, i, english("dbuf_start"), (PUlong)chnp->ch_bufstart);
			Fprintf(fd, nfmt, dir, i, english("dbuf_end"), (PUlong)chnp->ch_bufend);
		}
		else
		Fprintf(fd, english("\tdata buffer start %lu end %lu\n"), (PUlong)chnp->ch_bufstart, (PUlong)chnp->ch_bufend);
	}
}



/*
**	Channel statistics.
*/

void
prtchnstats(fd, out, dir)
	FILE *		fd;
	bool		out;
	char *		dir;
{
	register Chan *	chnp;
	register int	i;
	register Ulong *stats;
	register char **names;
	register char *	index;
	register int	size;

	static char	desc[]	= "Output statistics for channel 0";
	static char	fmt[]	= "%s statistics for channel 0";

	if ( out )
	{
		index = "Output";
		names = WCHstNames;
		size = (int)wch_nstats;
	}
	else
	{
		index = "Input ";
		names = RCHstNames;
		size = (int)rch_nstats;
	}

	if ( !NoFormat )
	{
		(void)sprintf(desc, fmt, index);
		index = &desc[strlen(desc)-1];
		dir = desc;
	}

	for ( chnp = Channels, i = 0 ; i < NCHANNELS ; i++, chnp++ )
	{
		stats = out?(Ulong *)chnp->ch_rw.ch_w.wc_stats:(Ulong *)chnp->ch_rw.ch_r.rc_stats;
		prtstats(fd, dir, stats, names, size, i);
		if ( !NoFormat )
			(*index)++;
	}
}



/*
**	Daemon statistics.
*/

void
prtdmnstats(fd, dir)
	FILE *	fd;
	char *	dir;
{
	if ( !(Status.st_flags & CHDMNSTATS) )
		return;

	prtstats
	(
		fd,
		NoFormat?dir:english("Daemon statistics"),
		(Ulong *)DmnStats,
		DmnStNames,
		DS_NSTATS,
		-1
	);
}



/*
**	Packet statistics.
*/

void
prtpktstats(fd, dir)
	FILE *	fd;
	char *	dir;
{
	if ( !(Status.st_flags & CHPROSTATS) )
		return;

	prtstats
	(
		fd,
		NoFormat?dir:english("Packet protocol statistics"),
		(Ulong *)PktStats,
		PktStNames,
		PS_NSTATS,
		-1
	);
}



/*
**	Any statistics.
*/

static void
prtstats(fd, ds, sp, dp, size, chan)
	FILE *		fd;
	char *		ds;
	register Ulong *sp;
	register char **dp;
	register int	size;
	int		chan;
{
	register Ulong	s;
	register bool	first = NoFormat?false:true;

	while ( --size >= 0 )
		if ( (s = *sp++) )
		{
			if ( first )
			{
				first = false;
				Fprintf(fd, "%s:-\n", ds);
			}

			if ( NoFormat )
			{
				register char *	np = newstr(*dp++);
				register char *	cp;
				register int	c;

				cp = np;
				while ( (c = *cp++) )
					if ( c == ' ' )
						cp[-1] = '_';

				if ( chan == -1 )
					Fprintf(fd, cfmt, ds, np, (PUlong)s);
				else
					Fprintf(fd, nfmt, ds, chan, np, (PUlong)s);

				free(np);
			}
			else
				Fprintf(fd, "%20lu %s\n", (PUlong)s, *dp++);
		}
		else
			++dp;
}
