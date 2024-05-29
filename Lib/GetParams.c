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


#define	ERRNO
#define	STDIO
#define	STAT_CALL
#define	TIME

#include	"global.h"
#include	"debug.h"
#include	"params.h"



/*
**	Global parameters.
*/

int		Pid;	/* Process ID */
Time_t		Time;	/* Invoke time */
char *		Junk;	/* If nowhere else to go */

ParTb		NetParams[] =
{
	{"BINMAIL",		&BINMAIL,			PSTRING},
	{"BINMAILARGS", 	(char **)&BINMAILARGS,		PVECTOR},
	{"HTTPSERVER",		&Junk,				PSTRING},
	{"LOCALSEND",		(char **)&LOCALSEND,		PINT},
	{"LOCAL_NODES",		&LOCAL_NODES,			PSTRING},
	{"MAIL_RFC822_HDR",	(char **)&MAIL_RFC822_HDR,	PINT},
	{"MAIL_TO",		(char **)&MAIL_TO,		PINT},
	{"MAX_MESG_DATA",	(char **)&MAX_MESG_DATA,	PLONG},
	{"MINSPOOLFSFREE",	(char **)&MINSPOOLFSFREE,	PLONG},
	{"MKDIR",		&MKDIR,				PSTRING},
	{"NCC_ADMIN",		&NCC_ADMIN,			PSTRING},
	{"NCC_MANAGER",		&NCC_MANAGER,			PSTRING},
	{"NETADMIN",		(char **)&NETADMIN,		PINT},
	{"NETGROUPNAME",	&NETGROUPNAME,			PSTRING},
	{"NETUSERNAME",		&NETUSERNAME,			PSTRING},
	{"NICEHANDLERS",	(char **)&NICEHANDLERS,		PINT},
	{"NOADDRCOMPL",		(char **)&NOADDRCOMPL,		PINT},
	{"NODENAMEFILE",	&NODENAMEFILE,			PSPOOL},
	{"PASSWDSORT",		&PASSWDSORT,			PSTRING},
	{"POSTMASTER",		&POSTMASTER,			PSTRING},
	{"POSTMASTERNAME",	&POSTMASTERNAME,		PSTRING},
	{"PRIVSFILE",		&PRIVSFILE,			PSPOOL},
	{"RMDIR",		&RMDIR,				PSTRING},
	{"SHELL",		&SHELL,				PSTRING},
	{"SPOOLDIR",		&SPOOLDIR,			PDIR},
	{"STATE",		&STATE,				PSPOOL},
	{"STOP",		&STOP,				PSTRING},
	{"STTY",		&STTY,				PSTRING},
#	if	SUN3 == 1
	{"SUN3",		(char **)&Sun3,			PINT},
#	endif	/* SUN3 */
	{"TMPDIR",		&TMPDIR,			PDIR},
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
	{"ULIMIT",		(char **)&Ulimit,		PLONG},
};

int		SizeNetParams	= sizeof NetParams;
bool		CheckParams;

static char *	DataPoint;

static ParTb *	lookup _FA_((char *, ParTb *, int));
static char *	get_def _FA_((char **));
static char **	splitvec _FA_((char *));
static char *	stripquote _FA_((char *, int));



/*
**	Compare two names (case insensitive).
*/

int
cmp_id(p1, p2)
	char *	p1;
	char *	p2;
{
	return strccmp(((ParTb *)p1)->id, ((ParTb *)p2)->id);
}



/*
**	Convert ascii number to long.
*/

static long
convert(s)
	register char *	s;
{
	while ( s[0] == ' ' || s[0] == '\t' )
		s++;
	if ( s[0] != '0' )
		return atol(s);
	if ( ((++s)[0] | 040) == 'x' )
		return xtol(++s);
	return otol(s);
}



/*
**	Fix up any strings requiring pre-pended SPOOLDIR.
*/

void
fix_PSPOOL(table, size)
	ParTb *		table;
	register int	size;
{
	register char *	cp;
	register ParTb *tp;

	for ( tp = table ; --size >= 0 ; tp++ )
		if ( tp->type == PSPOOL && (cp = *tp->val) != NULLSTR && cp[0] != '/' )
		{
			*tp->val = concat(SPOOLDIR, cp, NULLSTR);
			Trace4(2, "%s=%s => %s", tp->id, cp, *tp->val);
		}
}



/*
**	Fetch operating parameters from a file.
*/

void
GetParams(afile, table, size)
	char *		afile;
	ParTb *		table;
	register int	size;
{
	register char *	cp;
	register ParTb *tp;
	char *		val;
	char *		data;
	char *		file;
	bool		free_file;

	Trace4(1, "GetParams(%s, 0x%lx, %d)", afile, (PUlong)table, size);

	if ( (size /= sizeof(ParTb)) == 0 || afile == NULLSTR || afile[0] == '\0' )
		return;

	if ( afile[0] == '/' )
	{
again:		file = afile;
		free_file = false;
	}
	else
	{
		file = concat(SPOOLDIR, PARAMSDIR, afile, NULLSTR);
		free_file = true;
	}

	if ( (cp = ReadFile(file)) == NULLSTR )
	{
		if ( CheckParams && errno != ENOENT )
			(void)SysWarn(CouldNot, ReadStr, file);
		if ( free_file )
			free(file);
		if ( afile != NETPARAMS )
		{
			afile = NETPARAMS;
			CheckParams = false;
			goto again;
		}
		fix_PSPOOL(table, size);
		return;
	}

	DataPoint = data = cp;

	while ( (cp = get_def(&val)) != NULLSTR )
	{
		Trace2(3, "process param %s", cp);

		if ( (tp = lookup(cp, table, size)) == (ParTb *)0 )
		{
			if ( CheckParams )
				Warn(english("Unmatched parameter \"%s\" in \"%s\""), cp, file);
			continue;
		}

		Trace3(2, "%s=[%s]", cp, val==NULLSTR?NullStr:val);

		switch ( tp->type )
		{
		case PVECTOR:	*(char ***)tp->val = splitvec(val);	break;
		case PLONG:	*(long *)tp->val = convert(val);	break;
		case PINT:	*(int *)tp->val = (int)convert(val);	break;
		default:	*tp->val = stripquote(val, tp->type);	break;
		}

		tp->set = 1;
	}

	free(data);
	if ( free_file )
		free(file);

	fix_PSPOOL(table, size);
}



/*
**	Return next definition from file.
**
**	name<=>val<NL>
*/

static char *
get_def(vp)
	register char **vp;
{
	register int	c;
	register char *	cp;
	register char *	dp;
	register char *	ep;
	register int	state;
	register bool	skip;

	Trace2(5, "get_def() at {%s}", ExpandString(DataPoint, (DataPoint[0]=='\0')?1:8));

	skip = false;
	*vp = dp = NULLSTR;
	state = 1;

	for ( cp = DataPoint ; (c = *cp++) != '\0' ; )
	{
		if ( skip && c != '\n' )
			continue;

		switch ( c )
		{
		case '\n':	if ( state > 0 )
				{
					skip = false;
					break;
				}
				DataPoint = cp;
				*--cp = '\0';
				goto out;
		case '#':	skip = true;
				continue;
		case '=':	if ( dp == NULLSTR )
					break;
				if ( state == 1 )
				{
					ep = cp - 1;
					while ( (c = ep[-1]) == ' ' || c == '\t' )
						--ep;
					*ep = '\0';
					state--;
					continue;
				}
		default:	if ( dp == NULLSTR )
					dp = cp-1;
				else
				if ( state == 0 )
				{
					*vp = cp-1;
					state--;
				}
				continue;
		}

		*vp = dp = NULLSTR;
		state = 1;
	}

	DataPoint = --cp;

	if ( state > 0 )
		return NULLSTR;
out:
	if ( state == 0 )
		*vp = NULLSTR;

	return dp;
}



/*
**	Initialise operating parameters for a program.
*/

void
InitParams()
{
	ErrorFd = stderr;
	InitTrace();
	(void)umask(027);
	Pid = getpid();
	Time = time((time_t *)0);
	GetParams(NETPARAMS, NetParams, sizeof NetParams);
}



/*
**	Lookup parameter name in table.
*/

static ParTb *
lookup(name, table, nel)
	char *	name;
	ParTb *	table;
	int	nel;
{
	ParTb	key;
	size_t	n;

	Trace2(4, "lookup(%s)", name);

	key.id = name;
	n = nel;

	return (ParTb *)lfind
			(
				(char *)&key,
				(char *)table,
				&n,
				sizeof(ParTb),
				cmp_id
			);
}



/*
**	Skip over quoted strings, return non-null if more.
*/

static char *
skipquote(cp)
	register char *	cp;
{
	if ( cp[0] == '"' )
	{
		do
			if ( (cp = strchr(++cp, '"')) == NULLSTR )
				break;
		while
			( cp[-1] == '\\' );

		if ( *++cp == '\0' )
			cp = NULLSTR;
	}
	else
		cp = strchr(cp, ',');

	Trace2(5, "skipquote => %s", cp==NULLSTR?NullStr:cp);

	return cp;
}



/*
**	Split parameter of form:-
**		param	::= el[,el ...]
**		el	::= qstring|string
**		qstring	::= <">string<">
*/

static char **
splitvec(val)
	char *		val;
{
	register int	n;
	register char *	cp;
	register char *	np;
	register char **vp;
	char **		vec;

	if ( (cp = val) == NULLSTR )
	{
		Trace2(3, "splitvec(%s)", NullStr);
		return (char **)0;
	}

	Trace2(3, "splitvec(%s)", val);

	for ( n = 2 ; (cp = skipquote(cp)) != NULLSTR ; cp++ )
		n++;

	Trace2(4, "splitvec => %d strings", n);

	vec = vp = (char **)Malloc(n * sizeof(char *));

	for ( np = val ; (cp = np) != NULLSTR ; )
	{
		if ( (np = skipquote(cp)) != NULLSTR )
			*np++ = '\0';

		if ( (*vp++ = stripquote(cp, PSTRING)) == NULLSTR )
			--vp;
	}

	*vp = NULLSTR;

	return vec;
}



/*
**	Strip optional leading/trailing spaces and quotes from value.
*/

static char *
stripquote(cp, type)
	register char *	cp;
	int		type;
{
	register int	n;
	register int	c;
	register char *	np;
		
	Trace3(4, "stripquote(%s, %s)", cp==NULLSTR?NullStr:cp, type==PDIR?"DIR":"STRING");

	if ( cp == NULLSTR )
		goto null;

	while ( (c = cp[0]) == ' ' || c == '\t' )
		cp++;

	if ( cp[0] == '"' )
	{
		n = strlen(cp+1);

		while ( n > 0 )
			if ( (c = cp[n]) == ' ' || c == '\t' )
				cp[n--] = '\0';
			else
				break;

		if ( n > 0 && cp[n] == '"' )
		{
			cp[n--] = '\0';
			cp++;
		}
		else
			n++;
	}
	else
	{
		if ( cp[0] == '\\' && cp[1] == '"' )
			cp++;

		n = strlen(cp);

		while ( n > 0 )
			if ( (c = cp[n-1]) == ' ' || c == '\t' )
				cp[--n] = '\0';
			else
				break;
	}

	if ( n == 0 )
	{	
null:		Trace1(3, "stripquote => NULLSTR");
		return NULLSTR;
	}

	if ( type == PDIR && cp[--n] != '/' )
		cp = concat(cp, Slash, NULLSTR);
	else
		cp = newstr(cp);

	for ( np = cp ; (np = strchr(np, '"')) != NULLSTR ; )
		if ( np[-1] == '\\' )
			(void)strcpy(np-1, np);
		else
			++np;
	
	Trace2(3, "stripquote => <%s>", cp);

	return cp;
}
