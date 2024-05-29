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
**	Define format of message command files.
*/

#define	MINCOMZ		2		/* Shorter than this is suspicious */
#define	MAXCOMZ		3000		/* Longer than this is incredible */

/*
**	Command types.
*/

typedef enum
{
	string_type, number_type, error_type
}
			CmdC;

/*
**	Each command file contains sequences of 2 fields:
**	1.	type;
**	2.	value.
*/

typedef enum
{
	null_cmd,	/* Must be first */
	address_cmd,
	bounce_cmd,
	deleted_cmd,
	end_cmd,
	env_cmd,
	file_cmd,
	flags_cmd,
	linkname_cmd,
	pid_cmd,
	start_cmd,
	timeout_cmd,
	traveltime_cmd,
	unlink_cmd,
	uselink_cmd,
	useaddr_cmd,
	hdrlength_cmd,
	delenv_cmd,
	usesrce_cmd,
	linkdir_cmd,
	filter_cmd,
	last_cmd	/* Must be last */
}
			CmdT;

#ifndef	CMD_NAMES_DATA
extern char *		CmdfDescs[]
#else	/* CMD_NAMES_DATA */
char *			CmdfDescs[]
=
{
	"null",
	"address",
	"bounce",
	"deleted",
	"end",
	"environment",
	"file",
	"flags",
	"link_name",
	"pid",
	"start",
	"timeout",
	"travel_time",
	"unlink",
	"use_link",
	"use_addr",
	"hdr_length",
	"delete_env",
	"use_source",
	"link_dir",
	"filter",
	"LAST"
}
#endif	/* CMD_NAMES_DATA */
;

#define	MAX_CMD_NAMES_LENGTH	11	/* Widest name */


Extern CmdC		CmdfTypes[]
#ifdef	CMDF_DATA
=
{
	error_type,
	string_type,	/* address */
	string_type,	/* bounce */
	number_type,	/* deleted */
	number_type,	/* end */
	string_type,	/* env */
	string_type,	/* file */
	number_type,	/* flags */
	string_type,	/* linkname */
	number_type,	/* pid */
	number_type,	/* start */
	number_type,	/* timeout */
	number_type,	/* traveltime */
	string_type,	/* unlink */
	string_type,	/* uselink */
	string_type,	/* useaddr */
	number_type,	/* hdrlength */
	string_type,	/* delenv */
	string_type,	/* usesrce */
	string_type,	/* linkdir */
	string_type,	/* filter */
	error_type
}
#endif
;

/*
**	Flags that can be passed.
*/

enum
{
	mesg_dup_flag, re_route_flag, out_filtered_flag, nak_on_timeout_flag, bounce_flag,
	link_up_flag, link_down_flag, retry_flag, in_filtered_flag, delivered_flag, low_space_flag
};

#ifndef	CMD_FLAGS_DATA
extern char *	CmdFlgDescs[];
#else	/* CMD_FLAGS_DATA */
char *		CmdFlgDescs[] =
{
	"MESG_DUP", "RE_ROUTE", "OUT_FILTERED", "NAK_ON_TIMEOUT", "BOUNCE_MESG",
	"LINK_UP", "LINK_DOWN", "RETRY", "IN_FILTERED", "DELIVERED", "LOW_SPACE",
	NULLSTR
};
#endif	/* CMD_FLAGS_DATA */

#define	BOUNCE_MESG	(1<<(int)bounce_flag)
#define	DELIVERED	(1<<(int)delivered_flag)
#define	IN_FILTERED	(1<<(int)in_filtered_flag)
#define	LINK_DOWN	(1<<(int)link_down_flag)
#define	LINK_UP		(1<<(int)link_up_flag)
#define	LOW_SPACE	(1<<(int)low_space_flag)
#define	MESG_DUP	(1<<(int)mesg_dup_flag)
#define	NAK_ON_TIMEOUT	(1<<(int)nak_on_timeout_flag)
#define	OUT_FILTERED	(1<<(int)out_filtered_flag)
#define	RE_ROUTE	(1<<(int)re_route_flag)
#define	RETRY		(1<<(int)retry_flag)

/*
**	Command structures.
*/

typedef struct Cmd	Cmd;

typedef struct
{
	Cmd *	ch_first;
	Cmd **	ch_last;
	int	ch_count;
}
			CmdHead;

struct Cmd
{
	Cmd *	cd_next;
	char *	cd_value;
	Ulong	cd_number;
	char	cd_string[ULONG_SIZE];
	CmdT	cd_type;
};

typedef union
{
	char *	cv_name;
	Ulong	cv_number;
}
			CmdV;

/*
**	Cache for free'd Cmds.
*/

Extern Cmd *		CmdFreelist;

/*
**	Common data.
*/

extern Ulong		PartsLength;		/* Total size of parts from AllParts() */
extern vFuncp		NoLinkFunc;		/* Called from LinkCmds if link fails */

/*
**	Command manipulating routines:
*/

extern char *		AddCmd _FA_((CmdHead *, CmdT, CmdV));
extern bool		AllParts _FA_((CmdHead *, Ulong, Ulong, Ulong, CmdHead *, char *, char *));
extern bool		ChangeCmd _FA_((Cmd **, CmdT, CmdV, CmdV));
extern bool		CheckData _FA_((CmdHead *, Ulong, Ulong, Crc_t *));
extern char *		CmdToString _FA_((CmdT, Ulong));
extern bool		CollectData _FA_((CmdHead *, Ulong, Ulong, int, char *));
extern Ulong		ConvSun3Cmds _FA_((char *, CmdHead *));
extern void		CopyCmds _FA_((CmdHead *, CmdHead *));
extern void		FreeCmds _FA_((CmdHead *));
extern void		FreeFileCmds _FA_((CmdHead *));
extern void		InitCmds _FA_((CmdHead *));
extern void		LinkCmds _FA_((CmdHead *, CmdHead *, Ulong, Ulong, char *, Time_t));
extern void		MoveCmds _FA_((CmdHead *, CmdHead *));
extern bool		ReadCmds _FA_((char *, bool(*)(CmdT, CmdV)));
extern bool		ReadCmdsFd _FA_((int, bool(*)(CmdT, CmdV)));
extern bool		ReadFdCmds _FA_((FILE *, bool(*)(CmdT, CmdV)));
extern bool		WriteCmds _FA_((CmdHead *, int, char *));
