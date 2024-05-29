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
**	Files in STATEDIR.
*/

#define	COMMANDSFILE	"commandsfile"
#define	ERRORSDIR	"NOTES/"
#define	EXPORTFILE	"exportfile"
#define	MSGSDIR		"MSGS/"
#define	SITEFILE	"sitefile"
#define	STATEFILE	"statefile"

/*
**	The state file is a sequence of bytes terminated by:-
**		C_EOF
**		LOCRC
**		HICRC
**
**	The CRC checks all the preceding bytes.
*/

#define	TOKEN_C	0200	/* All statefile token bytes have top bit on */

/*
**	Separators used between tokens in state file.
**
**	(Order used in Accept[][] below.)
*/

#define	TOKEN_SIZE	1022

typedef enum
{
	t_eof, t_ordertypes,				/* [000], [001] */
	t_address, t_oldaddress, t_comment,		/* [002], [003], [004] */
	t_region, t_visible, t_rflags, t_date, t_expect, t_forward, /* [005] - [012] */
	t_spooler, t_caller, t_filter, t_linkname,	/* [013], [014], [015], [016] */
	t_link, t_lflags, t_cost, t_delay, t_speed, t_traffic, t_restrict,	/* ..[025] */
	t_mapadr, t_mapreg, t_maptyp, t_mapval, t_xalias, t_ialias,	/* ..[033] */
	t_fastroute, t_cheaproute,			/* [034], [035] */
	t_stringsize, t_string,				/* [036], [037] */
	t_n,					/* [32] */
	t_nextregion, t_nextlink,
	t_max,
	t_error, t_null
}
			Token;

typedef Ulong		Tokens;			/* Enough bits for 1<<(t_n-1) */

#ifdef	TOKEN_DATA
#ifdef	DEBUG
char *	TokenNames[] =
{
	"eof", "ordertypes",
	"address", "oldaddress", "comment",
	"region", "visible", "rflags", "date", "expect", "forward",
	"spooler", "caller", "filter", "linkname",
	"link", "lflags", "cost", "delay", "speed", "traffic", "restrict",
	"mapadr", "mapreg", "maptyp", "mapval", "xalias", "ialias",
	"fastadvroute", "cheapadvroute",
	"stringsize", "string",
	"n",
	"nextregion", "nextlink",
	"max",
	"*error*", NullStr
};
#endif	/* DEBUG */
#endif	/* TOKEN_DATA */

#define	C_ADDRESS	(int)t_address
#define	C_CALLER	(int)t_caller
#define	C_CHEAPROUTE	(int)t_cheaproute
#define	C_COMMENT	(int)t_comment
#define	C_COST		(int)t_cost
#define	C_DATE		(int)t_date
#define	C_DELAY		(int)t_delay
#define	C_EOF		(int)t_eof
#define	C_FASTROUTE	(int)t_fastroute
#define	C_FILTER	(int)t_filter
#define	C_FORWARD	(int)t_forward
#define	C_IALIAS	(int)t_ialias
#define	C_LFLAGS	(int)t_lflags
#define	C_LINK		(int)t_link
#define	C_LINKNAME	(int)t_linkname
#define	C_MAPADR	(int)t_mapadr
#define	C_MAPREG	(int)t_mapreg
#define	C_MAPTYP	(int)t_maptyp
#define	C_MAPVAL	(int)t_mapval
#define	C_N		(int)t_n
#define	C_OLDADDRESS	(int)t_oldaddress
#define	C_ORDERTYPES	(int)t_ordertypes
#define	C_REGION	(int)t_region
#define	C_RESTRICT	(int)t_restrict
#define	C_RFLAGS	(int)t_rflags
#define	C_SPEED		(int)t_speed
#define	C_SPOOLER	(int)t_spooler
#define	C_TRAFFIC	(int)t_traffic
#define	C_VISIBLE	(int)t_visible
#define	C_XALIAS	(int)t_xalias

#define	CHECKTOKEN()	{if(C_N>BITS_ULONG)Fatal1("Too many Tokens");}

#ifdef	TOKEN_DATA

#define	T_ADDRESS	(Tokens)(1L<<(int)t_address)
#define	T_CALLER	(Tokens)(1L<<(int)t_caller)
#define	T_CHEAPROUTE	(Tokens)(1L<<(int)t_cheaproute)
#define	T_COMMENT	(Tokens)(1L<<(int)t_comment)
#define	T_COST		(Tokens)(1L<<(int)t_cost)
#define	T_DATE		(Tokens)(1L<<(int)t_date)
#define	T_DELAY		(Tokens)(1L<<(int)t_delay)
#define	T_EOF		(Tokens)(1L<<(int)t_eof)
#define	T_EXPECT	(Tokens)(1L<<(int)t_expect)
#define	T_FASTROUTE	(Tokens)(1L<<(int)t_fastroute)
#define	T_FILTER	(Tokens)(1L<<(int)t_filter)
#define	T_FORWARD	(Tokens)(1L<<(int)t_forward)
#define	T_IALIAS	(Tokens)(1L<<(int)t_ialias)
#define	T_LFLAGS	(Tokens)(1L<<(int)t_lflags)
#define	T_LINK		(Tokens)(1L<<(int)t_link)
#define	T_LINKNAME	(Tokens)(1L<<(int)t_linkname)
#define	T_MAPADR	(Tokens)(1L<<(int)t_mapadr)
#define	T_MAPREG	(Tokens)(1L<<(int)t_mapreg)
#define	T_MAPTYP	(Tokens)(1L<<(int)t_maptyp)
#define	T_MAPV		(Tokens)(1L<<(int)t_mapval)
#define	T_OLDADDRESS	(Tokens)(1L<<(int)t_oldaddress)
#define	T_ORDERTYPES	(Tokens)(1L<<(int)t_ordertypes)
#define	T_REGION	(Tokens)(1L<<(int)t_region)
#define	T_RESTRICT	(Tokens)(1L<<(int)t_restrict)
#define	T_RFLAGS	(Tokens)(1L<<(int)t_rflags)
#define	T_SPEED		(Tokens)(1L<<(int)t_speed)
#define	T_SPOOLER	(Tokens)(1L<<(int)t_spooler)
#define	T_STRING	(Tokens)((Ulong)1L<<(int)t_string)
#define	T_STRINGSIZE	(Tokens)(1L<<(int)t_stringsize)
#define	T_TRAFFIC	(Tokens)(1L<<(int)t_traffic)
#define	T_VISIBLE	(Tokens)(1L<<(int)t_visible)
#define	T_XALIAS	(Tokens)(1L<<(int)t_xalias)

#define	F_EOF		(T_ADDRESS|T_ORDERTYPES|T_STRINGSIZE)
#define	F_STRINGSIZE	(T_ADDRESS|T_ORDERTYPES|T_STRING)
#define	F_ADDRESS	(T_COMMENT|T_OLDADDRESS|T_REGION)
#define	F_OLDADDRESS	(T_COMMENT|T_REGION)
#define	F_COMMENT	(T_REGION)
#define	F_MAP		(T_XALIAS)
#define	F_LINK		(F_MAP|T_COST|T_DELAY|T_LFLAGS|T_LINK|T_REGION|T_RESTRICT|T_SPEED|T_TRAFFIC)
#define	F_REGION	(F_MAP|T_LINK|T_REGION|T_VISIBLE)
#define	F_NEXTREGION	(F_MAP|T_REGION)
#define	F_NEXTLINK	(F_NEXTREGION|T_LINK)

#define	L_EOF		(F_EOF)
#define	L_STRINGSIZE	(F_STRINGSIZE)
#define	L_ADDRESS	(F_ADDRESS)
#define	L_OLDADDRESS	(F_OLDADDRESS)
#define	L_COMMENT	(T_REGION)
#define	L_MAP		(F_MAP|T_CHEAPROUTE|T_FASTROUTE|T_FORWARD|T_MAPADR|T_MAPREG|T_MAPTYP|T_IALIAS)
#define	L_LINK		(L_MAP|F_LINK)
#define	L_REGION	(L_MAP|F_REGION|T_CALLER|T_DATE|T_FILTER|T_LINKNAME|T_RFLAGS|T_SPOOLER)
#define	L_NEXTREGION	(L_MAP|T_REGION)
#define	L_NEXTLINK	(L_NEXTREGION|T_LINK)

/*
**	Token expectations (in same order as Token.)
**
**	(local)		(foreign)
*/

Tokens			Accept[(int)t_max][2] = 
{
	{L_EOF,			F_EOF},			/* t_eof */
	{T_ADDRESS,		T_ADDRESS},		/* t_ordertypes */
	{L_ADDRESS,		F_ADDRESS},		/* t_address */
	{L_OLDADDRESS,		F_OLDADDRESS},		/* t_oldaddress */
	{L_COMMENT,		F_COMMENT},		/* t_comment */
	{L_REGION,		F_REGION},		/* t_region */
	{L_REGION,		F_REGION},		/* t_visible */
	{L_REGION,		F_REGION},		/* t_rflags */
	{L_REGION,		F_REGION},		/* t_date */
	{0,			0},			/* t_expect */
	{T_MAPV|T_EXPECT,	T_MAPV|T_EXPECT},	/* t_forward */
	{L_REGION,		F_REGION},		/* t_spooler */
	{L_REGION,		F_REGION},		/* t_caller */
	{L_REGION,		F_REGION},		/* t_filter */
	{L_REGION,		F_REGION},		/* t_linkname */
	{L_LINK,		F_LINK},		/* t_link */
	{L_LINK,		F_LINK},		/* t_lflags */
	{L_LINK,		F_LINK},		/* t_cost */
	{L_LINK,		F_LINK},		/* t_delay */
	{L_LINK,		F_LINK},		/* t_speed */
	{L_LINK,		F_LINK},		/* t_traffic */
	{L_LINK,		F_LINK},		/* t_restrict */
	{T_MAPV|T_EXPECT,	T_MAPV|T_EXPECT},	/* t_mapadr */
	{T_MAPV|T_EXPECT,	T_MAPV|T_EXPECT},	/* t_mapreg */
	{T_MAPV|T_EXPECT,	T_MAPV|T_EXPECT},	/* t_maptyp */
	{L_MAP|T_REGION,	F_MAP},			/* t_mapval */
	{L_MAP,			F_MAP},			/* t_xalias */
	{T_MAPV|T_EXPECT,	T_MAPV|T_EXPECT},	/* t_ialias */
	{T_MAPV|T_EXPECT,	T_MAPV|T_EXPECT},	/* t_fastroute */
	{T_MAPV|T_EXPECT,	T_MAPV|T_EXPECT},	/* t_cheaproute */
	{L_STRINGSIZE,		F_STRINGSIZE},		/* t_stringsize */
	{L_STRINGSIZE,		F_STRINGSIZE},		/* t_string */
	{0,			0},			/* t_n */
	{L_NEXTREGION,		F_NEXTREGION},		/* t_nextregion */
	{L_NEXTLINK,		F_NEXTLINK},		/* t_nextlink */
};
#endif	/* TOKEN_DATA */

/*
**	State export type.
*/

typedef enum { local_state, export_state, site_state } State_t;

/*
**	State print options.
*/

enum
{
	verbose_print, links_print, route_print,
	fastest_print, cheapest_print,
	all_print, dsort_print, home_print, aliases_print
};

#define	PRINT_ALIASES	(1<<(int)aliases_print)
#define	PRINT_ALL	(1<<(int)all_print)
#define	PRINT_CHEAPEST	(1<<(int)fastest_print)
#define	PRINT_DSORT	(1<<(int)dsort_print)
#define	PRINT_FASTEST	(1<<(int)cheapest_print)
#define	PRINT_HOME	(1<<(int)home_print)
#define	PRINT_LINKS	(1<<(int)links_print)
#define	PRINT_ROUTE	(1<<(int)route_print)
#define	PRINT_VERBOSE	(1<<(int)verbose_print)

/*
**	Miscellaneous variables.
*/

#ifndef	DEFUNCT_TIME
#define	DEFUNCT_TIME	WEEK			/* Statefiles time out after 1 week */
#endif

#define	WORLD_NAME	"0"			/* Global address */
#define	STATE_ENQ_ALL	"ALLSTATE"		/* HdrEnv flag for ``enq_all'' request */

Extern bool		AnteState;		/* True if state info. is out-of-date */
extern char		Buf[];			/* Buffer used for Read/Write State */
extern char *		BufEnd;			/* End of Buf */
Extern bool		ChangeState;		/* True if vital info. about Home has changed and should be propagated */
Extern char *		OldAddress;		/* Non-null if active */
Extern char *		OldName;		/* Non-null if address changed */
Extern char *		OldVisName;		/* Non-null if address changed */

/*
**	Externals.
*/

extern char *		FindState _FA_((char *));
#ifdef	STDIO
extern void		NewState _FA_((FILE *, bool));
extern void		PrintAliases _FA_((FILE *));
extern void		PrintForced _FA_((FILE *));
extern void		PrintOnly _FA_((char *));
#ifdef	RF_ALIAS
extern void		PrintState _FA_((FILE *, RFlags, int));
#endif	/* RF_ALIAS */
extern void		R3state _FA_((FILE *, bool, char *, Time_t, bool));
extern FILE *		ReadState _FA_((Lock_t, bool));
extern void		Rstate _FA_((FILE *, bool, char *, Time_t, bool));
extern Time_t		SetState _FA_((char *));
extern int		WriteOrderTypes _FA_((char *));
extern void		Write_State _FA_((FILE *, State_t));
#endif	/* STDIO */
