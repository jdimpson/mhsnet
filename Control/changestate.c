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


static char	sccsid[]	= "@(#)changestate.c	1.32 05/12/16";

/*
**	Change of state processor, invoked by router/admin.
**
**	Invoke `request' to broadcast changed state info. to statehandlers.
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	LOCKING
#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"exec.h"
#include	"expand.h"
#include	"handlers.h"
#include	"header.h"
#include	"link.h"
#include	"params.h"
#include	"Passwd.h"
#include	"route.h"
#include	"routefile.h"
#include	"spool.h"
#include	"statefile.h"
#include	"sysexits.h"
#if	SUN3 == 1
#include	"sun3.h"
#endif	/* SUN3 == 1 */


/*
**	Parameters set from arguments.
*/

bool	Break;			/* Break link to home */
char *	ChangeFlags;		/* Change these flags for link */
char *	Filter;			/* Filter for link */
bool	Input;			/* Out-bound link only */
bool	MarkDead;		/* Mark link dead */
bool	MarkDown;		/* Mark link down */
char *	Name	= sccsid;	/* Program invoked name */
bool	Output;			/* In-bound link only */
int	Traceflag;		/* Global tracing control */

AFuncv	getAddress _FA_((PassVal, Pointer));
AFuncv	getLink _FA_((PassVal, Pointer));

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(b, &Break, 0),
	Arg_bool(d, &MarkDown, 0),
	Arg_bool(i, &Input, 0),
	Arg_bool(o, &Output, 0),
	Arg_bool(z, &MarkDead, 0),
	Arg_string(F, &ChangeFlags, 0, english("flags"), OPTARG),
	Arg_string(L, 0, getLink, english("link"), OPTARG),
	Arg_string(R, &Filter, 0, english("link_filter"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_noflag(0, getAddress, english("linkname|address"), OPTARG|MANY),
	Arg_end
};

/*
**	Link structure, one per link being processed.
*/

typedef struct Lnk	Lnk;

struct Lnk
{
	Lnk *	next;
	Link	data;
};

Lnk *	Links;
int	NLinks;

/*
**	Miscellaneous
*/

Time_t	DeadTime;		/* Search for inactive links older than this */
Addr *	Destinations;		/* For resolving complex address arguments */
Passwd	Invoker;		/* Person invoked by */
char *	LinkAddress;		/* Typed link address */
bool	NewHost;		/* This is a new host */
bool	Validated;		/* New host pre-validated by passwd file */

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"TRACEFLAG",		(char **)&Traceflag,		PINT},
};

#define	Fprintf	(void)fprintf

void	errf _FA_((Addr *, Addr *));
bool	find_links _FA_((void));
bool	linkf _FA_((Addr *, Addr *, Link *));
void	list_link _FA_((Link *));
void	mail _FA_((Lnk *, bool));
void	mailpr _FA_((FILE *));
void	newstate _FA_((Lnk *, bool));
int	do_links _FA_((void));

#ifdef	SUN3SPOOLDIR

/*
**	Configurable parameters.
*/

ParTb	Sun3Params[] =
{
	{"SUN3STATEP",		&SUN3STATEP,			PSTRING},
};

char	*FirstName _FA_((char *)), *getdoms _FA_((char *)), *gethier _FA_((char *));
void	sun3state _FA_((ExBuf *, Lnk *));

#endif	/* SUN3SPOOLDIR */



/*
**	Argument processing.
*/

int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	int	count;

	InitParams();

	SetUlimit();

	GetInvoker(&Invoker);
	SetUser(NetUid, NetGid);

	if ( geteuid() != NetUid || !(Invoker.P_flags & P_SU) )
	{
		Warn(english("No permission."));
		exit(EX_NOPERM);
	}

	if ( !ReadRoute(SUMCHECK) )
	{
		Warn("No routing tables.");
		exit(EX_OSFILE);
	}

	while ( chdir(SPOOLDIR) == SYSERROR )
		Syserror(CouldNot, "chdir", SPOOLDIR);

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

#	ifdef	SUN3SPOOLDIR
	GetParams(SUN3PARAMS, Sun3Params, sizeof Sun3Params);
#	endif	/* SUN3SPOOLDIR */

	if ( Destinations != (Addr *)0 )
	{
		static Passwd	nullpw;
		Addr *		source = StringToAddr(newstr(HomeName), STA_MAP);

		if ( Break || ChangeFlags != NULLSTR )
		{
			Error(english("no link to change"));
			exit(EX_NOHOST);
		}

		nullpw.P_flags = P_ALL;

		(void)CheckAddr(&Destinations, &nullpw, Error, true);

		(void)DoRoute(source, &Destinations, FASTEST, (Link *)0, (DR_fp)linkf, errf);

		FreeAddr(&source);
		FreeAddr(&Destinations);
	}
	else
	if ( NLinks == 0 )
	{
		if ( Break || ChangeFlags != NULLSTR )
		{
			Error(english("need link to change"));
			exit(EX_USAGE);
		}

		(void)find_links();
		DeadTime = Time - WEEK;
		MarkDead = true;
	}

	if ( NLinks == 0 )
	{
		Error(english("no network links found!"));
		exit(EX_OSFILE);
	}

	/*
	**	Process links.
	*/

	count = do_links();

	/*
	**	If routing tables changed, try re-routing hung messages.
	*/

	if ( CheckRoute(SUMCHECK) )
		count += ReRoute(REROUTEDIR, ROUTINGDIR);

	if ( count && !DaemonActive(ROUTINGDIR, true) )
		Warn(english("routing daemon not running."));

	exit(EX_OK);
	return 0;
}



/*
**	Process state for each listed link.
*/

int
do_links()
{
	register Lnk *	lp;
	register char *	cp;
	register int	count;
	Passwd		pw;

	Trace2(1, "do_links(), %d links", NLinks);

	for ( count = 0, lp = Links ; lp != (Lnk *)0 ; lp = lp->next )
	{
		/*
		**	If link is unknown, add it to statefile.
		*/

		if ( lp->data.ln_name == NULLSTR )
		{
			if ( Break || MarkDead || ChangeFlags != NULLSTR )
				continue;

			if
			(
				GetPassword(&pw, lp->data.ln_rname)
				&&
				pw.P_name[0] != ATYP_BROADCAST
			)
				Validated = true;	/* Link has been validated */
			else
				Validated = false;

			newstate(lp, true);

			if ( NETADMIN >= 1 )
				mail(lp, true);

			continue;
		}

		if
		(
			Break
			||
			ChangeFlags != NULLSTR
			||
			(
				Filter != NULLSTR
				&&
				(
					lp->data.ln_filter == NULLSTR
					||
					(strcmp(Filter, lp->data.ln_filter) != STREQUAL)
				)
			)
		)
		{
			newstate(lp, false);
		}
		else
		if ( lp->data.ln_flags & FL_NOCHANGE )
		{
			if ( MarkDown || (MarkDead && DeadTime == 0) )
				Warn(english("link %s is marked \"nochange\""), lp->data.ln_rname);
			continue;
		}
		else
		if ( DeadTime != 0 )
		{
			Time_t	t;

			if ( lp->data.ln_flags & FL_DEAD )
				continue;

			t = GetLnkTime(lp->data.ln_name);

			Trace4(2, "link %s: linktime %lu deadtime %lu", lp->data.ln_name, (PUlong)t, (PUlong)DeadTime);

			if ( t == 0 || t > DeadTime )
				continue;

			newstate(lp, false);

			if ( NETADMIN >= 1 )
				mail(lp, false);
		}
		else
		if
		(
			((lp->data.ln_flags & (FL_DEAD|FL_DOWN)) && !MarkDead && !MarkDown)
			||
			(!(lp->data.ln_flags & FL_DEAD) && MarkDead)
			||
			(!(lp->data.ln_flags & FL_DOWN) && MarkDown)
		)
		{
			int	level;

			newstate(lp, false);

			if ( MarkDead )
				level = 1;
			else
				level = 3;

			if ( NETADMIN >= level )
				mail(lp, false);
		}

		/*
		**	Re-route any messages queued from node just broken/dead/down.
		*/

		if ( Break || ((MarkDead || MarkDown) && NLinks > 1) )
		{
			cp = concat(lp->data.ln_name, Slash, LINKCMDSNAME, Slash, NULLSTR);
			count += ReRoute(cp, ROUTINGDIR);
			free(cp);
		}
	}

	return count;
}



/*
**	Unknown address error.
*/
/*ARGSUSED*/
void
errf(source, dest)
	Addr *	source;
	Addr *	dest;
{
	if ( AddrAtHome(&dest, false, false) )
		Error(english("This IS \"%s\"!"), UnTypAddress(dest));
	else
		Error(english("Link for \"%s\" unknown."), UnTypAddress(dest));
}



/*
**	Search routing table for links.
*/

bool
find_links()
{
	register int	i;
	bool		found = false;
	Link		data;

	Trace1(1, "find_links()");

	for ( i = LinkCount ; --i >= 0 ; )
	{
		GetLink(&data, (Index)i);

		list_link(&data);

		found = true;
	}

	return found;
}



/*
**	Called from the errors routines to cleanup
*/

void
finish(error)
	int		error;
{
	register int	i;

	for ( i = 0 ; i < 9 ; i++ )
		UnLock(i);

	(void)exit(error);
}



/*
**	Find name of explicit link or save address for later resolution.
*/

/*ARGSUSED*/
AFuncv
getAddress(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	name;
	Addr *		ap;
	Link		data;

	if ( (name = strrchr(val.p, '/')) != NULLSTR )
		name++;
	else
		name = val.p;

	ap = StringToAddr(name, NO_STA_MAP);

	if ( !FindLink(TypedName(ap), &data) )
	{
		AddAddr(&Destinations, ap);
		return ACCEPTARG;
	}

	FreeAddr(&ap);

	list_link(&data);

	return ACCEPTARG;
}



/*
**	Find explicit link.
*/

/*ARGSUSED*/
AFuncv
getLink(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Link		data;

	if ( !FindLink(val.p, &data) )
	{
		(void)strclr((char *)&data, sizeof data);	/* New link */
		data.ln_rname = val.p;
		data.ln_flags = (FL_DEAD|FL_DOWN);
	}

	list_link(&data);

	return ACCEPTARG;
}



/*
**	List link from routing function.
*/
/*ARGSUSED*/
bool
linkf(source, dest, link)
	Addr *	source;
	Addr *	dest;
	Link *	link;
{
	list_link(link);
	return true;
}



/*
**	Add a link to list.
*/

void
list_link(datap)
	Link *		datap;
{
	register Lnk *	lp;

	Trace2(1, "list_link(%s)", datap->ln_rname);

	for ( lp = Links ; lp != (Lnk *)0 ; lp = lp->next )
		if ( strcmp(lp->data.ln_rname, datap->ln_rname) == STREQUAL )
			return;

	lp = Talloc(Lnk);

	lp->data = *datap;

	lp->next = Links;
	Links = lp;

	NLinks++;
}



/*
**	Send mail to local guru about new site.
*/

void
mail(lp, newhost)
	Lnk *	lp;
	bool	newhost;
{
	char *	errs;

	ExHomeAddress = StripTypes(HomeName);
	ExLinkAddress = StripTypes(lp->data.ln_rname);
	LinkAddress = lp->data.ln_rname;
	NewHost = newhost;

	if ( (errs = MailNCC(mailpr, newhost?NCC_MANAGER:NCC_ADMIN)) != NULLSTR )
	{
		Warn(StringFmt, errs);
		free(errs);
	}
}



/*
**	Called from "MailNCC()" to produce body of mail.
*/

void
mailpr(fd)
	FILE *	fd;
{
	char *	sl1;
	char *	sl2;
	static char	linkstr[] = english("Subject: Link from \"%s\" to \"%s\" marked \"%s\".\n\n");

	if ( NewHost )
	{
		Fprintf(fd, english("Subject: New site \"%s\" connected to \"%s\"\n\n"), ExLinkAddress, ExHomeAddress);

		if ( Validated )
			return;

		if ( Input )
		{
			sl1 = ExHomeAddress;
			sl2 = ExLinkAddress;
		}
		else
		if ( Output )
		{
			sl1 = ExLinkAddress;
			sl2 = ExHomeAddress;
		}
		else
			return;

		Fprintf(fd, english("The direction from \"%s\" to \"%s\"\n"), sl1, sl2);
		Fprintf
		(
			fd,
			english("has been marked \"dead\" in case there is a pre-existing route.\n\n")
		);
		Fprintf
		(
			fd,
			english("Please remove the flag if this link is to become the preferred route.\n\n")
		);
		Fprintf
		(
			fd,
			english("You can do this by invoking:\n  %s%s%s -F-dead,-nochange %s\n\n"),
			SPOOLDIR, LIBDIR, NEWSTATEHANDLER,
			ExLinkAddress
		);
	}
	else
	if ( MarkDead )
		Fprintf(fd, linkstr, ExHomeAddress, ExLinkAddress, "dead");
	else
	if ( MarkDown )
		Fprintf(fd, linkstr, ExHomeAddress, ExLinkAddress, "down");
	else
		Fprintf(fd, linkstr, ExHomeAddress, ExLinkAddress, "up");
}



/*
**	Run state program to alter state information.
**
**	english(" Consult ``Include/commands.h'' for command names. ")
*/

void
newstate(lp, newhost)
	Lnk *		lp;
	bool		newhost;
{
	register FILE *	fd;
	register char *	h1;
	register char *	h2;
	char *		errs;
	char *		cp1;
	bool		halfbreak;
	char		nbuf[NUMERICARGLEN];
	ExBuf		ea;

	static char	flagstr[]	= "flag\t%s,%s\t%s\n";
	static char	linkstr[]	= english("Link %s \"%s\" marked \"%s\"");
	static char	tostr[]		= "to";

	h1 = lp->data.ln_rname;
	h2 = HomeName;

	Trace3(1, "newstate(%s, %s)", h1, newhost?"newhost":EmptyString);

	FIRSTARG(&ea.ex_cmd) = STATE;
	/*
	**	(Accept new commands on stdin, and
	**	 update the state and routing files.)
	*/
	NEXTARG(&ea.ex_cmd) = "-crsxC";

	if ( !newhost && !MarkDead )
		NEXTARG(&ea.ex_cmd) = NumericArg(nbuf, 'Z', (Ulong)DOWN_DELAY);

	fd = (FILE *)Execute(&ea, NULLVFUNCP, ex_pipe, SYSERROR);

	if ( newhost )
	{
		Fprintf(fd, "add\t%s\n", h1);
		Fprintf(fd, "link\t%s,%s", h1, h2);

		if ( Input || Output )	/* One-way connection */
		{
			Fprintf(fd, "\tnochange\n");
			if ( !Validated )
			{
				if ( Input )
					Fprintf(fd, flagstr, h2, h1, "dead");
				else
					Fprintf(fd, flagstr, h1, h2, "dead");
			}
		}
		else
			putc('\n', fd);

		Report3(english("New site \"%s\" connected to \"%s\""), h1, h2);
	}
	else
	{
		if ( Break )
		{
			if ( Input || Output )
			{
				if ( Input )
				{
					h2 = h1;
					h1 = HomeName;
				}

				halfbreak = true;

				Report3(english("Breaking halflink from \"%s\" to \"%s\""), h2, h1);
			}
			else
			{
				halfbreak = false;

				Report2(english("Breaking link to \"%s\""), h1);
			}

			Fprintf
			(
				fd,
				"%sbreak\t%s,%s\n",
				halfbreak?"half":EmptyString,
				h2,
				h1
			);
		}
		else
		if ( ChangeFlags != NULLSTR )
		{
			if ( !Input )
			{
				cp1 = tostr;
				Fprintf(fd, flagstr, h2, h1, ChangeFlags);
			}
			else
			{
				cp1 = "from";
				Fprintf(fd, flagstr, h1, h2, ChangeFlags);
			}

			Report4(linkstr, cp1, h1, ChangeFlags);
		}
		else
		{
			if ( MarkDead )
			{
				errs = "+dead";
				cp1 = &errs[1];
			}
			else
			if ( MarkDown )
			{
				errs = "+down";
				cp1 = &errs[1];
			}
			else
			{
				errs = "-dead,-down";
				cp1 = "up";
			}

			Fprintf(fd, flagstr, h2, h1, errs);
			Report4(linkstr, tostr, h1, cp1);
		}
	}

	if ( Filter != NULLSTR )
		Fprintf
		(
			fd,
			"filter\t%s\t%s\n",
			lp->data.ln_rname,
			Filter
		);

	if ( (errs = ExClose(&ea, fd)) != NULLSTR )
	{
		Error(StringFmt, errs);
		free(errs);
	}

#	ifdef	SUN3SPOOLDIR
	if ( SUN3STATEP != NULLSTR && access(SUN3STATEP, 1) != SYSERROR )
		sun3state(&ea, lp);
#	endif	/* SUN3SPOOLDIR */

	if ( !newhost )
		return;

	/*
	**	Request state info. from new link.
	*/

	FIRSTARG(&ea.ex_cmd) = concat(LIBDIR, REQUESTER, NULLSTR);
	NEXTARG(&ea.ex_cmd) = lp->data.ln_rname;

	if ( (errs = Execute(&ea, NULLVFUNCP, ex_exec, SYSERROR)) != NULLSTR )
	{
		Error(StringFmt, errs);
		free(errs);
	}
}



#ifdef	SUN3SPOOLDIR

/*
**	Run SUN3 state program to update link state.
*/

void
sun3state(eap, lp)
	ExBuf *	eap;
	Lnk *	lp;
{
	FILE *	fd;
	char *	source = StripDomain(newstr(lp->data.ln_rname), OzAu, Au, false);
	char *	name = FirstName(source);
	char *	home = FirstName(HomeName);
	char *	doms = getdoms(source);
	char *	flags;

	Trace2(1, "sun3state(%s)", source);

	/** Add local links if necessary **/

	FIRSTARG(&eap->ex_cmd) = SUN3STATEP;
	NEXTARG(&eap->ex_cmd) = "-CORSc";	/* The 'O' is to prevent a state message being generated! */

	fd = (FILE *)Execute(eap, NULLVFUNCP, ex_pipe, SYSERROR);

	if ( MarkDead )
		flags = "+dead";
	else
	if ( MarkDown )
		flags = "+down";
	else
		flags = "-dead,-down";

	Fprintf
	(
		fd,
		"add\t%s\nhierarchy\t%s\t%s\ndomain\t%s\t%s\nlink\t%s,%s\tmsg,%s\n",
		name,
		name, gethier(source),
		name, doms,
		name, home,
		flags
	);

	free(doms); free(home); free(name); free(source);

	if ( (doms = ExClose(eap, fd)) != NULLSTR )
	{
		Warn(StringFmt, doms);
		free(doms);
	}
}



/*
**	Return name of first domain in string.
*/

char *
FirstName(acp)
	char *		acp;
{
	register char *	cp;
	register char *	cp1;
	register char *	cp2;

	if ( (cp = acp) == NULLSTR )
		return newstr(EmptyString);

	if ( (cp2 = strchr(cp, DOMAIN_SEP)) != NULLSTR )
		*cp2 = '\0';

	if ( (cp1 = strchr(cp, TYPE_SEP)) != NULLSTR )
		cp1++;
	else
		cp1 = cp;

	cp = newstr(cp1);

	if ( cp2 != NULLSTR )
		*cp2 = DOMAIN_SEP;

	return cp;
}



/*
**	Get domains from type-stripped address.
*/

char *
getdoms(name)
	char *		name;
{
	register char *	cp;
	register char *	dp;

	if ( (dp = strchr(name, DOMAIN_SEP)) != NULLSTR )
	{
		dp = cp = newstr(++dp);
		while ( (cp = strchr(cp, DOMAIN_SEP)) != NULLSTR )
			*cp++ = ',';
		return dp;
	}

	return newstr(EmptyString);
}



/*
**	Get hierarchy from type-stripped address.
*/

char *
gethier(name)
	char *		name;
{
	register char *	cp;

	if ( (cp = strchr(name, DOMAIN_SEP)) != NULLSTR )
		return ++cp;

	return EmptyString;
}

#endif	/* SUN3SPOOLDIR */
