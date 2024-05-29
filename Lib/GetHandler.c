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
#include	"handlers.h"
#include	"spool.h"
#include	"sub_proto.h"


/*
**	The following handlers are mandatory,
**	and override contents of "handlers" file.
*/

static Handler	DfltHandlers[] =
{
	{STATEHANDLER,	english("State"),	STATE_PROTO, '1', '0', '1'},
	{MAILHANDLER,	english("Mail"),	FTP},
	{FTPHANDLER,	english("Files"),	FTP},
};

#define	DFLTCOUNT	((sizeof DfltHandlers) / sizeof(Handler))

static bool	GotHandlers;
size_t		HandlCount;
static char *	HandlData;
Handler *	Handlers;
static char *	HandlFile;
static unsigned	HandlSize;
static Time_t	HandlTime;


int		GDHcompare _FA_((const void *, const void *));
int		GHcompare _FA_((const void *, const void *));
static void	read_handlers _FA_((void));



/*
**	Check `handlers' file changed, return true if so.
*/

bool
CheckHandlers()
{
	struct stat	statb;

	if ( !GotHandlers )
		return true;

	if
	(
		stat(HandlFile, &statb) == SYSERROR
		||
		HandlTime < statb.st_mtime
		||
		HandlSize != statb.st_size
	)
	{
		GotHandlers = false;
		return true;
	}

	return false;
}



/*
**	Read in and sort handlers description file if necessary,
**	then attempt to match `descrip' against one of the known handlers.
**
**	Return a Handler structure pointer if successful, else null.
*/

Handler *
GetDHandler(descrip)
	char *	descrip;
{
	Handler	key;

	Trace2(1, "GetDHandler(%s)", descrip);

	if ( !GotHandlers )
		read_handlers();

	key.descrip = descrip;

	return (Handler *)lfind((char *)&key, (char *)Handlers, &HandlCount, sizeof(Handler), GDHcompare);
}



int
#ifdef	ANSI_C
GDHcompare(const void *hp1, const void *hp2)
#else	/* ANSI_C */
GDHcompare(hp1, hp2)
	char *	hp1;
	char *	hp2;
#endif	/* ANSI_C */
{
	Trace3(4, "GDHcompare(%s, %s)", ((Handler *)hp1)->descrip, ((Handler *)hp2)->descrip);

	return strccmp(((Handler *)hp1)->descrip, ((Handler *)hp2)->descrip);
}



/*
**	Read in and sort handlers description file if necessary,
**	then attempt to match name against one of the known handlers.
**
**	Return a Handler structure pointer if successful, else null.
*/

Handler *
GetHandler(name)
	char *	name;
{
	Handler	key;

	Trace2(1, "GetHandler(%s)", name);

	if ( !GotHandlers )
		read_handlers();

	key.handler = name;

	return (Handler *)bsearch((char *)&key, (char *)Handlers, HandlCount, sizeof(Handler), GHcompare);
}



int
#ifdef	ANSI_C
GHcompare(const void *hp1, const void *hp2)
#else	/* ANSI_C */
GHcompare(hp1, hp2)
	char *	hp1;
	char *	hp2;
#endif	/* ANSI_C */
{
	Trace3(4, "GHcompare(%s, %s)", ((Handler *)hp1)->handler, ((Handler *)hp2)->handler);

	return strcmp(((Handler *)hp1)->handler, ((Handler *)hp2)->handler);
}



/*
**	Read in and sort handlers description file,
**	include any of the known handlers.
*/

static void
read_handlers()
{
	register char *		cp;
	register char *		np;
	register Handler *	hp;
	register int		count;
	register int		i;
	register Handler *	ohp;
	static char *		space	= " \t";

	Trace1(1, "read_handlers");

	GotHandlers = true;

	if ( DFLTCOUNT > 1 )		/* Sort the defaults */
		qsort((char *)DfltHandlers, DFLTCOUNT, sizeof(Handler), GHcompare);

	if ( HandlFile == NULLSTR )
		HandlFile = concat(SPOOLDIR, LIBDIR, HANDLERS, NULLSTR);

	if ( HandlData != NULLSTR )
		free(HandlData);

	if ( (HandlData = cp = ReadFile(HandlFile)) == NULLSTR )
	{
		Report3(CouldNot, ReadStr, HandlFile);
		Handlers = DfltHandlers;
		HandlCount = DFLTCOUNT;
		goto out;		/* Use defaults */
	}

	cp[RdFileSize++] = '\n';	/* Ensure terminating '\n' */
	cp[RdFileSize] = '\0';
	count = 0;

	HandlTime = RdFileTime;
	HandlSize = RdFileSize;

	while ( (cp = strchr(cp, '\n')) != NULLSTR )
	{
		count++;
		cp++;
	}

	if ( Handlers != (Handler *)0 && Handlers != DfltHandlers )
		free((char *)Handlers);

	Handlers = hp = (Handler *)Malloc((DFLTCOUNT+count) * sizeof(Handler));

	for ( count = 0, cp = HandlData ; (np = strchr(cp, '\n')) != NULLSTR ; cp = np )
	{
		*np++ = '\0';

		if ( cp[0] == '#' || (hp->handler = strtok(cp, space)) == NULLSTR )
			continue;

		hp->proto_type = UNK_PROTO;
		hp->restricted = DFLT_RESTRICT;
		hp->quality = CHOOSE_QUALITY;
		hp->order = DFLT_ORDER;
		hp->nice = NICEHANDLERS;
		hp->router = false;
		hp->duration = 0;
		hp->list = NULLSTR;
		hp->timeout = 0;

		if ( (hp->descrip = strtok(NULLSTR, space)) == NULLSTR )
			goto lookup;

		if ( (cp = strtok(NULLSTR, space)) == NULLSTR )
			goto lookup;

		hp->proto_type = *cp;

		if ( (cp = strtok(NULLSTR, space)) == NULLSTR )
			goto lookup;

		if ( (hp->restricted = *cp) == '-' )
			hp->restricted = DFLT_RESTRICT;

		if ( (cp = strtok(NULLSTR, space)) == NULLSTR )
			goto lookup;

		if ( (hp->quality = *cp) == '*' || *cp == '-' )
			hp->quality = CHOOSE_QUALITY;

		if ( (cp = strtok(NULLSTR, space)) == NULLSTR )
			goto lookup;

		hp->order = *cp;

		if ( (cp = strtok(NULLSTR, space)) == NULLSTR )
			goto lookup;

		if ( *cp != '*' )
		{
			hp->router = true;

			if ( *cp == '-' )
			{
				if ( cp[1] == '\0' )
					hp->router = false;
				else
					hp->nice = -atoi(++cp);
			}
			else
				hp->nice = atoi(cp);

			if ( (cp = strchr(cp, DURATION_SEP)) != NULLSTR )
				hp->duration = atoi(++cp);
		}

		if ( (cp = strtok(NULLSTR, space)) == NULLSTR )
			goto lookup;

		if ( *cp != '@' )
			hp->list = cp;

		if ( (cp = strtok(NULLSTR, space)) != NULLSTR )
			hp->timeout = atoi(cp);

lookup:
		if
		(
			(ohp = (Handler *)bsearch
					  (
						(char *)hp,
						(char *)DfltHandlers,
						DFLTCOUNT,
						sizeof(Handler),
						GHcompare
					  )
			) != (Handler *)0
		)
		{
			Trace2(3, "old handler %s", hp->handler);
			ohp->descrip = hp->descrip;
			if ( ohp->restricted <= DFLT_RESTRICT )
				ohp->restricted = hp->restricted;	/* Use new restrict if higher */
			if ( ohp->quality == CHOOSE_QUALITY )
				ohp->quality = hp->quality;	/* Use new quality if higher */
			if ( ohp->order <= DFLT_ORDER )
				ohp->order = hp->order;		/* Use new order if higher */
			ohp->nice = hp->nice;
			ohp->router = hp->router;
			ohp->duration = hp->duration;
			ohp->list = hp->list;
			ohp->timeout = hp->timeout;
		}
		else
		{
			Trace2(3, "new handler %s", hp->handler);
			hp++;
			count++;
		}
	}

	for ( i = DFLTCOUNT ; --i >= 0 ; )
		*hp++ = DfltHandlers[i];	/* Include default handlers */

	if ( (HandlCount = i = DFLTCOUNT + count) > 1 )
		qsort((char *)Handlers, i, sizeof(Handler), GHcompare);
out:
	for ( hp = Handlers, i = 0 ; i < HandlCount ; i++, hp++ )
		hp->index = i;

	Trace2(2, "found %d handlers", HandlCount);
}
