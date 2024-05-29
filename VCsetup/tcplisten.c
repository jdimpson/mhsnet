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


static char	sccsid[]	= "@(#)tcplisten.c	1.19 06/03/16";

/*
**	TCP/IP listener for MHSnet connections.
*/

#define	WAIT_CALL
#define	FILE_CONTROL
#define	SIGNALS
#define	STDIO
#define	ERRNO

#include	"global.h"
#include	"Args.h"
#undef	OPTVAL
#define	ARGOPTVAL	0002	/* KLUDGE - OPTVAL is defined in socket.h! */
#include	"debug.h"
#include	"params.h"
#include	"spool.h"
#include	"sysexits.h"

#if	UDP_IP == 1 || TCP_IP == 1
#ifdef	TWG_IP
#include	<sys/twg_config.h>
#endif

#ifndef	_TYPES_
#include	<sys/types.h>
#endif

#if	EXCELAN == 1
#include	<sys/socket.h>
#include	<netinet/in.h>
#else	/* EXCELAN == 1 */
#if	BSD4 >= 2 || BSD_IP == 1
#ifdef	M_XENIX
#ifndef	scounix
#include	<sys/types.tcp.h>
#endif
#endif	/* M_XENIX */
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>
#else	/* BSD4 >= 2 || BSD_IP == 1 */
#include	<socket.h>
#include	<in.h>
#include	<netdb.h>
#endif	/* BSD4 >= 2 || BSD_IP == 1 */
#endif	/* UDP_IP == 1 || TCP_IP == 1 */
#endif	/* EXCELAN == 1 */

#include <setjmp.h>


/*
**	Defaults:
*/

#define	SERVICE		"mhsnet"
#define	SERVER		"tcpshell"
#define	PORT		1989	/* Byte swapped looks like this: 50439 */
#define	BACKLOG		5	/* ``listen(2)'' parameter */

/*
**	Variables set from program arguments
*/

char *	LogName;		/* Log file [default: LIBDIR/tcplisten.log] */
char *	Name = sccsid;		/* Program invoked name */
bool	NoFork;			/* Don't fork off parent */
int	Port = PORT;		/* IP port number for ``Service'' */
char *	Server;			/* Path name of server program for ``Service'' */
char *	Service = SERVICE;	/* Name of TCP service */
int	Traceflag;		/* Debug trace level */

Args	Usage[] =
{
        Arg_0(0, getName),
	Arg_bool(f, &NoFork, 0),
	Arg_string(L, &LogName, 0, english("log file name"), OPTARG),
	Arg_string(N, &Service, 0, english("TCP service name"), OPTARG),
	Arg_int(P, &Port, 0, english("IP port number"), OPTARG),
	Arg_string(S, &Server, 0, english("server program"), OPTARG),
	Arg_int(T, &Traceflag, getInt1, english("level"), OPTARG|ARGOPTVAL),
	Arg_end
};

/*
**	Misc.
*/

#if	UDP_IP == 1 || TCP_IP == 1
struct sockaddr_in	Ctrladdr;
#endif	/* UDP_IP == 1 || TCP_IP == 1 */

bool	ExInChild;
int	Fd;
char *	LockName;
bool	LockSet;
jmp_buf	WaitJmp;

/*
**	Configurable parameters.
*/

ParTb	Params[] =
{
	{"PORT",	(char **)&Port,		PINT},
	{"SERVER",	&Server,		PSPOOL},
	{"SERVICE",	&Service,		PSTRING},
	{"TRACEFLAG",	(char **)&Traceflag,	PINT},
};

void	process _FA_((void));
void	start _FA_((void));
void	waitchild _FA_((void));

extern SigFunc	catch;
extern SigFunc	catchterm;



int
main(argc, argv)
	int	argc;
	char *	argv[];
{
	InitParams();

	Server = concat(SPOOLDIR, LIBDIR, SERVER, NULLSTR);

	DoArgs(argc, argv, Usage);

	GetParams(Name, Params, sizeof Params);

#	if	UDP_IP == 1 || TCP_IP == 1

	Pid = Detach(NoFork, 0, false, true);

	if ( geteuid() != 0 )
		Error(english("No permission."));

	while ( chdir(SPOOLDIR) == SYSERROR )
		Syserror(CouldNot, "chdir", SPOOLDIR);

	LockName = concat(LIBDIR, Name, ".", LOCKFILE, NULLSTR);

	if ( !SetLock(LockName, Pid, false) )
	{
		Warn(english("%s (%d) already running on %s."), Name, LockPid, LockNode);
		finish(EX_UNAVAILABLE);
	}

	LockSet = true;

	if ( LogName == NULLSTR )
		LogName = concat(LIBDIR, Name, ".", LOGFILE, NULLSTR);

	StderrLog(LogName);
	(void)fclose(stdin);
	(void)fclose(stdout);

	(void)signal(SIGTERM, catchterm);

	Report3("Vn=\"%s\" [%d] STARTED", Version, Pid);

	for ( ;; )	/* For EXCELAN, otherwise irrelevant */
	{
		start();

		Trace4(1, "Socket %d set up for %s/tcp/%d", Fd, Service, Port);

		process();
	}

#	else	/* UDP_IP == 1 || TCP_IP == 1 */

	Error("IP unavailable.");
	exit(EX_UNAVAILABLE);

#	endif	/* UDP_IP == 1 || TCP_IP == 1 */
	return 0;
}



void
catch(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	longjmp(WaitJmp, 1);
}



void
catchterm(sig)
	int	sig;
{
	(void)signal(sig, SIG_IGN);
	finish(EX_OK);
}



void
finish(err)
	int	err;
{
#	if	UDP_IP == 1 || TCP_IP == 1
	if ( !ExInChild && LockSet )
		(void)unlink(LockName);

	Report3("Vn=\"%s\" [%d] FINISHED", Version, Pid);
#	endif	/* UDP_IP == 1 || TCP_IP == 1 */
	exit(err);
}



#if	UDP_IP == 1 || TCP_IP == 1

void
process()
{
	for (;;)
	{
		register int		i;
		register int		ctrl;
		struct sockaddr_in	his_addr;
		socklen_t		hisaddrlen = sizeof(his_addr);
		char			addrbuf[32];

		waitchild();

#		if	EXCELAN == 1
		ctrl = accept(Fd, (struct sockaddr *)&his_addr);
#		else	/* EXCELAN == 1 */
		ctrl = accept(Fd, (struct sockaddr *)&his_addr, &hisaddrlen);
#		endif	/* EXCELAN == 1 */

		Trace2(1, "accept, fd %d\n", ctrl);

		if ( ctrl == SYSERROR )
		{
			Syserror("accept");
			continue;
		}

#		if	EXCELAN == 1
		ctrl = Fd;
#		endif	/* EXCELAN == 1 */

		switch ( fork() )
		{
		case SYSERROR:
			Syserror("fork");
			(void)close(ctrl);
#			if	EXCELAN == 1
			return;	/* Need new ``Fd'' */
#			else	/* EXCELAN == 1 */
			continue;
#			endif	/* EXCELAN == 1 */

		case 0:
			ExInChild = true;
			break;

		default:
			(void)close(ctrl);
#			if	EXCELAN == 1
			return;	/* Need new ``Fd'' */
#			else	/* EXCELAN == 1 */
			continue;
#			endif	/* EXCELAN == 1 */
		}

		(void)sprintf
		(
			addrbuf, "%lx.%d",
		 	(PUlong)ntohl(his_addr.sin_addr.s_addr),
			ntohs(his_addr.sin_port)
		);

		if ( ctrl != 0 )
		{
			(void)close(0);
			(void)dup(ctrl);
			(void)close(ctrl);
		}

		(void)close(1);
		(void)dup(0);

		for ( i=3 ; close(i) != SYSERROR ; i++ );

		Report4("Exec: pid %d \"%s %s\"\n", getpid(), Server, addrbuf);

		for ( ;; )
		{
			(void)execl(Server, strrchr(Server, '/')+1, addrbuf, NULLSTR);
			Syserror(CouldNot, "execl", Server);
		}
	}
}



void
start()
{
	Ctrladdr.sin_family = AF_INET;
	Ctrladdr.sin_addr.s_addr = INADDR_ANY;
	Ctrladdr.sin_port = htons((Ushort)Port);

#	if	EXCELAN == 1
	while ( (Fd = socket(SOCK_STREAM, (struct sockproto *)0, &Ctrladdr, SO_ACCEPTCONN|SO_REUSEADDR)) == SYSERROR )
#	else	/* EXCELAN == 1 */
	while ( (Fd = socket(AF_INET, SOCK_STREAM, 0)) == SYSERROR )
#	endif	/* EXCELAN == 1 */
		Syserror(english("Could not create socket for %s/tcp/%d"), Service, Port);

#	if	EXCELAN == 0
	/*
	**	``setsockopt'' doesn't work on some IBM RT PCs.
	*/

	if ( Traceflag > 1 && setsockopt(Fd, SOL_SOCKET, SO_DEBUG, 0, 0) == SYSERROR )
		SysWarn(english("Could not set SO_DEBUG for %s/tcp/%d"), Service, Port);

	if ( setsockopt(Fd, SOL_SOCKET, SO_REUSEADDR, 0, 0) == SYSERROR )
		SysWarn(english("Could not set SO_REUSEADDR for %s/tcp/%d"), Service, Port);

	if ( bind(Fd, (struct sockaddr *)&Ctrladdr, sizeof(Ctrladdr)) == SYSERROR )
		Syserror(english("Could not bind socket for %s/tcp/%d"), Service, Port);

	if ( listen(Fd, BACKLOG) == SYSERROR )
		Syserror(english("Could not listen to socket for %s/tcp/%d"), Service, Port);
#	endif	/* EXCELAN == 0 */
}



void
waitchild()
{
	register int	pid;
	int		status;

	if ( setjmp(WaitJmp) )
		return;

	(void)signal(SIGALRM, catch);
	(void)alarm((unsigned)2);

	for ( ;; )
	{
		pid = wait(&status);

		(void)alarm((unsigned)0);

		if ( pid == SYSERROR )
			return;

		Trace2(1, "%d exited\n", pid);
	}
}

#endif	/* UDP_IP == 1 || TCP_IP == 1 */
