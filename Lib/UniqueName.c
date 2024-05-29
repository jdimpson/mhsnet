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


#define	STAT_CALL

#include	"global.h"
#include	"debug.h"
#include	"spool.h"


/*
**	Default service qualities based on size.
*/

typedef struct
{
	Ulong	q_size;
	char	q_id;
}
		Qual;

static Qual	Qranges[] =
{
	{QUAL_0_SIZE,	'0'},
	{QUAL_1_SIZE,	'1'},
	{QUAL_2_SIZE,	'2'},
	{QUAL_3_SIZE,	'3'},
	{QUAL_4_SIZE,	'4'},
	{QUAL_5_SIZE,	'5'},
	{QUAL_6_SIZE,	'6'},
	{QUAL_7_SIZE,	'7'},
	{QUAL_8_SIZE,	'8'},
	{QUAL_9_SIZE,	'9'}
};

#define	NQRANGES	((sizeof Qranges)/sizeof (Qual))

static Ulong	Count;

extern int	Pid;
extern Time_t	Time;



/*
**	Make a unique path name using quality, size, service time, and process id.
**
**	"name" must be at least UNIQUE_NAME_LENGTH characters long.
**
**	Assumes that "Pid" has <= 18 bits,
**	and that "time" has <= 30 bits of useful info.
*/

char *
#ifdef	ANSI_C
UniqueName(char *name, int quality, bool noopt, Ulong size, Time_t time)
#else	/* ANSI_C */
UniqueName(name, quality, noopt, size, time)
	char *		name;
	int		quality;	/* Can be CHOOSE_QUALITY */
	bool		noopt;		/* No size optimisation */
	register Ulong	size;		/* Set to 0 for valid state messages */
	Time_t		time;
#endif	/* ANSI_C */
{
	register Qual *	qp;
	register char *	p;
	register int	i;
	struct stat	statb;

	/*
	**	Initialise Count.
	*/

	if ( Count == 0 )
		Count = Time;

	/*
	**	Find last element of path name
	*/

	if ( (p = strrchr(name, '/')) == NULLSTR )
		p = name;
	else
		p++;

	DODEBUG(
		if ( (int)strlen(p) < UNIQUE_NAME_LENGTH )
			Fatal("UniqueName last component too short: \"%s\"", name);
	);

	/*
	**	Set first char to reflect message quality.
	*/

	if
	(
		(*p = quality) < HIGHEST_QUALITY	/* Catches STATE_QUALITY & CHOOSE_QUALITY */
		||
		quality > LOWEST_QUALITY
	)
	{
		for ( qp = Qranges, i = NQRANGES-1 ; --i >= 0 ; qp++ )
			if ( size <= qp->q_size )
				break;

		*p = qp->q_id;

		if ( noopt )
			size = qp->q_size;
	}
	else
	if ( noopt )
	{
		for ( qp = Qranges, i = NQRANGES-1 ; --i >= 0 ; qp++ )
			if ( quality == qp->q_id )
				break;

		size = qp->q_size;
	}

	/*
	**	Next 5 chars reflect "time" + ~ 1 minute per 4Kb.
	*/

	(void)EncodeNum(p+TIME_POSN, (Ulong)time + (size >> 6), TIME_LENGTH);

	/*
	**	Next 3 chars reflect "Pid".
	*/

	(void)EncodeNum(p+PID_POSN, (Ulong)Pid, PID_LENGTH);

	/*
	**	Next char contains flags.
	*/

	if ( noopt )
		i = ORDER_MSG_FLAG;
	else
		i = 0;

	(void)EncodeNum(p+FLAGS_POSN, (Ulong)i, FLAGS_LENGTH);

	/*
	**	Last 4 chars increment every call.
	*/

	for ( i = 100 ;; )
	{
		static char	UCreateStr[] = "create UniqueName";

		(void)EncodeNum(p+COUNT_POSN, ++Count, COUNT_LENGTH);

		/*
		**	Filename generated shouldn't exist,
		**	but if it does, try new Count.
		*/

		if ( stat(name, &statb) == SYSERROR )
			break;

		if ( --i == 0 )
			Fatal(CouldNot, UCreateStr, name);
		else
			Trace3(1, CouldNot, UCreateStr, name);
	}

	return name;
}



/*
**	Set quality based on size.
*/

char
#ifdef	ANSI_C
SetQuality(int askq, Ulong size)
#else	/* ANSI_C */
SetQuality(askq, size)
	register int	askq;
	register Ulong	size;
#endif	/* ANSI_C */
{
	register Qual *	qp;
	register int	i;

	if
	(
		askq < HIGHEST_QUALITY	/* Catches STATE_QUALITY & CHOOSE_QUALITY */
		||
		askq > LOWEST_QUALITY
	)
	{
		for ( qp = Qranges, i = NQRANGES-1 ; --i >= 0 ; qp++ )
			if ( size <= qp->q_size )
				break;

		return qp->q_id;
	}

	return askq;
}
