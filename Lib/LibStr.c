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


#include	<time.h>

#include	"site.h"
#include	"typedefs.h"
#include	"strings-data.h"



#define	english(S)	S
#define	NULLSTR		(char *)0

/*
**	Parameters.
*/

char *	BaddirStr		= BADDIR;
char *	BinmailStr		= BINMAIL;
static char *	A_Binmlargs[]	= {BINMAILARGS,NULLSTR};
char **	BinmlargsStr		= A_Binmlargs;
char *	CalldirStr		= CALLDIR;
char *	ExplndirStr		= EXPLAINDIR;
char *	FilesdirStr		= FILESDIR;
char *	FwdingdirStr		= FORWARDINGDIR;
char *	LibdirStr		= LIBDIR;
char *	LocalnodesStr		= LOCAL_NODES;
int	LocalSend		= LOCALSEND;
int	MailRFC822Hdr		= MAIL_RFC822_HDR;
int	MailTo			= MAIL_TO;
Ulong	MaxMesgData		= MAX_MESG_DATA;
Ulong	MinSpoolFSFree		= MINSPOOLFSFREE;
char *	MkdirStr		= MKDIR;
char *	NccadminStr		= NCC_ADMIN;
char *	NccmanagerStr		= NCC_MANAGER;
int	NetAdmin		= NETADMIN;
char *	NetgroupnameStr		= NETGROUPNAME;
char *	NetparamsStr		= NETPARAMS;
char *	NetusernameStr		= NETUSERNAME;
int	NiceDaemon		= NICEDAEMON;
int	NiceHandlers		= NICEHANDLERS;
int	NoAddressCompletion	= NOADDRCOMPL;
char *	NodenamefileStr		= NODENAMEFILE;
int	NveChangeMax		= NVETIMECHANGE;
char *	ParamsdirStr		= PARAMSDIR;
char *	PasswdSortStr		= PASSWDSORT;
char *	PendngdirStr		= PENDINGDIR;
char *	PostmasterStr		= POSTMASTER;
char *	PrivsfileStr		= PRIVSFILE;
char *	PstmstrNameStr		= POSTMASTERNAME;
char *	ReroutdirStr		= REROUTEDIR;
char *	RmdirStr		= RMDIR;
char *	RoutngdirStr		= ROUTINGDIR;
char *	ServeruserStr		= SERVERUSER;
char *	ShellStr		= SHELL;
char *	SpooldirStr		= SPOOLDIR;
char *	StateStr		= STATE;
char *	StatsdirStr		= STATSDIR;
char *	StopStr			= STOP;
char *	SttyStr			= STTY;
char *	TmpdirStr		= TMPDIR;
char *	TracedirStr		= TRACEDIR;
char *	UUCPdirStr		= UUCPLCKDIR;
char *	UUCPpreStr		= UUCPLCKPRE;
int	UUCP_Locks		= UUCPLOCKS;
char *	WorkdirStr		= WORKDIR;
char *	WorkflagStr		= WORKFLAG;

/*
**	SUN3 specials.
*/

#if	SUN3 == 1
char	Au[]			= "au";
char	OzAu[]			= "oz.au";
int	Sun3			= 0;
#endif	/* SUN3 == 1 */

/*
**	Special cases.
*/

static
char	CopyRight[]		= english("GPL");

char	CouldNot[]		= english("Could not %s \"%s\"");
char	CreateStr[]		= english("create");
char	DevNull[]		= "/dev/null";
char	Dup[]			= english("dup");
char	EmptyString[]		= "";
char	ForkStr[]		= english("fork");
char	LockStr[]		= english("lock");
char	NullStr[]		= english("<null>");
char	OpenStr[]		= english("open");
char	PipeStr[]		= english("pipe");
char	ReadStr[]		= english("read");
char	SeekStr[]		= english("seek");
char	Slash[]			= "/";
char	StatStr[]		= english("stat");
char	StringFmt[]		= "%.1023s";
char	Unknown[]		= english("<unknown>");
char	UnlinkStr[]		= english("unlink");
char	WriteStr[]		= english("write");

/*
**	Globals.
*/

char *	ErrString;


char *
_use_copyright()
{
	return CopyRight;
}
