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
**	Link flags.
**
**	(The order of these must be maintained.)
*/

enum
{
	fl_dead, fl_down,
	fl_filtered, fl_call,
	fl_foreign,			/* last exported flag == 4 (020) */
	fl_spare1, fl_spare2,
	fl_forward, fl_duproute,
	fl_nochange, fl_pseudo,		/* Last routing flag == 9 (01000) */
	fl_remove, fl_nolink,		/* 11, 12 */
	fl_newcost, fl_newflags,	/* Used during commands processing */
	fl_newlink, fl_newregion,	/* Used during commands processing */
	fl_newdelay,			/* Used during commands processing */
	fl_newspeed, fl_newtraffic,	/* [NOT YET] Used during commands processing */
	fl_home, fl_linkname,
	fl_mark,
	fl_n				/* 23 */
};

#define	MAXFLAGS	((int)fl_n)
typedef Ulong		Flags;				/* Enough bits for MAXFLAGS */
typedef Ushort		RtFlags;			/* Enough bits for routing flags */

#define	FL_CALL		(Flags)(1L<<(int)fl_call)	/* Call on demand */
#define	FL_DEAD		(Flags)(1L<<(int)fl_dead)	/* Link seems dead */
#define	FL_DOWN		(Flags)(1L<<(int)fl_down)	/* Link seems down */
#define	FL_DUPROUTE	(Flags)(1L<<(int)fl_duproute)	/* More than one equidistant route to this region */
#define	FL_FILTERED	(Flags)(1L<<(int)fl_filtered)	/* A ``filter'' has been put on link */
#define	FL_FOREIGN	(Flags)(1L<<(int)fl_foreign)	/* Foreign protocols on this link */
#define	FL_FORWARD	(Flags)(1L<<(int)fl_forward)	/* Address being forwarded on this link */
#define	FL_HOME		(Flags)(1L<<(int)fl_home)	/* Link to/from Home */
#define	FL_LINKNAME	(Flags)(1L<<(int)fl_linkname)	/* Local name for link */
#define	FL_MARK		(Flags)(1L<<(int)fl_mark)	/* Mark link (in Check()) */
#define	FL_NEWCOST	(Flags)(1L<<(int)fl_newcost)	/* New cost for link */
#define	FL_NEWDELAY	(Flags)(1L<<(int)fl_newdelay)	/* New call delay for link */
#define	FL_NEWFLAGS	(Flags)(1L<<(int)fl_newflags)	/* New flags for link */
#define	FL_NEWLINK	(Flags)(1L<<(int)fl_newlink)	/* Link made by command */
#define	FL_NEWREGION	(Flags)(1L<<(int)fl_newregion)	/* New region for link */
#define	FL_NEWTRAFFIC	(Flags)(1L<<(int)fl_newtraffic)	/* New traffic rate for link */
#define	FL_NEWSPEED	(Flags)(1L<<(int)fl_newspeed)	/* New speed for link */
#define	FL_NOCHANGE	(Flags)(1L<<(int)fl_nochange)	/* Don't signal status changes for this link */
#define	FL_NOLINK	(Flags)(1L<<(int)fl_nolink)	/* Other half of uni-directional link to Home */
#define	FL_PSEUDO	(Flags)(1L<<(int)fl_pseudo)	/* Pseudo-link for otherwise unreachable regions */
#define	FL_REMOVE	(Flags)(1L<<(int)fl_remove)	/* Remove this link */

#define	FL_INTERNAL	(FL_DUPROUTE|FL_FORWARD|FL_HOME|FL_LINKNAME|FL_MARK\
			|FL_NEWCOST|FL_NEWDELAY|FL_NEWFLAGS|FL_NEWLINK\
			|FL_NEWREGION|FL_NEWTRAFFIC|FL_NEWSPEED|FL_REMOVE)
#define	FL_NOEXPORT	(FL_INTERNAL|FL_NOCHANGE)
#define	FL_NOIMPORT	(FL_NOEXPORT)
#define	FL_RESTRICT	(FL_INTERNAL|FL_FILTERED|FL_NOLINK|FL_PSEUDO)
#define	FL_ROUTING	(FL_DEAD|FL_DOWN)

/*
**	Table for matching flag names.
*/

typedef struct
{
	char *	fl_name;
	Flags	fl_value;
}
		Flag;

#define	FLGZ	sizeof(Flag)

/*
**	english( Sort this array. )
*/

Extern Flag	SortedFlags[]
#ifdef	FLAG_DATA
=
{
	{english("call"),	FL_CALL},
	{english("dead"),	FL_DEAD},
	{english("down"),	FL_DOWN},
#	if	DEBUG
	{english("duproute"),	FL_DUPROUTE},
#	endif	/* DEBUG */
	{english("filtered"),	FL_FILTERED},
	{english("foreign"),	FL_FOREIGN},
#	if	DEBUG
	{english("home"),	FL_HOME},
	{english("linkname"),	FL_LINKNAME},
	{english("mark"),	FL_MARK},
	{english("newcost"),	FL_NEWCOST},
	{english("newdelay"),	FL_NEWDELAY},
	{english("newflags"),	FL_NEWFLAGS},
	{english("newlink"),	FL_NEWLINK},
	{english("newregion"),	FL_NEWREGION},
	{english("newspeed"),	FL_NEWSPEED},
	{english("newtraffic"),	FL_NEWTRAFFIC},
#	endif	/* DEBUG */
	{english("nochange"),	FL_NOCHANGE},
#	if	DEBUG
	{english("nolink"),	FL_NOLINK},
#	endif	/* DEBUG */
	{english("pseudo"),	FL_PSEUDO},
#	if	DEBUG
	{english("remove"),	FL_REMOVE}
#	endif	/* DEBUG */
}
#endif	/* FLAG_DATA */
;

#ifdef	FLAG_DATA
#define	NSFLGS	((sizeof SortedFlags)/FLGZ)
int		NSFlags		= NSFLGS;
#else	/* FLAG_DATA */
extern int	NSFlags;
#define	NSFLGS	NSFlags
#endif	/* FLAG_DATA */

/*
**	Define result of routing lookup.
*/

typedef struct
{
	char *	ln_name;	/* Local name of link */
	char *	ln_rname;	/* Real name of link */
	char *	ln_caller;	/* Virtual circuit call set-up program */
	char *	ln_filter;	/* Message filter program */
	char *	ln_spooler;	/* Message spooler program */
	char *	ln_region;	/* Name of region found */
	Ulong	ln_delay;	/* Nominal call delay */
	Ulong	ln_regindex;	/* Index of region found */
	Ulong	ln_index;	/* Link number */
	Flags	ln_flags;	/* Routing parameters etc. */
}
		Link;

/*
**	Routing types.
*/

enum
{
	fastest_route, cheapest_route,
	n_routes
};

#define	CHEAPEST	((int)cheapest_route)
#define	FASTEST		((int)fastest_route)
#define	NROUTES		((int)n_routes)

/*
**	Function declarations.
*/

extern int		PrintFlags _FA_((FILE *, Flags));
extern char *		StringFlags _FA_((Flags));
