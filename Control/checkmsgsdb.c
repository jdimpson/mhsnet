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


static char	sccsid[]	= "@(#)checkmsgsdb.c	1.17 05/12/16";

/*
**	Copy and compress the messages data-base.
*/

#define	FILE_CONTROL
#define	RECOVER
#define	SIGNALS
#define	STAT_CALL
#define	STDIO

#include	"global.h"
#include	"Args.h"
#include	"debug.h"
#include	"msgsdb.h"
#include	"Passwd.h"
#include	"spool.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters set from arguments.
*/

bool	Check;				/* Just check */
bool	IgnoreRouter;			/* Ignore running router */
char *	Name	= sccsid;		/* Program invoked name */
char *	NewFile;			/* Compress data-base to this file */
bool	Silent;				/* Don't warble */
int	Traceflag;			/* Trace level */
bool	Verbose;			/* Show active messages */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(c, &Check, 0),
	Arg_bool(r, &IgnoreRouter, 0),
	Arg_bool(s, &Silent, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_string(O, &NewFile, 0, english("out file"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_end
};

/*
**	Miscellaneous info.
*/

bool	Bckt_full;			/* Set true if bucket has > 1 entries */
bool	Bckt_used;			/* Set true if bucket non-empty */
Uint	Bckts_full;			/* Count of buckets with > 1 entries */
Uint	Bckts_used;			/* Count of non-empty buckets read */
Uint	Buffers_used;			/* Count of non-empty buffers read */
Passwd	Invoker;			/* Person invoked by */
long	Messages;			/* Count of active message entries */

/*
**	Routines
*/

void	compact _FA_((void));
bool	search_bucket _FA_((Uint));

extern SigFunc	sigcatch;



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	char *	cp;

	InitParams();

	DoArgs(argc, argv, Usage);

	GetInvoker(&Invoker);
	SetUser(NetUid, NetGid);

	if ( geteuid() != NetUid || !(Invoker.P_flags & P_SU) )
	{
		Warn(english("No permission."));
		exit(EX_NOPERM);
	}

	cp = concat(SPOOLDIR, ROUTINGDIR, NULLSTR);

	if ( Check && NewFile == NULLSTR )
		NewFile = DevNull;
	else
	if ( !IgnoreRouter && NewFile == NULLSTR && DaemonActive(cp, false) )
	{
		Warn(english("routing daemon active."));
		exit(EX_UNAVAILABLE);
	}

	free(cp);

	if ( (SigFunc *)signal(SIGINT, SIG_IGN) != SIG_IGN )
		(void)signal(SIGINT, sigcatch);

	SetUlimit();

	OpenMsgsDB();

	compact();

	CloseMsgsDB();

	(void)close(MDB_wfd);

	if ( !Silent )
	{
		double	x, y, z;	/* Some cc couldn't handle complex FP statements */

		if ( Bckts_used==0 )
			x = y = z = 0.0;
		else
		{
			if ( Verbose )
			{
				x = (Bckts_full*100.0)/Bckts_used;
				y = ((Bckts_used-Messages)*100.0)/Bckts_used;
			}

			z = ((Buffers_used-MDB_writes)*100.0)/Buffers_used;
		}

		if ( Verbose )
			(void)printf
			(
				english("%lu active entries from %u buckets\n%.0f%% multiple occupancy\n%lu entries expired (%.0f%%)\n"),
				(PUlong)Messages, Bckts_used, x, (PUlong)(Bckts_used-Messages), y
			);

		(void)printf
		(
			english("%.0f%% compaction, %lu entries in %u file buffers (%luKb).\n"),
			z, (PUlong)Messages, MDB_writes, (PUlong)((MDB_writes*MDB_bsize)/1024)
		);
	}

	exit(EX_OK);
}



/*
**	Cleanup for error routines.
*/

void
finish(error)
	int	error;
{
	Recover(ert_exit);	/* Prevent error recursion */

	if ( MDB_wname != MDB_rname && MDB_wname != NULLSTR && NewFile == NULLSTR )
		(void)unlink(MDB_wname);

	CloseMsgsDB();

	(void)close(MDB_wfd);

	(void)exit(error);
}



/*
**	Create new data-base file, open old one,
**	copy non-empty records, and move new to old.
*/

void
compact()
{
	register long	addr;
	register long	max_addr;
	register Uint	bucket;
	register Uint	last_count;
	struct stat	statb;

	while ( fstat(MDB_rfd, &statb) == SYSERROR )
		Syserror(CouldNot, StatStr, MDB_rname);

	if ( (max_addr = statb.st_size) == 0 )
		return;

	if ( (MDB_wname = NewFile) == NULLSTR )
		MDB_wname = concat(MDB_rname, ".Q", NULLSTR);

	if ( strcmp(MDB_wname, DevNull) != STREQUAL )
	{
		(void)unlink(MDB_wname);
		MDB_wfd = create(MDB_wname);
	}
	else
		while ( (MDB_wfd = open(MDB_wname, O_WRITE)) == SYSERROR )
			Syserror(CouldNot, OpenStr, MDB_wname);

	last_count = 0;

	for ( bucket = 0, addr = 0 ; addr < max_addr ; addr += BUCKET_SIZE, bucket++ )
	{
		if ( ReadMsgsDB(addr) && search_bucket(bucket) )
			WriteMsgsDB(addr);

		if ( Bckt_used )
		{
			if ( MDB_reads > last_count )
				Buffers_used++;
			last_count = MDB_reads;

			if ( Bckt_full )
			{
				Bckts_full++;
				Bckt_full = false;
			}

			Bckts_used++;
			Bckt_used = false;
		}
	}

	if ( NewFile == NULLSTR )
	{
		move(MDB_wname, MDB_rname);
		FreeStr(&MDB_wname);
	}
}



/*
**	Count entries in record.
*/

bool
search_bucket(bucket)
	Uint			bucket;
{
	register MDB_Ent *	entry;
	register int		count;
	register int		slen;
	register bool		val = false;
	char *			str = NullStr;
	char			timeb[TIMESTRLEN];
	static char		fmt[] = "%-*.*s %-*.*s %*.*s %5u\n";

	Trace2(9, "search_bucket(%u)", bucket);

	for ( entry = MDB_bucket, count = 0 ; count < ENTRY_COUNT ; count++, entry++ )
	{
		if ( entry->timeout == 0 )
			continue;

		Bckt_used = true;

		Trace5(3, "bucket %5d, entry %d, timeout %lx, length %d", bucket, count, (PUlong)entry->timeout, entry->len);

		if ( entry->timeout >= Time )
		{
			Messages++;

			if ( val )
				Bckt_full = true;
			else
				val = true;

			if ( Verbose )
				str = TimeString(entry->timeout - Time, timeb);
		}
		else
		{
			str = english("EXPIRED");
			entry->timeout = 0;
		}

		if ( (slen = entry->len) == 0 )
		{
			Warn("bucket %d entry %d length error [0]", bucket, count);
			entry->timeout = 0;
			continue;
		}

		if ( (slen -= UNIQUE_NAME_LENGTH) > SOURCE_LENGTH )
		{
			Warn("bucket %d entry %d length error [%d > %d]", bucket, count, entry->len, sizeof entry->id);
			entry->timeout = 0;
			continue;
		}

		if ( Verbose )
			(void)printf
			(
				fmt,
				SOURCE_LENGTH, slen, entry->id,
				UNIQUE_NAME_LENGTH, UNIQUE_NAME_LENGTH, &entry->id[slen],
				TIMESTRLEN-1, TIMESTRLEN-1, str,
				bucket
			);
	}

	return val;
}



/*
**	Catch signals to clean up.
*/

void
sigcatch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	finish(sig);
}
