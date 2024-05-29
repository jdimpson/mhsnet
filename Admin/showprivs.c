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


static char	sccsid[]	= "%W% %E%";

/*
**	Show privileges.
*/

#define	STDIO

#include	"global.h"
#include	"Args.h"
#include	"sysexits.h"

#define	PFLAGS_DATA
#include	"Passwd.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */

#include	<ctype.h>


/*
**	Parameters set from arguments.
*/

bool	AllUsers;				/* Show all users' privileges */
char *	Name	= sccsid;
int	Traceflag;

AFuncv	getUser _FA_((PassVal, Pointer));	/* Particular user (SU only) */

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, &AllUsers, 0),
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_string(P, &PRIVSFILE, 0, "privsfile", OPTARG),
	Arg_noflag(0, getUser, english("user"), OPTARG|MANY),
	Arg_end
};

/*
**	User list for extract.
*/

typedef struct User	User;

struct User
{
	User *	next;
	char *	name;
};

User *	UserList;		/* All users mentioned in ``privsfile'' */
int	UserCount;		/* Number in list */

/*
**	Miscellaneous
*/

Passwd	Me;			/* Details of invoker */
bool	NotMe;			/* Not default */

#define	NAMELEN	14		/* Max length of a login name */

#define	Printf	(void)printf

void	raise_case _FA_((void));
void	show_all _FA_((void));
void	show_privs _FA_((Passwd *));



/*
**	Argument processing.
*/

int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	InitParams();

	GetNetUid();
	GetInvoker(&Me);

	raise_case();

	setbuf(stdout, SObuf);

	DoArgs(argc, argv, Usage);

	if ( AllUsers )
	{
		if ( !(Me.P_flags & P_SU) )
			Error(english("no permission to use \"-a\" flag"));

		if ( NotMe )
			Error(english("Ambiguous."));

		show_all();
	}
	else
	if ( !NotMe )
		show_privs(&Me);

	finish(EX_OK);
	return 0;
}



/*
**	Called from ``GetAllPrivUsers()''.
*/

void
build_users(name)
	char *		name;
{
	register User *	up;

	up = Talloc(User);
	up->name = name;
	up->next = UserList;
	UserList = up;
	UserCount++;
}



/*
**	Compare two user list elements.
*/

int
cmp_users(u1, u2)
	char *	u1;
	char *	u2;
{
	return strcmp(((User *)u1)->name, ((User *)u2)->name);
}



void
finish(error)
	int	error;
{
	exit(error);
}



/*
**	Particular user.
*/
/*ARGSUSED*/
AFuncv
getUser(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Passwd		pwd;

	if ( val.p[0] == '\0' )
		return ACCEPTARG;

	if
	(
		!(Me.P_flags & P_SU)
		&&
		strcmp(val.p, Me.P_user) != STREQUAL
	)
	{
		NoArgsUsage = true;
		return (AFuncv)english("no permission for other users");
	}

	if ( !GetUid(&pwd, val.p) )
	{
		pwd.P_name = val.p;
		pwd.P_flags = 0;
		pwd.P_region = NULLSTR;
		pwd.P_gid = 0;
	}

	show_privs(&pwd);

	NotMe = true;

	return ACCEPTARG;
}



/*
**	Raise case of flags.
*/

void
raise_case()
{
	register char *	cp;
	register char *	np;
	register int	c;
	register int	i;

	for ( i = NFLAGS ; --i >= 0 ; )
	{
		/** Copy in case strings are `readonly' **/

		np = newstr(PFlags[i].pf_name);

		for ( cp = np ; (c = *cp) != '\0' ; cp++ )
			if ( islower(c) )
				*cp = toupper(c);

		PFlags[i].pf_name = np;
	}
}



/*
**	Show all privileged users.
*/

void
show_all()
{
	register User *	up;
	Passwd		pwd;
	extern void	GetAllPrivUsers();

	/** Build list of all users. **/

	GetAllPrivUsers(build_users);

	/** Sort list. **/

	if ( UserCount > 1 )
		listsort((char **)&UserList, cmp_users);

	/** Show each. **/

	for ( up = UserList ; up != (User *)0 ; up = up->next )
	{
		if ( !GetUid(&pwd, up->name) )
		{
			pwd.P_name = up->name;
			pwd.P_flags = 0;
			pwd.P_region = NULLSTR;
			pwd.P_gid = 0;
		}

		show_privs(&pwd);
	}

	pwd.P_name = "*DEFAULT*";
	pwd.P_flags = 0;
	pwd.P_region = NULLSTR;
	pwd.P_gid = 0;

	show_privs(&pwd);
}



/*
**	Show privileges for a user.
*/

void
show_privs(pp)
	Passwd *	pp;
{
	register int	i;
	register int	bit;
	register int	flags;
	register int	notfirst;
	char		name[NAMELEN+1];

	if ( pp != &Me )
		GetPrivs(pp);

	(void)sprintf(name, "%.*s:", NAMELEN-1, pp->P_name);
	Printf("%-*s", NAMELEN, name);

	for ( notfirst = 0, flags = pp->P_flags, bit = 1 ; flags ; bit <<= 1 )
	{
		if ( (bit & flags) == 0 )
			continue;

		flags &= ~bit;

		if ( notfirst++ )
			putchar(',');
		else
			putchar(' ');

		for ( i = NFLAGS ; --i >= 0 ; )
		{
			if ( PFlags[i].pf_bit != bit )
				continue;
			Printf("%s", PFlags[i].pf_name);
			break;
		}
	}

	if ( pp->P_region != NULLSTR )
		Printf(" @%s", pp->P_region);

	putchar('\n');
}
