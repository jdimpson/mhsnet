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
**	Commands for manipulating statefile.
**
**	All `address's and `region's are typed addresses.
*/

typedef enum
{
/* 0*/	ct_add,		/* <address>			(add a new address) */
	ct_address,	/* <address>			(give address for home) */
	ct_break,	/* <addr1>,<addr2>		(remove both links) */
	ct_breakall,	/* <address>			(break all links from/to this address) */
	ct_breakpseudo,	/* [<address>]			(break pseudo links [from this address]) */
	ct_caller,	/* <address> [<caller>]		(set caller for link) */
	ct_cheaproute,	/* <address> <region>		(force all region's cheap routes via this link) */
	ct_clear,	/* <address>			(clear state date for this address) */
	ct_comment,	/* [<Remarks>]			(site info.) */
	ct_contact,	/* [<Contact person>]		(site info.) */
/*10*/	ct_cost,	/* <addr1>,<addr2> <cost> 	(set cost of link) */
	ct_delay,	/* <addr1>,<addr2> <delay> 	(set call delay of link) */
	ct_edited,	/* [<Entry last edited>]	(site info.) */
	ct_email,	/* [<Contact email addr>]	(site info.) */
	ct_fastroute,	/* <address> <region>		(force all region's fast routes via this link) */
	ct_filter,	/* <address> [<filter>]		(set filter for link) */
	ct_flag,	/* <addr1>,<addr2> {+|-}<flag>[,...]	(add|remove flags for link to addr2) */
	ct_forward,	/* <fwd_address> [<to_address>]	(give forwarding address) */
	ct_halfbreak,	/* <addr1>,<addr2>		(remove link) */
	ct_halflink,	/* <addr1>,<addr2> [flag[,...]]	(make link between addr1 and addr2) */
/*20*/	ct_ialias,	/* <alias_address> [source]	(import alias to source) */
	ct_link,	/* <addr1>,<addr2> [flag[,...]]	(make both links between addr1 and addr2) */
	ct_linkname,	/* <address> [<name>]		(give local name for link) */
	ct_location,	/* [<Location>]			(site info.) */
	ct_map,		/* <map_address> [real_address]	(map pseudo-address to real address) */
			/* <map_typename> [real_type]	(map type) */
	ct_maptype,	/* <map_typename> [real_typename]	(map typename) */
	ct_noroute,	/* <address>			(clear routes for this region) */
	ct_ordertypes,	/* <type;type;type;type...;type>(give ordering for preferred type names) */
	ct_org,		/* [<Organization>]		(site info.) */
	ct_post,	/* [<Postal address>]		(site info.) */
/*30*/	ct_remove,	/* <address>			(remove address) */
	ct_removemaps,	/* [<map>]			(remove maps [matching <map>]) */
	ct_restrict,	/* <addr1>,<addr2> [<region>]	(restrict level of link) */
	ct_route,	/* <address> [<region>]		(force all region's routes via this link) */
	ct_licence,	/* [<Licence Number>]		(site info.) */
/*	ct_speed,   */	/* <addr1>,<addr2> <speed> 	(set speed of link) */
	ct_spooler,	/* <address> [<spooler>]	(set spooler for link) */
	ct_systype,	/* [<System Type>]		(site info.) */
	ct_telno,	/* [<Contact phone no>]		(site info.) */
/*	ct_traffic, */	/* <addr1>,<addr2> <traffic> 	(set traffic rate for link) */
	ct_visible,	/* <address> [<region>]		(name region in which address is visible) */
/*39*/	ct_xalias	/* <alias_address> [remove]	(export alias for home) */
}
			CmdType;

/*
**	Characters to delimit command line fields.
*/

#define	COMMENT_C	'#'		/* Char to delimit start of comment */
#define	FLAG_C		','		/* Char to delimit flags */
#define	LINK_C		','		/* Char to delimit link from node */
#define	ORDER_C		';'		/* Char to delimit ordered types */
#define	QUOTE_C		'!'		/* Char to start quoted string */

/*
**	Command fields.
*/

typedef enum
{
	cf_command, cf_link, cf_optlink, cf_params, cf_optparams
}
			CmdFlag;

#define	CF_COMMAND	(1<<(int)cf_command)
#define	CF_LINK		(1<<(int)cf_link)
#define	CF_OPTLINK	(1<<(int)cf_optlink)
#define	CF_PARAMS	(1<<(int)cf_params)
#define	CF_OPTPARAMS	(1<<(int)cf_optparams)

#ifdef	COMMAND_DATA
/*
**	Table to match commands.
*/

typedef struct
{
	char *	cd_name;
	CmdType	cd_type;
	int	cd_flags;
}
			Command;

#define	CMDZ		sizeof(Command)

typedef Command *	Cmdp;

/*
**	english( Sort this array, and change commands in programs. )
*/

static Command		Cmds[] =
{
	{english("add"),		ct_add,		0},
	{english("address"),		ct_address,	0},
	{english("break"),		ct_break,	CF_LINK},
	{english("breakall"),		ct_breakall,	0},
	{english("breakpseudo"),	ct_breakpseudo,	0},
	{english("caller"),		ct_caller,	CF_OPTPARAMS},
	{english("cheaproute"),		ct_cheaproute,	CF_PARAMS},
	{english("clear"),		ct_clear,	0},
	{english("comment"),		ct_comment,	CF_OPTPARAMS},
	{english("cost"),		ct_cost,	CF_LINK|CF_PARAMS},
	{english("delay"),		ct_delay,	CF_LINK|CF_PARAMS},
	{english("email address"),	ct_email,	0},
	{english("email_address"),	ct_email,	0},
	{english("fastroute"),		ct_fastroute,	CF_PARAMS},
	{english("filter"),		ct_filter,	CF_OPTPARAMS},
	{english("flag"),		ct_flag,	CF_LINK|CF_PARAMS},
	{english("forward"),		ct_forward,	CF_OPTPARAMS},
	{english("halfbreak"),		ct_halfbreak,	CF_LINK},
	{english("halflink"),		ct_halflink,	CF_LINK|CF_OPTPARAMS},
	{english("ialias"),		ct_ialias,	CF_OPTPARAMS},
	{english("licence number"),	ct_licence,	0},
	{english("licence_number"),	ct_licence,	0},
	{english("link"),		ct_link,	CF_LINK|CF_OPTPARAMS},
	{english("linkname"),		ct_linkname,	CF_OPTPARAMS},
	{english("location"),		ct_location,	0},
	{english("map"),		ct_map,		CF_OPTPARAMS},
	{english("noroute"),		ct_noroute,	0},
	{english("ordertypes"),		ct_ordertypes,	0},
	{english("organisation"),	ct_org,		0},
	{english("organization"),	ct_org,		0},
	{english("person"),		ct_contact,	0},
	{english("postal address"),	ct_post,	0},
	{english("postal_address"),	ct_post,	0},
	{english("remarks"),		ct_comment,	0},
	{english("remove"),		ct_remove,	0},
	{english("removemaps"),		ct_removemaps,	0},
	{english("restrict"),		ct_restrict,	CF_LINK|CF_OPTPARAMS},
	{english("route"),		ct_route,	CF_OPTPARAMS},
	{english("serial number"),	ct_licence,	0},
	{english("serial_number"),	ct_licence,	0},
/*	{english("speed"),		ct_speed,	CF_LINK|CF_PARAMS},	*/
	{english("spooler"),		ct_spooler,	CF_OPTPARAMS},
	{english("system"),		ct_systype,	0},
	{english("telno"),		ct_telno,	0},
/*	{english("traffic"),		ct_traffic,	CF_LINK|CF_PARAMS},	*/
	{english("unlink"),		ct_break,	CF_LINK},
	{english("unlinkall"),		ct_breakall,	0},
	{english("visible"),		ct_visible,	CF_OPTPARAMS},
	{english("xalias"),		ct_xalias,	CF_OPTPARAMS},
};

#define	NCMDS		((sizeof Cmds)/CMDZ)

#endif	/* COMMAND_DATA */

/*
**	Table for real names of site info commands.
**	(In order of inclusion in comment field.)
*/

enum
{
	si_org, si_post, si_telno, si_contact, si_email, si_systype,
	si_comment, si_location, si_licence, si_edited
};

#undef	si_value	/* Defined in <sys/siginfo.h>!! */

typedef struct
{
	char *	si_name;
	CmdType	si_type;
	int	si_mandatory;
	int	si_namelength;
	char *	si_value;
	int	si_length;
}
			SiteInfo;

#ifndef	COMMENT_DATA
extern char		StInfoSep[];
extern
#endif	/* COMMENT_DATA */
SiteInfo		SiteInfoNames[]
#ifndef	COMMENT_DATA
;
#else	/* COMMENT_DATA */
=
{
	{english("Organization"),		ct_org,		1},
	{english("Postal address"),		ct_post,	1},
	{english("Phone number"),		ct_telno,	1},
	{english("Contact person"),		ct_contact,	1},
	{english("Email address"),		ct_email,	1},
	{english("System type"),		ct_systype,	0},
	{english("Remarks"),			ct_comment,	0},
	{english("Location"),			ct_location,	0},
	{english("Licence number"),		ct_licence,	CHECK_LICENCE},
	{english("Last edited"),		ct_edited,	0}	/* Must be last */
};

#define	NSINS		((sizeof SiteInfoNames)/sizeof(SiteInfo))

/*
**	String to separate comment record identifier from value.
*/

char			StInfoSep[]	= ":\t";
int			LenInfoSep	= 2;
#endif	/* COMMENT_DATA */

/*
**	Value returned by Pcommand().
*/

typedef enum { pr_ok, pr_warn, pr_fwarn, pr_error, pr_exit } PrType;

/*
**	Externals.
*/

extern PrType		Pcommand _FA_((CmdType, Entry *, Entry *, NLink *, char *, char **));
extern void		SplitComment _FA_((void));
