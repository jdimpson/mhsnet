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


static char	sccsid[]	= "@(#)address.c	1.26 05/12/16";

/*
**	Produce canonical addresses, show routes.
*/

#define	RECOVER
#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"header.h"
#include	"link.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters set from arguments.
*/

Addr *	Ap;		/* Composite address */
bool	Check_Addr;	/* Check composite address for legality */
Link	Llink;		/* Optional source link */
Link *	Linkp;		/* Points to Llink or (Link *)0 */
char *	Name = sccsid;	/* Program invoked name */
int	RouteType = FASTEST;	/* Fast / cheap */
bool	ShowNode;	/* Show nodename for address */
bool	ShowLinks;	/* Show linknames for address */
bool	ShowRoutes;	/* Show all routes for composite address */
bool	Site;		/* Show ``site'' addresses (strip node) */
Addr *	Source;		/* Optional source address */
int	Traceflag;	/* Optional tracing */
bool	Verbose;

/*
**	Arguments.
*/

AFuncv	getAddress _FA_((PassVal, Pointer));
AFuncv	getFlags _FA_((PassVal, Pointer));
AFuncv	getLink _FA_((PassVal, Pointer));
AFuncv	getQuality _FA_((PassVal, Pointer));	/* Message priority */
AFuncv	getRoute _FA_((PassVal, Pointer));	/* Route */
AFuncv	getRstrct _FA_((PassVal, Pointer));
AFuncv	getSource _FA_((PassVal, Pointer));
AFuncv	getType _FA_((PassVal, Pointer));	/* Address type */
AFuncv	getUseP _FA_((PassVal, Pointer));	/* Different privileges */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(c, &Check_Addr, 0),
	Arg_bool(l, &ShowLinks, 0),
	Arg_bool(n, &ShowNode, 0),
	Arg_bool(r, &ShowRoutes, 0),
	Arg_bool(s, &Site, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_string(t, 0, getType, english("External>|<Internal"), OPTARG|OPTVAL),
	Arg_number(F, 0, getFlags, english("routing flags"), OPTARG|MANY),
	Arg_string(L, 0, getLink, english("link"), OPTARG),
	Arg_string(N, 0, getRstrct, english("region restriction"), OPTARG|MANY),
	Arg_char(P, 0, getQuality, english("priority"), OPTARG),
	Arg_string(R, 0, getRoute, english("route"), OPTARG),
	Arg_string(S, 0, getSource, english("source"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_string(V, 0, getUseP, english("routing user"), OPTARG|MANY),
	Arg_noflag(0, getAddress, english("address"), OPTARG|MANY),
	Arg_end
};

/*
**	Miscellaneous definitions.
*/

ExpType	AddrType		= notype;
char *	(*AdrsFuncP)_FA_((Addr *)) = UnTypAddress;
Passwd	Me;					/* Details of user */
bool	NoAddress		= true;
int	RetVal			= EX_OK;

void	address _FA_((void));
void	clear_addr_nodes _FA_((Addr *));
void	clear_dest_node _FA_((Dest *));
void	errf _FA_((Addr *, Addr *));
void	routef _FA_((Addr *, Addr *, Link *));

#define	Printf	(void)printf

DODEBUG(extern int malloc_debug(int));
DODEBUG(extern int malloc_verify(void));



/*
**	Argument processing.
*/

int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	DODEBUG((void)malloc_debug(MALLOC_DEBUG));

	InitParams();

	if ( !ReadRoute(SUMCHECK) )
	{
		Warn(english("No routing tables."));
		exit(EX_NOPERM);
	}

	GetInvoker(&Me);

	setbuf(stdout, SObuf);

	HdrRoute = EmptyString;

	DoArgs(argc, argv, Usage);

	if ( Ap != (Addr *)0 )
		address();
	else
	if ( NoAddress )
	{
		PassVal	pv;

		pv.p = newstr(HomeName);

		(void)getAddress(pv, (Pointer)0);
	}

	DODEBUG(if(!malloc_verify())Fatal("malloc_verify() error"));

	exit(RetVal);
	return RetVal;
}



/*
**	Show full address.
*/

void
address()
{
	if ( Check_Addr )
	{
		RetVal = EX_NOHOST;
		(void)CheckAddr(&Ap, &Me, Warn, true);
		RetVal = EX_OK;
	}

	if ( ShowRoutes || ShowLinks )
	{
		if ( Source == (Addr *)0 )
			Source = StringToAddr(newstr(HomeName), NO_STA_MAP);

		if ( Site || ShowNode )
			clear_addr_nodes(Source);

		Recover(ert_return);

		if ( ShowLinks )
			RetVal = EX_NOHOST;

		(void)DoRoute(Source, &Ap, RouteType, Linkp, routef, errf);
	}
	else
	if ( Verbose )
		Printf("\t==> %s\n", (*AdrsFuncP)(Ap));
}



/*
**	Clear nodes from an address group.
*/

void
clear_addr_nodes(aa)
	Addr *		aa;
{
	register Addr *	ap;

	Trace2(2, "clear_addr_nodes(%s)", ADDRNAME(aa));

	if ( (ap = aa) == (Addr *)0 )
		return;

	FreeStr(&ap->ad_typed);
	FreeStr(&ap->ad_untyped);

	do
	{
		while ( ap->ad_down == (Addr *)0 )
			if ( (ap = ap->ad_next) == (Addr *)0 )
				return;

		if ( DESTTYPE(ap) )
			clear_dest_node(ADDRDEST(ap));
		else
			clear_addr_nodes(ap->ad_down);
	}
	while
		( (ap = ap->ad_next) != (Addr *)0 );
}



void
clear_dest_node(dp)
	register Dest *	dp;
{
	register int	i;
	register Dname *dn;
	register char *	cp;
	extern int	MaxTypC;

	Trace3(2, "clear_dest_node(%s) `%c'", DESTNAME(dp), dp->dt_names->dn_type);

	if ( dp == (Dest *)0 || dp->dt_count == 0 )
		return;

	Trace2(3, "clear_dest_node \"%s\"", dp->dt_names->dn_name);

	if ( ShowNode )
	{
		if ( dp->dt_names->dn_type != MaxTypC && dp->dt_names->dn_type != 0 )
		{
			dp->dt_count = 0;
			dp->dt_name[0] = '\0';
			dp->dt_tname[0] = '\0';
			return;
		}

		dp->dt_count = 1;

		if ( (cp = strchr(dp->dt_name, DOMAIN_SEP)) != NULLSTR )
			*cp++ = '\0';

		if ( (cp = strchr(dp->dt_tname, DOMAIN_SEP)) != NULLSTR )
			*cp++ = '\0';

		return;
	}

	if ( dp->dt_names->dn_type != MaxTypC && dp->dt_names->dn_type != 0 )
		return;

	dp->dt_count--;

	for ( dn = dp->dt_names, i = 0 ; i < dp->dt_count ; i++ )
		dn[i] = dn[i+1];

	if ( (cp = strchr(dp->dt_name, DOMAIN_SEP)) != NULLSTR )
		(void)strcpy(dp->dt_name, ++cp);
	else
		dp->dt_name[0] = '\0';

	if ( (cp = strchr(dp->dt_tname, DOMAIN_SEP)) != NULLSTR )
		(void)strcpy(dp->dt_tname, ++cp);
	else
		dp->dt_tname[0] = '\0';
}



/*
**	Called from DoRoute with error.
*/

/*ARGSUSED*/
void
errf(source, dest)
	Addr *	source;
	Addr *	dest;
{
	RetVal = EX_NOHOST;

	Error(english("Unknown address: %s"), UnTypAddress(dest));
}



/*
**	General cleanup.
*/

void
finish(err)
	int	err;
{
	if ( RetVal == EX_OK )
		RetVal = err;

	exit(RetVal);
}



/*
**	Process address argument.
*/

/*ARGSUSED*/
AFuncv
getAddress(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register Addr *	ap;
	register char * cp;
	register char * op;
	register char * fp;
	register bool	home;
	Addr *		fap;
	extern bool	NeedRemap;
	static char	seps[]	= { ATYP_DESTINATION, /* ATYP_MULTICAST, ATYP_EXPLICIT, */ '\0' };

	NoAddress = false;

	cp = val.p;
	op = newstr(cp);	/* Copies of original */
	fp = newstr(cp);

	for ( home = false ;; )
	{
		NeedRemap = false;

		fap = StringToAddr(fp, STA_MAP);

		DODEBUG(if(!malloc_verify())Fatal("malloc_verify() error"));

		if ( AddrAtHome(&fap, true, false) )
			home = true;

		DODEBUG(if(!malloc_verify())Fatal("malloc_verify() error"));

		Trace2(3, "fap = %#lx", (PUlong)fap);

		if ( !NeedRemap || fap == (Addr *)0 )
			break;

		fp = TypedAddress(fap);

		if ( fp == NULLSTR || fp[0] == '\0' )
			break;

		Trace3(1, "NeedRemap for \"%s\" => \"%s\"", op, fp);

		fp = newstr(fp);

		FreeAddr(&fap);
	}

	AddAddr(&Ap, fap);

	DODEBUG(if(!malloc_verify())Fatal("malloc_verify() error"));

	ap = StringToAddr(cp, NO_STA_MAP);

	if ( Site || ShowNode )
		clear_addr_nodes(ap);

	DODEBUG(if(!malloc_verify())Fatal("malloc_verify() error"));

	if ( (cp = fp = (*AdrsFuncP)(ap)) == NULLSTR )
		cp = EmptyString;

	if ( AddrType != maptype )
		fp = NULLSTR;

	DODEBUG(if(!malloc_verify())Fatal("malloc_verify() error"));

	cp += strspn(cp, seps);

	if ( Verbose )
	{
		Printf("%s\n%s\t==> %s\n", op, home?"(HOME)":EmptyString, cp);

		if ( AddrType != maptype )
			Printf("\t==> %s\n", fp = ExpTypAddress(ap));
	}
	else
	if ( !ShowRoutes && !ShowLinks )
		Printf("%s\n", cp);

	if ( fp != NULLSTR )
		free(fp);
	free(op);

	fap = ap;
	FreeAddr(&fap);

	return ACCEPTARG;
}



/*
**	Get optional permissions.
*/

/*ARGSUSED*/
AFuncv
getFlags(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Me.P_flags = val.l;
	Check_Addr = true;
	return ACCEPTARG;
}		



/*
**	Get source link for route.
*/

/*ARGSUSED*/
AFuncv
getLink(val, arg)
	PassVal		val;
	Pointer		arg;
{
	char *		cp;
	Addr *		link;

	link = StringToAddr((cp = newstr(val.p)), NO_STA_MAP);

	if ( !FindLink(TypedName(link), &Llink) )
	{
		NoArgsUsage = true;
		return (AFuncv)newprintf(english("unknown link: %s"), val.p);
	}

	free(cp);
	FreeAddr(&link);
	Linkp = &Llink;

	ShowRoutes = true;

	return ACCEPTARG;
}		



/*
**	Set message priority.
*/

/*ARGSUSED*/
AFuncv
getQuality(val, arg)
	PassVal		val;
	Pointer		arg;
{
	static char	fmt[] = english("priority: %c (high) to %c (low)");

	if ( val.c >= HIGHEST_QUALITY && val.c <= LOWEST_QUALITY )
	{
		RouteType = (val.c <= FAST_QUALITY) ? FASTEST : CHEAPEST;
		ShowRoutes = true;
		return ACCEPTARG;
	}

	NoArgsUsage = true;
	return (AFuncv)newprintf(fmt, HIGHEST_QUALITY, LOWEST_QUALITY);
}



/*
**	Get message route (expand ',' separated list).
*/

/*ARGSUSED*/
AFuncv
getRoute(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register char *	cp;
	register char *	hp;
	register char *	np;
	register char *	tp;
	Addr *		ap;

	if ( (cp = val.p) == NULLSTR || *cp == '\0' )
		return ACCEPTARG;

	HdrRoute = hp = Malloc(1024);

	do
	{
		*hp++ = ROUTE_RS;
		if ( (np = strchr(cp, ',')) != NULLSTR )
			*np++ = '\0';
		if ( (tp = strchr(cp, ROUTE_US)) != NULLSTR )
			*tp++ = '\0';
		ap = StringToAddr(cp, NO_STA_MAP);
		hp = strcpyend(hp, TypedAddress(ap));
		FreeAddr(&ap);
		*hp++ = ROUTE_US;
		if ( tp != NULLSTR )
			hp = strcpyend(hp, tp);
		else
			*hp++ = '0';
	}
	while
		( (cp = np) != NULLSTR );

	*hp = '\0';

	Trace2(1, "HdrRoute ==> %s", ExpandString(HdrRoute, hp-HdrRoute));

	return ACCEPTARG;
}



/*
**	Get region restriction for user.
*/

/*ARGSUSED*/
AFuncv
getRstrct(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Me.P_region = val.p;
	Check_Addr = true;
	return ACCEPTARG;
}



/*
**	Get source address for route.
*/

/*ARGSUSED*/
AFuncv
getSource(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Source = StringToAddr(val.p, NO_STA_MAP);
	ShowRoutes = true;
	return ACCEPTARG;
}



/*
**	Get optional type for addresses.
*/

/*ARGSUSED*/
AFuncv
getType(val, arg)
	PassVal		val;
	Pointer		arg;
{
	switch ( val.p[0] | 040 )
	{
	case 040:
	case 'e':
	case 'x':	AddrType = maptype;	AdrsFuncP = ExpTypAddress;	break;
	case 'i':	AddrType = intype;	AdrsFuncP = TypedAddress;	break;
	default:
		return (AFuncv)newprintf(english("unrecognised types option: %c"), val.p[0]);
	}

	return ACCEPTARG;
}



/*
**	Change invoker privileges.
*/
/*ARGSUSED*/
AFuncv
getUseP(val, arg)
	PassVal		val;
	Pointer		arg;
{
	if ( val.p[0] == '\0' )
		return ACCEPTARG;

	if ( !GetUid(&Me, val.p) )
		return (AFuncv)Me.P_error;

	GetPrivs(&Me);

	Check_Addr = true;

	return ACCEPTARG;
}



/*
**	Called from DoRoute with found route.
*/

/*ARGSUSED*/
void
routef(source, dest, link)
	Addr *	source;
	Addr *	dest;
	Link *	link;
{
	Addr *	la;
	char *	cp;
	char *	ls;
	char *	ds;

	la = StringToAddr(cp = newstr(link->ln_rname), NO_STA_MAP);
	ls = (*AdrsFuncP)(la);
	ds = (*AdrsFuncP)(dest);

	if ( Verbose )
	{
		if ( ShowRoutes )
			Printf(english(" Route to   %s\n\tvia %s\n"), ds, ls);

		if ( link->ln_spooler == NULLSTR )
			Printf(english("\tdir %s%s\n"), SPOOLDIR, link->ln_name);
		else
		if ( link->ln_spooler[0] == '/' )
			Printf(english("    spooler %s\n"), link->ln_spooler);
		else
			Printf(english("    spooler %s%s\n"), SPOOLDIR, link->ln_spooler);
	}
	else
	{
		if ( ShowRoutes )
			Printf("\t%s ==> ", ds);

		if ( ShowLinks )
			Printf("%s\n", (link->ln_spooler==NULLSTR)?link->ln_name:link->ln_spooler);
		else
			Printf("%s\n", ls);
	}

	if ( AdrsFuncP == ExpTypAddress )
	{
		free(ds);
		free(ls);
	}

	FreeAddr(&la);
	free(cp);
}
