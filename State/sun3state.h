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
**	SUN III statefile data based on:-
**
**		state.h		1.19 87/05/21
**		statefile.h	1.8  86/01/06
*/

#define	SUN3INTDELAY	86400	/* Default delay for Sun3 intermittent link */
#define	OZCC		".0=au"	/* Country code for "oz" regions */

/*
**	Node/Link states
*/

typedef enum
{
	s3_down, s3_msg, s3_con, s3_lan,
	s3_conterminal, s3_call, s3_msgterminal, s3_foreign,
	s3_local, s3_found, s3_dead, s3_print,
	s3_filtered, s3_spare1, s3_intermittent, s3_spare2,
	s3_max /* 16 */
}
			S3state;

typedef Ushort		S3states;				/* Enough for ``s3_max'' bits */

#ifdef	DEBUG

char *	Flag3Names[] =
{
	"down", "msg", "con", "lan",
	"conterminal", "call", "msgterminal", "foreign",
	"local", "found", "dead", "print",
	"filtered", "spare1", "intermittent", "spare2"
};

#endif	/* DEBUG */

#define	S3_CALL		(S3states)(1L<<(int)s3_call)		/* Call on demand */
#define	S3_CON		(S3states)(1L<<(int)s3_con)		/* Link allows connection protocol */
#define	S3_CONTERMINAL	(S3states)(1L<<(int)s3_conterminal)	/* Virtual circuits may not be connected through */
#define	S3_DEAD		(S3states)(1L<<(int)s3_dead)		/* Link appears dead */
#define	S3_DOWN		(S3states)(1L<<(int)s3_down)		/* Link is down */
#define	S3_FILTERED	(S3states)(1L<<(int)s3_filtered)	/* A ``filter'' has been put on link */
#define	S3_FOREIGN	(S3states)(1L<<(int)s3_foreign)		/* Link uses foreign protocols - use special handlers */
#define	S3_INTERMITTENT	(S3states)(1L<<(int)s3_intermittent)	/* Intermittent link */
#define	S3_LAN		(S3states)(1L<<(int)s3_lan)		/* Link uses Local Area Network */
#define	S3_LOCAL	(S3states)(1L<<(int)s3_local)		/* Node is "local" - details not exported */
#define	S3_MSG		(S3states)(1L<<(int)s3_msg)		/* Link allows Message transfer */
#define	S3_MSGTERMINAL	(S3states)(1L<<(int)s3_msgterminal)	/* Messages may not be routed through */

#define	LINK_3_FLAGS	(S3_DOWN|S3_MSG|S3_CALL|S3_LAN|S3_LOCAL|S3_INTERMITTENT|S3_DEAD|S3_FILTERED)
#define	NODE_3_FLAGS	(S3_MSGTERMINAL|S3_FOREIGN|S3_LOCAL)
#define	FOREIGN_3_FLAGS	(S3_DOWN|S3_MSG|S3_MSGTERMINAL|S3_FOREIGN|S3_LOCAL|S3_INTERMITTENT|S3_DEAD|S3_FILTERED)
#define	EXTERNAL_3_FLAGS (LINK_3_FLAGS|NODE_3_FLAGS)

/*
**	The state file is an ascii file, except for the last line,
**	which contains the bytes:-
**		C3_EOF
**		LOCRC
**		HICRC
**		'\n'
**
**	The CRC checks all the bytes on the preceding lines.
*/

/*
**	Separators used between tokens in state file.
*/

#define	TOKEN_3_SIZE	127

typedef enum
{
	t3_start,
	t3_domhier, t3_domain, t3_othdom, t3_topdom,
	t3_alias, t3_aliasv, t3_xalias,
	t3_node, t3_nflags, t3_comment, t3_state, t3_nodehier,
	t3_connector, t3_spooler, t3_caller, t3_handler, t3_filter,
	t3_date, t3_sent, t3_recvd, t3_passto, t3_passfrom,
	t3_link, t3_lflags, t3_cost, t3_speed, t3_time, t3_bytes,
	t3_eof,
	t3_expect,
	t3_n,				/* [31] */
	t3_nextlink, t3_nextnode,
	t3_max,
	t3_null, t3_error
}
			T3token;

typedef long		T3tokens;	/* Enough bits for 1<<t3_n */

#ifdef	DEBUG

char *	Token3Names[] =
{
	"start",
	"domhier", "domain", "othdom", "topdom",
	"alias", "aliasv", "xalias",
	"node", "nflags", "comment", "state", "nodehier",
	"connector", "spooler", "caller", "handler", "filter",
	"date", "sent", "recvd", "passto", "passfrom",
	"link", "lflags", "cost", "speed", "time", "bytes",
	"eof",
	"expect",
	"n",
	"nextlink", "nextnode",
	"max",
	"null", "error"
};

#endif	/* DEBUG */

/*
**	Mask bits per token type.
*/

#define	T3_ALIAS	(T3tokens)(1L<<(int)t3_alias)
#define	T3_ALIASV	(T3tokens)(1L<<(int)t3_aliasv)
#define	T3_ANY		(T3tokens)(1L<<(int)t3_any)
#define	T3_BYTES	(T3tokens)(1L<<(int)t3_bytes)
#define	T3_CALLER	(T3tokens)(1L<<(int)t3_caller)
#define	T3_COMMENT	(T3tokens)(1L<<(int)t3_comment)
#define	T3_CONNECTOR	(T3tokens)(1L<<(int)t3_connector)
#define	T3_COST		(T3tokens)(1L<<(int)t3_cost)
#define	T3_DATE		(T3tokens)(1L<<(int)t3_date)
#define	T3_DOMAIN	(T3tokens)(1L<<(int)t3_domain)
#define	T3_DOMHIER	(T3tokens)(1L<<(int)t3_domhier)
#define	T3_EOF		(T3tokens)(1L<<(int)t3_eof)
#define	T3_EXPECT	(T3tokens)(1L<<(int)t3_expect)
#define	T3_FILTER	(T3tokens)(1L<<(int)t3_filter)
#define	T3_FOREIGN	(T3tokens)(1L<<(int)t3_foreign)
#define	T3_HANDLER	(T3tokens)(1L<<(int)t3_handler)
#define	T3_IGNORE	(T3tokens)(1L<<(int)t3_ignore)
#define	T3_LFLAGS	(T3tokens)(1L<<(int)t3_lflags)
#define	T3_LINK		(T3tokens)(1L<<(int)t3_link)
#define	T3_NFLAGS	(T3tokens)(1L<<(int)t3_nflags)
#define	T3_NODE		(T3tokens)(1L<<(int)t3_node)
#define	T3_NODEHIER	(T3tokens)(1L<<(int)t3_nodehier)
#define	T3_OTHDOM	(T3tokens)(1L<<(int)t3_othdom)
#define	T3_PASSFROM	(T3tokens)(1L<<(int)t3_passfrom)
#define	T3_PASSTO	(T3tokens)(1L<<(int)t3_passto)
#define	T3_RECVD	(T3tokens)(1L<<(int)t3_recvd)
#define	T3_SENT		(T3tokens)(1L<<(int)t3_sent)
#define	T3_SPEED	(T3tokens)(1L<<(int)t3_speed)
#define	T3_SPOOLER	(T3tokens)(1L<<(int)t3_spooler)
#define	T3_START	(T3tokens)(1L<<(int)t3_start)
#define	T3_STATE	(T3tokens)(1L<<(int)t3_state)
#define	T3_TIME		(T3tokens)(1L<<(int)t3_time)
#define	T3_TOPDOM	(T3tokens)(1L<<(int)t3_topdom)
#define	T3_XALIAS	(T3tokens)(1L<<(int)t3_xalias)

#define	F3_START	T3_DOMHIER|T3_EXPECT
#define	F3_DOMHIER	T3_DOMHIER|T3_XALIAS|T3_NODE
#define	F3_NODE		T3_DOMAIN|T3_NFLAGS|T3_COMMENT|T3_LINK|T3_NODE
#define	F3_NODE2	T3_DOMAIN|T3_NFLAGS|T3_NODE
#define	F3_DOMAIN	T3_NFLAGS|T3_COMMENT|T3_LINK|T3_NODE
#define	F3_DOMAIN2	T3_NFLAGS|T3_NODE
#define	F3_LINK1	T3_NODEHIER|T3_EXPECT
#define	F3_LINK2	T3_LFLAGS|T3_SPEED|T3_COST|T3_LINK|T3_NODE

/*
**	Token expectations of statefile order.
**
**	(1st column: accept first (source) node and its links
**	 2nd column: accept nodes only.)
**
**	Token expectations vector in same order as T3token.
*/

T3tokens	Accept3[(int)t3_max][2] =
{
	{F3_START,		F3_NODE2},	/* t3_start */
	{F3_DOMHIER,		F3_NODE2},	/* t3_domhier */
	{F3_DOMAIN,		F3_DOMAIN2},	/* t3_domain */
	{F3_NODE,		F3_NODE2},	/* t3_othdom */
	{F3_NODE,		F3_NODE2},	/* t3_topdom */
	{F3_NODE,		F3_NODE2},	/* t3_alias */
	{F3_NODE,		F3_NODE2},	/* t3_aliasv */
	{T3_NODE,		T3_NODE},	/* t3_xalias */
	{F3_NODE,		F3_NODE2},	/* t3_node */
	{F3_NODE,		F3_NODE2},	/* t3_nflags */
	{F3_NODE,		F3_NODE2},	/* t3_comment */
	{F3_NODE,		F3_NODE2},	/* t3_state */
	{F3_LINK2,		F3_NODE2},	/* t3_nodehier */
	{F3_NODE,		F3_NODE2},	/* t3_connector */
	{F3_NODE,		F3_NODE2},	/* t3_spooler */
	{F3_NODE,		F3_NODE2},	/* t3_caller */
	{F3_NODE,		F3_NODE2},	/* t3_handler */
	{F3_NODE,		F3_NODE2},	/* t3_filter */
	{F3_NODE,		F3_NODE2},	/* t3_date */
	{F3_NODE,		F3_NODE2},	/* t3_sent */
	{F3_NODE,		F3_NODE2},	/* t3_recvd */
	{F3_NODE,		F3_NODE2},	/* t3_passto */
	{F3_NODE,		F3_NODE2},	/* t3_passfrom */
	{F3_LINK1,		F3_NODE2},	/* t3_link */
	{F3_LINK2,		F3_NODE2},	/* t3_lflags */
	{F3_LINK2,		F3_NODE2},	/* t3_cost */
	{F3_LINK2,		F3_NODE2},	/* t3_speed */
	{F3_LINK2,		F3_NODE2},	/* t3_time */
	{F3_LINK2,		F3_NODE2},	/* t3_bytes */
	{0,			0},		/* t3_eof */
	{0,			0},		/* t3_expect */
	{0,			0},		/* t3_n */
	{T3_LINK|T3_NODE,	F3_NODE2},	/* t3_nextlink */
	{T3_NODE,		T3_NODE}	/* t3_nextnode */
};

/*
**	Actual statefile tokens.
*/

#define	C3_ALIAS	'~'
#define	C3_ALIASV	'^'
#define	C3_BYTES	'='
#define	C3_CALLER	'\\'
#define	C3_COMMENT	'"'
#define	C3_CONNECTOR	'&'
#define	C3_COST		'$'
#define	C3_DATE		'@'
#define	C3_DOMAIN	'|'
#define	C3_DOMHIER	';'
#define	C3_EOF		'#'
#define	C3_FILTER	'('
#define	C3_HANDLER	'%'
#define	C3_LFLAGS	'`'
#define	C3_LINK		','
#define	C3_NFLAGS	'\''
#define	C3_NODE		'\n'
#define	C3_NODEHIER	'+'
#define	C3_OTHDOM	']'
#define	C3_PASSFROM	'<'
#define	C3_PASSTO	'>'
#define	C3_RECVD	'{'
#define	C3_SENT		'}'
#define	C3_SPEED	'*'
#define	C3_SPOOLER	'!'
#define	C3_STATE	'?'
#define	C3_TIME		':'
#define	C3_TOPDOM	'['
#define	C3_XALIAS	'/'

#define	C3_SPARE2	')'
