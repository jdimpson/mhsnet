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


#include	"site.h"
#include	"strings-data.h"

#define	english(S)	S
#define	NULLSTR		(char *)0

/*
**	Server strings data.
*/

char *	FileserverlogStr	= FILESERVERLOG;
char *	GetfileStr		= GETFILE;
char *	PublicfilesStr		= PUBLICFILES;
char *	ServergroupStr		= SERVERGROUP;
char *	StaternotlistStr	= STATERNOTLIST;

static char *	A_Mailerargs[]	= {MAILERARGS,NULLSTR};
char **	MailerargsStr		= A_Mailerargs;
char *	MailerStr		= MAILER;

static char *	A_Newsargs[]	= {NEWSARGS,NULLSTR};
char **	NewsargsStr		= A_Newsargs;
char *	NewscmdsStr		= NEWSCMDS;
char *	NewseditorStr		= NEWSEDITOR;

char *	PrinterStr		= PRINTER;
static char *	A_Printrargs[]	= {PRINTERARGS,NULLSTR};
char **	PrintrargsStr		= A_Printrargs;
char *	PrintoriginsStr		= PRINTORIGINS;

static char *	A_Whoisargs[]	= {WHOISARGS,NULLSTR};
char **	WhoisargsStr		= A_Whoisargs;
char *	WhoisfileStr		= WHOISFILE;
char *	WhoisprogStr		= WHOISPROG;

#ifdef	SUN3SPOOLDIR
char *	Sun3libdirStr		= SUN3LIBDIR;
char *	Sun3pstateStr		= SUN3STATEP;
char *	Sun3spooldirStr		= SUN3SPOOLDIR;
char *	Sun3staterStr		= SUN3STATER;
char *	Sun3usernameStr		= SUN3USERNAME;
char *	Sun3workdirStr		= SUN3WORKDIR;
#endif	/* SUN3SPOOLDIR */
