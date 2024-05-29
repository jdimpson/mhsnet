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


#define	PASSWD_USED

#include	"global.h"
#include	"debug.h"

#define	PFLAGS_DATA
#include	"Passwd.h"

#if	AUSAS == 0
#include	<grp.h>

extern struct group *	getgrnam _FA_((const char *));
#endif	/* AUSAS == 0 */


/*
**	Privileges list structure.
*/

typedef struct Upriv	Upriv;
struct Upriv
{
	Upriv *	next;	/* In list */
	char *	user;	/* Particular person, or NULLSTR for all */
	char *	region;	/* Addressing restriction */
	Pflg_t	on;	/* Set these */
	Pflg_t	off;	/* Turn off these */
	int	gid;	/* These are group privileges */
};


static char *	DataPoint;
static Upriv *	PrivsList;
static bool	PrivsRead;

static int	get_gid _FA_((char *));
static void	read_privs _FA_((void));
static void	set_privs _FA_((Upriv *, Passwd *));



/*
**	Pass back all users in privsfile.
*/

void
GetAllPrivUsers(funcp)
	vFuncp		funcp;
{
	register Upriv *lp;

	Trace1(1, "GetAllPrivUsers()");

	for ( lp = PrivsList ; lp != (Upriv *)0 ; lp = lp->next )
	{
		if ( lp->user == NULLSTR )
			continue;

		(*funcp)(lp->user);
	}
}



/*
**	Find privileges for a user.
*/

void
GetPrivs(pp)
	Passwd *	pp;
{
	register Upriv *lp;
	register bool	noglob;

	Trace2(1, "GetPrivs(%s)", pp->P_name);

	if ( !PrivsRead )
	{
		read_privs();
		PrivsRead = true;
	}

	for ( lp = PrivsList, noglob = false ; lp != (Upriv *)0 ; lp = lp->next )
	{
		if ( lp->user == NULLSTR )
		{
			if ( noglob )
				continue;
			set_privs(lp, pp);
			continue;
		}

		if ( lp->gid > 0 && lp->gid == pp->P_gid )
		{
			set_privs(lp, pp);
			noglob = true;
			continue;
		}

		if ( strcmp(lp->user, pp->P_name) == STREQUAL )
		{
			set_privs(lp, pp);
			return;
		}
	}
}



/*
**	Case insensitive compare of two flags.
*/

int
#if	ANSI_C
cmp_flg(const void *f1, const void *f2)
#else	/* ANSI_C */
cmp_flg(f1, f2)
	char *	f1;
	char *	f2;
#endif	/* ANSI_C */
{
	return strccmp(((PFlag *)f1)->pf_name, ((PFlag *)f2)->pf_name);
}



/*
**	Return new string minus escapes.
*/

static char *
escape(cp)
	register char *	cp;
{
	register int	c;
	register char *	xp;
	register bool	esc;
	char *		np;

	Trace2(4, "escape(%s)", cp);

	np = xp = newstr(cp);
	esc = false;

	while ( (c = *cp++) != '\0' )
		if ( esc )
		{
			esc = false;
			*xp++ = c;
		}
		else
		if ( c == '\\' )
			esc = true;
		else
			*xp++ = c;

	*xp = c;
	return np;
}



/*
**	Return `gid' for `name'.
*/

static int
get_gid(name)
	char *		name;
{
#	if	AUSAS == 0
	struct group *	gp;

	if ( (gp = getgrnam(name)) == (struct group *)0 )
	{
		Warn(english("%s: group id \"%s\" does not exist!"), PRIVSFILE, name);
		return 0;
	}

	return gp->gr_gid;
#	else	/* AUSAS == 0 */
	Passwd		grpwd;

	if ( !GetUid(&grpwd, name) )
	{
		Warn(english("%s: group id \"%s\" -- %s."), PRIVSFILE, name, grpwd.P_error);
		return 0;
	}

	return grpwd.P_uid;
#	endif	/* AUSAS == 0 */
}



/*
**	Return next privilege token from file.
**
**	<space>priv<space>
*/

static char *
get_priv(new)
	bool *		new;
{
	register int	c;
	register char *	cp;
	register char *	sp;
	register bool	esc;
	register bool	skip;
	register bool	copy;
	static bool	newline;

	Trace2(5, "get_priv() at {%s}", ExpandString(DataPoint, 8));

	*new = false;

	esc = false;
	copy = false;
	skip = false;
	sp = NULLSTR;

	for ( cp = DataPoint ; (c = *cp++) != '\0' ; )
	{
		if ( skip && c != '\n' )
			continue;

		if ( esc )
		{
			esc = false;
			copy = true;
			goto escaped;
		}

		switch ( c )
		{
		case '\\':	esc = true;
				continue;
		case '#':	skip = true;
				continue;
		case '\n':	newline = true;
				skip = false;
				if ( sp != NULLSTR )
					break;
				continue;
		case ' ':
		case '\t':
		case ';':
		case ',':	newline = false;
				if ( sp != NULLSTR )
					break;
				continue;
escaped:	default:	if ( newline )
					return NULLSTR;
				if ( sp == NULLSTR )
					sp = cp-1;
				continue;
		}

		DataPoint = cp;
		*--cp = '\0';
out:		if ( copy )
		{
			*new = true;
			return escape(sp);
		}
		return sp;
	}

	DataPoint = --cp;
	goto out;
}



/*
**	Return next user/group token from file.
**
**	<NL>user<:> || <NL>group<:><:>
*/

static char *
get_user(grp, new)
	bool *		grp;
	bool *		new;
{
	register int	c;
	register bool	newline;
	register char *	cp;
	register char *	sp;
	register bool	esc;
	register bool	copy;

	Trace2(5, "get_user() at {%s}", ExpandString(DataPoint, 8));

	*grp = false;
	*new = false;

	esc = false;
	copy = false;
	newline = true;
	sp = NULLSTR;

	for ( cp = DataPoint ; (c = *cp++) != '\0' ; )
	{
		if ( esc )
		{
			esc = false;
			copy = true;
			goto escaped;
		}

		switch ( c )
		{
		case ':':	if ( sp == NULLSTR )
					break;
				if ( *cp == ':' )
				{
					*grp =true;
					cp[-1] = '\0';
					cp++;
				}
				DataPoint = cp;
				*--cp = '\0';
				if ( copy )
				{
					*new = true;
					return escape(sp);
				}
				return sp;
		case '\n':	newline = true;
				sp = NULLSTR;
				continue;
		case '\\':	esc = true;
				continue;
		case '#':
		case ' ':
		case '\t':
		case ';':
		case ',':	sp = NULLSTR;
				break;
escaped:	default:	if ( newline )
					sp = cp-1;
		}

		newline = false;
	}

	DataPoint = --cp;
	return NULLSTR;
}



/*
**	Look up flag privilege.
*/

static Pflg_t
lookup(name)
	char *	name;
{
	PFlag *	fp;
	PFlag	key;

	Trace2(5, "lookup(%s)", name);

	if ( strcmp(name, "*") == STREQUAL )
		return P_ALL;

	key.pf_name = name;

	if
	(
		(fp = (PFlag *)bsearch
			(
				(char *)&key,
				(char *)PFlags,
				NFLAGS,
				PFLAGZ,
				cmp_flg
			)
		) == (PFlag *)0
	)
		return (Pflg_t)0;

	Trace2(6, "lookup bit %#x", fp->pf_bit);

	return fp->pf_bit;
}



/*
**	Read `privsfile', building list.
*/

static void
read_privs()
{
	register char *	cp;
	register Upriv *lp;
	register Upriv**lpp;
	char *		data;
	bool		grp;
	bool		new;

	Trace1(2, "read_privs");

	if ( (data = ReadFile(PRIVSFILE)) == NULLSTR )
		return;

	DataPoint = data;
	lpp = &PrivsList;

	while ( (cp = get_user(&grp, &new)) != NULLSTR )
	{
		Trace2(4, "process user %s", cp);

		lp = Talloc0(Upriv);
		*lpp = lp;
		lpp = &lp->next;

		if ( cp[0] != '*' || cp[1] != '\0' )
		{
			lp->user = newstr(cp);

			if ( grp )
				lp->gid = get_gid(cp);
		}

		if ( new )
			free(cp);

		while ( (cp = get_priv(&new)) != NULLSTR )
		{
			Trace2(4, "process priv %s", cp);

			switch ( cp[0] )
			{
			case '@':	lp->region = newstr(cp+1);	break;
			case '-':
			case '!':
			case '~':	lp->off |= lookup(cp+1);	break;
			case '+':	cp++;
			default:	lp->on |= lookup(cp);		break;
			}

			if ( new )
				free(cp);
		}

		Trace5(
			3,
			"%s: on %#x off %#x @%s",
			(lp->user==NULLSTR)?"*":lp->user,
			lp->on,
			lp->off,
			(lp->region==NULLSTR)?EmptyString:lp->region
		);
	}

#	if	AUSAS == 0
	endgrent();
#	endif	/* AUSAS == 0 */

	free(data);
}



/*
**	Set privileges for user from list.
*/

static void
set_privs(lp, pp)
	register Upriv *	lp;
	register Passwd *	pp;
{
	Trace5(
		2,
		"set_privs for %s [on %#x off %#x @%s]",
		(lp->user==NULLSTR)?"*":lp->user,
		lp->on,
		lp->off,
		(lp->region==NULLSTR)?EmptyString:lp->region
	);

	if ( lp->off == P_ALL )
		pp->P_flags &= ~P_ALL;

	pp->P_flags |= lp->on;

	if ( lp->off != P_ALL )
		pp->P_flags &= ~lp->off;

	if ( lp->region != NULLSTR )
	{
		if ( lp->region[0] == '\0' )
			pp->P_region = NULLSTR;
		else
			pp->P_region = lp->region;
	}
}
