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


#ifdef	ANSI_C

#ifndef	REGION

#ifdef	ExternU
#undef	Extern
#define	Extern	extern		/* Don't declare any data accidentally */
#endif	/* ExternU */

#include	"route.h"

#undef	free			/* Remove undesirable side-effect */

#ifdef	ExternU
#undef	Extern
#define	Extern
#endif	/* ExternU */

#endif	/* REGION */

#endif	/* ANSI_C */

/*
**	Files in STATEDIR.
*/

#define	ROUTEFILE	"routefile"

/*
**	Shape of routing file:
**
**	homeindex	index of our address in strings
**	visindex	index of visible address in strings
**	serialindex	index of serial number in strings
**	typecount	number of domains in local hierarchy
**	maxtypes	maximum hierachical domains
**	linkcount	number of links
**	typeentries	number of entries in typetables
**	regioncount	number of entries in regionvector
**	adrmapcount	number of mapped addresses
**	regmapcount	number of mapped regions
**	typmapcount	number of mapped types
**	maxlinklength	length of longest typed link name
**	maxstrlength	length of longest region name
**	typenamevector	one Index for maxtypes
**	typevector	one TypeTab for each local type
**	typetables	one table of TypeEnts for each TypeTab
**	linktable	one RLink for each link
**	regionvector	one RegionEnt for each region
**	namevector	one MapEnt for each region
**	fwdmapvector	one MapEnt for each forwarded region
**	adrmapvector	one MapEnt for each mapped address
**	regmapvector	one MapEnt for each mapped region
**	typmapvector	one MapEnt for each mapped type
**	regtable	one bit for each linkcount*regioncount
**	strings		all the strings from above
*/

/*
**	These must make {ri_count * sizeof(Index)} match alignment of CPU.
*/

enum
{
	ri_homeindex, ri_visindex,
	ri_serialindex, ri_fwdmapcount,
	ri_typecount, ri_maxtypes,
	ri_linkcount, ri_typeentries,
	ri_regioncount, ri_adrmapcount,
	ri_regmapcount, ri_typmapcount,
	ri_maxlinklength, ri_maxstrlength,
	ri_count
};

#define	ADRMAP_COUNT	((Index *)RouteBase)[(int)ri_adrmapcount]
#define	FWDMAP_COUNT	((Index *)RouteBase)[(int)ri_fwdmapcount]
#define	HOME_INDEX	((Index *)RouteBase)[(int)ri_homeindex]
#define	LINK_COUNT	((Index *)RouteBase)[(int)ri_linkcount]
#define	MAXLINKLENGTH	((Index *)RouteBase)[(int)ri_maxlinklength]
#define	MAXSTRLENGTH	((Index *)RouteBase)[(int)ri_maxstrlength]
#define	MAXTYPES	((Index *)RouteBase)[(int)ri_maxtypes]
#define	REGION_COUNT	((Index *)RouteBase)[(int)ri_regioncount]
#define	REGMAP_COUNT	((Index *)RouteBase)[(int)ri_regmapcount]
#define	SERIAL_INDEX	((Index *)RouteBase)[(int)ri_serialindex]
#define	TYPE_COUNT	((Index *)RouteBase)[(int)ri_typecount]
#define	TYPE_ENTRIES	((Index *)RouteBase)[(int)ri_typeentries]
#define	TYPMAP_COUNT	((Index *)RouteBase)[(int)ri_typmapcount]
#define	VIS_INDEX	((Index *)RouteBase)[(int)ri_visindex]

#define	ROUTE_HEADER_SIZE     (sizeof(Index)*(int)ri_count)

/*
**	Routing file structures.
*/

typedef Ulong		Index;

#define	NON_INDEX	MAX_ULONG	/* Change if typedef for Index changes */
#define	MAX_INDEX	(MAX_ULONG-1)	/* Change if typedef for Index changes */

typedef struct
{
	Index	tp_count;	/* Size of table */
	Index	tp_index;	/* To TypeTables */
	Index	tp_name;	/* To domain name */
	char	tp_type[2];	/* Canonical type */
}
			TypeTab;

typedef enum
{
	wh_home,		/* "index" is 0, address is "home" */
	wh_next,		/* "index" is 0, "home" in this region */
	wh_lixt,		/* "index" is to LinkTable, and try next type */
	wh_link,		/* "index" is to LinkTable, end of search */
	wh_map,			/* "index" is to Strings to map "name" */
	wh_none,		/* for no match type situations */
	wh_noroute		/* no routing tables */
}
			What;

typedef Uchar		What_t;	/* For fitting What into structures */

typedef struct
{
	Index	te_name;		/* Domain/Region name == &Strings[te_name] */
	Index	te_index;		/* To RegionVector entry if not map */
	char	te_type;		/* Type for name */
	What_t	te_what;
}
			TypeEnt;

typedef struct
{
	Index	re_name;		/* Domain/Region name == &Strings[re_name] */
	Index	re_route[NROUTES];	/* RLink entry */
	RtFlags	re_flags;		/* Domain flags */
	What_t	re_what;
}
			RegionEnt;

typedef struct
{
	Index	me_name;		/* Match name == &Strings[me_name] */
	Index	me_index;		/* Mapped name == &Strings[me_index] */
}					/* Or, to RegionVector entry */
			MapEnt;

typedef struct
{
	Index	rl_name;	/* Local name */
	Index	rl_rname;	/* Real name */
	Index	rl_spooler;	/* Spooler program */
	Index	rl_caller;	/* Virtual circuit call set-up program */
	Index	rl_filter;	/* Filter program */
	Flags	rl_flags;	/* Flags for link */
	Ulong	rl_delay;	/* Nominal call delay */
}
			RLink;

#ifdef	REGION
/*
**	Structures used to build TypeTables.
*/

typedef union
{
	Region *tt_region;	/* If what != map */
	char *	tt_map;		/* If what == map */
}
			TEType;

typedef struct TypeEl	TypeEl;

struct TypeEl
{
	TypeEl *tl_next;	/* When in list */
	TEType	tl_value;
	char *	tl_name;	/* Domain name */
	What	tl_what;
};

#define	TMAP	tl_value.tt_map
#define	TREGION	tl_value.tt_region

typedef struct
{
	TypeEl *tv_list;	/* List of mappings for this type */
	TypeEl *tv_sorted;	/* Vector of TypeEls for this type */
	char *	tv_name;	/* Home name in type */
	Index	tv_count;	/* Number of TypeEls in sorted list */
	char	tv_type[2];	/* TYPE */
}
			TypeVec;

/*
**	Data declarations.
*/

Extern TypeVec *	TypeTree;
Extern Index		MapEntries;

#endif	/* REGION */

/*
**	Routing table data:
*/

Extern Index		AdrMapCount;
Extern MapEnt	 *	AdrMapVector;
Extern Index		AliasCount;
Extern Index		FwdMapCount;
Extern MapEnt	 *	FwdMapVector;
Extern char *		HomeName;
Extern char *		LicenceNumber;
Extern Index		LinkCount;
Extern RLink *		LinkVector;
Extern Index		MaxLinkLength;
Extern Index		MaxStrLength;
Extern Index		MaxTypes;
Extern MapEnt	 *	NameVector;
Extern Index		RegionCount;
Extern RegionEnt *	RegionVector;
Extern Index		RegMapCount;
Extern MapEnt	 *	RegMapVector;
Extern char *		RegionTable;
Extern char *		RouteBase;
Extern char *		RouteFile;
Extern char *		Strings;
Extern Index		TypeCount;
Extern Index		TypeEntries;
Extern Index *		TypeNameVector;
Extern TypeTab *	TypeVector;
Extern TypeEnt *	TypeTables;
Extern Index		TypMapCount;
Extern MapEnt	 *	TypMapVector;
Extern char *		VisibleName;

Extern Index		Route_size;
Extern Index		Str_size;

/*
**	Function declarations.
*/

#define	SUMCHECK	true
#define	NOSUMCHECK	false

#ifdef	ANSI_C
#ifdef	ExternU
#undef	Extern
#define	Extern	extern		/* Don't declare any data accidentally */
#endif	/* ExternU */

#ifndef	DOMAIN_SEP
#include	"address.h"
#endif	/* DOMAIN_SEP */

#ifndef	CHEAPEST
#include	"link.h"
#endif	/* CHEAPEST */

typedef void	(*DR_fp)(Addr *, Addr *, Link *);
typedef void	(*DR_ep)(Addr *, Addr *);

#ifdef	ExternU
#undef	Extern
#define	Extern
#endif	/* ExternU */
#else	/* ANSI_C */
#define	DR_fp	vFuncp
#define	DR_ep	vFuncp
#endif	/* ANSI_C */

extern void		BuildTypeTree _FA_((void));
extern bool		CheckRoute _FA_((bool));
extern bool		DoRoute _FA_((Addr *, Addr **, int, Link *, DR_fp, DR_ep));
extern What		FindAddr _FA_((Addr *, Link *, int));
extern What		FindDest _FA_((Dest *, Link *, int));
extern bool		FindLink _FA_((char *, Link *));
extern void		FreeRoute _FA_((void));
extern void		GetLink _FA_((Link *, Index));
extern Index		MakeList _FA_((Entry ***, Entry **, Index));
extern void		MakeRoute _FA_((void));
extern Index		MakeTypeTree _FA_((TypeVec *, Region *));
extern void		PrintMaps _FA_((FILE *, bool));
extern void		PrintRoute _FA_((FILE *, bool));
extern bool		ReadRoute _FA_((bool));
extern void		SetLinkCount _FA_((void));
extern void		SetRoute _FA_((char *));
