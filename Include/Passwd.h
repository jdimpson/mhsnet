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


#ifndef	P_BROADCAST

/*
**	Network validation flags from "passwd" file.
*/

enum
{
	Pf_broadcast,		/* Addressing types allowed */
	Pf_multicast,
	Pf_explicit,
	Pf_world,
	Pf_otherhandlers,	/* Unusual handler requests allowed */
	Pf_cansend,		/* Message sending allowed */
	Pf_canrecv,		/* Message reception allowed */
	Pf_canexec,		/* Remote exec allowed */
	Pf_su			/* Network super-user */
};

#define	P_BROADCAST	(1<<(int)Pf_broadcast)
#define	P_CANEXEC	(1<<(int)Pf_canexec)
#define	P_CANRECV	(1<<(int)Pf_canrecv)
#define	P_CANSEND	(1<<(int)Pf_cansend)
#define	P_EXPLICIT	(1<<(int)Pf_explicit)
#define	P_MULTICAST	(1<<(int)Pf_multicast)
#define	P_OTHERHANDLERS	(1<<(int)Pf_otherhandlers)
#define	P_SU		(1<<(int)Pf_su)
#define	P_WORLD		(1<<(int)Pf_world)

#define	P_ALL	(P_BROADCAST|P_CANEXEC|P_CANRECV|P_CANSEND|P_EXPLICIT|P_MULTICAST|P_OTHERHANDLERS|P_SU|P_WORLD)

typedef int		Pflg_t;

/*
**	Structure returned by "passwd" file accessing routines.
*/

typedef struct
{
	char *	P_name;		/* Login name */
	char *	P_shell;	/* Login shell */
	char *	P_dir;		/* Home directory */
	char *	P_rname;	/* Real user name */
	char *	P_region;	/* Address restriction */
	Pflg_t	P_flags;	/* Validation bits */
	int	P_uid;		/* System uid */
	int	P_gid;		/* System gid */
	char	P_crypt[16];	/* Encrypted password */
}
			Passwd;

#define	P_error		P_name
#define	P_user		P_name

/*
**	Global network user details setup by ``GetNetUid()''.
*/

extern Passwd	NetPasswd;

#define	NetUid	NetPasswd.P_uid
#define	NetGid	NetPasswd.P_gid

#endif	/* P_BROADCAST */

#ifdef	PFLAGS_DATA

/*
**	Table to match validation flags.
*/

typedef struct
{
	char *	pf_name;
	Pflg_t	pf_bit;
}
			PFlag;

#define	PFLAGZ		sizeof(PFlag)

/*
**	english( Sort this array. )
*/

static PFlag		PFlags[] =
{
	{english("broadcast"),		P_BROADCAST},
	{english("exec"),		P_CANEXEC},
	{english("explicit"),		P_EXPLICIT},
	{english("multicast"),		P_MULTICAST},
	{english("otherhandlers"),	P_OTHERHANDLERS},
	{english("receive"),		P_CANRECV},
	{english("send"),		P_CANSEND},
	{english("su"),			P_SU},
	{english("world"),		P_WORLD}
};

#define	NFLAGS		((sizeof PFlags)/PFLAGZ)

#endif	/* PFLAGS_DATA */

/*
**	"passwd" file accessing routines
*/

extern void		FreeUid _FA_((Passwd *));
extern void		GetInvoker _FA_((Passwd *));
extern void		GetNetUid _FA_((void));
extern void		GetPrivs _FA_((Passwd *));
extern bool		GetUid _FA_((Passwd *, char *));
extern bool		GetUser _FA_((Passwd *, int));

/*
**	"_call/passwd" file accessing routines
*/

Extern char *		PasswdData;
Extern char *		PasswdEnd;
Extern char *		PasswdFile;
Extern Ulong		PasswdLength;
Extern char *		PasswdPoint;
Extern char *		PasswdStart;
Extern char *		PasswdTail;

extern bool		GetPassword _FA_((Passwd *, char *));
