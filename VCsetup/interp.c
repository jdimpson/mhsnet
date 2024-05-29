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
#include	"sysexits.h"

#include	"call.h"
#include	"grammar.h"
#include	"setup.h"

#include	<ctype.h>


static Device *	devfun;

static bool	check_num _FA_((char *, Symbol *, long *));
static char *	do_device _FA_((DevOp, Vlist *));
static void	do_expect _FA_((Pat *, Op **, char *));
static void	do_match _FA_((Symbol *, Pat *, Op **));
static char *	do_read _FA_((char *));
static int	do_shift _FA_((Vlist *));
static char *	join _FA_((Vlist *));

static int	do_call _FA_((Op *));
static int	do_retry _FA_((Symbol *));
static int	do_sleep _FA_((Symbol *));


/*
**	Action for each command.
*/

int
interp(firstop, name)
	Op *		firstop;
	char *		name;
{
	register Op *	o;
	register char *	s;
	register int	status = OK;
	Op *		op;
	long		num;

	if ( (s = strrchr(name, '/')) != NULLSTR )
		name = ++s;

	NameProg("%s %s %s", Name, SLinkName, name);

	for ( o = firstop ; o != (Op *)0 ; )
	{
		int	om;

		if ( o->lab != (Symbol *)0 && Monitorflag >= 4 )
			MesgN(o->lab->name, NULLSTR);

		Trace2(2, "\"%s\"", command_name(o->code));

		switch ( o->code )
		{
		case CALL:	status = do_call(o);
				NameProg("%s %s %s", Name, SLinkName, name);
				break;
		case CLOSE:	RESULTsym->val.str = do_device(t_close, o->op1.vlist);
				break;
		case CRC:	o->op1.sym->val.str = StrCRC16(s = join(o->op2.vlist));
				free(s);
				break;
		case DAEMON:	status = do_daemon(o->op1.sym->val.str);
				break;
		case DEVICE:	RESULTsym->val.str = do_device(t_device, o->op1.vlist);
				break;
		case EXECD:	return do_execd((o->op1.sym==(Symbol *)0) ? NULLSTR : o->op1.sym->val.str);
		case EXPECT:	op = o;
				do_expect(o->op1.pat, &op, DELIMsym->val.str);
				o = op;
				RESULTsym->val.str = Reason;
				continue;
		case FAIL:	MesgN(english("FAIL "), "%s", s = join(o->op1.vlist));
				Reason = s;
				return EX_UNAVAILABLE;
		case FORKC:	status = do_forkc(s = join(o->op1.vlist));
				free(s);
				break;
		case FORKD:	status = do_forkd((o->op1.sym==(Symbol *)0) ? NULLSTR : o->op1.sym->val.str);
				break;
		case MATCH:	op = o;
				do_match(o->op1.sym, o->op2.pat, &op);
				o = op;
				continue;
		case MODE:	status = do_mode(s = join(o->op1.vlist));
				free(s);
				break;
		case MONITOR:	om = Monitorflag;
				if ( o->op1.sym == (Symbol *)0 )
					Monitorflag = om ? 0 : 1;
				else
				{
					if ( !check_num("monitor", o->op1.sym, &num) )
						return EX_DATAERR;
					Monitorflag = num;
					if ( (Monitorflag - 9) > Traceflag )
						Traceflag = Monitorflag - 9;
				}
				if ( om > 2 || Monitorflag > 2 )
					MesgN("debug", "level %d", Monitorflag);
				break;
		case NEXT:	o = o->op1.sym->val.op;
				continue;
		case OPEN:	RESULTsym->val.str = do_device(t_open, o->op1.vlist);
				break;
		case READ:	o->op1.sym->val.str = do_read(DELIMsym->val.str);
				o->op1.sym->vtype = STRING;
				RESULTsym->val.str = Reason;
				break;
		case READCHAR:	o->op1.sym->val.str = concat(o->op1.sym->val.str, s = do_read(NULLSTR), NULLSTR);
				o->op1.sym->vtype = STRING;
				free(s);
				RESULTsym->val.str = Reason;
				break;
		case RETURN:	o = (Op *)0;
				continue;
		case RETRY:	status = do_retry(o->op1.sym);
				break;
		case SET:	if ( o->op1.sym->vtype == STRING )
				{
					o->op1.sym->val.str = join(o->op2.vlist);
					if ( Monitorflag >= 3 )
						MesgN("set", "%s = %s", o->op1.sym->name, o->op1.sym->val.str);
				}
				else
				{
					o->op1.sym->val.num = o->op2.sym->val.num;
					o->op1.sym->vtype = NUMBER;
					if ( Monitorflag >= 3 )
						MesgN("set", "%s = %d", o->op1.sym->name, o->op1.sym->val.num);
				}
				break;
		case SHELL:	RESULTsym->val.str = do_shell(s = join(o->op1.vlist));
				free(s);
				break;
		case SHIFT:	status = do_shift(o->op1.vlist);
				break;
		case SLEEP:	status = do_sleep(o->op1.sym);
				break;
		case SLOWWRITE:	status = do_slowwrite(s = join(o->op1.vlist));
				free(s);
				break;
		case TEST:	if ( o->op1.sym->vtype != NUMBER )
				{
					if ( !check_num("test", o->op1.sym, &num) )
						return EX_DATAERR;
					if ( num > 0 )
						num--;
					(void)sprintf(o->op1.sym->val.str, "%ld", num);
				}
				else
				if ( (num = o->op1.sym->val.num) > 0 )
					o->op1.sym->val.num = --num;
				if ( Monitorflag >= 3 )
					MesgN("test", "%s [%ld <= 0]", o->op1.sym->name, num);
				if ( num <= 0 )
				{
					o = o->op2.sym->val.op;
					continue;
				}
				break;
		case TIME_OUT:	if ( !check_num("timeout", o->op1.sym, &num) )
					return EX_DATAERR;
#				if	MINSLEEP > 1
				if ( (TimeOut = num) < MINSLEEP && TimeOut != 0 )
					TimeOut = MINSLEEP;
#				else	/* MINSLEEP > 1 */
				TimeOut = num;
#				endif	/* MINSLEEP > 1 */
				break;
		case TRACE:	MesgN("trace", "%s", s = join(o->op1.vlist));
				free(s);
				break;
		case WRITE:	status = do_write(s = join(o->op1.vlist));
				free(s);
				break;
		default:	Error(english("unknown opcode %d\n"), o->code);
				return EX_DATAERR;
		}

		if ( status != OK )
			return status;

		o = o->next;
	}

	return OK;
}



static bool
check_num(name, sym, nump)
	char *	name;
	Symbol *sym;
	long *	nump;
{
	int	c;
	long	n;
	char *	cp;

	static char	numerr[] = english("need number for \"%s\" [have %s]");

	if ( sym->vtype == NUMBER )
	{
		*nump = sym->val.num;
		return true;
	}

	if ( sym->vtype != STRING )
	{
		Error(numerr, name, UNDEFINED);
		return false;
	}

	cp = sym->val.str;
	if ( cp[0] == '-' )
		cp++;
	
	for ( n = 0 ; (c = *cp++) != '\0' ; )
	{
		if ( !isdigit(c) )
		{
			Error(numerr, name, sym->val.str);
			return false;
		}

		n *= 10;
		n += c-'0';
	}

	if ( sym->val.str[0] == '-' )
		n = -n;

	*nump = n;
	return true;
}



int
#if	ANSI_C
devcmp(const void *p1, const void *p2)
#else	/* ANSI_C */
devcmp(p1, p2)
	char *	p1;
	char *	p2;
#endif	/* ANSI_C */
{
	return strcmp(((Device *)p1)->devname, ((Device *)p2)->devname);
}



int
do_call(o)
	Op *	o;
{
	int	val;

	Trace2(1, "do_call(%s)", o->op1.sym->val.str);

	if ( Monitorflag >= 3 )
		MesgN
		(
			english("call >>"),
			o->op1.sym->val.str
		);

	if 
	(
		o->op2.op == (Op *)0
		&&
		(o->op2.op = MakeCall(o->op1.sym->val.str)) == (Op *)0
	)
		return EX_DATAERR;

	val = interp(o->op2.op, o->op1.sym->val.str);

	if ( Monitorflag >= 3 )
		MesgN
		(
			english("call <<"),
			o->op1.sym->val.str
		);

	return val;
}



static char *
do_device(dev_op, v)
	DevOp		dev_op;
	register Vlist *v;
{
	VarArgs		va;
	Device		device;
	static bool	sorted;

	Trace2(1, "do_device(), fd = %d", Fd);

	if ( dev_op == t_open )
	{
		if ( v == (Vlist *)0 )
		{
			Error(english("device name not specified in open"));
			return DEVFAIL;
		}

		if ( Fd != SYSERROR )
		{
			Error(english("device \"%s\" already open"), Fn);
			return DEVFAIL;
		}
	}
	else
	if ( devfun == (Device *)0 || Fd == SYSERROR )
	{
		if ( dev_op == t_close )
			return OK;

		Error(english("device not open"));
		return DEVFAIL;
	}

	/** Find last element **/
	if ( v != (Vlist *)0 )
		for ( ; v->next != (Vlist *)0 ; v = v->next )
			;

	if ( dev_op == t_open )
	{
		device.devname = v->sym->val.str;
		v = v->prev;
	}

	/** Build argument list in reverse **/
	NARGS(&va) = 0;
	for ( ; v != (Vlist *)0 ; v = v->prev )
		NEXTARG(&va) = v->sym->val.str;
	ARG(&va, NARGS(&va)) = NULLSTR;

	if ( Monitorflag >= 1 )
	{
		char *	a0;
		char *	a1;
		char *	a2;
		char *	a3;

		switch ( NARGS(&va) )
		{
		case 0:	a2 = a1 = EmptyString;			break;
		case 1:	a2 = EmptyString; a1 = ARG(&va, 0);	break;
		default:a2 = ARG(&va, 1); a1 = ARG(&va, 0);	break;
		}

		if ( dev_op == t_open )
		{
			a3 = a2;
			a2 = a1;
			a1 = device.devname;
			a0 = "open ";
		}
		else
		{
			a3 = EmptyString;

			if ( dev_op == t_device )
				a0 = "device";
			else
				a0 = "close";
		}

		MesgN(a0, "%s %s %s", a1, a2, a3);
	}

	if ( dev_op == t_open )
	{
		if ( !sorted )
		{
			qsort((char *)DevFuns, NDevs, sizeof(Device), devcmp);
			sorted = true;
		}

#		if	XTI == 1
		/* XTI requires all sorts of messing around. In order to
		** make write()s faster, we use a flag to indicate an XTI
		** connection rather than relying on a string. Unfortunately
		** this means we have to take care to reset the flag in case
		** a cal script opens an XTI connection, then opens a different
		** connection later. Bleah. -- RC, 09/98
		**/
		xti_connect = false;
#		endif	/* XTI == 1 */

		if
		(
			(devfun = (Device *)bsearch
					    (
						(char *)&device,
						(char *)DevFuns,
						(unsigned)NDevs,
						sizeof(Device),
						devcmp
					    )
			)
			==
			(Device *)0
		)
		{
			Error(english("unrecognised device/circuit type \"%s\""), device.devname);
			return DEVFAIL;
		}
	}

	return (*devfun->func)(dev_op, &va);
}



static void
do_expect(p, o, delimchars)
	Pat *	p;
	Op **	o;
	char *	delimchars;
{
	Pat *	q;
	Time_t	t;

	if ( (t = TimeOut) > 0 )
		t += time(&Time);

	if ( Monitorflag >= 3 )
		MesgN
		(
			english("expect"),
			english("[timeout: %lu]"),
			(PUlong)TimeOut
		);

	for ( ;; )
	{
		if ( t > 0 && time(&Time) > t )
		{
			t += TimeOut;
			(void)strcpy(input, TIMEOUT);
		}
		else
			getinput(delimchars);

		for ( q = p ; q != (Pat *)0 ; q = q->next )
			if ( match(input, q) == true )
			{
				*o = q->nextsym->val.op;
				return;
			}
	}
}



static void
do_match(s, p, o)
	Symbol *s;
	Pat *	p;
	Op **	o;
{
	char *	cp;
	char	num[ULONG_SIZE+1];

	if ( s->vtype == STRING )
	{
		if ( (cp = s->val.str) == NULLSTR )
			cp = UNDEFINED;
	}
	else
	if ( s->vtype == NUMBER )
	{
		(void)sprintf(num, "%ld", (long)s->val.num);
		cp = num;
	}
	else
		cp = UNDEFINED;

	if ( Monitorflag >= 3 )
		MesgN
		(
			english("match"),
			"%s [%s]", s->name, cp
		);

	for ( ; p != (Pat *)0 ; p = p->next )
		if ( match(cp, p) == true )
		{
			*o = p->nextsym->val.op;
			return;
		}

	*o = (*o)->next;
}



static char *
do_read(delimchars)
	char *	delimchars;
{
	getinput(delimchars);
	return newstr(input);
}



int
do_retry(s)
	Symbol *	s;
{
	if ( s->vtype != NUMBER )
	{
		Error(english("retry requires a number"));
		return EX_DATAERR;
	}
	if ( s->val.num < 0 )
	{
		Error(english("retries must be >= 0"));
		return EX_DATAERR;
	}
	Retries = s->val.num;

	if ( Monitorflag >= 1 )
		MesgN("retry", "%d", (int)Retries);

	return OK;
}



static int
do_shift(v)
	Vlist *			v;	/* shift new seps string; */
{
	register Vlist *	rv;
	register Symbol *	new	= (Symbol *)0;
	register Symbol *	seps	= (Symbol *)0;
	register Symbol *	string	= (Symbol *)0;
	register char *		cp;
	register char *		ep;
	register int		state;
	static char		emesg[]	= english("too %s parameters for \"shift\"");

	for ( state = 0, rv = v ; rv != (Vlist *)0 ; rv = rv->next )
		switch ( state++ )	/* Reverse order */
		{
		case 2:	new = rv->sym;		break;
		case 1:	seps = rv->sym;		break;
		case 0:	string = rv->sym;	break;
		default:	Error(emesg, english("many"));
				return EX_DATAERR;
		}

	if ( state < 3 )
	{
		Error(emesg, english("few"));
		return EX_DATAERR;
	}

	if ( seps->vtype != STRING || (ep = seps->val.str) == NULLSTR )
	{
		Error(english("second parameter to \"shift\" must be a string of separator characters"));
		return EX_DATAERR;
	}

	if ( string->vtype != STRING || (cp = string->val.str) == NULLSTR )
	{
undef:		if ( Monitorflag >= 3 )
			MesgN("shift", "%s \"%s\" UNDEFINED", string->name, ep);
		new->vtype = UNDEF;
		new->val.str = NULLSTR;
		return OK;
	}

	if ( Monitorflag >= 3 )
		MesgN("shift", "%s \"%s\" \"%s\"", string->name, ep, cp);

	cp += strspn(cp, ep);
	if ( *cp == '\0' )
		goto undef;
	if ( (ep = strpbrk(cp, ep)) != NULLSTR )
		*ep++ = '\0';
	string->val.str = ep;
	new->val.str = cp;
	new->vtype = STRING;

	return OK;
}



int
do_sleep(s)
	Symbol *	s;
{
	unsigned	secs;

	if ( s->vtype != NUMBER )
	{
		Error(english("sleep requires a number"));
		return EX_DATAERR;
	}
	if ( (secs = s->val.num) == 0 )
	{
		Error(english("sleep period is 0"));
		return EX_DATAERR;
	}
	if ( secs < MINSLEEP )
		secs = MINSLEEP;
	(void)alarm((unsigned)0);
	if ( Monitorflag >= 3 )
		MesgN("sleep", "%u", secs);
	(void)sleep(secs);
	return OK;
}



static char *
join(v)
	Vlist *			v;
{
	register Vlist *	rv;
	register char *		r;
	register int		i = 0;
	char *			result;

	if ( (rv = v) == (Vlist *)0 )
	{
		Error(english("no strings specified"));
		finish(EX_DATAERR);
		return NULLSTR;
	}

	for ( ;; )
	{
		if
		(
			rv->sym->vtype == UNDEF
			||
			(
				(rv->sym->vtype == STRING) 
				&&
				(rv->sym->val.str == (char *)0)
			)
		)
		{
			Error(english("undefined variable \"%s\""), v->sym->name);
			finish(EX_DATAERR);
			return NULLSTR;
		}

		if ( rv->sym->vtype == STRING )
			i += strlen(rv->sym->val.str);
		else
			i += ULONG_SIZE+1;

		if ( rv->next == (Vlist *)0 )
			break;	/* `rv' is now end of list */

		rv = rv->next;
	}

	result = r = Malloc(i+1);

	do
	{
		if ( rv->sym->vtype == STRING )
			r = strcpyend(r, rv->sym->val.str);
		else
		{
			(void)sprintf(r, "%ld", (long)rv->sym->val.num);
			r += strlen(r);
		}
	}
		while ( (rv = rv->prev) != (Vlist *)0 );

	return result;
}
