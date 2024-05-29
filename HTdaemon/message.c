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
#include	"driver.h"
#include	"packet.h"
#include	"chnstats.h"
#include	"channel.h"
#include	"dmnstats.h"
#include	"pktstats.h"
#include	"spool.h"
#include	"status.h"

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
	char	mql_name[MSGIDLENGTH+1];
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

static CLhead		CmdLists[NCHANNELS];	/* The sorted lists for each channel */
static MQl_p		MQfreelist;
static MQl_p		MQactive[NCHANNELS];	/* The message active on each channel */

/*
**	Queue quality -> channel policy.
*/

#define	NPOLS	10

int	Policy[NPOLS]	=
{
	0,		/* STATE_QUALITY */
	1, 1, 1,	/* '1', '2', '3' */
	2, 2, 2,	/* '4', '5', '6' */
	3, 3, 3		/* '7', '8', LOWEST_QUALITY */
};

#if	(NPOLS - 1) != (LOWEST_QUALITY - STATE_QUALITY) || NCHANNELS != 4
	*** rewrite needed ***
#endif

/*
**	Directory variables.
*/

static DIR *		CmdDirFd;
static Time_t		CmdDirMtime;
static int		MsgsCount;
static Time_t		NextLook;

/*
**	Miscellaneous.
*/


int			cacomp _FA_((const void *, const void *));
int			slcomp _FA_((const void *, const void *));
static void		buildlists _FA_((void));
static void		checkactive _FA_((void));
static void		freelists _FA_((void));
static bool		setactive _FA_((Chan *, MQl_p));
static void		sortlists _FA_((void));



/*
**	Find lexically smallest file name for channel.
**	In the interests of speed, a sorted list of known command files
**	is maintained for each channel, and the directory is only searched
**	when a list is empty, and the modify time of the directory has changed.
*/

bool
FindMessage(chnp)
	register Chan *	chnp;
{
	register MQl_p *mqpp;
	register MQl_p	mqp;
	register int	i;
	register int	chan;
	bool		stat_done = false;
	struct stat	statb;

	Trace3(2, "FindMessage for channel %d state %d", chnp->ch_number, chnp->ch_state);

	/*
	**	Take last entry out of list, and unlink it.
	*/
again:
	if ( chnp->ch_msgid[0] != '\0' )
	{
		if ( chnp->ch_flags & CH_MSGFILE )
		{
			Close(chnp->ch_msgfd, CLOSE_NOSYNC, chnp->ch_current->filename_part);
			chnp->ch_flags &= ~CH_MSGFILE;
		}

		if ( access(chnp->ch_cmdfilename, 0) != SYSERROR )
		{
			UnlinkParts(chnp, true);

			/*
			**	Unlinking the commands file will change the directory `mtime',
			**	(except on a MIPS!), so fetch it first.
			*/

			if ( !stat_done && CmdDirFd != NULL && stat(CmdsName, &statb) != SYSERROR )
				stat_done = true;

			Unlink(chnp->ch_cmdfilename);
		}
		else
			UnlinkParts(chnp, false);

		if ( (mqp = MQactive[chnp->ch_number]) != (MQl_p)0 )
		{
			mqp->mql_use = m_sent;
			MsgsCount--;
		}

		chnp->ch_msgid[0] = '\0';
	}

	/*
	**	Search lists for new command file.
	*/

	for ( ;; )
	{
		/**	Search higher-priority lists for highest priority message **/

		for ( chan = 0 ; chan <= chnp->ch_number ; chan++ )
		for ( mqpp = CmdLists[chan].cl_vec, i = CmdLists[chan].cl_count ; --i >= 0 ; )
		{
			if ( (mqp = *mqpp++)->mql_use != m_wait )
				continue;

			if ( chan < chnp->ch_number && mqp->mql_order )
				continue;

			if ( mqp->mql_name[0] > Quality )
				continue;

			if ( setactive(chnp, mqp) )
				return true;

			goto again;
		}

		/*
		**	Nothing to do, if directory hasn't changed, give up.
		*/

		MQactive[chnp->ch_number] = (MQl_p)0;

		while ( !stat_done )
		{
			if ( NextLook >= Time )
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
			"stat dir mtime %lu, CmdDirMtime %lu, NextLook %lu, Time %lu",
			(PUlong)statb.st_mtime,
			(PUlong)CmdDirMtime,
			(PUlong)NextLook,
			(PUlong)Time
		);

		if ( (i = MsgsCount/10) > 60 )
			i = 60;		/* Max. delay for long queue */

		/*
		**	The directory may have been written in the same period
		**	that we last looked at it, but just a bit later...
		**	(of course, all this is pointless if NFS is in the way!)
		*/

		if ( NextLook > CmdDirMtime && statb.st_mtime == CmdDirMtime )
		{
			NextLook = Time + i;
			return false;
		}

		NextLook = Time + i;
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

	for ( clp = CmdLists, i = NCHANNELS ; --i >= 0 ; clp++ )
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

	MsgsCount = 0;
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
		{
			register bool	unl = false;

			if ( strcmp(dp->d_name, STOPFILE) == STREQUAL )
			{
				Finish = true;
				unl = true;
			}
			else
			if
			(
				strcmp(dp->d_name, TRACEONFILE) == STREQUAL
				||
				strcmp(dp->d_name, "TRACEON") == STREQUAL
			)
			{
				Traceflag++;
				PktTraceflag++;
				Report2(english("Trace level %d"), Traceflag);
				unl = true;
			}
			else
			if
			(
				strcmp(dp->d_name, TRACEOFFFILE) == STREQUAL
				||
				strcmp(dp->d_name, "TRACEOFF") == STREQUAL
			)
			{
				Traceflag = PktTraceflag = 0;
				Report2(english("Trace level %d"), Traceflag);
				unl = true;
			}

			if ( unl )
			{
				char *	cp = concat(CmdsName, Slash, dp->d_name, NULLSTR);

				Unlink(cp);
				free(cp);
			}

			continue;
		}

		if ( strlen(dp->d_name) != MSGIDLENGTH )
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

	for ( clp = CmdLists, i = NCHANNELS ; --i >= 0 ; clp++ )
	{
		if ( clp->cl_count == 0 )
			continue;		/* Empty List */

		MsgsCount += clp->cl_count;

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
	register Chan *	chnp;
	register MQl_p *mqpp;
	register CLh_p	clp;
	register int	i;

	for ( i = 0 ; i < NCHANNELS ; i++ )
	{
		MQactive[i] = (MQl_p)0;

		chnp = &Channels[i];

		if ( chnp->ch_msgid[0] == '\0' )
			continue;

		if ( chnp->ch_msgid[0] < STATE_QUALITY || chnp->ch_msgid[0] > LOWEST_QUALITY )
			Fatal2(english("checkactive bad msgid \"%s\""), chnp->ch_msgid);

		clp = &CmdLists[Policy[chnp->ch_msgid[0] - STATE_QUALITY]];

		if
		(
			(mqpp = (MQl_p *)bsearch
					 (
						chnp->ch_msgid,
						(char *)clp->cl_vec,
						(unsigned)clp->cl_count,
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
		if ( !(chnp->ch_flags & CH_WARNED) )
		{
			Report3(english("Channel %d active command file \"%s\" removed"), i, chnp->ch_msgid);
			chnp->ch_flags |= CH_WARNED;
			BadMesg(chnp, english("command file removed"));
		}
	}
}



/*
**	Start a commandsfile on its way.
*/

static bool
setactive(chnp, mqp)
	register Chan *	chnp;
	register MQl_p	mqp;
{
	Trace2(2, "setactive \"%s\"", mqp->mql_name);

	(void)strcpy(chnp->ch_msgid, mqp->mql_name);
	(void)strcpy(chnp->ch_msgidp, mqp->mql_name);

	chnp->ch_msgaddr = 0;
	chnp->ch_bufstart = 0;
	chnp->ch_bufend = 0;
	chnp->ch_flags &= ~CH_WARNED;

	if ( !SetupParts(chnp) )
	{
		mqp->mql_use = m_sent;
		return false;
	}

	mqp->mql_use = m_active;
	MQactive[chnp->ch_number] = mqp;

	return true;
}



/*
**	Compare two command file names for bsearch.
*/

int
#ifdef	ANSI_C
cacomp(const void *key, const void *mqpp)
#else	/* ANSI_C */
cacomp(key, mqpp)
	char *		key;
	char *		mqpp;
#endif	/* ANSI_C */
{
	return strcmp(key, (*(MQl_p *)mqpp)->mql_name);
}



/*
**	Compare two command file names for qsort.
*/

int
#ifdef	ANSI_C
slcomp(const void *mqpp1, const void *mqpp2)
#else	/* ANSI_C */
slcomp(mqpp1, mqpp2)
	char *		mqpp1;
	char *		mqpp2;
#endif	/* ANSI_C */
{
	return strcmp((*(MQl_p *)mqpp1)->mql_name, (*(MQl_p *)mqpp2)->mql_name);
}
