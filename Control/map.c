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


static char	sccsid[]	= "@(#)map.c	1.17 05/12/16";

/*
**	Show state and routing files.
*/

#define	RECOVER
#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"handlers.h"
#include	"link.h"
#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */
#include	"route.h"
#include	"routefile.h"
#include	"statefile.h"
#include	"sysexits.h"



/*
**	Parameters set from arguments.
*/

bool	Aliases;			/* Print aliases */
int	CTraceflag;			/* Commands trace level (not used) */
bool	DistanceSort;			/* Sort regions by distance */
bool	DistAsTime;			/* Print distances as times */
bool	IgnoreCRC	= true;		/* Ignore CRC in statefile */
char *	Name		= sccsid;	/* Program invoked name */
bool	NoImport;			/* Use real statefile */
bool	NoLinks;			/* Just regions */
bool	Print_All;			/* Print all regions despite arguments */
bool	Print_Forced;			/* Print forced routes */
bool	Print_Maps;			/* Print aliases */
bool	Print_Route;			/* Print routing tables */
bool	Print_State;			/* Print network map */
bool	PrintInRegion;			/* Print all regions contained in arguments */
bool	PrintVisible;			/* Print all regions whose visible regions match arguments */
int	Traceflag;			/* Trace level */
bool	Verbose;			/* For lots of verbiage on print-outs */
bool	Warnings;			/* For warnings rather than termination on errors */

/*
**	Arguments.
*/

AFuncv	getNotbool _FA_((PassVal, Pointer));
AFuncv	getParams _FA_((PassVal, Pointer));
AFuncv	getRegion _FA_((PassVal, Pointer));
AFuncv	getRoute _FA_((PassVal, Pointer));
AFuncv	getState _FA_((PassVal, Pointer));
AFuncv	getType _FA_((PassVal, Pointer));

Args	Usage[]	=
{
	Arg_0(0, getName),
	Arg_bool(a, &Print_All, 0),
	Arg_bool(b, &PrintInRegion, 0),
	Arg_bool(c, &IgnoreCRC, getNotbool),
	Arg_bool(d, &DistanceSort, 0),
	Arg_bool(f, &Print_Forced, 0),
	Arg_bool(i, &NoImport, 0),
	Arg_bool(k, &Aliases, 0),
	Arg_bool(l, &NoLinks, 0),
	Arg_bool(m, &Print_Maps, 0),
	Arg_bool(r, &Print_Route, 0),
	Arg_bool(s, &Print_State, 0),
	Arg_bool(v, &Verbose, 0),
	Arg_bool(w, &Warnings, 0),
	Arg_bool(y, &DistAsTime, 0),
	Arg_bool(z, &PrintVisible, 0),
	Arg_string(t, 0, getType, english("External>|<Internal"), OPTARG|OPTVAL),
	Arg_string(R, 0, getRoute, english("routefile"), OPTARG),
	Arg_string(S, 0, getState, english("statefile"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_string(Y, 0, getParams, english("All>|<Cheap>|<Fast"), OPTARG|OPTVAL),
	Arg_noflag(0, getRegion, english("region"), OPTARG|MANY),
	Arg_end
};

/*
**	Name variations.
*/

void	set_map _FA_((void));
void	set_route _FA_((void));

struct Names
{
	char *	n_match;
	vFuncp	n_setup;
}
	Names[]	=
{
	{"map",		set_map		},
	{"route",	set_route	},
	{NULLSTR,	NULLVFUNCP	}
};

/*
**	Miscellaneous definitions.
*/

char *	OtherState;			/* Use alternative statefile */
int	PrintType = PRINT_LINKS|PRINT_HOME;	/* Control printing */
Time_t	StateDate;			/* Statefile date */
bool	StateSet;			/* Statefile set from args */

extern bool	NoJmp;

void	map _FA_((void));

#define	Fprintf		(void)fprintf



/*
**	Argument processing.
*/

int
main(argc, argv)
	int			argc;
	char *			argv[];
{
	register struct Names *	np;
	register int		l1;
	register int		l2;

	InitParams();

	SpoolDirLen = strlen(SPOOLDIR);
	HomeName = EmptyString;	/* In case no ReadRoute */
	NoTypes = true;

	DoArgs(argc, argv, Usage);

	for ( l1 = strlen(Name), np = Names ; np->n_match != NULLSTR ; np++ )
	{
		if ( (l2 = l1 - strlen(np->n_match)) < 0 )
			continue;
		if ( strccmp(&Name[l2], np->n_match) == STREQUAL )
			(*np->n_setup)();
	}

	if ( !(Print_Maps||Print_Route||Print_Forced||Aliases) )
		Print_State = true;

	if ( Print_All )
		PrintType |= PRINT_ALL;

	if ( DistanceSort )
		PrintType |= PRINT_DSORT;

	if ( Verbose )
		PrintType |= PRINT_VERBOSE;

	if ( NoLinks )
		PrintType &= ~PRINT_LINKS;

	if ( Traceflag == 0 )
		setbuf(stdout, SObuf);

	map();

	exit(EX_OK);
	return 0;
}



/*
**	Cleanup for error routines.
*/

void
finish(error)
	int	error;
{
	(void)exit(error);
}



/*
**	Invert boolean.
*/

/*ARGSUSED*/
AFuncv
getNotbool(val, arg)
	PassVal		val;
	Pointer		arg;
{
	*(bool *)arg = false;
	return ACCEPTARG;
}



/*ARGSUSED*/
AFuncv
getParams(val, arg)
	PassVal		val;
	Pointer		arg;
{
	PrintType |= PRINT_ROUTE;
	Print_State = true;

	switch ( val.p[0] | 040 )
	{
	case 'c':	PrintType |= PRINT_CHEAPEST;		break;
	case 'a':	PrintType |= PRINT_CHEAPEST;
	case 040:
	case 'f':	PrintType |= PRINT_FASTEST;		break;
	default:	return english("unrecognised routing parameter");
	}

	return ACCEPTARG;
}



AFuncv
getRegion(val, arg)
	PassVal		val;
	Pointer		arg;
{
	Addr *		ap;
	static bool	print_only;

	ap = StringToAddr(val.p, true);

	if ( !Print_All )
	{
		PrintOnly(newstr(TypedName(ap)));
		PrintOnly(newstr(UnTypName(ap)));
	}

	if ( print_only || (PrintType&PRINT_ROUTE) || NoImport )
	{
		PrintType |= PRINT_HOME;
		FreeStr(&OtherState);
	}
	else
	{
		OtherState = newstr(TypedName(ap));
		print_only = true;
		if ( strccmp(HomeName, TypedName(ap)) != STREQUAL )
			PrintType &= ~PRINT_HOME;
	}

	FreeAddr(&ap);

	if ( Aliases )
	{
		Aliases = false;
		Print_State = true;
		PrintType |= PRINT_ALIASES;
	}

	return ACCEPTARG;
}



AFuncv
getRoute(val, arg)
	PassVal		val;
	Pointer		arg;
{
	SetRoute(val.p);
	return ACCEPTARG;
}



AFuncv
getState(val, arg)
	PassVal		val;
	Pointer		arg;
{
	StateSet = true;
	StateDate = SetState(val.p);
	NoImport = true;
	return ACCEPTARG;
}



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
	case 'x':	ExpTypes = true;
	case 'i':	NoTypes = false;	break;
	default:	return english("unrecognised types option");
	}

	return ACCEPTARG;
}



void
set_map()
{
}



void
set_route()
{
	PrintType |= PRINT_ROUTE|PRINT_FASTEST;
	Print_State = true;
	Warnings = true;
	NoLinks = true;
	FreeStr(&OtherState);
}



/*
**	Read in state file and check it, or read route file,
**	print and/or update statefile, and/or write routefile.
*/

void
map()
{
	if ( Print_State || Print_Forced || Aliases )
	{
		FreeRoute();

		if
		(
			OtherState != NULLSTR
			&&
			!(StateSet || NoImport)
			&&
			(OtherState = FindState(OtherState)) != NULLSTR
		)
			StateDate = SetState(OtherState);

		if ( Warnings )
			Recover(ert_return);

		MakeTopRegion();
		InitTypeMap();

		if ( NoTypes )
			NoJmp = true;

		(void)ReadState(for_reading, IgnoreCRC);

		Recover(ert_finish);

		if ( HomeRegion == (Region *)0 )
			Error(english("Empty state file, or missing \"address\"."));

		if ( HomeRegion->rg_state == 0 && StateDate != 0 )
			HomeRegion->rg_state = StateDate;

		if ( Aliases )
			PrintAliases(stdout);

		if ( Print_Forced )
			PrintForced(stdout);

		if ( Print_State )
			PrintState(stdout, SetPrint(), PrintType);
	}

	if ( Print_Route )
		PrintRoute(stdout, Verbose);

	if ( Print_Maps )
		PrintMaps(stdout, Verbose);
}
