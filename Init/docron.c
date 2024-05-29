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


#define	STDIO
#define	TIME

#include	"global.h"
#include	"debug.h"
#include	"Netinit.h"

#include	<ctype.h>


#if	DEBUG
#define		DumpCron(A,B)	if(Traceflag>=2)dumpcron(A,B)
static void	dumpcron _FA_((char *, Timespec *));
#else	/* DEBUG */
#define		DumpCron(A,B)
#endif	/* DEBUG */

static bool	chktime _FA_((Timespec *, int));
static void	getcron _FA_((Timespec *, char **));
static int	getnum _FA_((char **));
static int	getrange _FA_((int, int, char **, int *));



void
mkcron(p, s)
	Proc *		p;
	char *		s;
{
	Cron *		c;
	int		i;
	int		count;
	Timespec *	tp;

	Trace3(1, "mkcron(%s, %s)", p->procId, s);

	p->crontime = c = Talloc(Cron);
	p->delay = CRONDELAY;
	c->string = s;

	getcron(&(c->min), &s);		DumpCron("min", &(c->min));
	getcron(&(c->hour), &s);	DumpCron("hour", &(c->hour));
	getcron(&(c->mday), &s);	DumpCron("mday", &(c->mday));
	getcron(&(c->month), &s);	DumpCron("month", &(c->month));

	tp = &(c->wday);
	getcron(tp, &s);

	switch ( (int)tp->type )	/* Fix weekday to match "localtime(3)" */
	{
	case (int)exact_t:
		if ( tp->val[0] == 7 )
			tp->val[0] = 0;
		break;

	case (int)list_t:
		for ( i = 1 ; i <= tp->val[0] ; i++ )
			if ( tp->val[i] == 7 )
				tp->val[i] = 0;
		break;

	case (int)range_t:
		if ( tp->val[1] == 7 )
		{
			tp->val[1] = 6;
			if ( tp->val[0] == 0 )
				break;
			if ( tp->val[0] == 1 )
			{
				tp->val[0] = 0;
				break;
			}
			/** Convert to list **/
			tp->type = list_t;
			tp->val[1] = 0;
			for ( count = 1, i = tp->val[0] ; i <= 6 ; i++ )
				tp->val[++count] = i;
			tp->val[0] = count;
		}
	}

	DumpCron("wday", tp);
}



static void
getcron(ct, vp)
	Timespec *	ct;
	char **		vp;
{
	register int	n;
	register int	i;
	char *		s = *vp;
	char *		e;
	static char	space[] = " \t";

	Trace2(3, "getcron(%s)", s);

	s += strspn(s, space);

	if ( *s == '*' )
	{
		ct->type = any_t;
		*vp = ++s;
		return;
	}

	if ( (e = strpbrk(s, space)) != NULLSTR )
		*e = '\0';

	n = getnum(&s);

	if ( *s == '\0' )
	{
		ct->type = exact_t;
		ct->val[0] = n;
	}
	else
	if ( strchr(s, ',') != NULLSTR )
	{
		ct->type = list_t;

		for ( i = 1 ;; )
		{
			i = getrange(i, n, &s, ct->val);
			if ( *s != ',' )
				break;
			s++;
			n = getnum(&s);
		}

		ct->val[0] = i-1;
	}
	else
	if ( *s == '-' )
	{
		ct->type = range_t;
		ct->val[0] = n;
		s++;
		ct->val[1] = getnum(&s);
	}
	else
	{
		ct->type = err_t;
		warning(english("unrecognised cron specification"));
	}

	if ( e != NULLSTR )
		*e = ' ';

	*vp = s;
	return;
}



static int
getnum(s)
	char **	s;
{
	int	v = 0;
	int	ok = 0;

	Trace2(4, "getnum(%s)", *s);

	while ( isdigit(**s) )
	{
		v = **s - '0' + (v * 10);
		(*s)++;
		ok++;
	}

	if ( !ok )
	{
		warning(english("bad number in crontime"));
		return -1;
	}

	return v;
}



static int
getrange(i, n, s, val)
	register int	i;
	register int	n;
	register char **s;
	register int *	val;
{
	register int	r;

	Trace4(3, "getrange(%d, %d, %s)", i, n, *s);

	if ( i >= MAXLIST )
	{
err:		warning(english("too many cron list items"));
		return MAXLIST - 1;
	}

	val[i++] = n;

	switch ( **s )
	{
	case '\0':
		break;

	case ',':
		break;

	case '-':
		(*s)++;
		r = getnum(s);
		while ( ++n <= r )
		{
			if ( i >= MAXLIST )
				goto err;
			val[i++] = n;
		}
		break;

	default:
		warning(english("bad cron list"));
	}

	return i;
}



#if	DEBUG
static void
dumpcron(s, ct)
	char *			s;
	register Timespec *	ct;
{
	register int		i;

	switch ( (int)ct->type )
	{
	case (int)any_t:	Trace(2, "%s: any", s);					break;
	case (int)exact_t:	Trace(2, "%s: %d", s, ct->val[0]);			break;
	case (int)range_t:	Trace(2, "%s: range %d to %d", s, ct->val[0], ct->val[1]);	break;
	case (int)list_t:	for ( i = 1 ; i <= ct->val[0] ; i++ )
					Trace(2, "%s: list item %d = %d", s, i, ct->val[i]);
				break;
	default:		Trace(2, "%s: UNKNOWN TYPE %d", s, (int)ct->type);
	}
}
#endif	/* DEBUG */



int
cron(lt, pp)
	struct tm *	lt;
	Proc *		pp;
{
	register Cron *	cp;

	if ( (cp = pp->crontime) == (Cron *)0 )
		return 0;

	Trace3(
		2,
		"cron(%s) \"%s\"",
		pp->procId,
		cp->string
	);

	if
	(
		chktime(&(cp->min), lt->tm_min)
		||
		chktime(&(cp->hour), lt->tm_hour)
		||
		chktime(&(cp->mday), lt->tm_mday)
		||
		chktime(&(cp->month), lt->tm_mon)
		||
		chktime(&(cp->wday), lt->tm_wday)
	)
		return 0;

	Trace2(1, "cron(%s) RUN", pp->procId);

	return 1;
}



static bool
chktime(ts, t)
	register Timespec *	ts;
	register int		t;
{
	register int		i;
	DODEBUG(static char	fmt[]	= "chktime(%s, %d) == %d MATCH");
	DODEBUG(static char	fmtn[]	= "chktime(%s, %d) != %d NOMATCH");

	switch ( (int)ts->type )
	{
	case (int)any_t:
		Trace4(4, fmt, "any", t, -1);
		return false;

	case (int)exact_t:
		if ( ts->val[0] != t )
		{
			Trace4(4, fmtn, "exact", t, ts->val[0]);
			return true;
		}
		Trace4(3, fmt, "exact", t, ts->val[0]);
		return false;

	case (int)list_t:
		for ( i = 1 ; i <= ts->val[0] ; i++ )
		{
			if ( ts->val[i] == t )
			{
				Trace4(3, fmt, "list", t, ts->val[i]);
				return false;
			}
			Trace4(4, fmtn, "list", t, ts->val[i]);
		}
		return true;

	case (int)range_t:
		if ( ts->val[0] > t || ts->val[1] < t )
		{
			Trace4(4, fmtn, "range", t, ts->val[0]);
			Trace4(4, fmtn, "range", t, ts->val[1]);
			return true;
		}
		Trace4(3, fmt, "range", t, ts->val[0]);
		Trace4(3, fmt, "range", t, ts->val[1]);
		return false;
	}

	Trace4(1, fmtn, "** UNKNOWN type **", t, (int)ts->type);
	return true;
}
