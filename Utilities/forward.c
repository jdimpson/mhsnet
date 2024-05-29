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


static char	sccsid[]	= "@(#)forward.c	1.22 05/12/16";

/*
**	Set message forwarding for a user.
**
**	SETUID ==> NETUSER
*/

#define	FILE_CONTROL
#define	STAT_CALL
#define	RECOVER
#define	STDIO

#include	"global.h"
#include	"address.h"
#include	"Args.h"
#include	"debug.h"
#include	"forward.h"
#include	"ftheader.h"
#include	"handlers.h"
#include	"link.h"
#include	"ndir.h"
#include	"Passwd.h"
#include	"routefile.h"
#include	"spool.h"
#include	"sub_proto.h"
#include	"sysexits.h"

#undef	Extern
#define	Extern
#define	ExternU
#include	"exec.h"		/* For ExInChild */


/*
**	Parameters set from arguments.
*/

bool	AllUsers;		/* Show all forwarding (root only) */
bool	Clear;			/* Clear forwarding */
char *	Name	= sccsid;	/* Program invoked name */
int	Traceflag;		/* Global tracing control */
char *	UserName;		/* User's name for files to be retrieved (root only) */

AFuncv	getForward _FA_((PassVal, Pointer));	/* Set forward addresses */
AFuncv	getHandler _FA_((PassVal, Pointer));	/* Set different handler */

Args	Usage[] =
{
	Arg_0(0, getName),
	Arg_bool(a, &AllUsers, 0),
	Arg_bool(c, &Clear, 0),
	Arg_string(A, 0, getHandler, english("handler"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|OPTVAL),
	Arg_string(U, &UserName, 0, english("user"), OPTARG),
	Arg_noflag(0, getForward, english("address"), OPTARG|MANY),
	Arg_end
};

/*
**	Structure for sorting forward files.
*/

typedef struct FwdFile
{
	struct FwdFile *next;
	char *		name;
}
	FwdFile;

/*
**	Miscellaneous
*/

char	Ambiguous[]	= english("Ambiguous arguments.");
Addr *	ForwAddr;		/* Address for forwarding messages */
char *	ForwDir;		/* Directory for spooling messages */
Handler *MsgHandler;		/* Message handler */
bool	NewForwd;		/* Change forwarding */
bool	NewHandler;		/* `MsgHandler' set */
Passwd	To;			/* Details of invoker */

#define	Fprintf		(void)fprintf
#define	Fflush		(void)fflush


void	set_forward _FA_((void));
void	show_all _FA_((void));
void	show_forward _FA_((char *));
void	show_handler _FA_((Forward *, bool, char *));



int
main(argc, argv)
	int		argc;
	char *		argv[];
{
	InitParams();

	GetNetUid();
	GetInvoker(&To);

	if ( !(To.P_flags & P_CANSEND) )
		Error(english("no permission to forward network messages"));

	if ( !ReadRoute(SUMCHECK) )
		Error(english("No routefile."));

	if ( (MsgHandler = GetHandler(MAILHANDLER)) == (Handler *)0 )
		Error(english("Default handler \"%s\" unknown!"), MAILHANDLER);

	DoArgs(argc, argv, Usage);

	if ( Clear )
	{
		if ( NewForwd )
			Error(Ambiguous);

		NewForwd = true;
	}

	if ( UserName != NULLSTR )
	{
		if ( !(To.P_flags & P_SU) )
			Error(english("no permission to use \"-U\" flag"));

		/*
		**	Don't really care if UserName exists or not,
		**	so use invoker's privs.
		*/
		To.P_name = ToLower(UserName, strlen(UserName));
	}

	setbuf(stdout, SObuf);

	if ( AllUsers )
	{
		if ( !(To.P_flags & P_SU) )
			Error(english("no permission to use \"-a\" flag"));

		if ( UserName != NULLSTR || NewForwd )
			Error(Ambiguous);

		show_all();
	}
	else
	if ( NewForwd )
	{
		if ( ForwAddr != (Addr *)0 && ForwDir != NULLSTR )
			Error(Ambiguous);

		set_forward();
	}
	else
		show_forward(To.P_name);

	Fflush(stdout);
	finish(EX_OK);
	return 0;
}



/*
**	Match name in FthUsers.
*/

bool
checkusers(name)
	char *			name;
{
	register FthUlist *	up;

	Trace2(1, "checkusers(%s)", name);

	if ( MsgHandler->proto_type != FTP )
		return true;

	for ( up = FthUsers ; up != (FthUlist *)0 ; up = up->u_next )
		if ( strcmp(name, up->u_name) == STREQUAL )
			return true;

	return false;
}



/*
**	Called from the errors routines to cleanup
*/

void
finish(error)
	int	error;
{
	(void)exit(error);
}



/*
**	Get forwarding address.
*/

/*ARGSUSED*/
AFuncv
getForward(val, arg)
	PassVal		val;
	Pointer		arg;
{
	register Addr *	ap;

	if ( val.p[0] == '/' )
	{
		if ( ForwDir != NULLSTR )
			return (AFuncv)english("only one spooling directory specifiable");

		ForwDir = val.p;
	}
	else
	{
		if ( MsgHandler->proto_type == FTP )
		{
			if ( (ap = AddrFromUser(val.p)) == (Addr *)0 )
				return (AFuncv)english("invalid address");
		}
		else
			ap = StringToAddr(val.p, NO_STA_MAP);

		AddAddr(&ForwAddr, ap);
	}

	NewForwd = true;
	return ACCEPTARG;
}



/*
**	Set different handler.
*/

/*ARGSUSED*/
AFuncv
getHandler(val, arg)
	PassVal		val;
	Pointer		arg;
{
	static Handler	own_handler;
	static char	unkhandler[] = english("unknown handler");

	if ( NewForwd )
		return (AFuncv)english("handler argument must come first");

	if
	(
		(MsgHandler = GetHandler(val.p)) == (Handler *)0
		&&
		(MsgHandler = GetDHandler(val.p)) == (Handler *)0
	)
	{
		if ( !(To.P_flags & P_SU) )
			return (AFuncv)unkhandler;

		Warn("%s: %s", unkhandler, val.p);
		own_handler.handler = newstr(val.p);
		own_handler.proto_type = UNK_PROTO;
		MsgHandler = &own_handler;
	}

	NewHandler = true;

	return ACCEPTARG;
}



/*
**	Compare two forward file names.
*/

int
listcmp(fp1, fp2)
	char *		fp1;
	char *		fp2;
{
	return strccmp(((FwdFile *)fp1)->name, ((FwdFile *)fp2)->name);
}



/*
**	Check and set forwarding address.
*/

void
set_forward()
{
	if ( Clear )
	{
		Trace2(1, "set_forward() %s", NullStr);
		SetForward(MsgHandler->handler, To.P_name, NULLSTR, NULLSTR);
		return;
	}

	if ( ForwDir != NULLSTR )
	{
		struct stat	statb;

		if ( access(ForwDir, 2) == SYSERROR )
			Syserror(CouldNot, WriteStr, ForwDir);

		if ( stat(ForwDir, &statb) == SYSERROR )
			Syserror(CouldNot, StatStr, ForwDir);

		if ( (statb.st_mode & S_IFMT) != S_IFDIR )
			Error(english("%s: not a directory."), ForwDir);

		Error(english("Directory spooling disabled."));	/** NEEDS FIXES AND CHECKS TO filer **/

		SetForward(MsgHandler->handler, To.P_name, ForwDir, NULLSTR);
		return;
	}

	Trace2(1, "set_forward() \"%s\"", UnTypAddress(ForwAddr));

	(void)CheckAddr(&ForwAddr, &To, Error, true);

	if ( AddrAtHome(&ForwAddr, false, false) && checkusers(To.P_name) )
		Error(english("Forwarding loop."));

	if ( MsgHandler->proto_type != FTP )
		FthTo = UnTypAddress(ForwAddr);
	else
		SetFthTo();

	SetForward(MsgHandler->handler, To.P_name, TypedAddress(ForwAddr), FthTo);

	FreeAddr(&ForwAddr);
}



/*
**	Show forwarding addresses for all users.
*/

void
show_all()
{
	register FwdFile **	ffpp;
	register FwdFile *	ffp;
	register DIR *		dirp;
	register struct direct *dp;
	char *			fwd;
	FwdFile *		fwdlist;

	fwd = concat(SPOOLDIR, FORWARDINGDIR, NULLSTR);

	if ( (dirp = opendir(fwd)) == NULL )
	{
		free(fwd);
		return;
	}

	ffpp = &fwdlist;

	while ( (dp = readdir(dirp)) != NULL )
	{
		if
		(
			dp->d_name[0] == '.'
			&&
			(
				dp->d_name[1] == '\0'
				||
				(dp->d_name[1] == '.' && dp->d_name[2] == '\0')
			)
		)
			continue;

		ffp = Talloc(FwdFile);
		ffp->name = newstr(dp->d_name);

		*ffpp = ffp;
		ffpp = &ffp->next;
	}

	*ffpp = (FwdFile *)0;

	closedir(dirp);

	listsort((char **)&fwdlist, listcmp);

	for ( ffp = fwdlist ; ffp != (FwdFile *)0 ; ffp = fwdlist )
	{
		show_forward(ffp->name);
		fwdlist = ffp->next;
		free(ffp->name);
		free((char *)ffp);
	}

	free(fwd);
}



/*
**	Show forwarding addresses for a user.
*/

void
show_forward(user)
	char *			user;
{
	register Forward *	fp;
	register int		i;
	register bool		own;

	fp = GetForward(MsgHandler->handler, user);

	if ( AllUsers || UserName != NULLSTR )
		own = false;
	else
	{
		own = true;
		user = EmptyString;
	}

	if ( NewHandler )
	{
		if ( fp != (Forward *)0 )
			show_handler(fp, own, user);

		return;
	}

	for ( fp = Forwds, i = ForwdCount ; --i >= 0 ; fp++ )
		show_handler(fp, own, user);
}



/*
**	Show forwarding for a handler.
*/

void
show_handler(fp, own, user)
	register Forward *	fp;
	bool			own;
	char *			user;
{
	register Handler *	handler;
	register char *		type;
	register char *		op;
	register char *		cp;

	if ( strcmp(fp->handler, MAILHANDLER) == STREQUAL )
		type = english("mail is");
	else
	if ( strcmp(fp->handler, FTPHANDLER) == STREQUAL )
		type = english("files are");
	else
	{
		type = english("messages are");

		if ( (handler = GetHandler(fp->handler)) != (Handler *)0 )
			type = concat(handler->descrip, " ", type, NULLSTR);
	}

	if ( own )
		op = english("Your ");
	else
		op = ":\t";

	if ( (cp = fp->address) != NULLSTR )
	{
		if ( fp->sub_addr != NULLSTR )
			cp = fp->sub_addr;

		Fprintf(stdout, english("%s%s%s being forwarded to %s\n"), user, op, type, cp);
	}
	else
	if ( own )
		Fprintf(stdout, english("%s%s%s not being forwarded.\n"), user, op, type);
}
