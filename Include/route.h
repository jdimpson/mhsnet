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


/*
**	Network topology header.
*/

/*
**	Routing flags.
*/

enum
{
	rf_home, rf_link,
	rf_linkto, rf_linkfrom,
	rf_duproute, rf_foreign,
	rf_printroute, rf_print,
	rf_reglink, rf_pseudo,
	rf_remove, rf_new,
	rf_cheapest, rf_fastest,
	rf_mark, rf_alias,
	rf_newregion, rf_leaf,
	rf_force
	/* 19 */
};

typedef Ulong		RFlags;				/* Enough bits for RFlags */

#define	RF_ALIAS	(RFlags)(1L<<(int)rf_alias)	/* Region has aliases */
#define	RF_CHEAPEST	(RFlags)(1L<<(int)rf_cheapest)	/* Routing */
#define	RF_DUPROUTE	(RFlags)(1L<<(int)rf_duproute)	/* More than one equidistance route to this region */
#define	RF_FASTEST	(RFlags)(1L<<(int)rf_fastest)	/* Routing */
#define	RF_FORCE	(RFlags)(1L<<(int)rf_force)	/* Don't coalesce region */
#define	RF_FOREIGN	(RFlags)(1L<<(int)rf_foreign)	/* Region hasn't sent Sun4 statefile */
#define	RF_HOME		(RFlags)(1L<<(int)rf_home)	/* Home in this region */
#define	RF_LEAF		(RFlags)(1L<<(int)rf_leaf)	/* Region has one link */
#define	RF_LINK		(RFlags)(1L<<(int)rf_link)	/* Region linked to Home */
#define	RF_LINKFROM	(RFlags)(1L<<(int)rf_linkfrom)	/* Region has link from Home */
#define	RF_LINKTO	(RFlags)(1L<<(int)rf_linkto)	/* Region has link to Home */
#define	RF_MARK		(RFlags)(1L<<(int)rf_mark)	/* Mark region during searches */
#define	RF_NEW		(RFlags)(1L<<(int)rf_new)	/* New region needs defaults */
#define	RF_NEWREGION	(RFlags)(1L<<(int)rf_newregion)	/* New region added by command */
#define	RF_PSEUDO	(RFlags)(1L<<(int)rf_pseudo)	/* Region doesn't have real links */
#define	RF_PRINT	(RFlags)(1L<<(int)rf_print)	/* Region marked for printing */
#define	RF_PRINTROUTE	(RFlags)(1L<<(int)rf_printroute)/* Route marked for printing */
#define	RF_REGLINK	(RFlags)(1L<<(int)rf_reglink)	/* Set region reachable link table */
#define	RF_REMOVE	(RFlags)(1L<<(int)rf_remove)	/* Region marked for removal */

#define	RF_INTERNAL	(RF_ALIAS|RF_CHEAPEST|RF_DUPROUTE|RF_FASTEST|RF_FORCE|RF_HOME|RF_LEAF|RF_LINK|RF_LINKFROM|RF_LINKTO|RF_MARK|RF_NEW|RF_NEWREGION|RF_PRINT|RF_PRINTROUTE|RF_REGLINK|RF_REMOVE)
#define	RF_NOEXPORT	(RF_INTERNAL|RF_FOREIGN|RF_PSEUDO)
#define	RF_NOIMPORT	(RF_NOEXPORT)
#define	RF_RESTRICT	(RF_INTERNAL|RF_FOREIGN|RF_PSEUDO)
#define	RF_ROUTING	(RF_CHEAPEST|RF_FASTEST)

/*
**	Routing parameters -- DON'T CHANGE!
*/

#define	DOWN_DELAY	DAY			/* Cost of a link being ``down'' */
#define	DEAD_DELAY	(100*DOWN_DELAY)	/* Cost of a link being ``dead'' */
#define	DIV_DELAY_TO_COST	1		/* Delay to cost divider */
#define	MUL_DELAY_TO_COST	1		/* Delay to cost multiplier */

#define	MAX_COST	1000000			/* Maximum cost specifiable */
#define	MIN_COST	0
#define	MAX_DELAY	WEEK			/* Maximum delay specifiable */
#define	MIN_DELAY	64
#define	MUL_DELAY	8			/* To get integers */
#define	MAX_SPEED	100000000		/* Maximum speed specifiable */
#define	DFL_SPEED	120
#define	MAX_TRAFFIC	10000000		/* Maximum traffic specifiable */
#define	MIN_TRAFFIC	1
#define	MIN_WEIGHT	1			/* Minimum link ``distance'' */

/*
**	Hashing parameters.
*/

#ifndef	HASH_SIZE
#define	HASH_SIZE	256			/* 256 preferred for table lookup hash */
#endif

/*
**	Forward references.
*/

typedef struct AdvRoute	AdvRoute;
typedef struct Entry	Entry;
typedef struct FLink	FLink;
typedef struct NLink	NLink;
typedef struct Region	Region;

/*
**	A part of the address space.
*/

struct Region
{
	Region *rg_down;		/* Contained regions */
	Region *rg_up;			/* Containing region */
	Region *rg_next;		/* Region list at same level */
	Region *rg_visible;		/* Visible region */
	Region *rg_route[NROUTES];	/* For shortest paths */
	Region *rg_found[NROUTES];	/* For shortest paths */
	NLink *	rg_links;		/* Start of links list */
	NLink **rg_llink;		/* End of links list */
	char *	rg_name;		/* Typed name for region */
	Time_t	rg_state;		/* Date of last state message */
	Ulong	rg_dist[NROUTES];	/* Distance from Home used in routing calculation */
	Ulong	rg_index;		/* Position in sorted list */
	Ulong	rg_number;		/* Region allocated number */
	RFlags	rg_flags;		/* If RF_LINK, rg_down is (Link *) */
};

/*
**	Entry flags.
*/

enum
{
	ef_nowrite
};

typedef Ulong		EFlags;				/* Enough bits for EFlags */

#define	EF_NOWRITE	(EFlags)(1L<<(int)ef_nowrite)	/* Don't write to statefile */

/*
**	Type of an Entry.
*/

typedef union
{
	Region *et_region;
	Entry *	et_entry;
	char *	et_name;
	int	et_index;
}
			EntType;

/*
**	An entry in a hash table.
*/

struct Entry
{
	EntType	en_type;		/* Value of entry */
	Entry *	en_great;		/* Branches of binary tree */
	Entry *	en_less;
	char *	en_name;		/* Name of entry */
	EFlags	en_flags;		/* Entry flags */
};

#define	REGION	en_type.et_region
#define	MAP	en_type.et_name
#define	INDEX	en_type.et_index

/*
**	Link forwarding descriptor.
*/

struct FLink
{
	FLink *	fl_next;		/* Next in list */
	Region *fl_to;			/* Forwarded to */
};

/*
**	Link descriptor.
*/

struct NLink
{
	NLink *	nl_next;		/* Next in list */
	Region *nl_to;			/* Linked to */
	Region *nl_restrict;		/* Level of traffic allowed */
	Ulong	nl_cost;		/* Notional dollars per 100Mb. */
	Ulong	nl_delay;		/* Queueing delay in seconds */
#if	0
	Ulong	nl_speed;		/* Notional bytes/second */
	Ulong	nl_traffic;		/* Notional bytes/second */
	Ulong	nl_dist;		/* Combination of delay,speed,traffic */
#endif	/* 0 */
	Flags	nl_flags;		/* Down, dead, ... */
};

/*
**	Advisory routes for specific regions.
*/

struct AdvRoute
{
	AdvRoute *	ar_next;
	Region *	ar_region;
	Region *	ar_route[NROUTES];
};

#define	CLEAR_ROUTE	(Region *)0
#define	NULL_ROUTE	(Region *)1
#define	REMOVE_ROUTE	(Region *)2

/*
**	Name errors.
*/

typedef enum
{
	ne_ok, ne_illc, ne_null, ne_notype, ne_noname, ne_floor, ne_unktype, ne_illtype, ne_toolong
}
			NameErr;

extern char		Ne_floor[];
extern char		Ne_illc[];
extern char		Ne_illtype[];
extern char		Ne_noname[];
extern char		Ne_notype[];
extern char		Ne_null[];
extern char		Ne_toolong[];
extern char		Ne_unktype[];

/*
**	Data declarations.
*/

Extern AdvRoute *	AdvisoryRoutes;
Extern long		AllRegCount;
Extern char *		AllRegTable;
Extern Entry *		AdrMapHash[HASH_SIZE];
Extern Entry **		AdrMapList;
Extern Entry *		AliasHash[HASH_SIZE];
Extern Entry **		AliasList;
Extern Region *		ChangeRegion;		/* Largest region affected if ChangeState true */
Extern bool		Check_Conflict;		/* Conflicts in commands are errors if set */
Extern Entry *		DomMapHash[HASH_SIZE];
Extern Entry **		DomMapList;
Extern bool		ExpTypes;		/* Substitute exported types in region names */
Extern Entry *		FwdMapHash[HASH_SIZE];
Extern Entry **		FwdMapList;
Extern char *		HomeComment;
Extern Region *		HomeRegion;
Extern bool		IsCommand;		/* True when processing commands */
extern char		Legal_c[];
Extern bool		NewComment;
Extern bool		NoTypes;		/* Strip types from region names */
extern char *		OrderTypes[];
Extern Entry *		RegMapHash[HASH_SIZE];
Extern Entry **		RegMapList;
Extern Entry *		RegionHash[HASH_SIZE];
Extern Entry **		RegionList;
Extern bool		RegionsCoalesced;
Extern bool		RegionRemoved;
Extern long		RemovedRegions;
Extern char *		SourceComment;
Extern Region *		TopRegion;
Extern Entry *		TypMapHash[HASH_SIZE];
Extern Entry **		TypMapList;
Extern Region *		VisRegion;

/*
**	Function declarations.
*/

extern void		BreakLinks _FA_((Region *));
extern void		BreakPLinks _FA_((Region *));
extern char *		CanonicalName _FA_((char *, char *));
extern void		CheckComment _FA_((void));
extern void		CheckState _FA_((bool));
extern void		ClearRoute _FA_((Region *, RFlags));
extern bool		CoalesceRegion _FA_((Region *, Region *, Region *));
extern Region *		CommonRegion _FA_((Region *, Region *));
extern Ulong		CostLink _FA_((NLink *, int));
extern Entry *		Enter _FA_((char *, Entry **));
extern Entry *		EnterC _FA_((char *, Entry **));
extern Entry *		EnterRegion _FA_((char *, bool));
extern char *		FirstDomain _FA_((char *));
extern char *		FirstName _FA_((char *));
extern char *		FirstType _FA_((char *));
extern void		ForceRoutes _FA_((void));
extern int		HashName _FA_((Uchar *));
extern void		InitTypeMap _FA_((void));
extern bool		InRegion _FA_((Region *, Region *));
extern char *		InternalType _FA_((char *));
extern bool		IntTypeName _FA_((char *));
extern NLink *		IsLinked _FA_((Region *, Region *));
extern bool		LegalLink _FA_((Region *, Region *));
extern void		LegalString _FA_((char *));
extern Entry *		Lookup _FA_((char *, Entry **));
extern NLink *		MakeLink _FA_((Region *, Region *));
extern void		MakeTopRegion _FA_((void));
extern Entry *		MapDomain _FA_((char *, char *));
extern void		NewAdvRoutes _FA_((Region *, Region *, Region *));
extern void		NewFwd _FA_((Region *, Region *));
extern char *		ParseOrderTypes _FA_((char *));
extern void		Paths _FA_((Region *, RFlags, int));
extern void		PathRoutes _FA_((Region *, vFuncp, bool));
extern bool		RcheckAddress _FA_((Region *));
extern bool		Rcommands _FA_((FILE *, bool));
extern bool		RdStCommands _FA_((bool));
extern void		RemEntMap _FA_((char *, Entry **));
extern bool		RemoveRegion _FA_((Region *));
extern void		RmFwd _FA_((Region *, Region *));
extern void		SetChange _FA_((Region *));
extern void		SetCommands _FA_((char *));
extern bool		SetFwd _FA_((Region *, char *, int));
extern void		SetFwdBuf _FA_((int));
extern RFlags		SetPrint _FA_((void));
extern void		SortFwd _FA_((Region *));
extern void		SortLinks _FA_((Region *));
extern char *		StripCopy _FA_((char *, char *));
extern void		UnLink _FA_((Region *, Region *));
extern void		UnLinkP _FA_((Region *, NLink **));
extern Ulong		WriteRoute _FA_((void));

#undef	free
#define	free(A)
#define	LEGAL_C(C)	Legal_c[c]
#define	MaxRegion(A,B)	InRegion(B,A)?(A):(B)
#define	MinRegion(A,B)	InRegion(A,B)?(A):(B)
