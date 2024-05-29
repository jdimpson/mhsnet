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
**	Spooling parameters.
*/

#define	DFLT_BC_TIMEOUT	WEEK		/* Default timeout for all broadcast messages */
#define	MAX_REROUTE_WAIT (60*30)	/* Maximum wait time for message in re-route directory */
#define	MAXTIMEOUT	YEAR		/* Messages timeout after a year */
#define	TRACETIMEOUT	DAY		/* Trace messages timeout after 24 hours */

/*
**	Message service qualifiers. (See UniqueName().)
*/

#define	STATE_QUALITY	'0'		/* STATE messages only -- time ordering only */
#define	HIGHEST_QUALITY	'1'		/* Highest for non-STATE messages */
#define	LOWEST_QUALITY	'9'
#define	FAST_QUALITY	'4'		/* Anything <= goes cheapest */

/*
**	Arguments for UniqueName().
*/

#define	CHOOSE_QUALITY	'\0'		/* UniqueName will choose qualifier based on size */
#define	OPTNAME		false		/* Use messages size to sort within quality group */
#define	NOOPTNAME	true
#define	ORDER_MSG_FLAG	01		/* For ``flags'' component of unique name */

/*
**	Default size ranges for choosing quality.
*/

#define	QUAL_0_SIZE	0		/* Group 0 -- state messages only */

#define	QUAL_1_SIZE	2048		/* Group 1 */
#define	QUAL_2_SIZE	4096
#define	QUAL_3_SIZE	8192

#define	QUAL_4_SIZE	16384		/* Group 2 */
#define	QUAL_5_SIZE	32768
#define	QUAL_6_SIZE	65536

#define	QUAL_7_SIZE	131072		/* Group 3 */
#define	QUAL_8_SIZE	262144
#define	QUAL_9_SIZE	MAX_ULONG	/* (Infinity!) */

/*
**	Directory names.
*/

#define	LINKCMDSNAME	"cmds"		/* Name of command spooling directory in link directory */
#define	LINKMSGSINNAME	"in"		/* Name of received message directory in link directory */
#define	LINKSPOOLNAME	"out"		/* Name of message spooling directory in link directory */

/*
**	File names.
*/

#define	CALLFILE	"call"		/* Name of caller command file */
#define	DMNPROG		"endprog"	/* Name of program to be run on termination */
#define	DROPFILE	"DROP"		/* Remove and start ignoring state messages */
#define	LOCKFILE	"lock"		/* Lock file for a daemon */
#define	LOGFILE		"log"		/* Name of a daemon's log file */
#define	NODROPFILE	"NODROP"	/* Remove and start processing state messages */
#define	PARAMSFILE	"params"	/* Parameter file for a daemon */
#define	REROUTELOCKFILE	"reroute.lock"	/* Lock file for a queue re-router */
#define	STOPFILE	"STOP"		/* Remove and terminate */
#define	TRACEOFFFILE	"NOTRACE"	/* Remove and turn off traces */
#define	TRACEONFILE	"TRACE"		/* Remove and increase trace level */

/*
**	Status file names.
*/

#define	READERSTATUS	"readerstatus"
#define	WRITERSTATUS	"writerstatus"

/*
**	Programs in INSLIB.
*/

#define	VCCALL		"VCcall"
#define	VCDAEMON	"VCdaemon"
#define	CHECKBAD	"checkbad"

/*
**	All chars in name are encoded from the following array.
*/

extern char *	ValidChars;		/* which is 64 elements long */

#define	VC_SHIFT	6		/* log2(sizeof ValidChars) */
#define	VC_MASK		077		/* sizeof ValidChars - 1 */
#define	VC_32_LENGTH	6		/* (32+VC_SHIFT-1)/VC_SHIFT */

/*
**	UniqueName components.
*/

#define	PRI_LENGTH	1		/* Priority is one char in range '0'->'9' */
#define	PRI_POSN	0
#define	TIME_LENGTH	5		/* Number of chars for a time ==> 30 bits */
#define	TIME_POSN	(PRI_POSN+PRI_LENGTH)
#define	PID_LENGTH	3		/* Number of chars for a pid ==> 18 bits */
#define	PID_POSN	(TIME_POSN+TIME_LENGTH)
#define	UID_LENGTH	PID_LENGTH
#define	UID_POSN	PID_POSN
#define	FLAGS_LENGTH	1		/* Number of chars for flags ==> 6 flags */
#define	FLAGS_POSN	(PID_POSN+PID_LENGTH)
#define	COUNT_LENGTH	4		/* Number of chars for count ==> 24 bits */
#define	COUNT_POSN	(FLAGS_POSN+FLAGS_LENGTH)

#if	(COUNT_POSN+COUNT_LENGTH) != UNIQUE_NAME_LENGTH
	*** UniqueName configuration error ***
#endif

/*
**	Routines.
*/

extern bool	GetInStatusFiles _FA_((char *, vFuncp));
extern bool	GetLnkState _FA_((char *, char **, char **, int *, bool));
extern Time_t	GetLnkTime _FA_((char *));
extern bool	Shell _FA_((char *, char *, char *, int));
