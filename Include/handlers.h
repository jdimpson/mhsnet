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


#define	HANDLERS	"handlers"	/* File containing descriptions */

/*
**	Order / restriction.
*/

#define	NEVER_ORDER	'0'
#define	ALWAYS_ORDER	'1'
#define	UNSPEC_ORDER	'-'
#define	DFLT_ORDER	UNSPEC_ORDER

#define	USE_RESTRICT	'1'
#define	UN_RESTRICT	'0'
#define	DFLT_RESTRICT	UN_RESTRICT

#define	DURATION_SEP	':'

/*
**	Fields from handlers description file.
*/

typedef struct
{
	char *	handler;		/* Name of handler */
	char *	descrip;		/* Description of contents */
	char	proto_type;		/* Protocol type ('-' means none) */
	char	restricted;		/* Not allowed for mortals (if not '-') */
	char	quality;		/* Minimum quality ('0'-'9') ('*' or '-' means none) */
	char	order;			/* Order flag ('0' never, '1' always, '-' unspecified) */
	int	nice;			/* `nice(2)' value for handler ('*' means none) */
	char *	list;			/* List, or filename ('@' means none) */
	int	timeout;		/* Timeout for handler (default set by router) */
	bool	router;			/* True if sub-router required */
	int	duration;		/* Non-zero if limited duration sub-router */
	int	index;			/* Index in sorted array */
}
			Handler;

/*
**	Well known handlers to be found in LIBDIR (needed for gateways.)
*/

#define	FSVRHANDLER	"fileserver"	/* File requests */
#define	FTPHANDLER	"filer"		/* Files */
#define	MAILHANDLER	"mailer"	/* Mail */
#define	NEWSHANDLER	"reporter"	/* News */
#define	PRINTHANDLER	"printer"	/* Print jobs */
#define	STATEHANDLER	"stater"	/* State changes */
#define	WHOISHANDLER	"peter"		/* Whois queries */

/*
**	Other message handlers to be found in LIBDIR.
*/

#define	BADHANDLER	"badhandler"	/* Handler for bad messages */
#define	LOOPHANDLER	"loophandler"	/* Handler for looping messages */
#define	NEWSTATEHANDLER	"changestate"	/* Handler for link state changes */
#define	REQUESTER	"request"	/* State message requester */
#define	REROUTER	"reroute"	/* Message re-router */
#define	ROUTER		"router"	/* Message router */

/*
**	Routines.
*/

extern size_t		HandlCount;			/* Number of known handlers */
extern Handler *	Handlers;			/* Array of handlers */

extern bool		CheckHandlers _FA_((void));	/* Check for change in `handlers' file */
extern Handler *	GetDHandler _FA_((char *));	/* Find handler details by description */
extern Handler *	GetHandler _FA_((char *));	/* Find handler details by name */
