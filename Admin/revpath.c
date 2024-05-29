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
**	Change pathname into address, or vice-versa.
*/

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"link.h"
#include	"routefile.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters set from arguments.
*/

bool	ForceAddress;
bool	ForcePath;
char *	Name	= sccsid;
char *	Path;
char *	Remove;
bool	ShowLink;
bool	Strip;
int	Traceflag;

/*
**	Arguments.
*/

AFuncv	getType _FA_((PassVal, Pointer));	/* Address type */

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, &ForceAddress, 0),
	Arg_bool(l, &ShowLink, 0),
	Arg_bool(p, &ForcePath, 0),
	Arg_bool(s, &Strip, 0),
	Arg_string(t, 0, getType, english("External>|<Internal"), OPTARG|OPTVAL),
	Arg_int(T, &Traceflag, getInt1, "trace level", OPTARG|OPTVAL),
	Arg_noflag(&Path, 0, "path>|<address", 0),
	Arg_noflag(&Remove, 0, "remove>|<prefix", OPTARG),
	Arg_end
};

/*
**	Miscellaneous definitions.
*/

char *	(*AdrsFuncP) _FA_((Addr *))	= TypedAddress;
void	process _FA_((void));

#define	Printf		(void)printf



/*
**	Argument processing.
*/

int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	InitParams();

	DoArgs(argc, argv, Usage);

	process();

	exit(EX_OK);
}



void
finish(error)
	int	error;
{
	exit(error);
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
	case 040:	AdrsFuncP = UnTypAddress;	break;
	case 'e':
	case 'x':	AdrsFuncP = ExpTypAddress;	break;
	case 'i':	AdrsFuncP = TypedAddress;	break;
	default:	return (AFuncv)english("unrecognised types option");
	}

	return ACCEPTARG;
}



void
process()
{
	char *	pp;
	char *	np;
	char *	cp;
	char *	ad;
	Addr *	ap;
	int	len;

	Trace3(1, "process() path=%s, remove=%s", Path, Remove);

	if ( Remove != NULLSTR )
		len = strlen(Remove);
	else
		len = 0;

	pp = EmptyString;
	np = Path;

	if ( np == NULLSTR )
		return;

	if ( *np == '.' )
		np++;

	if ( *np == '\0' )
		return;

	if ( ShowLink )
	{
		Link	link;

		do
		{
			ap = StringToAddr(concat(pp, np, NULLSTR), NO_STA_MAP);
			cp = TypedName(ap);
			pp = "9=";
		}
			while ( DESTTYPE(ap) && cp[0] != '9' );

		if ( !FindLink(cp, &link) && FindAddr(ap, &link, FASTEST) != wh_link )
			Error(english("can't find link to \"%s\""), Path);

		Printf("%s%s\n", SPOOLDIR, link.ln_name);

		FreeAddr(&ap);
		return;
	}

	if ( !ForcePath && (ForceAddress || strchr(np, '/') == NULLSTR) )
	{
		do
		{
			ap = StringToAddr(concat(pp, np, NULLSTR), NO_STA_MAP);
			cp = TypedName(ap);
			pp = "9=";
		}
			while ( !Strip && DESTTYPE(ap) && cp[0] != '9' );

		np = DomainToPath(Strip?UnTypName(ap):TypedName(ap));

		if ( len > 0 )
		{
			Printf("%s", Remove);

			if ( Remove[--len] != '/' )
				putchar('/');
		}

		if ( np != NULLSTR )
		{
			Printf("%s\n", np);
			free(np);
		}
		else
			putchar('\n');

		FreeAddr(&ap);
		return;
	}

	if ( len > 0 && strncmp(Path, Remove, len) == STREQUAL )
		Path += len;

	if ( *Path == '/' )
		Path++;

	np = ad = newstr(Path);
	
	cp = newstr(np);

	while ( (pp = strrchr(cp, '/')) != NULLSTR )
	{
		*pp++ = '\0';
		np = strcpyend(np, pp);
		*np++ = DOMAIN_SEP;
	}

	np = strcpyend(np, cp);
	free(cp);

	ap = StringToAddr(ad, true);

	Printf("%s\n", (*AdrsFuncP)(ap));

	FreeAddr(&ap);
	free(ad);
}
