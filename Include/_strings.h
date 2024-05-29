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
**	String definitions.
*/

#include	"strings-data.h"

#ifdef	BADDIR
#undef	BADDIR
#define	BADDIR		BaddirStr
extern char *		BaddirStr;
#endif	/* BADDIR */

#ifdef	BINMAIL
#undef	BINMAIL
#define	BINMAIL		BinmailStr
extern char *		BinmailStr;
#endif	/* BINMAIL */

#ifdef	BINMAILARGS
#undef	BINMAILARGS
#define	BINMAILARGS	BinmlargsStr
extern char **		BinmlargsStr;
#endif	/* BINMAILARGS */

#ifdef	CALLDIR
#undef	CALLDIR
#define	CALLDIR	CalldirStr
extern char *		CalldirStr;
#endif	/* CALLDIR */

#ifdef	EXPLAINDIR
#undef	EXPLAINDIR
#define	EXPLAINDIR	ExplndirStr
extern char *		ExplndirStr;
#endif	/* EXPLAINDIR */

#ifdef	FILESDIR
#undef	FILESDIR
#define	FILESDIR	FilesdirStr
extern char *		FilesdirStr;
#endif	/* FILESDIR */

#ifdef	FILESERVERLOG
#undef	FILESERVERLOG
#define	FILESERVERLOG	FileserverlogStr
extern char *		FileserverlogStr;
#endif	/* FILESERVERLOG */

#ifdef	FORWARDINGDIR
#undef	FORWARDINGDIR
#define	FORWARDINGDIR	FwdingdirStr
extern char *		FwdingdirStr;
#endif	/* FORWARDINGDIR */

#ifdef	GETFILE
#undef	GETFILE
#define	GETFILE		GetfileStr
extern char *		GetfileStr;
#endif	/* GETFILE */

#ifdef	HANDLERSDIR
#undef	HANDLERSDIR
#define	HANDLERSDIR	HandlrsdirStr
extern char *		HandlrsdirStr;
#endif	/* HANDLERSDIR */

#ifdef	LIBDIR
#undef	LIBDIR
#define	LIBDIR		LibdirStr
extern char *		LibdirStr;
#endif	/* LIBDIR */

#ifdef	LOCAL_NODES
#undef	LOCAL_NODES
#define	LOCAL_NODES	LocalnodesStr
extern char *		LocalnodesStr;
#endif	/* LOCAL_NODES */

#ifdef	LOCALSEND
#undef	LOCALSEND
#define	LOCALSEND	LocalSend
extern int		LocalSend;
#endif	/* LOCALSEND */

#ifdef	MAIL_RFC822_HDR
#undef	MAIL_RFC822_HDR
#define	MAIL_RFC822_HDR	MailRFC822Hdr
extern int		MailRFC822Hdr;
#endif	/* MAIL_RFC822_HDR */

#ifdef	MAIL_TO
#undef	MAIL_TO
#define	MAIL_TO		MailTo
extern int		MailTo;
#endif	/* MAIL_TO */

#ifdef	MAILER
#undef	MAILER
#define	MAILER		MailerStr
extern char *		MailerStr;
#endif	/* MAILER */

#ifdef	MAILERARGS
#undef	MAILERARGS
#define	MAILERARGS	MailerargsStr
extern char **		MailerargsStr;
#endif	/* MAILERARGS */

#ifdef	MAX_MESG_DATA
#undef	MAX_MESG_DATA
#define	MAX_MESG_DATA	MaxMesgData
extern Ulong		MaxMesgData;
#endif	/* MAX_MESG_DATA */

#ifdef	MINSPOOLFSFREE
#undef	MINSPOOLFSFREE
#define	MINSPOOLFSFREE	MinSpoolFSFree
extern Ulong		MinSpoolFSFree;
#endif	/* MINSPOOLFSFREE */

#ifdef	MKDIR
#undef	MKDIR
#define	MKDIR		MkdirStr
extern char *		MkdirStr;
#endif	/* MKDIR */

#ifdef	MPMSGSDIR
#undef	MPMSGSDIR
#define	MPMSGSDIR	MpmsgsdirStr
extern char *		MpmsgsdirStr;
#endif	/* MPMSGSDIR */

#ifdef	NCC_ADMIN
#undef	NCC_ADMIN
#define	NCC_ADMIN	NccadminStr
extern char *		NccadminStr;
#endif	/* NCC_ADMIN */

#ifdef	NCC_MANAGER
#undef	NCC_MANAGER
#define	NCC_MANAGER	NccmanagerStr
extern char *		NccmanagerStr;
#endif	/* NCC_MANAGER */

#ifdef	NETADMIN
#undef	NETADMIN
#define	NETADMIN	NetAdmin
extern int		NetAdmin;
#endif	/* NETADMIN */

#ifdef	NETGROUPNAME
#undef	NETGROUPNAME
#define	NETGROUPNAME	NetgroupnameStr
extern char *		NetgroupnameStr;
#endif	/* NETGROUPNAME */

#ifdef	NETPARAMS
#undef	NETPARAMS
#define	NETPARAMS	NetparamsStr
extern char *		NetparamsStr;
#endif	/* NETPARAMS */

#ifdef	NETUSERNAME
#undef	NETUSERNAME
#define	NETUSERNAME	NetusernameStr
extern char *		NetusernameStr;
#endif	/* NETUSERNAME */

#ifdef	NEWSARGS
#undef	NEWSARGS
#define	NEWSARGS	NewsargsStr
extern char **		NewsargsStr;
#endif	/* NEWSARGS */

#ifdef	NEWSCMDS
#undef	NEWSCMDS
#define	NEWSCMDS	NewscmdsStr
extern char *		NewscmdsStr;
#endif	/* NEWSCMDS */

#ifdef	NEWSEDITOR
#undef	NEWSEDITOR
#define	NEWSEDITOR	NewseditorStr
extern char *		NewseditorStr;
#endif	/* NEWSEDITOR */

#ifdef	NICEDAEMON
#undef	NICEDAEMON
#define	NICEDAEMON	NiceDaemon
extern int		NiceDaemon;
#endif	/* NICEDAEMON */

#ifdef	NICEHANDLERS
#undef	NICEHANDLERS
#define	NICEHANDLERS	NiceHandlers
extern int		NiceHandlers;
#endif	/* NICEHANDLERS */

#ifdef	NOADDRCOMPL
#undef	NOADDRCOMPL
#define	NOADDRCOMPL	NoAddressCompletion
extern int		NoAddressCompletion;
#endif	/* NOADDRCOMPL */

#ifdef	NODENAMEFILE
#undef	NODENAMEFILE
#define	NODENAMEFILE	NodenamefileStr
extern char *		NodenamefileStr;
#endif	/* NODENAMEFILE */

#ifdef	NVETIMECHANGE
#undef	NVETIMECHANGE
#define	NVETIMECHANGE	NveChangeMax
extern int		NveChangeMax;
#endif	/* NVETIMECHANGE */

#ifdef	PARAMSDIR
#undef	PARAMSDIR
#define	PARAMSDIR	ParamsdirStr
extern char *		ParamsdirStr;
#endif	/* PARAMSDIR */

#ifdef	PASSWDSORT
#undef	PASSWDSORT
#define	PASSWDSORT	PasswdSortStr
extern char *		PasswdSortStr;
#endif	/* PASSWDSORT */

#ifdef	PENDINGDIR
#undef	PENDINGDIR
#define	PENDINGDIR	PendngdirStr
extern char *		PendngdirStr;
#endif	/* PENDINGDIR */

#ifdef	POSTMASTER
#undef	POSTMASTER
#define	POSTMASTER	PostmasterStr
extern char *		PostmasterStr;
#endif	/* POSTMASTER */

#ifdef	POSTMASTERNAME
#undef	POSTMASTERNAME
#define	POSTMASTERNAME	PstmstrNameStr
extern char *		PstmstrNameStr;
#endif	/* POSTMASTERNAME */

#ifdef	PRINTER
#undef	PRINTER
#define	PRINTER		PrinterStr
extern char *		PrinterStr;
#endif	/* PRINTER */

#ifdef	PRINTERARGS
#undef	PRINTERARGS
#define	PRINTERARGS	PrintrargsStr
extern char **		PrintrargsStr;
#endif	/* PRINTERARGS */

#ifdef	PRINTORIGINS
#undef	PRINTORIGINS
#define	PRINTORIGINS	PrintoriginsStr
extern char *		PrintoriginsStr;
#endif	/* PRINTORIGINS */

#ifdef	PRIVSFILE
#undef	PRIVSFILE
#define	PRIVSFILE	PrivsfileStr
extern char *		PrivsfileStr;
#endif	/* PRIVSFILE */

#ifdef	PUBLICFILES
#undef	PUBLICFILES
#define	PUBLICFILES	PublicfilesStr
extern char *		PublicfilesStr;
#endif	/* PUBLICFILES */

#ifdef	REROUTEDIR
#undef	REROUTEDIR
#define	REROUTEDIR	ReroutdirStr
extern char *		ReroutdirStr;
#endif	/* REROUTEDIR */

#ifdef	RMDIR
#undef	RMDIR
#define	RMDIR		RmdirStr
extern char *		RmdirStr;
#endif	/* RMDIR */

#ifdef	ROUTINGDIR
#undef	ROUTINGDIR
#define	ROUTINGDIR	RoutngdirStr
extern char *		RoutngdirStr;
#endif	/* ROUTINGDIR */

#ifdef	SERVERGROUP
#undef	SERVERGROUP
#define	SERVERGROUP	ServergroupStr
extern char *		ServergroupStr;
#endif	/* SERVERGROUP */

#ifdef	SERVERUSER
#undef	SERVERUSER
#define	SERVERUSER	ServeruserStr
extern char *		ServeruserStr;
#endif	/* SERVERUSER */

#ifdef	SHELL
#undef	SHELL
#define	SHELL		ShellStr
extern char *		ShellStr;
#endif	/* SHELL */

#ifdef	SPOOLDIR
#undef	SPOOLDIR
#define	SPOOLDIR	SpooldirStr
extern char *		SpooldirStr;
#endif	/* SPOOLDIR */

#ifdef	STATE
#undef	STATE
#define	STATE		StateStr
extern char *		StateStr;
#endif	/* STATE */

#ifdef	STATEDIR
#undef	STATEDIR
#define	STATEDIR	StatedirStr
extern char *		StatedirStr;
#endif	/* STATEDIR */

#ifdef	STATERNOTLIST
#undef	STATERNOTLIST
#define	STATERNOTLIST	StaternotlistStr
extern char *		StaternotlistStr;
#endif	/* STATERNOTLIST */

#ifdef	STATSDIR
#undef	STATSDIR
#define	STATSDIR	StatsdirStr
extern char *		StatsdirStr;
#endif	/* STATSDIR */

#ifdef	STOP
#undef	STOP
#define	STOP		StopStr
extern char *		StopStr;
#endif	/* STOP */

#ifdef	STTY
#undef	STTY
#define	STTY		SttyStr
extern char *		SttyStr;
#endif	/* STTY */

#ifdef	SUN3
extern int		Sun3;
#endif	/* SUN3 */

#ifdef	SUN3LIBDIR
#undef	SUN3LIBDIR
#define	SUN3LIBDIR	Sun3libdirStr
extern char *		Sun3libdirStr;
#endif	/* SUN3LIBDIR */

#ifdef	SUN3SPOOLDIR
#undef	SUN3SPOOLDIR
#define	SUN3SPOOLDIR	Sun3spooldirStr
extern char *		Sun3spooldirStr;
#endif	/* SUN3SPOOLDIR */

#ifdef	SUN3STATEP
#undef	SUN3STATEP
#define	SUN3STATEP	Sun3pstateStr
extern char *		Sun3pstateStr;
#endif	/* SUN3STATEP */

#ifdef	SUN3STATER
#undef	SUN3STATER
#define	SUN3STATER	Sun3staterStr
extern char *		Sun3staterStr;
#endif	/* SUN3STATER */

#ifdef	SUN3USERNAME
#undef	SUN3USERNAME
#define	SUN3USERNAME	Sun3usernameStr
extern char *		Sun3usernameStr;
#endif	/* SUN3USERNAME */

#ifdef	SUN3WORKDIR
#undef	SUN3WORKDIR
#define	SUN3WORKDIR	Sun3workdirStr
extern char *		Sun3workdirStr;
#endif	/* SUN3WORKDIR */

#ifdef	TMPDIR
#undef	TMPDIR
#define	TMPDIR		TmpdirStr
extern char *		TmpdirStr;
#endif	/* TMPDIR */

#ifdef	TRACEDIR
#undef	TRACEDIR
#define	TRACEDIR	TracedirStr
extern char *		TracedirStr;
#endif	/* TRACEDIR */

#ifdef	UUCPLCKDIR
#undef	UUCPLCKDIR
#define	UUCPLCKDIR	UUCPdirStr
extern char *		UUCPdirStr;
#endif	/* UUCPLCKDIR */

#ifdef	UUCPLCKPRE
#undef	UUCPLCKPRE
#define	UUCPLCKPRE	UUCPpreStr
extern char *		UUCPpreStr;
#endif	/* UUCPLCKPRE */

#ifdef	UUCPLOCKS
#undef	UUCPLOCKS
#define	UUCPLOCKS	UUCP_Locks
extern int		UUCP_Locks;
#endif	/* UUCPLOCKS */

#ifdef	WHOISARGS
#undef	WHOISARGS
#define	WHOISARGS	WhoisargsStr
extern char **		WhoisargsStr;
#endif	/* WHOISARGS */

#ifdef	WHOISFILE
#undef	WHOISFILE
#define	WHOISFILE	WhoisfileStr
extern char *		WhoisfileStr;
#endif	/* WHOISFILE */

#ifdef	WHOISPROG
#undef	WHOISPROG
#define	WHOISPROG	WhoisprogStr
extern char *		WhoisprogStr;
#endif	/* WHOISPROG */

#ifdef	WORKDIR
#undef	WORKDIR
#define	WORKDIR		WorkdirStr
extern char *		WorkdirStr;
#endif	/* WORKDIR */

#ifdef	WORKFLAG
#undef	WORKFLAG
#define	WORKFLAG	WorkflagStr[0]
extern char *		WorkflagStr;
#endif	/* WORKFLAG */
