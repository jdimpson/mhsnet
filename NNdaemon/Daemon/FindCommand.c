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
**	Message processing.
*/

#define	STAT_CALL

#include	"global.h"
#include	"debug.h"
#include	"spool.h"

#include	"Stream.h"
#include	"daemon.h"
#include	"driver.h"

#include	<ndir.h>


/*
**	Message file queues.
*/

typedef struct MQel	MQel;
typedef MQel *		MQl_p;

typedef enum
{
	m_wait, m_active, m_sent
}
			Use_t;

struct MQel
{
	MQl_p	mql_next;	/* list */
	Use_t	mql_use;	/* state */
	bool	mql_order;	/* message is ordered */
	char	mql_name[STREAMIDZ+1];
};

typedef struct
{
	MQl_p	cl_first;	/* list */
	MQl_p *	cl_last;	/* to build list */
	MQl_p *	cl_vec;		/* sorted list */
	int	cl_size;	/* size of cl_vec */
	int	cl_count;	/* number in list */
}
			CLhead;

typedef CLhead *	CLh_p;

static CLhead		CmdLists[NSTREAMS];	/* The sorted lists for each channel */
static MQl_p		MQfreelist;
static MQl_p		MQactive[NSTREAMS];	/* The message active on each channel */

/*
**	Queue quality -> channel policy.
*/

#define	NPOLS	10

int	Policy[NPOLS]	=
{
	0,		/* STATE_QUALITY */
	0, 0, 0,	/* '1', '2', '3' */
	1, 1, 1,	/* '4', '5', '6' */
	2, 2, 2		/* '7', '8', LOWEST_QUALITY */
};

#if	(NPOLS - 1) != (LOWEST_QUALITY - STATE_QUALITY) || NSTREAMS != 3
	*** rewrite needed ***
#endif

/*
**	Directory variables.
*/

static DIR *		CmdDirFd;
static Time_t		CmdDirMtime;
static Time_t		LastLook;

/*
**	Miscellaneous.
*/

int			cacomp _FA_((const void *, const void *));
int			slcomp _FA_((const void *, const void *));

static void		buildlists _FA_((void));
static void		checkactive _FA_((void));
static void		freelists _FA_((void));
static void		setactive _FA_((int, MQl_p));
static void		sortlists _FA_((void));



/*
**	Find lexically smallest file name for channel.
**	In the interests of speed, a sorted list of known command files
**	is maintained for each channel, and the directory is only searched
**	when a list is empty, and the modify time of the directory has changed.
*/

bool
FindCommand(chan)
	int		chan;
{
	register MQl_p *mqpp;
	register MQl_p	mqp;
	register int	i;
	register Str_p	strp = &outStreams[chan];
	register int	c;
	bool		stat_done = false;
	struct stat	statb;

	Trace3(1, "FindMessage for channel %d state %d", chan, strp->str_state);

	/*
	**	Take last entry out of list, and unlink it.
	*/

	if ( strp->str_id[0] != '\0' )
	{
		/*
		**	Unlinking the commands file will change the directory ``mtime'',
		**	(except on a MIPS!), so fetch it first.
		*/

		if ( !stat_done && CmdDirFd != NULL && stat(CmdsName, &statb) != SYSERROR )
			stat_done = true;

		Trace2(1, "unlink \"%s\"", strp->str_cmdf);

		(void)unlink(strp->str_cmdf);

		if ( (mqp = MQactive[chan]) != (MQl_p)0 )
			mqp->mql_use = m_sent;

		strp->str_id[0] = '\0';

		StID(STOUTCHAN, chan, EmptyString, strp->str_size, strp->str_posn, strp->str_time);
	}

	/*
	**	Search lists for new command file.
	*/

	for ( ;; )
	{
		/*
		**	Search higher-priority lists for highest priority message.
		*/

		for ( c = 0 ; c <= chan ; c++ )
		for ( mqpp = CmdLists[c].cl_vec, i = CmdLists[c].cl_count ; --i >= 0 ; )
		{
			if ( (mqp = *mqpp++)->mql_use != m_wait )
				continue;

			if ( c < chan && mqp->mql_order && c >= Fstream )
				continue;

			if ( mqp->mql_name[0] > Quality )
				continue;

			setactive(chan, mqp);
			return true;
		}

		/*
		**	Nothing to do, if directory hasn't changed, give up.
		*/

		MQactive[chan] = (MQl_p)0;

		while ( !stat_done )
		{
			if ( LastLook == Time )
				return false;

			if ( CmdDirFd == NULL && (CmdDirFd = opendir(CmdsName)) == NULL )
				return false;

			if ( stat(CmdsName, &statb) != SYSERROR )
				break;

			(void)SysWarn(CouldNot, StatStr, CmdsName);

			closedir(CmdDirFd);
			CmdDirFd = NULL;
		}

		Trace5(
			2,
			"stat dir mtime %lu, CmdDirMtime %lu, LastLook %lu, Time %lu",
			(PUlong)statb.st_mtime,
			(PUlong)CmdDirMtime,
			(PUlong)LastLook,
			(PUlong)Time
		);

		/*
		**	The directory may have been written in the same period
		**	that we last looked at it, but just a bit later...
		**	(of course, all this is pointless if NFS is in the way!)
		*/

		if ( LastLook > CmdDirMtime && statb.st_mtime == CmdDirMtime )
		{
			LastLook = Time;
			return false;
		}

		LastLook = Time;
		CmdDirMtime = statb.st_mtime;
		stat_done = false;

		freelists();
		buildlists();
		sortlists();
		checkactive();
	}
}



/*
**	Free all elements from lists.
*/

static void
freelists()
{
	register CLh_p	clp;
	register MQl_p	mqp;
	register int	i;

	for ( clp = CmdLists, i = NSTREAMS ; --i >= 0 ; clp++ )
	{
		if ( clp->cl_count != 0 )
		{
			Trace2(3, "free list %d", clp-CmdLists);

			while ( (mqp = clp->cl_first) != (MQl_p)0 )
			{
				clp->cl_first = mqp->mql_next;
				mqp->mql_next = MQfreelist;
				MQfreelist = mqp;
			}

			clp->cl_count = 0;
		}

		clp->cl_last = &clp->cl_first;
	}
}



/*
**	Read the directory, building lists.
*/

static void
buildlists()
{
	register MQl_p		mqp;
	register CLh_p		clp;
	register struct direct *dp;

	Trace1(2, "buildlists");

#	if	BAD_REWINDDIR
	closedir(CmdDirFd);
	if ( (CmdDirFd = opendir(CmdsName)) == NULL )
		return;
#	else	/* BAD_REWINDDIR */
	rewinddir(CmdDirFd);
#	endif	/* BAD_REWINDDIR */

	while ( (dp = readdir(CmdDirFd)) != NULL )
	{
		if ( dp->d_name[0] < STATE_QUALITY || dp->d_name[0] > LOWEST_QUALITY )
			continue;

		if ( strlen(dp->d_name) != STREAMIDZ )
		{
			Report2(english("Bad name for command file: \"%s\""), dp->d_name);
			continue;
		}

		clp = &CmdLists[Policy[dp->d_name[0] - STATE_QUALITY]];

		Trace3(3, "add \"%s\" to chan %d", dp->d_name, clp-CmdLists);

		if ( (mqp = MQfreelist) != (MQl_p)0 )
			MQfreelist = mqp->mql_next;
		else
			mqp = Talloc(MQel);

		mqp->mql_next = (MQl_p)0;
		mqp->mql_use = m_wait;
		(void)strcpy(mqp->mql_name, dp->d_name);
		if ( DecodeNum(dp->d_name+FLAGS_POSN, FLAGS_LENGTH) & ORDER_MSG_FLAG )
			mqp->mql_order = true;
		else
			mqp->mql_order = false;

		*clp->cl_last = mqp;
		clp->cl_last = &mqp->mql_next;
		clp->cl_count++;
	}
}



/*
**	Allocate and sort vectors for new lists.
*/

static void
sortlists()
{
	register CLh_p	clp;
	register MQl_p	mqp;
	register MQl_p *clvp;
	register int	i;

	for ( clp = CmdLists, i = NSTREAMS ; --i >= 0 ; clp++ )
	{
		if ( clp->cl_count == 0 )
			continue;		/* Empty List */

		if ( clp->cl_count <= clp->cl_size )
			clvp = clp->cl_vec;
		else
		{
			if ( clp->cl_vec != (MQl_p *)0 )
				free((char *)clp->cl_vec);

			clp->cl_size = clp->cl_count + 8;

			clp->cl_vec = clvp = (MQl_p *)Malloc(clp->cl_size * sizeof(MQl_p));
		}

		for ( mqp = clp->cl_first ; mqp != (MQl_p)0 ; mqp = mqp->mql_next )
			*clvp++ = mqp;

		Trace3(2, "new vector for channel %d, %d elements", clp-CmdLists, clp->cl_count);

		if ( clp->cl_count == 1 )
			continue;

		qsort((char *)clp->cl_vec, clp->cl_count, sizeof(MQl_p), slcomp);
	}
}



/*
**	Take note of active messages.
*/

static void
checkactive()
{
	register Str_p	strp;
	register MQl_p *mqpp;
	register CLh_p	clp;
	register int	i;

	for ( i = 0 ; i < NSTREAMS ; i++ )
	{
		MQactive[i] = (MQl_p)0;

		strp = &outStreams[i];

		if ( strp->str_id[0] == '\0' )
			continue;

		if ( strp->str_cmdid[0] < STATE_QUALITY || strp->str_cmdid[0] > LOWEST_QUALITY )
			Fatal2(english("checkactive bad msgid \"%s\""), strp->str_cmdid);

		clp = &CmdLists[Policy[strp->str_cmdid[0] - STATE_QUALITY]];

		if
		(
			(mqpp = (MQl_p *)bsearch
					 (
						strp->str_cmdid,
						(char *)clp->cl_vec,
						clp->cl_count,
						sizeof(MQl_p),
						cacomp
					 )
			) != (MQl_p *)0
		)
		{
			Trace3(2, "\"%s\" active on channel %d", (*mqpp)->mql_name, i);
			(*mqpp)->mql_use = m_active;
			MQactive[i] = *mqpp;
		}
		else
		if ( !(strp->str_flags & STR_WARNED) )
		{
			Report3(english("Channel %d active command file \"%s\" removed"), i, strp->str_cmdid);
			strp->str_flags |= STR_WARNED;
		}
	}
}



/*
**	Start a commandsfile on its way.
*/

static void
setactive(chan, mqp)
	register int	chan;
	register MQl_p	mqp;
{
	register Str_p	strp = &outStreams[chan];

	Trace2(2, "setactive \"%s\"", mqp->mql_name);

	(void)strcpy(strp->str_id, mqp->mql_name+1);
	(void)strcpy(strp->str_cmdid, mqp->mql_name);
	(void)strcpy(strp->str_cmdfp, mqp->mql_name);
	strp->str_flags &= ~STR_WARNED;

	mqp->mql_use = m_active;
	MQactive[chan] = mqp;
}



/*
**	Compare two command file names for bsearch.
*/

int
#if	ANSI_C
cacomp(const void *key, const void *mqpp)
#else	/* ANSI_C */
cacomp(key, mqpp)
	char *		key;
	char *		mqpp;
#endif	/* ANSI_C */
{
	return strcmp((char *)key, (*(MQl_p *)mqpp)->mql_name);
}



/*
**	Compare two command file names for qsort.
*/

int
#if	ANSI_C
slcomp(const void *mqpp1, const void *mqpp2)
#else	/* ANSI_C */
slcomp(mqpp1, mqpp2)
	char *		mqpp1;
	char *		mqpp2;
#endif	/* ANSI_C */
{
	return strcmp((*(MQl_p *)mqpp1)->mql_name, (*(MQl_p *)mqpp2)->mql_name);
}
